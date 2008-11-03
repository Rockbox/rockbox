/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ramdisk.c 18965 2008-11-01 17:33:21Z gevaerts $
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

#include "storage.h"

#define SECTOR_SIZE 512
#define NUM_SECTORS 16384

unsigned char ramdisk[SECTOR_SIZE * NUM_SECTORS];

long last_disk_activity = -1;

int ramdisk_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int count,
                     void* buf)
{
    if(start+count>=NUM_SECTORS)
    {
        return -1;
    }
    memcpy(buf,&ramdisk[start*SECTOR_SIZE],count*SECTOR_SIZE);
    return 0;
}

int ramdisk_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    if(start+count>=NUM_SECTORS)
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

void ramdisk_spindown(int seconds)
{
    (void)seconds;
}
#ifdef STORAGE_GET_INFO
void ramdisk_get_info(struct storage_info *info)
{
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

