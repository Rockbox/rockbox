/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Michael Sparmann
 * Copyright (C) 2026 by Andrew Rice
 * Copyright (C) 2026 by Robin Tschirschnitz
 *
 * Interface modelled on
 * firmware/target/arm/s5l8702/ipodnano3g/nand-target.h.
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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Toshiba TH58NVG6-class parts on the Nano 4G are 4 KiB pages. The FMI
 * still moves a page as 512-byte ECC chunks, eight of them. */
#define NAND_PAGE_SIZE      4096

/* Return values of nand_init(). When the FTL fails to mount, nand_init()
 * returns NAND_ERR_FTL plus the ftl_init() code, so "Storage error: -103"
 * means ftl_init() returned -3. */
#define NAND_ERR_RESET_CMD      (-1)
#define NAND_ERR_IDENTIFY       (-2)
#define NAND_ERR_READY_CMD      (-3)
#define NAND_ERR_UNKNOWN        (-5)
#define NAND_ERR_FTL            (-100)

#define NAND_ECC_CLEAN      0
#define NAND_ECC_CORRECTED  1
#define NAND_ECC_FAILED     3
#define NAND_ECC_STATUS6    6

#define NAND_META_WORDS     3

struct nand_geometry
{
    unsigned int banks;
    unsigned int blocks;        /* per bank */
    unsigned int pagesperblock;
    unsigned int planes;
    unsigned int userblocks;    /* per bank, as Whimory counts them */
    unsigned int vflspares;     /* VFL reserved blocks per plane */
    bool twoplane;
};

int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta);
/* 0x600 bytes, BootROM boot-block format */
int nand_read_boot_page(uint32_t bank, uint32_t page, void *databuf);
/* first len bytes of the page, no ECC (diagnostic) */
int nand_read_raw(uint32_t bank, uint32_t page, void *buf, unsigned int len);
/* nand_write_pages() and nand_erase_block() refuse every call until this
 * is set, and refuse block 0 of every bank always */
void nand_set_writable(bool on);
#ifdef BOOTLOADER
/* Private boot-block path. Refuses every call until nand_boot_arm_writes(),
 * and then only block 0.
 *
 * Each boot page is 0x600 bytes, three 512-byte codewords with 26 parity
 * bytes. */
void nand_boot_arm_writes(void);
int nand_boot_erase_block(uint32_t bank, uint32_t block);
int nand_boot_program_page(uint32_t bank, uint32_t page, const void *data);
/* 4 KiB page with metadata, the user-page format, same gate */
int nand_boot_program_user_page(uint32_t bank, uint32_t page,
                                const void *data, const uint32_t *meta);
#endif

struct nand_read
{
    uint32_t bank;
    uint32_t page;
    void *buf;
    uint32_t *meta;
    int ecc;
};

int nand_read_pages(struct nand_read *r, unsigned int n);

#define NAND_OP_FAILED      (-2)
int nand_erase_block(uint32_t bank, uint32_t block);
int nand_write_page(uint32_t bank, uint32_t page, const void *databuf,
                    const uint32_t *meta);

struct nand_write
{
    uint32_t bank;
    uint32_t page;
    const void *buf;
    const uint32_t *meta;
};

int nand_write_pages(const struct nand_write *w, unsigned int n,
                     bool two_plane, uint32_t *failbank);

unsigned int nand_get_bank_count(void);
uint32_t nand_chip_id(void);
/* FTL sector where the disk offered to Rockbox and USB begins */
uint32_t nand_user_disk_start(void);
const struct nand_geometry *nand_get_geometry(void);

/* Bootloader: load the original-firmware image from NAND into dest.
 * Returns 0 after the decrypted body has been copied, or a negative
 * number. Does not jump. */
int nandboot_load_onb(void *dest, size_t dest_max);

#endif /* __NAND_TARGET_H__ */
