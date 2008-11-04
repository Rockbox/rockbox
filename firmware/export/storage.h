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
#if (CONFIG_STORAGE & STORAGE_RAMDISK)
#include "ramdisk.h"
#endif

struct storage_info
{
    unsigned int sector_size;
    unsigned int num_sectors;
    char *vendor;
    char *product;
    char *revision;
};

#ifndef SIMULATOR
    #ifndef CONFIG_STORAGE_MULTI
    /* storage_spindown, storage_sleep and storage_spin are passed as
     * pointers, which doesn't work with argument-macros.
     */
        #if (CONFIG_STORAGE & STORAGE_ATA)
            #define storage_spindown ata_spindown
            #define storage_sleep ata_sleep
            #define storage_spin ata_spin
    
            #define storage_enable(on) ata_enable(on)
            #define storage_sleepnow() ata_sleepnow()
            #define storage_disk_is_active() ata_disk_is_active()
            #define storage_soft_reset() ata_soft_reset()
            #define storage_init() ata_init()
            #define storage_close() ata_close()
            #define storage_read_sectors(drive, start, count, buf) ata_read_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_write_sectors(drive, start, count, buf) ata_write_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_last_disk_activity() ata_last_disk_activity()
            #define storage_spinup_time() ata_spinup_time()
            #define storage_get_identify() ata_get_identify()
    
            #if (CONFIG_LED == LED_REAL)
                #define storage_set_led_enabled(enabled) ata_set_led_enabled(enabled)
            #endif
            #ifdef STORAGE_GET_INFO
                #define storage_get_info(drive, info) ata_get_info(IF_MV2(drive,) info)
            #endif
            #ifdef HAVE_HOTSWAP
                #define storage_removable(drive) ata_removable(IF_MV(drive))
                #define storage_present(drive) ata_present(IF_MV(drive))
            #endif
        #elif (CONFIG_STORAGE & STORAGE_SD)
            #define storage_spindown sd_spindown
            #define storage_sleep sd_sleep
            #define storage_spin sd_spin
    
            #define storage_enable(on) sd_enable(on)
            #define storage_sleepnow() sd_sleepnow()
            #define storage_disk_is_active() 0
            #define storage_soft_reset() (void)0
            #define storage_init() sd_init()
            #define storage_close() sd_close()
            #define storage_read_sectors(drive, start, count, buf) sd_read_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_write_sectors(drive, start, count, buf) sd_write_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_last_disk_activity() sd_last_disk_activity()
            #define storage_spinup_time() 0
            #define storage_get_identify() sd_get_identify()
    
            #if (CONFIG_LED == LED_REAL)
                #define storage_set_led_enabled(enabled) sd_set_led_enabled(enabled)
            #endif
            #ifdef STORAGE_GET_INFO
                #define storage_get_info(drive, info) sd_get_info(IF_MV2(drive,) info)
            #endif
            #ifdef HAVE_HOTSWAP
                #define storage_removable(drive) sd_removable(IF_MV(drive))
                #define storage_present(drive) sd_present(IF_MV(drive))
            #endif
         #elif (CONFIG_STORAGE & STORAGE_MMC)
            #define storage_spindown mmc_spindown
            #define storage_sleep mmc_sleep
            #define storage_spin mmc_spin
    
            #define storage_enable(on) mmc_enable(on)
            #define storage_sleepnow() mmc_sleepnow()
            #define storage_disk_is_active() mmc_disk_is_active()
            #define storage_soft_reset() (void)0
            #define storage_init() mmc_init()
            #define storage_close() mmc_close()
            #define storage_read_sectors(drive, start, count, buf) mmc_read_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_write_sectors(drive, start, count, buf) mmc_write_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_last_disk_activity() mmc_last_disk_activity()
            #define storage_spinup_time() 0
            #define storage_get_identify() mmc_get_identify()
           
            #if (CONFIG_LED == LED_REAL)
                #define storage_set_led_enabled(enabled) mmc_set_led_enabled(enabled)
            #endif
            #ifdef STORAGE_GET_INFO
                #define storage_get_info(drive, info) mmc_get_info(IF_MV2(drive,) info)
            #endif
            #ifdef HAVE_HOTSWAP
                #define storage_removable(drive) mmc_removable(IF_MV(drive))
                #define storage_present(drive) mmc_present(IF_MV(drive))
            #endif
        #elif (CONFIG_STORAGE & STORAGE_NAND)
            #define storage_spindown nand_spindown
            #define storage_sleep nand_sleep
            #define storage_spin nand_spin
    
            #define storage_enable(on) (void)0
            #define storage_sleepnow() nand_sleepnow()
            #define storage_disk_is_active() 0
            #define storage_soft_reset() (void)0
            #define storage_init() nand_init()
            #define storage_close() nand_close()
            #define storage_read_sectors(drive, start, count, buf) nand_read_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_write_sectors(drive, start, count, buf) nand_write_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_last_disk_activity() nand_last_disk_activity()
            #define storage_spinup_time() 0
            #define storage_get_identify() nand_get_identify()
           
            #if (CONFIG_LED == LED_REAL)
                #define storage_set_led_enabled(enabled) nand_set_led_enabled(enabled)
            #endif
            #ifdef STORAGE_GET_INFO
                #define storage_get_info(drive, info) nand_get_info(IF_MV2(drive,) info)
            #endif
            #ifdef HAVE_HOTSWAP
                #define storage_removable(drive) nand_removable(IF_MV(drive))
                #define storage_present(drive) nand_present(IF_MV(drive))
            #endif
        #elif (CONFIG_STORAGE & STORAGE_RAMDISK)
            #define storage_spindown ramdisk_spindown
            #define storage_sleep ramdisk_sleep
            #define storage_spin ramdisk_spin
    
            #define storage_enable(on) (void)0
            #define storage_sleepnow() ramdisk_sleepnow()
            #define storage_disk_is_active() 0
            #define storage_soft_reset() (void)0
            #define storage_init() ramdisk_init()
            #define storage_close() ramdisk_close()
            #define storage_read_sectors(drive, start, count, buf) ramdisk_read_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_write_sectors(drive, start, count, buf) ramdisk_write_sectors(IF_MV2(drive,) start, count, buf)
            #define storage_last_disk_activity() ramdisk_last_disk_activity()
            #define storage_spinup_time() 0
            #define storage_get_identify() ramdisk_get_identify()
           
            #if (CONFIG_LED == LED_REAL)
                #define storage_set_led_enabled(enabled) ramdisk_set_led_enabled(enabled)
            #endif
            #ifdef STORAGE_GET_INFO
                #define storage_get_info(drive, info) ramdisk_get_info(IF_MV2(drive,) info)
            #endif
            #ifdef HAVE_HOTSWAP
                #define storage_removable(drive) ramdisk_removable(IF_MV(drive))
                #define storage_present(drive) ramdisk_present(IF_MV(drive))
            #endif
        #else
            //#error No storage driver!
        #endif
    #else /* NOT CONFIG_STORAGE_MULTI */
    
    /* TODO : implement multi-driver here */
    #error Multi-driver storage not implemented yet
    
    #endif /* NOT CONFIG_STORAGE_MULTI */
#else /*NOT SIMULATOR */
void storage_enable(bool on);
void storage_sleep(void);
void storage_sleepnow(void);
bool storage_disk_is_active(void);
int storage_soft_reset(void);
int storage_init(void);
void storage_close(void);
int storage_read_sectors(int drive, unsigned long start, int count, void* buf);
int storage_write_sectors(int drive, unsigned long start, int count, const void* buf);
void storage_spin(void);
void storage_spindown(int seconds);
#if (CONFIG_LED == LED_REAL)
void storage_set_led_enabled(bool enabled);
#endif
long storage_last_disk_activity(void);
int storage_spinup_time(void);
#ifdef STORAGE_GET_INFO
void storage_get_info(int drive, struct storage_info *info);
#endif
#ifdef HOTSWAP
bool storage_removable(int drive);
bool storage_present(int drive);
#endif
#endif/*NOT SIMULATOR */
#endif
