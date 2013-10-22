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
#include "string.h"

static bool enable_window = true;

void imx233_partitions_enable_window(bool enable)
{
    enable_window = enable;
}

bool imx233_partitions_is_window_enabled(void)
{
    return enable_window;
}

#if (IMX233_PARTITIONS & IMX233_CREATIVE)
#define MBLK_MAGIC  0x4d424c4b /* MBLK */
#define MBLK_COUNT  31
/* MBLK is not located in the first sector !
 * Creative code uses the hard-coded *absolute* address 0x3ffe00,
 * bypassing all partition related information !!
 * NOTE: for some reason, the ZEN uses a different value ?! */
#ifdef CREATIVE_ZEN
#define MBLK_ADDR   0x400000
#else
#define MBLK_ADDR   0x3ffe00
#endif

struct mblk_header_t
{
    uint32_t magic;
    uint32_t block_size;
    uint64_t total_size;
} __attribute__((packed));

struct mblk_partition_t
{
    uint32_t size;
    uint32_t start;
    char name[8];
} __attribute__((packed));

static const char *creative_part_name(enum imx233_part_t part)
{
    switch(part)
    {
        case IMX233_PART_USER: return "cfs";
        case IMX233_PART_CFS: return "cfs";
        case IMX233_PART_MINIFS: return "minifs";
        default: return "";
    }
}

static int compute_window_creative(intptr_t user, part_read_fn_t read_fn,
    enum imx233_part_t part, unsigned *start, unsigned *end)
{
    uint8_t mblk[512];
    int ret = read_fn(user, MBLK_ADDR / 512, 1, mblk);
    if(ret < 0)
        return ret;
    struct mblk_header_t *hdr = (void *)mblk;
    if(hdr->magic != MBLK_MAGIC)
        return -70; /* bad magic */
    struct mblk_partition_t *ent = (void *)(hdr + 1);
    const char *name = creative_part_name(part);
    for(int i = 0; i < MBLK_COUNT; i++)
    {
        if(ent[i].name[0] == 0)
            continue;
        if(strcmp(ent[i].name, name) == 0)
        {
            *start = ent[i].start * hdr->block_size / 512;
            *end = *start + ent[i].size * hdr->block_size / 512;
            return 0;
        }
    }
    return -80; /* not found */
}
#endif /* #(IMX233_PARTITIONS & IMX233_CREATIVE) */

#if (IMX233_PARTITIONS & IMX233_FREESCALE)
static int compute_window_freescale(intptr_t user, part_read_fn_t read_fn,
    enum imx233_part_t part, unsigned *start, unsigned *end)
{
    uint8_t mbr[512];
    int ret = read_fn(user, 0, 1, mbr);
    if(ret < 0)
        return ret;
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
    if(part == IMX233_PART_USER)
    {
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
    else if(part == IMX233_PART_BOOT)
    {
        /* sanity check that the second partition is correct */
        uint8_t *ent = &mbr[462];
        if(ent[4] != 0x53)
            return -104; /* wrong type */
        *start = ent[8] | ent[9] << 8 | ent[10] << 16 | ent[11] << 24;
        *end = (ent[12] | ent[13] << 8 | ent[14] << 16 | ent[15] << 24);
        *end += *start;

        return 0;
    }
    else
        return -50;
}
#endif /* (IMX233_PARTITIONS & IMX233_FREESCALE) */

int imx233_partitions_compute_window(intptr_t user, part_read_fn_t read_fn,
    enum imx233_part_t part, unsigned *start, unsigned *end)
{
    int ret = -1;
#if (IMX233_PARTITIONS & IMX233_CREATIVE)
    ret = compute_window_creative(user, read_fn, part, start, end);
    if(ret >= 0)
        return ret;
#endif
#if (IMX233_PARTITIONS & IMX233_FREESCALE)
    ret = compute_window_freescale(user, read_fn, part, start, end);
    if(ret >= 0)
        return ret;
#endif
    return ret;
}
