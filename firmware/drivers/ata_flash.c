/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Tomasz Malesinski
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

#include "storage.h"
#include <stdbool.h>
#include <string.h>

#if CONFIG_CPU == PNX0101
#include "pnx0101.h"
#endif

/*
#include "kernel.h"
#include "thread.h"
#include "led.h"
#include "cpu.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
*/

#define SECTOR_SIZE     (512)

long last_disk_activity = -1;

#if CONFIG_FLASH == FLASH_IFP7XX
static unsigned char flash_ce[4] = {0x20, 0x02, 0x10, 0x08};

#define FLASH_IO_BASE 0x28000000
#define FLASH_REG_DATA (*((volatile unsigned char*)(FLASH_IO_BASE)))
#define FLASH_REG_CMD  (*((volatile unsigned char*)(FLASH_IO_BASE + 4)))
#define FLASH_REG_ADDR (*((volatile unsigned char*)(FLASH_IO_BASE + 8)))

#define SEGMENT_SIZE 1000
#define MAX_N_SEGMENTS 8

#endif

#define FLASH_MODEL_NONE 0
#define FLASH_MODEL_256 1
#define FLASH_MODEL_512 2

struct flash_disk
{
    unsigned short block_map[MAX_N_SEGMENTS][SEGMENT_SIZE];
    short cur_block;
    int cur_phblock_start;
    int n_chips;
    unsigned char chip_no[4];
    unsigned char model;
};

static struct flash_disk flash_disk;

void flash_select_chip(int no, int sel)
{
#if CONFIG_FLASH == FLASH_IFP7XX
    if (sel)
        GPIO5_CLR = flash_ce[no];
    else
        GPIO5_SET = flash_ce[no];
#endif
}

static inline unsigned char flash_read_data(void)
{
    return FLASH_REG_DATA;
}

static inline void flash_write_data(unsigned char data)
{
    FLASH_REG_DATA = data;
}

/* TODO: these two doesn't work when inlined, probably some
   delay is required */

static void flash_write_cmd(unsigned char cmd)
{
    FLASH_REG_CMD = cmd;
}

static void flash_write_addr(unsigned char addr)
{
    FLASH_REG_ADDR = addr;
}

static void flash_wait_ready(void)
{
    int i;
    for (i = 0; i < 5; i++)
        while ((GPIO6_READ & 8) == 0);
}

static unsigned char model_n_sectors_order[] = {0, 19, 20};

int flash_map_sector(int sector, int* chip, int* chip_sector)
{
    int ord, c;
    if (flash_disk.model ==  FLASH_MODEL_NONE)
        return -1;
    
    ord = model_n_sectors_order[flash_disk.model];
    c = sector >> ord;
    *chip_sector = sector & ((1 << ord) - 1);

    if (c >= flash_disk.n_chips)
        return -1;

    *chip = flash_disk.chip_no[c];
    return 0;
}

int flash_read_id(int no) {
    int id;

    flash_select_chip(no, 1);
    flash_write_cmd(0x90);
    flash_write_addr(0);

    flash_read_data();
    id = flash_read_data();

    flash_select_chip(no, 0);
    return id;
}

int flash_read_sector(int sector, unsigned char* buf,
                      unsigned char* oob)
{
    unsigned long *bufl = (unsigned long *)buf;
    int chip, chip_sector;
    int i;
    
    if (flash_map_sector(sector, &chip, &chip_sector) < 0)
        return -1;

    flash_select_chip(chip, 1);

    flash_write_cmd(0x00);
    flash_write_addr(0);
    flash_write_addr((chip_sector << 1) & 7);
    flash_write_addr((chip_sector >> 2) & 0xff);
    flash_write_addr((chip_sector >> 10) & 0xff);
    flash_write_addr((chip_sector >> 18) & 0xff);
    flash_write_cmd(0x30);
    
    flash_wait_ready();
    
    if ((unsigned long)buf & 3)
    {
        for (i = 0; i < 512; i++)
            buf[i] = flash_read_data();
    }
    else
    {
        for (i = 0; i < 512 / 4; i++) {
            unsigned long v;
#ifdef ROCKBOX_LITTLE_ENDIAN
            v = flash_read_data();
            v |= (unsigned long)flash_read_data() << 8;
            v |= (unsigned long)flash_read_data() << 16;
            v |= (unsigned long)flash_read_data() << 24;
#else
            v = (unsigned long)flash_read_data() << 24;
            v |= (unsigned long)flash_read_data() << 16;
            v |= (unsigned long)flash_read_data() << 8;
            v |= flash_read_data();
#endif
            bufl[i] = v;
        }
    }

    flash_write_cmd(0x05);
    flash_write_addr((chip_sector & 3) * 0x10);
    flash_write_addr(8);
    flash_write_cmd(0xe0);

    for (i = 0; i < 16; i++)
        oob[i] = flash_read_data();

    flash_select_chip(chip, 0);
    return 0;
}

int flash_read_sector_oob(int sector, unsigned char* oob)
{
    int chip, chip_sector;
    int i;
    
    if (flash_map_sector(sector, &chip, &chip_sector) < 0)
        return -1;

    flash_select_chip(chip, 1);

    flash_write_cmd(0x00);
    flash_write_addr((chip_sector & 3) * 0x10);
    flash_write_addr(8);
    flash_write_addr((chip_sector >> 2) & 0xff);
    flash_write_addr((chip_sector >> 10) & 0xff);
    flash_write_addr((chip_sector >> 18) & 0xff);
    flash_write_cmd(0x30);
    
    flash_wait_ready();
    
    for (i = 0; i < 16; i++)
        oob[i] = flash_read_data();

    flash_select_chip(chip, 0);
    return 0;
}

static unsigned char model_n_segments[] = {0, 2, 4};

static inline int flash_get_n_segments(void)
{
    return model_n_segments[flash_disk.model] * flash_disk.n_chips;
}

static inline int flash_get_n_phblocks(void)
{
    return 1024;
}

static int model_n_sectors_in_block[] = {0, 256, 256};

static int flash_get_n_sectors_in_block(void)
{
    return model_n_sectors_in_block[flash_disk.model];
}

static int flash_phblock_to_sector(int segment, int block)
{
    return (segment * flash_get_n_phblocks() + block)
        * flash_get_n_sectors_in_block();
}

static int flash_is_bad_block(unsigned char* oob)
{
    /* TODO: should we check two pages? (see datasheet) */
    return oob[0] != 0xff;
}

static int count_1(int n) {
    int r = 0;
    while (n != 0) {
        r += (n & 1);
        n >>= 1;
    }
    return r;
}

static int flash_get_logical_block_no(unsigned char* oob)
{
    int no1, no2;
    no1 = oob[6] + (oob[7] << 8);
    no2 = oob[11] + (oob[12] << 8);

    if (no1 == no2 && (no1 & 0xf000) == 0x1000)
        return (no1 & 0xfff) >> 1;

    if (count_1(no1 ^ no2) > 1)
        return -1;

    if ((no1 & 0xf000) == 0x1000
        && (count_1(no1) & 1) == 0)
        return (no1 & 0xfff) >> 1;

    if ((no2 & 0xf000) == 0x1000
        && (count_1(no2) & 1) == 0)
        return (no2 & 0xfff) >> 1;

    return -1;
}

int flash_disk_scan(void)
{
    int n_segments, n_phblocks;
    unsigned char oob[16];
    int s, b;
    
    /* TODO: checking for double blocks */

    n_segments = flash_get_n_segments();
    n_phblocks = flash_get_n_phblocks();

    flash_disk.cur_block = -1;
    flash_disk.cur_phblock_start = -1;

    for (s = 0; s < n_segments; s++)
    {
        for (b = 0; b < n_phblocks; b++)
        {
            int r;
            r = flash_read_sector_oob(flash_phblock_to_sector(s, b),
                                      oob);
            if (r >= 0 && !flash_is_bad_block(oob))
            {
                int lb;
                lb = flash_get_logical_block_no(oob);
                if (lb >= 0 && lb < SEGMENT_SIZE)
                    flash_disk.block_map[s][lb] = b;
            }
        }
    }
    return 0;
}

int flash_disk_find_block(int block)
{
    int seg, bmod, phb;
    unsigned char oob[16];
    int r;
   
    if (block >= SEGMENT_SIZE * flash_get_n_segments())
        return -1;

    if (block == flash_disk.cur_block)
        return flash_disk.cur_phblock_start;

    seg = block / SEGMENT_SIZE;
    bmod = block % SEGMENT_SIZE;

    phb = flash_disk.block_map[seg][bmod];
    r = flash_read_sector_oob(flash_phblock_to_sector(seg, phb), oob);
    if (r < 0)
        return -1;
    if (flash_is_bad_block(oob))
        return -1;
    if (flash_get_logical_block_no(oob) != bmod)
        return -1;

    flash_disk.cur_block = block;
    flash_disk.cur_phblock_start = flash_phblock_to_sector(seg, phb);
    return flash_disk.cur_phblock_start;
}

int flash_disk_read_sectors(unsigned long start,
                            int count,
                            void* buf)
{
    int block, secmod, done;
    int phb;
    char oob[16];

    block = start / flash_get_n_sectors_in_block();
    secmod = start % flash_get_n_sectors_in_block();

    phb = flash_disk_find_block(block);
    done = 0;
    while (count > 0 && secmod < flash_get_n_sectors_in_block())
    {
        if (phb >= 0)
            flash_read_sector(phb + secmod, buf, oob);
        else
            memset(buf, 0, SECTOR_SIZE);

        buf += SECTOR_SIZE;
        count--;
        secmod++;
        done++;
    }
    return done;
}

int nand_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{
    while (incount > 0)
    {
        int done = flash_disk_read_sectors(start, incount, inbuf);
        if (done < 0)
            return -1;
        start += done;
        incount -= done;
        inbuf += SECTOR_SIZE * done;
    }
    return 0;
}

int nand_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    (void)start;
    (void)count;
    (void)buf;
    return -1;
}

int nand_init(void)
{
    int i, id, id2;

    id = flash_read_id(0);
    switch (id)
    {
        case 0xda:
            flash_disk.model = FLASH_MODEL_256;
            break;
        case 0xdc:
            flash_disk.model = FLASH_MODEL_512;
            break;
        default:
            flash_disk.model = FLASH_MODEL_NONE;
            return -1;
    }

    flash_disk.n_chips = 1;
    flash_disk.chip_no[0] = 0;
    for (i = 1; i < 4; i++)
    {
        id2 = flash_read_id(i);
        if (id2 == id)
            flash_disk.chip_no[flash_disk.n_chips++] = i;
    }
        
    if (flash_disk_scan() < 0)
        return -2;

    return 0;
}

long nand_last_disk_activity(void)
{
    return last_disk_activity;
}

#ifdef STORAGE_GET_INFO
void nand_get_info(struct storage_info *info)
{
    unsigned long blocks;
    int i;

    /* firmware version */
    info->revision="0.00";

    /* vendor field, need better name? */
    info->vendor="Rockbox";
    /* model field, need better name? */
    info->product="TNFL";

    /* blocks count */
    info->num_sectors = 0;
    info->sector_size=SECTOR_SIZE;

    info->serial=0;
}
#endif

