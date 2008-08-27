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

void sleep_ms(int ms)
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
    //TODO: mimic OF inits...
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
 Creative File Systems parsing code
 ---------------------------------------------------------------------------
 */

struct main_header
{
    char mblk[4];
    unsigned char sector_size[4];
    unsigned char disk_size[8];
    struct partition_header
    {
        unsigned char end[4];
        unsigned char start[4];
        char name[8];
    } partitions[31];
};

struct minifs_file
{
    char name[0x10];
    unsigned char unk[4];
    unsigned char size[4];
    unsigned char chain1[4];
    unsigned char chain2[4];
};

struct minifs_chain
{
    unsigned char unknown[4];
    unsigned char chain[2*0x27FE];
    unsigned char unknown2[4];
    unsigned char length[4];
};


#define DIR_BITMAP_START       0x0143
#define DIR_START              0x0144
#define DATASPACE_BITMAP_START 0x0145
#define DATASPACE_START        0x0146

#define CLUSTER_CHAIN_SIZE     0x5008
#define CLUSTER_CHAIN_HEAD     0x0000
#define CLUSTER_CHAIN_BITMAP   0x0001
#define CLUSTER_CHAIN_CHAIN    0x0002


static unsigned short le2int16(unsigned char* buf)
{
   return (buf[1] << 8) | buf[0];
}

static unsigned int le2int32(unsigned char* buf)
{
   return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

int load_minifs_file(char* filename, unsigned char* location)
{
    struct main_header *hdr;
    static struct minifs_file files[128];
    struct minifs_chain *chain;
    unsigned int i;
    int found = -1;
    unsigned char sector[512];
    static unsigned char chain_data[42*512]; /* stack overflow if not static */
    
    /* Reading MBLK */
    ata_read_sectors(0, 1, &sector);
    hdr = (struct main_header*)&sector;
    
    /* Reading directory listing */
#define CLUSTER2SECTOR(x) ( (le2int32(hdr->partitions[0].start) + (x)*8) )
    ata_read_sectors(CLUSTER2SECTOR(DIR_START), 8, &files);
    
    for(i=0; i<127; i++)
    {
        if(strcmp(files[i].name, filename) == 0)
            found = i;
    }
    
    if(found == -1)
        return -1;
    
#define GET_CHAIN(x)   ( CLUSTER2SECTOR(CLUSTER_CHAIN_CHAIN)*512 + (x)*CLUSTER_CHAIN_SIZE )
#define FILE2SECTOR(x) ( CLUSTER2SECTOR(DATASPACE_START + (x)) )
    
    /* Reading chain list */
    ata_read_sectors(GET_CHAIN(le2int32(files[found].chain1))/512, 41, &chain_data[0]);
    
    chain = (struct minifs_chain*)&chain_data[GET_CHAIN(le2int32(files[found].chain1))%512];
    
    /* Copying data */
    for(i=0; i<le2int32(chain->length); i++)
    {
        ata_read_sectors(FILE2SECTOR(le2int16(&chain->chain[i*2])), 8, location);
        location += 0x1000;
    }
    
    return le2int32(files[found].size);
}
