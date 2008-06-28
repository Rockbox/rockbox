/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "fat.h"
#include "ata.h"
#include "debug.h"
#include "panic.h"
#include "system.h"
#include "timefuncs.h"
#include "kernel.h"
#include "rbunicode.h"
#include "logf.h"

#define BYTES2INT16(array,pos) \
          (array[pos] | (array[pos+1] << 8 ))
#define BYTES2INT32(array,pos) \
    ((long)array[pos] | ((long)array[pos+1] << 8 ) | \
    ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

#define FATTYPE_FAT12       0
#define FATTYPE_FAT16       1
#define FATTYPE_FAT32       2

/* BPB offsets; generic */
#define BS_JMPBOOT          0
#define BS_OEMNAME          3
#define BPB_BYTSPERSEC      11
#define BPB_SECPERCLUS      13
#define BPB_RSVDSECCNT      14
#define BPB_NUMFATS         16
#define BPB_ROOTENTCNT      17
#define BPB_TOTSEC16        19
#define BPB_MEDIA           21
#define BPB_FATSZ16         22
#define BPB_SECPERTRK       24
#define BPB_NUMHEADS        26
#define BPB_HIDDSEC         28
#define BPB_TOTSEC32        32

/* fat12/16 */
#define BS_DRVNUM           36
#define BS_RESERVED1        37
#define BS_BOOTSIG          38
#define BS_VOLID            39
#define BS_VOLLAB           43
#define BS_FILSYSTYPE       54

/* fat32 */
#define BPB_FATSZ32         36
#define BPB_EXTFLAGS        40
#define BPB_FSVER           42
#define BPB_ROOTCLUS        44
#define BPB_FSINFO          48
#define BPB_BKBOOTSEC       50
#define BS_32_DRVNUM        64
#define BS_32_BOOTSIG       66
#define BS_32_VOLID         67
#define BS_32_VOLLAB        71
#define BS_32_FILSYSTYPE    82

#define BPB_LAST_WORD       510


/* attributes */
#define FAT_ATTR_LONG_NAME   (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                              FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)
#define FAT_ATTR_LONG_NAME_MASK (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                                 FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | \
                                 FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE )

/* NTRES flags */
#define FAT_NTRES_LC_NAME    0x08
#define FAT_NTRES_LC_EXT     0x10

#define FATDIR_NAME          0
#define FATDIR_ATTR          11
#define FATDIR_NTRES         12
#define FATDIR_CRTTIMETENTH  13
#define FATDIR_CRTTIME       14
#define FATDIR_CRTDATE       16
#define FATDIR_LSTACCDATE    18
#define FATDIR_FSTCLUSHI     20
#define FATDIR_WRTTIME       22
#define FATDIR_WRTDATE       24
#define FATDIR_FSTCLUSLO     26
#define FATDIR_FILESIZE      28

#define FATLONG_ORDER        0
#define FATLONG_TYPE         12
#define FATLONG_CHKSUM       13

#define CLUSTERS_PER_FAT_SECTOR (SECTOR_SIZE / 4)
#define CLUSTERS_PER_FAT16_SECTOR (SECTOR_SIZE / 2)
#define DIR_ENTRIES_PER_SECTOR  (SECTOR_SIZE / DIR_ENTRY_SIZE)
#define DIR_ENTRY_SIZE       32
#define NAME_BYTES_PER_ENTRY 13
#define FAT_BAD_MARK         0x0ffffff7
#define FAT_EOF_MARK         0x0ffffff8
#define FAT_LONGNAME_PAD_BYTE 0xff
#define FAT_LONGNAME_PAD_UCS 0xffff

struct fsinfo {
    unsigned long freecount; /* last known free cluster count */
    unsigned long nextfree;  /* first cluster to start looking for free
                               clusters, or 0xffffffff for no hint */
};
/* fsinfo offsets */
#define FSINFO_FREECOUNT 488
#define FSINFO_NEXTFREE  492

/* Note: This struct doesn't hold the raw values after mounting if
 * bpb_bytspersec isn't 512. All sector counts are normalized to 512 byte
 * physical sectors. */
struct bpb
{
    int bpb_bytspersec;  /* Bytes per sector, typically 512 */
    unsigned int bpb_secperclus;  /* Sectors per cluster */
    int bpb_rsvdseccnt;  /* Number of reserved sectors */
    int bpb_numfats;     /* Number of FAT structures, typically 2 */
    int bpb_totsec16;    /* Number of sectors on the volume (old 16-bit) */
    int bpb_media;       /* Media type (typically 0xf0 or 0xf8) */
    int bpb_fatsz16;     /* Number of used sectors per FAT structure */
    unsigned long bpb_totsec32;    /* Number of sectors on the volume
                                     (new 32-bit) */
    unsigned int last_word;       /* 0xAA55 */

    /**** FAT32 specific *****/
    long bpb_fatsz32;
    long bpb_rootclus;
    long bpb_fsinfo;

    /* variables for internal use */
    unsigned long fatsize;
    unsigned long totalsectors;
    unsigned long rootdirsector;
    unsigned long firstdatasector;
    unsigned long startsector;
    unsigned long dataclusters;
    struct fsinfo fsinfo;
#ifdef HAVE_FAT16SUPPORT
    int bpb_rootentcnt;  /* Number of dir entries in the root */
    /* internals for FAT16 support */
    bool is_fat16; /* true if we mounted a FAT16 partition, false if FAT32 */
    unsigned int rootdiroffset; /* sector offset of root dir relative to start
                                 * of first pseudo cluster */
#endif /* #ifdef HAVE_FAT16SUPPORT */
#ifdef HAVE_MULTIVOLUME
    int drive; /* on which physical device is this located */
    bool mounted; /* flag if this volume is mounted */
#endif
};

static struct bpb fat_bpbs[NUM_VOLUMES]; /* mounted partition info */
static bool initialized = false;

static int update_fsinfo(IF_MV_NONVOID(struct bpb* fat_bpb));
static int flush_fat(IF_MV_NONVOID(struct bpb* fat_bpb));
static int bpb_is_sane(IF_MV_NONVOID(struct bpb* fat_bpb));
static void *cache_fat_sector(IF_MV2(struct bpb* fat_bpb,)
                              long secnum, bool dirty);
static void create_dos_name(const unsigned char *name, unsigned char *newname);
static void randomize_dos_name(unsigned char *name);
static unsigned long find_free_cluster(IF_MV2(struct bpb* fat_bpb,)
                                       unsigned long start);
static int transfer(IF_MV2(struct bpb* fat_bpb,) unsigned long start,
                    long count, char* buf, bool write );

#define FAT_CACHE_SIZE 0x20
#define FAT_CACHE_MASK (FAT_CACHE_SIZE-1)

struct fat_cache_entry
{
    long secnum;
    bool inuse;
    bool dirty;
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_vol ; /* shared cache for all volumes */
#endif
};

static char fat_cache_sectors[FAT_CACHE_SIZE][SECTOR_SIZE];
static struct fat_cache_entry fat_cache[FAT_CACHE_SIZE];
static struct mutex cache_mutex SHAREDBSS_ATTR;

#if defined(HAVE_HOTSWAP) && !defined(HAVE_MMC) /* A better condition ?? */
void fat_lock(void)
{
    mutex_lock(&cache_mutex);
}

void fat_unlock(void)
{
    mutex_unlock(&cache_mutex);
}
#endif

static long cluster2sec(IF_MV2(struct bpb* fat_bpb,) long cluster)
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
#ifdef HAVE_FAT16SUPPORT
    /* negative clusters (FAT16 root dir) don't get the 2 offset */
    int zerocluster = cluster < 0 ? 0 : 2;
#else
    const long zerocluster = 2;
#endif

    if (cluster > (long)(fat_bpb->dataclusters + 1))
    {
      DEBUGF( "cluster2sec() - Bad cluster number (%ld)\n", cluster);
        return -1;
    }

    return (cluster - zerocluster) * fat_bpb->bpb_secperclus 
           + fat_bpb->firstdatasector;
}

void fat_size(IF_MV2(int volume,) unsigned long* size, unsigned long* free)
{
#ifndef HAVE_MULTIVOLUME
    const int volume = 0;
#endif
    struct bpb* fat_bpb = &fat_bpbs[volume];
    if (size)
      *size = fat_bpb->dataclusters * fat_bpb->bpb_secperclus / 2;
    if (free)
      *free = fat_bpb->fsinfo.freecount * fat_bpb->bpb_secperclus / 2;
}

void fat_init(void)
{
    unsigned int i;

    if (!initialized)
    {
        initialized = true;
        mutex_init(&cache_mutex);
    }

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Disable this because it is dangerous due to the assumption that
     * mutex_unlock won't yield */
    mutex_set_preempt(&cache_mutex, false);
#endif

    /* mark the FAT cache as unused */
    for(i = 0;i < FAT_CACHE_SIZE;i++)
    {
        fat_cache[i].secnum = 8; /* We use a "safe" sector just in case */
        fat_cache[i].inuse = false;
        fat_cache[i].dirty = false;
#ifdef HAVE_MULTIVOLUME
        fat_cache[i].fat_vol = NULL;
#endif
    }
#ifdef HAVE_MULTIVOLUME
    /* mark the possible volumes as not mounted */
    for (i=0; i<NUM_VOLUMES;i++)
    {
        fat_bpbs[i].mounted = false;
    }
#endif
}

int fat_mount(IF_MV2(int volume,) IF_MV2(int drive,) long startsector)
{
#ifndef HAVE_MULTIVOLUME
    const int volume = 0;
#endif
    struct bpb* fat_bpb = &fat_bpbs[volume];
    unsigned char buf[SECTOR_SIZE];
    int rc;
    int secmult;
    long datasec;
#ifdef HAVE_FAT16SUPPORT
    int rootdirsectors;
#endif

    /* Read the sector */
    rc = ata_read_sectors(IF_MV2(drive,) startsector,1,buf);
    if(rc)
    {
        DEBUGF( "fat_mount() - Couldn't read BPB (error code %d)\n", rc);
        return rc * 10 - 1;
    }

    memset(fat_bpb, 0, sizeof(struct bpb));
    fat_bpb->startsector    = startsector;
#ifdef HAVE_MULTIVOLUME
    fat_bpb->drive          = drive;
#endif

    fat_bpb->bpb_bytspersec = BYTES2INT16(buf,BPB_BYTSPERSEC);
    secmult = fat_bpb->bpb_bytspersec / SECTOR_SIZE; 
    /* Sanity check is performed later */

    fat_bpb->bpb_secperclus = secmult * buf[BPB_SECPERCLUS];
    fat_bpb->bpb_rsvdseccnt = secmult * BYTES2INT16(buf,BPB_RSVDSECCNT);
    fat_bpb->bpb_numfats    = buf[BPB_NUMFATS];
    fat_bpb->bpb_media      = buf[BPB_MEDIA];
    fat_bpb->bpb_fatsz16    = secmult * BYTES2INT16(buf,BPB_FATSZ16);
    fat_bpb->bpb_fatsz32    = secmult * BYTES2INT32(buf,BPB_FATSZ32);
    fat_bpb->bpb_totsec16   = secmult * BYTES2INT16(buf,BPB_TOTSEC16);
    fat_bpb->bpb_totsec32   = secmult * BYTES2INT32(buf,BPB_TOTSEC32);
    fat_bpb->last_word      = BYTES2INT16(buf,BPB_LAST_WORD);

    /* calculate a few commonly used values */
    if (fat_bpb->bpb_fatsz16 != 0)
        fat_bpb->fatsize = fat_bpb->bpb_fatsz16;
    else
        fat_bpb->fatsize = fat_bpb->bpb_fatsz32;

    if (fat_bpb->bpb_totsec16 != 0)
        fat_bpb->totalsectors = fat_bpb->bpb_totsec16;
    else
        fat_bpb->totalsectors = fat_bpb->bpb_totsec32;

#ifdef HAVE_FAT16SUPPORT
    fat_bpb->bpb_rootentcnt = BYTES2INT16(buf,BPB_ROOTENTCNT);
    if (!fat_bpb->bpb_bytspersec)
        return -2;
    rootdirsectors = secmult * ((fat_bpb->bpb_rootentcnt * DIR_ENTRY_SIZE
                     + fat_bpb->bpb_bytspersec - 1) / fat_bpb->bpb_bytspersec);
#endif /* #ifdef HAVE_FAT16SUPPORT */

    fat_bpb->firstdatasector = fat_bpb->bpb_rsvdseccnt
#ifdef HAVE_FAT16SUPPORT
        + rootdirsectors
#endif
        + fat_bpb->bpb_numfats * fat_bpb->fatsize;

    /* Determine FAT type */
    datasec = fat_bpb->totalsectors - fat_bpb->firstdatasector;
    if (fat_bpb->bpb_secperclus)
        fat_bpb->dataclusters = datasec / fat_bpb->bpb_secperclus;
    else
       return -2;

#ifdef TEST_FAT
    /*
      we are sometimes testing with "illegally small" fat32 images,
      so we don't use the proper fat32 test case for test code
    */
    if ( fat_bpb->bpb_fatsz16 )
#else
    if ( fat_bpb->dataclusters < 65525 )
#endif
    { /* FAT16 */
#ifdef HAVE_FAT16SUPPORT
        fat_bpb->is_fat16 = true;
        if (fat_bpb->dataclusters < 4085)
        { /* FAT12 */
            DEBUGF("This is FAT12. Go away!\n");
            return -2;
        }
#else /* #ifdef HAVE_FAT16SUPPORT */
        DEBUGF("This is not FAT32. Go away!\n");
        return -2;
#endif /* #ifndef HAVE_FAT16SUPPORT */
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    { /* FAT16 specific part of BPB */
        int dirclusters;  
        fat_bpb->rootdirsector = fat_bpb->bpb_rsvdseccnt
            + fat_bpb->bpb_numfats * fat_bpb->bpb_fatsz16;
        dirclusters = ((rootdirsectors + fat_bpb->bpb_secperclus - 1)
            / fat_bpb->bpb_secperclus); /* rounded up, to full clusters */
        /* I assign negative pseudo cluster numbers for the root directory,
           their range is counted upward until -1. */
        fat_bpb->bpb_rootclus = 0 - dirclusters; /* backwards, before the data*/
        fat_bpb->rootdiroffset = dirclusters * fat_bpb->bpb_secperclus
            - rootdirsectors;
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    { /* FAT32 specific part of BPB */
        fat_bpb->bpb_rootclus  = BYTES2INT32(buf,BPB_ROOTCLUS);
        fat_bpb->bpb_fsinfo    = secmult * BYTES2INT16(buf,BPB_FSINFO);
        fat_bpb->rootdirsector = cluster2sec(IF_MV2(fat_bpb,)
                                             fat_bpb->bpb_rootclus);
    }

    rc = bpb_is_sane(IF_MV(fat_bpb));
    if (rc < 0)
    {
        DEBUGF( "fat_mount() - BPB is not sane\n");
        return rc * 10 - 3;
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        fat_bpb->fsinfo.freecount = 0xffffffff; /* force recalc below */
        fat_bpb->fsinfo.nextfree = 0xffffffff;
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        /* Read the fsinfo sector */
        rc = ata_read_sectors(IF_MV2(drive,) 
            startsector + fat_bpb->bpb_fsinfo, 1, buf);
        if (rc < 0)
        {
            DEBUGF( "fat_mount() - Couldn't read FSInfo (error code %d)\n", rc);
            return rc * 10 - 4;
        }
        fat_bpb->fsinfo.freecount = BYTES2INT32(buf, FSINFO_FREECOUNT);
        fat_bpb->fsinfo.nextfree = BYTES2INT32(buf, FSINFO_NEXTFREE);
    }

    /* calculate freecount if unset */
    if ( fat_bpb->fsinfo.freecount == 0xffffffff )
    {
        fat_recalc_free(IF_MV(volume));
    }

    LDEBUGF("Freecount: %ld\n",fat_bpb->fsinfo.freecount);
    LDEBUGF("Nextfree: 0x%lx\n",fat_bpb->fsinfo.nextfree);
    LDEBUGF("Cluster count: 0x%lx\n",fat_bpb->dataclusters);
    LDEBUGF("Sectors per cluster: %d\n",fat_bpb->bpb_secperclus);
    LDEBUGF("FAT sectors: 0x%lx\n",fat_bpb->fatsize);

#ifdef HAVE_MULTIVOLUME
    fat_bpb->mounted = true;
#endif

    return 0;
}

#ifdef HAVE_HOTSWAP
int fat_unmount(int volume, bool flush)
{
    int rc;
    struct bpb* fat_bpb = &fat_bpbs[volume];

    if(flush)
    {
        rc = flush_fat(fat_bpb); /* the clean way, while still alive */
    }
    else
    {   /* volume is not accessible any more, e.g. MMC removed */
        int i;
        mutex_lock(&cache_mutex);
        for(i = 0;i < FAT_CACHE_SIZE;i++)
        {
            struct fat_cache_entry *fce = &fat_cache[i];
            if(fce->inuse && fce->fat_vol == fat_bpb)
            {
                fce->inuse = false; /* discard all from that volume */
                fce->dirty = false;
            }
        }
        mutex_unlock(&cache_mutex);
        rc = 0;
    }
    fat_bpb->mounted = false;
    return rc;
}
#endif /* #ifdef HAVE_HOTSWAP */

void fat_recalc_free(IF_MV_NONVOID(int volume))
{
#ifndef HAVE_MULTIVOLUME
    const int volume = 0;
#endif
    struct bpb* fat_bpb = &fat_bpbs[volume];
    long free = 0;
    unsigned long i;
#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        for (i = 0; i<fat_bpb->fatsize; i++) {
            unsigned int j;
            unsigned short* fat = cache_fat_sector(IF_MV2(fat_bpb,) i, false);
            for (j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++) {
                unsigned int c = i * CLUSTERS_PER_FAT16_SECTOR + j;
                if ( c > fat_bpb->dataclusters+1 ) /* nr 0 is unused */
                    break;

                if (letoh16(fat[j]) == 0x0000) {
                    free++;
                    if ( fat_bpb->fsinfo.nextfree == 0xffffffff )
                        fat_bpb->fsinfo.nextfree = c;
                }
            }
        }
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        for (i = 0; i<fat_bpb->fatsize; i++) {
            unsigned int j;
            unsigned long* fat = cache_fat_sector(IF_MV2(fat_bpb,) i, false);
            for (j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++) {
                unsigned long c = i * CLUSTERS_PER_FAT_SECTOR + j;
                if ( c > fat_bpb->dataclusters+1 ) /* nr 0 is unused */
                    break;

                if (!(letoh32(fat[j]) & 0x0fffffff)) {
                    free++;
                    if ( fat_bpb->fsinfo.nextfree == 0xffffffff )
                        fat_bpb->fsinfo.nextfree = c;
                }
            }
        }
    }
    fat_bpb->fsinfo.freecount = free;
    update_fsinfo(IF_MV(fat_bpb));
}

static int bpb_is_sane(IF_MV_NONVOID(struct bpb* fat_bpb))
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    if(fat_bpb->bpb_bytspersec % SECTOR_SIZE)
    {
        DEBUGF( "bpb_is_sane() - Error: sector size is not sane (%d)\n",
                fat_bpb->bpb_bytspersec);
        return -1;
    }
    if((long)fat_bpb->bpb_secperclus * (long)fat_bpb->bpb_bytspersec
                                                                   > 128L*1024L)
    {
        DEBUGF( "bpb_is_sane() - Error: cluster size is larger than 128K "
                "(%d * %d = %d)\n",
                fat_bpb->bpb_bytspersec, fat_bpb->bpb_secperclus,
                fat_bpb->bpb_bytspersec * fat_bpb->bpb_secperclus);
        return -2;
    }
    if(fat_bpb->bpb_numfats != 2)
    {
        DEBUGF( "bpb_is_sane() - Warning: NumFATS is not 2 (%d)\n",
                fat_bpb->bpb_numfats);
    }
    if(fat_bpb->bpb_media != 0xf0 && fat_bpb->bpb_media < 0xf8)
    {
        DEBUGF( "bpb_is_sane() - Warning: Non-standard "
                "media type (0x%02x)\n",
                fat_bpb->bpb_media);
    }
    if(fat_bpb->last_word != 0xaa55)
    {
        DEBUGF( "bpb_is_sane() - Error: Last word is not "
                "0xaa55 (0x%04x)\n", fat_bpb->last_word);
        return -3;
    }

    if (fat_bpb->fsinfo.freecount >
        (fat_bpb->totalsectors - fat_bpb->firstdatasector)/
        fat_bpb->bpb_secperclus)
    {
        DEBUGF( "bpb_is_sane() - Error: FSInfo.Freecount > disk size "
                 "(0x%04lx)\n", fat_bpb->fsinfo.freecount);
        return -4;
    }

    return 0;
}

static void flush_fat_sector(struct fat_cache_entry *fce,
                             unsigned char *sectorbuf)
{
    int rc;
    long secnum;

    /* With multivolume, use only the FAT info from the cached sector! */
#ifdef HAVE_MULTIVOLUME
    secnum = fce->secnum + fce->fat_vol->startsector;
#else
    secnum = fce->secnum + fat_bpbs[0].startsector;
#endif

    /* Write to the first FAT */
    rc = ata_write_sectors(IF_MV2(fce->fat_vol->drive,)
                           secnum, 1,
                           sectorbuf);
    if(rc < 0)
    {
        panicf("flush_fat_sector() - Could not write sector %ld"
               " (error %d)\n",
               secnum, rc);
    }
#ifdef HAVE_MULTIVOLUME
    if(fce->fat_vol->bpb_numfats > 1)
#else
    if(fat_bpbs[0].bpb_numfats > 1)
#endif
    {
        /* Write to the second FAT */
#ifdef HAVE_MULTIVOLUME
        secnum += fce->fat_vol->fatsize;
#else
        secnum += fat_bpbs[0].fatsize;
#endif
        rc = ata_write_sectors(IF_MV2(fce->fat_vol->drive,)
                               secnum, 1, sectorbuf);
        if(rc < 0)
        {
            panicf("flush_fat_sector() - Could not write sector %ld"
                   " (error %d)\n",
                   secnum, rc);
        }
    }
    fce->dirty = false;
}

/* Note: The returned pointer is only safely valid until the next
         task switch! (Any subsequent ata read/write may yield.) */
static void *cache_fat_sector(IF_MV2(struct bpb* fat_bpb,)
                              long fatsector, bool dirty)
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    long secnum = fatsector + fat_bpb->bpb_rsvdseccnt;
    int cache_index = secnum & FAT_CACHE_MASK;
    struct fat_cache_entry *fce = &fat_cache[cache_index];
    unsigned char *sectorbuf = &fat_cache_sectors[cache_index][0];
    int rc;

    mutex_lock(&cache_mutex); /* make changes atomic */

    /* Delete the cache entry if it isn't the sector we want */
    if(fce->inuse && (fce->secnum != secnum
#ifdef HAVE_MULTIVOLUME
        || fce->fat_vol != fat_bpb
#endif
    ))
    {
        /* Write back if it is dirty */
        if(fce->dirty)
        {
            flush_fat_sector(fce, sectorbuf);
        }
        fce->inuse = false;
    }

    /* Load the sector if it is not cached */
    if(!fce->inuse)
    {
        rc = ata_read_sectors(IF_MV2(fat_bpb->drive,)
                              secnum + fat_bpb->startsector,1,
                              sectorbuf);
        if(rc < 0)
        {
            DEBUGF( "cache_fat_sector() - Could not read sector %ld"
                    " (error %d)\n", secnum, rc);
            mutex_unlock(&cache_mutex);
            return NULL;
        }
        fce->inuse = true;
        fce->secnum = secnum;
#ifdef HAVE_MULTIVOLUME
        fce->fat_vol = fat_bpb;
#endif
    }
    if (dirty)
        fce->dirty = true; /* dirt remains, sticky until flushed */
    mutex_unlock(&cache_mutex);
    return sectorbuf;
}

static unsigned long find_free_cluster(IF_MV2(struct bpb* fat_bpb,)
                                       unsigned long startcluster)
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    unsigned long sector;
    unsigned long offset;
    unsigned long i;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        sector = startcluster / CLUSTERS_PER_FAT16_SECTOR;
        offset = startcluster % CLUSTERS_PER_FAT16_SECTOR;

        for (i = 0; i<fat_bpb->fatsize; i++) {
            unsigned int j;
            unsigned int nr = (i + sector) % fat_bpb->fatsize;
            unsigned short* fat = cache_fat_sector(IF_MV2(fat_bpb,) nr, false);
            if ( !fat )
                break;
            for (j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++) {
                int k = (j + offset) % CLUSTERS_PER_FAT16_SECTOR;
                if (letoh16(fat[k]) == 0x0000) {
                    unsigned int c = nr * CLUSTERS_PER_FAT16_SECTOR + k;
                     /* Ignore the reserved clusters 0 & 1, and also
                        cluster numbers out of bounds */
                    if ( c < 2 || c > fat_bpb->dataclusters+1 )
                        continue;
                    LDEBUGF("find_free_cluster(%x) == %x\n",startcluster,c);
                    fat_bpb->fsinfo.nextfree = c;
                    return c;
                }
            }
            offset = 0;
        }
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        sector = startcluster / CLUSTERS_PER_FAT_SECTOR;
        offset = startcluster % CLUSTERS_PER_FAT_SECTOR;

        for (i = 0; i<fat_bpb->fatsize; i++) {
            unsigned int j;
            unsigned long nr = (i + sector) % fat_bpb->fatsize;
            unsigned long* fat = cache_fat_sector(IF_MV2(fat_bpb,) nr, false);
            if ( !fat )
                break;
            for (j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++) {
                int k = (j + offset) % CLUSTERS_PER_FAT_SECTOR;
                if (!(letoh32(fat[k]) & 0x0fffffff)) {
                    unsigned long c = nr * CLUSTERS_PER_FAT_SECTOR + k;
                     /* Ignore the reserved clusters 0 & 1, and also
                        cluster numbers out of bounds */
                    if ( c < 2 || c > fat_bpb->dataclusters+1 )
                        continue;
                    LDEBUGF("find_free_cluster(%lx) == %lx\n",startcluster,c);
                    fat_bpb->fsinfo.nextfree = c;
                    return c;
                }
            }
            offset = 0;
        }
    }

    LDEBUGF("find_free_cluster(%lx) == 0\n",startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry(IF_MV2(struct bpb* fat_bpb,) unsigned long entry,
                            unsigned long val)
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        int sector = entry / CLUSTERS_PER_FAT16_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT16_SECTOR;
        unsigned short* sec;

        val &= 0xFFFF;

        LDEBUGF("update_fat_entry(%x,%x)\n",entry,val);

        if (entry==val)
            panicf("Creating FAT loop: %lx,%lx\n",entry,val);

        if ( entry < 2 )
            panicf("Updating reserved FAT entry %ld.\n",entry);

        sec = cache_fat_sector(IF_MV2(fat_bpb,) sector, true);
        if (!sec)
        {
            DEBUGF( "update_fat_entry() - Could not cache sector %d\n", sector);
            return -1;
        }

        if ( val ) {
            if (letoh16(sec[offset]) == 0x0000 && fat_bpb->fsinfo.freecount > 0)
                fat_bpb->fsinfo.freecount--;
        }
        else {
            if (letoh16(sec[offset]))
                fat_bpb->fsinfo.freecount++;
        }

        LDEBUGF("update_fat_entry: %d free clusters\n",
                fat_bpb->fsinfo.freecount);

        sec[offset] = htole16(val);
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        long sector = entry / CLUSTERS_PER_FAT_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT_SECTOR;
        unsigned long* sec;

        LDEBUGF("update_fat_entry(%lx,%lx)\n",entry,val);

        if (entry==val)
            panicf("Creating FAT loop: %lx,%lx\n",entry,val);

        if ( entry < 2 )
            panicf("Updating reserved FAT entry %ld.\n",entry);

        sec = cache_fat_sector(IF_MV2(fat_bpb,) sector, true);
        if (!sec)
        {
            DEBUGF("update_fat_entry() - Could not cache sector %ld\n", sector);
            return -1;
        }

        if ( val ) {
            if (!(letoh32(sec[offset]) & 0x0fffffff) &&
                fat_bpb->fsinfo.freecount > 0)
                fat_bpb->fsinfo.freecount--;
        }
        else {
            if (letoh32(sec[offset]) & 0x0fffffff)
                fat_bpb->fsinfo.freecount++;
        }

        LDEBUGF("update_fat_entry: %ld free clusters\n",
                fat_bpb->fsinfo.freecount);

        /* don't change top 4 bits */
        sec[offset] &= htole32(0xf0000000);
        sec[offset] |= htole32(val & 0x0fffffff);
    }

    return 0;
}

static long read_fat_entry(IF_MV2(struct bpb* fat_bpb,) unsigned long entry)
{
#ifdef HAVE_FAT16SUPPORT
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    if (fat_bpb->is_fat16)
    {
        int sector = entry / CLUSTERS_PER_FAT16_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT16_SECTOR;
        unsigned short* sec;

        sec = cache_fat_sector(IF_MV2(fat_bpb,) sector, false);
        if (!sec)
        {
            DEBUGF( "read_fat_entry() - Could not cache sector %d\n", sector);
            return -1;
        }

        return letoh16(sec[offset]);
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        long sector = entry / CLUSTERS_PER_FAT_SECTOR;
        int offset = entry % CLUSTERS_PER_FAT_SECTOR;
        unsigned long* sec;

        sec = cache_fat_sector(IF_MV2(fat_bpb,) sector, false);
        if (!sec)
        {
            DEBUGF( "read_fat_entry() - Could not cache sector %ld\n", sector);
            return -1;
        }

        return letoh32(sec[offset]) & 0x0fffffff;
    }
}

static long get_next_cluster(IF_MV2(struct bpb* fat_bpb,) long cluster)
{
    long next_cluster;
    long eof_mark = FAT_EOF_MARK;

#ifdef HAVE_FAT16SUPPORT
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    if (fat_bpb->is_fat16)
    {
        eof_mark &= 0xFFFF; /* only 16 bit */
        if (cluster < 0) /* FAT16 root dir */
            return cluster + 1; /* don't use the FAT */
    }
#endif
    next_cluster = read_fat_entry(IF_MV2(fat_bpb,) cluster);

    /* is this last cluster in chain? */
    if ( next_cluster >= eof_mark )
        return 0;
    else
        return next_cluster;
}

static int update_fsinfo(IF_MV_NONVOID(struct bpb* fat_bpb))
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    unsigned char fsinfo[SECTOR_SIZE];
    unsigned long* intptr;
    int rc;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
        return 0; /* FAT16 has no FsInfo */
#endif /* #ifdef HAVE_FAT16SUPPORT */

    /* update fsinfo */
    rc = ata_read_sectors(IF_MV2(fat_bpb->drive,) 
                          fat_bpb->startsector + fat_bpb->bpb_fsinfo, 1,fsinfo);
    if (rc < 0)
    {
        DEBUGF( "flush_fat() - Couldn't read FSInfo (error code %d)\n", rc);
        return rc * 10 - 1;
    }
    intptr = (long*)&(fsinfo[FSINFO_FREECOUNT]);
    *intptr = htole32(fat_bpb->fsinfo.freecount);

    intptr = (long*)&(fsinfo[FSINFO_NEXTFREE]);
    *intptr = htole32(fat_bpb->fsinfo.nextfree);

    rc = ata_write_sectors(IF_MV2(fat_bpb->drive,)
                           fat_bpb->startsector + fat_bpb->bpb_fsinfo,1,fsinfo);
    if (rc < 0)
    {
        DEBUGF( "flush_fat() - Couldn't write FSInfo (error code %d)\n", rc);
        return rc * 10 - 2;
    }

    return 0;
}

static int flush_fat(IF_MV_NONVOID(struct bpb* fat_bpb))
{
    int i;
    int rc;
    unsigned char *sec;
    LDEBUGF("flush_fat()\n");

    mutex_lock(&cache_mutex);
    for(i = 0;i < FAT_CACHE_SIZE;i++)
    {
        struct fat_cache_entry *fce = &fat_cache[i];
        if(fce->inuse 
#ifdef HAVE_MULTIVOLUME
            && fce->fat_vol == fat_bpb
#endif
            && fce->dirty)
        {
            sec = fat_cache_sectors[i];
            flush_fat_sector(fce, sec);
        }
    }
    mutex_unlock(&cache_mutex);

    rc = update_fsinfo(IF_MV(fat_bpb));
    if (rc < 0)
        return rc * 10 - 3;

    return 0;
}

static void fat_time(unsigned short* date,
                     unsigned short* time,
                     unsigned short* tenth )
{
#if CONFIG_RTC
    struct tm* tm = get_time();

    if (date)
        *date = ((tm->tm_year - 80) << 9) |
            ((tm->tm_mon + 1) << 5) |
            tm->tm_mday;

    if (time)
        *time = (tm->tm_hour << 11) |
            (tm->tm_min << 5) |
            (tm->tm_sec >> 1);

    if (tenth)
        *tenth = (tm->tm_sec & 1) * 100;
#else
    /* non-RTC version returns an increment from the supplied time, or a
     * fixed standard time/date if no time given as input */
    bool next_day = false;

    if (time)
    {
        if (0 == *time)
        {
            /* set to 00:15:00 */
            *time = (15 << 5);
        }
        else
        {
            unsigned short mins = (*time >> 5) & 0x003F;
            unsigned short hours = (*time >> 11) & 0x001F;
            if ((mins += 10) >= 60)
            {
                mins = 0;
                hours++;
            }
            if ((++hours) >= 24)
            {
                hours = hours - 24;
                next_day = true;
            }
            *time = (hours << 11) | (mins << 5);
        }
    }

    if (date)
    {
        if (0 == *date)
        {
/* Macros to convert a 2-digit string to a decimal constant. 
   (YEAR), MONTH and DAY are set by the date command, which outputs
   DAY as 00..31 and MONTH as 01..12. The leading zero would lead to
   misinterpretation as an octal constant. */
#define S100(x) 1 ## x
#define C2DIG2DEC(x) (S100(x)-100)
            /* set to build date */
            *date = ((YEAR - 1980) << 9) | (C2DIG2DEC(MONTH) << 5)
                    | C2DIG2DEC(DAY);
        }
        else
        {
            unsigned short day = *date & 0x001F;
            unsigned short month = (*date >> 5) & 0x000F;
            unsigned short year = (*date >> 9) & 0x007F;
            if (next_day)
            {
                /* do a very simple day increment - never go above 28 days */
                if (++day > 28)
                {
                    day = 1;
                    if (++month > 12)
                    {
                        month = 1;
                        year++;
                    }
                }
                *date = (year << 9) | (month << 5) | day;
            }
        }
    }
    if (tenth)
        *tenth = 0;
#endif /* CONFIG_RTC */
}

static int write_long_name(struct fat_file* file,
                           unsigned int firstentry,
                           unsigned int numentries,
                           const unsigned char* name,
                           const unsigned char* shortname,
                           bool is_directory)
{
    unsigned char buf[SECTOR_SIZE];
    unsigned char* entry;
    unsigned int idx = firstentry % DIR_ENTRIES_PER_SECTOR;
    unsigned int sector = firstentry / DIR_ENTRIES_PER_SECTOR;
    unsigned char chksum = 0;
    unsigned int i, j=0;
    unsigned int nameidx=0, namelen = utf8length(name);
    int rc;
    unsigned short name_utf16[namelen + 1];

    LDEBUGF("write_long_name(file:%lx, first:%d, num:%d, name:%s)\n",
            file->firstcluster, firstentry, numentries, name);

    rc = fat_seek(file, sector);
    if (rc<0)
        return rc * 10 - 1;

    rc = fat_readwrite(file, 1, buf, false);
    if (rc<1)
        return rc * 10 - 2;

    /* calculate shortname checksum */
    for (i=11; i>0; i--)
        chksum = ((chksum & 1) ? 0x80 : 0) + (chksum >> 1) + shortname[j++];

    /* calc position of last name segment */
    if ( namelen > NAME_BYTES_PER_ENTRY )
        for (nameidx=0;
             nameidx < (namelen - NAME_BYTES_PER_ENTRY);
             nameidx += NAME_BYTES_PER_ENTRY);

    /* we need to convert the name first    */
    /* since it is written in reverse order */
    for (i = 0; i <= namelen; i++)
        name = utf8decode(name, &name_utf16[i]);

    for (i=0; i < numentries; i++) {
        /* new sector? */
        if ( idx >= DIR_ENTRIES_PER_SECTOR ) {
            /* update current sector */
            rc = fat_seek(file, sector);
            if (rc<0)
                return rc * 10 - 3;

            rc = fat_readwrite(file, 1, buf, true);
            if (rc<1)
                return rc * 10 - 4;

            /* read next sector */
            rc = fat_readwrite(file, 1, buf, false);
            if (rc<0) {
                LDEBUGF("Failed writing new sector: %d\n",rc);
                return rc * 10 - 5;
            }
            if (rc==0)
                /* end of dir */
                memset(buf, 0, sizeof buf);

            sector++;
            idx = 0;
        }

        entry = buf + idx * DIR_ENTRY_SIZE;

        /* verify this entry is free */
        if (entry[0] && entry[0] != 0xe5 )
            panicf("Dir entry %d in sector %x is not free! "
                   "%02x %02x %02x %02x",
                   idx, sector,
                   entry[0], entry[1], entry[2], entry[3]);

        memset(entry, 0, DIR_ENTRY_SIZE);
        if ( i+1 < numentries ) {
            /* longname entry */
            unsigned int k, l = nameidx;

            entry[FATLONG_ORDER] = numentries-i-1;
            if (i==0) {
                /* mark this as last long entry */
                entry[FATLONG_ORDER] |= 0x40;

                /* pad name with 0xffff  */
                for (k=1; k<11; k++) entry[k] = FAT_LONGNAME_PAD_BYTE;
                for (k=14; k<26; k++) entry[k] = FAT_LONGNAME_PAD_BYTE;
                for (k=28; k<32; k++) entry[k] = FAT_LONGNAME_PAD_BYTE;
            };
            /* set name */
            for (k=0; k<5 && l <= namelen; k++) {
                entry[k*2 + 1] = (unsigned char)(name_utf16[l] & 0xff);
                entry[k*2 + 2] = (unsigned char)(name_utf16[l++] >> 8);
            }
            for (k=0; k<6 && l <= namelen; k++) {
                entry[k*2 + 14] = (unsigned char)(name_utf16[l] & 0xff);
                entry[k*2 + 15] = (unsigned char)(name_utf16[l++] >> 8);
            }
            for (k=0; k<2 && l <= namelen; k++) {
                entry[k*2 + 28] = (unsigned char)(name_utf16[l] & 0xff);
                entry[k*2 + 29] = (unsigned char)(name_utf16[l++] >> 8);
            }

            entry[FATDIR_ATTR] = FAT_ATTR_LONG_NAME;
            entry[FATDIR_FSTCLUSLO] = 0;
            entry[FATLONG_TYPE] = 0;
            entry[FATLONG_CHKSUM] = chksum;
            LDEBUGF("Longname entry %d: %s\n", idx, name+nameidx);
        }
        else {
            /* shortname entry */
            unsigned short date=0, time=0, tenth=0;
            LDEBUGF("Shortname entry: %s\n", shortname);
            strncpy(entry + FATDIR_NAME, shortname, 11);
            entry[FATDIR_ATTR] = is_directory?FAT_ATTR_DIRECTORY:0;
            entry[FATDIR_NTRES] = 0;

            fat_time(&date, &time, &tenth);
            entry[FATDIR_CRTTIMETENTH] = tenth;
            *(unsigned short*)(entry + FATDIR_CRTTIME) = htole16(time);
            *(unsigned short*)(entry + FATDIR_WRTTIME) = htole16(time);
            *(unsigned short*)(entry + FATDIR_CRTDATE) = htole16(date);
            *(unsigned short*)(entry + FATDIR_WRTDATE) = htole16(date);
            *(unsigned short*)(entry + FATDIR_LSTACCDATE) = htole16(date);
        }
        idx++;
        nameidx -= NAME_BYTES_PER_ENTRY;
    }

    /* update last sector */
    rc = fat_seek(file, sector);
    if (rc<0)
        return rc * 10 - 6;

    rc = fat_readwrite(file, 1, buf, true);
    if (rc<1)
        return rc * 10 - 7;

    return 0;
}

static int fat_checkname(const unsigned char* newname)
{
    static const char invalid_chars[] = "\"*/:<>?\\|";
    int len = strlen(newname);
    /* More sanity checks are probably needed */
    if (len > 255 || newname[len - 1] == '.')
    {
        return -1;
    }
    while (*newname)
    {
        if (*newname < ' ' || strchr(invalid_chars, *newname) != NULL)
            return -1;
        newname++;
    }
    /* check trailing space(s) */
    if(*(--newname) == ' ')
        return -1;

    return 0;
}

static int add_dir_entry(struct fat_dir* dir,
                         struct fat_file* file,
                         const char* name,
                         bool is_directory,
                         bool dotdir)
{
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[dir->file.volume];
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    unsigned char buf[SECTOR_SIZE];
    unsigned char shortname[12];
    int rc;
    unsigned int sector;
    bool done = false;
    int entries_needed, entries_found = 0;
    int firstentry;

    LDEBUGF( "add_dir_entry(%s,%lx)\n",
             name, file->firstcluster);

    /* Don't check dotdirs name for validity */
    if (dotdir == false) {
        rc = fat_checkname(name);
        if (rc < 0) {
            /* filename is invalid */
            return rc * 10 - 1;
        }
    }

#ifdef HAVE_MULTIVOLUME
    file->volume = dir->file.volume; /* inherit the volume, to make sure */
#endif

    /* The "." and ".." directory entries must not be long names */
    if(dotdir) {
        int i;
        strncpy(shortname, name, 12);
        for(i = strlen(shortname); i < 12; i++)
            shortname[i] = ' ';

        entries_needed = 1;
    } else {
        create_dos_name(name, shortname);

        /* one dir entry needed for every 13 bytes of filename,
           plus one entry for the short name */
        entries_needed = (utf8length(name) + (NAME_BYTES_PER_ENTRY-1))
                         / NAME_BYTES_PER_ENTRY + 1;
    }

  restart:
    firstentry = -1;

    rc = fat_seek(&dir->file, 0);
    if (rc < 0)
        return rc * 10 - 2;

    /* step 1: search for free entries and check for duplicate shortname */
    for (sector = 0; !done; sector++)
    {
        unsigned int i;

        rc = fat_readwrite(&dir->file, 1, buf, false);
        if (rc < 0) {
            DEBUGF( "add_dir_entry() - Couldn't read dir"
                    " (error code %d)\n", rc);
            return rc * 10 - 3;
        }

        if (rc == 0) { /* current end of dir reached */
            LDEBUGF("End of dir on cluster boundary\n");
            break;
        }

        /* look for free slots */
        for (i = 0; i < DIR_ENTRIES_PER_SECTOR; i++)
        {
            switch (buf[i * DIR_ENTRY_SIZE]) {
              case 0:
                entries_found += DIR_ENTRIES_PER_SECTOR - i;
                LDEBUGF("Found end of dir %d\n",
                        sector * DIR_ENTRIES_PER_SECTOR + i);
                i = DIR_ENTRIES_PER_SECTOR - 1;
                done = true;
                break;

              case 0xe5:
                entries_found++;
                LDEBUGF("Found free entry %d (%d/%d)\n",
                        sector * DIR_ENTRIES_PER_SECTOR + i,
                        entries_found, entries_needed);
                break;

              default:
                entries_found = 0;

                /* check that our intended shortname doesn't already exist */
                if (!strncmp(shortname, buf + i * DIR_ENTRY_SIZE, 11)) {
                    /* shortname exists already, make a new one */
                    randomize_dos_name(shortname);
                    LDEBUGF("Duplicate shortname, changing to %s\n",
                            shortname);

                    /* name has changed, we need to restart search */
                    goto restart;
                }
                break;
            }
            if (firstentry < 0 && (entries_found >= entries_needed))
                firstentry = sector * DIR_ENTRIES_PER_SECTOR + i + 1
                             - entries_found;
        }
    }

    /* step 2: extend the dir if necessary */
    if (firstentry < 0)
    {
        LDEBUGF("Adding new sector(s) to dir\n");
        rc = fat_seek(&dir->file, sector);
        if (rc < 0)
            return rc * 10 - 4;
        memset(buf, 0, sizeof buf);

        /* we must clear whole clusters */
        for (; (entries_found < entries_needed) ||
               (dir->file.sectornum < (int)fat_bpb->bpb_secperclus); sector++)
        {
            if (sector >= (65536/DIR_ENTRIES_PER_SECTOR))
                return -5; /* dir too large -- FAT specification */

            rc = fat_readwrite(&dir->file, 1, buf, true);
            if (rc < 1)  /* No more room or something went wrong */
                return rc * 10 - 6;

            entries_found += DIR_ENTRIES_PER_SECTOR;
        }

        firstentry = sector * DIR_ENTRIES_PER_SECTOR - entries_found;
    }

    /* step 3: add entry */
    sector = firstentry / DIR_ENTRIES_PER_SECTOR;
    LDEBUGF("Adding longname to entry %d in sector %d\n",
            firstentry, sector);

    rc = write_long_name(&dir->file, firstentry,
                         entries_needed, name, shortname, is_directory);
    if (rc < 0)
        return rc * 10 - 7;

    /* remember where the shortname dir entry is located */
    file->direntry = firstentry + entries_needed - 1;
    file->direntries = entries_needed;
    file->dircluster = dir->file.firstcluster;
    LDEBUGF("Added new dir entry %d, using %d slots.\n",
            file->direntry, file->direntries);

    return 0;
}

static unsigned char char2dos(unsigned char c, int* randomize)
{
    static const char invalid_chars[] = "\"*+,./:;<=>?[\\]|";

    if (c <= 0x20)
        c = 0;   /* Illegal char, remove */
    else if (strchr(invalid_chars, c) != NULL)
    {
        /* Illegal char, replace */
        c = '_';
        *randomize = 1; /* as per FAT spec */
    }
    else
        c = toupper(c);

    return c;
}

static void create_dos_name(const unsigned char *name, unsigned char *newname)
{
    int i;
    unsigned char *ext;
    int randomize = 0;

    /* Find extension part */
    ext = strrchr(name, '.');
    if (ext == name)         /* handle .dotnames */
        ext = NULL;

    /* needs to randomize? */
    if((ext && (strlen(ext) > 4)) ||
       ((ext ? (unsigned int)(ext-name) : strlen(name)) > 8) )
        randomize = 1;

    /* Name part */
    for (i = 0; *name && (!ext || name < ext) && (i < 8); name++)
    {
        unsigned char c = char2dos(*name, &randomize);
        if (c)
            newname[i++] = c;
    }

    /* Pad both name and extension */
    while (i < 11)
        newname[i++] = ' ';

    if (newname[0] == 0xe5) /* Special kanji character */
        newname[0] = 0x05;

    if (ext)
    {   /* Extension part */
        ext++;
        for (i = 8; *ext && (i < 11); ext++)
        {
            unsigned char c = char2dos(*ext, &randomize);
            if (c)
                newname[i++] = c;
        }
    }

    if(randomize)
        randomize_dos_name(newname);
}

static void randomize_dos_name(unsigned char *name)
{
    unsigned char* tilde = NULL;    /* ~ location */
    unsigned char* lastpt = NULL;   /* last point of filename */
    unsigned char* nameptr = name;  /* working copy of name pointer */
    unsigned char num[9];           /* holds number as string */
    int i = 0;
    int cnt = 1;
    int numlen;
    int offset;

    while(i++ < 8)
    {
        /* hunt for ~ and where to put it */
        if((!tilde) && (*nameptr == '~'))
            tilde = nameptr;
        if((!lastpt) && ((*nameptr == ' ' || *nameptr == '~')))
            lastpt = nameptr;
        nameptr++;
    }
    if(tilde)
    {
        /* extract current count and increment */
        memcpy(num,tilde+1,7-(unsigned int)(tilde-name));
        num[7-(unsigned int)(tilde-name)] = 0;
        cnt = atoi(num) + 1;
    }
    cnt %= 10000000; /* protection */
    snprintf(num, 9, "~%d", cnt);   /* allow room for trailing zero */
    numlen = strlen(num);           /* required space */
    offset = (unsigned int)(lastpt ? lastpt - name : 8); /* prev startpoint */
    if(offset > (8-numlen)) offset = 8-numlen;  /* correct for new numlen */

    memcpy(&name[offset], num, numlen);

    /* in special case of counter overflow: pad with spaces */
    for(offset = offset+numlen; offset < 8; offset++)
        name[offset] = ' ';
}

static int update_short_entry( struct fat_file* file, long size, int attr )
{
    unsigned char buf[SECTOR_SIZE];
    int sector = file->direntry / DIR_ENTRIES_PER_SECTOR;
    unsigned char* entry =
        buf + DIR_ENTRY_SIZE * (file->direntry % DIR_ENTRIES_PER_SECTOR);
    unsigned long* sizeptr;
    unsigned short* clusptr;
    struct fat_file dir;
    int rc;

    LDEBUGF("update_file_size(cluster:%lx entry:%d size:%ld)\n",
            file->firstcluster, file->direntry, size);

    /* create a temporary file handle for the dir holding this file */
    rc = fat_open(IF_MV2(file->volume,) file->dircluster, &dir, NULL);
    if (rc < 0)
        return rc * 10 - 1;

    rc = fat_seek( &dir, sector );
    if (rc<0)
        return rc * 10 - 2;

    rc = fat_readwrite(&dir, 1, buf, false);
    if (rc < 1)
        return rc * 10 - 3;

    if (!entry[0] || entry[0] == 0xe5)
        panicf("Updating size on empty dir entry %d\n", file->direntry);

    entry[FATDIR_ATTR] = attr & 0xFF;

    clusptr = (short*)(entry + FATDIR_FSTCLUSHI);
    *clusptr = htole16(file->firstcluster >> 16);

    clusptr = (short*)(entry + FATDIR_FSTCLUSLO);
    *clusptr = htole16(file->firstcluster & 0xffff);

    sizeptr = (long*)(entry + FATDIR_FILESIZE);
    *sizeptr = htole32(size);

    {
#if CONFIG_RTC
        unsigned short time = 0;
        unsigned short date = 0;
#else
        /* get old time to increment from */
        unsigned short time = htole16(*(unsigned short*)(entry+FATDIR_WRTTIME));
        unsigned short date = htole16(*(unsigned short*)(entry+FATDIR_WRTDATE));
#endif
        fat_time(&date, &time, NULL);
        *(unsigned short*)(entry + FATDIR_WRTTIME) = htole16(time);
        *(unsigned short*)(entry + FATDIR_WRTDATE) = htole16(date);
        *(unsigned short*)(entry + FATDIR_LSTACCDATE) = htole16(date);
    }

    rc = fat_seek( &dir, sector );
    if (rc < 0)
        return rc * 10 - 4;

    rc = fat_readwrite(&dir, 1, buf, true);
    if (rc < 1)
        return rc * 10 - 5;

    return 0;
}

static int parse_direntry(struct fat_direntry *de, const unsigned char *buf)
{
    int i=0,j=0;
    unsigned char c;
    bool lowercase;

    memset(de, 0, sizeof(struct fat_direntry));
    de->attr = buf[FATDIR_ATTR];
    de->crttimetenth = buf[FATDIR_CRTTIMETENTH];
    de->crtdate = BYTES2INT16(buf,FATDIR_CRTDATE);
    de->crttime = BYTES2INT16(buf,FATDIR_CRTTIME);
    de->wrtdate = BYTES2INT16(buf,FATDIR_WRTDATE);
    de->wrttime = BYTES2INT16(buf,FATDIR_WRTTIME);
    de->filesize = BYTES2INT32(buf,FATDIR_FILESIZE);
    de->firstcluster = ((long)(unsigned)BYTES2INT16(buf,FATDIR_FSTCLUSLO)) |
        ((long)(unsigned)BYTES2INT16(buf,FATDIR_FSTCLUSHI) << 16);
    /* The double cast is to prevent a sign-extension to be done on CalmRISC16.
       (the result of the shift is always considered signed) */

    /* fix the name */
    lowercase = (buf[FATDIR_NTRES] & FAT_NTRES_LC_NAME);
    c = buf[FATDIR_NAME];
    if (c == 0x05)  /* special kanji char */
        c = 0xe5;
    i = 0;
    while (c != ' ') {
        de->name[j++] = lowercase ? tolower(c) : c;
        if (++i >= 8)
            break;
        c = buf[FATDIR_NAME+i];
    }
    if (buf[FATDIR_NAME+8] != ' ') {
        lowercase = (buf[FATDIR_NTRES] & FAT_NTRES_LC_EXT);
        de->name[j++] = '.';
        for (i = 8; (i < 11) && ((c = buf[FATDIR_NAME+i]) != ' '); i++)
            de->name[j++] = lowercase ? tolower(c) : c;
    }
    return 1;
}

int fat_open(IF_MV2(int volume,)
             long startcluster,
             struct fat_file *file,
             const struct fat_dir* dir)
{
    file->firstcluster = startcluster;
    file->lastcluster = startcluster;
    file->lastsector = 0;
    file->clusternum = 0;
    file->sectornum = 0;
    file->eof = false;
#ifdef HAVE_MULTIVOLUME
    file->volume = volume;
    /* fixme: remove error check when done */
    if (volume >= NUM_VOLUMES || !fat_bpbs[volume].mounted)
    {
        LDEBUGF("fat_open() illegal volume %d\n", volume);
        return -1;
    }
#endif

    /* remember where the file's dir entry is located */
    if ( dir ) {
        file->direntry = dir->entry - 1;
        file->direntries = dir->entrycount;
        file->dircluster = dir->file.firstcluster;
    }
    LDEBUGF("fat_open(%lx), entry %d\n",startcluster,file->direntry);
    return 0;
}

int fat_create_file(const char* name,
                    struct fat_file* file,
                    struct fat_dir* dir)
{
    int rc;

    LDEBUGF("fat_create_file(\"%s\",%lx,%lx)\n",name,(long)file,(long)dir);
    rc = add_dir_entry(dir, file, name, false, false);
    if (!rc) {
        file->firstcluster = 0;
        file->lastcluster = 0;
        file->lastsector = 0;
        file->clusternum = 0;
        file->sectornum = 0;
        file->eof = false;
    }

    return rc;
}

int fat_create_dir(const char* name,
                   struct fat_dir* newdir,
                   struct fat_dir* dir)
{
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[dir->file.volume];
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    unsigned char buf[SECTOR_SIZE];
    int i;
    long sector;
    int rc;
    struct fat_file dummyfile;

    LDEBUGF("fat_create_dir(\"%s\",%lx,%lx)\n",name,(long)newdir,(long)dir);

    memset(newdir, 0, sizeof(struct fat_dir));
    memset(&dummyfile, 0, sizeof(struct fat_file));

    /* First, add the entry in the parent directory */
    rc = add_dir_entry(dir, &newdir->file, name, true, false);
    if (rc < 0)
        return rc * 10 - 1;

    /* Allocate a new cluster for the directory */
    newdir->file.firstcluster = find_free_cluster(IF_MV2(fat_bpb,)
                                                  fat_bpb->fsinfo.nextfree);
    if(newdir->file.firstcluster == 0)
        return -1;

    update_fat_entry(IF_MV2(fat_bpb,) newdir->file.firstcluster, FAT_EOF_MARK);

    /* Clear the entire cluster */
    memset(buf, 0, sizeof buf);
    sector = cluster2sec(IF_MV2(fat_bpb,) newdir->file.firstcluster);
    for(i = 0;i < (int)fat_bpb->bpb_secperclus;i++) {
        rc = transfer(IF_MV2(fat_bpb,) sector + i, 1, buf, true );
        if (rc < 0)
            return rc * 10 - 2;
    }

    /* Then add the "." entry */
    rc = add_dir_entry(newdir, &dummyfile, ".", true, true);
    if (rc < 0)
        return rc * 10 - 3;
    dummyfile.firstcluster = newdir->file.firstcluster;
    update_short_entry(&dummyfile, 0, FAT_ATTR_DIRECTORY);

    /* and the ".." entry */
    rc = add_dir_entry(newdir, &dummyfile, "..", true, true);
    if (rc < 0)
        return rc * 10 - 4;

    /* The root cluster is cluster 0 in the ".." entry */
    if(dir->file.firstcluster == fat_bpb->bpb_rootclus)
        dummyfile.firstcluster = 0;
    else
        dummyfile.firstcluster = dir->file.firstcluster;
    update_short_entry(&dummyfile, 0, FAT_ATTR_DIRECTORY);

    /* Set the firstcluster field in the direntry */
    update_short_entry(&newdir->file, 0, FAT_ATTR_DIRECTORY);

    rc = flush_fat(IF_MV(fat_bpb));
    if (rc < 0)
        return rc * 10 - 5;

    return rc;
}

int fat_truncate(const struct fat_file *file)
{
    /* truncate trailing clusters */
    long next;
    long last = file->lastcluster;
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];
#endif

    LDEBUGF("fat_truncate(%lx, %lx)\n", file->firstcluster, last);

    for ( last = get_next_cluster(IF_MV2(fat_bpb,) last); last; last = next ) {
        next = get_next_cluster(IF_MV2(fat_bpb,) last);
        update_fat_entry(IF_MV2(fat_bpb,) last,0);
    }
    if (file->lastcluster)
        update_fat_entry(IF_MV2(fat_bpb,) file->lastcluster,FAT_EOF_MARK);

    return 0;
}

int fat_closewrite(struct fat_file *file, long size, int attr)
{
    int rc;
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];
#endif
    LDEBUGF("fat_closewrite(size=%ld)\n",size);

    if (!size) {
        /* empty file */
        if ( file->firstcluster ) {
            update_fat_entry(IF_MV2(fat_bpb,) file->firstcluster, 0);
            file->firstcluster = 0;
        }
    }

    if (file->dircluster) {
        rc = update_short_entry(file, size, attr);
        if (rc < 0)
            return rc * 10 - 1;
    }

    flush_fat(IF_MV(fat_bpb));

#ifdef TEST_FAT
    if ( file->firstcluster ) {
        /* debug */
#ifdef HAVE_MULTIVOLUME
        struct bpb* fat_bpb = &fat_bpbs[file->volume];
#else
        struct bpb* fat_bpb = &fat_bpbs[0];
#endif
        long count = 0;
        long len;
        long next;
        for ( next = file->firstcluster; next;
              next = get_next_cluster(IF_MV2(fat_bpb,) next) ) {
            LDEBUGF("cluster %ld: %lx\n", count, next);
            count++;
        }
        len = count * fat_bpb->bpb_secperclus * SECTOR_SIZE;
        LDEBUGF("File is %ld clusters (chainlen=%ld, size=%ld)\n",
                count, len, size );
        if ( len > size + fat_bpb->bpb_secperclus * SECTOR_SIZE)
            panicf("Cluster chain is too long\n");
        if ( len < size )
            panicf("Cluster chain is too short\n");
    }
#endif

    return 0;
}

static int free_direntries(struct fat_file* file)
{
    unsigned char buf[SECTOR_SIZE];
    struct fat_file dir;
    int numentries = file->direntries;
    unsigned int entry = file->direntry - numentries + 1;
    unsigned int sector = entry / DIR_ENTRIES_PER_SECTOR;
    int i;
    int rc;

    /* create a temporary file handle for the dir holding this file */
    rc = fat_open(IF_MV2(file->volume,) file->dircluster, &dir, NULL);
    if (rc < 0)
        return rc * 10 - 1;

    rc = fat_seek( &dir, sector );
    if (rc < 0)
        return rc * 10 - 2;

    rc = fat_readwrite(&dir, 1, buf, false);
    if (rc < 1)
        return rc * 10 - 3;

    for (i=0; i < numentries; i++) {
        LDEBUGF("Clearing dir entry %d (%d/%d)\n",
                entry, i+1, numentries);
        buf[(entry % DIR_ENTRIES_PER_SECTOR) * DIR_ENTRY_SIZE] = 0xe5;
        entry++;

        if ( (entry % DIR_ENTRIES_PER_SECTOR) == 0 ) {
            /* flush this sector */
            rc = fat_seek(&dir, sector);
            if (rc < 0)
                return rc * 10 - 4;

            rc = fat_readwrite(&dir, 1, buf, true);
            if (rc < 1)
                return rc * 10 - 5;

            if ( i+1 < numentries ) {
                /* read next sector */
                rc = fat_readwrite(&dir, 1, buf, false);
                if (rc < 1)
                    return rc * 10 - 6;
            }
            sector++;
        }
    }

    if ( entry % DIR_ENTRIES_PER_SECTOR ) {
        /* flush this sector */
        rc = fat_seek(&dir, sector);
        if (rc < 0)
            return rc * 10 - 7;

        rc = fat_readwrite(&dir, 1, buf, true);
        if (rc < 1)
            return rc * 10 - 8;
    }

    return 0;
}

int fat_remove(struct fat_file* file)
{
    long next, last = file->firstcluster;
    int rc;
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];
#endif

    LDEBUGF("fat_remove(%lx)\n",last);

    while ( last ) {
        next = get_next_cluster(IF_MV2(fat_bpb,) last);
        update_fat_entry(IF_MV2(fat_bpb,) last,0);
        last = next;
    }

    if ( file->dircluster ) {
        rc = free_direntries(file);
        if (rc < 0)
            return rc * 10 - 1;
    }

    file->firstcluster = 0;
    file->dircluster = 0;

    rc = flush_fat(IF_MV(fat_bpb));
    if (rc < 0)
        return rc * 10 - 2;

    return 0;
}

int fat_rename(struct fat_file* file, 
                struct fat_dir* dir, 
                const unsigned char* newname,
                long size,
                int attr)
{
    int rc;
    struct fat_dir olddir;
    struct fat_file newfile = *file;
    unsigned char buf[SECTOR_SIZE];
    unsigned char* entry = NULL;
    unsigned short* clusptr = NULL;
    unsigned int parentcluster;
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];

    if (file->volume != dir->file.volume) {
        DEBUGF("No rename across volumes!\n");
        return -1;
    }
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif

    if ( !file->dircluster ) {
        DEBUGF("File has no dir cluster!\n");
        return -2;
    }

    /* create a temporary file handle */
    rc = fat_opendir(IF_MV2(file->volume,) &olddir, file->dircluster, NULL);
    if (rc < 0)
        return rc * 10 - 1;

    /* create new name */
    rc = add_dir_entry(dir, &newfile, newname, false, false);
    if (rc < 0)
        return rc * 10 - 2;

    /* write size and cluster link */
    rc = update_short_entry(&newfile, size, attr);
    if (rc < 0)
        return rc * 10 - 3;

    /* remove old name */
    rc = free_direntries(file);
    if (rc < 0)
        return rc * 10 - 4;

    rc = flush_fat(IF_MV(fat_bpb));
    if (rc < 0)
        return rc * 10 - 5;

    /* if renaming a directory, update the .. entry to make sure
       it points to its parent directory (we don't check if it was a move) */
    if(FAT_ATTR_DIRECTORY == attr) {
        /* open the dir that was renamed, we re-use the olddir struct */
        rc = fat_opendir(IF_MV2(file->volume,) &olddir, newfile.firstcluster,
                                                                          NULL);
        if (rc < 0)
            return rc * 10 - 6;

        /* get the first sector of the dir */
        rc = fat_seek(&olddir.file, 0);
        if (rc < 0)
            return rc * 10 - 7;

        rc = fat_readwrite(&olddir.file, 1, buf, false);
        if (rc < 0)
            return rc * 10 - 8;

        /* parent cluster is 0 if parent dir is the root - FAT spec (p.29) */
        if(dir->file.firstcluster == fat_bpb->bpb_rootclus)
            parentcluster = 0;
        else
            parentcluster = dir->file.firstcluster;

        entry = buf + DIR_ENTRY_SIZE;
        if(strncmp("..         ", entry, 11))
        {
            /* .. entry must be second entry according to FAT spec (p.29) */
            DEBUGF("Second dir entry is not double-dot!\n");
            return rc * 10 - 9;
        }
        clusptr = (short*)(entry + FATDIR_FSTCLUSHI);
        *clusptr = htole16(parentcluster >> 16);

        clusptr = (short*)(entry + FATDIR_FSTCLUSLO);
        *clusptr = htole16(parentcluster & 0xffff);

        /* write back this sector */
        rc = fat_seek(&olddir.file, 0);
        if (rc < 0)
            return rc * 10 - 7;

        rc = fat_readwrite(&olddir.file, 1, buf, true);
        if (rc < 1)
            return rc * 10 - 8;
    }

    return 0;
}

static long next_write_cluster(struct fat_file* file,
                              long oldcluster,
                              long* newsector)
{
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    long cluster = 0;
    long sector;

    LDEBUGF("next_write_cluster(%lx,%lx)\n",file->firstcluster, oldcluster);

    if (oldcluster)
        cluster = get_next_cluster(IF_MV2(fat_bpb,) oldcluster);

    if (!cluster) {
        if (oldcluster > 0)
            cluster = find_free_cluster(IF_MV2(fat_bpb,) oldcluster+1);
        else if (oldcluster == 0)
            cluster = find_free_cluster(IF_MV2(fat_bpb,)
                                        fat_bpb->fsinfo.nextfree);
#ifdef HAVE_FAT16SUPPORT
        else /* negative, pseudo-cluster of the root dir */
            return 0; /* impossible to append something to the root */
#endif

        if (cluster) {
            if (oldcluster)
                update_fat_entry(IF_MV2(fat_bpb,) oldcluster, cluster);
            else
                file->firstcluster = cluster;
            update_fat_entry(IF_MV2(fat_bpb,) cluster, FAT_EOF_MARK);
        }
        else {
#ifdef TEST_FAT
            if (fat_bpb->fsinfo.freecount>0)
                panicf("There is free space, but find_free_cluster() "
                       "didn't find it!\n");
#endif
            DEBUGF("next_write_cluster(): Disk full!\n");
            return 0;
        }
    }
    sector = cluster2sec(IF_MV2(fat_bpb,) cluster);
    if (sector<0)
        return 0;

    *newsector = sector;
    return cluster;
}

static int transfer(IF_MV2(struct bpb* fat_bpb,) 
                    unsigned long start, long count, char* buf, bool write )
{
#ifndef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    int rc;

    LDEBUGF("transfer(s=%lx, c=%lx, %s)\n",
        start+ fat_bpb->startsector, count, write?"write":"read");
    if (write) {
        unsigned long firstallowed;
#ifdef HAVE_FAT16SUPPORT
        if (fat_bpb->is_fat16)
            firstallowed = fat_bpb->rootdirsector;
        else
#endif
            firstallowed = fat_bpb->firstdatasector;

        if (start < firstallowed)
            panicf("Write %ld before data\n", firstallowed - start);
        if (start + count > fat_bpb->totalsectors)
            panicf("Write %ld after data\n",
                start + count - fat_bpb->totalsectors);
        rc = ata_write_sectors(IF_MV2(fat_bpb->drive,)
                               start + fat_bpb->startsector, count, buf);
    }
    else
        rc = ata_read_sectors(IF_MV2(fat_bpb->drive,)
                              start + fat_bpb->startsector, count, buf);
    if (rc < 0) {
        DEBUGF( "transfer() - Couldn't %s sector %lx"
                " (error code %d)\n", 
                write ? "write":"read", start, rc);
        return rc;
    }
    return 0;
}


long fat_readwrite( struct fat_file *file, long sectorcount,
                   void* buf, bool write )
{
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    long cluster = file->lastcluster;
    long sector = file->lastsector;
    long clusternum = file->clusternum;
    long numsec = file->sectornum;
    bool eof = file->eof;
    long first=0, last=0;
    long i;
    int rc;

    LDEBUGF( "fat_readwrite(file:%lx,count:0x%lx,buf:%lx,%s)\n",
             file->firstcluster,sectorcount,(long)buf,write?"write":"read");
    LDEBUGF( "fat_readwrite: sec=%lx numsec=%ld eof=%d\n",
             sector,numsec, eof?1:0);

    if ( eof && !write)
        return 0;

    /* find sequential sectors and write them all at once */
    for (i=0; (i < sectorcount) && (sector > -1); i++ ) {
        numsec++;
        if ( numsec > (long)fat_bpb->bpb_secperclus || !cluster ) {
            long oldcluster = cluster;
            long oldsector = sector;
            long oldnumsec = numsec;
            if (write)
                cluster = next_write_cluster(file, cluster, &sector);
            else {
                cluster = get_next_cluster(IF_MV2(fat_bpb,) cluster);
                sector = cluster2sec(IF_MV2(fat_bpb,) cluster);
            }

            clusternum++;
            numsec=1;

            if (!cluster) {
                eof = true;
                if ( write ) {
                    /* remember last cluster, in case
                       we want to append to the file */
                    sector = oldsector;
                    cluster = oldcluster;
                    numsec = oldnumsec;
                    clusternum--;
                    i = -1; /* Error code */
                    break;
                }
            }
            else
                eof = false;
        }
        else {
            if (sector)
                sector++;
            else {
                /* look up first sector of file */
                sector = cluster2sec(IF_MV2(fat_bpb,) file->firstcluster);
                numsec=1;
#ifdef HAVE_FAT16SUPPORT
                if (file->firstcluster < 0)
                {   /* FAT16 root dir */
                    sector += fat_bpb->rootdiroffset;
                    numsec += fat_bpb->rootdiroffset;
                }
#endif
            }
        }

        if (!first)
            first = sector;

        if ( ((sector != first) && (sector != last+1)) || /* not sequential */
             (last-first+1 == 256) ) { /* max 256 sectors per ata request */
            long count = last - first + 1;
            rc = transfer(IF_MV2(fat_bpb,) first, count, buf, write );
            if (rc < 0)
                return rc * 10 - 1;

            buf = (char *)buf + count * SECTOR_SIZE;
            first = sector;
        }

        if ((i == sectorcount-1) && /* last sector requested */
            (!eof))
        {
            long count = sector - first + 1;
            rc = transfer(IF_MV2(fat_bpb,) first, count, buf, write );
            if (rc < 0)
                return rc * 10 - 2;
        }

        last = sector;
    }

    file->lastcluster = cluster;
    file->lastsector = sector;
    file->clusternum = clusternum;
    file->sectornum = numsec;
    file->eof = eof;

    /* if eof, don't report last block as read/written */
    if (eof)
        i--;

    DEBUGF("Sectors written: %ld\n", i);
    return i;
}

int fat_seek(struct fat_file *file, unsigned long seeksector )
{
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[file->volume];
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    long clusternum=0, numclusters=0, sectornum=0, sector=0;
    long cluster = file->firstcluster;
    long i;

#ifdef HAVE_FAT16SUPPORT
    if (cluster < 0) /* FAT16 root dir */
        seeksector += fat_bpb->rootdiroffset;
#endif

    file->eof = false;
    if (seeksector) {
        /* we need to find the sector BEFORE the requested, since
           the file struct stores the last accessed sector */
        seeksector--;
        numclusters = clusternum = seeksector / fat_bpb->bpb_secperclus;
        sectornum = seeksector % fat_bpb->bpb_secperclus;

        if (file->clusternum && clusternum >= file->clusternum)
        {
            cluster = file->lastcluster;
            numclusters -= file->clusternum;
        }

        for (i=0; i<numclusters; i++) {
            cluster = get_next_cluster(IF_MV2(fat_bpb,) cluster);
            if (!cluster) {
                DEBUGF("Seeking beyond the end of the file! "
                       "(sector %ld, cluster %ld)\n", seeksector, i);
                return -1;
            }
        }

        sector = cluster2sec(IF_MV2(fat_bpb,) cluster) + sectornum;
    }
    else {
        sectornum = -1;
    }

    LDEBUGF("fat_seek(%lx, %lx) == %lx, %lx, %lx\n",
            file->firstcluster, seeksector, cluster, sector, sectornum);

    file->lastcluster = cluster;
    file->lastsector = sector;
    file->clusternum = clusternum;
    file->sectornum = sectornum + 1;
    return 0;
}

int fat_opendir(IF_MV2(int volume,) 
                struct fat_dir *dir, unsigned long startcluster,
                const struct fat_dir *parent_dir)
{
#ifdef HAVE_MULTIVOLUME
    struct bpb* fat_bpb = &fat_bpbs[volume];
    /* fixme: remove error check when done */
    if (volume >= NUM_VOLUMES || !fat_bpbs[volume].mounted)
    {
        LDEBUGF("fat_open() illegal volume %d\n", volume);
        return -1;
    }
#else
    struct bpb* fat_bpb = &fat_bpbs[0];
#endif
    int rc;

    dir->entry = 0;
    dir->sector = 0;

    if (startcluster == 0)
        startcluster = fat_bpb->bpb_rootclus;

    rc = fat_open(IF_MV2(volume,) startcluster, &dir->file, parent_dir);
    if(rc)
    {
        DEBUGF( "fat_opendir() - Couldn't open dir"
                " (error code %d)\n", rc);
        return rc * 10 - 1;
    }

    return 0;
}

/* Copies a segment of long file name (UTF-16 LE encoded) to the
 * destination buffer (UTF-8 encoded). Copying is stopped when
 * either 0x0000 or 0xffff (FAT pad char) is encountered.
 * Trailing \0 is also appended at the end of the UTF8-encoded
 * string.
 *
 * utf16src   utf16 (little endian) segment to copy
 * utf16count max number of the utf16-characters to copy
 * utf8dst    where to write UTF8-encoded string to
 *
 * returns the number of UTF-16 characters actually copied
 */
static int fat_copy_long_name_segment(unsigned char *utf16src,
                int utf16count, unsigned char *utf8dst) {
    int cnt = 0;
    while ((utf16count--) > 0) {
        unsigned short ucs = utf16src[0] | (utf16src[1] << 8);
        if ((ucs == 0) || (ucs == FAT_LONGNAME_PAD_UCS)) {
            break;
        }
        utf8dst = utf8encode(ucs, utf8dst);
        utf16src += 2;
        cnt++;
    }
    *utf8dst = 0;
    return cnt;
}

int fat_getnext(struct fat_dir *dir, struct fat_direntry *entry)
{
    bool done = false;
    int i;
    int rc;
    unsigned char firstbyte;
    /* Long file names are stored in special entries. Each entry holds
       up to 13 characters. Names can be max 255 chars (not bytes!) long
       hence max 20 entries are required. */
    int longarray[20];
    int longs=0;
    int sectoridx=0;
    unsigned char* cached_buf = dir->sectorcache[0];

    dir->entrycount = 0;

    while(!done)
    {
        if ( !(dir->entry % DIR_ENTRIES_PER_SECTOR) || !dir->sector )
        {
            rc = fat_readwrite(&dir->file, 1, cached_buf, false);
            if (rc == 0) {
                /* eof */
                entry->name[0] = 0;
                break;
            }
            if (rc < 0) {
                DEBUGF( "fat_getnext() - Couldn't read dir"
                        " (error code %d)\n", rc);
                return rc * 10 - 1;
            }
            dir->sector = dir->file.lastsector;
        }

        for (i = dir->entry % DIR_ENTRIES_PER_SECTOR;
             i < DIR_ENTRIES_PER_SECTOR; i++)
        {
            unsigned int entrypos = i * DIR_ENTRY_SIZE;

            firstbyte = cached_buf[entrypos];
            dir->entry++;

            if (firstbyte == 0xe5) {
                /* free entry */
                sectoridx = 0;
                dir->entrycount = 0;
                continue;
            }

            if (firstbyte == 0) {
                /* last entry */
                entry->name[0] = 0;
                dir->entrycount = 0;
                return 0;
            }

            dir->entrycount++;

            /* longname entry? */
            if ( ( cached_buf[entrypos + FATDIR_ATTR] &
                   FAT_ATTR_LONG_NAME_MASK ) == FAT_ATTR_LONG_NAME ) {
                longarray[longs++] = entrypos + sectoridx;
            }
            else {
                if ( parse_direntry(entry,
                                    &cached_buf[entrypos]) ) {

                    /* don't return volume id entry */
                    if ( (entry->attr &
                          (FAT_ATTR_VOLUME_ID|FAT_ATTR_DIRECTORY))
                         == FAT_ATTR_VOLUME_ID)
                        continue;

                    /* replace shortname with longname? */
                    if ( longs ) {
                        int j;
                        /* This should be enough to hold any name segment
                           utf8-encoded */
                        unsigned char shortname[13]; /* 8+3+dot+\0 */
                        /* Add 1 for trailing \0 */
                        unsigned char longname_utf8segm[6*4 + 1];
                        int longname_utf8len = 0;
                        /* Temporarily store it */
                        strcpy(shortname, entry->name);
                        entry->name[0] = 0;

                        /* iterate backwards through the dir entries */
                        for (j=longs-1; j>=0; j--) {
                            unsigned char* ptr = cached_buf;
                            int index = longarray[j];
                            /* current or cached sector? */
                            if ( sectoridx >= SECTOR_SIZE ) {
                                if ( sectoridx >= SECTOR_SIZE*2 ) {
                                    if ( ( index >= SECTOR_SIZE ) &&
                                         ( index < SECTOR_SIZE*2 ))
                                        ptr = dir->sectorcache[1];
                                    else
                                        ptr = dir->sectorcache[2];
                                }
                                else {
                                    if ( index < SECTOR_SIZE )
                                        ptr = dir->sectorcache[1];
                                }

                                index &= SECTOR_SIZE-1;
                            }

                            /* Try to append each segment of the long name.
                               Check if we'd exceed the buffer.
                               Also check for FAT padding characters 0xFFFF. */
                            if (fat_copy_long_name_segment(ptr + index + 1, 5,
                                    longname_utf8segm) == 0) break;
                            /* logf("SG: %s, EN: %s", longname_utf8segm,
                                    entry->name); */
                            longname_utf8len += strlen(longname_utf8segm);
                            if (longname_utf8len < FAT_FILENAME_BYTES)
                                strcat(entry->name, longname_utf8segm);
                            else
                                break;

                            if (fat_copy_long_name_segment(ptr + index + 14, 6,
                                    longname_utf8segm) == 0) break;
                            /* logf("SG: %s, EN: %s", longname_utf8segm,
                                    entry->name); */
                            longname_utf8len += strlen(longname_utf8segm);
                            if (longname_utf8len < FAT_FILENAME_BYTES)
                                strcat(entry->name, longname_utf8segm);
                            else
                                break;

                            if (fat_copy_long_name_segment(ptr + index + 28, 2,
                                    longname_utf8segm) == 0) break;
                            /* logf("SG: %s, EN: %s", longname_utf8segm,
                                    entry->name); */
                            longname_utf8len += strlen(longname_utf8segm);
                            if (longname_utf8len < FAT_FILENAME_BYTES)
                                strcat(entry->name, longname_utf8segm);
                            else
                                break;
                        }

                        /* Does the utf8-encoded name fit into the entry? */
                        if (longname_utf8len >= FAT_FILENAME_BYTES) {
                            /* Take the short DOS name. Need to utf8-encode it
                               since it may contain chars from the upper half of
                               the OEM code page which wouldn't be a valid utf8.
                               Beware: this file will be shown with strange
                               glyphs in file browser since unicode 0x80 to 0x9F
                               are control characters. */
                            logf("SN-DOS: %s", shortname);
                            unsigned char *utf8;
                            utf8 = iso_decode(shortname, entry->name, -1,
                                              strlen(shortname));
                        *utf8 = 0;
                            logf("SN: %s", entry->name);
                        } else {
                            /* logf("LN: %s", entry->name);
                               logf("LNLen: %d (%c)", longname_utf8len,
                                    entry->name[0]); */
                        }
                    }
                    done = true;
                    sectoridx = 0;
                    i++;
                    break;
                }
            }
        }

        /* save this sector, for longname use */
        if ( sectoridx )
            memcpy( dir->sectorcache[2], dir->sectorcache[0], SECTOR_SIZE );
        else
            memcpy( dir->sectorcache[1], dir->sectorcache[0], SECTOR_SIZE );
        sectoridx += SECTOR_SIZE;

    }
    return 0;
}

unsigned int fat_get_cluster_size(IF_MV_NONVOID(int volume))
{
#ifndef HAVE_MULTIVOLUME
    const int volume = 0;
#endif
    struct bpb* fat_bpb = &fat_bpbs[volume];
    return fat_bpb->bpb_secperclus * SECTOR_SIZE;
}

#ifdef HAVE_MULTIVOLUME
bool fat_ismounted(int volume)
{
    return (volume<NUM_VOLUMES && fat_bpbs[volume].mounted);
}
#endif
