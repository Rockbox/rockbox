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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef FAT_H
#define FAT_H

#include <stdbool.h>
#include "ata.h" /* for volume definitions */

#define SECTOR_SIZE 512

struct fat_direntry
{
    unsigned char name[256];        /* Name plus \0 */
    unsigned short attr;            /* Attributes */
    unsigned char crttimetenth;     /* Millisecond creation
                                       time stamp (0-199) */
    unsigned short crttime;         /* Creation time */
    unsigned short crtdate;         /* Creation date */
    unsigned short lstaccdate;      /* Last access date */
    unsigned short wrttime;         /* Last write time */
    unsigned short wrtdate;         /* Last write date */
    unsigned int filesize;          /* File size in bytes */
    int firstcluster;               /* fstclusterhi<<16 + fstcluslo */
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
    int firstcluster;    /* first cluster in file */
    int lastcluster;     /* cluster of last access */
    int lastsector;      /* sector of last access */
    int clusternum;      /* current clusternum */
    int sectornum;       /* sector number in this cluster */
    unsigned int direntry;   /* short dir entry index from start of dir */
    unsigned int direntries; /* number of dir entries used by this file */
    int dircluster;      /* first cluster of dir */
    bool eof;
#ifdef HAVE_MULTIVOLUME
    int volume;          /* file resides on which volume */
#endif
};

struct fat_dir
{
    unsigned int entry;
    unsigned int entrycount;
    int sector;
    struct fat_file file;
    unsigned char sectorcache[3][SECTOR_SIZE];
};


extern void fat_init(void);
extern int fat_mount(IF_MV2(int volume,) IF_MV2(int drive,) int startsector);
extern void fat_size(IF_MV2(int volume,) unsigned int* size, unsigned int* free); // public for info
extern void fat_recalc_free(IF_MV_NONVOID(int volume)); // public for debug info screen
extern int fat_create_dir(const char* name,
                          struct fat_dir* newdir,
                          struct fat_dir* dir);
extern int fat_startsector(IF_MV_NONVOID(int volume)); // public for config sector
extern int fat_open(IF_MV2(int volume,)
                    int cluster,
                    struct fat_file* ent,
                    const struct fat_dir* dir);
extern int fat_create_file(const char* name,
                           struct fat_file* ent,
                           struct fat_dir* dir);
extern int fat_readwrite(struct fat_file *ent, int sectorcount, 
                         void* buf, bool write );
extern int fat_closewrite(struct fat_file *ent, int size, int attr);
extern int fat_seek(struct fat_file *ent, unsigned int sector );
extern int fat_remove(struct fat_file *ent);
extern int fat_truncate(const struct fat_file *ent);
extern int fat_rename(struct fat_file* file, 
                      struct fat_dir* dir,
                      const unsigned char* newname,
                      int size, int attr);

extern int fat_opendir(IF_MV2(int volume,)
                       struct fat_dir *ent, unsigned int currdir,
                       const struct fat_dir *parent_dir);
extern int fat_getnext(struct fat_dir *ent, struct fat_direntry *entry);
extern int fat_get_cluster_size(IF_MV_NONVOID(int volume));
extern bool fat_ismounted(int volume);

#endif
