/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "mv.h"
#include "debug.h"
#include "pathfuncs.h"
#include "string-extra.h"

#include <3ds/archive.h>

void paths_init(void)
{
    /* is this needed in 3DS? */
#if 0
    char config_dir[MAX_PATH];

    const char *home = "/3ds";
    mkdir("/3ds/.rockbox" __MKDIR_MODE_ARG);

    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    mkdir(config_dir __MKDIR_MODE_ARG);
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org", home);
    mkdir(config_dir __MKDIR_MODE_ARG);
    /* Plugin data directory */
    snprintf(config_dir, sizeof(config_dir), "%s/.config/rockbox.org/rocks.data", home);
    mkdir(config_dir __MKDIR_MODE_ARG);
#endif
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
