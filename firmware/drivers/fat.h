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

#define BLOCK_SIZE 512

#define FATTYPE_FAT12       0
#define FATTYPE_FAT16       1
#define FATTYPE_FAT32       2

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

#define BS_DRVNUM           36
#define BS_RESERVED1        37
#define BS_BOOTSIG          38
#define BS_VOLID            39
#define BS_VOLLAB           43
#define BS_FILSYSTYPE       54

#define BPB_FATSZ32          36

#define BPB_LAST_WORD       510

#define MIN(a,b) (((a) < (b))?(a):(b))

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
    int fat_type;       /* FAT12, FAT16 or FAT32 */
    int last_word;      /* must be 0xAA55 */
};

#define FAT_ATTR_READ_ONLY   0x01
#define FAT_ATTR_HIDDEN      0x02
#define FAT_ATTR_SYSTEM      0x04
#define FAT_ATTR_VOLUME_ID   0x08
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_ARCHIVE     0x20
#define FAT_ATTR_LONG_NAME   (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                              FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)
#define FAT_ATTR_LONG_NAME_MASK (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | \
                                 FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | \
                                 FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE )


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

struct fat_direntry
{
        unsigned char name[12];         /* Name plus \0 */
        unsigned short attr;            /* Attributes */
        unsigned char crttimetenth;     /* Millisecond creation
                                           time stamp (0-199) */
        unsigned short crttime;         /* Creation time */
        unsigned short crtdate;         /* Creation date */
        unsigned short lstaccdate;      /* Last access date */
        unsigned short fstclushi;       /* High word of first cluster
                                           (0 for FAT12/16) */
        unsigned short wrttime;         /* Last write time */
        unsigned short wrtdate;         /* Last write date */
        unsigned short fstcluslo;       /* Low word of first cluster */
        unsigned int filesize;          /* File size in bytes */
};

struct fat_context
{
        unsigned int curr_dir_sec; /* Current directory sector */
        
};

struct disk_info
{
        int num_sectors;
        int sec_per_track;
        int num_heads;
        unsigned int hidden_sectors;
};

struct fat_dirent
{
        int entry;
        unsigned int cached_sec;
        unsigned int num_sec;
        char cached_buf[BLOCK_SIZE];
};

extern int fat_create_file(struct bpb *bpb, 
                           unsigned int currdir, 
                           char *name);
extern int fat_opendir(struct bpb *bpb, 
                       struct fat_dirent *ent, 
                       unsigned int currdir);
extern int fat_getnext(struct bpb *bpb, 
                       struct fat_dirent *ent,
                       struct fat_direntry *entry);

#endif
