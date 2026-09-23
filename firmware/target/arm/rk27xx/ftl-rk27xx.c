/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 by Bertrik Sikken
 * Copyright (C) 2026 by Marcin Bukat
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

/* Rockbox storage on the rk27xx Scheme A FTL (ftl-scheme-a.c).
 *
 * Writing is opt-in: a build without FTL_ALLOW_WRITE mounts read-only and
 * never writes the flash, not even the repairs a mount can make. */

#include "config.h"
#include "system.h"
#include "ftl-target.h"
#include "nand-target.h"
#include "flash-rk27xx.h"
#include "ftl-scheme-a.h"

/* The boot ROM looks for ID blocks at every 512th raw sector of the boot
 * area, up to 50 positions, by metadata type 0x69. */
#define IDB_STRIDE          512
#define IDB_POSITIONS       50
#define IDB_META_TYPE       2
#define IDB_TYPE            0x69
#define IDB_MAX_BOOT_BLOCKS 64

static bool ftl_mounted = false;

/* The size of the SYS volume, from ID block 1.
 *
 * ID block 0 is scrambled; ID block 1, the sector after it, is plain:
 *
 *     +0  uint16_t LE   blocks of bootloader
 *     +2  uint16_t LE   SYS volume size, MB
 *
 * A sector marked 0x69 whose ID block 1 gives a sane block count and a SYS
 * volume smaller than the chip is taken; descrambling ID block 0 to check its
 * signature would buy little over that. Returns 0 if none is found. */
static uint32_t idb_sys_sectors(void)
{
    const struct flash_geometry *geo = flash_get_geometry();
    uint8_t data[FLASH_SECTOR_SIZE], meta[FLASH_META_SIZE];
    uint32_t sectors = 0;
    uint32_t pos;

    for (pos = 0; pos < IDB_POSITIONS && sectors == 0; pos++)
    {
        uint32_t raw = pos * IDB_STRIDE;
        uint32_t blocks, mb;

        if (raw + 1 >= geo->total_sectors)
        {
            break;
        }

        if (flash_read_raw(raw, data, meta) != 0
            || meta[IDB_META_TYPE] != IDB_TYPE)
        {
            continue;
        }
        if (flash_read_raw(raw + 1, data, meta) != 0)
        {
            continue;
        }

        blocks = data[0] | (data[1] << 8);
        mb     = data[2] | (data[3] << 8);

        if (blocks > 0 && blocks <= IDB_MAX_BOOT_BLOCKS && mb > 0 &&
            mb * 2048 < geo->total_sectors)
        {
            sectors = mb * 2048;
        }
    }
    return sectors;
}

uint32_t ftl_init(void)
{
    struct ftl_a_config config;
    uint32_t ret = 0;

    flash_init();

    if (flash_layer_init() != 0)
    {
        ret = 1;
    }
    else
    {
        config.sys_sectors = idb_sys_sectors();
#ifdef FTL_ALLOW_WRITE
        config.read_only = false;
        /* the rk2705 NAND bootloader's generation formats with flag 1; its
         * write logic is the same as the standard one's */
        config.alt_format_flag = 1;
        config.alt_format_writable = true;
#else
        config.read_only = true;
        config.alt_format_flag = 1;
        config.alt_format_writable = false;
#endif

        if (config.sys_sectors == 0)
        {
            ret = 2;        /* without it USER cannot be told from SYS */
        }
        else if (ftl_a_mount(&config) != FTL_A_OK)
        {
            ret = 3;
        }
        else if (ftl_a_capacity(FTL_A_VOL_USER) == 0)
        {
            ret = 4;
        }
        else
        {
            ftl_mounted = true;
        }
    }
    return ret;
}

/* The FTL volume behind a drive. SYS is reachable only when exposed. */
static int ftl_volume(int drive)
{
    int volume = FTL_A_VOL_USER;

#ifdef HAVE_RK27XX_NAND_SYS
    if (drive == FTL_DRIVE_SYS)
    {
        volume = FTL_A_VOL_SYS;
    }
#else
    (void)drive;
#endif
    return volume;
}

static bool drive_valid(int drive)
{
    return ftl_mounted && drive >= 0 && drive < FTL_NUM_DRIVES;
}

uint32_t ftl_get_sectors(int drive)
{
    uint32_t sectors = 0;

    if (drive_valid(drive))
    {
        sectors = ftl_a_capacity(ftl_volume(drive));
    }
    return sectors;
}

uint32_t ftl_read(int drive, uint32_t sector, uint32_t count, void *buffer)
{
    uint32_t ret = 1;

    if (drive_valid(drive))
    {
        ret = ftl_a_read(ftl_volume(drive), sector, buffer, count) ? 2 : 0;
    }
    return ret;
}

uint32_t ftl_write(int drive, uint32_t sector, uint32_t count,
                   const void *buffer)
{
    uint32_t ret = 1;

#ifdef FTL_ALLOW_WRITE
    if (drive_valid(drive))
    {
        ret = ftl_a_write(ftl_volume(drive), sector, buffer, count) ? 2 : 0;
    }
#else
    /* refuse rather than pretend: a silent success would let the filesystem
     * believe data was committed */
    (void)drive; (void)sector; (void)count; (void)buffer;
#endif
    return ret;
}

uint32_t ftl_sync(void)
{
    if (ftl_mounted)
    {
        ftl_a_sync();
    }
    return 0;
}
