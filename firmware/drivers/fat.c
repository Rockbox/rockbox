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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "fat.h"
#include "ata.h"
#include "debug.h"
#include "panic.h"
#include "system.h"

#define BYTES2INT16(array,pos) \
          (array[pos] | (array[pos+1] << 8 ))
#define BYTES2INT32(array,pos) \
          (array[pos] | (array[pos+1] << 8 ) | \
          (array[pos+2] << 16 ) | (array[pos+3] << 24 ))

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

#define CLUSTERS_PER_FAT_SECTOR (SECTOR_SIZE / 4)
#define DIR_ENTRIES_PER_SECTOR  (SECTOR_SIZE / 32)
#define DIR_ENTRY_SIZE       32
#define FAT_BAD_MARK         0x0ffffff7
#define FAT_EOF_MARK         0x0ffffff8

struct fsinfo {
    int freecount; /* last known free cluster count */
    int nextfree;  /* first cluster to start looking for free clusters,
                      or 0xffffffff for no hint */
};
/* fsinfo offsets */
#define FSINFO_FREECOUNT 488
#define FSINFO_NEXTFREE  492

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
    int startsector;
    int dataclusters;
    struct fsinfo fsinfo;
};

static struct bpb fat_bpb;

static int first_sector_of_cluster(int cluster);
static int bpb_is_sane(void);
static void *cache_fat_sector(int secnum);
#ifdef DISK_WRITE
static unsigned int getcurrdostime(unsigned short *dosdate,
                                   unsigned short *dostime,
                                   unsigned char *dostenth);
static int create_dos_name(unsigned char *name, unsigned char *newname);
static int find_free_cluster(int start);
#endif

#define FAT_CACHE_SIZE 0x20
#define FAT_CACHE_MASK (FAT_CACHE_SIZE-1)

struct fat_cache_entry
{
    int secnum;
    bool inuse;
    bool dirty;
};

static char fat_cache_sectors[FAT_CACHE_SIZE][SECTOR_SIZE];
static struct fat_cache_entry fat_cache[FAT_CACHE_SIZE];

/* sectors cache for longname use */
static unsigned char lastsector[SECTOR_SIZE];
static unsigned char lastsector2[SECTOR_SIZE];

static int sec2cluster(int sec)
{
    if ( sec < fat_bpb.firstdatasector )
    {
        DEBUGF( "sec2cluster() - Bad sector number (%d)\n", sec);
        return -1;
    }

    return ((sec - fat_bpb.firstdatasector) / fat_bpb.bpb_secperclus) + 2;
}

static int cluster2sec(int cluster)
{
    int max_cluster = fat_bpb.totalsectors -
        fat_bpb.firstdatasector / fat_bpb.bpb_secperclus + 1;
    
    if(cluster > max_cluster)
    {
        DEBUGF( "cluster2sec() - Bad cluster number (%d)\n",
                cluster);
        return -1;
    }

    return first_sector_of_cluster(cluster);
}

static int first_sector_of_cluster(int cluster)
{
    return (cluster - 2) * fat_bpb.bpb_secperclus + fat_bpb.firstdatasector;
}

int fat_startsector(void)
{
    return fat_bpb.startsector;
}

int fat_mount(int startsector)
{
    unsigned char buf[SECTOR_SIZE];
    int err;
    int datasec;
    int i;

    for(i = 0;i < FAT_CACHE_SIZE;i++)
    {
        fat_cache[i].secnum = 8; /* We use a "safe" sector just in case */
        fat_cache[i].inuse = false;
        fat_cache[i].dirty = false;
    }

    /* Read the sector */
    err = ata_read_sectors(startsector,1,buf);
    if(err)
    {
        DEBUGF( "fat_mount() - Couldn't read BPB (error code %d)\n", err);
        return -1;
    }

    memset(&fat_bpb, 0, sizeof(struct bpb));
    fat_bpb.startsector = startsector;
    
    strncpy(fat_bpb.bs_oemname, &buf[BS_OEMNAME], 8);
    fat_bpb.bs_oemname[8] = 0;

    fat_bpb.bpb_bytspersec = BYTES2INT16(buf,BPB_BYTSPERSEC);
    fat_bpb.bpb_secperclus = buf[BPB_SECPERCLUS];
    fat_bpb.bpb_rsvdseccnt = BYTES2INT16(buf,BPB_RSVDSECCNT);
    fat_bpb.bpb_numfats    = buf[BPB_NUMFATS];
    fat_bpb.bpb_totsec16   = BYTES2INT16(buf,BPB_TOTSEC16);
    fat_bpb.bpb_media      = buf[BPB_MEDIA];
    fat_bpb.bpb_fatsz16    = BYTES2INT16(buf,BPB_FATSZ16);
    fat_bpb.bpb_fatsz32    = BYTES2INT32(buf,BPB_FATSZ32);
    fat_bpb.bpb_secpertrk  = BYTES2INT16(buf,BPB_SECPERTRK);
    fat_bpb.bpb_numheads   = BYTES2INT16(buf,BPB_NUMHEADS);
    fat_bpb.bpb_hiddsec    = BYTES2INT32(buf,BPB_HIDDSEC);
    fat_bpb.bpb_totsec32   = BYTES2INT32(buf,BPB_TOTSEC32);
    fat_bpb.last_word      = BYTES2INT16(buf,BPB_LAST_WORD);

    /* calculate a few commonly used values */
    if (fat_bpb.bpb_fatsz16 != 0)
        fat_bpb.fatsize = fat_bpb.bpb_fatsz16;
    else
        fat_bpb.fatsize = fat_bpb.bpb_fatsz32;

    if (fat_bpb.bpb_totsec16 != 0)
        fat_bpb.totalsectors = fat_bpb.bpb_totsec16;
    else
        fat_bpb.totalsectors = fat_bpb.bpb_totsec32;
    fat_bpb.firstdatasector = fat_bpb.bpb_rsvdseccnt +
        fat_bpb.bpb_numfats * fat_bpb.fatsize;

    /* Determine FAT type */
    datasec = fat_bpb.totalsectors - fat_bpb.firstdatasector;
    fat_bpb.dataclusters = datasec / fat_bpb.bpb_secperclus;

#ifdef TEST_FAT
    /*
      we are sometimes testing with "illegally small" fat32 images,
      so we don't use the proper fat32 test case for test code
    */
    if ( fat_bpb.bpb_fatsz16 )
#else
    if ( fat_bpb.dataclusters < 65525 )
#endif
    {
        DEBUGF("This is not FAT32. Go away!\n");
        return -2;
    }

    fat_bpb.bpb_extflags  = BYTES2INT16(buf,BPB_EXTFLAGS);
    fat_bpb.bpb_fsver     = BYTES2INT16(buf,BPB_FSVER);
    fat_bpb.bpb_rootclus  = BYTES2INT32(buf,BPB_ROOTCLUS);
    fat_bpb.bpb_fsinfo    = BYTES2INT16(buf,BPB_FSINFO);
    fat_bpb.bpb_bkbootsec = BYTES2INT16(buf,BPB_BKBOOTSEC);
    fat_bpb.bs_drvnum     = buf[BS_32_DRVNUM];
    fat_bpb.bs_bootsig    = buf[BS_32_BOOTSIG];

    if(fat_bpb.bs_bootsig == 0x29)
    {
        fat_bpb.bs_volid = BYTES2INT32(buf,BS_32_VOLID);
        strncpy(fat_bpb.bs_vollab, &buf[BS_32_VOLLAB], 11);
        strncpy(fat_bpb.bs_filsystype, &buf[BS_32_FILSYSTYPE], 8);
    }

    if (bpb_is_sane() < 0)
    {
        DEBUGF( "fat_mount() - BPB is not sane\n");
        return -3;
    }

    fat_bpb.rootdirsector = cluster2sec(fat_bpb.bpb_rootclus);

    /* Read the fsinfo sector */
    err = ata_read_sectors(startsector + fat_bpb.bpb_fsinfo, 1, buf);
    if (err)
    {
        DEBUGF( "fat_mount() - Couldn't read FSInfo (error code %d)\n", err);
        return -4;
    }
    fat_bpb.fsinfo.freecount = BYTES2INT32(buf, FSINFO_FREECOUNT);
    fat_bpb.fsinfo.nextfree = BYTES2INT32(buf, FSINFO_NEXTFREE);
    LDEBUGF("Freecount: %x\n",fat_bpb.fsinfo.freecount);
    LDEBUGF("Nextfree: %x\n",fat_bpb.fsinfo.nextfree);
    LDEBUGF("Cluster count: %x\n",fat_bpb.dataclusters);
    LDEBUGF("FAT sectors: %x\n",fat_bpb.fatsize);

    return 0;
}

static int bpb_is_sane(void)
{
    if(fat_bpb.bpb_bytspersec != 512)
    {
        DEBUGF( "bpb_is_sane() - Error: sector size is not 512 (%d)\n",
                fat_bpb.bpb_bytspersec);
        return -1;
    }
    if(fat_bpb.bpb_secperclus * fat_bpb.bpb_bytspersec > 128*1024)
    {
        DEBUGF( "bpb_is_sane() - Error: cluster size is larger than 128K "
                "(%d * %d = %d)\n",
                fat_bpb.bpb_bytspersec, fat_bpb.bpb_secperclus,
                fat_bpb.bpb_bytspersec * fat_bpb.bpb_secperclus);
        return -2;
    }
    if (fat_bpb.bpb_rsvdseccnt != 32)
    {
        DEBUGF( "bpb_is_sane() - Warning: Reserved sectors is not 32 (%d)\n",
                fat_bpb.bpb_rsvdseccnt);
    }
    if(fat_bpb.bpb_numfats != 2)
    {
        DEBUGF( "bpb_is_sane() - Warning: NumFATS is not 2 (%d)\n",
                fat_bpb.bpb_numfats);
    }
    if(fat_bpb.bpb_media != 0xf0 && fat_bpb.bpb_media < 0xf8)
    {
        DEBUGF( "bpb_is_sane() - Warning: Non-standard "
                "media type (0x%02x)\n",
                fat_bpb.bpb_media);
    }
    if(fat_bpb.last_word != 0xaa55)
    {
        DEBUGF( "bpb_is_sane() - Error: Last word is not "
                "0xaa55 (0x%04x)\n", fat_bpb.last_word);
        return -3;
    }

    if (fat_bpb.fsinfo.freecount >
        (fat_bpb.totalsectors - fat_bpb.firstdatasector)/
        fat_bpb.bpb_secperclus)
    {
        DEBUGF( "bpb_is_sane() - Error: FSInfo.Freecount > disk size "
                 "(0x%04x)\n", fat_bpb.fsinfo.freecount);
        return -4;
    }

    return 0;
}

static void *cache_fat_sector(int fatsector)
{
    int secnum = fatsector + fat_bpb.bpb_rsvdseccnt;
    int cache_index = secnum & FAT_CACHE_MASK;

    /* Delete the cache entry if it isn't the sector we want */
    if(fat_cache[cache_index].inuse &&
       fat_cache[cache_index].secnum != secnum)
    {
        /* Write back if it is dirty */
        if(fat_cache[cache_index].dirty)
        {
            if(ata_write_sectors(secnum + fat_bpb.startsector, 1,
                                 fat_cache_sectors[cache_index]))
            {
                panicf("cache_fat_sector() - Could not write sector %d\n",
                       secnum);
            }
        }
        fat_cache[cache_index].secnum = 8; /* Normally an unused sector */
        fat_cache[cache_index].dirty = false;
        fat_cache[cache_index].inuse = false;
    }
    
    /* Load the sector if it is not cached */
    if(!fat_cache[cache_index].inuse)
    {
        if(ata_read_sectors(secnum + fat_bpb.startsector,1,
                            fat_cache_sectors[cache_index]))
        {
            DEBUGF( "cache_fat_sector() - Could not read sector %d\n", secnum);
            return NULL;
        }
        fat_cache[cache_index].inuse = true;
        fat_cache[cache_index].secnum = secnum;
    }
    return fat_cache_sectors[cache_index];
}

static int find_free_cluster(int startcluster)
{
    int sector = startcluster / CLUSTERS_PER_FAT_SECTOR;
    int offset = startcluster % CLUSTERS_PER_FAT_SECTOR;
    int i;

    /* don't waste time scanning if the disk is already full */
    if (!fat_bpb.fsinfo.freecount)
        return 0;

    for (i = 0; i<fat_bpb.fatsize; i++) {
        int j;
        int nr = (i + sector) % fat_bpb.fatsize;
        unsigned int* fat = cache_fat_sector(nr);
        if ( !fat )
            break;
        for (j = offset; j < CLUSTERS_PER_FAT_SECTOR; j++) {
            if (!(SWAB32(fat[j]) & 0x0fffffff)) {
                int c = nr * CLUSTERS_PER_FAT_SECTOR + j;
                if ( c > fat_bpb.dataclusters+1 ) /* nr 0 is unused */
                    break;
                LDEBUGF("find_free_cluster(%x) == %x\n",startcluster,c);
                fat_bpb.fsinfo.nextfree = c;
                return c;
            }
        }
        offset = 0;
    }
    LDEBUGF("find_free_cluster(%x) == 0\n",startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry(unsigned int entry, unsigned int val)
{
    int sector = entry / CLUSTERS_PER_FAT_SECTOR;
    int offset = entry % CLUSTERS_PER_FAT_SECTOR;
    unsigned int* sec;

    LDEBUGF("update_fat_entry(%x,%x)\n",entry,val);

    if (entry==val)
        panicf("Creating FAT loop: %x,%x\n",entry,val);

    if ( entry < 2 )
        panicf("Updating reserved FAT entry %d.\n",entry);

    sec = cache_fat_sector(sector);
    if (!sec)
    {
        DEBUGF( "update_entry() - Could not cache sector %d\n", sector);
        return -1;
    }
    fat_cache[sector & FAT_CACHE_MASK].dirty = true;

    if ( val ) {
        if (!(SWAB32(sec[offset]) & 0x0fffffff))
            fat_bpb.fsinfo.freecount--;
    }
    else {
        if (SWAB32(sec[offset]) & 0x0fffffff)
            fat_bpb.fsinfo.freecount++;
    }

    LDEBUGF("update_fat_entry: %d free clusters\n", fat_bpb.fsinfo.freecount);

    /* don't change top 4 bits */
    sec[offset] &= SWAB32(0xf0000000);
    sec[offset] |= SWAB32(val & 0x0fffffff);

    return 0;
}

static int read_fat_entry(int entry)
{
    int sector = entry / CLUSTERS_PER_FAT_SECTOR;
    int offset = entry % CLUSTERS_PER_FAT_SECTOR;
    unsigned int* sec;

    sec = cache_fat_sector(sector);
    if (!sec)
    {
        DEBUGF( "read_fat_entry() - Could not cache sector %d\n", sector);
        return -1;
    }

    return SWAB32(sec[offset]) & 0x0fffffff;
}

static int get_next_cluster(unsigned int cluster)
{
    int next_cluster;

    next_cluster = read_fat_entry(cluster);

    /* is this last cluster in chain? */
    if ( next_cluster >= FAT_EOF_MARK )
        return 0;
    else
        return next_cluster;
}

static int flush_fat(void)
{
    int i;
    int err;
    unsigned char *sec;
    int secnum;
    unsigned char fsinfo[SECTOR_SIZE];
    unsigned int* intptr;

    LDEBUGF("flush_fat()\n");

    for(i = 0;i < FAT_CACHE_SIZE;i++)
    {
        if(fat_cache[i].inuse && fat_cache[i].dirty)
        {
            secnum = fat_cache[i].secnum + fat_bpb.startsector;
            LDEBUGF("Flushing FAT sector %x\n", secnum);
            sec = fat_cache_sectors[i];

            /* Write to the first FAT */
            err = ata_write_sectors(secnum, 1, sec);
            if(err)
            {
                DEBUGF( "flush_fat() - Couldn't write"
                        " sector (%d)\n", secnum);
                return -1;
            }

            if(fat_bpb.bpb_numfats > 1 )
            {
                /* Write to the second FAT */
                err = ata_write_sectors(secnum + fat_bpb.fatsize, 1, sec);
                if (err)
                {
                    DEBUGF( "flush_fat() - Couldn't write"
                             " sector (%d)\n", secnum + fat_bpb.fatsize);
                    return -2;
                }
            }
            fat_cache[i].dirty = false;
        }
    }

    /* update fsinfo */
    err = ata_read_sectors(fat_bpb.startsector + fat_bpb.bpb_fsinfo, 1,fsinfo);
    if (err)
    {
        DEBUGF( "flush_fat() - Couldn't read FSInfo (error code %d)\n", err);
        return -1;
    }
    intptr = (int*)&(fsinfo[FSINFO_FREECOUNT]);
    *intptr = SWAB32(fat_bpb.fsinfo.freecount);

    intptr = (int*)&(fsinfo[FSINFO_NEXTFREE]);
    *intptr = SWAB32(fat_bpb.fsinfo.nextfree);

    err = ata_write_sectors(fat_bpb.startsector + fat_bpb.bpb_fsinfo,1,fsinfo);
    if (err)
    {
        DEBUGF( "flush_fat() - Couldn't write FSInfo (error code %d)\n", err);
        return -1;
    }

    return 0;
}

static unsigned int getcurrdostime(unsigned short *dosdate,
                                   unsigned short *dostime,
                                   unsigned char *dostenth)
{
#if 0
    struct tm *tm;
    unsigned long now = time();
    tm = localtime(&tb.time);

    *dosdate = ((tm->tm_year - 80) << 9) |
        ((tm->tm_mon + 1)  << 5) |
        (tm->tm_mday);

    *dostime = (tm->tm_hour << 11) |
        (tm->tm_min << 5) |
        (tm->tm_sec >> 1);

    *dostenth = (tm->tm_sec & 1) * 100 + tb.millitm / 10;
#else
    *dosdate = 0;
    *dostime = 0;
    *dostenth = 0;
#endif
    return 0;
}

static int add_dir_entry(struct fat_dir* dir,
                         struct fat_direntry* de,
                         struct fat_file* file)
{
    unsigned char buf[SECTOR_SIZE];
    unsigned char *eptr;
    int i;
    int err;
    int sec;
    int sec_cnt = 0;
    bool need_to_update_last_empty_marker = false;
    bool done = false;
    unsigned char firstbyte;
    int currdir = dir->startcluster;
    bool is_rootdir = (currdir == 0);

    LDEBUGF( "add_dir_entry(%x,%s,%x)\n",
             dir->startcluster, de->name, file->firstcluster);

    if (is_rootdir)
        sec = fat_bpb.rootdirsector;
    else
        sec = first_sector_of_cluster(currdir);

    while(!done)
    {
        bool new = false;
        if (sec_cnt >= fat_bpb.bpb_secperclus)
        {
            int oldcluster;

            /* we're not adding a whole new sector 
               just for the end-of-dir marker */
            if ( need_to_update_last_empty_marker )
                break;

            if (!currdir)
                currdir = sec2cluster(fat_bpb.rootdirsector);
            oldcluster = currdir;

            /* We have reached the end of this cluster */
            LDEBUGF("Moving to the next cluster...\n");
            currdir = get_next_cluster(currdir);
            if (!currdir)
            {
                /* end of dir, add new cluster */
                LDEBUGF("Adding cluster to dir\n");
                currdir = find_free_cluster(oldcluster+1);
                if (!currdir) {
                    DEBUGF("add_dir_entry(): Disk full!\n");
                    return -1;
                }
                update_fat_entry(oldcluster, currdir);
                update_fat_entry(currdir, FAT_EOF_MARK);
                new = true;
                memset(buf, 0, sizeof buf);

                /* clear remaining sectors in this cluster */
                if (fat_bpb.bpb_secperclus > 1) {
                    int i;
                    sec = cluster2sec(currdir);
                    for (i=1; i<fat_bpb.bpb_secperclus; i++ ) {
                        err = ata_write_sectors(sec + fat_bpb.startsector + i,
                                                1, buf);
                        if (err) {
                            DEBUGF( "add_dir_entry() - "
                                    " Couldn't write dir"
                                    " sector (error code %d)\n", err);
                            return -3;
                        }
                    }
                }       
            }
            LDEBUGF("new cluster is %x\n", currdir);
            sec = cluster2sec(currdir);
            sec_cnt = 0;
        }

        if (!new) {
            /* Read the next sector in the current dir */
            err = ata_read_sectors(sec + fat_bpb.startsector,1,buf);
            if (err)
            {
                DEBUGF( "add_dir_entry() - Couldn't read dir sector"
                        " (error code %d)\n", err);
                return -2;
            }
        }

        if (need_to_update_last_empty_marker)
        {
            /* All we need to do is to set the first entry to 0 */
            LDEBUGF("Clearing the first entry in sector %x\n", sec);
            buf[0] = 0;
            done = true;
        }
        else
        {
            /* Look for a free slot */
            for (i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE)
            {
                firstbyte = buf[i];
                if (firstbyte == 0xe5 || firstbyte == 0)
                {
                    LDEBUGF("Found free entry %d in sector %x\n",
                           i/DIR_ENTRY_SIZE, sec);
                    eptr = &buf[i];
                    memset(eptr, 0, DIR_ENTRY_SIZE);
                    strncpy(&eptr[FATDIR_NAME], de->name, 11);
                    eptr[FATDIR_ATTR] = de->attr;
                    eptr[FATDIR_NTRES] = 0;
                    
                    /* remember where the dir entry is located */
                    file->dirsector = sec;
                    file->direntry = i/DIR_ENTRY_SIZE;

                    /* Advance the last_empty_entry marker */
                    if (firstbyte == 0)
                    {
                        i += DIR_ENTRY_SIZE;
                        if (i < SECTOR_SIZE)
                        {
                            buf[i] = 0;
                            /* We are done */
                            done = true;
                        }
                        else
                        {
                            /* We must fill in the first entry
                               in the next sector */
                            need_to_update_last_empty_marker = true;
                            LDEBUGF("need_to_update_last_empty\n");
                        }
                    }
                    else
                        done = true;

                    err = ata_write_sectors(sec + fat_bpb.startsector,1,buf);
                    if (err)
                    {
                        DEBUGF( "add_dir_entry() - "
                                " Couldn't write dir"
                                " sector (error code %d)\n", err);
                        return -3;
                    }
                    break;
                }
            }
        }
        sec++;
        sec_cnt++;
    }

    return 0;
}

unsigned char char2dos(unsigned char c)
{
    switch(c)
    {
        case 0xe5: /* Special kanji character */
            c = 0x05;
            break;
        case 0x22:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2e:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x7c:
            /* Illegal name */
            c = 0;
            break;
                
        default:
            if(c < 0x20)
            {
                /* Illegal name */
                c = 0;
            }
            break;
    }
    return c;
}

static int create_dos_name(unsigned char *name, unsigned char *newname)
{
    unsigned char n[12];
    unsigned char c;
    int i;
    char *ext;

    strcpy(n, name);
    
    ext = strchr(n, '.');
    if(ext)
    {
        *ext++ = 0;
    }

    /* The file name is either empty, or there was only an extension.
       In either case it is illegal. */
    if(n[0] == 0)
    {
        return -2;
    }
    
    /* Name part */
    for(i = 0;n[i] && (i < 8);i++)
    {
        c = char2dos(n[i]);
        if(c)
        {
            newname[i] = toupper(c);
        }
    }
    while(i < 8)
    {
        newname[i++] = ' ';
    }

    /* Extension part */
    for (i = 0;ext && ext[i] && (i < 3);i++)
    {
        c = char2dos(ext[i]);
        if (c)
        {
            newname[8+i] = toupper(c);
        }
    }
    while(i < 3)
    {
        newname[8+i++] = ' ';
    }
    return 0;
}

static void update_dir_entry( struct fat_file* file, int size )
{
    unsigned char buf[SECTOR_SIZE];
    int sector = file->dirsector + fat_bpb.startsector;
    unsigned char* entry = buf + file->direntry * DIR_ENTRY_SIZE;
    unsigned int* sizeptr;
    unsigned short* clusptr;
    int err;

    LDEBUGF("update_dir_entry(cluster:%x entry:%d sector:%x size:%d)\n",
            file->firstcluster,file->direntry,sector,size);

    if ( file->direntry >= (SECTOR_SIZE / DIR_ENTRY_SIZE) )
        panicf("update_dir_entry(): Illegal entry %d!\n",file->direntry);

    if ( file->direntry < 0 )
        panicf("update_dir_entry(): Illegal entry %d!\n",file->direntry);

    err = ata_read_sectors(sector, 1, buf);
    if (err)
    {
        DEBUGF( "update_dir_entry() - Couldn't read dir sector %d"
                " (error code %d)\n", sector, err);
        return;
    }

    if ( size == -1 ) {
        /* mark entry deleted */
        entry[0] = 0xe5;
    }
    else {
        clusptr = (short*)(entry + FATDIR_FSTCLUSHI);
        *clusptr = SWAB16(file->firstcluster >> 16);
        
        clusptr = (short*)(entry + FATDIR_FSTCLUSLO);
        *clusptr = SWAB16(file->firstcluster & 0xffff);

        sizeptr = (int*)(entry + FATDIR_FILESIZE);
        *sizeptr = SWAB32(size);
    }

    err = ata_write_sectors(sector, 1, buf);
    if (err)
    {
        DEBUGF( "update_file_size() - Couldn't write dir sector %d"
                " (error code %d)\n", sector, err);
        return;
    }
}

static int parse_direntry(struct fat_direntry *de, unsigned char *buf)
{
    int i=0,j=0;
    memset(de, 0, sizeof(struct fat_direntry));
    de->attr = buf[FATDIR_ATTR];
    de->crttimetenth = buf[FATDIR_CRTTIMETENTH];
    de->crtdate = BYTES2INT16(buf,FATDIR_CRTDATE);
    de->crttime = BYTES2INT16(buf,FATDIR_CRTTIME);
    de->wrtdate = BYTES2INT16(buf,FATDIR_WRTDATE);
    de->wrttime = BYTES2INT16(buf,FATDIR_WRTTIME);
    de->filesize = BYTES2INT32(buf,FATDIR_FILESIZE);
    de->firstcluster = BYTES2INT16(buf,FATDIR_FSTCLUSLO) |
                      (BYTES2INT16(buf,FATDIR_FSTCLUSHI) << 16);

    /* fix the name */
    for (i=0; (i<8) && (buf[FATDIR_NAME+i] != ' '); i++)
        de->name[j++] = buf[FATDIR_NAME+i];
    if ( buf[FATDIR_NAME+8] != ' ' ) {
        de->name[j++] = '.';
        for (i=8; (i<11) && (buf[FATDIR_NAME+i] != ' '); i++)
            de->name[j++] = buf[FATDIR_NAME+i];
    }
    return 1;
}

int fat_open(unsigned int startcluster,
             struct fat_file *file,
             struct fat_dir* dir)
{
    file->firstcluster = startcluster;
    file->lastcluster = startcluster;
    file->lastsector = 0;
    file->sectornum = 0;

    /* remember where the file's dir entry is located */
    file->dirsector = dir->cached_sec;
    file->direntry = dir->entry - 1;
    LDEBUGF("fat_open(%x), entry %d\n",startcluster,file->direntry);
    return 0;
}

int fat_create_file(char* name,
                    struct fat_file* file,
                    struct fat_dir* dir)
{
    struct fat_direntry de;
    int err;

    LDEBUGF("fat_create_file(\"%s\",%x,%x)\n",name,file,dir);
    memset(&de, 0, sizeof(struct fat_direntry));
    if (create_dos_name(name, de.name) < 0)
    {
        DEBUGF( "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }
    getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    
    err = add_dir_entry(dir, &de, file);
    if (!err) {
        file->firstcluster = 0;
        file->lastcluster = 0;
        file->lastsector = 0;
        file->sectornum = 0;
    }

    return err;
}

int fat_closewrite(struct fat_file *file, int size)
{
    int next, last = file->lastcluster;
    int endcluster = last;

    LDEBUGF("fat_closewrite(size=%d, last=%x)\n",size, last);

    /* truncate trailing clusters */
    last = get_next_cluster(last);
    while ( last && last != FAT_EOF_MARK ) {
        next = get_next_cluster(last);
        update_fat_entry(last,0);
        last = next;
    }

    /* if no cluster was written, size is 0 */
    if (!endcluster)
        size = 0;

    if (!size) {
        /* empty file */
        if ( file->firstcluster ) {
            update_fat_entry(file->firstcluster, 0);
            file->firstcluster = 0;
        }
    }
    else
        update_fat_entry(endcluster, FAT_EOF_MARK);

    if (file->dirsector)
        update_dir_entry(file, size);
    flush_fat();

#ifdef TEST_FAT
    {
        /* debug */
        int count = 0;
        int len;
        for ( next = file->firstcluster; next;
              next = get_next_cluster(next) )
            LDEBUGF("cluster %d: %x\n", count++, next);
        len = count * fat_bpb.bpb_secperclus * SECTOR_SIZE;
        LDEBUGF("File is %d clusters (chainlen=%d, size=%d)\n",
                count, len, size );
        if ( len > size + fat_bpb.bpb_secperclus * SECTOR_SIZE)
            panicf("Cluster chain is too long\n");
        if ( len < size )
            panicf("Cluster chain is too short\n");
    }
#endif

    return 0;
}

int fat_remove(struct fat_file* file)
{
    int next, last = file->firstcluster;

    LDEBUGF("fat_remove(%x)\n",last);

    while ( last ) {
        next = get_next_cluster(last);
        update_fat_entry(last,0);
        last = next;
    }
    update_dir_entry(file, -1);
    file->dirsector = 0;
    file->firstcluster = 0;

    return 0;
}


static int next_write_cluster(struct fat_file* file,
                              int oldcluster,
                              int* newsector)
{
    int cluster = 0;
    int sector;

    LDEBUGF("next_write_cluster(%x,%x)\n",file->firstcluster, oldcluster);

    if (oldcluster)
        cluster = get_next_cluster(oldcluster);

    if (!cluster) {
        if (oldcluster)
            cluster = find_free_cluster(oldcluster+1);
        else
            cluster = find_free_cluster(fat_bpb.fsinfo.nextfree);

        if (cluster) {
            if (oldcluster)
                update_fat_entry(oldcluster, cluster); 
            else
                file->firstcluster = cluster;
            update_fat_entry(cluster, FAT_EOF_MARK); 
        }
        else {
#ifdef TEST_FAT
            if (fat_bpb.fsinfo.freecount>0)
                panicf("There is free space, but find_free_cluster() "
                       "didn't find it!\n");
#endif
            DEBUGF("next_write_cluster(): Disk full!\n");
            return 0;
        }
    }
    sector = cluster2sec(cluster);
    if (sector<0)
        return 0;

    *newsector = sector;
    return cluster;
}

static bool transfer( int start, int count, char* buf, bool write )
{
    int err;

    LDEBUGF("transfer(s=%x, c=%x, %s)\n",start, count, write?"write":"read");
    if (write)
        err = ata_write_sectors(start, count, buf);
    else
        err = ata_read_sectors(start, count, buf);
    if (err) {
        DEBUGF( "transfer() - Couldn't %s sector %x"
                " (error code %d)\n", 
                write ? "write":"read", start, err);
        return false;
    }
    return true;
}


int fat_readwrite( struct fat_file *file, int sectorcount, 
                   void* buf, bool write )
{
    int cluster = file->lastcluster;
    int sector = file->lastsector;
    int numsec = file->sectornum;
    int first=0, last=0;
    int i;

    LDEBUGF( "fat_readwrite(file:%x,count:0x%x,buf:%x,%s)\n",
             cluster,sectorcount,buf,write?"write":"read");
    LDEBUGF( "fat_readwrite: sec=%x numsec=%d\n", sector,numsec);
    if ( sector == -1 )
        return 0;

    /* find sequential sectors and write them all at once */
    for (i=0; (i < sectorcount) && (sector > -1); i++ ) {
        numsec++;
        if ( numsec > fat_bpb.bpb_secperclus || !cluster ) {
            int oldcluster = cluster;
            if (write)
                cluster = next_write_cluster(file, cluster, &sector);
            else {
                cluster = get_next_cluster(cluster);
                sector = cluster2sec(cluster);
            }
            if (!cluster) {
                sector = -1;
                if ( write ) {
                    /* set file->lastcluster to last available cluster,
                       so we can terminate the file in closewrite() */
                    cluster = oldcluster;
                }
            }
            numsec=1;
        }
        else {
            if (sector)
                sector++;
            else {
                /* look up first sector of file */
                sector = cluster2sec(file->firstcluster);
                numsec=1;
            }
        }

        if (!first)
            first = sector;

        if ( ((sector != first) && (sector != last+1)) || /* not sequential */
             (last-first+1 == 256) ) { /* max 256 sectors per ata request */
            int count = last - first + 1;
            if (!transfer( first + fat_bpb.startsector, count, buf, write ))
                return -2;

            ((char*)buf) += count * SECTOR_SIZE;
            first = sector;
        }

        if (i == sectorcount-1) { /* last sector requested */
            int count = sector - first + 1;
            if (!transfer( first + fat_bpb.startsector, 
                           count, buf, write ))
                return -2;
        }

        last = sector;
    }

    file->lastcluster = cluster;
    file->lastsector = sector;
    file->sectornum = numsec;

    /* if eof, don't report last block as read/written */
    if (sector == -1)
        i--;

    return i;
}

int fat_seek(struct fat_file *file, int seeksector )
{
    int clusternum, sectornum, sector=0;
    int cluster = file->firstcluster;
    int i;

    seeksector--;
    clusternum = seeksector / fat_bpb.bpb_secperclus;
    sectornum = seeksector % fat_bpb.bpb_secperclus;

    for (i=0; i<clusternum; i++) {
        cluster = get_next_cluster(cluster);
        if (!cluster) {
            sector = -1;
            break;
        }
    }

    if ( sector > -1 )
        sector = cluster2sec(cluster) + sectornum;

    LDEBUGF("fat_seek(%x) == %x, %x, %x\n",
            seeksector, cluster, sector, sectornum);

    file->lastcluster = cluster;
    file->lastsector = sector;
    file->sectornum = sectornum + 1;
    return 0;
}

int fat_opendir(struct fat_dir *dir, unsigned int startcluster)
{
    int is_rootdir = (startcluster == 0);
    unsigned int sec;
    int err;

    if(is_rootdir)
    {
        sec = fat_bpb.rootdirsector;
    }
    else
    {
        sec = first_sector_of_cluster(startcluster);
    }

    /* Read the first sector in the current dir */
    err = ata_read_sectors(sec + fat_bpb.startsector,1,dir->cached_buf);
    if(err)
    {
        DEBUGF( "fat_opendir() - Couldn't read dir sector"
                " (error code %d)\n", err);
        return -1;
    }

    dir->entry = 0;
    dir->cached_sec = sec;
    dir->num_sec = 0;
    dir->startcluster = startcluster;

    return 0;
}

int fat_getnext(struct fat_dir *dir, struct fat_direntry *entry)
{
    bool done = false;
    int i;
    int err;
    unsigned char firstbyte;
    int longarray[20];
    int longs=0;
    int sectoridx=0;

    while(!done)
    {
        for (i = dir->entry; i < SECTOR_SIZE/DIR_ENTRY_SIZE; i++)
        {
            firstbyte = dir->cached_buf[i*DIR_ENTRY_SIZE];

            if (firstbyte == 0xe5) {
                /* free entry */
                sectoridx = 0;
                continue;
            }

            if (firstbyte == 0) {
                /* last entry */
                entry->name[0] = 0;
                return 0;
            }

            /* longname entry? */
            if ( ( dir->cached_buf[i*DIR_ENTRY_SIZE + FATDIR_ATTR] &
                   FAT_ATTR_LONG_NAME_MASK ) == FAT_ATTR_LONG_NAME ) {
                longarray[longs++] = i*DIR_ENTRY_SIZE + sectoridx;
            }
            else {
                if ( parse_direntry(entry,
                                    &dir->cached_buf[i*DIR_ENTRY_SIZE]) ) {

                    /* don't return volume id entry */
                    if ( entry->attr == FAT_ATTR_VOLUME_ID )
                        continue;

                    /* replace shortname with longname? */
                    if ( longs ) {
                        int j,k,l=0;
                        /* iterate backwards through the dir entries */
                        for (j=longs-1; j>=0; j--) {
                            unsigned char* ptr = dir->cached_buf;
                            int index = longarray[j];
                            /* current or cached sector? */
                            if ( sectoridx >= SECTOR_SIZE ) {
                                if ( sectoridx >= SECTOR_SIZE*2 ) {
                                    if ( ( index >= SECTOR_SIZE ) &&
                                         ( index < SECTOR_SIZE*2 ))
                                        ptr = lastsector;
                                    else
                                        ptr = lastsector2;
                                }
                                else {
                                    if ( index < SECTOR_SIZE )
                                        ptr = lastsector;
                                }

                                index &= SECTOR_SIZE-1;
                            }

                            /* names are stored in unicode, but we
                               only grab the low byte (iso8859-1). */
                            for (k=0; k<5; k++)
                                entry->name[l++] = ptr[index + k*2 + 1];
                            for (k=0; k<6; k++)
                                entry->name[l++] = ptr[index + k*2 + 14];
                            for (k=0; k<2; k++)
                                entry->name[l++] = ptr[index + k*2 + 28];
                        }
                        entry->name[l]=0;
                    }
                    done = true;
                    sectoridx = 0;
                    break;
                }
            }
        }

        /* save this sector, for longname use */
        if ( sectoridx )
            memcpy( lastsector2, dir->cached_buf, SECTOR_SIZE );
        else
            memcpy( lastsector, dir->cached_buf, SECTOR_SIZE );
        sectoridx += SECTOR_SIZE;

        /* Next sector? */
        if (i < SECTOR_SIZE / DIR_ENTRY_SIZE)
        {
            i++;
        }
        else
        {
            dir->num_sec++;

            /* Do we need to advance one cluster? */
            if (dir->num_sec < fat_bpb.bpb_secperclus)
            {
                dir->cached_sec++;
            }
            else
            {
                int cluster = sec2cluster(dir->cached_sec);
                if ( cluster < 0 ) {
                    DEBUGF("sec2cluster failed\n");
                    return -1;
                }
                dir->num_sec = 0;
                cluster = get_next_cluster( cluster );
                if (!cluster)
                {
                    DEBUGF("End of cluster chain.\n");
                    return -2;
                }
                dir->cached_sec = cluster2sec(cluster);
                if ( dir->cached_sec < 0 )
                {
                    DEBUGF("Invalid cluster: %d\n",cluster);
                    return -3;
                }

            }
   
            /* Read the next sector */
            err = ata_read_sectors(dir->cached_sec + fat_bpb.startsector, 1,
                                   dir->cached_buf);
            if (err)
            {
                DEBUGF( "fat_getnext() - Couldn't read dir sector"
                        " (error code %d)\n", err);
                return -4;
            }

            i = 0;
        }
        dir->entry = i;
    }
    return 0;
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
