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
 * Interface modelled on
 * firmware/target/arm/s5l8700/ipodnano2g/nand-target.h.
 *
 * Copyright (C) 2026 by Andrew Rice
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

/* The smallest page, which is also the storage layer's sector: 4KiB-page
 * chips hold two sectors per page */
#define NAND_PAGE_SIZE      2048
#define NAND_MAX_PAGE_SIZE  4096

/* The on-flash VFL suggests four banks on the 4GB model. FMCTRL0 has chip
 * enable bits for eight. */
#define NAND_MAX_BANKS      4

/* Non-negative results of nand_read_page(): the ECC verdict of Apple's
 * the firmware's page read. Every written page reads as CORRECTED and every
 * erased page
 * as CLEAN (measured over a whole 4GB unit), so CORRECTED is the normal
 * result for data. FAILED and STATUS6 are named from how the ROM derives
 * them and have not been seen. */
#define NAND_ECC_CLEAN      0
#define NAND_ECC_CORRECTED  1
#define NAND_ECC_FAILED     3
#define NAND_ECC_STATUS6    6

/* Size of the spare metadata nand_read_page() returns, in 32-bit words.
 * This is the 12-byte Whimory spare header (lpn/usn/type/eccmark). */
#define NAND_META_WORDS     3

/* How a VFL block's planes sit on a bank (Apple's mode functions for the
 * FIL, which the original firmware installs per chip mode) */
#define NAND_LAYOUT_UNKNOWN     0
#define NAND_LAYOUT_SINGLE      1   /* one block: mode 1 */
#define NAND_LAYOUT_ADJACENT    2   /* block 2b + plane: modes 3, 8 */
#define NAND_LAYOUT_HALVES      3   /* b + plane * blocks / 2: 2, 9, 12 */
#define NAND_LAYOUT_BOTH        4   /* both of the above: mode 4 */
#define NAND_LAYOUT_SPLIT13     5   /* Toshiba's 8320 blocks: mode 13 */

struct nand_geometry
{
    unsigned int banks;
    unsigned int blocks;        /* per bank */
    unsigned int pagesperblock;
    unsigned int planes;        /* physical blocks per bank in a VFL block */
    unsigned int userblocks;    /* per bank, as Whimory counts them */
    unsigned int vflspares;     /* VFL reserved blocks per plane */
    bool twoplane;              /* whole rows may be programmed two-plane */
    unsigned int pagesize;      /* bytes of data per page */
    unsigned int mode;          /* Apple's chip table mode */
    unsigned int layout;        /* NAND_LAYOUT_* */
    bool validated;             /* may be mounted writable */
};

/* Reset one bank (chip enable) and select it. Returns 0 or a NAND_ERR_*. */
int nand_reset(uint32_t bank);

/* Read the first four ID bytes of the bank the last nand_reset() selected.
 * ext, if not NULL, gets the next four. Returns 0, or -1 on a timeout. */
int nand_identify(uint32_t *id, uint32_t *ext);

/* Read one page. meta may be NULL. Returns a negative number if the
 * controller timed out, else one of NAND_ECC_*. */
int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta);

/* One page of a read run. nand_read_pages() fills ecc with one of
 * NAND_ECC_* when the controller completed the page. */
struct nand_read
{
    uint32_t bank;
    uint32_t page;
    void *buf;
    uint32_t *meta;
    int ecc;
    uint32_t result;    /* the read program's result bitmap: bit 30
                         * uncorrectable, bit 29 erased, bit n-1 an n-bit
                         * correction in some chunk */
};

/* Read a run of pages with Apple's firmware read sequence, overlapping the
 * NAND page-load time on the chip enables in the run. Returns 0 if every
 * page completed or -1 on a controller timeout; inspect each page's ecc for
 * its ECC verdict. */
int nand_read_pages(struct nand_read *r, unsigned int n);

/* Erase one block, or write one page with its NAND_META_WORDS words of spare
 * metadata. Return 0, -1 if the controller timed out, or NAND_OP_FAILED if
 * the chip reports a failure (the block should be treated as bad). */
#define NAND_OP_FAILED      (-2)
int nand_erase_block(uint32_t bank, uint32_t block);
int nand_write_page(uint32_t bank, uint32_t page, const void *databuf,
                    const uint32_t *meta);

/* One page of a run: where it goes, its data and its spare metadata */
struct nand_write
{
    uint32_t bank;
    uint32_t page;
    const void *buf;
    const uint32_t *meta;
};

/* Program a run of pages at once, as Apple's firmware does: the chips
 * program in parallel, each waited for only when it is given its next
 * page, and with two_plane the pages come in pairs - plane 0 then plane 1
 * of one bank - programmed by one command each. Returns as
 * nand_write_page(). After a failure *failbank (if not NULL) is the bank
 * that was being programmed or reported it, and any page of the run may not
 * have been written. */
int nand_write_pages(const struct nand_write *w, unsigned int n,
                     bool two_plane, uint32_t *failbank);

/* nand_init() results. -4 is retired: early builds used it for a ready
 * bit timeout that turned out not to be an error. */
enum nand_init_result
{
    NAND_ERR_RESET_CMD   = -1,   /* RESET never completed */
    NAND_ERR_IDENTIFY    = -2,   /* READ ID failed */
    NAND_ERR_READY_CMD   = -3,   /* READ STATUS never completed */
    NAND_ERR_UNSUPPORTED = -5,   /* not a chip Rockbox has been tested on */
    NAND_ERR_MIXED       = -6,   /* chip enables answer with different ids */
    NAND_ERR_FTL         = -100, /* plus ftl_init()'s code */
};

/* The READ ID bytes bank 0 answered with, 0 before nand_init() */
uint32_t nand_get_id(void);

/* Number of banks (chip enables) nand_init() found holding the same chip
 * as bank 0 */
unsigned int nand_get_bank_count(void);

/* Geometry of the attached chips, or NULL before nand_init() succeeded */
const struct nand_geometry *nand_get_geometry(void);

#endif /* __NAND_TARGET_H__ */
