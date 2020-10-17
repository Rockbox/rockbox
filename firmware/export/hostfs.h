/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright © 2013 by Thomas Martitz
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

/* hosted storage driver
 *
 * Small functions to support storage_* api on hosted targets
 * where we don't use raw access to storage devices but run on OS-mounted
 * file systems */

#ifndef HOSTFS_H
#define HOSTFS_H

#include <stdbool.h>
#ifdef __unix__
#include <unistd.h>
#endif
#include "config.h"

extern int  hostfs_init(void);
extern int  hostfs_flush(void);

#ifdef HAVE_HOTSWAP
extern bool hostfs_removable(int drive);
extern bool hostfs_present(int drive);
#endif

/* This has to be repeated here for now for sim's sake since HAVE_HOSTFS
   eats all the other stuff in storage.h. The sim probably shouldn't use
   this. */
#ifdef CONFIG_STORAGE_MULTI
extern int hostfs_driver_type(int drive);
#else
# ifdef APPLICATION
#  define hostfs_driver_type(drive) (STORAGE_HOSTFS_NUM)
# else /* !APPLICATION */
#  if (CONFIG_STORAGE & STORAGE_ATA)
#   define hostfs_driver_type(drive) (STORAGE_ATA_NUM)
#  elif (CONFIG_STORAGE & STORAGE_SD)
#   define hostfs_driver_type(drive) (STORAGE_SD_NUM)
#  elif (CONFIG_STORAGE & STORAGE_USB)
#   define hostfs_driver_type(drive) (STORAGE_USB_NUM)
#  elif (CONFIG_STORAGE & STORAGE_MMC)
#   define hostfs_driver_type(drive) (STORAGE_MMC_NUM)
#  elif (CONFIG_STORAGE & STORAGE_NAND)
#   define hostfs_driver_type(drive) (STORAGE_NAND_NUM)
#  elif (CONFIG_STORAGE & STORAGE_RAMDISK)
#   define hostfs_driver_type(drive) (STORAGE_RAMDISK_NUM)
/* we may have hostfs without application when building sims for applications! */
#  elif (CONFIG_STORAGE & STORAGE_HOSTFS)
#   define hostfs_driver_type(drive) (STORAGE_HOSTFS_NUM)
#  else
#   error Unknown storage driver
#  endif /* CONFIG_STORAGE */
# endif /* APPLICATION */
#endif /* CONFIG_STORAGE_MULTI */

#endif /* HOSTFS_H */
