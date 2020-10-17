/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#ifndef __MV_H__
#define __MV_H__

#include <stdbool.h>
#include "config.h"

/* FixMe: These macros are a bit nasty and perhaps misplaced here.
   We'll get rid of them once decided on how to proceed with multivolume. */

/* Drives are things like a disk, a card, a flash chip. They can have volumes
   on them */
#ifdef HAVE_MULTIDRIVE
#define IF_MD(x...) x          /* valist contents or empty */
#define IF_MD_NONVOID(x...) x  /* valist contents or 'void' */
#define IF_MD_DRV(d)  d        /* drive argument or '0' */
#else /* empty definitions if no multi-drive */
#define IF_MD(x...)
#define IF_MD_NONVOID(x...) void
#define IF_MD_DRV(d)  0
#endif /* HAVE_MULTIDRIVE */

/* Volumes mean things that have filesystems on them, like partitions */
#ifdef HAVE_MULTIVOLUME
#define IF_MV(x...) x          /* valist contents or empty */
#define IF_MV_NONVOID(x...) x  /* valist contents or 'void' */
#define IF_MV_VOL(v) v         /* volume argument or '0' */

/* Format: "/<DEC###>/foo/bar"
 * The "DEC" is pure decoration and treated as a comment. Only an unbroken
 * trailing string of digits within the brackets is parsed as the volume
 * number.
 *
 * IMPORTANT!: Adjust VOL_DEC_MAX_LEN if needed to the longest of these
 */
#define DEFAULT_VOL_DEC "Volume"

#if (CONFIG_STORAGE & STORAGE_ATA)
#define ATA_VOL_DEC     "HDD"
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
#define MMC_VOL_DEC     "MMC"
#endif
#if (CONFIG_STORAGE & STORAGE_SD)
#define SD_VOL_DEC      "microSD"
#endif
#if (CONFIG_STORAGE & STORAGE_NAND)
#define NAND_VOL_DEC    "NAND"
#endif
#if (CONFIG_STORAGE & STORAGE_RAMDISK)
#define RAMDISK_VOL_DEC "RAMDisk"
#endif
#if (CONFIG_STORAGE & STORAGE_USB)
#define USB_VOL_DEC "USB"
#endif
#if (CONFIG_STORAGE & STORAGE_HOSTFS)
#ifndef HOSTFS_VOL_DEC /* overridable */
#define HOSTFS_VOL_DEC  DEFAULT_VOL_DEC
#endif
#endif

/* Characters that delimit a volume specifier at any root point in the path.
   The tokens must be outside of legal filename characters */
#define VOL_START_TOK    '<'
#define VOL_END_TOK      '>'
#define VOL_DEC_MAX_LEN  7 /* biggest of all xxx_VOL_DEC defines */
#define VOL_MAX_LEN      (1 + VOL_DEC_MAX_LEN + 2 + 1)
#define VOL_NUM_MAX      100

#else /* empty definitions if no multi-volume */
#define IF_MV(x...)
#define IF_MV_NONVOID(x...) void
#define IF_MV_VOL(v) 0
#endif /* HAVE_MULTIVOLUME */

#define CHECK_VOL(volume) \
    ((unsigned int)IF_MV_VOL(volume) < NUM_VOLUMES)

#define CHECK_DRV(drive) \
    ((unsigned int)IF_MD_DRV(drive) < NUM_DRIVES)

/* Volume-centric functions (in disk.c) */
void volume_recalc_free(IF_MV_NONVOID(int volume));
unsigned int volume_get_cluster_size(IF_MV_NONVOID(int volume));
void volume_size(IF_MV(int volume,) unsigned long *size, unsigned long *free);
#ifdef HAVE_DIRCACHE
bool volume_ismounted(IF_MV_NONVOID(int volume));
#endif
#ifdef HAVE_HOTSWAP
bool volume_removable(int volume);
bool volume_present(int volume);
#endif /* HAVE_HOTSWAP */

#ifdef HAVE_MULTIDRIVE
int volume_drive(int volume);
#else /* !HAVE_MULTIDRIVE */
static inline int volume_drive(int volume)
{
    return 0;
    (void)volume;
}
#endif /* HAVE_MULTIDRIVE */

#endif /* __MV_H__ */
