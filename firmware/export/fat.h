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
#include "mv.h" /* for volume definitions */
#include "config.h"
#include "system.h"

/****************************************************************************
 ** Values that can be overridden by a target in config-[target].h
 **/

/* If your ATA implementation can do better, go right ahead and increase
 * this value. */
#ifndef FAT_MAX_TRANSFER_SIZE
#define FAT_MAX_TRANSFER_SIZE 256
#endif

/* still experimental? */
#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512
#endif

/**
 ****************************************************************************/

/* Number of bytes reserved for a file name (including the trailing \0).
   Since names are stored in the entry as UTF-8, we won't be able to
   store all names allowed by FAT. In FAT, a name can have max 255
   characters (not bytes!). Since the UTF-8 encoding of a char may take
   up to 4 bytes, there will be names that we won't be able to store
   completely. For such names, the short DOS name is used. */
struct fat_direntry
{
    unsigned char name[256];        /* UTF-8 encoded name plus \0 */
    unsigned char shortname[13];    /* DOS filename (OEM charset) */
    unsigned short attr;            /* Attributes */
    unsigned char crttimetenth;     /* Millisecond creation
                                       time stamp (0-199) */
    unsigned short crttime;         /* Creation time */
    unsigned short crtdate;         /* Creation date */
    unsigned short lstaccdate;      /* Last access date */
    unsigned short wrttime;         /* Last write time */
    unsigned short wrtdate;         /* Last write date */
    unsigned long filesize;         /* File size in bytes */
    long firstcluster;              /* fstclusterhi<<16 + fstcluslo */
};

#define FAT_ATTR_READ_ONLY   0x01
#define FAT_ATTR_HIDDEN      0x02
#define FAT_ATTR_SYSTEM      0x04
#define FAT_ATTR_VOLUME_ID   0x08
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_ARCHIVE     0x20
#define FAT_ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

struct fat_file
{
    long          firstcluster;/* first cluster in file */
    long          lastcluster; /* cluster of last access */
    unsigned long lastsector;  /* sector of last access */
    long          clusternum;  /* cluster number of last access */
    unsigned long sectornum;   /* sector number within current cluster */
    unsigned int  direntry;    /* short dir entry index from start of dir */
    unsigned int  direntries;  /* number of dir entries used by this file */
    long          dircluster;  /* first cluster of dir */
    bool eof;
#ifdef HAVE_MULTIVOLUME
    int volume;          /* file resides on which volume */
#endif
};

struct fat_dir
{
    struct fat_file file;
    unsigned int entry;
    unsigned int entrycount;
    void *cache;
    bool dirty;
};

void fat_init(void);
int fat_get_bytes_per_sector(IF_MV_NONVOID(int volume));
int fat_mount(IF_MV(int volume,) IF_MD(int drive,) long startsector);
int fat_unmount(IF_MV(int volume,) bool flush);
void fat_size(IF_MV(int volume,) /* public for info */
              unsigned long *size,
              unsigned long *free);
int fat_create_dir(const char *name,
                   struct fat_dir *newdir,
                   struct fat_dir *dir);
int fat_open(IF_MV(int volume,)
             long cluster,
             struct fat_file *ent,
             const struct fat_dir *dir);
int fat_create_file(const char *name,
                    struct fat_file *ent,
                    struct fat_dir *dir);
long fat_readwrite(struct fat_file *ent,
                   unsigned long sectorcount,
                   void *buf, bool write );
int fat_closewrite(struct fat_file *ent, unsigned long size);
int fat_seek(struct fat_file *ent, unsigned long sector);
int fat_remove(struct fat_file *ent);
int fat_truncate(const struct fat_file *ent);
int fat_rename(struct fat_file *file,
               struct fat_dir *dir,
               const unsigned char *newname);

int fat_opendir(IF_MV(int volume,)
                struct fat_dir *ent,
                long startcluster,
                const struct fat_dir *parent_dir);
int fat_closedir(struct fat_dir *dir);
int fat_getnext(struct fat_dir *ent, struct fat_direntry *entry);
bool fat_ismounted(IF_MV_NONVOID(int volume));

/* grab a buffer from the cache as a temp */
void * fat_get_sector_buffer(void);
void fat_release_sector_buffer(void *buf);

/* debug screen stuff */
void fat_recalc_free(IF_MV_NONVOID(int volume));
unsigned int fat_get_cluster_size(IF_MV_NONVOID(int volume));

#endif /* FAT_H */
