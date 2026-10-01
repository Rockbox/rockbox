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

/* Rockbox storage on the rk27xx NAND, through the FTL scheme the target's
 * config names in CONFIG_RK27XX_FTL: ftl-scheme-a.c or ftl-scheme-b.c.
 *
 * Writing is opt-in: a build without FTL_ALLOW_WRITE mounts read-only and
 * never writes the flash, not even the repairs a mount can make. */

#include "config.h"
#include "system.h"
#include "ftl-target.h"
#include "nand-target.h"
#include "flash-rk27xx.h"

#if CONFIG_RK27XX_FTL == RK27XX_FTL_SCHEME_A
#include "ftl-scheme-a.h"
#elif CONFIG_RK27XX_FTL == RK27XX_FTL_SCHEME_B
#include "ftl-scheme-b.h"
#else
#error "NAND storage needs CONFIG_RK27XX_FTL in the target config"
#endif

/* The boot ROM looks for ID blocks at every 512th raw sector of the boot
 * area, up to 50 positions, by metadata type 0x69. */
#define IDB_STRIDE          512
#define IDB_POSITIONS       50
#define IDB_META_TYPE       2
#define IDB_TYPE            0x69
#define IDB_MAX_BOOT_BLOCKS 64

#define SECTORS_PER_MB      2048

#if CONFIG_RK27XX_FTL == RK27XX_FTL_SCHEME_B
/* Scheme B's original firmware keeps its open exchange blocks to 8, and its
 * mount recovers no more (ftl-scheme-b.h) */
#define SCHEME_B_EXCH_BLOCKS 8
#endif

/* What ID block 1 records */
struct idb_info
{
    uint32_t boot_blocks;       /* raw blocks of the boot area */
    uint32_t sys_sectors;       /* the SYS volume (code disk) */
    uint32_t data_sectors;      /* Scheme B: the system data area after it */
};

static bool ftl_mounted = false;

#if CONFIG_RK27XX_FTL == RK27XX_FTL_SCHEME_B
/* Scheme B's logical space holds the volumes back to back: SYS, the system
 * data area, then USER to the end */
static uint32_t vol_base[FTL_NUM_DRIVES];
static uint32_t vol_size[FTL_NUM_DRIVES];
#endif

/* ID block 1, the sector after ID block 0:
 *
 *     +0  uint16_t LE   raw blocks of the boot area
 *     +2  uint16_t LE   SYS volume size, MB
 *     +4  uint16_t LE   system data area size, MB (Scheme B)
 *
 * ID block 0 is scrambled; ID block 1 is plain. A sector marked 0x69 whose
 * ID block 1 gives a sane block count and a SYS volume smaller than the
 * chip is taken; descrambling ID block 0 to check its signature would buy
 * little over that. Returns false if none is found. */
static bool idb_read(struct idb_info *idb)
{
    const struct flash_geometry *geo = flash_get_geometry();
    uint8_t data[FLASH_SECTOR_SIZE], meta[FLASH_META_SIZE];
    bool found = false;
    uint32_t pos;

    for (pos = 0; pos < IDB_POSITIONS && !found; pos++)
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
            mb * SECTORS_PER_MB < geo->total_sectors)
        {
            idb->boot_blocks  = blocks;
            idb->sys_sectors  = mb * SECTORS_PER_MB;
            idb->data_sectors = (data[4] | (data[5] << 8)) * SECTORS_PER_MB;
            found = true;
        }
    }
    return found;
}

#if CONFIG_RK27XX_FTL == RK27XX_FTL_SCHEME_A
static uint32_t mount_scheme(const struct idb_info *idb)
{
    struct ftl_a_config config;
    uint32_t ret = 0;

    config.sys_sectors = idb->sys_sectors;
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

    if (ftl_a_mount(&config) != FTL_A_OK)
    {
        ret = 3;
    }
    else if (ftl_a_capacity(FTL_A_VOL_USER) == 0)
    {
        ret = 4;
    }
    return ret;
}
#else
static uint32_t mount_scheme(const struct idb_info *idb)
{
    const struct flash_geometry *geo = flash_get_geometry();
    struct ftl_b_config config;
    uint32_t user_base = idb->sys_sectors + idb->data_sectors;
    uint32_t ret = 0;

    config.first_block = (uint16_t)(idb->boot_blocks / geo->planes);
    config.exch_blocks = SCHEME_B_EXCH_BLOCKS;
#ifdef FTL_ALLOW_WRITE
    config.read_only = false;
#else
    config.read_only = true;
#endif

    if (ftl_b_mount(&config) != FTL_B_OK)
    {
        ret = 3;
    }
    else if (ftl_b_capacity() <= user_base)
    {
        ret = 4;
    }
    else
    {
#ifdef HAVE_RK27XX_NAND_SYS
        vol_base[FTL_DRIVE_SYS] = 0;
        vol_size[FTL_DRIVE_SYS] = idb->sys_sectors;
#endif
        vol_base[FTL_DRIVE_USER] = user_base;
        vol_size[FTL_DRIVE_USER] = ftl_b_capacity() - user_base;
    }
    return ret;
}
#endif

uint32_t ftl_init(void)
{
    struct idb_info idb;
    uint32_t ret = 0;

    flash_init();

    if (flash_layer_init() != 0)
    {
        ret = 1;
    }
    else if (!idb_read(&idb))
    {
        ret = 2;        /* without it USER cannot be told from SYS */
    }
    else
    {
        ret = mount_scheme(&idb);
        ftl_mounted = ret == 0;
    }
    return ret;
}

static bool drive_valid(int drive)
{
    return ftl_mounted && drive >= 0 && drive < FTL_NUM_DRIVES;
}

#if CONFIG_RK27XX_FTL == RK27XX_FTL_SCHEME_A
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

static uint32_t drive_sectors(int drive)
{
    return ftl_a_capacity(ftl_volume(drive));
}

static int drive_read(int drive, uint32_t sector, uint32_t count, void *buffer)
{
    return ftl_a_read(ftl_volume(drive), sector, buffer, count);
}

#ifdef FTL_ALLOW_WRITE
static int drive_write(int drive, uint32_t sector, uint32_t count,
                       const void *buffer)
{
    return ftl_a_write(ftl_volume(drive), sector, buffer, count);
}
#endif

static void drive_sync(void)
{
    ftl_a_sync();
}
#else
static uint32_t drive_sectors(int drive)
{
    return vol_size[drive];
}

static bool in_volume(int drive, uint32_t sector, uint32_t count)
{
    return sector < vol_size[drive] && count <= vol_size[drive] - sector;
}

static int drive_read(int drive, uint32_t sector, uint32_t count, void *buffer)
{
    int ret = 1;

    if (in_volume(drive, sector, count))
    {
        ret = ftl_b_read(vol_base[drive] + sector, buffer, count);
    }
    return ret;
}

#ifdef FTL_ALLOW_WRITE
static int drive_write(int drive, uint32_t sector, uint32_t count,
                       const void *buffer)
{
    int ret = 1;

    if (in_volume(drive, sector, count))
    {
        ret = ftl_b_write(vol_base[drive] + sector, buffer, count);
    }
    return ret;
}
#endif

static void drive_sync(void)
{
    ftl_b_sync();
}
#endif

uint32_t ftl_get_sectors(int drive)
{
    uint32_t sectors = 0;

    if (drive_valid(drive))
    {
        sectors = drive_sectors(drive);
    }
    return sectors;
}

uint32_t ftl_read(int drive, uint32_t sector, uint32_t count, void *buffer)
{
    uint32_t ret = 1;

    if (drive_valid(drive))
    {
        ret = drive_read(drive, sector, count, buffer) ? 2 : 0;
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
        ret = drive_write(drive, sector, count, buffer) ? 2 : 0;
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
        drive_sync();
    }
    return 0;
}
