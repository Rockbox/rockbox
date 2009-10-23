/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "ata_idle_notify.h"
#include "system.h"
#include <string.h>
#include "thread.h"
#include "disk.h"
#include "storage.h"
#include "panic.h"
#include "usb.h"
#include "ftl-target.h"
#include "nand-target.h"

/* for compatibility */
long last_disk_activity = -1;

/** static, private data **/ 
static bool initialized = false;

static long nand_stack[20];

/* API Functions */

int nand_read_sectors(IF_MD2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{
    int rc = ftl_read(start, incount, inbuf);
    last_disk_activity = current_tick;
    return rc;
}

int nand_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
    int rc = ftl_write(start, count, outbuf);
    last_disk_activity = current_tick;
    return rc;
}

void nand_spindown(int seconds)
{
    (void)seconds;
}

void nand_sleep(void)
{
    nand_power_down();
}

void nand_sleepnow(void)
{
    nand_power_down();
}

void nand_spin(void)
{
    last_disk_activity = current_tick;
    nand_power_up();
}

void nand_enable(bool on)
{
    (void)on;
}

void nand_get_info(IF_MD2(int drive,) struct storage_info *info)
{
    uint32_t ppb = ftl_banks * (*ftl_nand_type).pagesperblock;
    (*info).sector_size = SECTOR_SIZE;
    (*info).num_sectors = (*ftl_nand_type).userblocks * ppb;
    (*info).vendor = "Apple";
    (*info).product = "iPod Nano 2G";
    (*info).revision = "1.0";
}

long nand_last_disk_activity(void)
{
    return last_disk_activity;
}

#ifdef HAVE_STORAGE_FLUSH
int nand_flush(void)
{
    last_disk_activity = current_tick;
    int rc = ftl_sync();
    if (rc != 0) panicf("Failed to unmount flash: %X", rc);
    return rc;
}
#endif

static void nand_thread(void)
{
    while (1)
    {
        if (TIME_AFTER(current_tick, last_disk_activity + HZ / 5))
            nand_power_down();
        sleep(HZ / 10);
    }
}

int nand_init(void)
{
    if (ftl_init()) return 1;

    last_disk_activity = current_tick;

    create_thread(nand_thread, nand_stack,
                  sizeof(nand_stack), 0, "nand"
                  IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));

    initialized = true;
    return 0;
}

#ifdef CONFIG_STORAGE_MULTI
int nand_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;
    
    return 1;
}
#endif
