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

/* use plain C code in copy_read_sectors(), instead of tweaked assembler */
#define PREFER_C /* mystery: assembler caused problems with some disks */

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
static char ata_thread_name[] = "ata";
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
    int timeout = current_tick + HZ*10;
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
    unsigned short tmp = 0; /* have to init to prevent warning? */

    if ( (unsigned int)buf & 1) 
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned char* bufend = buf + wordcount*2;
#ifdef PREFER_C
        do
        {   /* loop compiles to 9 assembler instructions */
            tmp = ATA_DATA;
            *buf++ = tmp & 0xff; /* I assume big endian */
            *buf++ = tmp >> 8;   /*  and don't use the SWAB16 macro */
        } while (buf < bufend); /* tail loop is faster */
#else
        /* I can bring it down to 7 instructions/loop, and exploit pipeline */
        asm (
            "mov    #1, r0 \n"      /* r0 = 1; */
            /* correct for the "early increment" below */
            "add  	#-2,%2 \n"    /* buf -= 2; */
            "add  	#-2,%3 \n"    /* bufend -= 2; */
            "loop_b: \n"
            "mov.w	@%1,%0 \n"      /* tmp = ATA_DATA; */
            /* Now we're reading from the bus, I do something independent we 
               need later, to avoid pipeline stall */
            "add  	#0x02,%2 \n"    /* buf += 2; */
            "cmp/hs	%3,%2 \n"       /* if (buf < bufend) */
            /* now use the read result */
            "mov.b	%0,@%2 \n"      /* buf[0] = lowbyte(tmp); */
            "shlr8	%0 \n"          /* tmp >>= 8; */
            "mov.b	%0,@(r0,%2) \n" /* buf[r0] = lowbyte(tmp); */
            "bf	    loop_b \n"      /* goto loop_b; */
            : /* outputs */
            : /* inputs */
            /* %0 */ "r"(tmp),
            /* %1 */ "r"(&ATA_DATA),
            /* %2 */ "r"(buf),
            /* %3 */ "r"(bufend)
            : /* trashed */
            "r0"
        );
#endif
    }
    else 
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
#ifdef PREFER_C
        do
        {   /* loop compiles to 7 assembler instructions */
            *wbuf = SWAB16(ATA_DATA);
        } while (++wbuf < wbufend); /* tail loop is faster */
#else
        /* I can bring it down to 9 instructions for 2 loops, and pipeline */
        asm (
            "mov    #2, r0 \n"      /* r0 = 2 */
            /* correct for the "early increment" below */
            "add  	#-4,%2 \n"      /* wbuf -= 4; */
            "bra enter_loop \n"     /* goto enter_loop, after next instr. */
            "add  	#-4,%3 \n"      /* wbufend -= 4; */
            "loop_w: \n"
            /* use read result and store, from last round */
            "swap.b	%0,%0 \n"       /* endian_swap(tmp); */
            "mov.w	%0,@(r0,%2) \n" /* wbuf[r0] = tmp; */
            "enter_loop: \n"
            "mov.w	@%1,%0 \n"      /* tmp = ATA_DATA; */
            /* keep the pipeline busy with 2 independent instructions */
            "add  	#0x04,%2 \n"    /* wbuf += 4; */
            "cmp/hs	%3,%2 \n"       /* if (wbuf < wbufend) */
            "swap.b	%0,%0 \n"       /* endian_swap(tmp); */
            "mov.w	%0,@%2 \n"      /* wbuf[0] = tmp; */
            /* unrolled, do one more */
            "mov.w	@%1,%0 \n"      /* tmp = ATA_DATA; */
            /* use and store later, to keep pipeline busy */
            "bf	    loop_w \n"      /* goto loop_w; */
            "swap.b	%0,%0 \n"       /* endian_swap(tmp); */
            "mov.w	%0,@(r0,%2) \n" /* wbuf[r0] = tmp; */
            : /* outputs */
            : /* inputs */
            /* %0 */ "r"(tmp),
            /* %1 */ "r"(&ATA_DATA),
            /* %2 */ "r"(wbuf),
            /* %3 */ "r"(wbufend)
            : /* trashed */
            "r0"
        );
#endif
    }
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
                ret = -5;
                goto retry;
            }
             
            buf += sectors * SECTOR_SIZE; /* Advance one chunk of sectors */
            count -= sectors;

            last_disk_activity = current_tick;
        }

        if(!ret && !wait_for_end_of_transfer()) {
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

int ata_write_sectors(unsigned long start,
                      int count,
                      void* buf)
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
        int j;
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

        for (j=0; j<SECTOR_SIZE/2; j++) {
            ATA_DATA = (unsigned short)
                (((unsigned char *)buf)[j*2+1] << 8) |
                ((unsigned char *)buf)[j*2];
        }

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

extern void ata_delayed_write(unsigned long sector, void* buf)
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
        { 83, 3, 0x05, 1 },    /* power management: lowest power */
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
            /* Reset both master and slave, we don't yet know what's in */
            /* this is safe because non-present devices don't report busy */
            ata_device = 0;
            if (ata_hard_reset())
                return -1;
            ata_device = SELECT_DEVICE1;
            if (ata_hard_reset())
                return -2;
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
