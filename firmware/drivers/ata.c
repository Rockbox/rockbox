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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "cpu.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
#include "ata_idle_notify.h"
#include "ata-target.h"
#include "storage.h"

#define SECTOR_SIZE     (512)

#define ATA_FEATURE     ATA_ERROR

#define ATA_STATUS      ATA_COMMAND
#define ATA_ALT_STATUS  ATA_CONTROL

#define SELECT_DEVICE1  0x10
#define SELECT_LBA      0x40

#define CONTROL_nIEN    0x02
#define CONTROL_SRST    0x04

#define CMD_READ_SECTORS           0x20
#define CMD_WRITE_SECTORS          0x30
#define CMD_WRITE_SECTORS_EXT      0x34
#define CMD_READ_MULTIPLE          0xC4
#define CMD_READ_MULTIPLE_EXT      0x29
#define CMD_WRITE_MULTIPLE         0xC5
#define CMD_SET_MULTIPLE_MODE      0xC6
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2
#define CMD_IDENTIFY               0xEC
#define CMD_SLEEP                  0xE6
#define CMD_SET_FEATURES           0xEF
#define CMD_SECURITY_FREEZE_LOCK   0xF5

/* Should all be < 0x100 (which are reserved for control messages) */
#define Q_SLEEP    0
#define Q_CLOSE    1

#define READ_TIMEOUT 5*HZ

#ifdef HAVE_ATA_POWER_OFF
#define ATA_POWER_OFF_TIMEOUT 2*HZ
#endif

#ifdef ATA_DRIVER_CLOSE
static unsigned int ata_thread_id = 0;
#endif

#if defined(MAX_PHYS_SECTOR_SIZE) && MEM == 64
/* Hack - what's the deal with 5g? */
struct ata_lock
{
    struct thread_entry *thread;
    int count;
    volatile unsigned char locked;
    IF_COP( struct corelock cl; )
};

static void ata_lock_init(struct ata_lock *l)
{
    corelock_init(&l->cl);
    l->locked = 0;
    l->count = 0;
    l->thread = NULL;
}

static void ata_lock_lock(struct ata_lock *l)
{
    struct thread_entry * const current =
        thread_id_entry(THREAD_ID_CURRENT);

    if (current == l->thread)
    {
        l->count++;
        return;
    }

    corelock_lock(&l->cl);

    IF_PRIO( current->skip_count = -1; )

    while (l->locked != 0)
    {
        corelock_unlock(&l->cl);
        switch_thread();
        corelock_lock(&l->cl);
    }

    l->locked = 1;
    l->thread = current;
    corelock_unlock(&l->cl);
}

static void ata_lock_unlock(struct ata_lock *l)
{
    if (l->count > 0)
    {
        l->count--;
        return;
    }

    corelock_lock(&l->cl);

    IF_PRIO( l->thread->skip_count = 0; )

    l->thread = NULL;
    l->locked = 0;

    corelock_unlock(&l->cl);
}

#define mutex           ata_lock
#define mutex_init      ata_lock_init
#define mutex_lock      ata_lock_lock
#define mutex_unlock    ata_lock_unlock
#endif /* MAX_PHYS_SECTOR_SIZE */

#if defined(HAVE_USBSTACK) && defined(USE_ROCKBOX_USB)
#define ALLOW_USB_SPINDOWN
#endif

static struct mutex ata_mtx SHAREDBSS_ATTR;
static int ata_device; /* device 0 (master) or 1 (slave) */

static int spinup_time = 0;
#if (CONFIG_LED == LED_REAL)
static bool ata_led_enabled = true;
static bool ata_led_on = false;
#endif
static bool spinup = false;
static bool sleeping = true;
static bool poweroff = false;
static long sleep_timeout = 5*HZ;
#ifdef HAVE_LBA48
static bool lba48 = false; /* set for 48 bit addressing */
#endif
static long ata_stack[(DEFAULT_STACK_SIZE*3)/sizeof(long)];
static const char ata_thread_name[] = "ata";
static struct event_queue ata_queue SHAREDBSS_ATTR;
static bool initialized = false;

static long last_user_activity = -1;
static long last_disk_activity = -1;

static unsigned long total_sectors;
static int multisectors; /* number of supported multisectors */
static unsigned short identify_info[SECTOR_SIZE/2];

#ifdef MAX_PHYS_SECTOR_SIZE

struct sector_cache_entry {
    bool inuse;
    unsigned long sectornum;  /* logical sector */
    unsigned char data[MAX_PHYS_SECTOR_SIZE];
};
/* buffer for reading and writing large physical sectors */
#define NUMCACHES 2
static struct sector_cache_entry sector_cache;
static int phys_sector_mult = 1;
#endif

static int ata_power_on(void);
static int perform_soft_reset(void);
static int set_multiple_mode(int sectors);
static int set_features(void);

STATICIRAM ICODE_ATTR int wait_for_bsy(void)
{
    long timeout = current_tick + HZ*30;
    
    do 
    {
        if (!(ATA_STATUS & STATUS_BSY))
            return 1;
        last_disk_activity = current_tick;
        yield();
    } while (TIME_BEFORE(current_tick, timeout));

    return 0; /* timeout */
}

STATICIRAM ICODE_ATTR int wait_for_rdy(void)
{
    long timeout;

    if (!wait_for_bsy())
        return 0;

    timeout = current_tick + HZ*10;
    
    do 
    {
        if (ATA_ALT_STATUS & STATUS_RDY)
            return 1;
        last_disk_activity = current_tick;
        yield();
    } while (TIME_BEFORE(current_tick, timeout));

    return 0; /* timeout */
}

STATICIRAM ICODE_ATTR int wait_for_start_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;

    return (ATA_ALT_STATUS & (STATUS_BSY|STATUS_DRQ)) == STATUS_DRQ;
}

STATICIRAM ICODE_ATTR int wait_for_end_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_RDY|STATUS_DRQ)) == STATUS_RDY;
}    

#if (CONFIG_LED == LED_REAL)
/* Conditionally block LED access for the ATA driver, so the LED can be
 * (mis)used for other purposes */
static void ata_led(bool on) 
{
    ata_led_on = on;
    if (ata_led_enabled)
        led(ata_led_on);
}
#else
#define ata_led(on) led(on)
#endif

#ifndef ATA_OPTIMIZED_READING
STATICIRAM ICODE_ATTR void copy_read_sectors(unsigned char* buf, int wordcount)
{
    unsigned short tmp = 0;

    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned char* bufend = buf + wordcount*2;
        do
        {
            tmp = ATA_DATA;
#if defined(SWAP_WORDS) || defined(ROCKBOX_LITTLE_ENDIAN)
            *buf++ = tmp & 0xff; /* I assume big endian */
            *buf++ = tmp >> 8;   /*  and don't use the SWAB16 macro */
#else
            *buf++ = tmp >> 8;
            *buf++ = tmp & 0xff;
#endif
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {
#ifdef SWAP_WORDS
            *wbuf = swap16(ATA_DATA);
#else
            *wbuf = ATA_DATA;
#endif
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
}
#endif /* !ATA_OPTIMIZED_READING */

#ifdef MAX_PHYS_SECTOR_SIZE
static int _read_sectors(unsigned long start,
                         int incount,
                         void* inbuf)
#else
int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
#endif
{
    int ret = 0;
    long timeout;
    int count;
    void* buf;
    long spinup_start;

#ifndef MAX_PHYS_SECTOR_SIZE
#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mtx);
#endif

    if (start + incount > total_sectors) {
        ret = -1;
        goto error;
    }

    last_disk_activity = current_tick;
    spinup_start = current_tick;

    ata_led(true);

    if ( sleeping ) {
        spinup = true;
        if (poweroff) {
            if (ata_power_on()) {
                ret = -2;
                goto error;
            }
        }
        else {
            if (perform_soft_reset()) {
                ret = -2;
                goto error;
            }
        }
    }

    timeout = current_tick + READ_TIMEOUT;

    SET_REG(ATA_SELECT, ata_device);
    if (!wait_for_rdy())
    {
        ret = -3;
        goto error;
    }

 retry:
    buf = inbuf;
    count = incount;
    while (TIME_BEFORE(current_tick, timeout)) {
        ret = 0;
        last_disk_activity = current_tick;

#ifdef HAVE_LBA48
        if (lba48)
        {
            SET_REG(ATA_NSECTOR, count >> 8);
            SET_REG(ATA_NSECTOR, count & 0xff);
            SET_REG(ATA_SECTOR, (start >> 24) & 0xff); /* 31:24 */
            SET_REG(ATA_SECTOR, start & 0xff); /* 7:0 */
            SET_REG(ATA_LCYL, 0); /* 39:32 */
            SET_REG(ATA_LCYL, (start >> 8) & 0xff); /* 15:8 */
            SET_REG(ATA_HCYL, 0); /* 47:40 */
            SET_REG(ATA_HCYL, (start >> 16) & 0xff); /* 23:16 */
            SET_REG(ATA_SELECT, SELECT_LBA | ata_device);
            SET_REG(ATA_COMMAND, CMD_READ_MULTIPLE_EXT);
        }
        else
#endif
        {
            SET_REG(ATA_NSECTOR, count & 0xff); /* 0 means 256 sectors */
            SET_REG(ATA_SECTOR, start & 0xff);
            SET_REG(ATA_LCYL, (start >> 8) & 0xff);
            SET_REG(ATA_HCYL, (start >> 16) & 0xff);
            SET_REG(ATA_SELECT, ((start >> 24) & 0xf) | SELECT_LBA | ata_device);
            SET_REG(ATA_COMMAND, CMD_READ_MULTIPLE);
        }

        /* wait at least 400ns between writing command and reading status */
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");

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
                perform_soft_reset();
                ret = -5;
                goto retry;
            }

            if (spinup) {
                spinup_time = current_tick - spinup_start;
                spinup = false;
                sleeping = false;
                poweroff = false;
            }

            /* read the status register exactly once per loop */
            status = ATA_STATUS;

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
                perform_soft_reset();
                ret = -6;
                goto retry;
            }

            buf += sectors * SECTOR_SIZE; /* Advance one chunk of sectors */
            count -= sectors;

            last_disk_activity = current_tick;
        }

        if(!ret && !wait_for_end_of_transfer()) {
            perform_soft_reset();
            ret = -4;
            goto retry;
        }
        break;
    }

  error:
    ata_led(false);
#ifndef MAX_PHYS_SECTOR_SIZE
    mutex_unlock(&ata_mtx);
#endif

    return ret;
}

#ifndef ATA_OPTIMIZED_WRITING
STATICIRAM ICODE_ATTR void copy_write_sectors(const unsigned char* buf,
                                              int wordcount)
{
    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned short tmp = 0;
        const unsigned char* bufend = buf + wordcount*2;
        do
        {
#if defined(SWAP_WORDS) || defined(ROCKBOX_LITTLE_ENDIAN)
            tmp = (unsigned short) *buf++;
            tmp |= (unsigned short) *buf++ << 8;
            SET_16BITREG(ATA_DATA, tmp);
#else
            tmp = (unsigned short) *buf++ << 8;
            tmp |= (unsigned short) *buf++;
            SET_16BITREG(ATA_DATA, tmp);
#endif
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {
#ifdef SWAP_WORDS
            SET_16BITREG(ATA_DATA, swap16(*wbuf));
#else
            SET_16BITREG(ATA_DATA, *wbuf);
#endif
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
}
#endif /* !ATA_OPTIMIZED_WRITING */

#ifdef MAX_PHYS_SECTOR_SIZE
static int _write_sectors(unsigned long start,
                          int count,
                          const void* buf)
#else
int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
#endif
{
    int i;
    int ret = 0;
    long spinup_start;

#ifndef MAX_PHYS_SECTOR_SIZE
#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mtx);
#endif
    
    if (start + count > total_sectors)
        panicf("Writing past end of disk");

    last_disk_activity = current_tick;
    spinup_start = current_tick;

    ata_led(true);

    if ( sleeping ) {
        spinup = true;
        if (poweroff) {
            if (ata_power_on()) {
                ret = -1;
                goto error;
            }
        }
        else {
            if (perform_soft_reset()) {
                ret = -1;
                goto error;
            }
        }
    }
    
    SET_REG(ATA_SELECT, ata_device);
    if (!wait_for_rdy())
    {
        ret = -2;
        goto error;
    }

#ifdef HAVE_LBA48
    if (lba48)
    {
        SET_REG(ATA_NSECTOR, count >> 8);
        SET_REG(ATA_NSECTOR, count & 0xff);
        SET_REG(ATA_SECTOR, (start >> 24) & 0xff); /* 31:24 */
        SET_REG(ATA_SECTOR, start & 0xff); /* 7:0 */
        SET_REG(ATA_LCYL, 0); /* 39:32 */
        SET_REG(ATA_LCYL, (start >> 8) & 0xff); /* 15:8 */
        SET_REG(ATA_HCYL, 0); /* 47:40 */
        SET_REG(ATA_HCYL, (start >> 16) & 0xff); /* 23:16 */
        SET_REG(ATA_SELECT, SELECT_LBA | ata_device);
        SET_REG(ATA_COMMAND, CMD_WRITE_SECTORS_EXT);
    }
    else
#endif
    {
        SET_REG(ATA_NSECTOR, count & 0xff); /* 0 means 256 sectors */
        SET_REG(ATA_SECTOR, start & 0xff);
        SET_REG(ATA_LCYL, (start >> 8) & 0xff);
        SET_REG(ATA_HCYL, (start >> 16) & 0xff);
        SET_REG(ATA_SELECT, ((start >> 24) & 0xf) | SELECT_LBA | ata_device);
        SET_REG(ATA_COMMAND, CMD_WRITE_SECTORS);
    }

    for (i=0; i<count; i++) {

        if (!wait_for_start_of_transfer()) {
            ret = -3;
            break;
        }

        if (spinup) {
            spinup_time = current_tick - spinup_start;
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

    if(!ret && !wait_for_end_of_transfer()) {
        DEBUGF("End on transfer failed. -- jyp");
        ret = -4;
    }

  error:
    ata_led(false);
#ifndef MAX_PHYS_SECTOR_SIZE
    mutex_unlock(&ata_mtx);
#endif

    return ret;
}

#ifdef MAX_PHYS_SECTOR_SIZE
static int cache_sector(unsigned long sector)
{
    int rc;

    sector &= ~(phys_sector_mult - 1); 
              /* round down to physical sector boundary */

    /* check whether the sector is already cached */
    if (sector_cache.inuse && (sector_cache.sectornum == sector))
        return 0;

    /* not found: read the sector */
    sector_cache.inuse = false;
    rc = _read_sectors(sector, phys_sector_mult, sector_cache.data);
    if (!rc)
    {    
        sector_cache.sectornum = sector;
        sector_cache.inuse = true;
    }
    return rc;
}

static inline int flush_current_sector(void)
{
    return _write_sectors(sector_cache.sectornum, phys_sector_mult,
                          sector_cache.data);
}

int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{
    int rc = 0;
    int offset;

#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mtx);
    
    offset = start & (phys_sector_mult - 1);
    
    if (offset) /* first partial sector */
    {
        int partcount = MIN(incount, phys_sector_mult - offset);
        
        rc = cache_sector(start);
        if (rc)
        {
            rc = rc * 10 - 1;
            goto error;
        }                          
        memcpy(inbuf, sector_cache.data + offset * SECTOR_SIZE,
               partcount * SECTOR_SIZE);

        start += partcount;
        inbuf += partcount * SECTOR_SIZE;
        incount -= partcount;
    }
    if (incount)
    {
        offset = incount & (phys_sector_mult - 1);
        incount -= offset;
        
        if (incount)
        {
            rc = _read_sectors(start, incount, inbuf);
            if (rc)
            {
                rc = rc * 10 - 2;
                goto error;
            }
            start += incount;
            inbuf += incount * SECTOR_SIZE;
        }
        if (offset)
        {
            rc = cache_sector(start);
            if (rc)
            {
                rc = rc * 10 - 3;
                goto error;
            }
            memcpy(inbuf, sector_cache.data, offset * SECTOR_SIZE);
        }
    }

  error:
    mutex_unlock(&ata_mtx);

    return rc;
}

int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    int rc = 0;
    int offset;

#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mtx);
    
    offset = start & (phys_sector_mult - 1);
    
    if (offset) /* first partial sector */
    {
        int partcount = MIN(count, phys_sector_mult - offset);

        rc = cache_sector(start);
        if (rc)
        {
            rc = rc * 10 - 1;
            goto error;
        }                          
        memcpy(sector_cache.data + offset * SECTOR_SIZE, buf,
               partcount * SECTOR_SIZE);
        rc = flush_current_sector();
        if (rc)
        {
            rc = rc * 10 - 2;
            goto error;
        }                          
        start += partcount;
        buf += partcount * SECTOR_SIZE;
        count -= partcount;
    }
    if (count)
    {
        offset = count & (phys_sector_mult - 1);
        count -= offset;
        
        if (count)
        {
            rc = _write_sectors(start, count, buf);
            if (rc)
            {
                rc = rc * 10 - 3;
                goto error;
            }
            start += count;
            buf += count * SECTOR_SIZE;
        }
        if (offset)
        {
            rc = cache_sector(start);
            if (rc)
            {
                rc = rc * 10 - 4;
                goto error;
            }
            memcpy(sector_cache.data, buf, offset * SECTOR_SIZE); 
            rc = flush_current_sector();
            if (rc)
            {
                rc = rc * 10 - 5;
                goto error;
            }
        }
    }

  error:
    mutex_unlock(&ata_mtx);

    return rc;
}
#endif /* MAX_PHYS_SECTOR_SIZE */

static int check_registers(void)
{
    int i;
    if ( ATA_STATUS & STATUS_BSY )
            return -1;

    for (i = 0; i<64; i++) {
        SET_REG(ATA_NSECTOR, WRITE_PATTERN1);
        SET_REG(ATA_SECTOR,  WRITE_PATTERN2);
        SET_REG(ATA_LCYL,    WRITE_PATTERN3);
        SET_REG(ATA_HCYL,    WRITE_PATTERN4);

        if (((ATA_NSECTOR & READ_PATTERN1_MASK) == READ_PATTERN1) &&
            ((ATA_SECTOR & READ_PATTERN2_MASK) == READ_PATTERN2) &&
            ((ATA_LCYL & READ_PATTERN3_MASK) == READ_PATTERN3) &&
            ((ATA_HCYL & READ_PATTERN4_MASK) == READ_PATTERN4))
            return 0;
    }
    return -2;
}

static int freeze_lock(void)
{
    /* does the disk support Security Mode feature set? */
    if (identify_info[82] & 2)
    {
        SET_REG(ATA_SELECT, ata_device);

        if (!wait_for_rdy())
            return -1;

        SET_REG(ATA_COMMAND, CMD_SECURITY_FREEZE_LOCK);

        if (!wait_for_rdy())
            return -2;
    }

    return 0;
}

void ata_spindown(int seconds)
{
    sleep_timeout = seconds * HZ;
}

bool ata_disk_is_active(void)
{
    return !sleeping;
}

static int ata_perform_sleep(void)
{
    mutex_lock(&ata_mtx);

    SET_REG(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("ata_perform_sleep() - not RDY\n");
        mutex_unlock(&ata_mtx);
        return -1;
    }

    SET_REG(ATA_COMMAND, CMD_SLEEP);

    if (!wait_for_rdy())
    {
        DEBUGF("ata_perform_sleep() - CMD failed\n");
        mutex_unlock(&ata_mtx);
        return -2;
    }

    sleeping = true;
    mutex_unlock(&ata_mtx);
    return 0; 
}

void ata_sleep(void)
{
    queue_post(&ata_queue, Q_SLEEP, 0);
}

void ata_sleepnow(void)
{
    if (!spinup && !sleeping && !ata_mtx.locked && initialized)
    {
        call_storage_idle_notifys(false);
        ata_perform_sleep();
    }
}

void ata_spin(void)
{
    last_user_activity = current_tick;
}

static void ata_thread(void)
{
    static long last_sleep = 0;
    struct queue_event ev;
    static long last_seen_mtx_unlock = 0;
#ifdef ALLOW_USB_SPINDOWN
    static bool usb_mode = false;
#endif
    
    while (1) {
        queue_wait_w_tmo(&ata_queue, &ev, HZ/2);

        switch ( ev.id ) {
            case SYS_TIMEOUT:
                if (!spinup && !sleeping)
                {
                    if (!ata_mtx.locked)
                    {
                        if (!last_seen_mtx_unlock)
                            last_seen_mtx_unlock = current_tick;
                        if (TIME_AFTER(current_tick, last_seen_mtx_unlock+(HZ*2)))
                        {
#ifdef ALLOW_USB_SPINDOWN
                            if(!usb_mode)
#endif
                            {
                                call_storage_idle_notifys(false);
                            }
                            last_seen_mtx_unlock = 0;
                        }
                    }
                    if ( sleep_timeout &&
                         TIME_AFTER( current_tick, 
                                    last_user_activity + sleep_timeout ) &&
                         TIME_AFTER( current_tick, 
                                    last_disk_activity + sleep_timeout ) )
                    {
#ifdef ALLOW_USB_SPINDOWN
                        if(!usb_mode)
#endif
                        {
                            call_storage_idle_notifys(true);
                        }
                        ata_perform_sleep();
                        last_sleep = current_tick;
                    }
                }

#ifdef HAVE_ATA_POWER_OFF
                if ( !spinup && sleeping && !poweroff &&
                     TIME_AFTER( current_tick, last_sleep + ATA_POWER_OFF_TIMEOUT ))
                {
                    mutex_lock(&ata_mtx);
                    ide_power_enable(false);
                    poweroff = true;
                    mutex_unlock(&ata_mtx);
                }
#endif
                break;

#ifndef USB_NONE
            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("ata_thread got SYS_USB_CONNECTED\n");
#ifdef ALLOW_USB_SPINDOWN
                usb_mode = true;
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                /* There is no need to force ATA power on */
#else
                if (sleeping) {
                    mutex_lock(&ata_mtx);
                    ata_led(true);
                    if (poweroff) {
                        ata_power_on();
                        poweroff = false;
                    }
                    else {
                        perform_soft_reset();
                    }
                    sleeping = false;                   
                    ata_led(false);
                    mutex_unlock(&ata_mtx);
                }

                /* Wait until the USB cable is extracted again */
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&ata_queue);
#endif
                break;
                
#ifdef ALLOW_USB_SPINDOWN
            case SYS_USB_DISCONNECTED:
                /* Tell the USB thread that we are ready again */
                DEBUGF("ata_thread got SYS_USB_DISCONNECTED\n");
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                usb_mode = false;
                break;
#endif
#endif /* USB_NONE */

            case Q_SLEEP:
#ifdef ALLOW_USB_SPINDOWN
                if(!usb_mode)
#endif
                {
                    call_storage_idle_notifys(false);
                }
                last_disk_activity = current_tick - sleep_timeout + (HZ/2);
                break;

#ifdef ATA_DRIVER_CLOSE
            case Q_CLOSE:
                return;
#endif
        }
    }
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
static int ata_hard_reset(void)
{
    int ret;

    mutex_lock(&ata_mtx);

    ata_reset();

    /* state HRR2 */
    SET_REG(ATA_SELECT, ata_device); /* select the right device */
    ret = wait_for_bsy();

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    mutex_unlock(&ata_mtx);

    return ret;
}

static int perform_soft_reset(void)
{
/* If this code is allowed to run on a Nano, the next reads from the flash will
 * time out, so we disable it. It shouldn't be necessary anyway, since the
 * ATA -> Flash interface automatically sleeps almost immediately after the
 * last command.
 */
    int ret;
    int retry_count;

    SET_REG(ATA_SELECT, SELECT_LBA | ata_device );
    SET_REG(ATA_CONTROL, CONTROL_nIEN|CONTROL_SRST );
    sleep(1); /* >= 5us */

    SET_REG(ATA_CONTROL, CONTROL_nIEN);
    sleep(1); /* >2ms */

    /* This little sucker can take up to 30 seconds */
    retry_count = 8;
    do
    {
        ret = wait_for_rdy();
    } while(!ret && retry_count--);

    if (!ret)
        return -1;

    if (set_features())
        return -2;

    if (set_multiple_mode(multisectors))
        return -3;

    if (freeze_lock())
        return -4;

    return 0;
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
    sleep(HZ/4); /* allow voltage to build up */

    /* Accessing the PP IDE controller too early after powering up the disk
     * makes the core hang for a short time, causing an audio dropout. This
     * also depends on the disk; iPod Mini G2 needs at least HZ/5 to get rid
     * of the dropout. Since this time isn't additive (the wait_for_bsy() in
     * ata_hard_reset() will shortened by the same amount), it's a good idea
     * to do this on all HDD based targets. */

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
    SET_REG(ATA_SELECT, 0);
    if ( ATA_STATUS & (STATUS_RDY|STATUS_BSY) ) {
        ata_device = 0;
        DEBUGF("Found master harddisk\n");
    }
    else {
        /* slave? */
        SET_REG(ATA_SELECT, SELECT_DEVICE1);
        if ( ATA_STATUS & (STATUS_RDY|STATUS_BSY) ) {
            ata_device = SELECT_DEVICE1;
            DEBUGF("Found slave harddisk\n");
        }
        else
            return -1;
    }
    return 0;
}

static int identify(void)
{
    int i;

    SET_REG(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("identify() - not RDY\n");
        return -1;
    }
    SET_REG(ATA_COMMAND, CMD_IDENTIFY);

    if (!wait_for_start_of_transfer())
    {
        DEBUGF("identify() - CMD failed\n");
        return -2;
    }

    for (i=0; i<SECTOR_SIZE/2; i++) {
        /* the IDENTIFY words are already swapped, so we need to treat
           this info differently that normal sector data */
#if defined(ROCKBOX_BIG_ENDIAN) && !defined(SWAP_WORDS)
        identify_info[i] = swap16(ATA_DATA);
#else
        identify_info[i] = ATA_DATA;
#endif
    }

    return 0;
}

static int set_multiple_mode(int sectors)
{
    SET_REG(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("set_multiple_mode() - not RDY\n");
        return -1;
    }

    SET_REG(ATA_NSECTOR, sectors);
    SET_REG(ATA_COMMAND, CMD_SET_MULTIPLE_MODE);

    if (!wait_for_rdy())
    {
        DEBUGF("set_multiple_mode() - CMD failed\n");
        return -2;
    }

    return 0;
}

static int set_features(void)
{
    static struct {
        unsigned char id_word;
        unsigned char id_bit;
        unsigned char subcommand;
        unsigned char parameter;
    } features[] = {
        { 83, 14, 0x03, 0 },   /* force PIO mode */
        { 83, 3, 0x05, 0x80 }, /* adv. power management: lowest w/o standby */
        { 83, 9, 0x42, 0x80 }, /* acoustic management: lowest noise */
        { 82, 6, 0xaa, 0 },    /* enable read look-ahead */
    };
    int i;
    int pio_mode = 2;

    /* Find out the highest supported PIO mode */
    if(identify_info[64] & 2)
        pio_mode = 4;
    else
        if(identify_info[64] & 1)
            pio_mode = 3;

    /* Update the table: set highest supported pio mode that we also support */
    features[0].parameter = 8 + pio_mode;

    SET_REG(ATA_SELECT, ata_device);

    if (!wait_for_rdy()) {
        DEBUGF("set_features() - not RDY\n");
        return -1;
    }

    for (i=0; i < (int)(sizeof(features)/sizeof(features[0])); i++) {
        if (identify_info[features[i].id_word] & (1u << features[i].id_bit)) {
            SET_REG(ATA_FEATURE, features[i].subcommand);
            SET_REG(ATA_NSECTOR, features[i].parameter);
            SET_REG(ATA_COMMAND, CMD_SET_FEATURES);

            if (!wait_for_rdy()) {
                DEBUGF("set_features() - CMD failed\n");
                return -10 - i;
            }

            if((ATA_ALT_STATUS & STATUS_ERR) && (i != 1)) {
                /* some CF cards don't like advanced powermanagement
                   even if they mark it as supported - go figure... */
                if(ATA_ERROR & ERROR_ABRT) {
                    return -20 - i;
                }
            }
        }
    }

#ifdef ATA_SET_DEVICE_FEATURES
    ata_set_pio_timings(pio_mode);
#endif

    return 0;
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

static int init_and_check(bool hard_reset)
{
    int rc;

    if (hard_reset)
    {
        /* This should reset both master and slave, we don't yet know what's in */
        ata_device = 0;
        if (ata_hard_reset())
            return -1;
    }

    rc = master_slave_detect();
    if (rc)
        return -10 + rc;

    /* symptom fix: else check_registers() below may fail */
    if (hard_reset && !wait_for_bsy())
        return -20;

    rc = check_registers();
    if (rc)
        return -30 + rc;
    
    return 0;
}

int ata_init(void)
{
    int rc = 0;
    bool coldstart;

    if ( !initialized ) {
        mutex_init(&ata_mtx);
        queue_init(&ata_queue, true);
    }

    mutex_lock(&ata_mtx);

    /* must be called before ata_device_init() */
    coldstart = ata_is_coldstart();
    ata_led(false);
    ata_device_init();
    sleeping = false;
    ata_enable(true);
#ifdef MAX_PHYS_SECTOR_SIZE
    memset(&sector_cache, 0, sizeof(sector_cache));
#endif

    if ( !initialized ) {
        /* First call won't have multiple thread contention - this
         * may return at any point without having to unlock */
        mutex_unlock(&ata_mtx);

        if (!ide_powered()) /* somebody has switched it off */
        {
            ide_power_enable(true);
            sleep(HZ/4); /* allow voltage to build up */
        }

        /* first try, hard reset at cold start only */
        rc = init_and_check(coldstart);

        if (rc)
        {   /* failed? -> second try, always with hard reset */
            DEBUGF("ata: init failed, retrying...\n");
            rc  = init_and_check(true);
            if (rc)
                return rc;
        }

        rc = identify();

        if (rc)
            return -40 + rc;

        multisectors = identify_info[47] & 0xff;
        if (multisectors == 0) /* Invalid multisector info, try with 16 */
            multisectors = 16;

        DEBUGF("ata: %d sectors per ata request\n",multisectors);

#ifdef MAX_PHYS_SECTOR_SIZE
        /* Find out the physical sector size */
        if((identify_info[106] & 0xe000) == 0x6000)
            phys_sector_mult = 1 << (identify_info[106] & 0x000f);
        else
            phys_sector_mult = 1;

        DEBUGF("ata: %d logical sectors per phys sector", phys_sector_mult);

        if (phys_sector_mult > (MAX_PHYS_SECTOR_SIZE/SECTOR_SIZE))
            panicf("Unsupported physical sector size: %d",
                   phys_sector_mult * SECTOR_SIZE);
#endif

        total_sectors = identify_info[60] | (identify_info[61] << 16);

#ifdef HAVE_LBA48
        if (identify_info[83] & 0x0400       /* 48 bit address support */
            && total_sectors == 0x0FFFFFFF)  /* and disk size >= 128 GiB */
        {                                    /* (needs BigLBA addressing) */
            if (identify_info[102] || identify_info[103])
                panicf("Unsupported disk size: >= 2^32 sectors");
                
            total_sectors = identify_info[100] | (identify_info[101] << 16);
            lba48 = true; /* use BigLBA */
        }
#endif
        rc = freeze_lock();

        if (rc)
            return -50 + rc;

        rc = set_features();
        if (rc)
            return -60 + rc;

        mutex_lock(&ata_mtx); /* Balance unlock below */

        last_disk_activity = current_tick;
#ifdef ATA_DRIVER_CLOSE
        ata_thread_id =
#endif
        create_thread(ata_thread, ata_stack,
                      sizeof(ata_stack), 0, ata_thread_name
                      IF_PRIO(, PRIORITY_USER_INTERFACE)
		              IF_COP(, CPU));
        initialized = true;

    }
    rc = set_multiple_mode(multisectors);
    if (rc)
        rc = -70 + rc;

    mutex_unlock(&ata_mtx);
    return rc;
}

#ifdef ATA_DRIVER_CLOSE
void ata_close(void)
{
    unsigned int thread_id = ata_thread_id;
    
    if (thread_id == 0)
        return;

    ata_thread_id = 0;

    queue_post(&ata_queue, Q_CLOSE, 0);
    thread_wait(thread_id);
}
#endif /* ATA_DRIVER_CLOSE */

#if (CONFIG_LED == LED_REAL)
void ata_set_led_enabled(bool enabled) 
{
    ata_led_enabled = enabled;
    if (ata_led_enabled)
        led(ata_led_on);
    else
        led(false);
}
#endif

long ata_last_disk_activity(void)
{
    return last_disk_activity;
}

int ata_spinup_time(void)
{
    return spinup_time;
}

#ifdef STORAGE_GET_INFO
void ata_get_info(struct storage_info *info)
{
    unsigned short *src,*dest;
    static char vendor[8];
    static char product[16];
    static char revision[4];
    int i;
    info->sector_size = SECTOR_SIZE;
    info->num_sectors= total_sectors;

    src = (unsigned short*)&identify_info[27];
    dest = (unsigned short*)vendor;
    for (i=0;i<4;i++)
        dest[i] = htobe16(src[i]);
    info->vendor=vendor;

    src = (unsigned short*)&identify_info[31];
    dest = (unsigned short*)product;
    for (i=0;i<8;i++)
        dest[i] = htobe16(src[i]);
    info->product=product;

    src = (unsigned short*)&identify_info[23];
    dest = (unsigned short*)revision;
    for (i=0;i<2;i++)
        dest[i] = htobe16(src[i]);
    info->revision=revision;
}
#endif
