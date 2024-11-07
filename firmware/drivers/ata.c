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

//#define LOGF_ENABLE

#include <stdbool.h>
#include <inttypes.h>
#include "led.h"
#include "cpu.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "power.h"
#include "string.h"
#include "ata-driver.h"
#include "ata-defines.h"
#include "fs_defines.h"
#include "storage.h"
#include "logf.h"

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
#define CMD_WRITE_MULTIPLE_EXT     0x39
#define CMD_SET_MULTIPLE_MODE      0xC6
#ifdef HAVE_ATA_SMART
#define CMD_SMART                  0xB0
#endif
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2
#define CMD_IDENTIFY               0xEC
#define CMD_SLEEP                  0xE6
#define CMD_FLUSH_CACHE            0xE7
#define CMD_FLUSH_CACHE_EXT        0xEA
#define CMD_SET_FEATURES           0xEF
#define CMD_SECURITY_FREEZE_LOCK   0xF5
#ifdef HAVE_ATA_DMA
#define CMD_READ_DMA               0xC8
#define CMD_READ_DMA_EXT           0x25
#define CMD_WRITE_DMA              0xCA
#define CMD_WRITE_DMA_EXT          0x35
#endif

#define READWRITE_TIMEOUT 5*HZ

#ifdef HAVE_ATA_POWER_OFF
#define ATA_POWER_OFF_TIMEOUT 2*HZ
#endif

#if defined(HAVE_USBSTACK)
#define ATA_ACTIVE_IN_USB 1
#else
#define ATA_ACTIVE_IN_USB 0
#endif

enum {
    ATA_BOOT = -1,
    ATA_OFF,
    ATA_SLEEPING,
    ATA_SPINUP,
    ATA_ON,
};

static int ata_state = ATA_BOOT;

static struct mutex ata_mutex SHAREDBSS_ATTR;
static int ata_device; /* device 0 (master) or 1 (slave) */

static int spinup_time = 0;
#if (CONFIG_LED == LED_REAL)
static bool ata_led_enabled = true;
static bool ata_led_on = false;
#endif

static long sleep_timeout = 5*HZ;
#ifdef HAVE_LBA48
static bool ata_lba48 = false; /* set for 48 bit addressing */
#endif
static bool canflush = true;

static long last_disk_activity = -1;
#ifdef HAVE_ATA_POWER_OFF
static long power_off_tick = 0;
#endif

static sector_t total_sectors;
static uint32_t log_sector_size;
static int multisectors; /* number of supported multisectors */

static unsigned short identify_info[ATA_IDENTIFY_WORDS] STORAGE_ALIGN_ATTR;

#ifdef HAVE_ATA_DMA
static int dma_mode = 0;
#endif

#ifdef HAVE_ATA_POWER_OFF
static int ata_power_on(void);
#endif
static int perform_soft_reset(void);
static int set_multiple_mode(int sectors);
static int set_features(void);

static inline void keep_ata_active(void)
{
    last_disk_activity = current_tick;
}

static inline bool ata_sleep_timed_out(void)
{
    return sleep_timeout &&
           TIME_AFTER(current_tick, last_disk_activity + sleep_timeout);
}

static inline bool ata_power_off_timed_out(void)
{
#ifdef HAVE_ATA_POWER_OFF
    return power_off_tick && TIME_AFTER(current_tick, power_off_tick);
#else
    return false;
#endif
}

#ifndef ATA_TARGET_POLLING
static ICODE_ATTR int wait_for_bsy(void)
{
    long timeout = current_tick + HZ*30;

    do
    {
        if (!(ATA_IN8(ATA_STATUS) & STATUS_BSY))
            return 1;
        keep_ata_active();
        yield();
    } while (TIME_BEFORE(current_tick, timeout));

    return 0; /* timeout */
}

static ICODE_ATTR int wait_for_rdy(void)
{
    long timeout;

    if (!wait_for_bsy())
        return 0;

    timeout = current_tick + HZ*10;

    do
    {
        if (ATA_IN8(ATA_ALT_STATUS) & STATUS_RDY)
            return 1;
        keep_ata_active();
        yield();
    } while (TIME_BEFORE(current_tick, timeout));

    return 0; /* timeout */
}
#else
#define wait_for_bsy    ata_wait_for_bsy
#define wait_for_rdy    ata_wait_for_rdy
#endif

static int ata_perform_wakeup(int state)
{
    logf("ata WAKE %ld", current_tick);
    if (state > ATA_OFF) {
        if (perform_soft_reset()) {
            return -1;
        }
    }
#ifdef HAVE_ATA_POWER_OFF
    else {
        if (ata_power_on()) {
            return -2;
        }
    }
#endif

    return 0;
}

static int ata_perform_sleep(void)
{
    /* If device doesn't support PM features, don't try to sleep. */
    if (!ata_disk_can_sleep())
        return 0; // XXX or return a failure?

    logf("ata SLEEP %ld", current_tick);

    ATA_OUT8(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("ata_perform_sleep() - not RDY\n");
        return -1;
    }

    /* STANDBY IMMEDIATE
        - writes all cached data
        - transitions to PM2:Standby
        - enters Standby_z power condition

      This places the device into a state where power-off is safe.  We
      will cut power at a later time.
    */
    ATA_OUT8(ATA_COMMAND, CMD_STANDBY_IMMEDIATE);

    if (!wait_for_rdy()) {
        DEBUGF("ata_perform_sleep() - CMD failed\n");
        return -2;
    }

    return 0;
}

static int ata_perform_flush_cache(void)
{
    uint8_t cmd;

    if (!canflush) {
        return 0;
    } else if (ata_lba48 && identify_info[83] & (1 << 13)) {
        cmd = CMD_FLUSH_CACHE_EXT;  /* Flag, optional, ATA-6 and up, for use with LBA48 devices */
    } else if (identify_info[83] & (1 << 12)) {
        cmd = CMD_FLUSH_CACHE; /* Flag, mandatory, ATA-6 and up */
    } else if (identify_info[80] >= (1 << 5)) { /* Use >= instead of '&' because bits lower than the latest standard we support don't have to be set */
        cmd = CMD_FLUSH_CACHE; /* No flag, mandatory, ATA-5  (Optional for ATA-4) */
    } else {
        /* If neither (mandatory!) command is supported
           then don't issue it. */
       canflush = 0;
       return 0;
    }

    logf("ata FLUSH CACHE %ld", current_tick);

    ATA_OUT8(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("ata_perform_flush_cache() - not RDY\n");
        return -1;
    }

    ATA_OUT8(ATA_COMMAND, cmd);

    if (!wait_for_rdy()) {
        DEBUGF("ata_perform_flush_cache() - CMD failed\n");
        return -2;
    }

    return 0;
}

int ata_flush(void)
{
    if (ata_state >= ATA_SPINUP) {
        mutex_lock(&ata_mutex);
        ata_perform_flush_cache();
        mutex_unlock(&ata_mutex);
    }
    return 0;
}

static ICODE_ATTR int wait_for_start_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;

    return (ATA_IN8(ATA_ALT_STATUS) & (STATUS_BSY|STATUS_DRQ)) == STATUS_DRQ;
}

static ICODE_ATTR int wait_for_end_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_IN8(ATA_ALT_STATUS) &
            (STATUS_BSY|STATUS_RDY|STATUS_DF|STATUS_DRQ|STATUS_ERR))
           == STATUS_RDY;
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
static ICODE_ATTR void copy_read_sectors(unsigned char* buf, int wordcount)
{
    unsigned short tmp = 0;

    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned char* bufend = buf + wordcount*2;
        do
        {
            tmp = ATA_IN16(ATA_DATA);
#if defined(ROCKBOX_LITTLE_ENDIAN)
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
            *wbuf = ATA_IN16(ATA_DATA);
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
}
#endif /* !ATA_OPTIMIZED_READING */

#ifndef ATA_OPTIMIZED_WRITING
static ICODE_ATTR void copy_write_sectors(const unsigned char* buf,
                                              int wordcount)
{
    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned short tmp = 0;
        const unsigned char* bufend = buf + wordcount*2;
        do
        {
#if defined(ROCKBOX_LITTLE_ENDIAN)
            tmp = (unsigned short) *buf++;
            tmp |= (unsigned short) *buf++ << 8;
#else
            tmp = (unsigned short) *buf++ << 8;
            tmp |= (unsigned short) *buf++;
#endif
            ATA_OUT16(ATA_DATA, tmp);
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {
            ATA_OUT16(ATA_DATA, *wbuf);
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
}
#endif /* !ATA_OPTIMIZED_WRITING */

static int ata_transfer_sectors(uint64_t start,
                                int incount,
                                void* inbuf,
                                int write)
{
    int ret = 0;
    long timeout;
    int count;
    void* buf;
    long spinup_start = spinup_start;
#ifdef HAVE_ATA_DMA
    bool usedma = false;
#endif

    if (start + incount > total_sectors) {
        ret = -1;
        goto error;
    }

    keep_ata_active();

    ata_led(true);

    if (ata_state < ATA_ON) {
        spinup_start = current_tick;
        int state = ata_state;
        ata_state = ATA_SPINUP;
        if (ata_perform_wakeup(state)) {
            ret = -2;
            goto error;
        }
    }

    timeout = current_tick + READWRITE_TIMEOUT;

    ATA_OUT8(ATA_SELECT, ata_device);
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
        keep_ata_active();

#ifdef HAVE_ATA_DMA
        /* If DMA is supported and parameters are ok for DMA, use it */
        if (dma_mode && ata_dma_setup(inbuf, incount * log_sector_size, write))
            usedma = true;
#endif

#ifdef HAVE_LBA48
        if (ata_lba48)
        {
            ATA_OUT8(ATA_NSECTOR, count >> 8);
            ATA_OUT8(ATA_NSECTOR, count & 0xff);
            ATA_OUT8(ATA_SECTOR, (start >> 24) & 0xff); /* 31:24 */
            ATA_OUT8(ATA_SECTOR, start & 0xff); /* 7:0 */
            ATA_OUT8(ATA_LCYL, (start >> 32) & 0xff); /* 39:32 */
            ATA_OUT8(ATA_LCYL, (start >> 8) & 0xff); /* 15:8 */
            ATA_OUT8(ATA_HCYL, (start >> 40) & 0xff); /* 47:40 */
            ATA_OUT8(ATA_HCYL, (start >> 16) & 0xff); /* 23:16 */
            ATA_OUT8(ATA_SELECT, SELECT_LBA | ata_device);
#ifdef HAVE_ATA_DMA
            if (write)
                ATA_OUT8(ATA_COMMAND, usedma ? CMD_WRITE_DMA_EXT : CMD_WRITE_MULTIPLE_EXT);
            else
                ATA_OUT8(ATA_COMMAND, usedma ? CMD_READ_DMA_EXT : CMD_READ_MULTIPLE_EXT);
#else
            ATA_OUT8(ATA_COMMAND, write ? CMD_WRITE_MULTIPLE_EXT : CMD_READ_MULTIPLE_EXT);
#endif
        }
        else
#endif
        {
            ATA_OUT8(ATA_NSECTOR, count & 0xff); /* 0 means 256 sectors */
            ATA_OUT8(ATA_SECTOR, start & 0xff);
            ATA_OUT8(ATA_LCYL, (start >> 8) & 0xff);
            ATA_OUT8(ATA_HCYL, (start >> 16) & 0xff);
            ATA_OUT8(ATA_SELECT, ((start >> 24) & 0xf) | SELECT_LBA | ata_device);  /* LBA28, mask off upper 4 bits of 32-bit sector address */
#ifdef HAVE_ATA_DMA
            if (write)
                ATA_OUT8(ATA_COMMAND, usedma ? CMD_WRITE_DMA : CMD_WRITE_MULTIPLE);
            else
                ATA_OUT8(ATA_COMMAND, usedma ? CMD_READ_DMA : CMD_READ_MULTIPLE);
#else
            ATA_OUT8(ATA_COMMAND, write ? CMD_WRITE_MULTIPLE : CMD_READ_MULTIPLE);
#endif
        }

        /* wait at least 400ns between writing command and reading status */
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");

#ifdef HAVE_ATA_DMA
        if (usedma) {
            if (!ata_dma_finish())
                ret = -7;

            if (ret != 0) {
                perform_soft_reset();
                goto retry;
            }

            if (ata_state == ATA_SPINUP) {
                ata_state = ATA_ON;
                spinup_time = current_tick - spinup_start;
            }
        }
        else
#endif /* HAVE_ATA_DMA */
        {
            while (count) {
                int sectors;
                int wordcount;
                int status;
                int error;

                if (!wait_for_start_of_transfer()) {
                    /* We have timed out waiting for RDY and/or DRQ, possibly
                       because the hard drive is shaking and has problems
                       reading the data. We have two options:
                       1) Wait some more
                       2) Perform a soft reset and try again.

                       We choose alternative 2.
                    */
                    perform_soft_reset();
                    ret = -5;
                    goto retry;
                }

                if (ata_state == ATA_SPINUP) {
                    ata_state = ATA_ON;
                    spinup_time = current_tick - spinup_start;
                }

                /* read the status register exactly once per loop */
                status = ATA_IN8(ATA_STATUS);
                error = ATA_IN8(ATA_ERROR);

                if (count >= multisectors)
                    sectors = multisectors;
                else
                    sectors = count;

                wordcount = sectors * log_sector_size / 2;

                if (write)
                    copy_write_sectors(buf, wordcount);
                else
                    copy_read_sectors(buf, wordcount);

                /*
                  "Device errors encountered during READ MULTIPLE commands
                  are posted at the beginning of the block or partial block
                  transfer, but the DRQ bit is still set to one and the data
                  transfer shall take place, including transfer of corrupted
                  data, if any."
                    -- ATA specification
                */
                if ( status & (STATUS_BSY | STATUS_ERR | STATUS_DF) ) {
                    perform_soft_reset();
                    ret = -6;
                    /* no point retrying IDNF, sector no. was invalid */
                    if (error & ERROR_IDNF)
                        break;
                    goto retry;
                }

                buf += sectors * log_sector_size; /* Advance one chunk of sectors */
                count -= sectors;

                keep_ata_active();
            }
        }

        if(!ret && !wait_for_end_of_transfer()) {
            int error;

            error = ATA_IN8(ATA_ERROR);
            perform_soft_reset();
            ret = -4;
            /* no point retrying IDNF, sector no. was invalid */
            if (error & ERROR_IDNF)
                break;
            goto retry;
        }
        break;
    }

  error:
    ata_led(false);

    if (ret < 0 && ata_state == ATA_SPINUP) {
        /* bailed out before updating */
        ata_state = ATA_ON;
    }

    return ret;
}

#include "ata-common.c"

#ifndef MAX_PHYS_SECTOR_SIZE
int ata_read_sectors(IF_MD(int drive,)
                     sector_t start,
                     int incount,
                     void* inbuf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif

    mutex_lock(&ata_mutex);
    int rc = ata_transfer_sectors(start, incount, inbuf, false);
    mutex_unlock(&ata_mutex);
    return rc;
}

int ata_write_sectors(IF_MD(int drive,)
                      sector_t start,
                      int count,
                      const void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif

    mutex_lock(&ata_mutex);
    int rc = ata_transfer_sectors(start, count, (void*)buf, true);
    mutex_unlock(&ata_mutex);
    return rc;
}
#endif /* ndef MAX_PHYS_SECTOR_SIZE */

static int STORAGE_INIT_ATTR check_registers(void)
{
    int i;
    wait_for_bsy();
    if (ATA_IN8(ATA_STATUS) & STATUS_BSY)
            return -1;

    for (i = 0; i<64; i++) {
        ATA_OUT8(ATA_NSECTOR, TEST_PATTERN1);
        ATA_OUT8(ATA_SECTOR,  TEST_PATTERN2);
        ATA_OUT8(ATA_LCYL,    TEST_PATTERN3);
        ATA_OUT8(ATA_HCYL,    TEST_PATTERN4);

        if (((ATA_IN8(ATA_NSECTOR) & 0xff) == TEST_PATTERN1) &&
            ((ATA_IN8(ATA_SECTOR) & 0xff) == TEST_PATTERN2) &&
            ((ATA_IN8(ATA_LCYL) & 0xff) == TEST_PATTERN3) &&
            ((ATA_IN8(ATA_HCYL) & 0xff) == TEST_PATTERN4))
            return 0;

        sleep(1);
    }
    return -2;
}

static int freeze_lock(void)
{
    /* does the disk support Security Mode feature set? */
    if (identify_info[82] & 2)
    {
        ATA_OUT8(ATA_SELECT, ata_device);

        if (!wait_for_rdy())
            return -1;

        ATA_OUT8(ATA_COMMAND, CMD_SECURITY_FREEZE_LOCK);

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
    return (ata_state >= ATA_SPINUP);
}

void ata_sleepnow(void)
{
    if (ata_state >= ATA_SPINUP) {
        logf("ata SLEEPNOW %ld", current_tick);
        mutex_lock(&ata_mutex);
        if (ata_state == ATA_ON) {
            if (!ata_perform_flush_cache() && !ata_perform_sleep()) {
                ata_state = ATA_SLEEPING;
#ifdef HAVE_ATA_POWER_OFF
                if (ata_disk_can_sleep() || canflush) {
                    power_off_tick = current_tick + ATA_POWER_OFF_TIMEOUT;
                }
#endif
            }
        }
        mutex_unlock(&ata_mutex);
    }
}

void ata_spin(void)
{
    keep_ata_active();
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
#ifdef HAVE_ATA_POWER_OFF
static int ata_hard_reset(void)
#else
static int STORAGE_INIT_ATTR ata_hard_reset(void)
#endif
{
    int ret;

    mutex_lock(&ata_mutex);

    ata_reset();

    /* state HRR2 */
    ATA_OUT8(ATA_SELECT, ata_device); /* select the right device */
    ret = wait_for_bsy();

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    mutex_unlock(&ata_mutex);

    return ret;
}

// not putting this into STORAGE_INIT_ATTR, as ATA spec recommends to
// re-read identify_info after soft reset. So we'll do that.
static int identify(void)
{
    int i;

    ATA_OUT8(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("identify() - not RDY\n");
        return -1;
    }
    ATA_OUT8(ATA_COMMAND, CMD_IDENTIFY);

    if (!wait_for_start_of_transfer())
    {
        DEBUGF("identify() - CMD failed\n");
        return -2;
    }

    for (i=0; i<ATA_IDENTIFY_WORDS; i++) {
        /* the IDENTIFY words are already swapped, so we need to treat
           this info differently that normal sector data */
        identify_info[i] = ATA_SWAP_IDENTIFY(ATA_IN16(ATA_DATA));
    }

    return 0;
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

    logf("ata SOFT RESET %ld", current_tick);

    ATA_OUT8(ATA_SELECT, SELECT_LBA | ata_device );
    ATA_OUT8(ATA_CONTROL, CONTROL_nIEN|CONTROL_SRST );
    sleep(1); /* >= 5us */

#ifdef HAVE_ATA_DMA
    /* DMA requires INTRQ be enabled */
    ATA_OUT8(ATA_CONTROL, 0);
#else
    ATA_OUT8(ATA_CONTROL, CONTROL_nIEN);
#endif
    sleep(1); /* >2ms */

    /* This little sucker can take up to 30 seconds */
    retry_count = 8;
    do
    {
        ret = wait_for_rdy();
    } while(!ret && retry_count--);

    if (!ret)
        return -1;

    if (identify())
        return -5;

    if ((ret = set_features()))
        return -60 + ret;

    if (set_multiple_mode(multisectors))
        return -3;

    if (identify())
        return -2;

    if (freeze_lock())
        return -4;

    return 0;
}

int ata_soft_reset(void)
{
    int ret = -6;

    mutex_lock(&ata_mutex);

    if (ata_state > ATA_OFF) {
        ret = perform_soft_reset();
    }

    mutex_unlock(&ata_mutex);
    return ret;
}

#ifdef HAVE_ATA_POWER_OFF
static int ata_power_on(void)
{
    int rc;

    logf("ata ON %ld", current_tick);

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

    if (identify())
        return -5;

    rc = set_features();
    if (rc)
        return -60 + rc;

    if (set_multiple_mode(multisectors))
        return -3;

    if (identify())
        return -2;

    if (freeze_lock())
        return -4;

    return 0;
}
#endif /* HAVE_ATA_POWER_OFF */

static int STORAGE_INIT_ATTR master_slave_detect(void)
{
    /* master? */
    ATA_OUT8(ATA_SELECT, 0);
    if (ATA_IN8(ATA_STATUS) & (STATUS_RDY|STATUS_BSY)) {
        ata_device = 0;
        DEBUGF("Found master harddisk\n");
    }
    else {
        /* slave? */
        ATA_OUT8(ATA_SELECT, SELECT_DEVICE1);
        if (ATA_IN8(ATA_STATUS) & (STATUS_RDY|STATUS_BSY)) {
            ata_device = SELECT_DEVICE1;
            DEBUGF("Found slave harddisk\n");
        }
        else
            return -1;
    }
    return 0;
}

static int set_multiple_mode(int sectors)
{
    ATA_OUT8(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("set_multiple_mode() - not RDY\n");
        return -1;
    }

    ATA_OUT8(ATA_NSECTOR, sectors);
    ATA_OUT8(ATA_COMMAND, CMD_SET_MULTIPLE_MODE);

    if (!wait_for_rdy())
    {
        DEBUGF("set_multiple_mode() - CMD failed\n");
        return -2;
    }

    return 0;
}

#ifdef HAVE_ATA_DMA
static int ata_get_best_mode(unsigned short identword, int max, int modetype)
{
    unsigned short testbit = BIT_N(max);

    while (1) {
        if (identword & testbit)
            return max | modetype;
        testbit >>= 1;
        if (!testbit)
            return 0;
        max--;
    }
}
#endif

static int set_features(void)
{
    static struct {
        unsigned char id_word;
        unsigned char id_bit;
        unsigned char subcommand;
        unsigned char parameter;
    } features[] = {
        { 83, 14, 0x03, 0 },   /* force PIO mode by default */
#ifdef HAVE_ATA_DMA
        { 0, 0, 0x03, 0 },     /* DMA mode */
#endif
        /* NOTE: Above two MUST come first! */
        { 83, 3, 0x05, 0x80 }, /* adv. power management: lowest w/o standby */
        { 83, 9, 0x42, 0x80 }, /* acoustic management: lowest noise */
        { 82, 5, 0x02, 0 },    /* enable volatile write cache */
        { 82, 6, 0xaa, 0 },    /* enable read look-ahead */
    };
    int i;
    int pio_mode = 2; /* Lowest */

    /* Find out the highest supported PIO mode */
    if (identify_info[53] & (1<<1)) {  /* Is word 64 valid? */
      if (identify_info[64] & 2)
        pio_mode = 4;
      else if(identify_info[64] & 1)
        pio_mode = 3;
    }

    /* Update the table: set highest supported pio mode that we also support */
    features[0].parameter = 8 + pio_mode;

#ifdef HAVE_ATA_DMA
    if (identify_info[53] & (1<<2)) {
        int max_udma = ATA_MAX_UDMA;
#if ATA_MAX_UDMA > 2
        if (!(identify_info[93] & (1<<13)))
            max_udma = 2;
#endif
        /* Ultra DMA mode info present, find a mode */
        dma_mode = ata_get_best_mode(identify_info[88], max_udma, 0x40);
    }

    if (!dma_mode) {
        /* No UDMA mode found, try to find a multi-word DMA mode */
        dma_mode = ata_get_best_mode(identify_info[63], ATA_MAX_MWDMA, 0x20);
        features[1].id_word = 63;
    } else {
        features[1].id_word = 88;
    }

    features[1].id_bit = dma_mode & 7;
    features[1].parameter = dma_mode;
#endif /* HAVE_ATA_DMA */

    ATA_OUT8(ATA_SELECT, ata_device);

    if (!wait_for_rdy()) {
        DEBUGF("set_features() - not RDY\n");
        return -1;
    }

    for (i=0; i < (int)(sizeof(features)/sizeof(features[0])); i++) {
        if (identify_info[features[i].id_word] & BIT_N(features[i].id_bit)) {
            ATA_OUT8(ATA_FEATURE, features[i].subcommand);
            ATA_OUT8(ATA_NSECTOR, features[i].parameter);
            ATA_OUT8(ATA_COMMAND, CMD_SET_FEATURES);

            if (!wait_for_rdy()) {
                DEBUGF("set_features() - CMD failed\n");
                return -10 - i;
            }

            if((ATA_IN8(ATA_ALT_STATUS) & STATUS_ERR) && (features[i].subcommand != 0x05)) {
                /* some CF cards don't like advanced powermanagement
                   even if they mark it as supported - go figure... */
                if(ATA_IN8(ATA_ERROR) & ERROR_ABRT) {
                    return -20 - i;
                }
            }
        }
    }

#ifdef ATA_SET_PIO_TIMING
    ata_set_pio_timings(pio_mode);
#endif

#ifdef HAVE_ATA_DMA
    ata_dma_set_mode(dma_mode);
#endif

    return 0;
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

static int STORAGE_INIT_ATTR init_and_check(bool hard_reset)
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

int STORAGE_INIT_ATTR ata_init(void)
{
    int rc = 0;
    bool coldstart;

    if (ata_state == ATA_BOOT) {
        mutex_init(&ata_mutex);
    }

    mutex_lock(&ata_mutex);

    /* must be called before ata_device_init() */
    coldstart = ata_is_coldstart();
    ata_led(false);
    ata_device_init();
    ata_enable(true);

    if (ata_state == ATA_BOOT) {
        ata_state = ATA_OFF;

        if (!ide_powered()) /* somebody has switched it off */
        {
            ide_power_enable(true);
            sleep(HZ/4); /* allow voltage to build up */
        }

#ifdef HAVE_ATA_DMA
        /* DMA requires INTRQ be enabled */
        ATA_OUT8(ATA_CONTROL, 0);
#endif

        /* first try, hard reset at cold start only */
        rc = init_and_check(coldstart);

        if (rc)
        {   /* failed? -> second try, always with hard reset */
            DEBUGF("ata: init failed, retrying...\n");
            rc  = init_and_check(true);
            if (rc) {
                goto error;
            }
        }

        rc = identify();
        if (rc) {
            rc = -40 + rc;
            goto error;
        }

        multisectors = identify_info[47] & 0xff;
        if (multisectors == 0) /* Invalid multisector info, try with 16 */
            multisectors = 16;

        DEBUGF("ata: %d sectors per ata request\n", multisectors);

        total_sectors = (identify_info[61] << 16) | identify_info[60];

#ifdef HAVE_LBA48
        if (identify_info[83] & 0x0400 && total_sectors == 0x0FFFFFFF) {
            total_sectors = ((uint64_t)identify_info[103] << 48) |
                    ((uint64_t)identify_info[102] << 32) |
                    ((uint64_t)identify_info[101] << 16) |
                    identify_info[100];
            ata_lba48 = true; /* use BigLBA */
        }
#endif /* HAVE_LBA48 */

        /* Logical sector size > 512B ? */
        if ((identify_info[106] & 0xd000) == 0x5000) /* B14, B12 */
            log_sector_size = (identify_info[117] | (identify_info[118] << 16)) * 2;
        else
            log_sector_size = 512;

        rc = freeze_lock();
        if (rc) {
            rc = -50 + rc;
            goto error;
        }

        rc = set_features(); // error codes are between -1 and -49
        if (rc) {
            rc = -60 + rc;
            goto error;
        }

#ifdef MAX_PHYS_SECTOR_SIZE
        rc = ata_get_phys_sector_mult();
        if (rc) {
            rc = -70 + rc;
            goto error;
        }
#endif
        ata_state = ATA_ON;
        keep_ata_active();
    }
    rc = set_multiple_mode(multisectors);
    if (rc)
        rc = -100 + rc;

    rc = identify();
    if (rc) {
        rc = -40 + rc;
        goto error;
    }

error:
    mutex_unlock(&ata_mutex);
    return rc;
}

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
void ata_get_info(IF_MD(int drive,)struct storage_info *info)
{
    unsigned short *src,*dest;
    static char vendor[8];
    static char product[16];
    static char revision[4];
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    int i;

    info->sector_size = log_sector_size;
    info->num_sectors = total_sectors;

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

#ifdef HAVE_ATA_DMA
/* Returns last DMA mode as set by set_features() */
int ata_get_dma_mode(void)
{
    return dma_mode;
}

/* Needed to allow updating while waiting for DMA to complete */
void ata_keep_active(void)
    __attribute__((alias("ata_spin")));
#endif

#ifdef CONFIG_STORAGE_MULTI
int ata_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;

    return 1;
}
#endif

int ata_event(long id, intptr_t data)
{
    int rc = 0;

    /* GCC does a lousy job culling unreachable cases in the default handler
       if statements are in a switch statement, so we'll do it this way. Only
       the first case is frequently hit anyway. */
    if (LIKELY(id == Q_STORAGE_TICK)) {
        /* won't see ATA_BOOT in here */
        if (ata_state != ATA_ON || !ata_sleep_timed_out()) {
#ifdef HAVE_ATA_POWER_OFF
            if (ata_state == ATA_SLEEPING && ata_power_off_timed_out()) {
                power_off_tick = 0;
                mutex_lock(&ata_mutex);
                logf("ata OFF %ld", current_tick);
                ide_power_enable(false);
                ata_state = ATA_OFF;
                mutex_unlock(&ata_mutex);
            }
#endif
            STG_EVENT_ASSERT_ACTIVE(STORAGE_ATA);
        }
    }
    else if (id == Q_STORAGE_SLEEPNOW) {
        ata_sleepnow();
    }
    else if (id == Q_STORAGE_SLEEP) {
        last_disk_activity = current_tick - sleep_timeout + HZ / 5;
    }
#ifndef USB_NONE
    else if (id == SYS_USB_CONNECTED) {
        logf("deq USB %ld", current_tick);
        if (ATA_ACTIVE_IN_USB) {
            /* There is no need to force ATA power on */
            STG_EVENT_ASSERT_ACTIVE(STORAGE_ATA);
        }
        else {
            mutex_lock(&ata_mutex);
            if (ata_state < ATA_ON) {
                ata_led(true);
                if (!(rc = ata_perform_wakeup(ata_state))) {
                    ata_state = ATA_ON;
                }
                ata_led(false);
            }
            mutex_unlock(&ata_mutex);
        }
    }
#endif /* ndef USB_NONE */
    else {
        rc = storage_event_default_handler(id, data, last_disk_activity,
                                           STORAGE_ATA);
    }

    return rc;
}
