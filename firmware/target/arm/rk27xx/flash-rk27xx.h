/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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

/* NAND access for the rk27xx flash translation layer.
 *
 * This layer drives the NAND controller and its BCH engine and nothing else:
 * it knows the chip's geometry and how to read, program, erase and copy
 * sectors, but not what the sectors mean. The FTL (ftl-scheme-a.c) is built
 * on it.
 *
 * ADDRESSES are sector numbers (512 bytes) in the FTL's linear view of the
 * chip, where a block is a SUPER-BLOCK: on a two-plane part, one physical
 * block from each plane, with consecutive pages alternating between the
 * planes. This layer maps that view to the chip; the FTL never sees planes.
 *
 * METADATA: every sector carries 16 spare bytes. The last 13 are the BCH
 * code and belong to the hardware; the first three are the FTL's, and are
 * what the meta arguments below carry - FLASH_META_SIZE bytes per sector.
 * Byte 1 is special: by default this layer writes 0x00 there on every
 * program, so that the FTL can tell a programmed page from an erased one
 * whatever it wrote. An FTL that keeps its own data in byte 1 turns that off
 * with flash_set_meta_passthrough().
 *
 * Only the first chip is supported; every device the FTL has been validated
 * on has one. */

#ifndef __FLASH_RK27XX_H__
#define __FLASH_RK27XX_H__

#include <stdbool.h>
#include <stdint.h>

#define FLASH_SECTOR_SIZE   512
#define FLASH_META_SIZE     3       /* FTL metadata bytes per sector */

/* The largest page the layer handles: 8 sectors per plane, two planes. */
#define FLASH_MAX_SEC_PER_PAGE  16

struct flash_geometry
{
    uint8_t  planes;            /* 1 or 2 */
    uint8_t  sec_per_page_raw;  /* sectors in one physical page */
    uint8_t  sec_per_page;      /* sectors in one page of every plane */
    uint16_t sec_per_block_raw; /* sectors in one physical block */
    uint16_t sec_per_block;     /* sectors in one super-block */
    uint32_t total_blocks;      /* super-blocks on the chip */
    uint32_t total_sectors;
};

/* Write-path error counters. Most write results do not change what the FTL
 * does next - it has no way to recover from most of them - so these are the
 * record that something went wrong. */
struct flash_stats
{
    uint32_t prog_failures;     /* programs the chip reported failed */
    uint32_t erase_failures;
    uint32_t timeouts;          /* the chip never became ready */
    uint32_t boot_area_skips;   /* writes into the boot area, dropped */
    uint32_t write_refusals;    /* writes attempted while not writable */
    uint32_t copy_uncorrectable;/* copy sources failing ECC, copied anyway */
    uint32_t refresh_pending;   /* reads near the ECC limit: blocks decaying */
};

/* Take the geometry flash_init() detected. Returns 0 if the chip is usable. */
int flash_layer_init(void);
const struct flash_geometry *flash_get_geometry(void);

/* Read n sectors from sec. data (n * 512 bytes) and meta (n * 3 bytes) may
 * each be NULL. Returns 0, or 1 if any sector was uncorrectable - its data
 * is then delivered as read. */
int flash_read(uint32_t sec, void *data, void *meta, unsigned n);

/* Program n sectors from sec, which must be erased. data NULL programs
 * 0xff; meta NULL programs {0xff, 0x00, 0xff} - {0xff, 0xff, 0xff} with
 * passthrough on. The last program is left running: its result arrives
 * with the next flash call - a program returns 1 if the one before failed -
 * or with flash_sync(). Returns 0 or 1 on failure. */
int flash_program(uint32_t sec, const void *data, const void *meta, unsigned n);

/* Wait for the program left running, if any. Returns 1 if it failed. */
int flash_sync(void);

/* Program the whole page (every plane) that contains sec. */
int flash_program_page(uint32_t sec, const void *data, const void *meta);

/* Erase the super-block containing sec. */
int flash_erase(uint32_t sec);

/* Copy n sectors (at most one super-block) from src to dst through the ECC
 * engine, so that correctable errors are not propagated. The destination
 * gets fresh metadata {0xff, 0x00, 0xff}. */
int flash_copy(uint32_t src, uint32_t dst, unsigned n);

/* As flash_copy(), but with chosen metadata: page_meta NULL keeps each
 * sector's own; otherwise sector k of every destination page is programmed
 * with page_meta[k] (sec_per_page * FLASH_META_SIZE bytes). */
int flash_copy_meta(uint32_t src, uint32_t dst, unsigned n,
                    const void *page_meta);

/* Program metadata byte 1 as given rather than 0x00. */
void flash_set_meta_passthrough(bool on);

/* The BCH strength of the FTL's area: t = 8 (the default) or 14, the
 * controller's BCH_T14 mode. flash_read_raw() always reads t=8, as the
 * boot area is on every device seen. Writing is refused in t=14 mode: its
 * sectors are 538-byte records on the media (3 + 23 spare bytes), and
 * programming them has not been tried.
 * Returns 0, or 1 for an unsupported t. */
int flash_set_ecc(unsigned t);

/* Read one PHYSICAL sector, bypassing the super-block view: for the boot
 * area, which the boot ROM addresses physically. */
int flash_read_raw(uint32_t raw_sec, void *data, void *meta);

/* Writes - program, erase and copy - are refused (return 1, counted) until
 * the FTL has confirmed it understands the media and enables them. */
void flash_set_writable(bool writable);

/* The first `sectors` sectors hold the boot area. A write there is dropped
 * and reported successful, as the original firmware does: no FTL error may
 * reach the bootloader. UINT32_MAX protects the whole chip. */
void flash_set_boot_area(uint32_t sectors);

void flash_get_stats(struct flash_stats *stats);

#endif /* __FLASH_RK27XX_H__ */
