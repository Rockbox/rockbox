/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "ata.h"
#include "kernel.h"
#include "thread.h"
#include "led.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
#include "hwcompat.h"

/* skip whole file for an MMC-based system, FIXME in makefile */
#ifndef HAVE_MMC 

/* Uncomment the matching #define to use plain C code instead if the tweaked 
 * assembler code for disk reading or writing should cause problems. */
/* #define PREFER_C_READING */
/* #define PREFER_C_WRITING */

#define SECTOR_SIZE     512
#define ATA_DATA        (*((volatile unsigned short*)0x06104100))
#define ATA_ERROR       (*((volatile unsigned char*)0x06100101))
#define ATA_FEATURE     ATA_ERROR
#define ATA_NSECTOR     (*((volatile unsigned char*)0x06100102))
#define ATA_SECTOR      (*((volatile unsigned char*)0x06100103))
#define ATA_LCYL        (*((volatile unsigned char*)0x06100104))
#define ATA_HCYL        (*((volatile unsigned char*)0x06100105))
#define ATA_SELECT      (*((volatile unsigned char*)0x06100106))
#define ATA_COMMAND     (*((volatile unsigned char*)0x06100107))
#define ATA_STATUS      (*((volatile unsigned char*)0x06100107))

#define ATA_CONTROL1    ((volatile unsigned char*)0x06200206)
#define ATA_CONTROL2    ((volatile unsigned char*)0x06200306)

#define ATA_CONTROL     (*ata_control)
#define ATA_ALT_STATUS  ATA_CONTROL

#define SELECT_DEVICE1  0x10
#define SELECT_LBA      0x40

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01

#define ERROR_ABRT      0x04

#define CONTROL_nIEN    0x02
#define CONTROL_SRST    0x04

#define CMD_READ_SECTORS           0x20
#define CMD_WRITE_SECTORS          0x30
#define CMD_READ_MULTIPLE          0xC4
#define CMD_WRITE_MULTIPLE         0xC5
#define CMD_SET_MULTIPLE_MODE      0xC6
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2
#define CMD_IDENTIFY               0xEC
#define CMD_SLEEP                  0xE6
#define CMD_SET_FEATURES           0xEF
#define CMD_SECURITY_FREEZE_LOCK   0xF5

#define Q_SLEEP 0

#define READ_TIMEOUT 5*HZ

static struct mutex ata_mtx;
char ata_device; /* device 0 (master) or 1 (slave) */
int ata_io_address; /* 0x300 or 0x200, only valid on recorder */
static volatile unsigned char* ata_control;

bool old_recorder = false;
int ata_spinup_time = 0;
static bool spinup = false;
static bool sleeping = true;
static int sleep_timeout = 5*HZ;
static bool poweroff = false;
#ifdef HAVE_ATA_POWER_OFF
static int poweroff_timeout = 2*HZ;
#endif
static char ata_stack[DEFAULT_STACK_SIZE];
static const char ata_thread_name[] = "ata";
static struct event_queue ata_queue;
static bool initialized = false;
static bool delayed_write = false;
static unsigned char delayed_sector[SECTOR_SIZE];
static int delayed_sector_num;

static long last_user_activity = -1;
long last_disk_activity = -1;

static int multisectors; /* number of supported multisectors */
static unsigned short identify_info[SECTOR_SIZE];

static int ata_power_on(void);
static int perform_soft_reset(void);
static int set_multiple_mode(int sectors);
static int set_features(void);

static int wait_for_bsy(void) __attribute__ ((section (".icode")));
static int wait_for_bsy(void)
{
    int timeout = current_tick + HZ*30;
    while (TIME_BEFORE(current_tick, timeout) && (ATA_STATUS & STATUS_BSY)) {
        last_disk_activity = current_tick;
        yield();
    }

    if (TIME_BEFORE(current_tick, timeout))
        return 1;
    else
        return 0; /* timeout */
}

static int wait_for_rdy(void) __attribute__ ((section (".icode")));
static int wait_for_rdy(void)
{
    int timeout;
    
    if (!wait_for_bsy())
        return 0;

    timeout = current_tick + HZ*10;

    while (TIME_BEFORE(current_tick, timeout) &&
           !(ATA_ALT_STATUS & STATUS_RDY)) {
        last_disk_activity = current_tick;
        yield();
    }

    if (TIME_BEFORE(current_tick, timeout))
        return STATUS_RDY;
    else
        return 0; /* timeout */
}

static int wait_for_start_of_transfer(void) __attribute__ ((section (".icode")));
static int wait_for_start_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_BSY|STATUS_DRQ)) == STATUS_DRQ;
}

static int wait_for_end_of_transfer(void) __attribute__ ((section (".icode")));
static int wait_for_end_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_RDY|STATUS_DRQ)) == STATUS_RDY;
}    


/* the tight loop of ata_read_sectors(), to avoid the whole in IRAM */
static void copy_read_sectors(unsigned char* buf,
                         int wordcount)
                         __attribute__ ((section (".icode")));
static void copy_read_sectors(unsigned char* buf, int wordcount)
{
#ifdef PREFER_C_READING
    unsigned short tmp = 0;

    if ( (unsigned int)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned char* bufend = buf + wordcount*2;
        do
        {   /* loop compiles to 9 assembler instructions */
            /* takes 14 clock cycles (2 pipeline stalls, 1 wait) */
            tmp = ATA_DATA;
            *buf++ = tmp & 0xff; /* I assume big endian */
            *buf++ = tmp >> 8;   /*  and don't use the SWAB16 macro */
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {   /* loop compiles to 7 assembler instructions */
            /* takes 12 clock cycles (2 pipeline stalls, 1 wait) */
            *wbuf = SWAB16(ATA_DATA);
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
#else
    /* turbo-charged assembler version */
    /* this assumes wordcount to be a multiple of 4 */
    asm (
        "add     %1,%1       \n"  /* wordcount -> bytecount */
        "add     %0,%1       \n"  /* bytecount -> bufend */
        "mov     %0,r0       \n"
        "tst     #1,r0       \n"  /* 16-bit aligned ? */
        "bt      .aligned    \n"  /* yes, do word copy */

        /* not 16-bit aligned */
        "mov     #-1,r3      \n"  /* prepare a bit mask for high byte */
        "shll8   r3          \n"  /* r3 = 0xFFFFFF00 */

        "mov.w   @%2,r2      \n"  /* read first word (1st round) */
        "add     #-12,%1     \n"  /* adjust end address for offsets */
        "mov.b   r2,@%0      \n"  /* store low byte of first word */
        "bra     .start4_b   \n"  /* jump into loop after next instr. */
        "add     #-5,%0      \n"  /* adjust for dest. offsets; now even */

        ".align  2           \n"
    ".loop4_b:               \n"  /* main loop: copy 4 words in a row */
        "mov.w   @%2,r2      \n"  /* read first word (2+ round) */
        "and     r3,r1       \n"  /* get high byte of fourth word (2+ round) */
        "extu.b  r2,r0       \n"  /* get low byte of first word (2+ round) */
        "or      r1,r0       \n"  /* combine with high byte of fourth word */
        "mov.w   r0,@(4,%0)  \n"  /* store at buf[4] */
        "nop                 \n"  /* maintain alignment */
    ".start4_b:              \n"
        "mov.w   @%2,r1      \n"  /* read second word */
        "and     r3,r2       \n"  /* get high byte of first word */
        "extu.b  r1,r0       \n"  /* get low byte of second word */
        "or      r2,r0       \n"  /* combine with high byte of first word */
        "mov.w   r0,@(6,%0)  \n"  /* store at buf[6] */
        "add     #8,%0       \n"  /* buf += 8 */
        "mov.w   @%2,r2      \n"  /* read third word */
        "and     r3,r1       \n"  /* get high byte of second word */
        "extu.b  r2,r0       \n"  /* get low byte of third word */
        "or      r1,r0       \n"  /* combine with high byte of second word */
        "mov.w   r0,@%0      \n"  /* store at buf[0] */
        "cmp/hi  %0,%1       \n"  /* check for end */
        "mov.w   @%2,r1      \n"  /* read fourth word */
        "and     r3,r2       \n"  /* get high byte of third word */
        "extu.b  r1,r0       \n"  /* get low byte of fourth word */
        "or      r2,r0       \n"  /* combine with high byte of third word */
        "mov.w   r0,@(2,%0)  \n"  /* store at buf[2] */
        "bt      .loop4_b    \n"
        /* 24 instructions for 4 copies, takes 30 clock cycles (4 wait) */
        /* avg. 7.5 cycles per word - 86% faster */

        "swap.b  r1,r0       \n"  /* get high byte of last word */
        "bra     .exit       \n"
        "mov.b   r0,@(4,%0)  \n"  /* and store it */

        ".align  2           \n"
        /* 16-bit aligned, loop(read and store word) */
    ".aligned:               \n"
        "mov.w   @%2,r2      \n"  /* read first word (1st round) */
        "add     #-12,%1     \n"  /* adjust end address for offsets */
        "bra     .start4_w   \n"  /* jump into loop after next instr. */
        "add     #-6,%0      \n"  /* adjust for destination offsets */

    ".loop4_w:               \n"  /* main loop: copy 4 words in a row */
        "mov.w   @%2,r2      \n"  /* read first word (2+ round) */
        "swap.b  r1,r0       \n"  /* swap fourth word (2+ round) */
        "mov.w   r0,@(4,%0)  \n"  /* store fourth word (2+ round) */
        "nop                 \n"  /* maintain alignment */
    ".start4_w:              \n"
        "mov.w   @%2,r1      \n"  /* read second word */
        "swap.b  r2,r0       \n"  /* swap first word */
        "mov.w   r0,@(6,%0)  \n"  /* store first word in buf[6] */
        "add     #8,%0       \n"  /* buf += 8 */
        "mov.w   @%2,r2      \n"  /* read third word */
        "swap.b  r1,r0       \n"  /* swap second word */
        "mov.w   r0,@%0      \n"  /* store second word in buf[0] */
        "cmp/hi  %0,%1       \n"  /* check for end */
        "mov.w   @%2,r1      \n"  /* read fourth word */
        "swap.b  r2,r0       \n"  /* swap third word */
        "mov.w   r0,@(2,%0)  \n"  /* store third word */
        "bt      .loop4_w    \n"
        /* 16 instructions for 4 copies, takes 22 clock cycles (4 wait) */
        /* avg. 5.5 cycles per word - 118% faster */

        "swap.b  r1,r0       \n"  /* swap fourth word (last round) */
        "mov.w   r0,@(4,%0)  \n"  /* and store it */

    ".exit:                  \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(buf),
        /* %1 */ "r"(wordcount),
        /* %2 */ "r"(&ATA_DATA)
        : /*trashed */
        "r0","r1","r2","r3"
    );
#endif
}

int ata_read_sectors(unsigned long start,
                     int incount,
                     void* inbuf)
{
    int ret = 0;
    int timeout;
    int count;
    void* buf;
    int spinup_start;

    mutex_lock(&ata_mtx);

    last_disk_activity = current_tick;
    spinup_start = current_tick;

    led(true);

    if ( sleeping ) {
        spinup = true;
        if (poweroff) {
            if (ata_power_on()) {
                mutex_unlock(&ata_mtx);
                led(false);
                return -1;
            }
        }
        else {
            if (perform_soft_reset()) {
                mutex_unlock(&ata_mtx);
                led(false);
                return -1;
            }
        }
    }

    timeout = current_tick + READ_TIMEOUT;

    ATA_SELECT = ata_device;
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        led(false);
        return -2;
    }

 retry:
    buf = inbuf;
    count = incount;
    while (TIME_BEFORE(current_tick, timeout)) {
        ret = 0;
        last_disk_activity = current_tick;

        if ( count == 256 )
            ATA_NSECTOR = 0; /* 0 means 256 sectors */
        else
            ATA_NSECTOR = (unsigned char)count;

        ATA_SECTOR  = start & 0xff;
        ATA_LCYL    = (start >> 8) & 0xff;
        ATA_HCYL    = (start >> 16) & 0xff;
        ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA | ata_device;
        ATA_COMMAND = CMD_READ_MULTIPLE;

        /* wait at least 400ns between writing command and reading status */
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");
        asm volatile ("nop");

        while (count) {
            int sectors;
            int wordcount;
            int status;

            if (!wait_for_start_of_transfer()) {
                /* We have timed out waiting for RDY and/or DRQ, possibly
                   because the hard drive is shaking and has problems reading
                   the data. We have two options:
                   1) Wait some more
                   2) Perform a soft reset and try again.

                   We choose alternative 2.
                */
                ata_soft_reset();
                ret = -4;
                goto retry;
            }

            if (spinup) {
                ata_spinup_time = current_tick - spinup_start;
                spinup = false;
                sleeping = false;
                poweroff = false;
            }

            /* read the status register exactly once per loop */
            status = ATA_STATUS;

            /* if destination address is odd, use byte copying,
               otherwise use word copying */

            if (count >= multisectors )
                sectors = multisectors;
            else
                sectors = count;

            wordcount = sectors * SECTOR_SIZE / 2;

            copy_read_sectors(buf, wordcount);

            /*
              "Device errors encountered during READ MULTIPLE commands are
              posted at the beginning of the block or partial block transfer,
              but the DRQ bit is still set to one and the data transfer shall
              take place, including transfer of corrupted data, if any."
                -- ATA specification
            */
            if ( status & (STATUS_BSY | STATUS_ERR | STATUS_DF) ) {
                ata_soft_reset();
                ret = -5;
                goto retry;
            }
             
            buf += sectors * SECTOR_SIZE; /* Advance one chunk of sectors */
            count -= sectors;

            last_disk_activity = current_tick;
        }

        if(!ret && !wait_for_end_of_transfer()) {
            ata_soft_reset();
            ret = -3;
            goto retry;
        }
        break;
    }
    led(false);

    mutex_unlock(&ata_mtx);

    /* only flush if reading went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

/* the tight loop of ata_write_sectors(), to avoid the whole in IRAM */
static void copy_write_sectors(const unsigned char* buf,
                               int wordcount)
                               __attribute__ ((section (".icode")));

static void copy_write_sectors(const unsigned char* buf, int wordcount)
{
#ifdef PREFER_C_WRITING

    if ( (unsigned int)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned short tmp = 0;
        unsigned char* bufend = buf + wordcount*2;
        do
        {   /* loop compiles to 9 assembler instructions */
            /* takes 13 clock cycles (2 pipeline stalls) */
            tmp = (unsigned short) *buf++;
            tmp |= (unsigned short) *buf++ << 8; /* I assume big endian */
            ATA_DATA = tmp;           /* and don't use the SWAB16 macro */
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {   /* loop compiles to 6 assembler instructions */
            /* takes 10 clock cycles (2 pipeline stalls) */
            ATA_DATA = SWAB16(*wbuf);
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
#else
    /* optimized assembler version */
    /* this assumes wordcount to be a multiple of 2 */

/* writing is not unrolled as much as reading, for several reasons:
 * - a similar instruction sequence is faster for writing than for reading
 *   because the auto-incrementing load inctructions can be used
 * - writing profits from warp mode
 * Both of these add up to have writing faster than the more unrolled reading.
 */
    asm (
        "add     %1,%1       \n"  /* wordcount -> bytecount */
        "add     %0,%1       \n"  /* bytecount -> bufend */
        "mov     %0,r0       \n"
        "tst     #1,r0       \n"  /* 16-bit aligned ? */
        "bt      .w_aligned  \n"  /* yes, do word copy */

        /* not 16-bit aligned */
        "mov     #-1,r6      \n"  /* prepare a bit mask for high byte */
        "shll8   r6          \n"  /* r6 = 0xFFFFFF00 */

        "mov.b   @%0+,r2     \n"  /* load (initial old second) first byte */
        "add     #-4,%1      \n"  /* adjust end address for early check */
        "mov.w   @%0+,r3     \n"  /* load (initial) first word */
        "bra     .w_start2_b \n"
        "extu.b  r2,r0       \n"  /* extend unsigned */

        ".align  2           \n"
    ".w_loop2_b:             \n"  /* main loop: copy 2 words in a row */
        "mov.w   @%0+,r3     \n"  /* load first word (2+ round) */
        "extu.b  r2,r0       \n"  /* put away low byte of second word (2+ round) */
        "and     r6,r2       \n"  /* get high byte of second word (2+ round) */
        "or      r1,r2       \n"  /* combine with low byte of old first word */
        "mov.w   r2,@%2      \n"  /* write that */
    ".w_start2_b:            \n"
        "cmp/hi  %0,%1       \n"  /* check for end */
        "mov.w   @%0+,r2     \n"  /* load second word */
        "extu.b  r3,r1       \n"  /* put away low byte of first word */
        "and     r6,r3       \n"  /* get high byte of first word */
        "or      r0,r3       \n"  /* combine with high byte of old second word */
        "mov.w   r3,@%2      \n"  /* write that */
        "bt      .w_loop2_b  \n"
        /* 12 instructions for 2 copies, takes 14 clock cycles */
        /* avg. 7 cycles per word - 85% faster */

        /* the loop "overreads" 1 byte past the buffer end, however, the last */
        /* byte is not written to disk */
        "and     r6,r2       \n"  /* get high byte of last word */
        "or      r1,r2       \n"  /* combine with low byte of old first word */
        "bra     .w_exit     \n"
        "mov.w   r2,@%2      \n"  /* write last word */

        /* 16-bit aligned, loop(load and write word) */
    ".w_aligned:             \n"
        "mov.w   @%0+,r2     \n"  /* load first word (1st round) */
        "bra     .w_start2_w \n"  /* jump into loop after next instr. */
        "add     #-4,%1      \n"  /* adjust end address for early check */

        ".align  2           \n"
    ".w_loop2_w:             \n"  /* main loop: copy 2 words in a row */
        "mov.w   @%0+,r2     \n"  /* load first word (2+ round) */
        "swap.b  r1,r0       \n"  /* swap second word (2+ round) */
        "mov.w   r0,@%2      \n"  /* write second word (2+ round) */
    ".w_start2_w:            \n"
        "cmp/hi  %0,%1       \n"  /* check for end */
        "mov.w   @%0+,r1     \n"  /* load second word */
        "swap.b  r2,r0       \n"  /* swap first word */
        "mov.w   r0,@%2      \n"  /* write first word */
        "bt      .w_loop2_w  \n"
        /* 8 instructions for 2 copies, takes 10 clock cycles */
        /* avg. 5 cycles per word - 100% faster */

        "swap.b  r1,r0       \n"  /* swap second word (last round) */
        "mov.w   r0,@%2      \n"  /* and write it */

    ".w_exit:                \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(buf),
        /* %1 */ "r"(wordcount),
        /* %2 */ "r"(&ATA_DATA)
        : /*trashed */
        "r0","r1","r2","r3","r6"
    );
#endif
}

int ata_write_sectors(unsigned long start,
                      int count,
                      const void* buf)
{
    int i;
    int ret = 0;
    int spinup_start;

    if (start == 0)
        panicf("Writing on sector 0\n");

    mutex_lock(&ata_mtx);
    
    last_disk_activity = current_tick;
    spinup_start = current_tick;

    led(true);

    if ( sleeping ) {
        spinup = true;
        if (poweroff) {
            if (ata_power_on()) {
                mutex_unlock(&ata_mtx);
                led(false);
                return -1;
            }
        }
        else {
            if (perform_soft_reset()) {
                mutex_unlock(&ata_mtx);
                led(false);
                return -1;
            }
        }
    }
    
    ATA_SELECT = ata_device;
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        led(false);
        return -2;
    }

    if ( count == 256 )
        ATA_NSECTOR = 0; /* 0 means 256 sectors */
    else
        ATA_NSECTOR = (unsigned char)count;
    ATA_SECTOR  = start & 0xff;
    ATA_LCYL    = (start >> 8) & 0xff;
    ATA_HCYL    = (start >> 16) & 0xff;
    ATA_SELECT  = ((start >> 24) & 0xf) | SELECT_LBA | ata_device;
    ATA_COMMAND = CMD_WRITE_SECTORS;

    for (i=0; i<count; i++) {

        if (!wait_for_start_of_transfer()) {
            ret = -3;
            break;
        }

        if (spinup) {
            ata_spinup_time = current_tick - spinup_start;
            spinup = false;
            sleeping = false;
            poweroff = false;
        }

        copy_write_sectors(buf, SECTOR_SIZE/2);

#ifdef USE_INTERRUPT
        /* reading the status register clears the interrupt */
        j = ATA_STATUS;
#endif
        buf += SECTOR_SIZE;

        last_disk_activity = current_tick;
    }

    if(!ret && !wait_for_end_of_transfer())
        ret = -4;

    led(false);

    mutex_unlock(&ata_mtx);

    /* only flush if writing went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

extern void ata_delayed_write(unsigned long sector, const void* buf)
{
    memcpy(delayed_sector, buf, SECTOR_SIZE);
    delayed_sector_num = sector;
    delayed_write = true;
}

extern void ata_flush(void)
{
    if ( delayed_write ) {
        DEBUGF("ata_flush()\n");
        delayed_write = false;
        ata_write_sectors(delayed_sector_num, 1, delayed_sector);
    }
}



static int check_registers(void)
{
    if ( ATA_STATUS & STATUS_BSY )
            return -1;

    ATA_NSECTOR = 0xa5;
    ATA_SECTOR  = 0x5a;
    ATA_LCYL    = 0xaa;
    ATA_HCYL    = 0x55;

    if ((ATA_NSECTOR == 0xa5) &&
        (ATA_SECTOR  == 0x5a) &&
        (ATA_LCYL    == 0xaa) &&
        (ATA_HCYL    == 0x55))
        return 0;

    return -2;
}

static int freeze_lock(void)
{
    ATA_SELECT = ata_device;

    if (!wait_for_rdy())
        return -1;

    ATA_COMMAND = CMD_SECURITY_FREEZE_LOCK;

    if (!wait_for_rdy())
        return -2;

    return 0;
}

void ata_spindown(int seconds)
{
    sleep_timeout = seconds * HZ;
}

#ifdef HAVE_ATA_POWER_OFF
void ata_poweroff(bool enable)
{
    if (enable)
        poweroff_timeout = 2*HZ;
    else
        poweroff_timeout = 0;
}
#endif

bool ata_disk_is_active(void)
{
    return !sleeping;
}

static int ata_perform_sleep(void)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("ata_perform_sleep() - not RDY\n");
        mutex_unlock(&ata_mtx);
        return -1;
    }

    ATA_COMMAND = CMD_SLEEP;

    if (!wait_for_rdy())
    {
        DEBUGF("ata_perform_sleep() - CMD failed\n");
        ret = -2;
    }

    sleeping = true;
    mutex_unlock(&ata_mtx);
    return ret;
}

int ata_standby(int time)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("ata_standby() - not RDY\n");
        mutex_unlock(&ata_mtx);
        return -1;
    }

    if(time)
        ATA_NSECTOR = ((time + 5) / 5) & 0xff; /* Round up to nearest 5 secs */
    else
        ATA_NSECTOR = 0; /* Disable standby */
        
    ATA_COMMAND = CMD_STANDBY;

    if (!wait_for_rdy())
    {
        DEBUGF("ata_standby() - CMD failed\n");
        ret = -2;
    }

    mutex_unlock(&ata_mtx);
    return ret;
}

int ata_sleep(void)
{
    queue_post(&ata_queue, Q_SLEEP, NULL);
    return 0;
}

void ata_spin(void)
{
    last_user_activity = current_tick;
}

static void ata_thread(void)
{
    static long last_sleep = 0;
    struct event ev;
    
    while (1) {
        while ( queue_empty( &ata_queue ) ) {
            if ( !spinup && sleep_timeout && !sleeping &&
                 TIME_AFTER( current_tick, 
                             last_user_activity + sleep_timeout ) &&
                 TIME_AFTER( current_tick, 
                             last_disk_activity + sleep_timeout ) )
            {
                ata_perform_sleep();
                last_sleep = current_tick;
            }

#ifdef HAVE_ATA_POWER_OFF
            if ( !spinup && sleeping && poweroff_timeout && !poweroff &&
                 TIME_AFTER( current_tick, last_sleep + poweroff_timeout ))
            {
                mutex_lock(&ata_mtx);
                ide_power_enable(false);
                mutex_unlock(&ata_mtx);
                poweroff = true;
            }
#endif

            sleep(HZ/4);
        }
        queue_wait(&ata_queue, &ev);
        switch ( ev.id ) {
#ifndef USB_NONE
            case SYS_USB_CONNECTED:
                if (poweroff) {
                    mutex_lock(&ata_mtx);
                    led(true);
                    ata_power_on();
                    led(false);
                    mutex_unlock(&ata_mtx);
                }

                /* Tell the USB thread that we are safe */
                DEBUGF("ata_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&ata_queue);
                break;
#endif
            case Q_SLEEP:
                last_disk_activity = current_tick - sleep_timeout + (HZ/2);
                break;
        }
    }
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    int ret;
    
    /* state HRR0 */
    and_b(~0x02, &PADRH); /* assert _RESET */
    sleep(1); /* > 25us */

    /* state HRR1 */
    or_b(0x02, &PADRH); /* negate _RESET */
    sleep(1); /* > 2ms */

    /* state HRR2 */
    ATA_SELECT = ata_device; /* select the right device */
    ret = wait_for_bsy();

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    return ret;
}

static int perform_soft_reset(void)
{
    int ret;
    int retry_count;
    
    ATA_SELECT = SELECT_LBA | ata_device;
    ATA_CONTROL = CONTROL_nIEN|CONTROL_SRST;
    sleep(1); /* >= 5us */

    ATA_CONTROL = CONTROL_nIEN;
    sleep(1); /* >2ms */

    /* This little sucker can take up to 30 seconds */
    retry_count = 8;
    do
    {
        ret = wait_for_rdy();
    } while(!ret && retry_count--);

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    return ret;
}

int ata_soft_reset(void)
{
    int ret;
    
    mutex_lock(&ata_mtx);

    ret = perform_soft_reset();

    mutex_unlock(&ata_mtx);
    return ret;
}

static int ata_power_on(void)
{
    int rc;
    
    ide_power_enable(true);
    if( ata_hard_reset() )
        return -1;

    rc = set_features();
    if (rc)
        return rc * 10 - 2;

    if (set_multiple_mode(multisectors))
        return -3;

    if (freeze_lock())
        return -4;

    return 0;
}

static int master_slave_detect(void)
{
    /* master? */
    ATA_SELECT = 0;
    if ( ATA_STATUS & (STATUS_RDY|STATUS_BSY) ) {
        ata_device = 0;
        DEBUGF("Found master harddisk\n");
    }
    else {
        /* slave? */
        ATA_SELECT = SELECT_DEVICE1;
        if ( ATA_STATUS & (STATUS_RDY|STATUS_BSY) ) {
            ata_device = SELECT_DEVICE1;
            DEBUGF("Found slave harddisk\n");
        }
        else
            return -1;
    }
    return 0;
}

static int io_address_detect(void)
{   /* now, use the HW mask instead of probing */
    if (read_hw_mask() & ATA_ADDRESS_200)
    {
        ata_io_address = 0x200; /* For debug purposes only */
        old_recorder = false;
        ata_control = ATA_CONTROL1;
    }
    else
    {
        ata_io_address = 0x300; /* For debug purposes only */
        old_recorder = true;
        ata_control = ATA_CONTROL2;
    }

    return 0;
}

void ata_enable(bool on)
{
    if(on)
        and_b(~0x80, &PADRL); /* enable ATA */
    else
        or_b(0x80, &PADRL); /* disable ATA */

    or_b(0x80, &PAIORL);
}

static int identify(void)
{
    int i;

    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("identify() - not RDY\n");
        return -1;
    }

    ATA_COMMAND = CMD_IDENTIFY;

    if (!wait_for_start_of_transfer())
    {
        DEBUGF("identify() - CMD failed\n");
        return -2;
    }

    for (i=0; i<SECTOR_SIZE/2; i++)
        /* the IDENTIFY words are already swapped */
        identify_info[i] = ATA_DATA;
    
    return 0;
}

static int set_multiple_mode(int sectors)
{
    ATA_SELECT = ata_device;

    if(!wait_for_rdy()) {
        DEBUGF("set_multiple_mode() - not RDY\n");
        return -1;
    }

    ATA_NSECTOR = sectors;
    ATA_COMMAND = CMD_SET_MULTIPLE_MODE;

    if (!wait_for_rdy())
    {
        DEBUGF("set_multiple_mode() - CMD failed\n");
        return -2;
    }

    return 0;
}

static int set_features(void)
{
    struct {
        unsigned char id_word;
        unsigned char id_bit;
        unsigned char subcommand;
        unsigned char parameter;
    } features[] = {
        { 83, 3, 0x05, 0x80 }, /* power management: lowest power without standby */
        { 83, 9, 0x42, 0x80 }, /* acoustic management: lowest noise */
        { 82, 6, 0xaa, 0 },    /* enable read look-ahead */
        { 83, 14, 0x03, 0 },   /* force PIO mode */
        { 0, 0, 0, 0 }         /* <end of list> */
    };
    int i;
    int pio_mode = 2;

    /* Find out the highest supported PIO mode */
    if(identify_info[64] & 2)
        pio_mode = 4;
    else
        if(identify_info[64] & 1)
            pio_mode = 3;

    /* Update the table */
    features[3].parameter = 8 + pio_mode;
    
    ATA_SELECT = ata_device;

    if (!wait_for_rdy()) {
        DEBUGF("set_features() - not RDY\n");
        return -1;
    }

    for (i=0; features[i].id_word; i++) {
        if (identify_info[features[i].id_word] & (1 << features[i].id_bit)) {
            ATA_FEATURE = features[i].subcommand;
            ATA_NSECTOR = features[i].parameter;
            ATA_COMMAND = CMD_SET_FEATURES;

            if (!wait_for_rdy()) {
                DEBUGF("set_features() - CMD failed\n");
                return -10 - i;
            }

            if(ATA_ALT_STATUS & STATUS_ERR) {
                if(ATA_ERROR & ERROR_ABRT) {
                    return -20 - i;
                }
            }
        }
    }

    return 0;
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

int ata_init(void)
{
    int rc;
    bool coldstart = (PACR2 & 0x4000) != 0; 

    mutex_init(&ata_mtx);

    led(false);

    /* Port A setup */
    or_b(0x02, &PAIORH); /* output for ATA reset */
    or_b(0x02, &PADRH); /* release ATA reset */
    PACR2 &= 0xBFFF; /* GPIO function for PA7 (IDE enable) */

    sleeping = false;
    ata_enable(true);

    if ( !initialized ) {
        if (!ide_powered()) /* somebody has switched it off */
        {
            ide_power_enable(true);
            sleep(HZ); /* allow voltage to build up */
        }

        if (coldstart)
        {
            /* This should reset both master and slave, we don't yet know what's in */
            ata_device = 0;
            if (ata_hard_reset())
                return -1;
        }

        rc = master_slave_detect();
        if (rc)
            return -10 + rc;

        rc = io_address_detect();
        if (rc)
            return -20 + rc;

        /* symptom fix: else check_registers() below may fail */
        if (coldstart && !wait_for_bsy())
        {   
            return -29;
        }

        rc = check_registers();
        if (rc)
            return -30 + rc;

        rc = freeze_lock();
        if (rc)
            return -40 + rc;

        rc = identify();
        if (rc)
            return -50 + rc;
        multisectors = identify_info[47] & 0xff;
        DEBUGF("ata: %d sectors per ata request\n",multisectors);
        
        rc = set_features();
        if (rc)
            return -60 + rc;

        queue_init(&ata_queue);

        last_disk_activity = current_tick;
        create_thread(ata_thread, ata_stack,
                      sizeof(ata_stack), ata_thread_name);
        initialized = true;
    }
    rc = set_multiple_mode(multisectors);
    if (rc)
        return -70 + rc;

    return 0;
}

#endif /* #ifndef HAVE_MMC */
