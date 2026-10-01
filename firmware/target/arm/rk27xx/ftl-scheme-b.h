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

/* The "Scheme B" flash translation layer of rk27xx devices - the
 * self-describing on-flash format the original firmware of the HiFiMAN
 * HM-601 and similar players uses, read and written compatibly so that the
 * original firmware keeps working on the same media. A reimplementation
 * from reverse engineering; see ftl-scheme-b.c for how it works. */

#ifndef __FTL_SCHEME_B_H__
#define __FTL_SCHEME_B_H__

#include <stdbool.h>
#include <stdint.h>

struct ftl_b_config
{
    /* The first block the FTL owns; the boot area is everything before it.
     * Not recorded in the FTL's own structures. */
    uint16_t first_block;
    /* Exchange blocks open at once. The original firmware's own count must
     * not be exceeded: its mount recovers no more than that many, and loses
     * the data of the rest. 8 on the HM-601. */
    uint8_t  exch_blocks;
    /* Never write, not even the repairs a mount normally makes. */
    bool     read_only;
};

/* Why a mount failed */
enum ftl_b_error
{
    FTL_B_OK = 0,
    FTL_B_ERR_GEOMETRY,         /* the chip is not one this FTL can map */
    FTL_B_ERR_CONFIG,           /* an unusable configuration */
    FTL_B_ERR_NO_TABLE,         /* no bad-block table: not formatted */
    FTL_B_ERR_TABLE,            /* the bad-block table is damaged */
    FTL_B_ERR_VERSION,          /* a format generation not supported */
    FTL_B_ERR_TOO_BIG,          /* more logical blocks than fit in RAM */
};

struct ftl_b_status
{
    enum ftl_b_error error;
    bool     writable;
    uint16_t format_version;    /* as recorded in the bad-block table */
    uint16_t logical_blocks;
    uint16_t bad_blocks;
    uint16_t free_blocks;       /* erased or erasable, queued for use */
    uint8_t  open_exch;         /* exchange blocks in progress */
    uint8_t  cached_pages;      /* pages held in the write cache */
};

/* Mount the media. Returns FTL_B_OK or the reason it failed. */
enum ftl_b_error ftl_b_mount(const struct ftl_b_config *config);
void ftl_b_get_status(struct ftl_b_status *status);

/* Sectors in the logical space; 0 when not mounted */
uint32_t ftl_b_capacity(void);

/* Return 0, or non-zero if the request was refused or a sector could not be
 * read reliably. Sectors are 512 bytes. */
int ftl_b_read(uint32_t sector, void *buf, uint32_t count);
int ftl_b_write(uint32_t sector, const void *buf, uint32_t count);

/* Every write is on the flash when ftl_b_write() returns - the write cache
 * is journalled as it changes - so there is nothing to write out. Kept for
 * the storage layer's interface. */
void ftl_b_sync(void);

#endif /* __FTL_SCHEME_B_H__ */
