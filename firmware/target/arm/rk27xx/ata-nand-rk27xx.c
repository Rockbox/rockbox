/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "config.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "disk.h"
#include "storage.h"
#include "panic.h"
#include "usb.h"
#include "ftl-target.h"
#include "nand-target.h"

/** static, private data **/
static bool initialized = false;

/* The NAND is FTL_NUM_DRIVES drives - USER, and SYS when the target exposes
 * it. storage.c hands us a drive index relative to our first, which is the
 * FTL_DRIVE_* number. */
#ifdef HAVE_MULTIDRIVE
#define NAND_DRIVE(d)  (d)
#else
#define NAND_DRIVE(d)  FTL_DRIVE_USER
#endif

/* API Functions */
int nand_read_sectors(IF_MD(int drive,) sector_t start, int incount,
                     void* inbuf)
{
    return ftl_read(NAND_DRIVE(IF_MD_DRV(drive)), start, incount, inbuf);
}

int nand_write_sectors(IF_MD(int drive,) sector_t start, int count,
                      const void* outbuf)
{
    return ftl_write(NAND_DRIVE(IF_MD_DRV(drive)), start, count, outbuf);
}

void nand_spindown(int seconds)
{
    (void)seconds;
}

void nand_sleepnow(void)
{
    nand_power_down();
}

void nand_spin(void)
{
    nand_set_active();
}

void nand_enable(bool on)
{
    (void)on;
}

void nand_get_info(IF_MD(int drive,) struct storage_info *info)
{
    int d = NAND_DRIVE(IF_MD_DRV(drive));

    /* Capacity comes from the FTL's own tables, not from raw block
     * geometry: Scheme A's usable size depends on the per-zone valid-block
     * counts and on the system/user split recorded in ID block 1. */
    (*info).sector_size = SECTOR_SIZE;
    (*info).num_sectors = ftl_get_sectors(d);
    (*info).vendor = "Rockchip";
#ifdef HAVE_RK27XX_NAND_SYS
    (*info).product = (d == FTL_DRIVE_SYS) ? "NAND SYS" : "NAND USER";
#else
    (*info).product = "NAND USER";
#endif
    (*info).revision = "1.0";
}

long nand_last_disk_activity(void)
{
    return nand_last_activity();
}

#ifdef HAVE_STORAGE_FLUSH
int nand_flush(void)
{
    int rc = ftl_sync();
    if (rc != 0) panicf("Failed to unmount flash: %X", rc);
    return rc;
}
#endif

int nand_init(void)
{
    if (ftl_init()) return 1;

    initialized = true;
    return 0;
}

#ifdef CONFIG_STORAGE_MULTI
int nand_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;

#ifdef HAVE_MULTIDRIVE
    return FTL_NUM_DRIVES;
#else
    return 1;
#endif
}
#endif

int nand_event(long id, intptr_t data)
{
    int rc = 0;

#if 0 /* The NAND functions do nothing right now; just provide template */
    if (LIKELY(id == Q_STORAGE_TICK))
    {
        if (!nand_powered ||
            TIME_BEFORE(current_tick, nand_last_activity() + HZ / 5))
        {
            STG_EVENT_ASSERT_ACTIVE(STORAGE_NAND);
        }
    }
    else if (id == Q_STORAGE_SLEEPNOW)
    {
        nand_power_down();
    }
    else
    {
        rc = storage_event_default_handler(id, data, nand_last_activity(),
                                           STORAGE_NAND);
    }
#endif
    return rc;
    (void)id; (void)data;
}
