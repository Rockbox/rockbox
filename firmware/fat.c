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

#define BLOCK_SIZE 512

#include "fat.h"
#include "ata.h"

#define NUM_ROOT_DIR_ENTRIES 512
#define NUM_FATS 2
#define NUM_RESERVED_SECTORS 1
#define NUM_BLOCKS 10000

struct dsksz2secperclus
{
        unsigned int disk_size;
        unsigned int sec_per_cluster;
};

/*
** This is the table for FAT16 drives. NOTE that this table includes
** entries for disk sizes larger than 512 MB even though typically
** only the entries for disks < 512 MB in size are used.
** The way this table is accessed is to look for the first entry
** in the table for which the disk size is less than or equal
** to the DiskSize field in that table entry. For this table to
** work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
** must be 2, and BPB_RootEntCnt must be 512. Any of these values
** being different may require the first table entries DiskSize value
** to be changed otherwise the cluster count may be to low for FAT16.
*/
struct dsksz2secperclus dsk_table_fat16 [] =
{
    { 8400, 0}, /* disks up to 4.1 MB, the 0 value for SecPerClusVal
                   trips an error */
    { 32680, 2}, /* disks up to 16 MB, 1k cluster */
    { 262144, 4}, /* disks up to 128 MB, 2k cluster */
    { 524288, 8}, /* disks up to 256 MB, 4k cluster */
    { 1048576, 16}, /* disks up to 512 MB, 8k cluster */
/* The entries after this point are not used unless FAT16 is forced */
    { 2097152, 32}, /* disks up to 1 GB, 16k cluster */
    { 4194304, 64}, /* disks up to 2 GB, 32k cluster */
    { 0xFFFFFFFF, 0} /* any disk greater than 2GB,
                        0 value for SecPerClusVal trips an error */
};

int fat_num_rootdir_sectors(struct bpb *bpb);
int fat_first_sector_of_cluster(struct bpb *bpb, unsigned int cluster);
int fat_get_fatsize(struct bpb* bpb);
int fat_get_totsec(struct bpb* bpb);
int fat_get_rootdir_sector(struct bpb *bpb);
int fat_first_data_sector(struct bpb* bpb);
int fat_get_bpb(struct bpb *bpb);
int fat_bpb_is_sane(struct bpb *bpb);
int fat_create_fat(struct bpb* bpb);
int fat_dbg_read_block(char *name, unsigned char *buf);
int fat_flush_fat(struct bpb *bpb);
unsigned char *fat_cache_fat_sector(struct bpb *bpb, int secnum);
int fat_update_entry(struct bpb *bpb, int entry, unsigned int val);
unsigned int fat_getcurrdostime(unsigned short *dosdate,
                                unsigned short *dostime,
                                unsigned char *dostenth);
int fat_create_root_dir(struct bpb *bpb);
int fat_create_dos_name(unsigned char *name, unsigned char *newname);
int fat_create_file(struct bpb *bpb, unsigned int currdir, char *name);

unsigned char *fat_cache[256];
int fat_cache_dirty[256];
char current_directory[256] = "\\";
struct bpb *global_bpb;
struct disk_info di;

extern int yyparse(void);


#ifdef TEST_FAT
void prompt(void)
{
   printf("C:%s>", current_directory);
}

int main(int argc, char *argv[])
{
    struct bpb bpb;
    
    memset(fat_cache, 0, sizeof(fat_cache));
    memset(fat_cache_dirty, 0, sizeof(fat_cache_dirty));
    
    disk_init(NUM_BLOCKS);

    di.num_sectors = NUM_BLOCKS;
    di.sec_per_track = 40;
    di.num_heads = 250;
    di.hidden_sectors = 0;

    if(read_disk("diskdump.dmp") < 0)
    {
        printf("*** Warning! The disk is uninitialized\n");
    }
    else
    {
        fat_get_bpb(&bpb);
    }

    global_bpb = &bpb;
    prompt();
    yyparse();

    dump_disk("diskdump.dmp");
    return 0;
}
#endif

int fat_sec2cluster(struct bpb *bpb, unsigned int sec)
{
    int first_sec = fat_first_data_sector(bpb);
    
    if(sec < first_sec)
    {
        fprintf(stderr, "fat_sec2cluster() - Bad sector number (%d)\n", sec);
        return -1;
    }

    return ((sec - first_sec) / bpb->bpb_secperclus) + 2;
}

int fat_last_cluster_in_chain(struct bpb *bpb, unsigned int cluster)
{
    int iseof = 0;

    switch(bpb->fat_type)
    {
        case FATTYPE_FAT12:
            if(cluster >= 0x0ff8)
                iseof = 1;
            break;
        case FATTYPE_FAT16:
            if(cluster >= 0xfff8)
                iseof = 1;
            break;
        case FATTYPE_FAT32:
            if(cluster >= 0x0ffffff8)
                iseof = 1;
            break;
    }
    return iseof;
}

int fat_cluster2sec(struct bpb *bpb, unsigned int cluster)
{
    int max_cluster = (fat_get_totsec(bpb) - fat_first_data_sector(bpb)) /
        bpb->bpb_secperclus + 1;
    
    if(cluster > max_cluster)
    {
        fprintf(stderr, "fat_cluster2sec() - Bad cluster number (%d)\n",
                cluster);
        return -1;
    }

    return fat_first_sector_of_cluster(bpb, cluster);
}

int fat_first_sector_of_cluster(struct bpb *bpb, unsigned int cluster)
{
    return (cluster - 2) * bpb->bpb_secperclus + fat_first_data_sector(bpb);
}

int fat_num_rootdir_sectors(struct bpb *bpb)
{
    return ((bpb->bpb_rootentcnt * 32) + (bpb->bpb_bytspersec - 1)) /
           bpb->bpb_bytspersec;
}

int fat_get_fatsize(struct bpb* bpb)
{
    if(bpb->bpb_fatsz16 != 0)
        return bpb->bpb_fatsz16;
    else
        return bpb->bpb_fatsz32;
}

int fat_get_totsec(struct bpb* bpb)
{
    if(bpb->bpb_totsec16 != 0)
        return bpb->bpb_totsec16;
    else
        return bpb->bpb_totsec32;
}

int fat_get_rootdir_sector(struct bpb *bpb)
{
    return bpb->bpb_rsvdseccnt + bpb->bpb_numfats * fat_get_fatsize(bpb);
}

int fat_first_data_sector(struct bpb* bpb)
{
    int fatsz;
    int rootdirsectors;

    fatsz = fat_get_fatsize(bpb);

    rootdirsectors = fat_num_rootdir_sectors(bpb);
    
    return bpb->bpb_rsvdseccnt + bpb->bpb_numfats * fatsz + rootdirsectors;
}

int fat_format(struct disk_info *di, char *vol_name)
{
    unsigned char buf[BLOCK_SIZE];
    struct bpb bpb;
    unsigned int root_dir_sectors;
    unsigned int tmp1, tmp2;
    int sec_per_clus = 0;
    int fat_size;
    int i = 0;
    int err;

    while(di->num_sectors > dsk_table_fat16[i].disk_size)
    {
        i++;
    }

    sec_per_clus = dsk_table_fat16[i].sec_per_cluster;
    
    if(sec_per_clus == 0)
    {
        fprintf(stderr, "fat_format() - Bad disk size (%u)\n",
                di->num_sectors);
        return -1;
    }

    /* First calculate how many sectors we need for
       the root directory */
    root_dir_sectors = ((NUM_ROOT_DIR_ENTRIES * 32) +
                        (BLOCK_SIZE - 1)) / BLOCK_SIZE;

    /* Now calculate the FAT size */
    tmp1 = di->num_sectors - (NUM_RESERVED_SECTORS + root_dir_sectors);
    tmp2 = (256 * sec_per_clus) + NUM_FATS;

    fat_size = (tmp1 + (tmp2 - 1)) / tmp2;

    /* Now create the BPB. We must be careful, so we really make
       it little endian. */
    memset(buf, 0xff, BLOCK_SIZE);

    strncpy(&buf[BS_OEMNAME], "MSWIN4.1", 8);
    buf[BPB_BYTSPERSEC] = BLOCK_SIZE & 0xff;
    buf[BPB_BYTSPERSEC+1] = BLOCK_SIZE >> 8;
    buf[BPB_SECPERCLUS] = sec_per_clus;
    buf[BPB_RSVDSECCNT] = 1;
    buf[BPB_RSVDSECCNT+1] = 0;
    buf[BPB_NUMFATS] = 2;
    buf[BPB_ROOTENTCNT] = NUM_ROOT_DIR_ENTRIES & 0xff;
    buf[BPB_ROOTENTCNT+1] = NUM_ROOT_DIR_ENTRIES >> 8;
    buf[BPB_TOTSEC16] = di->num_sectors & 0xff;
    buf[BPB_TOTSEC16+1] = di->num_sectors >> 8;
    buf[BPB_MEDIA] = 0xf0;
    buf[BPB_FATSZ16] = fat_size & 0xff;
    buf[BPB_FATSZ16+1] = fat_size >> 8;
    buf[BPB_SECPERTRK] = di->sec_per_track & 0xff;
    buf[BPB_SECPERTRK+1] = di->sec_per_track >> 8;
    buf[BPB_NUMHEADS] = di->num_heads & 0xff;
    buf[BPB_NUMHEADS+1] = di->num_heads >> 8;
    buf[BPB_HIDDSEC] = di->hidden_sectors & 0xff;
    buf[BPB_HIDDSEC+1] = (di->hidden_sectors >> 8) & 0xff;
    buf[BPB_HIDDSEC+2] = (di->hidden_sectors >> 16) & 0xff;
    buf[BPB_HIDDSEC+3] = (di->hidden_sectors >> 24) & 0xff;
    buf[BPB_TOTSEC32] = 0;
    buf[BPB_TOTSEC32+1] = 0;
    buf[BPB_TOTSEC32+2] = 0;
    buf[BPB_TOTSEC32+3] = 0;

    buf[BS_DRVNUM] = 0;
    buf[BS_RESERVED1] = 0;
    buf[BS_BOOTSIG] = 0x29;
    buf[BS_VOLID] = 0x78;
    buf[BS_VOLID+1] = 0x56;
    buf[BS_VOLID+2] = 0x34;
    buf[BS_VOLID+3] = 0x12;
    memset(&buf[BS_VOLLAB], ' ', 11);
    strncpy(&buf[BS_VOLLAB], vol_name, MIN(11, strlen(vol_name));
    strncpy(&buf[BS_FILSYSTYPE], "FAT16   ", 8);

    /* The final signature */
    buf[BPB_LAST_WORD] = 0x55;
    buf[BPB_LAST_WORD+1] = 0xaa;

    /* Now write the sector to disk */
    err = ata_write_sectors(0,1,buf);
    if(err)
    {
        fprintf(stderr, "fat_format() - Couldn't write BSB (error code %i)\n",
                err);
        return -1;
    }

    if(fat_get_bpb(&bpb) < 0)
    {
        fprintf(stderr, "fat_format() - Couldn't read BPB\n");
        return -1;
    }

    if(fat_create_fat(&bpb) < 0)
    {
        fprintf(stderr, "fat_format() - Couldn't create FAT\n");
        return -1;
    }
    
    if(fat_create_root_dir(&bpb) < 0)
    {
        fprintf(stderr, "fat_format() - Couldn't write root dir sector\n");
        return -1;
    }
    
    return 0;
}

int fat_get_bpb(struct bpb *bpb)
{
    unsigned char buf[BLOCK_SIZE];
    int err;
    int fatsz;
    int rootdirsectors;
    int totsec;
    int datasec;
    int countofclusters;

    /* Read the sector */
    err = ata_read_sectors(0,1,buf);
    if(err)
    {
        fprintf(stderr, "fat_get_bpb() - Couldn't read BPB (error code %i)\n",
                err);
        return -1;
    }

    memset(bpb, 0, sizeof(struct bpb));
    
    strncpy(bpb->bs_oemname, &buf[BS_OEMNAME], 8);
    bpb->bs_oemname[8] = 0;

    bpb->bpb_bytspersec = buf[BPB_BYTSPERSEC] | (buf[BPB_BYTSPERSEC+1] << 8);
    bpb->bpb_secperclus = buf[BPB_SECPERCLUS];
    bpb->bpb_rsvdseccnt = buf[BPB_RSVDSECCNT] | (buf[BPB_RSVDSECCNT+1] << 8);
    bpb->bpb_numfats = buf[BPB_NUMFATS];
    bpb->bpb_rootentcnt = buf[BPB_ROOTENTCNT] | (buf[BPB_ROOTENTCNT+1] << 8);
    bpb->bpb_totsec16 = buf[BPB_TOTSEC16] | (buf[BPB_TOTSEC16+1] << 8);
    bpb->bpb_media = buf[BPB_MEDIA];
    bpb->bpb_fatsz16 = buf[BPB_FATSZ16] | (buf[BPB_FATSZ16+1] << 8);
    bpb->bpb_secpertrk = buf[BPB_SECPERTRK] | (buf[BPB_SECPERTRK+1] << 8);
    bpb->bpb_numheads = buf[BPB_NUMHEADS] | (buf[BPB_NUMHEADS+1] << 8);
    bpb->bpb_hiddsec = buf[BPB_HIDDSEC] | (buf[BPB_HIDDSEC+1] << 8) |
        (buf[BPB_HIDDSEC+2] << 16) | (buf[BPB_HIDDSEC+3] << 24);
    bpb->bpb_totsec32 = buf[BPB_TOTSEC32] | (buf[BPB_TOTSEC32+1] << 8) |
        (buf[BPB_TOTSEC32+2] << 16) | (buf[BPB_TOTSEC32+3] << 24);

    bpb->bs_drvnum = buf[BS_DRVNUM];
    bpb->bs_bootsig = buf[BS_BOOTSIG];
    if(bpb->bs_bootsig == 0x29)
    {
        bpb->bs_volid = buf[BS_VOLID] | (buf[BS_VOLID+1] << 8) |
            (buf[BS_VOLID+2] << 16) | (buf[BS_VOLID+3] << 24);
        strncpy(bpb->bs_vollab, &buf[BS_VOLLAB], 11);
        strncpy(bpb->bs_filsystype, &buf[BS_FILSYSTYPE], 8);
    }

    bpb->bpb_fatsz32 = (buf[BPB_FATSZ32] + (buf[BPB_FATSZ32+1] << 8)) |
        (buf[BPB_FATSZ32+2] << 16) | (buf[BPB_FATSZ32+3] << 24);
    
    bpb->last_word = buf[BPB_LAST_WORD] | (buf[BPB_LAST_WORD+1] << 8);

    /* Determine FAT type */
    fatsz = fat_get_fatsize(bpb);

    if(bpb->bpb_totsec16 != 0)
        totsec = bpb->bpb_totsec16;
    else
        totsec = bpb->bpb_totsec32;

    rootdirsectors = fat_num_rootdir_sectors(bpb);
    datasec = totsec - (bpb->bpb_rsvdseccnt + bpb->bpb_numfats * fatsz +
                        rootdirsectors);
    countofclusters = datasec / bpb->bpb_secperclus;

    if(countofclusters < 4085)
    {
        bpb->fat_type = FATTYPE_FAT12;
    }
    else
    {
        if(countofclusters < 65525)
        {
            bpb->fat_type = FATTYPE_FAT16;
        }
        else
        {
            bpb->fat_type = FATTYPE_FAT32;
        }
    }

    if(fat_bpb_is_sane(bpb) < 0)
    {
        fprintf(stderr, "fat_get_bpb() - BPB is not sane\n");
        return -1;
    }

    return 0;
}

int fat_bpb_is_sane(struct bpb *bpb)
{
    if(bpb->fat_type == FATTYPE_FAT32)
    {
        fprintf(stderr, "fat_bpb_is_sane() - Error: FAT32 not supported\n");
        return -1;
    }
    
    if(bpb->bpb_bytspersec != 512)
    {
        fprintf(stderr,
                "fat_bpb_is_sane() - Warning: sector size is not 512 (%i)\n",
                bpb->bpb_bytspersec);
    }
    if(bpb->bpb_secperclus * bpb->bpb_bytspersec > 32768)
    {
        fprintf(stderr,
                "fat_bpb_is_sane() - Warning: cluster size is larger than 32K "
                "(%i * %i = %i)\n",
                bpb->bpb_bytspersec, bpb->bpb_secperclus,
                bpb->bpb_bytspersec * bpb->bpb_secperclus);
    }
    if(bpb->bpb_rsvdseccnt != 1)
    {
        fprintf(stderr,
                "fat_bpb_is_sane() - Warning: Reserved sectors is not 1 (%i)\n",
                bpb->bpb_rsvdseccnt);
    }
    if(bpb->bpb_numfats != 2)
    {
        fprintf(stderr,
                "fat_bpb_is_sane() - Warning: NumFATS is not 2 (%i)\n",
                bpb->bpb_numfats);
    }
    if(bpb->bpb_rootentcnt != 512)
    {
        fprintf(stderr,
                "fat_bpb_is_sane() - Warning: RootEntCnt is not 512 (%i)\n",
                bpb->bpb_rootentcnt);
    }
    if(bpb->bpb_totsec16 < 200)
    {
        if(bpb->bpb_totsec16 == 0)
        {
            fprintf(stderr, "fat_bpb_is_sane() - Error: TotSec16 is 0\n");
            return -1;
        }
        else
        {
            fprintf(stderr,
                    "fat_bpb_is_sane() - Warning: TotSec16 "
                    "is quite small (%i)\n",
                    bpb->bpb_totsec16);
        }
    }
    if(bpb->bpb_media != 0xf0 && bpb->bpb_media < 0xf8)
    {
        fprintf(stderr,
                "fat_bpb_is_sane() - Warning: Non-standard "
                "media type (0x%02x)\n",
                bpb->bpb_media);
    }
    if(bpb->last_word != 0xaa55)
    {
        fprintf(stderr, "fat_bpb_is_sane() - Error: Last word is not "
                "0xaa55 (0x%04x)\n", bpb->last_word);
        return -1;
    }
    return 0;
}

int fat_create_fat(struct bpb* bpb)
{
    unsigned char *sec;
    int i;
    int secnum = 0;
    int fatsz;

    if(fat_bpb_is_sane(bpb) < 0)
    {
        fprintf(stderr, "fat_create_fat() - BPB is not sane\n");
        return -1;
    }

    if(bpb->bpb_fatsz16 != 0)
        fatsz = bpb->bpb_fatsz16;
    else
        fatsz = bpb->bpb_fatsz32;

    sec = fat_cache_fat_sector(bpb, secnum);
    if(!sec)
    {
        fprintf(stderr, "fat_create_fat() - Couldn't cache fat sector"
                " (%d)\n", secnum);
        return -1;
    }

    fat_cache_dirty[secnum] = 1;
    
    /* First entry should have the media type in the
       low byte and the rest of the bits set to 1.
       The second should be the EOC mark. */
    memset(sec, 0, BLOCK_SIZE);
    sec[0] = bpb->bpb_media;
    if(bpb->fat_type == FATTYPE_FAT12)
    {
        sec[1] = 0xff;
        sec[2] = 0xff;
    }
    if(bpb->fat_type == FATTYPE_FAT16)
    {
        sec[0] = bpb->bpb_media;
        sec[1] = 0xff;
        sec[2] = 0xff;
        sec[3] = 0xff;
    }
    secnum++;
        
    for(i = 0; i < fatsz - 1;i++)
    {
        sec = fat_cache_fat_sector(bpb, secnum);
        if(!sec)
        {
            fprintf(stderr, "fat_create_fat() - Couldn't cache fat sector"
                    " (%d)\n", i);
            return -1;
        }
        fat_cache_dirty[secnum] = 1;
        secnum++;
        memset(sec, 0, BLOCK_SIZE);
    }

    if(fat_flush_fat(bpb) < 0)
    {
        fprintf(stderr, "fat_create_fat() - Couldn't flush fat\n");
        return -1;
    }
    return 0;
}

int fat_dbg_read_block(char *name, unsigned char *buf)
{
    FILE *f;

    f = fopen(name, "rb");
    if(f)
    {
        if(fread(buf, 1, 512, f) != 512)
        {
            fprintf(stderr, "Could not read file \"%s\"\n", name);
            fclose(f);
            return -1;
        }
        /* Now write the sector to disk */
        ata_write_sectors(0,1,buf);
        fclose(f);
    }
    else
    {
        fprintf(stderr, "Could not open file \"%s\"\n", name);
        return -1;
    }
    return 0;
}

unsigned char *fat_cache_fat_sector(struct bpb *bpb, int secnum)
{
    unsigned char *sec;

    sec = fat_cache[secnum];
    /* Load the sector if it is not cached */
    if(!sec)
    {
        sec = malloc(bpb->bpb_bytspersec);
        if(!sec)
        {
            fprintf(stderr, "fat_cache_fat_sector() - Out of memory\n");
            return NULL;
        }
        if(ata_read_sectors(secnum + bpb->bpb_rsvdseccnt,1,sec))
        {
            fprintf(stderr, "fat_cache_fat_sector() - Could"
                    " not read sector %d\n",
                    secnum);
            free(sec);
            return NULL;
        }
        fat_cache[secnum] = sec;
    }
    return sec;
}

int fat_update_entry(struct bpb *bpb, int entry, unsigned int val)
{
    unsigned char *sec;
    unsigned char *sec2;
    int fatsz;
    int fatoffset;
    int thisfatsecnum;
    int thisfatentoffset;
    unsigned int tmp;

    fatsz = fat_get_fatsize(bpb);
    
    if(bpb->fat_type == FATTYPE_FAT12)
    {
        fatoffset = entry + (entry / 2);
    }
    else 
    {
        if(bpb->fat_type == FATTYPE_FAT16)
            fatoffset = entry * 2;
        else
            fatoffset = entry * 4;
    }
    thisfatsecnum = fatoffset / bpb->bpb_bytspersec;
    thisfatentoffset = fatoffset % bpb->bpb_bytspersec;

    sec = fat_cache_fat_sector(bpb, thisfatsecnum);
    /* Load the sector if it is not cached */
    if(!sec)
    {
        fprintf(stderr, "fat_update_entry() - Could not cache sector %d\n",
                thisfatsecnum);
        return -1;
    }

    fat_cache_dirty[thisfatsecnum] = 1;
    
    switch(bpb->fat_type)
    {
        case FATTYPE_FAT12:
            if(thisfatentoffset == bpb->bpb_bytspersec - 1)
            {
                /* This entry spans a sector boundary. Take care */
                sec2 = fat_cache_fat_sector(bpb, thisfatsecnum + 1);
                /* Load the sector if it is not cached */
                if(!sec2)
                {
                    fprintf(stderr, "fat_update_entry() - Could not "
                            "cache sector %d\n",
                            thisfatsecnum + 1);
                    return -1;
                }
                fat_cache_dirty[thisfatsecnum + 1] = 1;
            }
            else
            {
                if(entry & 1) /* Odd entry number? */
                {
                    tmp = sec[thisfatentoffset] & 0xf0;
                    sec[thisfatentoffset] = tmp | (val & 0x0f);
                    sec[thisfatentoffset+1] = (val >> 4) & 0xff;
                }
                else
                {
                    sec[thisfatentoffset] = val & 0xff;
                    tmp = sec[thisfatentoffset+1] & 0x0f;
                    sec[thisfatentoffset+1] = tmp | ((val >> 4) & 0xf0);
                }
            }
            break;
        case FATTYPE_FAT16:
            *(unsigned short *)(&sec[thisfatentoffset]) = val;
            break;
        case FATTYPE_FAT32:
            tmp = *(unsigned short *)(&sec[thisfatentoffset]) & 0xf000000;
            val = tmp | (val & 0x0fffffff);
            *(unsigned short *)(&sec[thisfatentoffset]) = val;
            break;
    }
    return 0;
}

int fat_read_entry(struct bpb *bpb, int entry)
{
    unsigned char *sec;
    unsigned char *sec2;
    int fatsz;
    int fatoffset;
    int thisfatsecnum;
    int thisfatentoffset;
    int val = -1;

    fatsz = fat_get_fatsize(bpb);
    
    if(bpb->fat_type == FATTYPE_FAT12)
    {
        fatoffset = entry + (entry / 2);
    }
    else 
    {
        if(bpb->fat_type == FATTYPE_FAT16)
            fatoffset = entry * 2;
        else
            fatoffset = entry * 4;
    }
    thisfatsecnum = fatoffset / bpb->bpb_bytspersec;
    thisfatentoffset = fatoffset % bpb->bpb_bytspersec;

    sec = fat_cache_fat_sector(bpb, thisfatsecnum);
    /* Load the sector if it is not cached */
    if(!sec)
    {
        fprintf(stderr, "fat_update_entry() - Could not cache sector %d\n",
                thisfatsecnum);
        return -1;
    }

    switch(bpb->fat_type)
    {
        case FATTYPE_FAT12:
            if(thisfatentoffset == bpb->bpb_bytspersec - 1)
            {
                /* This entry spans a sector boundary. Take care */
                sec2 = fat_cache_fat_sector(bpb, thisfatsecnum + 1);
                /* Load the sector if it is not cached */
                if(!sec2)
                {
                    fprintf(stderr, "fat_update_entry() - Could not "
                            "cache sector %d\n",
                            thisfatsecnum + 1);
                    return -1;
                }
            }
            else
            {
                if(entry & 1) /* Odd entry number? */
                {
                    val = (sec[thisfatentoffset] & 0x0f) |
                        (sec[thisfatentoffset+1] << 4);
                }
                else
                {
                    val = (sec[thisfatentoffset] & 0xff) |
                        ((sec[thisfatentoffset+1] & 0x0f) << 8);
                }
            }
            break;
        case FATTYPE_FAT16:
            val = *(unsigned short *)(&sec[thisfatentoffset]);
            break;
        case FATTYPE_FAT32:
            val = *(unsigned int *)(&sec[thisfatentoffset]);
            break;
    }
    return val;
}

int fat_flush_fat(struct bpb *bpb)
{
    int i;
    int err;
    unsigned char *sec;
    int fatsz;
    unsigned short d, t;
    char m;

    fatsz = fat_get_fatsize(bpb);

    for(i = 0;i < 256;i++)
    {
        if(fat_cache[i] && fat_cache_dirty[i])
        {
            printf("Flushing FAT sector %d\n", i);
            sec = fat_cache[i];
            err = ata_write_sectors(i + bpb->bpb_rsvdseccnt,1,sec);
            if(err)
            {
                fprintf(stderr, "fat_flush_fat() - Couldn't write"
                        " sector (%d)\n", i + bpb->bpb_rsvdseccnt);
                return -1;
            }
            err = ata_write_sectors(i + bpb->bpb_rsvdseccnt + fatsz,1,sec);
            if(err)
            {
                fprintf(stderr, "fat_flush_fat() - Couldn't write"
                        " sector (%d)\n", i + bpb->bpb_rsvdseccnt + fatsz);
                return -1;
            }
            fat_cache_dirty[i] = 0;
        }
    }

    fat_getcurrdostime(&d, &t, &m);
    return 0;
}

unsigned int fat_getcurrdostime(unsigned short *dosdate,
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

int fat_create_root_dir(struct bpb *bpb)
{
    unsigned char buf[BLOCK_SIZE];
    int fatsz;
    int sec;
    int res;
    int i;
    unsigned short dosdate;
    unsigned short dostime;
    unsigned char dostenth;
    int num_root_sectors;

    fatsz = fat_get_fatsize(bpb);

    sec = bpb->bpb_rsvdseccnt + bpb->bpb_numfats * fatsz;

    memset(buf, 0, sizeof(buf));
    
    strncpy(&buf[FATDIR_NAME], bpb->bs_vollab, 11);
    buf[FATDIR_ATTR] = FAT_ATTR_VOLUME_ID;
    buf[FATDIR_NTRES] = 0;

    fat_getcurrdostime(&dosdate, &dostime, &dostenth);
    buf[FATDIR_WRTDATE] = dosdate & 0xff;
    buf[FATDIR_WRTDATE+1] = dosdate >> 8;
    buf[FATDIR_WRTTIME] = dostime & 0xff;
    buf[FATDIR_WRTTIME+1] = dostime >> 8;

    printf("Writing rootdir to sector %d...\n", sec);

    res = ata_write_sectors(sec,1,buf);
    if(res)
    {
        fprintf(stderr, "fat_create_root_dir() - Couldn't write sector (%d)\n",
                sec);
        return -1;
    }

    printf("Clearing the rest of the root dir.\n");
    sec++;
    num_root_sectors = bpb->bpb_rootentcnt * 32 / bpb->bpb_bytspersec;
    memset(buf, 0, BLOCK_SIZE);
    
    for(i = 1;i < num_root_sectors;i++)
    {
        if(ata_write_sectors(sec++,1,buf))
        {
            fprintf(stderr, "fat_create_root_dir() - "
                            " Couldn't write sector (%d)\n", sec);
            return -1;
        }
    }
    
    return 0;
}

int fat_get_next_cluster(struct bpb *bpb, unsigned int cluster)
{
    int next_cluster = fat_read_entry(bpb, cluster);

    if(fat_last_cluster_in_chain(bpb, next_cluster))
        return 0;
    else
        return next_cluster;
}

int fat_add_dir_entry(struct bpb *bpb, unsigned int currdir,
                      struct fat_direntry *de)
{
    unsigned char buf[BLOCK_SIZE];
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
        sec = fat_get_rootdir_sector(bpb);
    }
    else
    {
        sec = fat_first_sector_of_cluster(bpb, currdir);
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
                    fprintf(stderr, "fat_add_dir_entry() -"
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
                printf("Moving to the next cluster...");
                currdir = fat_get_next_cluster(bpb, currdir);
                printf("new cluster is %d\n", currdir);
                
                if(!currdir)
                {
                    /* This was the last in the chain,
                       we have to allocate a new cluster */
                    /* TODO */
                }
            }
        }

        printf("Reading sector %d...\n", sec);
        /* Read the next sector in the current dir */
        err = ata_read_sectors(sec,1,buf);
        if(err)
        {
            fprintf(stderr, "fat_add_dir_entry() - Couldn't read dir sector"
                    " (error code %i)\n", err);
            return -1;
        }

        if(need_to_update_last_empty_marker)
        {
            /* All we need to do is to set the first entry to 0 */
            printf("Clearing the first entry in sector %d\n", sec);
            buf[0] = 0;
            done = 1;
        }
        else
        {
            /* Look for a free slot */
            for(i = 0;i < BLOCK_SIZE;i+=32)
            {
                firstbyte = buf[i];
                if(firstbyte == 0xe5 || firstbyte == 0)
                {
                    printf("Found free slot at entry %d in sector %d\n",
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
                        if(i < BLOCK_SIZE)
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
                        fprintf(stderr, "fat_add_dir_entry() - "
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

unsigned char fat_char2dos(unsigned char c)
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

int fat_create_dos_name(unsigned char *name, unsigned char *newname)
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
        c = fat_char2dos(n[i]);
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
        c = fat_char2dos(ext[i]);
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

    printf("fat_create_file()\n");
    memset(&de, 0, sizeof(struct fat_direntry));
    if(fat_create_dos_name(name, de.name) < 0)
    {
        fprintf(stderr, "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }

    fat_getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    de.attr = FAT_ATTR_DIRECTORY;
    
    err = fat_add_dir_entry(bpb, currdir, &de);
    return 0;
}

int fat_create_file(struct bpb *bpb, unsigned int currdir, char *name)
{
    struct fat_direntry de;
    int err;

    printf("fat_create_file()\n");
    memset(&de, 0, sizeof(struct fat_direntry));
    if(fat_create_dos_name(name, de.name) < 0)
    {
        fprintf(stderr, "fat_create_file() - Illegal file name (%s)\n", name);
        return -1;
    }
    fat_getcurrdostime(&de.crtdate, &de.crttime, &de.crttimetenth);
    de.wrtdate = de.crtdate;
    de.wrttime = de.crttime;
    de.filesize = 0;
    
    err = fat_add_dir_entry(bpb, currdir, &de);
    return err;
}

void fat_fill_direntry(struct fat_direntry *de, char *buf)
{
    memset(de, 0, sizeof(struct fat_direntry));
    
    strncpy(de->name, &buf[FATDIR_NAME], 11);
    de->attr = buf[FATDIR_ATTR];
    de->crttimetenth = buf[FATDIR_CRTTIMETENTH];
    de->crtdate = buf[FATDIR_CRTDATE] | (buf[FATDIR_CRTDATE+1] << 8);
    de->crttime = buf[FATDIR_CRTTIME] | (buf[FATDIR_CRTTIME+1] << 8);
    de->wrtdate = buf[FATDIR_WRTDATE] | (buf[FATDIR_WRTDATE+1] << 8);
    de->wrttime = buf[FATDIR_WRTTIME] | (buf[FATDIR_WRTTIME+1] << 8);
    
    de->filesize = buf[FATDIR_FILESIZE] |
        (buf[FATDIR_FILESIZE+1] << 8) |
        (buf[FATDIR_FILESIZE+2] << 16) |
        (buf[FATDIR_FILESIZE+3] << 24);
}

int fat_opendir(struct bpb *bpb, struct fat_dirent *ent, unsigned int currdir)
{
    int is_rootdir = (currdir == 0);
    unsigned int sec;
    int err;

    if(is_rootdir)
    {
        sec = fat_get_rootdir_sector(bpb);
    }
    else
    {
        sec = fat_first_sector_of_cluster(bpb, currdir);
    }

    /* Read the first sector in the current dir */
    err = ata_read_sectors(sec,1,ent->cached_buf);
    if(err)
    {
        fprintf(stderr, "fat_getfirst() - Couldn't read dir sector"
                " (error code %i)\n", err);
        return -1;
    }

    ent->entry = 0;
    ent->cached_sec = sec;
    ent->num_sec = 0;
    return 0;
}

int fat_getnext(struct bpb *bpb, struct fat_dirent *ent,
                 struct fat_direntry *entry)
{
    int done = 0;
    int i;
    int err;
    unsigned char firstbyte;

    while(!done)
    {
        /* Look for a free slot */
        for(i = ent->entry;i < BLOCK_SIZE/32;i++)
        {
            firstbyte = ent->cached_buf[i*32];
            if(firstbyte == 0xe5)
            {
                continue;
            }

            if(firstbyte == 0)
            {
                return -1;
            }

            fat_fill_direntry(entry, &ent->cached_buf[i*32]);
            done = 1;
            break;
        }

        /* Next sector? */
        if(i >= BLOCK_SIZE/32)
        {
            ent->num_sec++;
            ent->cached_sec++;
            
            /* Do we need to advance one cluster? */
            if(ent->num_sec >= bpb->bpb_secperclus)
            {
                ent->num_sec = 0;
                ent->cached_sec = fat_get_next_cluster(
                    bpb, fat_sec2cluster(bpb, ent->cached_sec));
                if(!ent->cached_sec)
                {
                    printf("End of cluster chain.\n");
                    return -1;
                }
            }

            /* Read the next sector */
            err = ata_read_sectors(ent->cached_sec,1,ent->cached_buf);
            if(err)
            {
                fprintf(stderr, "fat_getnext() - Couldn't read dir sector"
                        " (error code %i)\n", err);
                return -1;
            }

            i = 0;
        }
        else
        {
            i++;
        }
        ent->entry = i;
    }
    return 0;
}
