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

#define SECTOR_SIZE 512

struct bpb
{
    char bs_oemname[9];  /* OEM string, ending with \0 */
    int bpb_bytspersec;  /* Bytes per sectory, typically 512 */
    int bpb_secperclus;  /* Sectors per cluster */
    int bpb_rsvdseccnt;  /* Number of reserved sectors */
    int bpb_numfats;     /* Number of FAT structures, typically 2 */
    int bpb_rootentcnt;  /* Number of dir entries in the root */
    int bpb_totsec16;    /* Number of sectors on the volume (old 16-bit) */
    int bpb_media;       /* Media type (typically 0xf0 or 0xf8) */
    int bpb_fatsz16;     /* Number of used sectors per FAT structure */
    int bpb_secpertrk;   /* Number of sectors per track */
    int bpb_numheads;    /* Number of heads */
    int bpb_hiddsec;     /* Hidden sectors before the volume */
    unsigned int bpb_totsec32;    /* Number of sectors on the volume
                                     (new 32-bit) */
    int last_word;       /* 0xAA55 */

    /**** FAT12/16 specific *****/
    int bs_drvnum;       /* Drive number */
    int bs_bootsig;      /* Is 0x29 if the following 3 fields are valid */
    unsigned int bs_volid; /* Volume ID */
    char bs_vollab[12];    /* Volume label, 11 chars plus \0 */
    char bs_filsystype[9]; /* File system type, 8 chars plus \0 */

    /**** FAT32 specific *****/
    int bpb_fatsz32;
    int bpb_extflags;
    int bpb_fsver;
    int bpb_rootclus;
    int bpb_fsinfo;
    int bpb_bkbootsec;

    /* variables for internal use */
    int fatsize;
    int totalsectors;
    int rootdirsector;
    int firstdatasector;
};

struct fat_direntry
{
    unsigned char name[12];         /* Name plus \0 */
    unsigned short attr;            /* Attributes */
    unsigned char crttimetenth;     /* Millisecond creation
                                       time stamp (0-199) */
    unsigned short crttime;         /* Creation time */
    unsigned short crtdate;         /* Creation date */
    unsigned short lstaccdate;      /* Last access date */
    unsigned short wrttime;         /* Last write time */
    unsigned short wrtdate;         /* Last write date */
    unsigned int filesize;          /* File size in bytes */
    unsigned short fstclusterlo;
    unsigned short fstclusterhi;
    int firstcluster;               /* fstclusterhi<<16 + fstcluslo */
};

#define FAT_ATTR_READ_ONLY   0x01
#define FAT_ATTR_HIDDEN      0x02
#define FAT_ATTR_SYSTEM      0x04
#define FAT_ATTR_VOLUME_ID   0x08
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_ARCHIVE     0x20

struct fat_dirent
{
    int entry;
    unsigned int cached_sec;
    unsigned int num_sec;
    char cached_buf[SECTOR_SIZE];
};

struct fat_fileent
{
    int firstcluster;    /* first cluster in file */
    int nextcluster;     /* cluster of last access */
    int nextsector;      /* sector of last access */
    int sectornum;       /* sector number in this cluster */
};

extern int fat_create_file(struct bpb *bpb, 
                           unsigned int currdir, 
                           char *name);
extern int fat_create_dir(struct bpb *bpb, 
                          unsigned int currdir,
                          char *name);
extern int fat_open(struct bpb *bpb, 
                    unsigned int cluster,
                    struct fat_fileent *ent);
extern int fat_read(struct bpb *bpb, 
                    struct fat_fileent *ent,
                    int sectorcount,
                    void* buf );

extern int fat_opendir(struct bpb *bpb, 
                       struct fat_dirent *ent, 
                       unsigned int currdir);
extern int fat_getnext(struct bpb *bpb, 
                       struct fat_dirent *ent,
                       struct fat_direntry *entry);

#endif
