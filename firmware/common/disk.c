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
#include "rb_namespace.h"
#include "disk.h"
#include "panic.h"

#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR) && !defined(BOOTLOADER)
#include "bootdata.h"
#include "crc32.h"
#endif

#ifndef CONFIG_DEFAULT_PARTNUM
#define CONFIG_DEFAULT_PARTNUM 0
#endif

#define disk_reader_lock()      file_internal_lock_READER()
#define disk_reader_unlock()    file_internal_unlock_READER()
#define disk_writer_lock()      file_internal_lock_WRITER()
#define disk_writer_unlock()    file_internal_unlock_WRITER()

/* "MBR" Partition table entry layout:
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

#define BYTES2INT64(array, pos) \
    (((uint64_t)array[pos+0] <<  0) | \
     ((uint64_t)array[pos+1] <<  8) | \
     ((uint64_t)array[pos+2] << 16) | \
     ((uint64_t)array[pos+3] << 24) | \
     ((uint64_t)array[pos+4] << 32) | \
     ((uint64_t)array[pos+5] << 40) | \
     ((uint64_t)array[pos+6] << 48) | \
     ((uint64_t)array[pos+7] << 56) )

#define BYTES2INT32(array, pos) \
    (((uint32_t)array[pos+0] <<  0) | \
     ((uint32_t)array[pos+1] <<  8) | \
     ((uint32_t)array[pos+2] << 16) | \
     ((uint32_t)array[pos+3] << 24))

#define BYTES2INT16(array, pos) \
    (((uint16_t)array[pos+0] << 0) | \
     ((uint16_t)array[pos+1] << 8))

static struct partinfo part[NUM_DRIVES*MAX_PARTITIONS_PER_DRIVE];
static struct volumeinfo volumes[NUM_VOLUMES];

/* check if the entry points to a free volume */
static bool is_free_volume(const struct volumeinfo *vi)
{
    return vi->drive < 0;
}

/* mark a volume entry as free */
static void mark_free_volume(struct volumeinfo *vi)
{
    vi->drive = -1;
    vi->partition = -1;
}

static int get_free_volume(void)
{
    for (int i = 0; i < NUM_VOLUMES; i++)
        if (is_free_volume(&volumes[i]))
            return i;

    return -1; /* none found */
}

static void init_volume(struct volumeinfo *vi, int drive, int part)
{
    vi->drive = drive;
    vi->partition = part;
}

#ifdef MAX_LOG_SECTOR_SIZE
static uint16_t disk_sector_multiplier[NUM_DRIVES] =
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

#if (CONFIG_STORAGE & STORAGE_ATA)  // XXX make this more generic?
static uint16_t disk_log_sector_size[NUM_DRIVES] =
    { [0 ... NUM_DRIVES-1] = SECTOR_SIZE }; /* Updated from STORAGE_INFO */
int disk_get_log_sector_size(IF_MD_NONVOID(int drive))
{
    if (!CHECK_DRV(drive))
        return 0;

    disk_reader_lock();
    int size = disk_log_sector_size[IF_MD_DRV(drive)];
    disk_reader_unlock();
    return size;
}
#define LOG_SECTOR_SIZE(__drive) disk_log_sector_size[IF_MD_DRV(__drive)]
#else /* !STORAGE_ATA */
#define LOG_SECTOR_SIZE(__drive) SECTOR_SIZE
int disk_get_log_sector_size(IF_MD_NONVOID(int drive))
{
    IF_MD((void)drive);
    return SECTOR_SIZE;
}
#endif /* !CONFIG_STORAGE & STORAGE_ATA */

bool disk_init(IF_MD_NONVOID(int drive))
{
    if (!CHECK_DRV(drive))
        return false; /* out of space in table */

    unsigned char *sector = dc_get_buffer();
    if (!sector)
        return false;

#if (CONFIG_STORAGE & STORAGE_ATA)
    /* Query logical sector size */
    struct storage_info *info = (struct storage_info*) sector;
    storage_get_info(IF_MD_DRV(drive), info);
    disk_writer_lock();
#ifdef MAX_LOG_SECTOR_SIZE
    disk_log_sector_size[IF_MD_DRV(drive)] = info->sector_size;
#endif
    disk_writer_unlock();

#ifdef MAX_LOG_SECTOR_SIZE
    if (info->sector_size > MAX_LOG_SECTOR_SIZE || info->sector_size > DC_CACHE_BUFSIZE) {
        panicf("Unsupported logical sector size: %d",
               info->sector_size);
    }
#else
    if (info->sector_size != SECTOR_SIZE) {
        panicf("Unsupported logical sector size: %d",
               info->sector_size);
    }
#endif
#endif /* CONFIG_STORAGE & STORAGE_ATA */

    memset(sector, 0, DC_CACHE_BUFSIZE);
    storage_read_sectors(IF_MD(drive,) 0, 1, sector);

    bool init = false;

    /* check that the boot sector is initialized */
    if (BYTES2INT16(sector, 510) == 0xaa55)
    {
        /* For each drive, start at a different position, in order not to
           destroy the first entry of drive 0. That one is needed to calculate
           config sector position. */
        struct partinfo *pinfo = &part[IF_MD_DRV(drive)*MAX_PARTITIONS_PER_DRIVE];
	uint8_t is_gpt = 0;

        disk_writer_lock();

        /* parse partitions */
        for (int i = 0; i < MAX_PARTITIONS_PER_DRIVE && i < 4; i++)
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

            if (pinfo[i].type == PARTITION_TYPE_GPT_GUARD) {
		is_gpt = 1;
	    }
	}

        // XXX backup GPT header at final LBA of drive...

	while (is_gpt) {
	    /* Re-start partition parsing using GPT */
	    uint64_t part_lba;
	    uint32_t part_entries;
	    uint32_t part_entry_size;
            unsigned char* ptr = sector;

	    storage_read_sectors(IF_MD(drive,) 1, 1, sector);

	    part_lba = BYTES2INT64(ptr, 0);
            if (part_lba != 0x5452415020494645ULL) {
                DEBUGF("GPT: Invalid signature\n");
                break;
            }
            part_entry_size = BYTES2INT32(ptr, 8);
            if (part_entry_size != 0x00010000) {
                DEBUGF("GPT: Invalid version\n");
                break;
            }
            part_entry_size = BYTES2INT32(ptr, 12);
            if (part_entry_size != 0x5c) {
                DEBUGF("GPT: Invalid header size\n");
                break;
            }
            // XXX checksum header -- u32 @ offset 16
            part_entry_size = BYTES2INT32(ptr, 24);
            if (part_entry_size != 1) {
                DEBUGF("GPT: Invalid header LBA\n");
                break;
            }

	    part_lba = BYTES2INT64(ptr, 72);
	    part_entries = BYTES2INT32(ptr, 80);
	    part_entry_size = BYTES2INT32(ptr, 84);

	    int part = 0;
reload:
	    storage_read_sectors(IF_MD(drive,) part_lba, 1, sector);
            uint8_t *pptr = ptr;
            while (part < MAX_PARTITIONS_PER_DRIVE && part_entries) {
                if (pptr - ptr >= LOG_SECTOR_SIZE(drive)) {
                    part_lba++;
                    goto reload;
                }

		/* Parse GPT entry.  We only care about the "General Data" type, ie:
		     EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
                     LE32     LE16 LE16 BE16 BE16
                */
                uint64_t tmp;
                tmp = BYTES2INT32(pptr, 0);
                if (tmp != 0xEBD0A0A2)
                    goto skip;
                tmp = BYTES2INT16(pptr, 4);
                if (tmp != 0xB9E5)
                    goto skip;
                tmp = BYTES2INT16(pptr, 6);
                if (tmp != 0x4433)
                    goto skip;
                if (pptr[8] != 0x87 || pptr[9] != 0xc0)
                    goto skip;
                if (pptr[10] != 0x68 || pptr[11] != 0xb6 || pptr[12] != 0xb7 ||
                    pptr[13] != 0x26 || pptr[14] != 0x99 || pptr[15] != 0xc7)
                    goto skip;

                tmp = BYTES2INT64(pptr, 48); /* Flags */
                if (tmp) {
                    DEBUGF("GPT: Skip parition with flags\n");
                    goto skip; /* Any flag makes us ignore this */
                }
                tmp = BYTES2INT64(pptr, 32); /* FIRST LBA */
#ifndef STORAGE_64BIT_SECTOR
                if (tmp > UINT32_MAX) {
                    DEBUGF("GPT: partition starts after 2TiB mark\n");
                    goto skip;
                }
#endif
                if (tmp < 34) {
                    DEBUGF("GPT: Invalid start LBA\n");
                    goto skip;
                }
                pinfo[part].start = tmp;
                tmp = BYTES2INT64(pptr, 40); /* LAST LBA */
#ifndef STORAGE_64BIT_SECTOR
                if (tmp > UINT32_MAX) {
                    DEBUGF("GPT: partition ends after 2TiB mark\n");
                    goto skip;
                }
#endif
                if (tmp <= pinfo[part].start) {
                    DEBUGF("GPT: Invalid end LBA\n");
                    goto skip;
                }
                pinfo[part].size = tmp - pinfo[part].start + 1;
                pinfo[part].type = PARTITION_TYPE_FAT32_LBA;

                DEBUGF("GPart%d: start: %016lx size: %016lx\n",
                       part,pinfo[part].start,pinfo[part].size);
                part++;

            skip:
                pptr += part_entry_size;
                part_entries--;
            }

            is_gpt = 0; /* To break out of the loop */
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

    if (volume < 0)
    {
        DEBUGF("No Free Volumes\n");
        disk_writer_unlock();
        return 0;
    }

    if (!disk_init(IF_MD(drive)))
    {
        disk_writer_unlock();
        return 0;
    }

    struct partinfo *pinfo = &part[IF_MD_DRV(drive)*4];
#ifdef MAX_LOG_SECTOR_SIZE
    disk_sector_multiplier[IF_MD_DRV(drive)] = 1;
#endif

    /* try "superfloppy" mode */
    DEBUGF("Trying to mount sector 0.\n");

    if (!fat_mount(IF_MV(volume,) IF_MD(drive,) 0))
    {
#ifdef MAX_LOG_SECTOR_SIZE
        disk_sector_multiplier[drive] = fat_get_bytes_per_sector(IF_MV(volume)) / LOG_SECTOR_SIZE(drive);
#endif
        mounted = 1;
        init_volume(&volumes[volume], drive, 0);
        volume_onmount_internal(IF_MV(volume));
    }

    if (mounted == 0 && volume != -1) /* not a "superfloppy"? */
    {
        for (int i = CONFIG_DEFAULT_PARTNUM;
             volume != -1 && i < MAX_PARTITIONS_PER_DRIVE && mounted < NUM_VOLUMES_PER_DRIVE;
             i++)
        {
            if (pinfo[i].type == 0 || pinfo[i].type == 5)
                continue;  /* skip free/extended partitions */

            DEBUGF("Trying to mount partition %d.\n", i);

#ifdef MAX_LOG_SECTOR_SIZE
            for (int j = 1; j <= (MAX_LOG_SECTOR_SIZE/LOG_SECTOR_SIZE(drive)); j <<= 1)
            {
                if (!fat_mount(IF_MV(volume,) IF_MD(drive,) pinfo[i].start * j))
                {
                    pinfo[i].start *= j;
                    pinfo[i].size *= j;
                    mounted++;
                    init_volume(&volumes[volume], drive, i);
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
                init_volume(&volumes[volume], drive, i);
                volume_onmount_internal(IF_MV(volume));
                volume = get_free_volume(); /* prepare next entry */
            }
#endif /* MAX_LOG_SECTOR_SIZE */
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

    /* mark all volumes as free */
    for (int i = 0; i < NUM_VOLUMES; i++)
        mark_free_volume(&volumes[i]);

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
        struct volumeinfo *vi = &volumes[i];
        /* unmount any volumes on the drive */
        if (vi->drive == drive)
        {
            mark_free_volume(vi); /* FIXME: should do this after unmount? */
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

void volume_size(IF_MV(int volume,) sector_t *sizep, sector_t *freep)
{
    disk_reader_lock();

    if (!CHECK_VOL(volume) || !fat_size(IF_MV(volume,) sizep, freep))
    {
        if (sizep) *sizep = 0;
        if (freep) *freep = 0;
    }

    disk_reader_unlock();
}

#if defined (HAVE_HOTSWAP) || defined (HAVE_MULTIDRIVE) \
    || defined (HAVE_DIRCACHE) || defined(HAVE_BOOTDATA)
enum volume_info_type
{
#ifdef HAVE_HOTSWAP
    VP_REMOVABLE,
    VP_PRESENT,
#endif
#if defined (HAVE_MULTIDRIVE) || defined (HAVE_DIRCACHE)
    VP_DRIVE,
#endif
    VP_PARTITION,
};

static int volume_properties(int volume, enum volume_info_type infotype)
{
    int res = -1;

    disk_reader_lock();

    if (CHECK_VOL(volume))
    {
        struct volumeinfo *vi = &volumes[volume];
        switch (infotype)
        {
    #ifdef HAVE_HOTSWAP
        case VP_REMOVABLE:
            res = storage_removable(vi->drive) ? 1 : 0;
            break;
        case VP_PRESENT:
            res = storage_present(vi->drive) ? 1 : 0;
            break;
    #endif
    #if defined(HAVE_MULTIDRIVE) || defined(HAVE_DIRCACHE)
        case VP_DRIVE:
            res = vi->drive;
            break;
    #endif
        case VP_PARTITION:
            res = vi->partition;
            break;
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

int volume_partition(int volume)
{
    return volume_properties(volume, VP_PARTITION);
}

#ifdef HAVE_DIRCACHE
bool volume_ismounted(IF_MV_NONVOID(int volume))
{
    return volume_properties(IF_MV_VOL(volume), VP_DRIVE) >= 0;
}
#endif /* HAVE_DIRCACHE */


#endif /* HAVE_HOTSWAP || HAVE_MULTIDRIVE || HAVE_DIRCACHE || HAVE_BOOTDATA */
