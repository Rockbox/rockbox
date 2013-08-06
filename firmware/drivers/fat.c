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
#include "config.h"
#include "system.h"
#include "sys/types.h"
#include "kernel.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "fat.h"
#include "storage.h"
#include "timefuncs.h"
#include "rbunicode.h"
#include "debug.h"
#include "panic.h"
/*#define LOGF_ENABLE*/
#include "logf.h"

#if 0
#define BYTES2INT32(array, pos) get32_le(&(array)[pos])
#define INT322BYTES(array, pos, val) put32_le(&(array)[pos], (val))
#define BYTES2INT16(array, pos) get16_le(&(array)[pos])
#define INT162BYTES(array, pos, val) put16_le(&(array)[pos], (val))
#endif

#define BYTES2INT32(array, pos) \
    ((long)array[pos] | ((long)array[pos+1] << 8 ) | \
     ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

#define INT322BYTES(array, pos, val) \
    ((array[pos] = (val)), (array[pos+1] = (val) >> 8), \
     (array[pos+2] = (val) >> 16), (array[pos+3] = (val) >> 24))

#define BYTES2INT16(array, pos) \
    ((long)array[pos] | ((long)array[pos+1] << 8))
#define INT162BYTES(array, pos, val) \
    ((array[pos] = (val)), (array[pos+1] = (val) >> 8))


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

/* Short and long name directory entry mask */
union fat_direntry_raw
{
    struct /* short entry */
    {
        uint8_t  name[8+3];         /*  0 */
        uint8_t  attr;              /* 11 */
        uint8_t  ntres;             /* 12 */
        uint8_t  crttimetenth;      /* 13 */
        uint16_t crttime;           /* 14 */
        uint16_t crtdate;           /* 16 */
        uint16_t lstaccdate;        /* 18 */
        uint16_t fstclushi;         /* 20 */
        uint16_t wrttime;           /* 22 */
        uint16_t wrtdate;           /* 24 */
        uint16_t fstcluslo;         /* 26 */
        uint32_t filesize;          /* 28 */
                                    /* 32 */
    };
    struct /* long entry */
    {
        uint8_t  ldir_ord;          /*  0 */
        uint8_t  ldir_name1[10];    /*  1 */
        uint8_t  ldir_attr;         /* 11 */
        uint8_t  ldir_type;         /* 12 */
        uint8_t  ldir_chksum;       /* 13 */
        uint8_t  ldir_name2[12];    /* 14 */
        uint16_t ldir_fstcluslo;    /* 26 */
        uint8_t  ldir_name3[4];     /* 28 */
                                    /* 32 */
    };
    struct /* raw byte array */
    {
        uint8_t  data[32];          /*  0 */
                                    /* 32 */
    };
};


/* at most 20 LFN entries */
#define FATLONG_MAX_ORDER           20
#define FATLONG_NAME_CHARS          13
#define FATLONG_ORD_F_LAST          0x40

/* attributes */
#define FAT_ATTR_LONG_NAME          (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                                     FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)
#define FAT_ATTR_LONG_NAME_MASK     (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                                     FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | \
                                     FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE )
/* NTRES flags */
#define FAT_NTRES_LC_NAME           0x08
#define FAT_NTRES_LC_EXT            0x10

#define CLUSTERS_PER_FAT_SECTOR     (SECTOR_SIZE / 4)
#define CLUSTERS_PER_FAT16_SECTOR   (SECTOR_SIZE / 2)
#define DIR_ENTRIES_PER_SECTOR      (SECTOR_SIZE / DIR_ENTRY_SIZE)
#define DIR_ENTRY_SIZE              32
#define FAT_BAD_MARK                0x0ffffff7
#define FAT_EOF_MARK                0x0ffffff8
#define FAT16_BAD_MARK              0xfff7
#define FAT16_EOF_MARK              0xfff8

struct fsinfo
{
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
    unsigned long bpb_bytspersec; /* Bytes per sector, typically 512 */
    unsigned long bpb_secperclus; /* Sectors per cluster */
    unsigned long bpb_rsvdseccnt; /* Number of reserved sectors */
    unsigned long bpb_totsec16;   /* Number of sectors on the volume (old 16-bit) */
    uint8_t       bpb_numfats;    /* Number of FAT structures, typically 2 */
    uint8_t       bpb_media;      /* Media type (typically 0xf0 or 0xf8) */
    uint16_t      bpb_fatsz16;    /* Number of used sectors per FAT structure */
    unsigned long bpb_totsec32;   /* Number of sectors on the volume
                                     (new 32-bit) */
    uint16_t      last_word;      /* 0xAA55 */
    long          bpb_rootclus;

    /**** FAT32 specific *****/
    unsigned long bpb_fatsz32;
    unsigned long bpb_fsinfo;

    /* variables for internal use */
    unsigned long fatsize;
    unsigned long totalsectors;
    unsigned long rootdirsector;
    unsigned long firstdatasector;
    unsigned long startsector;
    unsigned long dataclusters;
    unsigned long fatrgnstart;
    unsigned long fatrgnend;
    struct fsinfo fsinfo;
#ifdef HAVE_FAT16SUPPORT
    unsigned int bpb_rootentcnt;  /* Number of dir entries in the root */
    /* internals for FAT16 support */
    bool is_fat16; /* true if we mounted a FAT16 partition, false if FAT32 */
    unsigned long rootdirsectornum; /* sector offset of root dir relative to start
                                     * of first pseudo cluster */
#endif /* #ifdef HAVE_FAT16SUPPORT */

    /** Additional information kept for each volume **/

#if defined(HAVE_MULTIVOLUME) && defined(HAVE_MULTIDRIVE)
    int drive; /* on which physical device is this located */
#endif
    bool mounted; /* volume is mounted, false otherwise */

#ifdef HAVE_FAT16SUPPORT
    /* some functions are different for different FAT types */
    long (*get_next_cluster_ftype)(struct bpb *, long);
    long (*find_free_cluster_ftype)(struct bpb *,long);
    int (*update_fat_entry_ftype)(struct bpb *, unsigned long, unsigned long);
    void (*fat_recalc_free_internal_ftype)(struct bpb *);
#endif /* #ifdef HAVE_FAT16SUPPORT */

    /** Things that are NOT changed when mounting/unmounting **/
#if 0
    struct mrsw_monitor *mrsw; /* muti-reader, single-writer lock */
#endif
};

#ifdef HAVE_FAT16SUPPORT
#define get_next_cluster(bpb, cluster) \
    ((bpb)->get_next_cluster_ftype((bpb), (cluster)))
#define find_free_cluster(bpb, startcluster) \
    ((bpb)->find_free_cluster_ftype((bpb), (startcluster)))
#define update_fat_entry(bpb, entry, value) \
    ((bpb)->update_fat_entry_ftype((bpb), (entry), (value)))
#define fat_recalc_free_internal(bpb) \
    ((bpb)->fat_recalc_free_internal_ftype(bpb))
#else  /* #ifndef HAVE_FAT16SUPPORT */
#define get_next_cluster                get_next_cluster32
#define find_free_cluster               find_free_cluster32
#define update_fat_entry                update_fat_entry32
#define fat_recalc_free_internal        fat_recalc_free_internal32
#endif /* #ifdef HAVE_FAT16SUPPORT */
static int update_fsinfo32(struct bpb *fat_bpb);

static struct bpb fat_bpbs[NUM_VOLUMES]; /* mounted partition info */
#if 0
static struct mrsw_monitor fat_mrsw[NUM_VOLUMES] SHAREDBSS_ATTR;
#endif

/* APIs that alter directory entries must be synchronized */
static struct mutex dir_mutex SHAREDBSS_ATTR;

/* Cache: LRU cache with separately-chained hashtable
 *
 * Each entry of the map is the mapped location of the hashed sector value
 * where each bit in each map entry indicates which corresponding cache
 * entries are occupied by sector values that collide in that map entry.
 * To probe for a specific key, each bit in the map entry must be examined,
 * its position used as an index into the cache_entry array and the actual
 * sector information compared for that cache entry. If the search exhausts
 * all bits, the sector is not cached.
 *
 * To avoid long chains, the map entry count is much greater than the number
 * of cache entries.
 */

/* 2^n entries best for speed but may be anything > 0 */
#define FAT_CACHE_SIZE      32
#define CACHE_MAP_SIZE      (512 / (FAT_CACHE_SIZE/8))
#define HASH_SECTOR(secnum) ((unsigned long)(secnum) % CACHE_MAP_SIZE)

#if FAT_CACHE_SIZE > 32
  #error Need more cache map bits per entry!
#endif

enum FCE_FLAGS /* flags for cache_entry[].flags */
{
    FCE_INUSE = 0x1, /* entry in use and valid */
    FCE_DIRTY = 0x2, /* entry is dirty in need of writeback */
    FCE_BUF   = 0x4, /* entry is being used as a general buffer */
};

struct fat_cache_entry
{
    uint8_t next, prev;  /* LRU list */
    uint16_t flags;      /* entry flags */
    unsigned long secnum;/* cached sector number */
    void *sectorbuf;     /* buffer pointer */
    struct bpb *fat_bpb; /* volume BPB of sector (also, 4-word align) */
} cache_entry[FAT_CACHE_SIZE];

static unsigned int cache_lru; /* LRU cache entry */
static uint32_t cache_map[CACHE_MAP_SIZE] CACHEALIGN_ATTR;
static uint8_t cached_sector[FAT_CACHE_SIZE][SECTOR_SIZE] CACHEALIGN_ATTR;
static struct mutex cache_mutex SHAREDBSS_ATTR;

/* get the cache index from the buffer pointer (or address within buffer) */
#define FCIDX_FROM_BUF(buf) \
    ((uint8_t (*)[SECTOR_SIZE])(buf) - cached_sector)
/* return the cache entry from the buffer pointer (or address within buffer) */
#define FCE_FROM_BUF(buf) \
    (&cache_entry[FCIDX_FROM_BUF(buf)])
/* set error code and jump to routine exit */
#define FAT_ERR(_rc) \
    ({ rc = (_rc); goto fat_error; })

#define CHECK_VOL(volume) \
    ({ ((unsigned)IF_MV_VOL(volume) >= NUM_VOLUMES) ? \
       ({ LDEBUGF("%s() illegal volume %d\n", __func__, IF_MV_VOL(volume)); \
          false; }) : true; })

#define mrsw_init(x, y)
#define mrsw_read_acquire(x)
#define mrsw_read_release(x)
#define mrsw_write_acquire(x)
#define mrsw_write_release(x)

#define FAT_BPB_READ(volume) \
    ({  struct bpb * _bpb = CHECK_VOL(volume) ?       \
             &fat_bpbs[IF_MV_VOL(volume)] : NULL;     \
        if (_bpb)                                     \
        {                                             \
            mrsw_read_acquire(_bpb->mrsw);            \
            if (!_bpb->mounted)                       \
            {                                         \
                LDEBUGF("%s() not mounted %d\n",      \
                        __func__, IF_MV_VOL(volume)); \
                mrsw_read_release(_bpb->mrsw);        \
                _bpb = NULL;                          \
            }                                         \
        }                                             \
        _bpb;                                         \
    })

#define FAT_BPB_READ_DONE(bpb) \
    ({ mrsw_read_release((bpb)->mrsw); })

#define FAT_BPB_WRITE(volume) \
    ({  struct bpb * _bpb = CHECK_VOL(volume) ?       \
            &fat_bpbs[IF_MV_VOL(volume)] : NULL;      \
        if (_bpb)                                     \
        {                                             \
            mrsw_write_acquire(_bpb->mrsw);           \
        }                                             \
        _bpb; })

#define FAT_BPB_WRITE_DONE(bpb) \
    ({ mrsw_write_release((bpb)->mrsw); })


enum ADD_DIR_ENTRY_FLAGS
{
    DIRENTRY_DIR          = 0x1,
    DIRENTRY_KEEP_TIMES   = 0x2,
    DIRENTRY_RETURN_SHORT = 0x4,
};

#define IS_DOTDIR_NAME(name) \
    ((name)[0] == '.' && (!(name)[1] || ((name)[1] == '.' && !(name)[2])))

struct longname_parse
{
    unsigned int ord_max;
    unsigned int ord;
    unsigned int ord_last;
    unsigned int utf8namepos;
    uint8_t      chksums[1+FATLONG_MAX_ORDER];
};

static void rawent_set_fstclus(union fat_direntry_raw *ent, long fstclus)
{
    ent->fstclushi = htole16(fstclus >> 16);
    ent->fstcluslo = htole16(fstclus & 0xffff);
}

static void bpb_clear(struct bpb *fat_bpb)
{
#if 0
    memset(fat_bpb, 0, sizeof(struct bpb)-OFFSETOF(struct bpb, mrsw));
#else
    memset(fat_bpb, 0, sizeof(struct bpb));
#endif
}

static int bpb_is_sane(struct bpb *fat_bpb)
{
    if (fat_bpb->bpb_bytspersec % SECTOR_SIZE)
    {
        DEBUGF("bpb_is_sane() - Error: sector size is not sane (%lu)\n",
               fat_bpb->bpb_bytspersec);
        return -1;
    }

    if (fat_bpb->bpb_secperclus * fat_bpb->bpb_bytspersec > 128*1024ul)
    {
        DEBUGF("bpb_is_sane() - Error: cluster size is larger than 128K "
               "(%lu * %lu = %lu)\n",
               fat_bpb->bpb_bytspersec, fat_bpb->bpb_secperclus,
               fat_bpb->bpb_bytspersec * fat_bpb->bpb_secperclus);
        return -2;
    }

    if (fat_bpb->bpb_numfats != 2)
    {
        DEBUGF("bpb_is_sane() - Warning: NumFATS is not 2 (%u)\n",
               fat_bpb->bpb_numfats);
    }

    if (fat_bpb->bpb_media != 0xf0 && fat_bpb->bpb_media < 0xf8)
    {
        DEBUGF("bpb_is_sane() - Warning: Non-standard "
               "media type (0x%02x)\n",
               fat_bpb->bpb_media);
    }

    if (fat_bpb->last_word != 0xaa55)
    {
        DEBUGF("bpb_is_sane() - Error: Last word is not "
               "0xaa55 (0x%04x)\n", fat_bpb->last_word);
        return -3;
    }

    if (fat_bpb->fsinfo.freecount >
        (fat_bpb->totalsectors - fat_bpb->firstdatasector) /
        fat_bpb->bpb_secperclus)
    {
        DEBUGF("bpb_is_sane() - Error: FSInfo.Freecount > disk size "
                "(0x%04lx)\n", (unsigned long)fat_bpb->fsinfo.freecount);
        return -4;
    }

    return 0;
}

static inline void dir_lock(void)
{
    mutex_lock(&dir_mutex);
}

static inline void dir_unlock(void)
{
    mutex_unlock(&dir_mutex);
}

static inline void cache_lock(void)
{
    mutex_lock(&cache_mutex);
}

static inline void cache_unlock(void)
{
    mutex_unlock(&cache_mutex);
}

/* flush a cached sector to storage and mark it clean */
static void cache_commit_sector(struct fat_cache_entry *fce)
{
    struct bpb * const fat_bpb = fce->fat_bpb;
    unsigned long secnum = fce->secnum;
    unsigned int copies = secnum < fat_bpb->fatrgnend &&
                          secnum >= fat_bpb->fatrgnstart ?
                            fat_bpb->bpb_numfats : 1;

    secnum += fat_bpb->startsector;

    while (1)
    {
        int rc = storage_write_sectors(IF_MD(fat_bpb->drive,)
                                       secnum, 1, fce->sectorbuf);
        if (rc < 0)
        {
            panicf("cache_commit_sector() - Could not write sector %ld"
                   " (error %d)\n", secnum, rc);
        }

        if (--copies == 0)
            break;

        /* Update next FAT */
        secnum += fat_bpb->fatsize;
    }

    fce->flags &= ~FCE_DIRTY;
}

/* make entry MRU by moving it to the front */
static void cache_touch(unsigned int which)
{
    struct fat_cache_entry *lru = &cache_entry[cache_lru];

    if (which == cache_lru)
    {
        /* is the LRU, just rotate list */
        cache_lru = lru->prev;
        return;
    }

    /* move it */
    struct fat_cache_entry *fce = &cache_entry[which];

    cache_entry[fce->next].prev = fce->prev;
    cache_entry[fce->prev].next = fce->next;

    fce->prev = cache_lru;
    fce->next = lru->next;
    cache_entry[lru->next].prev = which;
    lru->next = which;
}

/* remove LRU entry from the cache list to use as buffer */
static bool cache_remove_lru(void)
{
    struct fat_cache_entry *lru = &cache_entry[cache_lru];

    if (lru->next == cache_lru)
        return false; /* must leave at least one for cache! */

    /* remove it */
    cache_lru = lru->prev;
    cache_entry[lru->next].prev = lru->prev;
    cache_entry[lru->prev].next = lru->next;
    return true;
}

/* return entry to the cache list and keep it LRU */
static void cache_return_lru(unsigned int which)
{
    struct fat_cache_entry *lru = &cache_entry[cache_lru];
    struct fat_cache_entry *fce = &cache_entry[which];

    fce->prev = cache_lru;
    fce->next = lru->next;
    cache_entry[lru->next].prev = which;
    lru->next = which;

    cache_lru = which;
}

/* caches a FAT or data area sector */
static void * cache_sector(struct bpb *fat_bpb, unsigned long secnum)
{
    unsigned int map_idx = HASH_SECTOR(secnum);
    uint32_t map = cache_map[map_idx];
    unsigned int index;
    struct fat_cache_entry *fce;

    /* if collision, probe map entry for this sector */
    while (map)
    {
        index = find_first_set_bit(map);
        fce = &cache_entry[index];

        if (fce->secnum == secnum IF_MV( && fce->fat_bpb == fat_bpb ))
            break; /* sector cached */

        map &= ~BIT_N(index);
    }

    if (!map)
    {
        /* sector not found so the LRU is the victim */
        index = cache_lru;
        fce = &cache_entry[index];
    }

    if (fce->prev != cache_lru)
        cache_touch(index);

    if (map)
        return fce->sectorbuf; /* yes, we have bananas */

    /* commit (if dirty) */
    if (fce->flags)
    {
        if (fce->flags & FCE_DIRTY)
            cache_commit_sector(fce);

        unsigned int old_map_idx = HASH_SECTOR(fce->secnum);
        cache_map[old_map_idx] &= ~BIT_N(index);
    }

    int rc = storage_read_sectors(IF_MD(fat_bpb->drive,)
                                  secnum + fat_bpb->startsector,
                                  1, fce->sectorbuf);
    if (rc < 0)
    {
        DEBUGF("cache_sector() - Could not read sector %ld"
               " (error %d)\n", secnum, rc);
        cache_map[map_idx] &= ~BIT_N(index);
        fce->flags = 0;
        return NULL;
    }

    cache_map[map_idx] |= BIT_N(index);
    fce->flags   = FCE_INUSE;
    fce->secnum  = secnum;
#ifdef HAVE_MULTIVOLUME
    fce->fat_bpb = fat_bpb;
#endif
    return fce->sectorbuf;
}

/* returns a raw buffer for any data area sector */
static void * cache_get_sector_buf(struct bpb *fat_bpb, unsigned long secnum)
{
    unsigned int map_idx = HASH_SECTOR(secnum);
    uint32_t map = 0;
    unsigned int index;
    struct fat_cache_entry *fce;

    if (fat_bpb)
    {
        map = cache_map[map_idx];

        /* if collision, probe map entry for this sector */
        while (map)
        {
            index = find_first_set_bit(map);
            fce = &cache_entry[index];

            if (fce->secnum == secnum IF_MV( && fce->fat_bpb == fat_bpb ))
                break; /* sector cached */

            map &= ~BIT_N(index);
        }
    }
    /* else just grabbing one of the buffers */

    if (!map)
    {
        /* sector not found so the LRU is the victim */
        index = cache_lru;
        fce = &cache_entry[index];
    }

    /* commit (if dirty) and discard cache entry */
    if (fce->flags)
    {
        if (fce->flags & FCE_DIRTY)
            cache_commit_sector(fce);

        unsigned int old_map_idx = HASH_SECTOR(fce->secnum);
        cache_map[old_map_idx] &= ~BIT_N(index);
    }

    if (fat_bpb)
    {
        /* keeping in main cache pool */
        if (fce->prev != cache_lru)
            cache_touch(index); /* make MRU */

        cache_map[map_idx] |= BIT_N(index);
        fce->flags   = FCE_INUSE;
        fce->secnum  = secnum;
    #ifdef HAVE_MULTIVOLUME
        fce->fat_bpb = fat_bpb;
    #endif
    }
    else
    {
        /* using as general buffer for something else */
        if (!cache_remove_lru())
        {
            fce->flags = 0;
            return NULL; /* oops, that was the last one */
        }

        fce->flags = FCE_BUF;
    }

    return fce->sectorbuf;
}

/* return entry to cache by buffer pointer */
static void cache_release_sector_buf(void *sectorbuf)
{
    unsigned int index = FCIDX_FROM_BUF(sectorbuf);

    if (index >= FAT_CACHE_SIZE)
        return;

    struct fat_cache_entry *fce = &cache_entry[index];

    if (!(fce->flags & FCE_BUF))
        return;

    /* return it to cache */
    cache_return_lru(index);
    fce->flags = 0;
}

/* mark used cache entry buffer as dirty */
static void cache_dirty_buf(void *sectorbuf)
{
    unsigned int index = FCIDX_FROM_BUF(sectorbuf);

    if (index >= FAT_CACHE_SIZE)
        return;

    struct fat_cache_entry *fce = &cache_entry[index];

    /* dirt remains, sticky until flushed */
    if (fce->flags & FCE_INUSE)
        fce->flags |= FCE_DIRTY;
}

#if 0
/* discard used cache entry by buffer */
static void cache_discard_buf(void *sectorbuf)
{
    unsigned int index = FCIDX_FROM_BUF(sectorbuf);

    if (index >= FAT_CACHE_SIZE)
        return;

    struct fat_cache_entry *fce = &cache_entry[index];

    if (fce->flags & FCE_INUSE)
    {
        unsigned int map_idx = HASH_SECTOR(fce->secnum);
        cache_map[map_idx] &= ~BIT_N(index);
        fce->flags = 0;
    }
}
#endif

/* commit all dirty cache entries to storage */
static int cache_commit(struct bpb *fat_bpb)
{
    LDEBUGF("cache_commit()\n");

    int rc;
    cache_lock();

#ifdef HAVE_FAT16SUPPORT
    if (!fat_bpb->is_fat16)
#endif
    {
        rc = update_fsinfo32(fat_bpb);
        if (rc < 0)
            FAT_ERR(10 * rc - 1);
    }

    for (unsigned int index = 0; index < FAT_CACHE_SIZE; index++)
    {
        struct fat_cache_entry *fce = &cache_entry[index];

        /* commit all dirty entries from that volume ignoring the ones used
           as buffers */
        if ((fce->flags & FCE_DIRTY) IF_MV( && fce->fat_bpb == fat_bpb))
            cache_commit_sector(fce);
    }

    rc = 0;
fat_error:
    cache_unlock();
    return rc;
}

/* discard all cache entries */
static void cache_discard(IF_MV_NONVOID(struct bpb *fat_bpb))
{
    LDEBUGF("cache_discard()\n");

    cache_lock();

    for (unsigned int index = 0; index < FAT_CACHE_SIZE; index++)
    {
        struct fat_cache_entry *fce = &cache_entry[index];

        /* discard all from that volume ignoring the ones used as buffers */
        if ((fce->flags & ~FCE_BUF) IF_MV( && fce->fat_bpb == fat_bpb ))
        {
            unsigned int map_idx = HASH_SECTOR(fce->secnum);
            cache_map[map_idx] &= ~BIT_N(index);
            fce->flags = 0;
        }
    }

    cache_unlock();
}

static void cache_init(bool startup)
{
    if (startup)
    {
        mutex_init(&cache_mutex);
        return;
    }

    cache_lock();

    cache_lru = 0;
    memset(cache_map, 0, sizeof (cache_map));

    for (unsigned int i = 0; i < FAT_CACHE_SIZE; i++)
    {
        struct fat_cache_entry *fce = &cache_entry[i];
        fce->prev      = (i - 1) % FAT_CACHE_SIZE;
        fce->next      = (i + 1) % FAT_CACHE_SIZE;
        fce->flags     = 0;
        fce->secnum    = 8; /* We use a "safe" sector just in case */
        fce->sectorbuf = cached_sector[i];
    #ifndef HAVE_MULTIVOLUME
        fce->fat_bpb   = &fat_bpbs[0];
    #endif
    }

    /* if not a 2^n size, then correct first entry's prev link */
    if (FAT_CACHE_SIZE & (FAT_CACHE_SIZE - 1))
        cache_entry[0].prev = FAT_CACHE_SIZE - 1;

    cache_unlock();
}

static uint8_t shortname_checksum(const unsigned char *shortname)
{
    /* calculate shortname checksum */
    uint8_t chksum = 0;

    for (unsigned int i = 0; i < 11; i++)
        chksum = (chksum << 7) + (chksum >> 1) + shortname[i];

    return chksum;
}

static unsigned char char2dos(unsigned char c, int *randomize)
{
    static const char invalid_chars[] = "\"*+,./:;<=>?[\\]|";

    if (c <= 0x20)
    {
        c = 0;   /* Illegal char, remove */
    }
    else if (strchr(invalid_chars, c) != NULL)
    {
        /* Illegal char, replace */
        c = '_';
        *randomize = 1; /* as per FAT spec */
    }
    else
    {
        c = toupper(c);
    }

    return c;
}

static void randomize_dos_name(unsigned char *name)
{
    unsigned char *tilde = NULL;    /* ~ location */
    unsigned char *lastpt = NULL;   /* last point of filename */
    unsigned char *nameptr = name;  /* working copy of name pointer */
    unsigned char num[9];           /* holds number as string */
    int i = 0;
    int cnt = 1;
    int numlen;
    int offset;

    while (i++ < 8)
    {
        /* hunt for ~ and where to put it */
        if (!tilde && *nameptr == '~')
            tilde = nameptr;

        if (!lastpt && (*nameptr == ' ' || *nameptr == '~'))
            lastpt = nameptr;

        nameptr++;
    }

    if (tilde)
    {
        /* extract current count and increment */
        memcpy(num, tilde + 1, 7 - (unsigned int)(tilde - name));
        num[7 - (unsigned int)(tilde - name)] = 0;
        cnt = atoi(num) + 1;
    }

    cnt %= 10000000; /* protection */
    snprintf(num, 9, "~%d", cnt);   /* allow room for trailing zero */
    numlen = strlen(num);           /* required space */
    offset = (unsigned int)(lastpt ? lastpt - name : 8); /* prev startpoint */

    if (offset > 8 - numlen)
        offset = 8 - numlen;  /* correct for new numlen */

    memcpy(&name[offset], num, numlen);

    /* in special case of counter overflow: pad with spaces */
    for (offset = offset + numlen; offset < 8; offset++)
        name[offset] = ' ';
}

static void create_dos_name(const unsigned char *name, unsigned char *newname)
{
    int randomize = 0;

    /* Find extension part */
    unsigned char *ext = strrchr(name, '.');
    if (ext == name)         /* handle .dotnames */
        ext = NULL;

    /* needs to randomize? */
    if ((ext && strlen(ext) > 4) ||
        (ext ? (unsigned int)(ext - name) : strlen(name)) > 8)
    {
        randomize = 1;
    }

    /* Name part */
    int i;
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

        for (int i = 8; *ext && i < 11; ext++)
        {
            unsigned char c = char2dos(*ext, &randomize);
            if (c)
                newname[i++] = c;
        }
    }

    if (randomize)
        randomize_dos_name(newname);
}

static unsigned long cluster2sec(struct bpb *fat_bpb, long cluster)
{
    long zerocluster = 2;

#ifdef HAVE_FAT16SUPPORT
    /* negative clusters (FAT16 root dir) don't get the 2 offset */
    if (fat_bpb->is_fat16 && cluster < 0)
    {
        zerocluster = 0;
    }
    else
#endif
    if ((unsigned long)cluster > fat_bpb->dataclusters + 1)
    {
        DEBUGF( "cluster2sec() - Bad cluster number (%ld)\n", cluster);
        return 0;
    }

    return (cluster - zerocluster) * (long)fat_bpb->bpb_secperclus
            + fat_bpb->firstdatasector;
}

#ifdef HAVE_FAT16SUPPORT
static long get_next_cluster16(struct bpb *fat_bpb, long startcluster)
{
    /* if FAT16 root dir, dont use the FAT */
    if (startcluster < 0)
        return startcluster + 1;

    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    cache_lock();

    uint16_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        cache_unlock();
        DEBUGF("get_next_cluster16: Could not cache sector %d\n", sector);
        return -1;
    }

    long next = letoh16(sec[offset]);

    /* is this last cluster in chain? */
    if (next >= FAT16_EOF_MARK)
        next = 0;

    cache_unlock();
    return next;
}

static long find_free_cluster16(struct bpb *fat_bpb, long startcluster)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        unsigned long nr = (i + sector) % fat_bpb->fatsize;
        uint16_t *sec = cache_sector(fat_bpb, nr + fat_bpb->fatrgnstart);
        if (!sec)
            break;

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++)
        {
            unsigned long k = (j + offset) % CLUSTERS_PER_FAT16_SECTOR;

            if (letoh16(sec[k]) == 0x0000)
            {
                unsigned long c = nr * CLUSTERS_PER_FAT16_SECTOR + k;
                 /* Ignore the reserved clusters 0 & 1, and also
                    cluster numbers out of bounds */
                if (c < 2 || c > fat_bpb->dataclusters + 1)
                    continue;

                LDEBUGF("find_free_cluster16(%lx) == %x\n", startcluster, c);

                fat_bpb->fsinfo.nextfree = c;
                return c;
            }
        }

        offset = 0;
    }

    LDEBUGF("find_free_cluster16(%lx) == 0\n", startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry16(struct bpb *fat_bpb, unsigned long entry,
                              unsigned long val)
{
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    val &= 0xFFFF;

    LDEBUGF("update_fat_entry16(%lx,%lx)\n", entry, val);

    if (entry == val)
        panicf("Creating FAT16 loop: %lx,%lx\n", entry, val);

    if (entry < 2)
        panicf("Updating reserved FAT16 entry %lu\n", entry);

    cache_lock();

    int16_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        cache_unlock();
        DEBUGF("update_fat_entry16: Could not cache sector %u\n", sector);
        return -1;
    }

    uint16_t curval = letoh16(sec[offset]);

    if (val)
    {
        /* being allocated */
        if (curval == 0x0000 && fat_bpb->fsinfo.freecount > 0)
            fat_bpb->fsinfo.freecount--;
    }
    else
    {
        /* being freed */
        if (curval != 0x0000)
            fat_bpb->fsinfo.freecount++;
    }

    LDEBUGF("update_fat_entry16: %lu free clusters\n",
            (unsigned long)fat_bpb->fsinfo.freecount);

    sec[offset] = htole16(val);
    cache_dirty_buf(sec);

    cache_unlock();
    return 0;
}

static void fat_recalc_free_internal16(struct bpb *fat_bpb)
{
    unsigned long free = 0;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        cache_lock();

        uint16_t *sec = cache_sector(fat_bpb, i + fat_bpb->fatrgnstart);
        if (!sec)
        {
            cache_unlock();
            break;
        }

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++)
        {
            unsigned long c = i * CLUSTERS_PER_FAT16_SECTOR + j;

            if (c < 2 || c > fat_bpb->dataclusters + 1) /* nr 0 is unused */
                continue;

            if (letoh16(sec[j]) != 0x0000)
                continue;

            free++;
            if (fat_bpb->fsinfo.nextfree == 0xffffffff)
                fat_bpb->fsinfo.nextfree = c;
        }

        cache_unlock();
    }

    cache_lock();
    fat_bpb->fsinfo.freecount = free;
    cache_unlock();
}
#endif /* #ifdef HAVE_FAT16SUPPORT */

static int update_fsinfo32(struct bpb *fat_bpb)
{
    uint8_t *fsinfo = cache_sector(fat_bpb, fat_bpb->bpb_fsinfo);
    if (!fsinfo)
    {
        DEBUGF( "update_fsinfo32() - Couldn't read FSInfo"
                " (err code %d)", rc);
        return -1;
    }

    INT322BYTES(fsinfo, FSINFO_FREECOUNT, fat_bpb->fsinfo.freecount);
    INT322BYTES(fsinfo, FSINFO_NEXTFREE, fat_bpb->fsinfo.nextfree);
    cache_dirty_buf(fsinfo);

    return 0;
}

static long get_next_cluster32(struct bpb *fat_bpb, long startcluster)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT_SECTOR;

    cache_lock();

    uint32_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        cache_unlock();
        DEBUGF("get_next_cluster32: Could not cache sector %d\n", sector);
        return -1;
    }

    long next = letoh32(sec[offset]) & 0x0fffffff;

    /* is this last cluster in chain? */
    if (next >= FAT_EOF_MARK)
        next = 0;

    cache_unlock();
    return next;
}

static long find_free_cluster32(struct bpb *fat_bpb, long startcluster)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT_SECTOR;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        unsigned long nr = (i + sector) % fat_bpb->fatsize;
        uint32_t *sec = cache_sector(fat_bpb, nr + fat_bpb->fatrgnstart);
        if (!sec)
            break;

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++)
        {
            unsigned long k = (j + offset) % CLUSTERS_PER_FAT_SECTOR;

            if (!(letoh32(sec[k]) & 0x0fffffff))
            {
                unsigned long c = nr * CLUSTERS_PER_FAT_SECTOR + k;
                 /* Ignore the reserved clusters 0 & 1, and also
                    cluster numbers out of bounds */
                if (c < 2 || c > fat_bpb->dataclusters + 1)
                    continue;

                LDEBUGF("find_free_cluster32(%lx) == %lx\n", startcluster, c);

                fat_bpb->fsinfo.nextfree = c;
                return c;
            }
        }

        offset = 0;
    }

    LDEBUGF("find_free_cluster32(%lx) == 0\n", startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry32(struct bpb *fat_bpb, unsigned long entry,
                              unsigned long val)
{
    unsigned long sector = entry / CLUSTERS_PER_FAT_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT_SECTOR;

    LDEBUGF("update_fat_entry32(%lx,%lx)\n", entry, val);

    if (entry == val)
        panicf("Creating FAT32 loop: %lx,%lx\n", entry, val);

    if (entry < 2)
        panicf("Updating reserved FAT32 entry %lu\n", entry);

    cache_lock();

    uint32_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        cache_unlock();
        DEBUGF("update_fat_entry32: Could not cache sector %u\n", sector);
        return -1;
    }

    uint32_t curval = letoh32(sec[offset]);

    if (val)
    {
        /* being allocated */
        if (!(curval & 0x0fffffff) && fat_bpb->fsinfo.freecount > 0)
            fat_bpb->fsinfo.freecount--;
    }
    else
    {
        /* being freed */
        if (curval & 0x0fffffff)
            fat_bpb->fsinfo.freecount++;
    }

    LDEBUGF("update_fat_entry32: %lu free clusters\n",
            (unsigned long)fat_bpb->fsinfo.freecount);

    /* don't change top 4 bits */
    sec[offset] = htole32((curval & 0xf0000000) | (val & 0x0fffffff));
    cache_dirty_buf(sec);

    cache_unlock();
    return 0;
}

static void fat_recalc_free_internal32(struct bpb *fat_bpb)
{
    unsigned long free = 0;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        cache_lock();

        uint32_t *sec = cache_sector(fat_bpb, i + fat_bpb->fatrgnstart);
        if (!sec)
        {
            cache_unlock();
            break;
        }

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++)
        {
            unsigned long c = i * CLUSTERS_PER_FAT_SECTOR + j;

            if (c < 2 || c > fat_bpb->dataclusters + 1) /* nr 0 is unused */
                continue;

            if (letoh32(sec[j]) & 0x0fffffff)
                continue;

            free++;
            if (fat_bpb->fsinfo.nextfree == 0xffffffff)
                fat_bpb->fsinfo.nextfree = c;
        }

        cache_unlock();
    }

    cache_lock();
    fat_bpb->fsinfo.freecount = free;
    update_fsinfo32(fat_bpb);
    cache_unlock();
}

static int fat_mount_internal(struct bpb *fat_bpb, IF_MD(int drive,)
                              long startsector)
{
    int rc;
    /* safe to grab buffer: bpb is irrelevant and no sector will be cached
       for this volume since it isn't mounted */
    uint8_t * const buf = fat_get_sector_buffer();
    if (!buf)
        FAT_ERR(-1);

    /* Read the sector */
    rc = storage_read_sectors(IF_MD(drive,) startsector, 1, buf);
    if(rc)
    {
        DEBUGF("fat_mount_internal() - Couldn't read BPB"
               " (error code %d)\n", rc);
        FAT_ERR(rc * 10 - 2);
    }

    bpb_clear(fat_bpb);

    fat_bpb->startsector    = startsector;
#ifdef HAVE_MULTIDRIVE
    fat_bpb->drive          = drive;
#endif
    fat_bpb->bpb_bytspersec = BYTES2INT16(buf, BPB_BYTSPERSEC);
    unsigned long secmult = fat_bpb->bpb_bytspersec / SECTOR_SIZE;
    /* Sanity check is performed later */

    fat_bpb->bpb_secperclus = secmult * buf[BPB_SECPERCLUS];
    fat_bpb->bpb_rsvdseccnt = secmult * BYTES2INT16(buf, BPB_RSVDSECCNT);
    fat_bpb->bpb_numfats    = buf[BPB_NUMFATS];
    fat_bpb->bpb_media      = buf[BPB_MEDIA];
    fat_bpb->bpb_fatsz16    = secmult * BYTES2INT16(buf, BPB_FATSZ16);
    fat_bpb->bpb_fatsz32    = secmult * BYTES2INT32(buf, BPB_FATSZ32);
    fat_bpb->bpb_totsec16   = secmult * BYTES2INT16(buf, BPB_TOTSEC16);
    fat_bpb->bpb_totsec32   = secmult * BYTES2INT32(buf, BPB_TOTSEC32);
    fat_bpb->last_word      = BYTES2INT16(buf, BPB_LAST_WORD);

    /* calculate a few commonly used values */
    if (fat_bpb->bpb_fatsz16 != 0)
        fat_bpb->fatsize = fat_bpb->bpb_fatsz16;
    else
        fat_bpb->fatsize = fat_bpb->bpb_fatsz32;

    fat_bpb->fatrgnstart = fat_bpb->bpb_rsvdseccnt;
    fat_bpb->fatrgnend   = fat_bpb->bpb_rsvdseccnt + fat_bpb->fatsize;

    if (fat_bpb->bpb_totsec16 != 0)
        fat_bpb->totalsectors = fat_bpb->bpb_totsec16;
    else
        fat_bpb->totalsectors = fat_bpb->bpb_totsec32;

    unsigned int rootdirsectors = 0;
#ifdef HAVE_FAT16SUPPORT
    fat_bpb->bpb_rootentcnt = BYTES2INT16(buf, BPB_ROOTENTCNT);

    if (!fat_bpb->bpb_bytspersec)
        FAT_ERR(-2);

    rootdirsectors = secmult * ((fat_bpb->bpb_rootentcnt * DIR_ENTRY_SIZE
                     + fat_bpb->bpb_bytspersec - 1) / fat_bpb->bpb_bytspersec);
#endif /* #ifdef HAVE_FAT16SUPPORT */

    fat_bpb->firstdatasector = fat_bpb->bpb_rsvdseccnt
        + fat_bpb->bpb_numfats * fat_bpb->fatsize
        + rootdirsectors;

    /* Determine FAT type */
    unsigned long datasec = fat_bpb->totalsectors - fat_bpb->firstdatasector;

    if (!fat_bpb->bpb_secperclus)
        FAT_ERR(-2);

    fat_bpb->dataclusters = datasec / fat_bpb->bpb_secperclus;

#ifdef TEST_FAT
    /*
      we are sometimes testing with "illegally small" fat32 images,
      so we don't use the proper fat32 test case for test code
    */
    if (fat_bpb->bpb_fatsz16)
#else
    if (fat_bpb->dataclusters < 65525)
#endif
    { /* FAT16 */
#ifdef HAVE_FAT16SUPPORT
        fat_bpb->is_fat16 = true;
        if (fat_bpb->dataclusters < 4085)
        { /* FAT12 */
            DEBUGF("This is FAT12. Go away!\n");
            FAT_ERR(-2);
        }
#else /* #ifdef HAVE_FAT16SUPPORT */
        DEBUGF("This is not FAT32. Go away!\n");
        FAT_ERR(-2);
#endif /* #ifndef HAVE_FAT16SUPPORT */
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        /* FAT16 specific part of BPB */
        fat_bpb->rootdirsector = fat_bpb->bpb_rsvdseccnt
            + fat_bpb->bpb_numfats * fat_bpb->bpb_fatsz16;
        long dirclusters = ((rootdirsectors + fat_bpb->bpb_secperclus - 1)
            / fat_bpb->bpb_secperclus); /* rounded up, to full clusters */
        /* I assign negative pseudo cluster numbers for the root directory,
           their range is counted upward until -1. */
        fat_bpb->bpb_rootclus = 0 - dirclusters; /* backwards, before the data */
        fat_bpb->rootdirsectornum = dirclusters * fat_bpb->bpb_secperclus
            - rootdirsectors;
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        /* FAT32 specific part of BPB */
        fat_bpb->bpb_rootclus  = BYTES2INT32(buf, BPB_ROOTCLUS);
        fat_bpb->bpb_fsinfo    = secmult * BYTES2INT16(buf, BPB_FSINFO);
        fat_bpb->rootdirsector = cluster2sec(fat_bpb, fat_bpb->bpb_rootclus);
    }

    rc = bpb_is_sane(fat_bpb);
    if (rc < 0)
    {
        DEBUGF("fat_mount_internal() - BPB is insane!\n");
        FAT_ERR(rc * 10 - 3);
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        fat_bpb->fsinfo.freecount = 0xffffffff; /* force recalc later */
        fat_bpb->fsinfo.nextfree = 0xffffffff;
    }
    else
#endif /* #ifdef HAVE_FAT16SUPPORT */
    {
        /* Read the fsinfo sector */
        rc = storage_read_sectors(IF_MD(drive,)
                startsector + fat_bpb->bpb_fsinfo, 1, buf);

        if (rc < 0)
        {
            DEBUGF("fat_mount_internal() - Couldn't read FSInfo"
                   " (error code %d)\n", rc);
            FAT_ERR(rc * 10 - 4);
        }

        fat_bpb->fsinfo.freecount = BYTES2INT32(buf, FSINFO_FREECOUNT);
        fat_bpb->fsinfo.nextfree = BYTES2INT32(buf, FSINFO_NEXTFREE);
    }

#ifdef HAVE_FAT16SUPPORT
    /* Fix up calls that change per FAT type */
    if (fat_bpb->is_fat16)
    {
        fat_bpb->get_next_cluster_ftype         = get_next_cluster16;
        fat_bpb->find_free_cluster_ftype        = find_free_cluster16;
        fat_bpb->update_fat_entry_ftype         = update_fat_entry16;
        fat_bpb->fat_recalc_free_internal_ftype = fat_recalc_free_internal16;
    }
    else
    {
        fat_bpb->get_next_cluster_ftype         = get_next_cluster32;
        fat_bpb->find_free_cluster_ftype        = find_free_cluster32;
        fat_bpb->update_fat_entry_ftype         = update_fat_entry32;
        fat_bpb->fat_recalc_free_internal_ftype = fat_recalc_free_internal32;
    }
#endif /* #ifdef HAVE_FAT16SUPPORT */

    rc = 0;
fat_error:
    fat_release_sector_buffer(buf);
    return rc;
}

static union fat_direntry_raw * cache_direntry(struct bpb *fat_bpb,
    struct fat_file *file, unsigned int entry)
{
    /* the cache lock must be held before calling */
    /* this returns success through the end of the last allocated cluster
       of the directory, returning EOF only if an attempt is made to read
       beyond it; at which time the file pointer is set at the last sector
       to allow appending to the file */
    bool eof = false;
    long file_clusternum = file->clusternum;
    long cluster = file->lastcluster;

    unsigned long seeksector = entry / DIR_ENTRIES_PER_SECTOR;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16 && file->firstcluster < 0)
        seeksector += fat_bpb->rootdirsectornum;
#endif

    long clusternum = seeksector / fat_bpb->bpb_secperclus;
    unsigned long sectornum = seeksector % fat_bpb->bpb_secperclus;

    if (clusternum != file_clusternum)
    {
        /* not same cluster so seek to it */

        if (clusternum < file_clusternum)
        {
            /* going backwards; seek from beginning */
            file_clusternum = 0;
            cluster = file->firstcluster;
        }

        while (file_clusternum < clusternum)
        {
            long next = get_next_cluster(fat_bpb, cluster);

            if (!next)
            {
                /* passed end */
                sectornum = fat_bpb->bpb_secperclus - 1;
                eof = true;
                break;
            }

            cluster = next;
            file_clusternum++;
        }
    }

    unsigned long sector = cluster2sec(fat_bpb, cluster) + sectornum;
    union fat_direntry_raw *ent = NULL;

    if (!eof)
    {
        ent = cache_sector(fat_bpb, sector);
        if (!ent)
            return NULL;

        ent += entry % DIR_ENTRIES_PER_SECTOR; 
    }

    file->lastcluster = cluster;
    file->lastsector  = sector;
    file->clusternum  = file_clusternum;
    file->sectornum   = sectornum;
    file->eof         = eof;

    return ent;
}

static long next_write_cluster(struct bpb *fat_bpb, struct fat_file *file,
                               long oldcluster)
{
    LDEBUGF("next_write_cluster(%lx,%lx)\n", file->firstcluster, oldcluster);

    long cluster = 0;

    /* cluster already allocated? */
    if (oldcluster)
        cluster = get_next_cluster(fat_bpb, oldcluster);

    if (!cluster)
    {
        cache_lock();

        /* passed end of existing entries and now need to append */
        if (oldcluster > 0)
            cluster = find_free_cluster(fat_bpb, oldcluster + 1);
        else if (oldcluster == 0)
            cluster = find_free_cluster(fat_bpb, fat_bpb->fsinfo.nextfree);
#ifdef HAVE_FAT16SUPPORT
        else /* negative, pseudo-cluster of the root dir */
        {
            cache_unlock();
            return 0; /* impossible to append something to the root */
        }
#endif

        if (cluster)
        {
            /* create the cluster chain */
            if (oldcluster)
                update_fat_entry(fat_bpb, oldcluster, cluster);
            else
                file->firstcluster = cluster;

            update_fat_entry(fat_bpb, cluster, FAT_EOF_MARK);
        }
        else
        {
#ifdef TEST_FAT
            if (fat_bpb->fsinfo.freecount > 0)
                panicf("There is free space, but find_free_cluster() "
                       "didn't find it!\n");
#endif
            DEBUGF("next_write_cluster(): Disk full!\n");
        }

        cache_unlock();
    }

    return cluster;
}

static int fat_clear_cluster(struct bpb *fat_bpb, long cluster)
{
    int rc;
    unsigned long sector = cluster2sec(fat_bpb, cluster);
    if (sector == 0)
        FAT_ERR(-1);

    for (unsigned int i = 0; i < fat_bpb->bpb_secperclus; i++, sector++)
    {
        cache_lock();

        void *sec = cache_get_sector_buf(fat_bpb, sector);
        if (!sec)
        {
            cache_unlock();
            FAT_ERR(-2);
        }

        memset(sec, 0, SECTOR_SIZE);
        cache_dirty_buf(sec);
        cache_unlock();
    }

    rc = 0;
fat_error:
    return rc;
}

static int transfer(struct bpb *fat_bpb, unsigned long start, long count,
                    char *buf, bool write)
{
    int rc;

    LDEBUGF("transfer(s=%lx, c=%lx, %s)\n",
            start + fat_bpb->startsector, count, write ? "write" : "read");

    if (write)
    {
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
        {
            panicf("Write %ld after data\n",
                   start + count - fat_bpb->totalsectors);
        }

        rc = storage_write_sectors(IF_MD(fat_bpb->drive,)
                                   start + fat_bpb->startsector, count, buf);
    }
    else
    {
        rc = storage_read_sectors(IF_MD(fat_bpb->drive,)
                                  start + fat_bpb->startsector, count, buf);
    }

    if (rc < 0)
    {
        DEBUGF("transfer() - Couldn't %s sector %lx"
               " (error code %d)\n",
               write ? "write":"read", start, rc);
        return rc;
    }

    return 0;
}

static void fat_open_internal(IF_MV(int volume,)
                              long startcluster,
                              struct fat_file *file,
                              const struct fat_dir *dir)
{
    if (dir)
    {
        /* Remember where the file's dir entry is located */
        file->direntry   = dir->entry - 1;
        file->direntries = dir->entrycount;
        file->dircluster = dir->file.firstcluster;
    }
    else /* root or don't care */
    {
        file->direntry   = 0;
        file->direntries = 0;
        file->dircluster = 0;
    }

    file->firstcluster = startcluster;
    file->lastcluster  = startcluster;
    file->lastsector   = 0;
    file->clusternum   = 0;
    file->sectornum    = -1;
    file->eof          = false;
#ifdef HAVE_MULTIVOLUME
    file->volume       = volume;
#endif

    LDEBUGF("fat_open_internal(%lx), entry %d\n", startcluster,
            file->direntry);
}

static void fat_time(uint16_t *date, uint16_t *time, int16_t *tenth)
{
#if CONFIG_RTC
#if 0
    struct tm tm;
    get_time_r(&tm);
#endif
    struct tm *tm = get_time();

    if (date)
    {
        *date = ((tm->tm_year - 80) << 9) |
                ((tm->tm_mon + 1) << 5) |
                tm->tm_mday;
    }

    if (time)
    {
        *time = (tm->tm_hour << 11) |
                (tm->tm_min << 5) |
                (tm->tm_sec >> 1);
    }

    if (tenth)
        *tenth = (tm->tm_sec & 1) * 100;
#else
    /* non-RTC version returns an increment from the supplied time, or a
     * fixed standard time/date if no time given as input */

/* Macros to convert a 2-digit string to a decimal constant.
   (YEAR), MONTH and DAY are set by the date command, which outputs
   DAY as 00..31 and MONTH as 01..12. The leading zero would lead to
   misinterpretation as an octal constant. */
#define S100(x) 1 ## x
#define C2DIG2DEC(x) (S100(x)-100)
/* The actual build date, as FAT date constant */
#define BUILD_DATE_FAT (((YEAR - 1980) << 9) \
                        | (C2DIG2DEC(MONTH) << 5) \
                        | C2DIG2DEC(DAY))

    bool date_forced = false;
    bool next_day = false;
    unsigned time2 = 0; /* double time, for CRTTIME with 1s precision */

    if (date && *date < BUILD_DATE_FAT)
    {
        *date = BUILD_DATE_FAT;
        date_forced = true;
    }

    if (time)
    {
        time2 = *time << 1;
        if (time2 == 0 || date_forced)
        {
            time2 = (11 < 6) | 11; /* set to 00:11:11 */
        }
        else
        {
            unsigned mins  = (time2 >> 6) & 0x3f;
            unsigned hours = (time2 >> 12) & 0x1f;

            mins = 11 * ((mins/11) + 1); /* advance to next multiple of 11 */
            if (mins > 59)
            {
                mins = 11; /* 00 would be a bad marker */
                if (++hours > 23)
                {
                    hours = 0;
                    next_day = true;
                }
            }
            time2 = (hours << 12) | (mins << 6) | mins; /* secs = mins */
        }
        *time = time2 >> 1;
    }

    if (tenth)
        *tenth = (time2 & 1) * 100;

    if (date && next_day)
    {
        static const unsigned char daysinmonth[] =
                              {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        unsigned day   = *date & 0x1f;
        unsigned month = (*date >> 5) & 0x0f;
        unsigned year  = (*date >> 9) & 0x7f;

        /* simplification: ignore leap years */
        if (++day > daysinmonth[month-1])
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

#endif /* CONFIG_RTC */
}

static int write_long_name(struct bpb *fat_bpb, struct fat_dir *dir,
                           struct fat_file *file,  unsigned int firstentry,
                           unsigned int numentries, const unsigned char *name,
                           const unsigned char *shortname,
                           union fat_direntry_raw *srcent, unsigned int flags)
{
    int rc;
    union fat_direntry_raw *ent;

    LDEBUGF("write_long_name(file:%lx, first:%d, num:%d, name:%s)\n",
            dir->file.firstcluster, firstentry, numentries, name);

    uint16_t date = 0, time = 0, tenth = 0;
    fat_time(&date, &time, &tenth);
    time = htole16(time);
    date = htole16(date);

    /* shortname checksum saved in each longname entry */
    uint8_t chksum = shortname_checksum(shortname);

    /* we need to convert the name first since the entries are written in
       reverse order */
    unsigned long namelen = utf8length(name);
    unsigned long namepadlen = ALIGN_UP(namelen, FATLONG_NAME_CHARS);
    uint16_t name_utf16[namepadlen];

    for (unsigned long i = 0; i < namepadlen; i++)
    {
        if (i < namelen)
            name = utf8decode(name, &name_utf16[i]);
        else if (i == namelen)
            name_utf16[i] = 0x0000; /* name doesn't fill last block */
        else /* i > namelen */
            name_utf16[i] = 0xffff; /* pad-out to end */
    }

    cache_lock();

    /* longame entries */
    for (unsigned int i = 0; i < numentries; i++)
    {
        ent =  cache_direntry(fat_bpb, &dir->file, firstentry + i);
        if (!ent)
            FAT_ERR(-1);

        /* verify this entry is free */
        if (ent->name[0] && ent->name[0] != 0xe5)
        {
            panicf("Dir entry %d in sector %x is not free! "
                   "%02x %02x %02x %02x",
                   i + firstentry, (unsigned)&dir->file.lastsector,
                   (unsigned)ent->data[0], (unsigned)ent->data[1],
                   (unsigned)ent->data[2], (unsigned)ent->data[3]);
        }

        memset(ent->data, 0, DIR_ENTRY_SIZE);

        unsigned int ord = numentries - i - 1;
        ent->ldir_ord = ord;

        if (i == 0)
            ent->ldir_ord |= FATLONG_ORD_F_LAST;

        ent->ldir_attr = FAT_ATTR_LONG_NAME;
        ent->ldir_chksum = chksum;

        /* set name */
        uint16_t *utf16 = &name_utf16[(ord - 1)*FATLONG_NAME_CHARS];
        memcpy(ent->ldir_name1, &utf16[ 0], 10);
        memcpy(ent->ldir_name2, &utf16[ 5], 12);
        memcpy(ent->ldir_name3, &utf16[11],  4);

        cache_dirty_buf(ent);
        LDEBUGF("Longname entry %d\n", ord);
    }

    /* shortname entry */
    LDEBUGF("Shortname entry: %s\n", shortname);

    ent = cache_direntry(fat_bpb, &dir->file, firstentry + numentries - 1);
    if (!ent)
        FAT_ERR(-2);

    if (srcent && !(flags & DIRENTRY_RETURN_SHORT))
    {
        *ent = *srcent;
    }
    else
    {
        memset(ent->data, 0, DIR_ENTRY_SIZE);

        if (flags & DIRENTRY_DIR)
            ent->attr = FAT_ATTR_DIRECTORY;
    }

    if (file)
        rawent_set_fstclus(ent, file->firstcluster);

    /* short name may change even if just moving */
    memcpy(ent->name, shortname, 11);

    if (!(flags & DIRENTRY_KEEP_TIMES))
    {
        ent->crttimetenth = tenth;
        ent->crttime      = time;
        ent->crtdate      = date;
        ent->wrttime      = time;
        ent->wrtdate      = date;
    }

    ent->lstaccdate = date;

    if (srcent && (flags & DIRENTRY_RETURN_SHORT))
        *srcent = *ent; /* caller wants */

    cache_dirty_buf(ent);

    rc = 0;
fat_error:
    cache_unlock();
    return rc;
}

static int fat_checkname(const unsigned char *newname)
{
    static const char invalid_chars[] = "\"*/:<>?\\|";

    if (!newname)
        return -1;

    int len = strlen(newname);

    /* More sanity checks are probably needed */
    if (len == 0 || len > 255 || newname[len - 1] == '.')
        return -1;

    while (*newname)
    {
        if (*newname < ' ' || strchr(invalid_chars, *newname) != NULL)
            return -1;

        newname++;
    }

    /* check trailing space(s) */
    if (*--newname == ' ')
        return -1;

    return 0;
}

static int add_dir_entry(struct bpb *fat_bpb, struct fat_dir *dir,
                         struct fat_file *file, const char *name,
                         union fat_direntry_raw *srcent, unsigned int flags)
{
    int rc;
    LDEBUGF("add_dir_entry(%s,%lx)\n", name, file->firstcluster);

    unsigned char shortname[11];
    int entries_needed;

    if ((flags & DIRENTRY_DIR) && IS_DOTDIR_NAME(name))
    {
        /* The "." and ".." directory entries must not be long names */
        int dots = strlcpy(shortname, name, 11);
        memset(&shortname[dots], ' ', 11 - dots);
        entries_needed = 1;
    }
    else
    {
        rc = fat_checkname(name);
        if (rc < 0)
            FAT_ERR(rc * 10 - 1); /* filename is invalid */

        create_dos_name(name, shortname);

        /* one dir entry needed for every 13 characters of filename,
           plus one entry for the short name */
        entries_needed = (utf8length(name) + FATLONG_NAME_CHARS - 1)
                         / FATLONG_NAME_CHARS + 1;
    }

    int entry = 0;
    int entries_found = 0;
    int firstentry = -1;
    int entperclus = DIR_ENTRIES_PER_SECTOR*fat_bpb->bpb_secperclus;

    dir->dirty = true;

    /* step 1: search for a sufficiently-long run of free entries and check
               for duplicate shortname */
    cache_lock();

    bool done = false;
    while (!done)
    {
        union fat_direntry_raw *ent =
            cache_direntry(fat_bpb, &dir->file, entry);

        if (!ent)
        {
            if (dir->file.eof)
            {
                LDEBUGF("add_dir_entry() - End of dir %d\n", entry);
                break;
            }

            DEBUGF("add_dir_entry() - Couldn't read dir"
                   " (error code %d)\n", rc);
            cache_unlock();
            FAT_ERR(-1);
        }

        switch (ent->name[0])
        {
        case 0:    /* all remaining entries in cluster are free */
            LDEBUGF("Found end of dir %d\n", entry);
            int found = entperclus - (entry % entperclus);
            entries_found += found;
            entry += found; /* move entry passed end of cluster */
            done = true;
            break;

        case 0xe5: /* individual free entry */
            entries_found++;
            entry++;
            LDEBUGF("Found free entry %d (%d/%d)\n",
                    entry, entries_found, entries_needed);
            break;

        default:   /* occupied */
            /* check that our intended shortname doesn't already exist */
            if (!strncmp(shortname, ent->name, 11))
            {
                /* shortname exists already, make a new one */
                randomize_dos_name(shortname);
                LDEBUGF("Duplicate shortname, changing to %s\n",
                        shortname);

                /* name has changed, we need to restart search */
                entry = 0;
                firstentry = -1;
            }
            else
            {
                entry++;
            }

            entries_found = 0;
            break;
        }

        if (firstentry < 0 && entries_found >= entries_needed)
        {
            /* found adequate space; point to initial free entry */
            firstentry = entry - entries_found;
        }
    }

    cache_unlock();

    /* step 2: extend the dir if necessary */
    if (firstentry < 0)
    {
        LDEBUGF("Adding new cluster(s) to dir\n");

        if (entry + entries_needed - entries_found > 65536)
        {
            /* FAT specification allows no more than 65536 entries (2MB)
               per directory */
            LDEBUGF("Directory would be too large.\n");
            FAT_ERR(-3);
        }

        unsigned long cluster    = dir->file.lastcluster;
        unsigned long clusternum = dir->file.clusternum;
        dir->file.eof = false;

        while (entries_found < entries_needed)
        {
            cluster = next_write_cluster(fat_bpb, &dir->file, cluster);
            if (!cluster)
                FAT_ERR(-4); /* no more room or something went wrong */

            /* we must clear whole clusters */
            rc = fat_clear_cluster(fat_bpb, cluster);
            if (rc < 0)
                FAT_ERR(rc * 10 - 5);

            entries_found += entperclus;
            entry += entperclus;
            clusternum++;
        }

        /* keep file pos valid */
        unsigned long sectornum = fat_bpb->bpb_secperclus - 1;
        dir->file.lastcluster = cluster;
        dir->file.lastsector  = cluster2sec(fat_bpb, cluster) + sectornum;
        dir->file.clusternum  = clusternum;
        dir->file.sectornum   = sectornum;

        firstentry = entry - entries_found;
    }

    /* step 3: add entry */
    LDEBUGF("Adding longname to entry %d\n", firstentry);
    rc = write_long_name(fat_bpb, dir, file, firstentry, entries_needed,
                         name, shortname, srcent, flags);
    if (rc < 0)
        FAT_ERR(rc * 10 - 6);

    /* remember where the shortname dir entry is located */
    file->direntry   = firstentry + entries_needed - 1;
    file->direntries = entries_needed;
    file->dircluster = dir->file.firstcluster;
#ifdef HAVE_MULTIVOLUME
    /* inherit the volume, to make sure */
    file->volume = dir->file.volume;
#endif
    LDEBUGF("Added new dir entry %d, using %d slots.\n",
            file->direntry, file->direntries);

    rc = 0;
fat_error:
    return rc;
}

static int update_short_entry(struct bpb *fat_bpb, struct fat_file *file,
                              unsigned long size)
{
    int rc;

    LDEBUGF("update_file_size(cluster:%lx entry:%d size:%ld)\n",
            file->firstcluster, file->direntry, size);

#if CONFIG_RTC
    uint16_t time = 0;
    uint16_t date = 0;
#else
    /* get old time to increment from */
    uint16_t time = le16toh(ent->wrttime);
    uint16_t date = le16toh(ent->wrtdate);
#endif
    fat_time(&date, &time, NULL);
    date = htole16(date);
    time = htole16(time);

    struct fat_file dir_file;
    fat_open_internal(IF_MV(file->volume,) file->dircluster, &dir_file, NULL);

    cache_lock();

    union fat_direntry_raw *ent =
        cache_direntry(fat_bpb, &dir_file, file->direntry);
    if (!ent)
        FAT_ERR(-1);

    if (!ent->name[0] || ent->name[0] == 0xe5)
        panicf("Updating size on empty dir entry %d\n", file->direntry);

    /* basic file data */
    rawent_set_fstclus(ent, file->firstcluster);
    ent->filesize = htole32(size);

    /* time and date info */
    ent->wrttime    = time;
    ent->wrtdate    = date;
    ent->lstaccdate = date;

    cache_dirty_buf(ent);

    rc = 0;
fat_error:
    cache_unlock();
    return rc;
}

static void parse_short_direntry(struct fat_direntry *de,
                                 const union fat_direntry_raw *ent)
{
    de->attr         = ent->attr;
    de->crttimetenth = ent->crttimetenth;
    de->crttime      = letoh16(ent->crttime);
    de->crtdate      = letoh16(ent->crtdate);
    de->lstaccdate   = letoh16(ent->lstaccdate);
    de->wrttime      = letoh16(ent->wrttime);
    de->wrtdate      = letoh16(ent->wrtdate);
    de->filesize     = letoh32(ent->filesize);
    de->firstcluster = ((uint32_t)letoh16(ent->fstcluslo)      ) |
                       ((uint32_t)letoh16(ent->fstclushi) << 16);

    /* fix the name */
    bool lowercase = ent->ntres & FAT_NTRES_LC_NAME;
    unsigned char c = ent->name[0];

    if (c == 0x05)  /* special kanji char */
        c = 0xe5;

    int j = 0;

    for (int i = 0; c != ' '; c = ent->name[i])
    {
        de->shortname[j++] = lowercase ? tolower(c) : c;

        if (++i >= 8)
            break;
    }

    if (ent->name[8] != ' ')
    {
        lowercase = ent->ntres & FAT_NTRES_LC_EXT;
        de->shortname[j++] = '.';

        for (int i = 8; i < 11 && (c = ent->name[i]) != ' '; i++)
            de->shortname[j++] = lowercase ? tolower(c) : c;
    }

    de->shortname[j] = 0;
}

/* initialize the parse state; call before parsing first long entry */
static void NO_INLINE parse_longname_start(struct longname_parse *data)
{
    /* no inline so gcc can't figure out what isn't initialized here;
       ord_max is king as to the validity of all other fields */
    data->ord_max = 0;
}

/* convert the FAT long name entry to utf8encoded segment; de->name is
   used as the working buffer and the output buffer */
static void parse_longname_entry(struct fat_direntry *de,
                                 struct longname_parse *data,
                                 union fat_direntry_raw *lfnent)
{
    unsigned int ord = lfnent->ldir_ord;

    /* is this entry the first long entry ? (first in order but
       containing last part) */
    if (ord & FATLONG_ORD_F_LAST)
    {
        ord &= ~FATLONG_ORD_F_LAST;

        if (ord == 0 || ord > FATLONG_MAX_ORDER)
        {
            data->ord_max = 0;
            return;
        }

        data->utf8namepos = sizeof (de->name);
        data->ord_max     = ord;
        data->ord         = ord;
    }
    else
    {
        if (data->ord_max == 0)
            return;

        if (ord != data->ord_last - 1)
        {
            data->ord_max = 0;
            return;
        }
    }

    data->chksums[ord] = lfnent->ldir_chksum;
    data->ord_last = ord; 

    /* One character at a time. "4" is the maximum size of a UTF8
     * encoded character in rockbox */
    unsigned char utf8seg[FATLONG_NAME_CHARS*4];
    unsigned char *utf8ptr = utf8seg;

    unsigned int i = 1;

    do
    {
        uint16_t ucs = BYTES2INT16(lfnent->data, i);

        /* check for one of the early-terminating characters */
        if (ucs == 0 || ucs == 0xffff)
            break;

        utf8ptr = utf8encode(ucs, utf8ptr);

        switch (i += 2)
        {
        case 11: i = 14; break;
        case 26: i = 28; break;
        }
    }
    while (i < 32);

    unsigned int utf8namepos = data->utf8namepos;
    unsigned int utf8seglen = utf8ptr - utf8seg;

    if (utf8seglen >= utf8namepos)
    {
        data->ord_max = 0;
        return; /* name will be too long */
    }

    utf8namepos -= utf8seglen;
    memcpy(&de->name[utf8namepos], utf8seg, utf8seglen);
    data->utf8namepos = utf8namepos;
}

/* finish parsing of the longname entries */
static bool parse_longname_finish(struct fat_direntry *de,
                                  struct longname_parse *data,
                                  union fat_direntry_raw *shortent)
{
    if (data->ord_max == 0 || data->ord_last != 1)
        return false;

    uint8_t chksum = shortname_checksum(shortent->name);

    /* check all the checksums */
    for (unsigned int i = 1; i <= data->ord_max; i++)
    {
        if (data->chksums[i] != chksum)
            return false;
    }

    /* shift the entry name */
    unsigned int utf8namepos = data->utf8namepos;
    unsigned int len = sizeof (de->name) - utf8namepos;
    memmove(de->name, &de->name[utf8namepos], len);
    de->name[len] = 0;

    return true;
}

static int free_direntries(struct bpb *fat_bpb, struct fat_file *file)
{
    int rc;

    struct fat_file dir_file;
    fat_open_internal(IF_MV(file->volume,) file->dircluster, &dir_file, NULL);

    unsigned int numentries = file->direntries;
    unsigned int entry = file->direntry - numentries + 1;

    for (unsigned int i = 0; i < numentries; i++, entry++)
    {
        LDEBUGF("Clearing dir entry %d (%d/%d)\n",
                entry, i + 1, numentries);

        cache_lock();

        union fat_direntry_raw *ent =
            cache_direntry(fat_bpb, &dir_file, entry);
        if (!ent)
        {
            cache_unlock();
            FAT_ERR(-1);
        }

        ent->name[0] = 0xe5;
        cache_dirty_buf(ent);
        cache_unlock();
    }

    rc = 0;
fat_error:
    return rc;
}

int fat_mount(IF_MV(int volume,) IF_MD(int drive,) long startsector)
{
    struct bpb * const fat_bpb = FAT_BPB_WRITE(volume);

    if (!fat_bpb)
        return -1;

    int rc;
    if (fat_bpb->mounted)
        FAT_ERR(-2); /* double mount */

    rc = fat_mount_internal(fat_bpb, IF_MD(drive,) startsector);
    if (rc < 0)
        FAT_ERR(10 * rc - 3);

    /* calculate freecount if unset */
    if (fat_bpb->fsinfo.freecount == 0xffffffff)
        fat_recalc_free_internal(fat_bpb);

    LDEBUGF("Freecount: %ld\n", (unsigned long)fat_bpb->fsinfo.freecount);
    LDEBUGF("Nextfree: 0x%lx\n", (unsigned long)fat_bpb->fsinfo.nextfree);
    LDEBUGF("Cluster count: 0x%lx\n", fat_bpb->dataclusters);
    LDEBUGF("Sectors per cluster: %d\n", fat_bpb->bpb_secperclus);
    LDEBUGF("FAT sectors: 0x%lx\n", fat_bpb->fatsize);

    fat_bpb->mounted = true;

    rc = 0;
fat_error:
    FAT_BPB_WRITE_DONE(fat_bpb);
    return rc;
}

int fat_unmount(IF_MV(int volume,) bool flush)
{
    struct bpb * const fat_bpb = FAT_BPB_WRITE(volume);

    if (!fat_bpb)
        return -1;

    int rc;

    if (flush && fat_bpb->mounted)
    {
        /* the clean way, while still alive */
        rc = cache_commit(fat_bpb);
        if (rc < 0)
            FAT_ERR(10 * rc - 2);
    }
    /* else volume is not accessible any more, e.g. MMC removed */

    /* free the entries for this volume */
    cache_discard(IF_MV(fat_bpb));

    fat_bpb->mounted = false;

    rc = 0;
fat_error:
    FAT_BPB_WRITE_DONE(fat_bpb);
    return rc;
}

void fat_recalc_free(IF_MV_NONVOID(int volume))
{
    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (fat_bpb)
    {
        fat_recalc_free_internal(fat_bpb);
        FAT_BPB_READ_DONE(fat_bpb);
    }
}

int fat_open(IF_MV(int volume,) long startcluster,
             struct fat_file *file, const struct fat_dir *dir)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (!fat_bpb)
        return -1;

    fat_open_internal(IF_MV(volume,) startcluster, file, dir);

    FAT_BPB_READ_DONE(fat_bpb);
    return 0;
}

int fat_create_file(const char *name, struct fat_file *file,
                    struct fat_dir *dir)
{
    LDEBUGF("fat_create_file(\"%s\",%lx,%lx)\n", name, (long)file, (long)dir);
    struct bpb * const fat_bpb = FAT_BPB_READ(dir->file.volume);

    if (!fat_bpb)
        return -1;

    int rc;
    dir_lock();

    fat_open_internal(IF_MV(dir->file.volume,) 0, file, NULL);
    rc = add_dir_entry(fat_bpb, dir, file, name, NULL, 0);
    if (rc < 0)
        FAT_ERR(10 * rc - 2);

    rc = cache_commit(fat_bpb);
    if (rc < 0)
        FAT_ERR(rc * 10 - 3);

    rc = 0;
fat_error:
    dir_unlock();
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_create_dir(const char *name, struct fat_dir *newdir,
                   struct fat_dir *dir)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(dir->file.volume);

    if (!fat_bpb)
        return -1;

    int rc;
    union fat_direntry_raw newent; /* short entry template for
                                      ".", ".." and parent entries */

    dir_lock();

    LDEBUGF("fat_create_dir(\"%s\",%lx,%lx)\n", name, (long)newdir, (long)dir);

    /* first, allocate a new (orphan) cluster for the directory; it won't
       be made enumerable unless everything succeeds */
    cache_lock();

    long firstcluster = find_free_cluster(fat_bpb, fat_bpb->fsinfo.nextfree);
    if (firstcluster == 0)
    {
        cache_unlock();
        FAT_ERR(-2);
    }

    fat_open_internal(IF_MV(dir->file.volume,) firstcluster,
                      &newdir->file, NULL);
    newdir->entry = 0;
    newdir->entrycount = 0;
    newdir->cache = fat_get_sector_buffer();

    rc = update_fat_entry(fat_bpb, firstcluster, FAT_EOF_MARK);
    if (rc < 0)
    {
        cache_unlock();
        FAT_ERR(rc * 10 - 3);
    }

    cache_unlock();

    /* clear the entire cluster */
    rc = fat_clear_cluster(fat_bpb, firstcluster);
    if (rc < 0)
        FAT_ERR(rc * 10 - 4);

    struct fat_file dummyfile;

    /* add the "." entry */
    memset(&dummyfile, 0, sizeof (struct fat_file));

    dummyfile.firstcluster = firstcluster;

    /* this returns the short entry template for the remaining entries */
    rc = add_dir_entry(fat_bpb, newdir, &dummyfile, ".", &newent,
                       DIRENTRY_DIR | DIRENTRY_RETURN_SHORT);
    if (rc < 0)
        FAT_ERR(rc * 10 - 5);

    /* and the ".." entry */
    memset(&dummyfile, 0, sizeof (struct fat_file));

    /* the root cluster is cluster 0 in the ".." entry */
    if (dir->file.firstcluster != fat_bpb->bpb_rootclus)
        dummyfile.firstcluster = dir->file.firstcluster;

    rc = add_dir_entry(fat_bpb, newdir, &dummyfile, "..", &newent,
                       DIRENTRY_DIR | DIRENTRY_KEEP_TIMES);
    if (rc < 0)
        FAT_ERR(rc * 10 - 6);

    /* finally, add the entry in the parent directory */
    rc = add_dir_entry(fat_bpb, dir, &newdir->file, name, &newent,
                       DIRENTRY_DIR | DIRENTRY_KEEP_TIMES);
    if (rc < 0)
        FAT_ERR(rc * 10 - 7);

    rc = cache_commit(fat_bpb);
    if (rc < 0)
        FAT_ERR(rc * 10 - 8);

    rc = 0;
fat_error:
    /* if no good, try to free this */
    if (rc < 0 && firstcluster)
    {
        fat_release_sector_buffer(newdir->cache);
        newdir->cache = NULL;
        update_fat_entry(fat_bpb, firstcluster, 0);
    }

    dir_unlock();
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_seek(struct fat_file *file, unsigned long seeksector)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(file->volume);

    if (!fat_bpb)
        return -1;

    int rc;
    long cluster = file->firstcluster;
    unsigned long sector = 0;
    long clusternum = 0;
    unsigned long sectornum = -1;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16 && cluster < 0) /* FAT16 root dir */
        seeksector += fat_bpb->rootdirsectornum;
#endif

    file->eof = false;
    if (seeksector)
    {
        /* we need to find the sector BEFORE the requested, since
           the file struct stores the last accessed sector */
        seeksector--;
        clusternum = seeksector / fat_bpb->bpb_secperclus;
        sectornum = seeksector % fat_bpb->bpb_secperclus;

        long numclusters = clusternum;

        if (file->clusternum && clusternum >= file->clusternum)
        {
            /* seek forward from current position */
            cluster = file->lastcluster;
            numclusters -= file->clusternum;
        }

        for (long i = 0; i < numclusters; i++)
        {
            cluster = get_next_cluster(fat_bpb, cluster);

            if (!cluster)
            {
                DEBUGF("Seeking beyond the end of the file! "
                       "(sector %ld, cluster %ld)\n", seeksector, i);
                FAT_ERR(-2);
            }
        }

        sector = cluster2sec(fat_bpb, cluster) + sectornum;
    }

    LDEBUGF("fat_seek(%lx, %lx) == %lx, %lx, %lx\n",
            file->firstcluster, seeksector, cluster, sector, sectornum);

    file->lastcluster = cluster;
    file->lastsector  = sector;
    file->clusternum  = clusternum;
    file->sectornum   = sectornum;

    rc = 0;
fat_error:
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_truncate(const struct fat_file *file)
{
    LDEBUGF("fat_truncate(%lx, %lx)\n", file->firstcluster, last);

    struct bpb * const fat_bpb = FAT_BPB_READ(file->volume);

    if (!fat_bpb)
        return -1;

    int rc;
    long last = file->lastcluster;
    long next = get_next_cluster(fat_bpb, last);

    /* truncate trailing clusters */

    /* reset eof */
    if (last)
        update_fat_entry(fat_bpb, last, FAT_EOF_MARK);

    /* free remaining cluster chain */
    for (last = next; last; last = next)
    {
        next = get_next_cluster(fat_bpb, last);
        rc = update_fat_entry(fat_bpb, last, 0);
        if (rc < 0)
            FAT_ERR(rc * 10 - 2);
    }

    rc = 0;
fat_error:
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_closewrite(struct fat_file *file, unsigned long size)
{
    LDEBUGF("fat_closewrite(size=%ld)\n", size);
    struct bpb * const fat_bpb = FAT_BPB_READ(file->volume);

    if (!fat_bpb)
        return -1;

    int rc;

    if (!size)
    {
        /* empty file */
        if (file->firstcluster)
        {
            rc = update_fat_entry(fat_bpb, file->firstcluster, 0);
            if (rc < 0)
                FAT_ERR(rc * 10 - 2);

            file->firstcluster = 0;
        }
    }

    if (file->dircluster)
    {
        dir_lock();
        rc = update_short_entry(fat_bpb, file, size);
        dir_unlock();

        if (rc < 0)
            FAT_ERR(rc * 10 - 3);
    }

    rc = cache_commit(fat_bpb);
    if (rc < 0)
        FAT_ERR(rc * 10 - 4);

#ifdef TEST_FAT
    if (file->firstcluster)
    {
        unsigned long count = 0;
        for (long next = file->firstcluster; next;
             next = get_next_cluster(fat_bpb, next))
        {
            LDEBUGF("cluster %ld: %lx\n", count, next);
            count++;
        }

        unsigned long len = count * fat_bpb->bpb_secperclus * SECTOR_SIZE;
        LDEBUGF("File is %lu clusters (chainlen=%lu, size=%lu)\n",
                count, len, size );

        if (len > size + fat_bpb->bpb_secperclus * SECTOR_SIZE)
            panicf("Cluster chain is too long\n");

        if (len < size)
            panicf("Cluster chain is too short\n");
    }
#endif /* TEST_FAT */

    rc = 0;
fat_error:
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_remove(struct fat_file *file)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(file->volume);

    if (!fat_bpb)
        return -1;

    int rc;
    dir_lock();

    /* free everything in the parent directory */
    if (file->dircluster)
    {
        rc = free_direntries(fat_bpb, file);
        if (rc < 0)
            FAT_ERR(rc * 10 - 3);
    }

    /* mark all clusters in the chain as free */
    long last = file->firstcluster;
    LDEBUGF("fat_remove(%lx)\n", last);

    while (last)
    {
        long next = get_next_cluster(fat_bpb, last);
        rc = update_fat_entry(fat_bpb, last, 0);
        if (rc < 0)
            FAT_ERR(rc * 10 - 2);
        last = next;
    }

    file->firstcluster = 0;
    file->dircluster = 0;

    rc = cache_commit(fat_bpb);
    if (rc < 0)
        FAT_ERR(rc * 10 - 4);

    rc = 0;
fat_error:
    dir_unlock();
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_rename(struct fat_file *file, struct fat_dir *dir,
               const unsigned char *newname)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(file->volume);

    if (!fat_bpb)
        return -1;

    int rc;
    union fat_direntry_raw *ent_p, ent;
    struct fat_file olddir_file;

    dir_lock();

#ifdef HAVE_MULTIVOLUME
    if (file->volume != dir->file.volume)
    {
        DEBUGF("No rename across volumes!\n");
        FAT_ERR(-2);
    }
#endif

    if (!file->dircluster)
    {
        DEBUGF("File has no dir cluster!\n");
        FAT_ERR(-3);
    }

    /* fetch a copy of the existing short entry */
    fat_open_internal(IF_MV(file->volume,) file->dircluster,
                      &olddir_file, NULL);

    cache_lock();
    ent_p = cache_direntry(fat_bpb, &olddir_file, file->direntry);
    if (!ent_p)
    {
        cache_unlock();
        FAT_ERR(-4);
    }

    ent = *ent_p;
    cache_unlock();

    struct fat_file newfile = *file;

    /* create new name; short entry info is copied from ent */
    rc = add_dir_entry(fat_bpb, dir, &newfile, newname, &ent,
                       DIRENTRY_KEEP_TIMES);
    if (rc < 0)
        FAT_ERR(rc * 10 - 5);

    /* remove old name */
    rc = free_direntries(fat_bpb, file);
    if (rc < 0)
        FAT_ERR(rc * 10 - 6);

    /* if renaming a directory, update the .. entry to make sure
       it points to its parent directory (we don't check if it was a move) */
    if (ent.attr & FAT_ATTR_DIRECTORY)
    {
        /* open the dir that was renamed */
        fat_open_internal(IF_MV(file->volume,) newfile.firstcluster,
                          &olddir_file, NULL);

        /* obtain dot-dot directory entry */
        cache_lock();
        ent_p = cache_direntry(fat_bpb, &olddir_file, 1);
        if (!ent_p)
        {
            cache_unlock();
            FAT_ERR(-7);
        }

        if (strncmp("..         ", ent_p->name, 11))
        {
            /* .. entry must be second entry according to FAT spec (p.29) */
            DEBUGF("Second dir entry is not double-dot!\n");
            cache_unlock();
            FAT_ERR(-8);
        }

        /* parent cluster is 0 if parent dir is the root - FAT spec (p.29) */
        long parentcluster = 0;
        if (dir->file.firstcluster != fat_bpb->bpb_rootclus)
            parentcluster = dir->file.firstcluster;
        rawent_set_fstclus(ent_p, parentcluster);

        cache_dirty_buf(ent_p);
        cache_unlock();
    }

    /* write back changes */
    rc = cache_commit(fat_bpb);
    if (rc < 0)
        FAT_ERR(rc * 10 - 9);

    rc = 0;
fat_error:
    dir_unlock();
    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

long fat_readwrite(struct fat_file *file, unsigned long sectorcount,
                   void *buf, bool write)
{
    bool eof = file->eof;

    if ((eof && !write) || !sectorcount)
        return 0;

    long rc;
    struct bpb * const fat_bpb = FAT_BPB_READ(file->volume);

    if (!fat_bpb)
        return -1;

    long cluster = file->lastcluster;
    unsigned long sector = file->lastsector;
    long clusternum = file->clusternum;
    unsigned long sectornum = file->sectornum;

    LDEBUGF("fat_readwrite(file:%lx,count:0x%lx,buf:%lx,%s)\n",
            file->firstcluster, sectorcount, (long)buf,
            write ? "write":"read");
    LDEBUGF("fat_readwrite: sec=%lx numsec=%ld eof=%d\n",
            sector, (long)sectornum, eof ? 1 : 0);

    eof = false;

    unsigned long transferred = 0;
    unsigned long count = 0;
    unsigned long last = 0;

    do
    {
        if (++sectornum >= fat_bpb->bpb_secperclus || !cluster)
        {
            /* out of sectors in this cluster; get the next cluster */
            long newcluster = write ? next_write_cluster(fat_bpb, file, cluster) :
                                      get_next_cluster(fat_bpb, cluster);

            if (newcluster)
            {
                cluster = newcluster;
                sector = cluster2sec(fat_bpb, newcluster);
                clusternum++;
                sectornum = 0;
            }
            else
            {
                sectornum--; /* remain in previous position */
                eof = true;
                break;
            }
        }
        else if (!sector++)
        {
            /* look up first sector of file */
            sector = cluster2sec(fat_bpb, file->firstcluster);
            sectornum = 0;
        #ifdef HAVE_FAT16SUPPORT
            if (fat_bpb->is_fat16 && file->firstcluster < 0)
            {
                sector += fat_bpb->rootdirsectornum;
                sectornum = fat_bpb->rootdirsectornum;
            }
        #endif
        }

        unsigned long dump = 0;

        /* find sequential sectors and transfer them all at once */
        if (count != 0 && sector != last + 1)
            dump = count; /* not sequential */
        else if (++count > FAT_MAX_TRANSFER_SIZE)
            dump = count - 1;

        if (dump)
        {
            rc = transfer(fat_bpb, last - dump + 1, dump, buf, write);
            if (rc < 0)
                FAT_ERR(rc * 10 - 3);

            buf += dump * SECTOR_SIZE;
            transferred += dump;
            count = 1;
        }

        last = sector;
    }
    while (transferred + count < sectorcount);

    if (count)
    {
        /* transfer any remainder */
        rc = transfer(fat_bpb, last - count + 1, count, buf, write);
        if (rc < 0)
            FAT_ERR(rc * 10 - 4);

        transferred += count;
    }

    rc = (eof && write) ? -5 : (long)transferred;
fat_error:
    file->lastcluster = cluster;
    file->lastsector  = sector;
    file->clusternum  = clusternum;
    file->sectornum   = sectornum;
    file->eof         = eof;

    if (rc >= 0)
        DEBUGF("Sectors written: %ld\n", rc);

    FAT_BPB_READ_DONE(fat_bpb);
    return rc;
}

int fat_opendir(IF_MV(int volume,) struct fat_dir *dir, long startcluster,
                const struct fat_dir *parent_dir)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (!fat_bpb)
        return -1;

    if (startcluster == 0)
        startcluster = fat_bpb->bpb_rootclus;

    fat_open_internal(IF_MV(volume,) startcluster, &dir->file, parent_dir);

    /* when dir == parent_dir, then it closes parent_dir to open dir */
    void *cache = (dir == parent_dir) ? dir->cache : fat_get_sector_buffer();

    if (!cache)
    {
        FAT_BPB_READ_DONE(fat_bpb);
        return -2;
    }

    dir->entry = 0;
    dir->entrycount = 0; 
    dir->cache = cache;
    dir->dirty = true;

    FAT_BPB_READ_DONE(fat_bpb);
    return 0;
}

int fat_closedir(struct fat_dir *dir)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(dir->file.volume);

    if (!fat_bpb)
        return -1;

    fat_release_sector_buffer(dir->cache);
    dir->cache = NULL;

    FAT_BPB_READ_DONE(fat_bpb);
    return 0;
}

int fat_getnext(struct fat_dir *dir, struct fat_direntry *entry)
{
    int rc;

    if (!dir->cache)
        return -1;

    dir->entrycount = 0;

    if (dir->dirty)
    {
        rc = fat_seek(&dir->file, dir->entry / DIR_ENTRIES_PER_SECTOR);
        if (rc < 0)
            FAT_ERR(rc * 10 - 2);
    }

    /* Long file names are stored in special entries. Each entry holds up to
       13 UTF-16 characters. Thus, UTF-8 converted names can be max 255 chars
       (1020 bytes) long, not including the trailing \0. */
    struct longname_parse lnparse;
    parse_longname_start(&lnparse);

    while (1)
    {
        unsigned int index = dir->entry % DIR_ENTRIES_PER_SECTOR;

        if (index == 0 || dir->dirty)
        {
            rc = fat_readwrite(&dir->file, 1, dir->cache, false);

            if (rc < 0)
            {
                DEBUGF( "fat_getnext() - Couldn't read dir"
                        " (error code %d)\n", rc);
                FAT_ERR(rc * 10 - 3);
            }

            dir->dirty = false;

            if (rc == 0)
            {
                /* eof */
                entry->name[0] = 0;
                entry->shortname[0] = 0;
                break;
            }
        }

        union fat_direntry_raw *ent =
            &((union fat_direntry_raw *)dir->cache)[index];

        dir->entry++;

        if (ent->name[0] == 0)
        {
            /* last entry */
            entry->name[0] = 0;
            entry->shortname[0] = 0;
            dir->entrycount = 0;
            rc = 0;
            break;
        }

        if (ent->name[0] == 0xe5)
        {
            /* free entry */
            dir->entrycount = 0;
            continue;
        }

        dir->entrycount++;

        if ((ent->ldir_attr & FAT_ATTR_LONG_NAME_MASK) == FAT_ATTR_LONG_NAME)
        {
            /* LFN entry */
            parse_longname_entry(entry, &lnparse, ent);
        }
        else if ((ent->attr & (FAT_ATTR_VOLUME_ID | FAT_ATTR_DIRECTORY))
                 != FAT_ATTR_VOLUME_ID) /* don't return volume id entry */
        {
            parse_short_direntry(entry, ent);

            if (!parse_longname_finish(entry, &lnparse, ent))
            {
                /* Take the short DOS name. Need to utf8-encode it
                   since it may contain chars from the upper half of
                   the OEM code page which wouldn't be a valid utf8.
                   Beware: this file will be shown with strange
                   glyphs in file browser since unicode 0x80 to 0x9F
                   are control characters. */
                logf("SN-DOS: %s", entry->shortname);
                iso_decode(entry->shortname, entry->name, -1,
                           strlen(entry->shortname) + 1);
                logf("SN: %s", entry->name);
            }
            else
            {
                logf("LN: %s", entry->name);
            }

            rc = 1;
            break;
        }
    }

fat_error:
    return rc;
}

unsigned int fat_get_cluster_size(IF_MV_NONVOID(int volume))
{
    unsigned int size = 0;

    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (fat_bpb)
    {
        size = fat_bpb->bpb_secperclus * SECTOR_SIZE;
        FAT_BPB_READ_DONE(fat_bpb);
    }

    return size;
}

bool fat_ismounted(IF_MV_NONVOID(int volume))
{
    bool mounted = false;

    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (fat_bpb)
    {
        mounted = fat_bpb->mounted;
        FAT_BPB_READ_DONE(fat_bpb);
    }

    return mounted;
}

void fat_size(IF_MV(int volume,) unsigned long *size, unsigned long *free)
{
    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (fat_bpb)
    {
        if (size)
            *size = fat_bpb->dataclusters *
                    (fat_bpb->bpb_secperclus * SECTOR_SIZE / 1024);

        if (free)
            *free = fat_bpb->fsinfo.freecount *
                    (fat_bpb->bpb_secperclus * SECTOR_SIZE / 1024);

        FAT_BPB_READ_DONE(fat_bpb);
    }
    else
    {
        if (size)
            *size = 0;

        if (free)
            *free = 0;
    }
}

#ifdef MAX_LOG_SECTOR_SIZE
int fat_get_bytes_per_sector(IF_MV_NONVOID(int volume))
{
    int bytes = 0;

    struct bpb * const fat_bpb = FAT_BPB_READ(volume);

    if (fat_bpb)
    {
        bytes = fat_bpb->bpb_bytspersec;
        FAT_BPB_READ_DONE(volume);
    }

    return bytes;
}
#endif /* MAX_LOG_SECTOR_SIZE */

void * fat_get_sector_buffer(void)
{
    cache_lock();
    void *buf = cache_get_sector_buf(NULL, 0);
    cache_unlock();
    return buf;
}

void fat_release_sector_buffer(void *buf)
{
    cache_lock();
    cache_release_sector_buf(buf);
    cache_unlock();
}

void fat_init(void)
{
    static bool initialized = false;

    if (!initialized)
    {
        initialized = true;

        for (unsigned int i = 0; i < NUM_VOLUMES; i++)
        {
            mrsw_init(&fat_mrsw[i], 0);
#if 0
            fat_bpbs[i].mrsw = &fat_mrsw[i];
#endif
        }

        mutex_init(&dir_mutex);
        cache_init(true);
    }

    /* acquire all writer locks in volume order */
    for (unsigned int i = 0; i < NUM_VOLUMES; i++)
        mrsw_write_acquire(&fat_bpb->mrsw);

    /* mark the FAT cache as unused */
    cache_init(false);

    /* mark the possible volumes as not mounted */
    for (unsigned int i = 0; i < NUM_VOLUMES; i++)
    {
        fat_bpbs[i].mounted = false;
        mrsw_write_release(fat_bpbs[i].mrsw);
    }
}
