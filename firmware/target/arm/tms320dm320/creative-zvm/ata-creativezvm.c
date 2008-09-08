/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "ata-target.h"
#include "dm320.h"
#include "ata.h"
#include "string.h"
#include "buffer.h"

#undef ata_read_sectors
#undef ata_write_sectors

static void sleep_ms(int ms)
{
    sleep(ms*HZ/1000);
}

void ide_power_enable(bool on)
{
/* Disabled until figured out what's wrong */
#if 0
    int old_level = disable_irq_save();
    if(on)
    {
        IO_GIO_BITSET0 = (1 << 14);
        ata_reset();
    }
    else
        IO_GIO_BITCLR0 = (1 << 14);
    restore_irq(old_level);
#else
    (void)on;
#endif
}

inline bool ide_powered()
{
#if 0
    return (IO_GIO_BITSET0 & (1 << 14));
#else
    return true;
#endif
}

void ata_reset(void)
{
    int old_level = disable_irq_save();
    if(!ide_powered())
    {
        ide_power_enable(true);
        sleep_ms(150);
    }
    else
    {
        IO_GIO_BITSET0 = (1 << 5);
        IO_GIO_BITCLR0 = (1 << 3);
        sleep_ms(1);
    }
    IO_GIO_BITCLR0 = (1 << 5);
    sleep_ms(10);
    IO_GIO_BITSET0 = (1 << 3);
    while(!(ATA_COMMAND & STATUS_RDY))
        sleep_ms(10);
    restore_irq(old_level);
}

void ata_enable(bool on)
{
    (void)on;
    return;
}

bool ata_is_coldstart(void)
{
    return true;
}

void ata_device_init(void)
{
    IO_INTC_EINT1 |= INTR_EINT1_EXT2;   /* enable GIO2 interrupt */
    /* TODO: mimic OF inits... */
    return;
}

void GIO2(void)
{
#ifdef DEBUG
    logf("GIO2 interrupt...");
#endif
    IO_INTC_IRQ1 = INTR_IRQ1_EXT2; /* Mask GIO2 interrupt */
    return;
}

/*
 ---------------------------------------------------------------------------
 CreativeFileSystem parsing/handling code
 ---------------------------------------------------------------------------
 */

#define VFAT_SECTOR_SIZE(x) ( (x)/0x8000 ) /* 1GB array requires 80kB of RAM */

extern int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf);
extern int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf);

struct main_header
{
    char mblk[4];
    unsigned int sector_size;
    long long disk_size;
    struct partition_header
    {
        unsigned long end;
        unsigned long start;
        char name[8];
    } partitions[31];
};

struct cfs_header
{
    unsigned int unk;
    unsigned int unk2;
    unsigned int sector_size;
    unsigned int unk4;
    unsigned int unk5;
    char identifier[4];
    unsigned int first_inode;
    unsigned int unk8;
    unsigned int unk9;
    unsigned int unk10;
    unsigned int unk11;
};

struct cfs_inode
{
    unsigned char magic[4];
    unsigned int number;
    unsigned int parent;
    unsigned int unk;
    unsigned int type;
    unsigned int created_time;
    unsigned int lastmodified_time;
    unsigned int unk2;
    unsigned int first_class_chain[12];
    unsigned int unk3;
    unsigned int unk4;
    unsigned int second_class_chain_first_cluster;
    unsigned int unk9;
    unsigned int unk10;
    unsigned int second_class_chain_second_cluster;
    unsigned int unk11;
    unsigned int unk12;
    unsigned int unk13;
    unsigned int filesize;
    unsigned int serial_number;
    unsigned int number_of_metadata_records;
};

struct cfs_direntry
{
    unsigned char identifier[4];
    unsigned int unk;
    unsigned int items;
    unsigned int unk2;
    unsigned char maxlen[2];
    unsigned char padding[202];
    /* struct cfs_direntry_item _items[items]; */
};
struct cfs_direntry_item
{
    unsigned int inode_number;
    unsigned short strlen;
    unsigned short bytesperchar;
    char string[32];
};

static bool cfs_inited = false;
static unsigned long cfs_start;
static unsigned long *sectors;

#define CFS_START              ( ((hdr->partitions[1].start*hdr->sector_size) & ~0xFFFF) + 0x10000 )
#define CFS_CLUSTER2CLUSTER(x) ( CFS_START+((x)-1)*64 )

/* Limited version of UCS -> ASCII */
static char* ucs2letostring(unsigned char* s)
{
    static char res[256];
    int i;
    
    for(i=0; (s[i] == 0 && s[i+1] == 0); i++)
        res[i] = s[i*2];
    
    return (char*)&res;
}

static void cfs_init(void)
{  
    struct main_header *hdr;
    struct cfs_header *cfs;
    struct cfs_inode *root_inode, *vfat_inode, *inode;
    struct cfs_direntry *root_direntry, *vfat_direntry;
    struct cfs_direntry_item *root_direntry_items, *vfat_direntry_items;
    unsigned int i, j, k, vfat_inode_nr=0, vfat_inodes_nr[10], vfat_sector_count;
    unsigned char sector[512];
    static unsigned int vfat_data[2][0x8000];
    static unsigned char sector2[0x8000];
    
    if(cfs_inited)
        return;
    
    /* Read MBLK */
    _ata_read_sectors(0, 1, &sector);
    hdr = (struct main_header*)&sector;
    
    //printf("CFS is at 0x%x", CFS_START);
    
    /* Read CFS header */
    _ata_read_sectors(CFS_START/512, 64, &sector2);
    cfs = (struct cfs_header*)&sector2;
    
    //printf("First inode = %d", cfs->first_inode);
    
    /* Read root inode */
    _ata_read_sectors(CFS_CLUSTER2CLUSTER(cfs->first_inode), 64, &sector2);
    root_inode = (struct cfs_inode*)&sector2;
    
    /* Read root inode's first sector */
    _ata_read_sectors(CFS_CLUSTER2CLUSTER(root_inode->first_class_chain[0]), 64, &sector2);
    root_direntry = (struct cfs_direntry*)&sector2;
    root_direntry_items = (struct cfs_direntry_item*)(&sector2+sizeof(struct cfs_direntry));
    
    /* Search VFAT inode */
    for(i=0; i < root_direntry->items; i++)
    {
        //printf(" * [%s] at 0x%x\n", ucs2letostring(&root_direntry_items[i].string[0]), root_direntry_items[i].inode_number);
        if(strcmp(ucs2letostring(&root_direntry_items[i].string[0]), "VFAT") == 0)
            vfat_inode_nr = root_direntry_items[i].inode_number;
    }
    
    /* Read VFAT inode */
    _ata_read_sectors(CFS_CLUSTER2CLUSTER(vfat_inode_nr), 64, &sector2);
    vfat_inode = (struct cfs_inode*)&sector2;
    
    /* Read VFAT inode's first sector */
    _ata_read_sectors(CFS_CLUSTER2CLUSTER(vfat_inode->first_class_chain[0]), 64, &sector2);
    vfat_direntry = (struct cfs_direntry*)&sector2;
    vfat_direntry_items = (struct cfs_direntry_item*)(&sector2+sizeof(struct cfs_direntry));
    
    /* Search for VFAT's subinodes */
    for(i=0; i < vfat_direntry->items; i++)
    {
        //printf(" * [%s] at 0x%x\n", ucs2letostring(&vfat_direntry_items[i].string[0]), vfat_direntry_items[i].inode_number);
        if(i > 0)
            vfat_inodes_nr[i-1] = vfat_direntry_items[i].inode_number;
    }
    
    /* Determine size of VFAT file */
    _ata_read_sectors(CFS_CLUSTER2CLUSTER(vfat_inodes_nr[1]), 1, &sector);
    inode = (struct cfs_inode*)&sector;
    sectors = (unsigned long*)buffer_alloc(VFAT_SECTOR_SIZE(inode->filesize));
    
    //printf("VFAT file size: 0x%x", inode->filesize);
    
    /* Clear data sectors */
    memset(&sectors, 0, VFAT_SECTOR_SIZE(inode->filesize)*sizeof(unsigned long));
    
    /* Read all data sectors' addresses in memory */
    vfat_sector_count = 0;
    for(i=0; vfat_inodes_nr[i] != 0; i++)
    {
        _ata_read_sectors(CFS_CLUSTER2CLUSTER(vfat_inodes_nr[i]), 1, &sector);
        inode = (struct cfs_inode*)&sector;
        
        /* Read second & third class chain */
        _ata_read_sectors(CFS_CLUSTER2CLUSTER(inode->second_class_chain_first_cluster), 64, &vfat_data[0]);
        _ata_read_sectors(CFS_CLUSTER2CLUSTER(inode->second_class_chain_second_cluster), 64, &vfat_data[1]);
        
        /* First class chain */
        for(j=0; j<12; j++)
        {
            if( (inode->first_class_chain[j] & 0xFFFF) != 0xFFFF &&
                 inode->first_class_chain[j] != 0
              )
                sectors[vfat_sector_count++] = inode->first_class_chain[j];
        }
        
        /* Second class chain */
        for(j=0; j<0x8000/4; j++)
        {
            if( (vfat_data[0][j] & 0xFFFF) != 0xFFFF &&
                vfat_data[0][j] != 0
              )
                sectors[vfat_sector_count++] = vfat_data[0][j];
        }
        
        /* Third class chain */
        for(j=0; j<0x8000/4; j++)
        {
            if( (vfat_data[1][j] & 0xFFFF) != 0xFFFF &&
                vfat_data[1][j] != 0
              )
            {
                memset(&vfat_data[0], 0, 0x8000*sizeof(unsigned int));
                
                /* Read third class subchain(s) */
                _ata_read_sectors(CFS_CLUSTER2CLUSTER(vfat_data[1][j]), 64, &vfat_data[0]);
                
                for(k=0; k<0x8000/4; k++)
                {
                    if( (vfat_data[0][k] & 0xFFFF) != 0xFFFF &&
                        vfat_data[0][k] != 0
                      )
                        sectors[vfat_sector_count++] = vfat_data[0][k];
                }
            }
        }
    }
    
    //printf("Sector count: %d 0x%x", vfat_sector_count, vfat_sector_count);
    
    cfs_inited = true;
}

static inline unsigned long map_sector(unsigned long sector)
{
    /*
     *  Sector mapping: start of CFS + FAT_SECTOR2CFS_SECTOR(sector) + missing part
     *  FAT works with sectors of 0x200 bytes, CFS with sectors of 0x8000 bytes.
     */
    return cfs_start+sectors[sector/64]*64+sector%64;
}

int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf)
{
    if(!cfs_inited)
        cfs_init();
    
    /* Check if count is lesser than or equal to 1 native CFS sector */
    if(count <= 64)
        return _ata_read_sectors(IF_MV2(drive,) map_sector(start), count, buf);
    else
    {
        int i, ret;
        unsigned char* dest = (unsigned char*)buf;
        
        /* Read sectors in parts of 0x8000 */
        for(i=0; i<count; i+=64)
        {
            ret = _ata_read_sectors(IF_MV2(drive,) map_sector(start+i), (count-i >= 64 ? 64 : count-i), (void*)dest);
            if(ret != 0)
                return ret;
            
            dest += (count-i >= 64 ? 0x8000 : (count-i)*512);
        }
        
        return ret;
    }
}

int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf)
{
    if(!cfs_inited)
        cfs_init();
    
    /* Check if count is lesser than or equal to 1 native CFS sector */
    if(count <= 64)
        return _ata_write_sectors(IF_MV2(drive,) map_sector(start), count, buf);
    else
    {
        int i, ret;
        unsigned char* dest = (unsigned char*)buf;
        
        /* Read sectors in parts of 0x8000 */
        for(i=0; i<count; i+=64)
        {
            ret = _ata_write_sectors(IF_MV2(drive,) map_sector(start+i), (count-i >= 64 ? 64 : count-i), (const void*)dest);
            if(ret != 0)
                return ret;
            
            dest += (count-i >= 64 ? 0x8000 : (count-i)*512);
        }
        
        return ret;
    }
}

/*
 ---------------------------------------------------------------------------
 MiniFileSystem parsing code
 ---------------------------------------------------------------------------
 */

struct minifs_file
{
    char name[0x10];
    unsigned int unk;
    unsigned long size;
    unsigned int chain1;
    unsigned int chain2;
};

struct minifs_chain
{
    unsigned int unknown;
    unsigned short chain[0x27FE];
    unsigned int unknown2;
    unsigned long length;
};


#define DIR_BITMAP_START       0x0143
#define DIR_START              0x0144
#define DATASPACE_BITMAP_START 0x0145
#define DATASPACE_START        0x0146

#define CLUSTER_CHAIN_SIZE     0x5008
#define CLUSTER_CHAIN_HEAD     0x0000
#define CLUSTER_CHAIN_BITMAP   0x0001
#define CLUSTER_CHAIN_CHAIN    0x0002


int load_minifs_file(char* filename, unsigned char* location)
{
    struct main_header *hdr;
    static struct minifs_file files[128];
    struct minifs_chain *chain;
    unsigned int i;
    int found = -1;
    unsigned char sector[512];
    static unsigned char chain_data[42*512]; /* stack overflow if not static */
    
    /* Read MBLK */
    _ata_read_sectors(0, 1, &sector);
    hdr = (struct main_header*)&sector;
    
    /* Read directory listing */
#define CLUSTER2SECTOR(x) ( (hdr->partitions[0].start + (x)*8) )
    _ata_read_sectors(CLUSTER2SECTOR(DIR_START), 8, &files);
    
    for(i=0; i<127; i++)
    {
        if(strcmp(files[i].name, filename) == 0)
            found = i;
    }
    
    if(found == -1)
        return -1;
    
#define GET_CHAIN(x)   ( CLUSTER2SECTOR(CLUSTER_CHAIN_CHAIN)*512 + (x)*CLUSTER_CHAIN_SIZE )
#define FILE2SECTOR(x) ( CLUSTER2SECTOR(DATASPACE_START + (x)) )
    
    /* Read chain list */
    _ata_read_sectors(GET_CHAIN(files[found].chain1)/512, 41, &chain_data[0]);
    
    chain = (struct minifs_chain*)&chain_data[GET_CHAIN(files[found].chain1)%512];
    
    /* Copy data */
    for(i=0; i<chain->length; i++)
    {
        _ata_read_sectors(FILE2SECTOR(chain->chain[i]), 8, location);
        location += 0x1000;
    }
    
    return files[found].size;
}
