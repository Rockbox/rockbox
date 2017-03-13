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
#include "mv.h" /* for volume definitions */

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

#define INVALID_SECNUM     (0xfffffffeul) /* sequential, not FAT */
#define FAT_MAX_FILE_SIZE  (0xfffffffful) /* 2^32-1 bytes */
#define MAX_DIRENTRIES     65536
#define MAX_DIRECTORY_SIZE (MAX_DIRENTRIES*32) /* 2MB max size */

/* these aren't I/O error conditions, so define specially; failure rc's
 * shouldn't return the last digit as "0", therefore this is unambiguous */
#define FAT_RC_ENOSPC (-10)
#define FAT_SEEK_EOF  (-20)

/* Number of bytes reserved for a file name (including the trailing \0).
   Since names are stored in the entry as UTF-8, we won't be ble to
   store all names allowed by FAT. In FAT, a name can have max 255
   characters (not bytes!). Since the UTF-8 encoding of a char may take
   up to 4 bytes, there will be names that we won't be able to store
   completely. For such names, the short DOS name is used. */
#define FAT_DIRENTRY_NAME_MAX 255
struct fat_direntry
{
    union {
    uint8_t  name[255+1+4];     /* UTF-8 name plus \0 plus parse slop */
    uint16_t ucssegs[5+20][13]; /* UTF-16 segment buffer - layout saves... */
    };                          /* ...130 bytes (important if stacked) */
    uint16_t ucsterm;           /* allow one NULL-term after ucssegs */
    uint8_t  shortname[13];     /* DOS filename (OEM charset) */
    uint8_t  attr;              /* file attributes */
    uint8_t  crttimetenth;      /* millisecond creation time stamp (0-199) */
    uint16_t crttime;           /* creation time */
    uint16_t crtdate;           /* creation date */
    uint16_t lstaccdate;        /* last access date */
    uint16_t wrttime;           /* last write time */
    uint16_t wrtdate;           /* last write date */
    uint32_t filesize;          /* file size in bytes */
    int32_t  firstcluster;      /* first FAT cluster of file, 0 if empty */
};

/* cursor structure used for scanning directories; holds the last-returned
   entry information */
#define FAT_DIRSCAN_RW_VAL (0u - 1u)

struct fat_dirscan_info
{
    unsigned int entry;         /* short dir entry index in parent */
    unsigned int entries;       /* number of dir entries used */
};

#define FAT_FILE_RW_VAL  (0ul - 1ul)

/* basic FAT file information about where to find a file and who houses it */
struct fat_file
{
#ifdef HAVE_MULTIVOLUME
    int    volume;              /* file resides on which volume (first!) */
#endif
    long   firstcluster;        /* first cluster in file */
    long   dircluster;          /* first cluster of parent directory */
    struct fat_dirscan_info e;  /* entry information */
};

/* this stores what was last accessed when read or writing a file's data */
struct fat_filestr
{
    struct fat_file *fatfilep;  /* common file information */
    long          lastcluster;  /* cluster of last access */
    unsigned long lastsector;   /* sector of last access */
    long          clusternum;   /* cluster number of last access */
    unsigned long sectornum;    /* sector number within current cluster */
    bool          eof;          /* end-of-file reached */
};

/** File entity functions **/
int fat_create_file(struct fat_file *parent, const char *name,
                    uint8_t attr, struct fat_file *file,
                    struct fat_direntry *fatent);
bool fat_dir_is_parent(const struct fat_file *dir, const struct fat_file *file);
bool fat_file_is_same(const struct fat_file *file1, const struct fat_file *file2);
int fat_fstat(struct fat_file *file, struct fat_direntry *entry);
int fat_open(const struct fat_file *parent, long startcluster,
             struct fat_file *file);
int fat_open_rootdir(IF_MV(int volume,) struct fat_file *dir);
enum fat_remove_op           /* what should fat_remove(), remove? */
{
    FAT_RM_DIRENTRIES = 0x1, /* remove only directory entries */
    FAT_RM_DATA       = 0x2, /* remove only file data */
    FAT_RM_ALL        = 0x3, /* remove all of above */
};
int fat_remove(struct fat_file *file, enum fat_remove_op what);
int fat_rename(struct fat_file *parent, struct fat_file *file,
               const unsigned char *newname);

/** File stream functions **/
int fat_closewrite(struct fat_filestr *filestr, uint32_t size,
                   struct fat_direntry *fatentp);
void fat_filestr_init(struct fat_filestr *filestr, struct fat_file *file);
unsigned long fat_query_sectornum(const struct fat_filestr *filestr);
long fat_readwrite(struct fat_filestr *filestr, unsigned long sectorcount,
                   void *buf, bool write);
void fat_rewind(struct fat_filestr *filestr);
int fat_seek(struct fat_filestr *filestr, unsigned long sector);
void fat_seek_to_stream(struct fat_filestr *filestr,
                        const struct fat_filestr *filestr_seek_to);
int fat_truncate(const struct fat_filestr *filestr);

/** Directory stream functions **/
struct filestr_cache;
int fat_readdir(struct fat_filestr *dirstr, struct fat_dirscan_info *scan,
                struct filestr_cache *cachep, struct fat_direntry *entry);
void fat_rewinddir(struct fat_dirscan_info *scan);

/** Mounting and unmounting functions **/
bool fat_ismounted(IF_MV_NONVOID(int volume));
int fat_mount(IF_MV(int volume,) IF_MD(int drive,) unsigned long startsector);
int fat_unmount(IF_MV_NONVOID(int volume));

/** Debug screen stuff **/
#ifdef MAX_LOG_SECTOR_SIZE
int fat_get_bytes_per_sector(IF_MV_NONVOID(int volume));
#endif /* MAX_LOG_SECTOR_SIZE */
unsigned int fat_get_cluster_size(IF_MV_NONVOID(int volume));
void fat_recalc_free(IF_MV_NONVOID(int volume));
bool fat_size(IF_MV(int volume,) unsigned long *size, unsigned long *free);

/** Misc. **/
time_t fattime_mktime(uint16_t fatdate, uint16_t fattime);
void fat_empty_fat_direntry(struct fat_direntry *entry);
void fat_init(void);

#endif /* FAT_H */
