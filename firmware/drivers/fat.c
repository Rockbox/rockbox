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
#include <time.h>
#include <sys/timeb.h>
#include <stdbool.h>
#include "fat.h"
#include "ata.h"
#include "debug.h"
#include "system.h"

#define BYTES2INT16(array,pos) \
          (array[pos] | (array[pos+1] << 8 ))
#define BYTES2INT32(array,pos) \
          (array[pos] | (array[pos+1] << 8 ) | \
          (array[pos+2] << 16 ) | (array[pos+3] << 24 ))

#define NUM_ROOT_DIR_ENTRIES 512
#define NUM_FATS 2
#define NUM_RESERVED_SECTORS 1
#define NUM_BLOCKS 10000

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

struct fsinfo {
    int freecount; /* last known free cluster count */
    int nextfree;  /* first cluster to start looking for free clusters,
                      or 0xffffffff for no hint */
};
/* fsinfo offsets */
#define FSINFO_FREECOUNT 488
#define FSINFO_NEXTFREE  492

static int first_sector_of_cluster(int cluster);
static int bpb_is_sane(void);
static void *cache_fat_sector(int secnum);
#ifdef DISK_WRITE
static unsigned int getcurrdostime(unsigned short *dosdate,
                                   unsigned short *dostime,
                                   unsigned char *dostenth);
static int create_dos_name(unsigned char *name, unsigned char *newname);
#endif

/* global FAT info struct */
struct bpb fat_bpb;

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

int fat_mount(int startsector)
{
    unsigned char buf[SECTOR_SIZE];
    int err;
    int datasec;
    int countofclusters;
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
        DEBUGF( "fat_mount() - Couldn't read BPB (error code %d)\n",
                err);
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
    fat_bpb.bpb_rootentcnt = BYTES2INT16(buf,BPB_ROOTENTCNT);
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
    countofclusters = datasec / fat_bpb.bpb_secperclus;

#ifdef TEST_FAT
    /*
      we are sometimes testing with "illegally small" fat32 images,
      so we don't use the proper fat32 test case for test code
    */
    if ( fat_bpb.bpb_fatsz16 )
#else
    if ( countofclusters < 65525 )
#endif
    {
        DEBUGF("This is not FAT32. Go away!\n");
        return -1;
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
        return -1;
    }

    fat_bpb.rootdirsector = cluster2sec(fat_bpb.bpb_rootclus);

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
    if(fat_bpb.bpb_secperclus * fat_bpb.bpb_bytspersec > 32768)
    {
        DEBUGF( "bpb_is_sane() - Warning: cluster size is larger than 32K "
                "(%d * %d = %d)\n",
                fat_bpb.bpb_bytspersec, fat_bpb.bpb_secperclus,
                fat_bpb.bpb_bytspersec * fat_bpb.bpb_secperclus);
    }
    if(fat_bpb.bpb_rsvdseccnt != 1)
    {
        DEBUGF( "bpb_is_sane() - Warning: Reserved sectors is not 1 (%d)\n",
                fat_bpb.bpb_rsvdseccnt);
    }
    if(fat_bpb.bpb_numfats != 2)
    {
        DEBUGF( "bpb_is_sane() - Warning: NumFATS is not 2 (%d)\n",
                fat_bpb.bpb_numfats);
    }
    if(fat_bpb.bpb_rootentcnt != 512)
    {
        DEBUGF( "bpb_is_sane() - Warning: RootEntCnt is not 512 (%d)\n",
                fat_bpb.bpb_rootentcnt);
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
        return -1;
    }
    return 0;
}

static void *cache_fat_sector(int secnum)
{
    int cache_index = secnum & FAT_CACHE_MASK;

    /* Delete the cache entry if it isn't the sector we want */
    if(fat_cache[cache_index].inuse &&
       fat_cache[cache_index].secnum != secnum)
    {
#ifdef WRITE
        /* Write back if it is dirty */
        if(fat_cache[cache_index].dirty)
        {
            if(ata_write_sectors(secnum + fat_bpb.startsector, 1, sec))
            {
                panic("cache_fat_sector() - Could"
                      " not write sector %d\n",
                      secnum);
            }
        }
        fat_cache[cache_index].secnum = 8; /* Normally an unused sector */
        fat_cache[cache_index].dirty = false;
#endif
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

#ifdef DISK_WRITE
static int update_entry(int entry, unsigned int val)
{
    unsigned long *sec;
    int fatoffset;
    int thisfatsecnum;
    int thisfatentoffset;

    fatoffset = entry * 4;
    thisfatsecnum = fatoffset / fat_bpb.bpb_bytspersec + fat_bpb.bpb_rsvdseccnt;
    thisfatentoffset = fatoffset % fat_bpb.bpb_bytspersec;

    /* Load the sector if it is not cached */
    sec = cache_fat_sector(thisfatsecnum);
    if(!sec)
    {
        DEBUGF( "update_entry() - Could not cache sector %d\n",
                thisfatsecnum);
        return -1;
    }

    /* We can safely assume that the correct sector is in the cache,
       so we mark it dirty without checking the sector number */
    fat_cache[thisfatsecnum & FAT_CACHE_MASK].dirty = 1;

    /* don't change top 4 bits */
    sec[thisfatentoffset/sizeof(int)] &= 0xf000000;
    sec[thisfatentoffset/sizeof(int)] |= val & 0x0fffffff;

    return 0;
}
#endif

static int read_entry(int entry)
{
    unsigned long *sec;
    int fatoffset;
    int thisfatsecnum;
    int thisfatentoffset;
    int val = -1;

    fatoffset = entry * 4;
    thisfatsecnum = fatoffset / fat_bpb.bpb_bytspersec + fat_bpb.bpb_rsvdseccnt;
    thisfatentoffset = fatoffset % fat_bpb.bpb_bytspersec;

    /* Load the sector if it is not cached */
    sec = cache_fat_sector(thisfatsecnum);
    if(!sec)
    {
        DEBUGF( "read_entry() - Could not cache sector %d\n",
                thisfatsecnum);
        return -1;
    }

    val = sec[thisfatentoffset/sizeof(int)];

    val = SWAB32(val);
    
    return val;
}

static int get_next_cluster(unsigned int cluster)
{
    int next_cluster;

    next_cluster = read_entry(cluster);

    /* is this last cluster in chain? */
    if ( next_cluster >= 0x0ffffff8 )
        return 0;
    else
        return next_cluster;
}

#ifdef DISK_WRITE
static int flush_fat(struct bpb *bpb)
{
    int i;
    int err;
    unsigned char *sec;
    int secnum;
    int fatsz;
    unsigned short d, t;
    char m;

    fatsz = fat_bpb.fatsize;

    for(i = 0;i < FAT_CACHE_SIZE;i++)
    {
        if(fat_cache[i].ptr && fat_cache[i].dirty)
        {
            secnum = fat_cache[i].secnum + fat_bpb.bpb_rsvdseccnt +
                fat_bpb.startsector;
            DEBUGF("Flushing FAT sector %d\n", secnum);
            sec = fat_cache[i].ptr;

            /* Write to the first FAT */
            err = ata_write_sectors(secnum, 1, sec);
            if(err)
            {
                DEBUGF( "flush_fat() - Couldn't write"
                        " sector (%d)\n", secnum);
                return -1;
            }

            /* Write to the second FAT */
            err = ata_write_sectors(secnum + fatsz, 1, sec);
            if(err)
            {
                DEBUGF( "flush_fat() - Couldn't write"
                        " sector (%d)\n", secnum + fatsz);
                return -1;
            }
            fat_cache[i].dirty = 0;
        }
    }

    getcurrdostime(&d, &t, &m);
    return 0;
}

static unsigned int getcurrdostime(unsigned short *dosdate,
                                   unsigned short *dostime,
                                   unsigned char *dostenth)
{
    struct timeb tb;
    struct tm *tm;

    ftime(&tb);
    tm = localtime(&tb.time);

    *dosdate = ((tm->tm_year - 80) << 9) |
        ((tm->tm_mon + 1)  << 5) |
        (tm->tm_mday);

    *dostime = (tm->tm_hour << 11) |
        (tm->tm_min << 5) |
        (tm->tm_sec >> 1);

    *dostenth = (tm->tm_sec & 1) * 100 + tb.millitm / 10;
    return 0;
}

static int add_dir_entry(unsigned int currdir, struct fat_direntry *de)
{
    unsigned char buf[SECTOR_SIZE];
    unsigned char *eptr;
    int i;
    int err;
    unsigned int sec;
    unsigned int sec_cnt;
    int need_to_update_last_empty_marker = 0;
    int is_rootdir = (currdir == 0);
    int done = 0;
    unsigned char firstbyte;

    if(is_rootdir)
    {
        sec = fat_bpb.rootdirsector;
    }
    else
    {
        sec = first_sector_of_cluster(currdir);
    }

    sec_cnt = 0;

    while(!done)
    {
        /* The root dir has a fixed size */
        if(is_rootdir)
        {
            if(sec_cnt >= fat_bpb.bpb_rootentcnt * 32 / fat_bpb.bpb_bytspersec)
            {
                /* We have reached the last sector of the root dir */
                if(need_to_update_last_empty_marker)
                {
                    /* Since the directory is full, there is no room for
                       a marker, so we just exit */
                    return 0;
                }
                else
                {
                    DEBUGF( "add_dir_entry() -"
                            " Root dir is full\n");
                    return -1;
                }
            }
        }
        else
        {
            if(sec_cnt >= fat_bpb.bpb_secperclus)
            {
                /* We have reached the end of this cluster */
                DEBUGF("Moving to the next cluster...");
                currdir = get_next_cluster(currdir);
                DEBUGF("new cluster is %d\n", currdir);
                
                if(!currdir)
                {
                    /* This was the last in the chain,
                       we have to allocate a new cluster */
                    /* TODO */
                }
            }
        }

        DEBUGF("Reading sector %d...\n", sec);
        /* Read the next sector in the current dir */
        err = ata_read_sectors(sec + fat_bpb.startsector,1,buf);
        if(err)
        {
            DEBUGF( "add_dir_entry() - Couldn't read dir sector"
                    " (error code %d)\n", err);
            return -1;
        }

        if(need_to_update_last_empty_marker)
        {
            /* All we need to do is to set the first entry to 0 */
            DEBUGF("Clearing the first entry in sector %d\n", sec);
            buf[0] = 0;
            done = 1;
        }
        else
        {
            /* Look for a free slot */
            for(i = 0;i < SECTOR_SIZE;i+=32)
            {
                firstbyte = buf[i];
                if(firstbyte == 0xe5 || firstbyte == 0)
                {
                    DEBUGF("Found free slot at entry %d in sector %d\n",
                           i/32, sec);
                    eptr = &buf[i];
                    memset(eptr, 0, 32);
                    strncpy(&eptr[FATDIR_NAME], de->name, 11);
                    eptr[FATDIR_ATTR] = de->attr;
                    eptr[FATDIR_NTRES] = 0;
                    
                    eptr[FATDIR_CRTTIMETENTH] = de->crttimetenth;
                    eptr[FATDIR_CRTDATE] = de->crtdate & 0xff;
                    eptr[FATDIR_CRTDATE+1] = de->crtdate >> 8;
                    eptr[FATDIR_CRTTIME] = de->crttime & 0xff;
                    eptr[FATDIR_CRTTIME+1] = de->crttime >> 8;
                    
                    eptr[FATDIR_WRTDATE] = de->wrtdate & 0xff;
                    eptr[FATDIR_WRTDATE+1] = de->wrtdate >> 8;
                    eptr[FATDIR_WRTTIME] = de->wrttime & 0xff;
                    eptr[FATDIR_WRTTIME+1] = de->wrttime >> 8;
                    
                    eptr[FATDIR_FILESIZE] = de->filesize & 0xff;
                    eptr[FATDIR_FILESIZE+1] = (de->filesize >> 8) & 0xff;
                    eptr[FATDIR_FILESIZE+2] = (de->filesize >> 16) & 0xff;
                    eptr[FATDIR_FILESIZE+3] = (de->filesize >> 24) & 0xff;
                    
                    /* Advance the last_empty_entry marker */
                    if(firstbyte == 0)
                    {
                        i += 32;
                        if(i < SECTOR_SIZE)
                        {
                            buf[i] = 0;
                            /* We are done */
                            done = 1;
                        }
                        else
                        {
                            /* We must fill in the first entry
                               in the next sector */
                            need_to_update_last_empty_marker = 1;
                        }
                    }

                    err = ata_write_sectors(sec + fat_bpb.startsector,1,buf);
                    if(err)
                    {
                        DEBUGF( "add_dir_entry() - "
                                " Couldn't write dir"
                                " sector (error code %d)\n", err);
                        return -1;
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

    if(strlen(name) > 12)
    {
        return -1;
    }

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
        return -1;
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
    for(i = 0;ext && ext[i] && (i < 3);i++)
    {
        c = char2dos(ext[i]);
        if(c)
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

int fat_create_dir(unsigned int currdir, char *name)
{
    struct fat_direntry de;
    int err;

    DEBUGF("fat_create_file()\n");
    memset(&de, 0, sizeof(struct fat_direntry));
    if(create_dos_name(name, de.name) < 0)
    {
        DEBUGF( "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }

    getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    de.attr = FAT_ATTR_DIRECTORY;
    
    err = add_dir_entry(currdir, &de);
    return 0;
}

int fat_create_file(unsigned int currdir, char *name)
{
    struct fat_direntry de;
    int err;

    DEBUGF("fat_create_file()\n");
    memset(&de, 0, sizeof(struct fat_direntry));
    if(create_dos_name(name, de.name) < 0)
    {
        DEBUGF( "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }
    getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    
    err = add_dir_entry(currdir, &de);
    return err;
}
#endif

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

int fat_open( unsigned int startcluster, struct fat_file *file)
{
    file->firstcluster = startcluster;
    file->nextcluster = startcluster;
    file->nextsector = cluster2sec(startcluster);
    file->sectornum = 0;
    return 0;
}

int fat_read( struct fat_file *file, int sectorcount, void* buf )
{
    int cluster = file->nextcluster;
    int sector = file->nextsector;
    int numsec = file->sectornum;
    int err, i;

    if ( sector == -1 )
        return 0;

    for ( i=0; i<sectorcount; i++ ) {
        err = ata_read_sectors(sector + fat_bpb.startsector, 1,
                               (char*)buf+(i*SECTOR_SIZE));
        if(err) {
            DEBUGF( "fat_read() - Couldn't read sector %d"
                    " (error code %d)\n", sector,err);
            return -1;
        }

        numsec++;
        if ( numsec >= fat_bpb.bpb_secperclus ) {
            cluster = get_next_cluster(cluster);
            if (!cluster) {
                /* end of file */
                sector = -1;
                break;
            }
            
            sector = cluster2sec(cluster);
            if (sector<0)
                return -1;
            numsec=0;
        }
        else
            sector++;
    }
    file->nextcluster = cluster;
    file->nextsector = sector;
    file->sectornum = numsec;

    return sectorcount;
}

int fat_seek(struct fat_file *file, int seeksector )
{
    int cluster = file->firstcluster;
    int sector = cluster2sec(cluster);
    int numsec = 0;
    int i;

    if ( seeksector ) {
        for (i=0; i<seeksector; i++) {
            numsec++;
            if ( numsec >= fat_bpb.bpb_secperclus ) {
                cluster = get_next_cluster(cluster);
                if (!cluster)
                    /* end of file */
                    return -1; 
                
                sector = cluster2sec(cluster);
                if (sector<0)
                    return -2;
                numsec=0;
            }
            else
                sector++;
        }
    }
        
    file->nextcluster = cluster;
    file->nextsector = sector;
    file->sectornum = numsec;
    return 0;
}

int fat_opendir(struct fat_dir *dir, unsigned int currdir)
{
    int is_rootdir = (currdir == 0);
    unsigned int sec;
    int err;

    if(is_rootdir)
    {
        sec = fat_bpb.rootdirsector;
    }
    else
    {
        sec = first_sector_of_cluster(currdir);
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

    return 0;
}

int fat_getnext(struct fat_dir *dir, struct fat_direntry *entry)
{
    int done = 0;
    int i;
    int err;
    unsigned char firstbyte;
    int longarray[20];
    int longs=0;
    int sectoridx=0;

    while(!done)
    {
        for(i = dir->entry;i < SECTOR_SIZE/32;i++)
        {
            firstbyte = dir->cached_buf[i*32];

            if(firstbyte == 0xe5)
                /* free entry */
                continue;

            if(firstbyte == 0) {
                /* last entry */
                entry->name[0] = 0;
                return 0;
            }

            /* longname entry? */
            if ( ( dir->cached_buf[i*32 + FATDIR_ATTR] &
                   FAT_ATTR_LONG_NAME_MASK ) == FAT_ATTR_LONG_NAME ) {
                longarray[longs++] = i*32 + sectoridx;
            }
            else {
                if ( parse_direntry(entry, &dir->cached_buf[i*32]) ) {

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

                            /* piece together the name subcomponents.
                               names are stored in unicode, but we
                               only grab the low byte (iso8859-1).
			     */
                            for (k=0; k<5; k++)
                                entry->name[l++] = ptr[index + k*2 + 1];
                            for (k=0; k<6; k++)
                                entry->name[l++] = ptr[index + k*2 + 14];
                            for (k=0; k<2; k++)
                                entry->name[l++] = ptr[index + k*2 + 28];
                        }
                        entry->name[l]=0;
                    }
                    done = 1;
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
        if(i < SECTOR_SIZE/32)
        {
            i++;
        }
        else
        {
            dir->num_sec++;

            /* Do we need to advance one cluster? */
            if(dir->num_sec < fat_bpb.bpb_secperclus)
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
                if(!cluster)
                {
                    DEBUGF("End of cluster chain.\n");
                    return -1;
                }
                dir->cached_sec = cluster2sec(cluster);
                if ( dir->cached_sec < 0 )
                {
                    DEBUGF("Invalid cluster: %d\n",cluster);
                    return -1;
                }

            }
   
            /* Read the next sector */
            err = ata_read_sectors(dir->cached_sec + fat_bpb.startsector, 1,
                                   dir->cached_buf);
            if(err)
            {
                DEBUGF( "fat_getnext() - Couldn't read dir sector"
                        " (error code %d)\n", err);
                return -1;
            }

            i = 0;
        }
        dir->entry = i;
    }
    return 0;
}
