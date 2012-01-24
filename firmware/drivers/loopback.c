/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include <file.h>
#include <disk.h>

#include "storage.h"
#include "kernel.h"

//#define LOGF_ENABLE
#include "logf.h"

#define SECTOR_SIZE 512

static char loopback_file[MAX_PATH];
static int loopback_fd=-1;
static size_t sectors;
static int driveno;


long last_disk_activity = -1;

int loopback_read_sectors(IF_MD2(int drive,)
                     unsigned long start,
                     int count,
                     void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    logf("loopback_read_sectors(%d,%d)",start,count);
    if(start+count>sectors)
    {
        logf("beyond end of disk");
        return -1;
    }
    if(lseek(loopback_fd,start*SECTOR_SIZE,SEEK_SET)<0)
    {
        logf("lseek failed");
        return -1;
    }
    if(read(loopback_fd,buf,count*SECTOR_SIZE)<0)
    {
        logf("read failed");
        return -1;
    }
    return 0;
}

int loopback_write_sectors(IF_MD2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    if(start+count>sectors)
    {
        return -1;
    }
    if(lseek(loopback_fd,start*SECTOR_SIZE,SEEK_SET)<0)
    {
        return -1;
    }
    if(write(loopback_fd,buf,count*SECTOR_SIZE)<0)
    {
        return -1;
    }
    return 0;
}

int loopback_set_file(const char *newfile)
{
    strlcpy(loopback_file,newfile,MAX_PATH);
    loopback_fd=open(loopback_file,O_RDWR);
    if(loopback_fd<0)
        return -1;
    sectors=filesize(loopback_fd)/SECTOR_SIZE;
    if(disk_mount(driveno)==0)
        return -2;
    queue_broadcast(SYS_FS_CHANGED, 0);
    return 0;
}

void loopback_clear_file(void)
{
    disk_unmount(1); /* release "by force" */
    queue_broadcast(SYS_FS_CHANGED, 0);
    loopback_fd=-1;
}

int loopback_init(void)
{
    loopback_fd=-1;
    return 0;
}

long loopback_last_disk_activity(void)
{
    return last_disk_activity;
}

void loopback_sleep(void)
{
}

void loopback_spin(void)
{
}

void loopback_sleepnow(void)
{
}

void loopback_enable(bool on)
{
    (void)on;
}

bool loopback_disk_is_active(void)
{
    return true;
}

int loopback_soft_reset(void)
{
    return 0;
}

int loopback_spinup_time(void)
{
    return 0;
}

void loopback_spindown(int seconds)
{
    (void)seconds;
}
#ifdef STORAGE_GET_INFO
void loopback_get_info(IF_MD2(int drive,) struct storage_info *info)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif
    /* firmware version */
    info->revision="0.00";

    /* vendor field, need better name? */
    info->vendor="Rockbox";
    /* model field, need better name? */
    info->product=strrchr(loopback_file,'/')+1;

    /* blocks count */
    info->num_sectors=sectors;
    info->sector_size=SECTOR_SIZE;
}
#endif

#ifdef CONFIG_STORAGE_MULTI
int loopback_num_drives(int first_drive)
{
    driveno=first_drive;
    
    return 1;
}
#endif

bool loopback_present(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    (void)drive;
#endif
    return (loopback_fd>=0);
}

void card_enable_monitoring(bool enable)
{
    (void)enable;
}
