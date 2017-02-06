/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "kernel.h"
#include "storage.h"
#include "debug.h"
#include "disk_cache.h"
#include "fileobj_mgr.h"
#include "dir.h"
#include "dircache_redirect.h"
#include "disk.h"

#ifndef CONFIG_DEFAULT_PARTNUM
#define CONFIG_DEFAULT_PARTNUM 0
#endif

#define disk_reader_lock()      file_internal_lock_READER()
#define disk_reader_unlock()    file_internal_unlock_READER()
#define disk_writer_lock()      file_internal_lock_WRITER()
#define disk_writer_unlock()    file_internal_unlock_WRITER()

/* Partition table entry layout:
   -----------------------
   0: 0x80 - active
   1: starting head
   2: starting sector
   3: starting cylinder
   4: partition type
   5: end head
   6: end sector
   7: end cylinder
   8-11: starting sector (LBA)
   12-15: nr of sectors in partition
*/

#define BYTES2INT32(array, pos) \
    (((uint32_t)array[pos+0] <<  0) | \
     ((uint32_t)array[pos+1] <<  8) | \
     ((uint32_t)array[pos+2] << 16) | \
     ((uint32_t)array[pos+3] << 24))

#define BYTES2INT16(array, pos) \
    (((uint32_t)array[pos+0] << 0) | \
     ((uint32_t)array[pos+1] << 8))

static const unsigned char fat_partition_types[] =
{
    0x0b, 0x1b, /* FAT32 + hidden variant */
    0x0c, 0x1c, /* FAT32 (LBA) + hidden variant */
#ifdef HAVE_FAT16SUPPORT
    0x04, 0x14, /* FAT16 <= 32MB + hidden variant */
    0x06, 0x16, /* FAT16  > 32MB + hidden variant */
    0x0e, 0x1e, /* FAT16 (LBA) + hidden variant */
#endif
};

/* space for 4 partitions on 2 drives */
static struct partinfo part[NUM_DRIVES*4];
/* mounted to which drive (-1 if none) */
static int vol_drive[NUM_VOLUMES];

static int get_free_volume(void)
{
    for (int i = 0; i < NUM_VOLUMES; i++)
    {
        if (vol_drive[i] == -1) /* unassigned? */
            return i;
    }

    return -1; /* none found */
}

#ifdef MAX_LOG_SECTOR_SIZE
static int disk_sector_multiplier[NUM_DRIVES] =
    { [0 ... NUM_DRIVES-1] = 1 };

int disk_get_sector_multiplier(IF_MD_NONVOID(int drive))
{
    if (!CHECK_DRV(drive))
        return 0;

    disk_reader_lock();
    int multiplier = disk_sector_multiplier[IF_MD_DRV(drive)];
    disk_reader_unlock();
    return multiplier;
}
#endif /* MAX_LOG_SECTOR_SIZE */

bool disk_init(IF_MD_NONVOID(int drive))
{
    if (!CHECK_DRV(drive))
        return false; /* out of space in table */

    unsigned char *sector = dc_get_buffer();
    if (!sector)
        return false;

    memset(sector, 0, SECTOR_SIZE);
    storage_read_sectors(IF_MD(drive,) 0, 1, sector);

    bool init = false;

    /* check that the boot sector is initialized */
    if (BYTES2INT16(sector, 510) == 0xaa55)
    {
        /* For each drive, start at a different position, in order not to
           destroy the first entry of drive 0. That one is needed to calculate
           config sector position. */
        struct partinfo *pinfo = &part[IF_MD_DRV(drive)*4];

        disk_writer_lock();

        /* parse partitions */
        for (int i = 0; i < 4; i++)
        {
            unsigned char* ptr = sector + 0x1be + 16*i;
            pinfo[i].type  = ptr[4];
            pinfo[i].start = BYTES2INT32(ptr, 8);
            pinfo[i].size  = BYTES2INT32(ptr, 12);

            DEBUGF("Part%d: Type %02x, start: %08lx size: %08lx\n",
                   i,pinfo[i].type,pinfo[i].start,pinfo[i].size);

            /* extended? */
            if ( pinfo[i].type == 5 )
            {
                /* not handled yet */
            }
        }

        disk_writer_unlock();

        init = true;
    }
    else
    {
        DEBUGF("Bad boot sector signature\n");
    }

    dc_release_buffer(sector);
    return init;
}

bool disk_partinfo(int partition, struct partinfo *info)
{
    if (partition < 0 || partition >= (int)ARRAYLEN(part) || !info)
        return false;

    disk_reader_lock();
    *info = part[partition];
    disk_reader_unlock();
    return true;
}

int disk_mount(int drive)
{
    int mounted = 0; /* reset partition-on-drive flag */

    disk_writer_lock();

    int volume = get_free_volume();

    if (!disk_init(IF_MD(drive)))
    {
        disk_writer_unlock();
        return 0;
    }

    struct partinfo *pinfo = &part[IF_MD_DRV(drive)*4];
#ifdef MAX_LOG_SECTOR_SIZE
    disk_sector_multiplier[IF_MD_DRV(drive)] = 1;
#endif

    for (int i = CONFIG_DEFAULT_PARTNUM;
         volume != -1 && i < 4 && mounted < NUM_VOLUMES_PER_DRIVE;
         i++)
    {
        if (memchr(fat_partition_types, pinfo[i].type,
                   sizeof(fat_partition_types)) == NULL)
            continue;  /* not an accepted partition type */

    #ifdef MAX_LOG_SECTOR_SIZE
        for (int j = 1; j <= (MAX_LOG_SECTOR_SIZE/SECTOR_SIZE); j <<= 1)
        {
            if (!fat_mount(IF_MV(volume,) IF_MD(drive,) pinfo[i].start * j))
            {
                pinfo[i].start *= j;
                pinfo[i].size *= j;
                mounted++;
                vol_drive[volume] = drive; /* remember the drive for this volume */
                disk_sector_multiplier[drive] = j;
                volume_onmount_internal(IF_MV(volume));
                volume = get_free_volume(); /* prepare next entry */
                break;
            }
        }
    #else /* ndef MAX_LOG_SECTOR_SIZE */
        if (!fat_mount(IF_MV(volume,) IF_MD(drive,) pinfo[i].start))
        {
            mounted++;
            vol_drive[volume] = drive; /* remember the drive for this volume */
            volume_onmount_internal(IF_MV(volume));
            volume = get_free_volume(); /* prepare next entry */
        }
    #endif /* MAX_LOG_SECTOR_SIZE */
    }

    if (mounted == 0 && volume != -1) /* none of the 4 entries worked? */
    {   /* try "superfloppy" mode */
        DEBUGF("No partition found, trying to mount sector 0.\n");

        if (!fat_mount(IF_MV(volume,) IF_MD(drive,) 0))
        {
        #ifdef MAX_LOG_SECTOR_SIZE
            disk_sector_multiplier[drive] =
                fat_get_bytes_per_sector(IF_MV(volume)) / SECTOR_SIZE;
        #endif
            mounted = 1;
            vol_drive[volume] = drive; /* remember the drive for this volume */
            volume_onmount_internal(IF_MV(volume));
        }
    }

    disk_writer_unlock();
    return mounted;
}

int disk_mount_all(void)
{
    int mounted = 0;

    disk_writer_lock();

    /* reset all mounted partitions */
    volume_onunmount_internal(IF_MV(-1));
    fat_init();

    for (int i = 0; i < NUM_VOLUMES; i++)
        vol_drive[i] = -1; /* mark all as unassigned */

    for (int i = 0; i < NUM_DRIVES; i++)
    {
    #ifdef HAVE_HOTSWAP
        if (storage_present(i))
    #endif
            mounted += disk_mount(i);
    }

    disk_writer_unlock();
    return mounted;
}

int disk_unmount(int drive)
{
    if (!CHECK_DRV(drive))
        return 0;

    int unmounted = 0;

    disk_writer_lock();

    for (int i = 0; i < NUM_VOLUMES; i++)
    {
        if (vol_drive[i] == drive)
        {   /* force releasing resources */
            vol_drive[i] = -1; /* mark unused */

            volume_onunmount_internal(IF_MV(i));
            fat_unmount(IF_MV(i));

            unmounted++;
        }
    }

    disk_writer_unlock();
    return unmounted;
}

int disk_unmount_all(void)
{
    int unmounted = 0;

    disk_writer_lock();

    volume_onunmount_internal(IF_MV(-1));

    for (int i = 0; i < NUM_DRIVES; i++)
    {
    #ifdef HAVE_HOTSWAP
        if (storage_present(i))
    #endif
            unmounted += disk_unmount(i);
    }

    disk_writer_unlock();
    return unmounted;
}

bool disk_present(IF_MD_NONVOID(int drive))
{
    int rc = -1;

    if (CHECK_DRV(drive))
    {
        void *sector = dc_get_buffer();
        if (sector)
        {
            rc = storage_read_sectors(IF_MD(drive,) 0, 1, sector);
            dc_release_buffer(sector);
        }
    }

    return rc == 0;
}


/** Volume-centric functions **/

void volume_recalc_free(IF_MV_NONVOID(int volume))
{
    if (!CHECK_VOL(volume))
        return;

    /* FIXME: this is crummy but the only way to ensure a correct freecount
       if other threads are writing and changing the fsinfo; it is possible
       to get multiple threads calling here and also writing and get correct
       freespace counts, however a bit complicated to do; if thou desireth I
       shall implement the concurrent version -- jethead71 */
    disk_writer_lock();
    fat_recalc_free(IF_MV(volume));
    disk_writer_unlock();
}

unsigned int volume_get_cluster_size(IF_MV_NONVOID(int volume))
{
    if (!CHECK_VOL(volume))
        return 0;

    disk_reader_lock();
    unsigned int clustersize = fat_get_cluster_size(IF_MV(volume));
    disk_reader_unlock();
    return clustersize;
}

void volume_size(IF_MV(int volume,) unsigned long *sizep, unsigned long *freep)
{
    disk_reader_lock();

    if (!CHECK_VOL(volume) || !fat_size(IF_MV(volume,) sizep, freep))
    {
        if (freep) *sizep = 0;
        if (freep) *freep = 0;
    }

    disk_reader_unlock();
}

#if defined (HAVE_HOTSWAP) || defined (HAVE_MULTIDRIVE) \
    || defined (HAVE_DIRCACHE)
enum volume_info_type
{
#ifdef HAVE_HOTSWAP
    VP_REMOVABLE,
    VP_PRESENT,
#endif
#if defined (HAVE_MULTIDRIVE) || defined (HAVE_DIRCACHE)
    VP_DRIVE,
#endif
};

static int volume_properties(int volume, enum volume_info_type infotype)
{
    int res = -1;

    disk_reader_lock();

    if (CHECK_VOL(volume))
    {
        int vd = vol_drive[volume];
        switch (infotype)
        {
    #ifdef HAVE_HOTSWAP
        case VP_REMOVABLE:
            res = storage_removable(vd) ? 1 : 0;
            break;
        case VP_PRESENT:
            res = storage_present(vd) ? 1 : 0;
            break;
    #endif
    #if defined(HAVE_MULTIDRIVE) || defined(HAVE_DIRCACHE)
        case VP_DRIVE:
            res = vd;
            break;
    #endif
        }
    }

    disk_reader_unlock();
    return res;
}

#ifdef HAVE_HOTSWAP
bool volume_removable(int volume)
{
    return volume_properties(volume, VP_REMOVABLE) > 0;
}

bool volume_present(int volume)
{
    return volume_properties(volume, VP_PRESENT) > 0;
}
#endif /* HAVE_HOTSWAP */

#ifdef HAVE_MULTIDRIVE
int volume_drive(int volume)
{
    return volume_properties(volume, VP_DRIVE);
}
#endif /* HAVE_MULTIDRIVE */

#ifdef HAVE_DIRCACHE
bool volume_ismounted(IF_MV_NONVOID(int volume))
{
    return volume_properties(IF_MV_VOL(volume), VP_DRIVE) >= 0;
}
#endif /* HAVE_DIRCACHE */

#endif /* HAVE_HOTSWAP || HAVE_MULTIDRIVE || HAVE_DIRCACHE */
