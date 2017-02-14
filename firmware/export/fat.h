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
#ifndef FAT_H
#define FAT_H

#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include "config.h"
#include "system.h"

/********
 **** DO NOT use these functions directly unless otherwise noted. Required
 **** synchronization is done by higher-level interfaces to minimize locking
 **** overhead.
 ****
 **** Volume, drive, string, etc. parameters should also be checked by
 **** callers for gross violations-- NULL strings, out-of-bounds values,
 **** etc.
 ****/

/****************************************************************************
 ** Values that can be overridden by a target in config-[target].h
 **/

/* if your ATA implementation can do better, go right ahead and increase this
 * value */
#ifndef FAT_MAX_TRANSFER_SIZE
#define FAT_MAX_TRANSFER_SIZE 256
#endif

/**
 ****************************************************************************/

/* Note: This struct doesn't hold the raw values after mounting if
 * bpb_bytspersec isn't 512. All sector counts are normalized to 512 byte
 * physical sectors. */
#ifdef HAVE_FAT16SUPPORT
#define IF_FAT16(...) __VA_ARGS__
#else
#define IF_FAT16(...)
#endif

struct fsinfo
{
    signed long freecount; /* last known free cluster count */
    signed long nextfree;  /* first cluster to start looking for free
                              clusters, or -1 for no hint */
    bool        dirty;     /* true if it needs writeback */
};

/* variables for internal use */
#define __struct_bpb_DRV_FAT \
    struct {                                                          \
    unsigned long secperclus;       /* sectors in each cluster */     \
    unsigned long clussize;         /* total size of a cluster */     \
    unsigned long fatsz;            /* number of FAT sectors */       \
    unsigned int  numfats;          /* number of FAT structures */    \
    unsigned long fatrgnstart;      /* first sector of FAT 0 */       \
    unsigned long firstdatasector;  /* sector of first data block */  \
    unsigned long firstallowed;     /* first writeable sector */      \
    unsigned long totsec;           /* sectors used by this volume */ \
    unsigned long dataclusters;     /* total data clusters in vol */  \
    signed   long rootclus;         /* negative if not FAT32 */       \
    union {                                                           \
    unsigned long fsinfosec;        /* fsinfo sector - FAT32 only */  \
    unsigned long rootdirsectornum; /* root sector   - FAT16 only */  \
    };                                                                \
    struct fsinfo fsinfo;           /* 16/32: kept, 32: persisted */  \
    IF_FAT16(                                                         \
    const struct  fat_ops *ops; /* functions specific to FAT type */  \
    ) /* IF_FAT16 */                                                  \
    }

#define FAT_DIRSCAN_RW_VAL (0u - 1u) /* before entry 0 */

#define __struct_DRV_FAT(...) __VA_ARGS__

#define __struct_dirscan_DRV_FAT \
    struct {                                                          \
    unsigned int direntry;      /* current dir index cursor */        \
    }

#define __struct_idx_info_DRV_FAT \
    struct {                                                          \
    unsigned int direntry;      /* short entry in parent */           \
    unsigned int numentries;    /* number of entries used */          \
    }

#define __struct_stat_DRV_FAT \
    struct {                                                          \
    uint8_t  attr;              /* file attributes */                 \
    uint8_t  crttimetenth;      /* ms creation time stamp (0-199) */  \
    uint16_t crttime;           /* creation time */                   \
    uint16_t crtdate;           /* creation date */                   \
    uint16_t lstaccdate;        /* last access date */                \
    uint16_t wrttime;           /* last write time */                 \
    uint16_t wrtdate;           /* last write date */                 \
    uint32_t filesize;          /* file size in bytes */              \
    int32_t  firstcluster;      /* file's first cluster, 0=empty */   \
    }

#define __struct_fileentry_DRV_FAT \
    struct {                                                          \
    uint16_t ucssegs[20][13];   /* UTF-16 segment buffer */           \
    uint16_t ucsterm;           /* NULL-term after ucssegs */         \
    uint8_t  shortname[13];     /* DOS filename (OEM charset) */      \
    __struct_stat_DRV_FAT;      /* numeric file entry stats */        \
    __struct_idx_info_DRV_FAT;  /* directory index info */            \
    }

/* basic FAT file information about where to find a file and who houses it */
#define __struct_fileinfo_DRV_FAT \
    struct {                                                          \
    signed   long firstcluster; /* first cluster in file */           \
    signed   long dircluster;   /* first cluster of parent */         \
    __struct_idx_info_DRV_FAT;  /* directory index info */            \
    }

/* this stores what was last accessed when read or writing a file's data */
#define __struct_filestr_DRV_FAT \
    struct {                                                          \
    signed   long lastcluster; /* cluster of last access */           \
    unsigned long lastsector;  /* sector of last access */            \
    signed   long clusternum;  /* cluster number of last access */    \
    unsigned long sectornum;   /* sector index in current cluster */  \
    bool          eof;         /* end-of-file reached */              \
    struct iobuf_handle fatiob; /* side handle for FAT sectors */     \
    }

struct bpb;
struct fileentry;
struct fileinfo;
struct filestr;

#ifdef __FILESYS_STRUCTS_DRV
/* complete the structure definitions in flattened, reduced form */
FS_DRV_STRUCT_COMPLETE(bpb, FAT);

/* the following use struct iobuf_handle */
#include "disk_cache.h"

FS_DRV_STRUCT_COMPLETE(fileentry, FAT);
FS_DRV_STRUCT_COMPLETE(fileinfo, FAT);
FS_DRV_STRUCT_COMPLETE(filestr, FAT);
FS_DRV_STRUCT_COMPLETE(dirscan, FAT);
#endif /* __FILESYS_STRUCTS_DRV */

/** File Ops **/

/* file */
enum cmpfi_result fat_fileop_compare_fileinfo(const struct fileinfo *info1,
                                              const struct fileinfo *info2);

int fat_fileop_open_volume(struct bpb *bpb,
                           struct fileinfo *info);

int fat_fileop_create(struct fileinfo *dirinfo,
                      unsigned int callflags,
                      struct fileentry *entry);

int fat_fileop_open(struct fileinfo *dirinfo,
                    struct fileentry *entry,
                    struct fileinfo *info);

int fat_fileop_close(struct fileinfo *info);

int fat_fileop_remove(struct fileinfo *info);

int fat_fileop_sync(struct fileinfo *info,
                    struct fileentry *entry);

int fat_fileop_rename(struct fileinfo *dirinfo,
                      struct fileinfo *info,
                      struct fileentry *entry);

int fat_fileop_open_filestr(struct fileinfo *info,
                            struct filestr *str);

static inline int fat_fileop_close_filestr(struct filestr *str)
    { return 0; (void)str; }

int fat_fileop_ftruncate(struct filestr *str,
                         file_size_t truncsize);

off_t fat_fileop_lseek(struct filestr *str,
                       off_t offset,
                       int whence);

ssize_t fat_fileop_readwrite(struct filestr *str,
                             void *buf,
                             size_t nbyte,
                             bool write);

/* directory */
int fat_fileop_compare_name(struct fileinfo *dirinfo,
                            const char *name1,
                            const char *name2,
                            size_t maxlen);

int fat_fileop_get_fileentry(struct fileinfo *dirinfo,
                             const char *name,
                             size_t length,
                             struct fileentry *entry);

int fat_fileop_readdir(struct filestr *dirstr,
                       struct dirscan *scan,
                       struct fileentry *entry);

int fat_fileop_rewinddir(struct filestr *dirstr,
                         struct dirscan *scan);

int fat_fileop_convert_entry(union direntry_cvt_t dst,
                             union direntry_cvt_t src,
                             unsigned int cvtflags);

/** Disk Ops **/
int fat_diskop_mount(struct bpb *bpb);
int fat_diskop_unmount(struct bpb *bpb);

/** Volume Ops **/
size_t fat_volop_cluster_size(struct bpb *bpb);

#ifdef MAX_LOG_SECTOR_SIZE
size_t fat_volop_get_bytes_per_sector(struct bpb *bpb);
#endif

void fat_volop_recalc_free(struct bpb *bpb);

void fat_volop_size(struct bpb *bpb,
                    unsigned long *size,
                    unsigned long *free);

void fat_volop_flush(struct bpb *bpb);

/** Misc. **/
time_t fattime_mktime(uint16_t fatdate,
                      uint16_t fattime);

#endif /* FAT_H */
