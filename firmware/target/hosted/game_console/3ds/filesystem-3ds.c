/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#define RB_FILESYSTEM_OS
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>

#include <3ds/archive.h>

#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "mv.h"
#include "debug.h"
#include "pathfuncs.h"
#include "string-extra.h"
#include "sys_fs_async_3ds.h"

void sys_dir_set_ui_thread(void); /* sys_dir_3ds.c */

void paths_init(void)
{
    /* kernel is up: move blocking FS calls off the Rockbox threads */
    sys_fs_async_init();

    /* main thread == UI thread, see sys_dir_3ds.c */
    sys_dir_set_ui_thread();

    mkdir(ROCKBOX_DIR __MKDIR_MODE_ARG);
    mkdir(ROCKBOX_DIR "/backdrops" __MKDIR_MODE_ARG);
    mkdir(ROCKBOX_DIR "/eqs" __MKDIR_MODE_ARG);
    mkdir(ROCKBOX_DIR "/icons" __MKDIR_MODE_ARG);
    mkdir(ROCKBOX_DIR "/rocks.data" __MKDIR_MODE_ARG);
    mkdir(ROCKBOX_DIR "/themes" __MKDIR_MODE_ARG);
    mkdir(ROCKBOX_DIR "/wps" __MKDIR_MODE_ARG);
}

/* only sdcard volume is accesible to the user  */
void volume_size(IF_MV(int volume,) sector_t *sizep, sector_t *freep)
{
    sector_t size = 0, free = 0;

    FS_ArchiveResource sdmcRSRC;
    FSUSER_GetSdmcArchiveResource(&sdmcRSRC);

    size = (sdmcRSRC.totalClusters * sdmcRSRC.clusterSize) / sdmcRSRC.sectorSize;
    free = (sdmcRSRC.freeClusters * sdmcRSRC.clusterSize) / sdmcRSRC.sectorSize;

    if (sizep)
        *sizep = size;

    if (freep)
        *freep = free;
}
