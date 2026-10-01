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

/* Which flash translation layer formatted an rk27xx target's NAND - for
 * targets whose config does not define CONFIG_RK27XX_FTL yet. Read-only:
 * it looks at the first page of the first blocks and nothing else. */

#ifndef __FTL_PROBE_RK27XX_H__
#define __FTL_PROBE_RK27XX_H__

#include <stdbool.h>
#include <stdint.h>

enum ftl_probe_scheme
{
    FTL_PROBE_UNKNOWN = 0,
    FTL_PROBE_SCHEME_A,         /* remap-log blocks: ftl-scheme-a.c */
    FTL_PROBE_SCHEME_B,         /* bad-block table and data headers:
                                 * ftl-scheme-b.c */
    FTL_PROBE_SCHEME_B_OTHER,   /* Scheme B tags of another generation */
    FTL_PROBE_UNREADABLE,       /* most blocks fail ECC in the mode used */
};

struct ftl_probe
{
    enum ftl_probe_scheme scheme;
    int      flash_error;       /* flash_layer_init() result; 0 = chip found */

    /* the chip */
    uint8_t  planes;
    uint8_t  sec_per_page;      /* both planes */
    uint16_t sec_per_block;
    uint32_t blocks;

    /* ID block 1, 0 if none found */
    uint16_t idb_boot_blocks;   /* raw blocks */
    uint16_t idb_sys_mb;
    uint16_t idb_data_mb;
    bool     idb_rk27;          /* the later layout, with 'RK27' at 0x0a ... */
    uint8_t  idb_ecc_t;         /* ... and the FTL area's BCH t at 0x1ed */

    uint8_t  scan_ecc_t;        /* the BCH t the blocks were read with */

    /* what page 0 of the blocks scanned holds */
    uint16_t scanned;
    uint16_t unreadable;
    uint16_t erased;
    uint16_t a_logs;            /* Scheme A remap-log blocks */
    uint16_t a_blocks;          /* other blocks Scheme A has programmed */
    uint16_t b_tables;          /* Scheme B: 0xf000 bad-block table */
    uint16_t b_cache;           /* 0xf1xx */
    uint16_t b_data;            /* 0xf2xx */
    uint16_t b_other;           /* other 0xfxxx tags */
    uint16_t other;             /* none of these */
    uint16_t first_b_table;     /* block of the first 0xf000; 0xffff = none */
    uint16_t first_b_other;     /* the first other 0xfxxx tag; 0 = none */
    uint16_t first_other;       /* first block of none of these ... */
    uint8_t  first_other_meta[3]; /* ... and its metadata */
};

/* Run the probe; takes a fraction of a second */
void ftl_probe(struct ftl_probe *p);

const char *ftl_probe_scheme_name(enum ftl_probe_scheme scheme);

#endif /* __FTL_PROBE_RK27XX_H__ */
