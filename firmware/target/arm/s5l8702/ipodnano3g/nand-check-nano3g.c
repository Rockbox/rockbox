/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/*
 * The contributor check image (a bootloader built with -DNAND_CHECK, run
 * from DFU; utils/ipodnano3g/nandcheck/build.sh builds it). It identifies
 * the chip, tries the read-only mount, and serves a raw, read-only view of
 * the NAND over USB for utils/ipodnano3g/nandcheck/nandcheck.py, in
 * SECTOR_SIZE sectors:
 *
 *   0                              a text report, NUL padded
 *   1 + block * banks + bank       that block's spare records: per page the
 *                                  12 spare bytes and a result word, little
 *                                  endian: -1 if the read timed out, else
 *                                  NAND_ECC_* | the most bits the
 *                                  controller corrected in a chunk << 8 |
 *                                  0x10000 if it found the page erased
 *   metaend + page * secperpage    page data, bank by bank
 *
 * Nothing here writes to the NAND. This file is built only in a NAND_CHECK
 * build, where it provides the storage API the driver (nand-nano3g.c) then
 * leaves out: the disk served is the raw NAND, not the FTL's.
 */

#include "config.h"
#include "system.h"
#include "mv.h"
#include "storage.h"
#include "nand-target.h"
#include "ftl-target.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "version.h"

#define NAND_CHECK_RECORD   16
#define NAND_CHECK_BATCH    8   /* pages read per bank per driver call */
/* A block's records fill one sector: every row has 128 pages per block */
#define NAND_CHECK_PPB      (SECTOR_SIZE / NAND_CHECK_RECORD)

static uint32_t nand_check_ids[NAND_MAX_BANKS];
static uint32_t nand_check_ext[NAND_MAX_BANKS];     /* ID bytes 5-8 */
static int nand_check_len;                          /* report bytes used */
static char nand_check_text[SECTOR_SIZE];
static uint8_t nand_check_spares[NAND_MAX_BANKS][SECTOR_SIZE];
static uint32_t nand_check_block = 0xffffffff;
static uint32_t nand_check_page = 0xffffffff;
static uint8_t nand_check_buf[NAND_MAX_BANKS * NAND_CHECK_BATCH
                              * NAND_MAX_PAGE_SIZE] STORAGE_ALIGN_ATTR;
/* The chip to serve, taken from the driver at init: without one only the
 * report is served */
static struct nand_geometry nand_check_geo;
static bool nand_check_have_chip;

/* rc is nand_init_chip()'s result: 0, a NAND_ERR_*, or NAND_ERR_FTL plus
 * ftl_init()'s result */
void nand_check_init(int rc)
{
    const struct nand_geometry *g;
    uint32_t bank;
    int ftl_rc = rc <= NAND_ERR_FTL ? rc - NAND_ERR_FTL : rc ? 1 : 0;
    int n;

    for (bank = 0; bank < NAND_MAX_BANKS; bank++)
        if (nand_reset(bank) || nand_identify(&nand_check_ids[bank],
                                              &nand_check_ext[bank]))
            nand_check_ids[bank] = nand_check_ext[bank] = 0;
    /* Serve the raw NAND of any chip the driver identified, mounted or not.
     * The raw view keeps a block's records in one sector, so a chip whose
     * blocks are another size is reported but not read. */
    g = nand_check_use_chip(NAND_CHECK_PPB);
    nand_check_have_chip = g != NULL;
    if (g)
        nand_check_geo = *g;

    n = snprintf(nand_check_text, sizeof(nand_check_text),
                 "nano3g-nandcheck 1\n"
                 "version %s\n"
                 "ids %08lx %08lx %08lx %08lx\n"
                 "ext %08lx %08lx %08lx %08lx\n"
                 "banks %u\n"
                 "nand %d\n",
                 rbversion, (unsigned long)nand_check_ids[0],
                 (unsigned long)nand_check_ids[1],
                 (unsigned long)nand_check_ids[2],
                 (unsigned long)nand_check_ids[3],
                 (unsigned long)nand_check_ext[0],
                 (unsigned long)nand_check_ext[1],
                 (unsigned long)nand_check_ext[2],
                 (unsigned long)nand_check_ext[3], nand_get_bank_count(),
                 rc > NAND_ERR_FTL ? rc : 0);
    if (!g)
        n += snprintf(nand_check_text + n, sizeof(nand_check_text) - n,
                      "row none\n");
    else
        n += snprintf(nand_check_text + n, sizeof(nand_check_text) - n,
                      "row %d\n"
                      "mode %u\n"
                      "pagesize %u\n"
                      "blocks %u\n"
                      "ppb %u\n"
                      "userblocks %u\n"
                      "planes %u\n"
                      "vflspares %u\n"
                      "validated %d\n"
                      "ftl %d\n"
                      "sectors %lu\n",
                      nand_get_chip_row(), g->mode,
                      g->pagesize, g->blocks, g->pagesperblock,
                      g->userblocks, g->planes, g->vflspares,
                      g->validated, ftl_rc,
                      (unsigned long)ftl_num_sectors());
    n += snprintf(nand_check_text + n, sizeof(nand_check_text) - n,
             "verdict %s\n",
             rc == NAND_ERR_UNSUPPORTED ? "chip not in the chip table"
             : rc == NAND_ERR_MIXED ? "different chips on the chip enables"
             : !g ? "NAND did not answer"
             : ftl_rc == 0 ? (g->validated ? "OK, validated chip"
                                           : "OK, needs validation")
             : ftl_rc == -2 ? "geometry beyond FTL"
             : ftl_rc == -3 ? "no VFL context"
             : ftl_rc == -4 ? "layout not supported"
             : ftl_rc == -5 ? "VFL does not fit row"
             : "mount failed");
    nand_check_len = MIN(n, (int)sizeof(nand_check_text) - 1);
}

const char *nand_check_report(void)
{
    return nand_check_text;
}

void nand_check_note(const char *line)
{
    nand_check_len += snprintf(nand_check_text + nand_check_len,
                               sizeof(nand_check_text) - nand_check_len,
                               "%s\n", line);
    nand_check_len = MIN(nand_check_len, (int)sizeof(nand_check_text) - 1);
}

/* The first sector of page data: the report and every block's records */
static uint32_t nand_check_metaend(void)
{
    return 1 + nand_check_geo.blocks * nand_check_geo.banks;
}

static uint64_t nand_check_sector_count(void)
{
    if (!nand_check_have_chip)
        return 1;
    return nand_check_metaend()
         + (uint64_t)nand_check_geo.banks * nand_check_geo.blocks
           * nand_check_geo.pagesperblock
           * (nand_check_geo.pagesize / SECTOR_SIZE);
}

/* Every bank's spare records for one block, reading the banks in parallel */
static void nand_check_load_spares(uint32_t block)
{
    static struct nand_read r[NAND_MAX_BANKS * NAND_CHECK_BATCH];
    static uint32_t meta[NAND_MAX_BANKS * NAND_CHECK_BATCH][NAND_META_WORDS];
    uint32_t ppb = nand_check_geo.pagesperblock, p, q, bank, i, n;

    for (p = 0; p < ppb; p += NAND_CHECK_BATCH)
    {
        n = 0;
        for (q = p; q < p + NAND_CHECK_BATCH && q < ppb; q++)
            for (bank = 0; bank < nand_check_geo.banks; bank++, n++)
            {
                r[n].bank = bank;
                r[n].page = block * ppb + q;
                r[n].buf = nand_check_buf + n * nand_check_geo.pagesize;
                r[n].meta = meta[n];
            }
        if (nand_read_pages(r, n))
            for (i = 0; i < n; i++)
                r[i].ecc = -1;
        for (i = 0; i < n; i++)
        {
            uint8_t *rec = nand_check_spares[r[i].bank]
                         + (r[i].page % ppb) * NAND_CHECK_RECORD;
            int32_t ecc = r[i].ecc;
            uint32_t bits;

            if (ecc >= 0)
            {
                for (bits = 8; bits && !(r[i].result & (1u << (bits - 1)));
                     bits--);
                ecc |= bits << 8;
                if (r[i].result & (1u << 29))
                    ecc |= 0x10000;
            }

            memcpy(rec, meta[i], NAND_META_WORDS * 4);
            memcpy(rec + 12, &ecc, 4);
        }
    }
    nand_check_block = block;
    nand_check_page = 0xffffffff;
}

static void nand_check_sector(uint32_t sector, uint8_t *dst)
{
    uint32_t metaend, spp, perbank, g;

    if (sector == 0)
    {
        memcpy(dst, nand_check_text, SECTOR_SIZE);
        return;
    }
    if (!nand_check_have_chip)
    {
        memset(dst, 0, SECTOR_SIZE);
        return;
    }
    metaend = nand_check_metaend();
    spp = nand_check_geo.pagesize / SECTOR_SIZE;
    perbank = nand_check_geo.blocks * nand_check_geo.pagesperblock;
    if (sector < metaend)
    {
        uint32_t block = (sector - 1) / nand_check_geo.banks;

        if (block != nand_check_block)
            nand_check_load_spares(block);
        memcpy(dst, nand_check_spares[(sector - 1) % nand_check_geo.banks],
               SECTOR_SIZE);
    }
    else
    {
        g = (sector - metaend) / spp;
        if (g != nand_check_page)
        {
            /* Data as the chip gives it, whatever the ECC verdict: the spare
             * records say which pages failed */
            nand_read_page(g / perbank, g % perbank, nand_check_buf, NULL);
            nand_check_page = g;
            nand_check_block = 0xffffffff;
        }
        memcpy(dst, nand_check_buf + (sector - metaend) % spp * SECTOR_SIZE,
               SECTOR_SIZE);
    }
}

/* ---- Rockbox storage API: the check's read-only raw NAND ---- */

int nand_read_sectors(IF_MD(int drive,) sector_t start, int incount,
                      void* inbuf)
{
    IF_MD((void)drive);
    uint8_t *buf = inbuf;

    if (start + incount > nand_check_sector_count())
        return -1;
    while (incount--)
    {
        nand_check_sector((uint32_t)start++, buf);
        buf += SECTOR_SIZE;
    }
    nand_spin();
    return 0;
}

int nand_write_sectors(IF_MD(int drive,) sector_t start, int count,
                       const void* outbuf)
{
    IF_MD((void)drive);
    (void)start;
    (void)count;
    (void)outbuf;
    return -1;
}

#ifdef HAVE_STORAGE_READONLY
bool nand_readonly(IF_MD_NONVOID(int drive))
{
    IF_MD((void)drive);
    return true;
}
#endif

int nand_event(long id, intptr_t data)
{
    (void)id;
    (void)data;
    return 0;
}

#ifdef STORAGE_GET_INFO
void nand_get_info(IF_MD(int drive,) struct storage_info *info)
{
    IF_MD((void)drive);
    info->sector_size = SECTOR_SIZE;
    info->num_sectors = nand_check_sector_count();
    info->vendor = "Rockbox";
    info->product = "Nano 3G NAND";
    info->revision = "chk1";
}
#endif
