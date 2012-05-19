/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "partitions-imx233.h"

static bool enable_window = true;

void imx233_partitions_enable_window(bool enable)
{
    enable_window = enable;
}

bool imx233_partitions_is_window_enabled(void)
{
    return enable_window;
}

int imx233_partitions_compute_window(uint8_t mbr[512], unsigned *start, unsigned *end)
{
    /**
     * Freescale uses a strange layout: is has a first MBR at sector 0 with four entries:
     * 1) Actual user partition
     * 2) Sigmatel boot partition
     * 3)4) Other (certificate related ?) partitions
     * The partition 1) has type 1 but it's actually a type 5 (logical partition) with
     * a second partition table with usually one entry which is the FAT32 one.
     * The first table uses 512-byte sector size and the second one usually uses
     * 2048-byte logical sector size.
     *
     * We restrict the window to the user partition
     *
     * WARNING HACK FIXME BUG
     * Reverse engineering and experiments suggests that the OF ignores the lowest 2 bits
     * of the LBAs in the partition table. There is at least one example
     * (the Creative Zen X-Fi3) where this is important because the LBA of the user partition
     * is not a multiple of 4. The behaviour of the size field is less clear but
     * it seems that it is similarly truncated. */
    if(mbr[510] != 0x55 || mbr[511] != 0xAA)
        return -101; /* invalid MBR */
    /* sanity check that the first partition is greater than 2Gib */
    uint8_t *ent = &mbr[446];
    *start = ent[8] | ent[9] << 8 | ent[10] << 16 | ent[11] << 24;
    /* ignore two lowest bits(see comment above) */
    *start &= ~3;
    *end = (ent[12] | ent[13] << 8 | ent[14] << 16 | ent[15] << 24);
    *end &= ~3;
    /* ignore two lowest bits(order is important, first truncate then add start) */
    *end += *start;
    
    if(ent[4] == 0x53)
        return -102; /* sigmatel partition */
    if((*end - *start) < 4 * 1024 * 1024)
        return -103; /* partition too small */
    return 0;
}
