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

#include "fat.h"
#include "ata.h"

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

static int first_sector_of_cluster(struct bpb *bpb, unsigned int cluster);
static int get_bpb(struct bpb *bpb);
static int bpb_is_sane(struct bpb *bpb);
static void *cache_fat_sector(struct bpb *bpb, int secnum);
#ifdef DISK_WRITE
static unsigned int getcurrdostime(unsigned short *dosdate,
                                   unsigned short *dostime,
                                   unsigned char *dostenth);
static int create_dos_name(unsigned char *name, unsigned char *newname);
#endif

/* fat cache */
static unsigned char *fat_cache[256];
static int fat_cache_dirty[256];

/* sectors cache for longname use */
static unsigned char lastsector[SECTOR_SIZE];
static unsigned char lastsector2[SECTOR_SIZE];

#ifdef TEST_FAT

#include "debug.h"
#define DEBUG(x) printf(x)
#define DEBUG1(x,y) printf(x,y)
#define DEBUG2(x,y1,y2) printf(x,y1,y2)
#define DEBUG3(x,y1,y2,y3) printf(x,y1,y2,y3)

int main(int argc, char *argv[])
{
    struct bpb bpb;
    
    memset(fat_cache, 0, sizeof(fat_cache));
    memset(fat_cache_dirty, 0, sizeof(fat_cache_dirty));
    
    if(ata_init())
        DEBUG("*** Warning! The disk is uninitialized\n");
    else
        get_bpb(&bpb);

    dbg_console(&bpb);
    return 0;
}
#else
#define DEBUG(x);
#define DEBUG1(x,y);
#define DEBUG2(x,y1,y2);
#define DEBUG3(x,y1,y2,y3);
#endif

static int sec2cluster(struct bpb *bpb, unsigned int sec)
{
    if ( sec < bpb->firstdatasector )
    {
        DEBUG1( "sec2cluster() - Bad sector number (%d)\n", sec);
        return -1;
    }

    return ((sec - bpb->firstdatasector) / bpb->bpb_secperclus) + 2;
}

static int cluster2sec(struct bpb *bpb, unsigned int cluster)
{
    int max_cluster = bpb->totalsectors -
        bpb->firstdatasector / bpb->bpb_secperclus + 1;
    
    if(cluster > max_cluster)
    {
        DEBUG1( "cluster2sec() - Bad cluster number (%d)\n",
                cluster);
        return -1;
    }

    return first_sector_of_cluster(bpb, cluster);
}

static int first_sector_of_cluster(struct bpb *bpb, unsigned int cluster)
{
    return (cluster - 2) * bpb->bpb_secperclus + bpb->firstdatasector;
}

static int get_bpb(struct bpb *bpb)
{
    unsigned char buf[SECTOR_SIZE];
    int err;
    int datasec;
    int countofclusters;

    /* Read the sector */
    err = ata_read_sectors(0,1,buf);
    if(err)
    {
        DEBUG1( "get_bpb() - Couldn't read BPB (error code %i)\n",
                err);
        return -1;
    }

    memset(bpb, 0, sizeof(struct bpb));
    
    strncpy(bpb->bs_oemname, &buf[BS_OEMNAME], 8);
    bpb->bs_oemname[8] = 0;

    bpb->bpb_bytspersec = BYTES2INT16(buf,BPB_BYTSPERSEC);
    bpb->bpb_secperclus = buf[BPB_SECPERCLUS];
    bpb->bpb_rsvdseccnt = BYTES2INT16(buf,BPB_RSVDSECCNT);
    bpb->bpb_numfats    = buf[BPB_NUMFATS];
    bpb->bpb_rootentcnt = BYTES2INT16(buf,BPB_ROOTENTCNT);
    bpb->bpb_totsec16   = BYTES2INT16(buf,BPB_TOTSEC16);
    bpb->bpb_media      = buf[BPB_MEDIA];
    bpb->bpb_fatsz16    = BYTES2INT16(buf,BPB_FATSZ16);
    bpb->bpb_fatsz32    = BYTES2INT32(buf,BPB_FATSZ32);
    bpb->bpb_secpertrk  = BYTES2INT16(buf,BPB_SECPERTRK);
    bpb->bpb_numheads   = BYTES2INT16(buf,BPB_NUMHEADS);
    bpb->bpb_hiddsec    = BYTES2INT32(buf,BPB_HIDDSEC);
    bpb->bpb_totsec32   = BYTES2INT32(buf,BPB_TOTSEC32);
    bpb->last_word      = BYTES2INT16(buf,BPB_LAST_WORD);

    /* calculate a few commonly used values */
    if (bpb->bpb_fatsz16 != 0)
        bpb->fatsize = bpb->bpb_fatsz16;
    else
        bpb->fatsize = bpb->bpb_fatsz32;

    if (bpb->bpb_totsec16 != 0)
        bpb->totalsectors = bpb->bpb_totsec16;
    else
        bpb->totalsectors = bpb->bpb_totsec32;
    bpb->firstdatasector = bpb->bpb_rsvdseccnt +
        bpb->bpb_numfats * bpb->fatsize;

    /* Determine FAT type */
    datasec = bpb->totalsectors - bpb->firstdatasector;
    countofclusters = datasec / bpb->bpb_secperclus;

#ifdef TEST_FAT
    /*
      we are sometimes testing with "illegally small" fat32 images,
      so we don't use the proper fat32 test case for test code
    */
    if ( bpb->bpb_fatsz16 )
#else
    if ( countofclusters < 65525 )
#endif
    {
        DEBUG("This is not FAT32. Go away!\n");
        return -1;
    }

    bpb->bpb_extflags  = BYTES2INT16(buf,BPB_EXTFLAGS);
    bpb->bpb_fsver     = BYTES2INT16(buf,BPB_FSVER);
    bpb->bpb_rootclus  = BYTES2INT32(buf,BPB_ROOTCLUS);
    bpb->bpb_fsinfo    = BYTES2INT16(buf,BPB_FSINFO);
    bpb->bpb_bkbootsec = BYTES2INT16(buf,BPB_BKBOOTSEC);
    bpb->bs_drvnum     = buf[BS_32_DRVNUM];
    bpb->bs_bootsig    = buf[BS_32_BOOTSIG];

    if(bpb->bs_bootsig == 0x29)
    {
        bpb->bs_volid = BYTES2INT32(buf,BS_32_VOLID);
        strncpy(bpb->bs_vollab, &buf[BS_32_VOLLAB], 11);
        strncpy(bpb->bs_filsystype, &buf[BS_32_FILSYSTYPE], 8);
    }

    if (bpb_is_sane(bpb) < 0)
    {
        DEBUG( "get_bpb() - BPB is not sane\n");
        return -1;
    }

    bpb->rootdirsector = cluster2sec(bpb,bpb->bpb_rootclus);

    return 0;
}

static int bpb_is_sane(struct bpb *bpb)
{
    if(bpb->bpb_bytspersec != 512)
    {
        DEBUG1( "bpb_is_sane() - Error: sector size is not 512 (%i)\n",
                bpb->bpb_bytspersec);
        return -1;
    }
    if(bpb->bpb_secperclus * bpb->bpb_bytspersec > 32768)
    {
        DEBUG3( "bpb_is_sane() - Warning: cluster size is larger than 32K "
                "(%i * %i = %i)\n",
                bpb->bpb_bytspersec, bpb->bpb_secperclus,
                bpb->bpb_bytspersec * bpb->bpb_secperclus);
    }
    if(bpb->bpb_rsvdseccnt != 1)
    {
        DEBUG1( "bpb_is_sane() - Warning: Reserved sectors is not 1 (%i)\n",
                bpb->bpb_rsvdseccnt);
    }
    if(bpb->bpb_numfats != 2)
    {
        DEBUG1( "bpb_is_sane() - Warning: NumFATS is not 2 (%i)\n",
                bpb->bpb_numfats);
    }
    if(bpb->bpb_rootentcnt != 512)
    {
        DEBUG1( "bpb_is_sane() - Warning: RootEntCnt is not 512 (%i)\n",
                bpb->bpb_rootentcnt);
    }
    if(bpb->bpb_totsec16 < 200)
    {
        if(bpb->bpb_totsec16 == 0)
        {
            DEBUG( "bpb_is_sane() - Error: TotSec16 is 0\n");
            return -1;
        }
        else
        {
            DEBUG1( "bpb_is_sane() - Warning: TotSec16 "
                    "is quite small (%i)\n",
                    bpb->bpb_totsec16);
        }
    }
    if(bpb->bpb_media != 0xf0 && bpb->bpb_media < 0xf8)
    {
        DEBUG1( "bpb_is_sane() - Warning: Non-standard "
                "media type (0x%02x)\n",
                bpb->bpb_media);
    }
    if(bpb->last_word != 0xaa55)
    {
        DEBUG1( "bpb_is_sane() - Error: Last word is not "
                "0xaa55 (0x%04x)\n", bpb->last_word);
        return -1;
    }
    return 0;
}

static void *cache_fat_sector(struct bpb *bpb, int secnum)
{
    unsigned char *sec;

    sec = fat_cache[secnum];
    /* Load the sector if it is not cached */
    if(!sec)
    {
        sec = malloc(bpb->bpb_bytspersec);
        if(!sec)
        {
            DEBUG( "cache_fat_sector() - Out of memory\n");
            return NULL;
        }
        if(ata_read_sectors(secnum,1,sec))
        {
            DEBUG1( "cache_fat_sector() - Could"
                    " not read sector %d\n",
                    secnum);
            free(sec);
            return NULL;
        }
        fat_cache[secnum] = sec;
    }
    return sec;
}

#ifdef DISK_WRITE
static int update_entry(struct bpb *bpb, int entry, unsigned int val)
{
    unsigned long *sec;
    int fatoffset;
    int thisfatsecnum;
    int thisfatentoffset;

    fatoffset = entry * 4;
    thisfatsecnum = fatoffset / bpb->bpb_bytspersec + bpb->bpb_rsvdseccnt;
    thisfatentoffset = fatoffset % bpb->bpb_bytspersec;

    /* Load the sector if it is not cached */
    sec = cache_fat_sector(bpb, thisfatsecnum);
    if(!sec)
    {
        DEBUG1( "update_entry() - Could not cache sector %d\n",
                thisfatsecnum);
        return -1;
    }

    fat_cache_dirty[thisfatsecnum] = 1;

    /* don't change top 4 bits */
    sec[thisfatentoffset/sizeof(int)] &= 0xf000000;
    sec[thisfatentoffset/sizeof(int)] |= val & 0x0fffffff;

    return 0;
}
#endif

static int read_entry(struct bpb *bpb, int entry)
{
    unsigned long *sec;
    int fatoffset;
    int thisfatsecnum;
    int thisfatentoffset;
    int val = -1;

    fatoffset = entry * 4;
    thisfatsecnum = fatoffset / bpb->bpb_bytspersec + bpb->bpb_rsvdseccnt;
    thisfatentoffset = fatoffset % bpb->bpb_bytspersec;

    /* Load the sector if it is not cached */
    sec = cache_fat_sector(bpb, thisfatsecnum);
    if(!sec)
    {
        DEBUG1( "update_entry() - Could not cache sector %d\n",
                thisfatsecnum);
        return -1;
    }

    val = sec[thisfatentoffset/sizeof(int)];
    return val;
}

static int get_next_cluster(struct bpb *bpb, unsigned int cluster)
{
    int next_cluster = read_entry(bpb, cluster);

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
    int fatsz;
    unsigned short d, t;
    char m;

    fatsz = bpb->fatsize;

    for(i = 0;i < 256;i++)
    {
        if(fat_cache[i] && fat_cache_dirty[i])
        {
            DEBUG1("Flushing FAT sector %d\n", i);
            sec = fat_cache[i];
            err = ata_write_sectors(i + bpb->bpb_rsvdseccnt,1,sec);
            if(err)
            {
                DEBUG1( "flush_fat() - Couldn't write"
                        " sector (%d)\n", i + bpb->bpb_rsvdseccnt);
                return -1;
            }
            err = ata_write_sectors(i + bpb->bpb_rsvdseccnt + fatsz,1,sec);
            if(err)
            {
                DEBUG1( "flush_fat() - Couldn't write"
                        " sector (%d)\n", i + bpb->bpb_rsvdseccnt + fatsz);
                return -1;
            }
            fat_cache_dirty[i] = 0;
        }
    }

    getcurrdostime(&d, &t, &m);
    return 0;
}

unsigned int getcurrdostime(unsigned short *dosdate,
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

static int add_dir_entry(struct bpb *bpb, 
                         unsigned int currdir,
                         struct fat_direntry *de)
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
        sec = bpb->rootdirsector;
    }
    else
    {
        sec = first_sector_of_cluster(bpb, currdir);
    }

    sec_cnt = 0;

    while(!done)
    {
        /* The root dir has a fixed size */
        if(is_rootdir)
        {
            if(sec_cnt >= bpb->bpb_rootentcnt * 32 / bpb->bpb_bytspersec)
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
                    DEBUG( "add_dir_entry() -"
                            " Root dir is full\n");
                    return -1;
                }
            }
        }
        else
        {
            if(sec_cnt >= bpb->bpb_secperclus)
            {
                /* We have reached the end of this cluster */
                DEBUG("Moving to the next cluster...");
                currdir = get_next_cluster(bpb, currdir);
                DEBUG1("new cluster is %d\n", currdir);
                
                if(!currdir)
                {
                    /* This was the last in the chain,
                       we have to allocate a new cluster */
                    /* TODO */
                }
            }
        }

        DEBUG1("Reading sector %d...\n", sec);
        /* Read the next sector in the current dir */
        err = ata_read_sectors(sec,1,buf);
        if(err)
        {
            DEBUG1( "add_dir_entry() - Couldn't read dir sector"
                    " (error code %i)\n", err);
            return -1;
        }

        if(need_to_update_last_empty_marker)
        {
            /* All we need to do is to set the first entry to 0 */
            DEBUG1("Clearing the first entry in sector %d\n", sec);
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
                    DEBUG2("Found free slot at entry %d in sector %d\n",
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

                    err = ata_write_sectors(sec,1,buf);
                    if(err)
                    {
                        DEBUG1( "add_dir_entry() - "
                                " Couldn't write dir"
                                " sector (error code %i)\n", err);
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

int fat_create_dir(struct bpb *bpb, unsigned int currdir, char *name)
{
    struct fat_direntry de;
    int err;

    DEBUG("fat_create_file()\n");
    memset(&de, 0, sizeof(struct fat_direntry));
    if(create_dos_name(name, de.name) < 0)
    {
        DEBUG1( "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }

    getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    de.attr = FAT_ATTR_DIRECTORY;
    
    err = add_dir_entry(bpb, currdir, &de);
    return 0;
}

int fat_create_file(struct bpb *bpb, unsigned int currdir, char *name)
{
    struct fat_direntry de;
    int err;

    DEBUG("fat_create_file()\n");
    memset(&de, 0, sizeof(struct fat_direntry));
    if(create_dos_name(name, de.name) < 0)
    {
        DEBUG1( "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }
    getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    
    err = add_dir_entry(bpb, currdir, &de);
    return err;
}
#endif

static int parse_direntry(struct fat_direntry *de, unsigned char *buf)
{
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
    strncpy(de->name, &buf[FATDIR_NAME], 11);

    return 1;
}

int fat_open(struct bpb *bpb, 
             unsigned int startcluster,
             struct fat_fileent *ent)
{
    ent->firstcluster = startcluster;
    ent->nextcluster = startcluster;
    ent->nextsector = cluster2sec(bpb,startcluster);
    ent->sectornum = 0;
    return 0;
}

int fat_read(struct bpb *bpb, 
             struct fat_fileent *ent,
             int sectorcount,
             void* buf )
{
    int cluster = ent->nextcluster;
    int sector = ent->nextsector;
    int numsec = ent->sectornum;
    int err, i;

    for ( i=0; i<sectorcount; i++ ) {
        err = ata_read_sectors(sector,1,(char*)buf+(i*SECTOR_SIZE));
        if(err) {
            DEBUG2( "fat_read() - Couldn't read sector %d"
                    " (error code %i)\n", sector,err);
            return -1;
        }

        numsec++;
        if ( numsec >= bpb->bpb_secperclus ) {
            cluster = get_next_cluster(bpb,cluster);
            if (!cluster)
                break; /* end of file */
            
            sector = cluster2sec(bpb,cluster);
            if (sector<0)
                return -1;
            numsec=0;
        }
        else
            sector++;
    }
    ent->nextcluster = cluster;
    ent->nextsector = sector;
    ent->sectornum = numsec;

    return sectorcount;
}

int fat_seek(struct bpb *bpb, 
             struct fat_fileent *ent,
             int seeksector )
{
    int cluster = ent->firstcluster;
    int sector;
    int numsec = 0;
    int i;

    for (i=0; i<seeksector; i++) {
        numsec++;
        if ( numsec >= bpb->bpb_secperclus ) {
            cluster = get_next_cluster(bpb,cluster);
            if (!cluster)
                /* end of file */
                return -1; 
            
            sector = cluster2sec(bpb,cluster);
            if (sector<0)
                return -2;
            numsec=0;
        }
    }
    ent->nextcluster = cluster;
    ent->nextsector = sector;
    ent->sectornum = numsec;
    return 0;
}

int fat_opendir(struct bpb *bpb, 
                struct fat_dirent *ent, 
                unsigned int currdir)
{
    int is_rootdir = (currdir == 0);
    unsigned int sec;
    int err;

    if(is_rootdir)
    {
        sec = bpb->rootdirsector;
    }
    else
    {
        sec = first_sector_of_cluster(bpb, currdir);
    }

    /* Read the first sector in the current dir */
    err = ata_read_sectors(sec,1,ent->cached_buf);
    if(err)
    {
        DEBUG1( "fat_getfirst() - Couldn't read dir sector"
                " (error code %i)\n", err);
        return -1;
    }

    ent->entry = 0;
    ent->cached_sec = sec;
    ent->num_sec = 0;

    return 0;
}

int fat_getnext(struct bpb *bpb, 
                struct fat_dirent *ent,
                struct fat_direntry *entry)
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
        for(i = ent->entry;i < SECTOR_SIZE/32;i++)
        {
            firstbyte = ent->cached_buf[i*32];

            if(firstbyte == 0xe5)
                /* free entry */
                continue;

            if(firstbyte == 0)
                /* last entry */
                return -1;

            /* longname entry? */
            if ( ( ent->cached_buf[i*32 + FATDIR_ATTR] &
                   FAT_ATTR_LONG_NAME_MASK ) == FAT_ATTR_LONG_NAME ) {
                longarray[longs++] = i*32 + sectoridx;
            }
            else {
                if ( parse_direntry(entry, &ent->cached_buf[i*32]) ) {

                    /* replace shortname with longname? */
                    if ( longs ) {
                        int j,k,l=0;

                        /* iterate backwards through the dir entries */
                        for (j=longs-1; j>=0; j--) {
                            unsigned char* ptr = ent->cached_buf;
                            int index = longarray[j];
                            
                            /* current or cached sector? */
                            if ( sectoridx >= SECTOR_SIZE ) {
                                if ( sectoridx >= SECTOR_SIZE*2 ) {
                                    if ( index >= SECTOR_SIZE ) {
                                        if ( index >= SECTOR_SIZE*2 )
                                            ptr = ent->cached_buf;
                                        else
                                            ptr = lastsector;
                                    }
                                    else
                                        ptr = lastsector2;
                                }
                                else {
                                    if ( index < SECTOR_SIZE )
                                        ptr = lastsector;
                                }

                                index &= SECTOR_SIZE-1;
                            }

                            /* piece together the name subcomponents */
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
            memcpy( lastsector2, ent->cached_buf, SECTOR_SIZE );
        else
            memcpy( lastsector, ent->cached_buf, SECTOR_SIZE );
        sectoridx += SECTOR_SIZE;

        /* Next sector? */
        if(i < SECTOR_SIZE/32)
        {
            i++;
        }
        else
        {
            ent->num_sec++;

            /* Do we need to advance one cluster? */
            if(ent->num_sec < bpb->bpb_secperclus)
            {
                ent->cached_sec++;
            }
            else
            {
                int cluster = sec2cluster(bpb, ent->cached_sec);
                if ( cluster < 0 ) {
                    DEBUG("sec2cluster failed\n");
                    return -1;
                }
                ent->num_sec = 0;
                cluster = get_next_cluster( bpb, cluster );
                if(!cluster)
                {
                    DEBUG("End of cluster chain.\n");
                    return -1;
                }
                ent->cached_sec = cluster2sec(bpb,cluster);
                if ( ent->cached_sec < 0 )
                {
                    DEBUG1("Invalid cluster: %d\n",cluster);
                    return -1;
                }

            }
   
            /* Read the next sector */
            err = ata_read_sectors(ent->cached_sec,1,ent->cached_buf);
            if(err)
            {
                DEBUG1( "fat_getnext() - Couldn't read dir sector"
                        " (error code %i)\n", err);
                return -1;
            }

            i = 0;
        }
        ent->entry = i;
    }
    return 0;
}
