/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
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

#ifndef __NAND_TARGET_H__
#define __NAND_TARGET_H__

#include "config.h"
#include "inttypes.h"

#define MAX_FLASH_NUM 4

/* flashspec_t.vendor: the index of the maker ID in manufacture_id_tbl[] */
enum vendor_t {
    SAMSUNG,
    TOSHIBA,
    HYNIX,
    INFINEON,
    MICRON,
    RENESAS,
    ST
};

/* Per-chip geometry, as the OF derives it from the READ_ID response.
 *
 * The "_raw" fields describe one physical plane; the others are the
 * multi-plane view the FTL addresses (sec_per_page = sec_per_page_raw *
 * mul_plane, and likewise for blocks). The Scheme A FTL's address mapping
 * depends on exactly this distinction, so both are kept. Populated by
 * flash_init(). */
struct flashspec_t
{
    uint8_t  cache_prog;
    uint8_t  mul_plane;
    uint8_t  interleave;
    uint8_t  large;
    uint8_t  five;
    uint8_t  mlc;
    uint8_t  vendor;            /* enum vendor_t */
    uint8_t  access_time;
    uint8_t  sec_per_page;
    uint8_t  sec_per_page_raw;
    uint16_t sec_per_block;
    uint16_t sec_per_block_raw;
    uint16_t page_per_block;
    uint16_t page_per_block_raw;

    uint32_t tot_logic_sec;
    uint32_t total_phy_sec;
    uint32_t total_bloks;

    uint32_t cmd;
    uint32_t addr;
    uint32_t data;
};

extern struct flashspec_t flash_spec[MAX_FLASH_NUM];
extern uint32_t total_phy_sec;

void flash_init(void);
void flash_chip_select(uint8_t chip);
void flash_chip_deselect(void);

struct nand_device_info_type
{
    uint32_t id;
    uint16_t blocks;
    uint16_t userblocks;
    uint16_t pagesperblock;
    uint8_t blocksizeexponent;
    uint8_t tunk1;
    uint8_t twp;
    uint8_t tunk2;
    uint8_t tunk3;
} __attribute__((packed));

uint32_t nand_read_page(uint32_t bank, uint32_t page, void* databuffer,
                        void* sparebuffer, uint32_t doecc,
                        uint32_t checkempty);
uint32_t nand_write_page(uint32_t bank, uint32_t page, void* databuffer,
                         void* sparebuffer, uint32_t doecc);
uint32_t nand_block_erase(uint32_t bank, uint32_t page);

const struct nand_device_info_type* nand_get_device_type(uint32_t bank);
uint32_t nand_reset(uint32_t bank);
uint32_t nand_device_init(void);
void nand_set_active(void);
long nand_last_activity(void);
void nand_power_up(void);
void nand_power_down(void);


#endif
