/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Frank Gevaerts
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
#include <string.h>

#include "config.h"
#include "fs_defines.h"
#include "storage.h"

#define NUM_SECTORS 16384

static unsigned char ramdisk[SECTOR_SIZE * NUM_SECTORS];

static long last_disk_activity = -1;

int ramdisk_read_sectors(IF_MD(int drive,)
                     sector_t start,
                     int count,
                     void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    if(start+count>NUM_SECTORS)
    {
        return -1;
    }
    memcpy(buf,&ramdisk[start*SECTOR_SIZE],count*SECTOR_SIZE);
    return 0;
}

int ramdisk_write_sectors(IF_MD(int drive,)
                      sector_t start,
                      int count,
                      const void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    if(start+count>NUM_SECTORS)
    {
        return -1;
    }
    memcpy(&ramdisk[start*SECTOR_SIZE],buf,count*SECTOR_SIZE);
    return 0;
}

int ramdisk_init(void)
{
    return 0;
}

long ramdisk_last_disk_activity(void)
{
    return last_disk_activity;
}

void ramdisk_sleep(void)
{
}

void ramdisk_spin(void)
{
}

void ramdisk_sleepnow(void)
{
}

void ramdisk_enable(bool on)
{
    (void)on;
}

bool ramdisk_disk_is_active(void)
{
    return true;
}

int ramdisk_soft_reset(void)
{
    return 0;
}

int ramdisk_spinup_time(void)
{
    return 0;
}

void ramdisk_spindown(int seconds)
{
    (void)seconds;
}
#ifdef STORAGE_GET_INFO
void ramdisk_get_info(IF_MD(int drive,) struct storage_info *info)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    /* firmware version */
    info->revision="0.00";

    /* vendor field, need better name? */
    info->vendor="Rockbox";
    /* model field, need better name? */
    info->product="Ramdisk";

    /* blocks count */
    info->num_sectors=NUM_SECTORS;
    info->sector_size=SECTOR_SIZE;
}
#endif

#ifdef CONFIG_STORAGE_MULTI
int ramdisk_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;

    return 1;
}
#endif

#ifdef HAVE_HOTSWAP
bool ramdisk_removable(IF_MD(int drive))
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif

    return false;
}

bool ramdisk_present(IF_MD(int drive))
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif

    return true;
}
#endif

int ramdisk_event(long id, intptr_t data)
{
    return storage_event_default_handler(id, data, last_disk_activity,
                                         STORAGE_RAMDISK);
}
