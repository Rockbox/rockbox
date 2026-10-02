/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Andrew Rice
 * Copyright (C) 2026 by Robin Tschirschnitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __NAND_TARGET_H__
#define __NAND_TARGET_H__

#include <stdint.h>

#define NAND_PAGE_SIZE          4096
#define NAND_BOOT_PAGE_SIZE     0x600
#define NAND_META_WORDS         3

#define NAND_ECC_CLEAN          0
#define NAND_ECC_CORRECTED      1
#define NAND_ECC_FAILED         3

#define NAND_ERR_RESET_CMD      (-1)
#define NAND_ERR_IDENTIFY       (-2)
#define NAND_ERR_READY_CMD      (-3)
#define NAND_ERR_UNSUPPORTED    (-5)
#define NAND_ERR_MIXED          (-6)

struct nand_geometry
{
    unsigned int banks;
    unsigned int blocks;          /* per bank */
    unsigned int pagesperblock;
    unsigned int pagesize;
};

/* Reads a 4 KiB Whimory user page and, optionally, its three metadata words. */
/* Returns an error until FMSS page reads are added. */
int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta);

/* Reads the 0x600-byte BootROM boot-block format. */
/* Returns an error until FMSS boot-page reads are added. */
int nand_read_boot_page(uint32_t bank, uint32_t page, void *databuf);

/* Reads the first len bytes without ECC; buf and len must be word-aligned. */
int nand_read_raw(uint32_t bank, uint32_t page, void *buf, unsigned int len);

unsigned int nand_get_bank_count(void);
uint32_t nand_chip_id(void);
const struct nand_geometry *nand_get_geometry(void);

#endif /* __NAND_TARGET_H__ */
