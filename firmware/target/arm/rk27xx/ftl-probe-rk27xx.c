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

/* The FTL scheme finder - see ftl-probe-rk27xx.h.
 *
 * The two formats tell themselves apart in the metadata of a block's first
 * sector (flash-rk27xx.h): Scheme A marks its remap-log blocks with type
 * 0x52 in byte 2 (ftl-scheme-a.c), Scheme B keeps a 0xfxxx tag in bytes 0-1
 * - 0xf000 for its bad-block table near the start of the chip, 0xf100 and
 * 0xf200 for its cache and data blocks (ftl-scheme-b.c). Every zone of
 * Scheme A has two log blocks, the first zone within its first 256 blocks;
 * Scheme B's table lies within 50 blocks of the boot area. So the first
 * SCAN_BLOCKS blocks decide.
 *
 * They are read in the BCH mode the FTL's area uses, which need not be the
 * boot area's t=8: the Archos Vision writes its FTL area with t=14. ID
 * block 1 of the later generation - 'RK27' at 0x0a - records it at 0x1ed
 * (8 on the HM-601, 14 on the Archos); the earlier generation's ID block
 * has no such field, and its devices are t=8 throughout. */

#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "nand-target.h"
#include "flash-rk27xx.h"
#include "ftl-probe-rk27xx.h"

#define SCAN_BLOCKS         512

/* ID blocks: every IDB_STRIDE-th raw sector of the boot area, metadata type
 * IDB_TYPE; ID block 1 is the sector after (ftl-rk27xx.c) */
#define IDB_STRIDE          512
#define IDB_POSITIONS       50
#define IDB_TYPE            0x69
#define IDB1_MAGIC          0x0a    /* 'RK27' in the later generation's ... */
#define IDB1_ECC_T          0x1ed   /* ... which records the BCH t here */
#define DEFAULT_ECC_T       8

#define A_LOG_TYPE          0x52
#define B_TAG_TABLE         0xf000
#define B_TAG_CACHE         0xf100
#define B_TAG_DATA          0xf200

static void probe_idb(struct ftl_probe *p)
{
    uint8_t data[FLASH_SECTOR_SIZE], meta[FLASH_META_SIZE];
    uint32_t pos;
    bool found = false;

    for (pos = 0; pos < IDB_POSITIONS && !found; pos++)
    {
        uint32_t raw = pos * IDB_STRIDE;

        if (flash_read_raw(raw, data, meta) == 0 && meta[2] == IDB_TYPE &&
            flash_read_raw(raw + 1, data, meta) == 0)
        {
            p->idb_boot_blocks = (uint16_t)(data[0] | data[1] << 8);
            p->idb_sys_mb      = (uint16_t)(data[2] | data[3] << 8);
            p->idb_data_mb     = (uint16_t)(data[4] | data[5] << 8);
            p->idb_rk27        = memcmp(data + IDB1_MAGIC, "RK27", 4) == 0;
            if (p->idb_rk27)
            {
                p->idb_ecc_t = data[IDB1_ECC_T];
            }
            found = true;
        }
    }
}

static void probe_block(struct ftl_probe *p, uint32_t blk)
{
    uint8_t meta[FLASH_META_SIZE];
    uint16_t tag;

    if (flash_read(blk * p->sec_per_block, NULL, meta, 1) != 0)
    {
        p->unreadable++;
    }
    else
    {
        tag = (uint16_t)(meta[0] | meta[1] << 8);

        /* tag 0xffff: erased, or a block only its byte 2 marks - an ID
         * block, type 0x69 */
        if (tag == 0xffff)
        {
            p->erased++;
        }
        else if (meta[0] == 0xff && meta[1] == 0x00 && meta[2] == A_LOG_TYPE)
        {
            p->a_logs++;
        }
        else if (meta[1] == 0x00)
        {
            /* Scheme A's flash layer zeroes byte 1 of every page */
            p->a_blocks++;
        }
        else if (tag == B_TAG_TABLE)
        {
            if (p->b_tables++ == 0)
            {
                p->first_b_table = (uint16_t)blk;
            }
        }
        else if ((tag & 0xff00) == B_TAG_CACHE)
        {
            p->b_cache++;
        }
        else if ((tag & 0xff00) == B_TAG_DATA)
        {
            p->b_data++;
        }
        else if ((tag & 0xf000) == 0xf000)
        {
            if (p->b_other++ == 0)
            {
                p->first_b_other = tag;
            }
        }
        else if (p->other++ == 0)
        {
            p->first_other = (uint16_t)blk;
            memcpy(p->first_other_meta, meta, sizeof(p->first_other_meta));
        }
    }
}

void ftl_probe(struct ftl_probe *p)
{
    const struct flash_geometry *geo;
    uint32_t blk;

    memset(p, 0, sizeof(*p));
    p->first_b_table = 0xffff;

    flash_init();
    p->flash_error = flash_layer_init();

    if (p->flash_error == 0)
    {
        geo = flash_get_geometry();
        p->planes = geo->planes;
        p->sec_per_page = geo->sec_per_page;
        p->sec_per_block = geo->sec_per_block;
        p->blocks = geo->total_blocks;

        probe_idb(p);

        /* a value the controller has no mode for is reported, and t=8 used */
        p->scan_ecc_t = DEFAULT_ECC_T;
        if (p->idb_rk27 && flash_set_ecc(p->idb_ecc_t) == 0)
        {
            p->scan_ecc_t = p->idb_ecc_t;
        }

        for (blk = 0; blk < SCAN_BLOCKS && blk < p->blocks; blk++)
        {
            probe_block(p, blk);
        }
        p->scanned = (uint16_t)blk;
        flash_set_ecc(DEFAULT_ECC_T);

        if (p->b_tables > 0 && p->b_data > 0)
        {
            p->scheme = p->b_other > 0 ? FTL_PROBE_SCHEME_B_OTHER
                                       : FTL_PROBE_SCHEME_B;
        }
        else if (p->a_logs >= 2 && p->b_tables == 0)
        {
            p->scheme = FTL_PROBE_SCHEME_A;
        }
        else if (p->unreadable > p->scanned / 2)
        {
            p->scheme = FTL_PROBE_UNREADABLE;
        }
    }
}

const char *ftl_probe_scheme_name(enum ftl_probe_scheme scheme)
{
    const char *name = "unknown";

    switch (scheme)
    {
    case FTL_PROBE_SCHEME_A:
        name = "A";
        break;
    case FTL_PROBE_SCHEME_B:
        name = "B";
        break;
    case FTL_PROBE_UNREADABLE:
        name = "unknown, ECC fails";
        break;
    case FTL_PROBE_SCHEME_B_OTHER:
        name = "B, other generation";
        break;
    default:
        break;
    }
    return name;
}
