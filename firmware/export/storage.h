/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdbool.h>
#include "config.h" /* for HAVE_MULTIVOLUME or not */
#include "mv.h"

#if (CONFIG_STORAGE & STORAGE_SD)
#include "sd.h"
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
#include "mmc.h"
#endif
#if (CONFIG_STORAGE & STORAGE_ATA)
#include "ata.h"
#endif
#if (CONFIG_STORAGE & STORAGE_NAND)
#include "nand.h"
#endif

struct storage_info
{
    unsigned int sector_size;
    unsigned int num_sectors;
    char *vendor;
    char *product;
    char *revision;
};

void storage_spindown(int seconds);

#ifndef SIMULATOR
static inline void storage_enable(bool on)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_enable(on);
#elif (CONFIG_STORAGE & STORAGE_SD)
    sd_enable(on);
#elif (CONFIG_STORAGE & STORAGE_MMC)
    mmc_enable(on);
#else
    (void)on;
#endif
}
static inline void storage_sleep(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_sleep();
#endif
}
static inline void storage_sleepnow(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_sleepnow();
#endif
}
static inline bool storage_disk_is_active(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_disk_is_active();
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_disk_is_active();
#else
    return 0;
#endif
}
static inline int storage_hard_reset(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_hard_reset();
#else
    return 0;
#endif
}
static inline int storage_soft_reset(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_soft_reset();
#else
    return 0;
#endif
}
static inline int storage_init(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_init();
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_init();
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_init();
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_init();
#else
    #error No storage driver!
#endif
}
static inline void storage_close(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_close();
#endif
}
static inline int storage_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_read_sectors(IF_MV2(drive,) start, count, buf);
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_read_sectors(IF_MV2(drive,) start, count, buf);
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_read_sectors(IF_MV2(drive,) start, count, buf);
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_read_sectors(IF_MV2(drive,) start, count, buf);
#else
    #error No storage driver!
#endif
}
static inline int storage_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_write_sectors(IF_MV2(drive,) start, count, buf);
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_write_sectors(IF_MV2(drive,) start, count, buf);
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_write_sectors(IF_MV2(drive,) start, count, buf);
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_write_sectors(IF_MV2(drive,) start, count, buf);
#else
    #error No storage driver!
#endif
}
static inline void storage_spin(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_spin();
#endif
}

#if (CONFIG_LED == LED_REAL)
static inline void storage_set_led_enabled(bool enabled)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    ata_set_led_enabled(enabled);
#elif (CONFIG_STORAGE & STORAGE_SD)
    sd_set_led_enabled(enabled);
#elif (CONFIG_STORAGE & STORAGE_NAND)
    nand_set_led_enabled(enabled);
#elif (CONFIG_STORAGE & STORAGE_MMC)
    mmc_set_led_enabled(enabled);
#else
    #error No storage driver!
#endif
}
#endif /*LED*/

static inline long storage_last_disk_activity(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_last_disk_activity();
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_last_disk_activity();
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_last_disk_activity();
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_last_disk_activity();
#else
    #error No storage driver!
#endif
}

static inline int storage_spinup_time(void)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_spinup_time();
#else
    return 0;
#endif
}

#ifdef STORAGE_GET_INFO
static inline void storage_get_info(IF_MV2(int drive,) struct storage_info *info)
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_get_info(IF_MV2(drive,) info);
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_get_info(IF_MV2(drive,) info);
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_get_info(IF_MV2(drive,) info);
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_get_info(IF_MV2(drive,) info);
#else
    #error No storage driver!
#endif
}
#endif

#ifdef HAVE_HOTSWAP
static inline bool storage_removable(IF_MV_NONVOID(int drive))
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_removable(IF_MV(drive));
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_removable(IF_MV(drive));
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_removable(IF_MV(drive));
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_removable(IF_MV(drive));
#else
    #error No storage driver!
#endif
}

static inline bool storage_present(IF_MV_NONVOID(int drive))
{
#if (CONFIG_STORAGE & STORAGE_ATA)
    return ata_present(IF_MV(drive));
#elif (CONFIG_STORAGE & STORAGE_SD)
    return sd_present(IF_MV(drive));
#elif (CONFIG_STORAGE & STORAGE_NAND)
    return nand_present(IF_MV(drive));
#elif (CONFIG_STORAGE & STORAGE_MMC)
    return mmc_present(IF_MV(drive));
#else
    #error No storage driver!
#endif
}
#endif /* HOTSWAP */
#else /* SIMULATOR */
static inline void storage_enable(bool on)
{
    (void)on;
}
static inline void storage_sleep(void)
{
}
static inline void storage_sleepnow(void)
{
}
static inline bool storage_disk_is_active(void)
{
    return 0;
}
static inline int storage_hard_reset(void)
{
    return 0;
}
static inline int storage_soft_reset(void)
{
    return 0;
}
static inline int storage_init(void)
{
    return 0;
}
static inline void storage_close(void)
{
}
static inline int storage_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    IF_MV((void)drive;)
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}
static inline int storage_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    IF_MV((void)drive;)
    (void)start;
    (void)count;
    (void)buf;
    return 0;
}
static inline void storage_spin(void)
{
}

#if (CONFIG_LED == LED_REAL)
static inline void storage_set_led_enabled(bool enabled)
{
    (void)enabled;
}
#endif /*LED*/

static inline long storage_last_disk_activity(void)
{
    return 0;
}

static inline int storage_spinup_time(void)
{
    return 0;
}

#ifdef STORAGE_GET_INFO
static inline void storage_get_info(IF_MV2(int drive,) struct storage_info *info)
{
    IF_MV((void)drive;)
    (void)info;
}
#endif

#ifdef HAVE_HOTSWAP
static inline bool storage_removable(IF_MV_NONVOID(int drive))
{
    IF_MV((void)drive;)
    return 0;
}

static inline bool storage_present(IF_MV_NONVOID(int drive))
{
    IF_MV((void)drive;)
    return 0;
}
#endif /* HOTSWAP */
#endif /* SIMULATOR */
#endif
