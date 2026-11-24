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
 * Whimory FTL for the Nano 4G. The FTL control-block layout is the Nano
 * 3G one's (ftl-nano3g.c), under a VSVFL. A 4 KiB page still carries the
 * 0x800-byte context at the start; a checksum miss fails the mount. The
 * mount is read-only unless the build defines N4G_FTL_WRITE; ftl_format()
 * is refused.
 *
 * Copyright (C) 2026 by Andrew Rice
 * Copyright (C) 2026 by Robin Tschirschnitz
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
 * FTL for the iPod Nano 4G: Apple's Whimory. The VFL is
 * VSVFL, as in Apple's EFI NAND driver for this model; the FTL context is
 * the Nano 3G one (each 0x800 bytes). Pages are 4 KiB, so the tables below
 * are sized for an 8 GB or 16 GB part. The bootrom's own FTL only loads the
 * IMG1 image; it is not this filesystem.
 *
 *
 * From the bottom up:
 *  - VFL: the context's vendor type splits each chip enable into one, two
 *    or four ways; a virtual bank is one way of one chip enable, and
 *    virtual bank vb is way vb / banks of chip enable vb % banks. Block b
 *    of a virtual bank is the physical block Apple's address conversion
 *    gives (ftl_vfl_c2p()), unless that chip enable's context lists it in
 *    its pool map: slot i stands in for poolmap[i]. Apple's VSVFL_Open
 *    builds its good-block bitmap from the same pool map, so a listed
 *    block is exactly a block Apple takes as remapped.
 *  - Superblocks: superblock s is block s of every virtual bank. Its pages
 *    are numbered v = page * vbanks + vbank.
 *  - FTL: logical page lpn is at index lpn % sbpages of logical block
 *    lpn / sbpages. The block map, committed in the FTL control blocks, says
 *    which superblock holds each logical block. Pages written since a block
 *    was last merged are in log superblocks, which the context and its log
 *    offset tables record at every commit; a page present in a log is newer
 *    than the one the map points at, and ftl_resolve() prefers it.
 *
 * The write path follows Apple's own FTL in this firmware (osos 1.1.3):
 * FTL_Write appends to logs, _Merge folds or compacts them when they fill or
 * their slot is needed, and ftl_sync() folds every log and commits, as
 * FTL_Flush does. Nothing else commits, so a mount that finds writes after
 * the last commit rebuilds the tables from the medium as _FTLRestore does.
 * Pages go to the flash in runs, as VFL_WriteMultiplePagesInVb sends them.
 * A block that fails a program goes on its chip enable's scrub list and is
 * replaced from the VFL's reserved spares when it is next erased; one that
 * fails an erase is replaced at once. Either way the VFL context is stored
 * as Apple's VSVFL stores it, and the FTL carries on in another superblock.
 */

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "panic.h"
#include "storage.h"
#include "nand-target.h"
#include "ftl-target.h"
#ifndef BOOTLOADER
#include "button.h"
#endif
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define FTL_MAX_BANKS       4
/* Generous: an 8 GB part has 3904 superblocks of 512 pages */
#define FTL_MAX_SB          65536   /* superblocks in the virtual area */
#define FTL_MAX_USERSB      65536   /* logical blocks */
#define FTL_MAX_SBPAGES     4096    /* pages per superblock */
#define FTL_MAX_LOGS        17      /* Whimory keeps 0x11 log blocks */
#define FTL_MAP_PAGES       (FTL_MAX_USERSB * 2 / NAND_PAGE_SIZE)
#define FTL_VFL_SCANBLOCKS  200     /* where to look for a VFL context, as Apple */
#define FTL_NO_PAGE         0xffff
#define FTL_RESERVED_SB     23      /* Apple's FTL: 3 control + 20 pool */

/* Spare metadata types */
#define SPARE_DATA          0x40
#define SPARE_DATA_LAST     0x41    /* last page of a superblock */
#define SPARE_FTL_CXT       0x43
#define SPARE_FTL_MAP       0x44
#define SPARE_FTL_LOG       0x45    /* log page-offset table */
#define SPARE_FTL_ERASECTR  0x46
#define SPARE_FTL_STATS     0x47
#define SPARE_FTL_READCOUNT 0x49
#define SPARE_FTL_MOUNT     0x4f    /* "written to since the last commit" */
#define SPARE_VFL_CXT       0x80
#define SPARE_ERASED        0xff

/* The FTL context, as it sits in its flash page. The named fields are the
 * ones we understand; the pad arrays keep everything else exactly as we
 * found it, so a commit of ours preserves what Apple's firmware expects.
 * Field offsets were derived from Apple's own _LoadFTLCxt and _StoreFTLCxt,
 * and the FTL_CHECK assertions below pin every one of them. */
#define FTL_POOL_SIZE       20      /* spare superblocks: free pool + logs */
#define FTL_CXT_LOGS        18      /* log slots in the context */
#define FTL_CXT_PAGELIST    36      /* entries in the side-table page lists */

struct ftl_cxt_log
{
    uint32_t usn;
    uint16_t sb;
    uint16_t lblock;
    uint32_t offsetsptr;        /* Whimory's RAM pointer, preserved unused */
    uint16_t pagesused;
    uint16_t pagescurrent;
    uint32_t issequential;
};

struct ftl_cxt
{
    uint32_t usn;               /* decrements once per control page */
    uint32_t maxusn;            /* highest user-data usn */
    uint16_t freecount;         /* live entries in blockpool */
    uint16_t nextfreeidx;       /* head of the free ring */
    uint16_t swapcounter;       /* wear-levelling budget: +1 per merge */
    uint16_t blockpool[FTL_POOL_SIZE];
    uint8_t field_36[2];
    uint32_t mappages[18];      /* vpages holding the block map */
    uint32_t ecpages[FTL_CXT_PAGELIST];     /* ... the erase counters */
    uint32_t logpages[FTL_MAX_LOGS];        /* ... the log offset tables */
    uint8_t field_154[0x44];
    uint32_t mapptr;            /* three more of Whimory's RAM pointers */
    uint32_t ecptr;
    uint32_t logoffsptr;
    struct ftl_cxt_log logs[FTL_CXT_LOGS];
    uint8_t field_30c[6];
    uint16_t ctrlblocks[3];
    uint32_t ctrlpage;
    uint32_t cleanflag;         /* set by a commit, cleared by a write */
    uint32_t rcpages[FTL_CXT_PAGELIST];     /* ... the read counters */
    uint32_t rcptr;
    uint8_t field_3b4[0x1c];
    uint32_t statspage;
    uint32_t statsflag;         /* 0xffffffff = statspage is set */
    uint8_t field_3d8[0x428];
};

#define FTL_SIDE_ENTRIES    (FTL_MAX_SB + 32)   /* per-superblock tables */
#define FTL_SIDE_PAGES      ((FTL_SIDE_ENTRIES * 2 + NAND_PAGE_SIZE - 1) \
                             / NAND_PAGE_SIZE)
#define FTL_LOG_PAGES       ((FTL_MAX_SBPAGES * FTL_MAX_LOGS * 2 \
                              + NAND_PAGE_SIZE - 1) / NAND_PAGE_SIZE)
/* Pages one commit occupies: erase counters, read counters, map, log
 * tables, stats, then the context itself */
#define FTL_COMMIT_PAGES(ec, rc, map, log) ((ec) + (rc) + (map) + (log) + 2)

/* VSVFL context, one per bank (chip enable). Apple's driver has the legacy
 * VFL too, but this NAND carries VSVFL: its _GetFTLCtrlBlock returns the
 * newest context + 0x69a, its info 0x70 is the halfword at 0x696, and the
 * contexts read back from the medium fit this layout (measured). */
struct ftl_vfl_cxt
{
    uint32_t usn;                   /* usn_inc: highest bank is newest */
    uint32_t updatecount;           /* usn_dec, also in the spare */
    uint32_t ftltype;
    uint16_t usnblock;              /* ring index of the active block */
    uint16_t usnpage;               /* pages of it used */
    uint16_t badcount;              /* blocks the format found bad */
    uint16_t field_12;
    uint16_t erasefail;             /* erases that failed */
    uint16_t cursor[8];             /* per way: next pool slot to try */
    uint16_t poolmap[820];          /* block each pool slot stands in for */
    uint16_t vflcxtblocks[4];       /* physical blocks of the context ring */
    uint16_t usableblocks;          /* virtual blocks per bank */
    uint16_t poolstart;             /* first reserved block */
    uint16_t ftlctrlblocks[3];      /* virtual blocks of the FTL control */
    uint16_t scrubcount;            /* blocks to replace at their next erase */
    uint16_t scrub[20];
    uint16_t field_6ca[8];          /* a pending FTL geometry change */
    uint16_t vendortype[2];
    uint8_t field_6de[0x116];
    uint32_t version;
    uint32_t checksum1;
    uint32_t checksum2;
};

/* Pool map entries other than the block a slot stands in for */
#define FTL_POOL_FREE       0xfff0
#define FTL_POOL_BAD        0xffff

#define FTL_CHECK(name, cond) typedef char name[(cond) ? 1 : -1]
FTL_CHECK(ftl_vfl_cxt_size, sizeof(struct ftl_vfl_cxt) == 0x800);
FTL_CHECK(ftl_vfl_cxt_cursor, offsetof(struct ftl_vfl_cxt, cursor) == 0x16);
FTL_CHECK(ftl_vfl_cxt_pool, offsetof(struct ftl_vfl_cxt, poolmap) == 0x26);
FTL_CHECK(ftl_vfl_cxt_blocks,
          offsetof(struct ftl_vfl_cxt, vflcxtblocks) == 0x68e);
FTL_CHECK(ftl_vfl_cxt_start,
          offsetof(struct ftl_vfl_cxt, poolstart) == 0x698);
FTL_CHECK(ftl_vfl_cxt_ctrl,
          offsetof(struct ftl_vfl_cxt, ftlctrlblocks) == 0x69a);
FTL_CHECK(ftl_vfl_cxt_scrub,
          offsetof(struct ftl_vfl_cxt, scrubcount) == 0x6a0);
FTL_CHECK(ftl_vfl_cxt_vendor,
          offsetof(struct ftl_vfl_cxt, vendortype) == 0x6da);
FTL_CHECK(ftl_vfl_cxt_cks, offsetof(struct ftl_vfl_cxt, checksum1) == 0x7f8);
FTL_CHECK(ftl_cxt_size, sizeof(struct ftl_cxt) == 0x800);
FTL_CHECK(ftl_cxt_log_size, sizeof(struct ftl_cxt_log) == 20);
FTL_CHECK(ftl_cxt_pool, offsetof(struct ftl_cxt, blockpool) == 0x00e);
FTL_CHECK(ftl_cxt_map, offsetof(struct ftl_cxt, mappages) == 0x038);
FTL_CHECK(ftl_cxt_ec, offsetof(struct ftl_cxt, ecpages) == 0x080);
FTL_CHECK(ftl_cxt_logp, offsetof(struct ftl_cxt, logpages) == 0x110);
FTL_CHECK(ftl_cxt_logs, offsetof(struct ftl_cxt, logs) == 0x1a4);
FTL_CHECK(ftl_cxt_ctrl, offsetof(struct ftl_cxt, ctrlblocks) == 0x312);
FTL_CHECK(ftl_cxt_clean, offsetof(struct ftl_cxt, cleanflag) == 0x31c);
FTL_CHECK(ftl_cxt_rc, offsetof(struct ftl_cxt, rcpages) == 0x320);
FTL_CHECK(ftl_cxt_stats, offsetof(struct ftl_cxt, statspage) == 0x3d0);

/* A log block: pages of one logical block written since the last commit,
 * scattered through a superblock of their own. Fields follow
 * ftl-nano2g.c's struct ftl_log_type, which is the same structure Apple
 * keeps at context offset 0x1a4. */
struct ftl_log
{
    uint32_t usn;
    uint16_t sb;                        /* the scattered superblock */
    uint16_t lblock;                    /* the logical block it stands for */
    uint16_t pagesused;                 /* append cursor */
    uint16_t pagescurrent;              /* pages still live, i.e. to copy */
    uint32_t issequential;              /* still written in order? */
    uint16_t offsets[FTL_MAX_SBPAGES];  /* index in lblock -> v in sb */
};

/* Source of replacement page data for ftl_write_merge(): a pointer to
 * NAND_PAGE_SIZE bytes for lpn, or NULL to keep what is there now. */
typedef const void *(*ftl_write_src)(uint32_t lpn, void *ctx);

static struct nand_geometry ftl_geo;
static const struct nand_geometry *geo;
static uint32_t pool_slots;     /* entries of each bank's pool map */
static uint32_t ftl_vendor;     /* VSVFL vendor type: address conversion */
static uint32_t ftl_ways;       /* ways per chip enable */
static uint32_t ftl_vbanks;     /* banks * ways: superblock stripe width */
static uint32_t nsuperblocks;   /* superblocks in the virtual area */
static uint32_t sbpages;        /* pages per superblock */
static uint32_t usersb;         /* logical blocks */
static bool ftl_mounted = false;
static bool ftl_vfl_ok = false;
static struct mutex ftl_mtx;

static struct ftl_vfl_cxt ftl_vfl_cxt[FTL_MAX_BANKS];
static uint16_t ftl_map[FTL_MAX_USERSB];
static uint16_t ftl_ctrl[3];
static struct ftl_log ftl_logs[FTL_MAX_LOGS];
static unsigned int ftl_nlogs;

static uint8_t ftl_buffer[NAND_PAGE_SIZE] STORAGE_ALIGN_ATTR;
static uint32_t ftl_meta[NAND_META_WORDS];

/* Write-path state. The committed context page is kept verbatim so that a
 * commit of ours preserves every field we do not understand. */
static bool ftl_writable = false;
static bool ftl_readonly = false;   /* not writable since the mount */
static struct ftl_cxt ftl_cxt STORAGE_ALIGN_ATTR;
static uint16_t ftl_erasectr[FTL_SIDE_PAGES * (NAND_PAGE_SIZE / 2)];
static uint16_t ftl_readcount[FTL_SIDE_PAGES * (NAND_PAGE_SIZE / 2)];
static uint8_t ftl_stats[NAND_PAGE_SIZE] STORAGE_ALIGN_ATTR;
static uint32_t ftl_vfl_usn;    /* cross-bank VFL context update counter */
static uint8_t ftl_vfl_page[NAND_PAGE_SIZE] STORAGE_ALIGN_ATTR;
static uint32_t ftl_ctrl_idx;   /* which of ftl_ctrl[] is being written */
static uint32_t ftl_ctrl_v;     /* next free page in it */
static uint32_t ftl_ctrl_good;  /* which of ftl_ctrl[] has the newest context */
static uint32_t ftl_necpages, ftl_nrcpages, ftl_nmappages, ftl_nlogpages;

/* The run of pages being gathered for one program call (see ftl_run_add) */
static struct nand_write ftl_run[FTL_MAX_SBPAGES];
static uint32_t ftl_run_meta[FTL_MAX_SBPAGES][NAND_META_WORDS];
static uint32_t ftl_run_sb, ftl_run_v, ftl_run_n;
/* The pages ftl_log_append() queued for a log, which ftl_log_flush()
 * enters into the log once they are programmed */
static uint16_t ftl_queued_idx[FTL_MAX_SBPAGES];
static uint32_t ftl_queued_n, ftl_queued_current;
static bool ftl_queued_sequential;
/* Merges read a batch of pages, then write it: an eighth of a superblock
 * at a time, as Apple's _DoSimpleMerge, _DoCopyMerge and _DoMoveMerge do */
#define FTL_COPY_PAGES      (FTL_MAX_SBPAGES / 8)
static uint8_t ftl_copybuf[FTL_COPY_PAGES * NAND_PAGE_SIZE] STORAGE_ALIGN_ATTR;
static struct nand_read ftl_read_run[FTL_MAX_SBPAGES];
static uint32_t ftl_read_meta[FTL_MAX_SBPAGES][NAND_META_WORDS];

static uint32_t meta_type(const uint32_t *meta)
{
    return (meta[2] >> 8) & 0xff;
}

static bool meta_is_data(const uint32_t *meta)
{
    return meta_type(meta) == SPARE_DATA || meta_type(meta) == SPARE_DATA_LAST;
}

/* Read a page into ftl_buffer and ftl_meta. Returns its spare type, or -1 */
static int ftl_raw_read(uint32_t bank, uint32_t page)
{
    int rc = nand_read_page(bank, page, ftl_buffer, ftl_meta);
    if (rc < 0 || rc == NAND_ECC_FAILED)
        return -1;
    return meta_type(ftl_meta);
}

static void ftl_vfl_calculate_checksum(const struct ftl_vfl_cxt *cxt,
                                       uint32_t *sum, uint32_t *x)
{
    const uint32_t *w = (const uint32_t *)cxt;
    unsigned int i;

    *sum = 0xAABBCCDD;
    *x = 0xAABBCCDD;
    for (i = 0; i < offsetof(struct ftl_vfl_cxt, checksum1) / 4; i++)
    {
        *sum += w[i];
        *x ^= w[i];
    }
}

static bool ftl_vfl_checksum_ok(const struct ftl_vfl_cxt *cxt)
{
    uint32_t sum, x;

    ftl_vfl_calculate_checksum(cxt, &sum, &x);
    return sum == cxt->checksum1 && x == cxt->checksum2;
}

/* A VFL context commit is 8 copies. Load the first good one at or after
 * page into ftl_buffer. */
static int ftl_vfl_read_cxt(uint32_t bank, uint32_t block, uint32_t page)
{
    uint32_t i;

    for (i = page; i < page + 8 && i < geo->pagesperblock; i++)
        if (ftl_raw_read(bank, block * geo->pagesperblock + i)
                == SPARE_VFL_CXT
            && (ftl_meta[2] & 0xff) == 0
            && ftl_vfl_checksum_ok((const struct ftl_vfl_cxt *)ftl_buffer))
            return 0;
    return -1;
}

/* Find the newest VFL context of a bank, as nano2g's ftl_vfl_open() does */
static int ftl_vfl_open_bank(uint32_t bank)
{
    uint16_t ring[4];
    uint32_t i, block, last, best = 4, bestusn = 0xffffffff;

    /* Any context lists the blocks the context ring lives in */
    for (block = 1; block < FTL_VFL_SCANBLOCKS; block++)
        if (!ftl_vfl_read_cxt(bank, block, 0))
            break;
    if (block == FTL_VFL_SCANBLOCKS)
        return -1;
    memcpy(ring, ((const struct ftl_vfl_cxt *)ftl_buffer)->vflcxtblocks,
           sizeof(ring));

    /* The ring's newest block has the lowest update count */
    for (i = 0; i < 4; i++)
    {
        if (ring[i] >= geo->blocks || ftl_vfl_read_cxt(bank, ring[i], 0))
            continue;
        if (ftl_meta[0] && ftl_meta[0] <= bestusn)
        {
            bestusn = ftl_meta[0];
            best = i;
        }
    }
    if (best == 4)
        return -1;

    /* Its last commit is the newest context */
    block = ring[best];
    last = 0;
    for (i = 8; i < geo->pagesperblock; i += 8)
    {
        if (ftl_vfl_read_cxt(bank, block, i))
            break;
        last = i;
    }
    if (ftl_vfl_read_cxt(bank, block, last))
        return -1;
    memcpy(&ftl_vfl_cxt[bank], ftl_buffer, sizeof(struct ftl_vfl_cxt));
    return 0;
}

/* VSVFL's address conversion (_pfnConvertC2P): block blk of virtual bank
 * vb as a physical block of its chip enable. The way is inserted into the
 * page address as one bit (two for four ways): the top block bit (die
 * select), block bit 0, or for type 0x11 a bit that depends on where the
 * block lies. Type 1 is the identity. */
static uint32_t ftl_vfl_c2p(uint32_t vb, uint32_t blk)
{
    uint32_t ppb = geo->pagesperblock, way = vb / geo->banks;
    uint32_t p = blk * ppb, half = ppb * geo->blocks / 2, m, u;

    switch (ftl_vendor & 0xffff)
    {
        case 0x10:
        case 0x12:
            m = half;
            break;
        case 0x11:
            u = (geo->blocks - 0x80) * ppb;
            m = (u >> 1) <= p ? ppb << 5 : u >> 2;
            break;
        case 0x13:
            u = (p & (ppb - 1)) | ((p & (half - 1) & ~(ppb - 1)) << 1)
              | ((p & ~(half - 1)) << 2);
            return (u | ((way & 1) ? ppb : 0) | ((way & 2) ? half : 0)) / ppb;
        case 0x14:
            m = ppb;
            break;
        default:
            return blk;
    }
    return (((way & 1) ? m : 0) | (p & (m - 1)) | ((p & ~(m - 1)) << 1))
           / ppb;
}

/* VSVFL's _GetPhysicalBlock: block vblock of virtual bank vb, or the pool
 * slot that stands in for it. Slot i of a chip enable's pool map is block
 * poolstart + i % per of way i / per. Bank 0 lists blocks 0 to 4 (boot and
 * context ring, measured); the FTL never uses them. */
static uint32_t ftl_vfl_phys_block(uint32_t vb, uint32_t vblock)
{
    uint32_t bank = vb % geo->banks, block = ftl_vfl_c2p(vb, vblock);
    const struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t i, per = geo->blocks / ftl_ways - cxt->poolstart;

    for (i = 0; i < pool_slots; i++)
        if (cxt->poolmap[i] == block)
            return ftl_vfl_c2p(geo->banks * (i / per) + bank,
                               cxt->poolstart + i % per);
    return block;
}

static void ftl_vpage_phys(uint32_t sb, uint32_t v,
                           uint32_t *bank, uint32_t *page)
{
    *bank = v % ftl_vbanks % geo->banks;
    *page = ftl_vfl_phys_block(v % ftl_vbanks, sb) * geo->pagesperblock
          + v / ftl_vbanks;
}

/* Read page v of superblock sb into ftl_buffer and ftl_meta */
static int ftl_read_vpage(uint32_t sb, uint32_t v)
{
    uint32_t bank, page;

    ftl_vpage_phys(sb, v, &bank, &page);
    return ftl_raw_read(bank, page);
}

static uint32_t ftl_vpn(uint32_t sb, uint32_t v)
{
    return sb * sbpages + v;
}

/* The spare types that live in FTL control blocks */
static bool type_is_ctrl(int type)
{
    return type >= SPARE_FTL_CXT && type <= SPARE_FTL_MOUNT;
}

/* Find the FTL context, as Apple's _LoadFTLCxt does: the newest control
 * block is the one whose page 0 has the lowest usn, and its last written
 * page must be a context. Returns 1 if it is - the context then describes
 * the medium exactly, open logs included - 0 if not, in which case the
 * medium has changed since that commit and ftl_restore() must rebuild it,
 * or a negative number if no context exists at all. Either way ftl_cxt
 * holds the newest context page found, so a commit of ours preserves the
 * fields we do not understand. */
static int ftl_load_cxt(void)
{
    const struct ftl_vfl_cxt *newest = &ftl_vfl_cxt[0];
    uint32_t i, v, ctrl = 3, cxtv = FTL_MAX_SBPAGES;
    uint32_t minusn = 0xffffffff, page0usn = 0xffffffff, newblk = 3;
    uint32_t used[3] = { 0, 0, 0 };
    int type, lasttype[3] = { -1, -1, -1 };
    bool clean;

    for (i = 1; i < geo->banks; i++)
        if (ftl_vfl_cxt[i].usn > newest->usn)
            newest = &ftl_vfl_cxt[i];
    memcpy(ftl_ctrl, newest->ftlctrlblocks, sizeof(ftl_ctrl));

    for (i = 0; i < 3; i++)
    {
        if (ftl_ctrl[i] >= nsuperblocks)
            continue;
        for (v = 0; v < sbpages; v++)
        {
            type = ftl_read_vpage(ftl_ctrl[i], v);
            if (type == SPARE_ERASED)
                break;
            if (v == 0 && type_is_ctrl(type) && ftl_meta[0] < page0usn)
            {
                page0usn = ftl_meta[0];
                newblk = i;
            }
            if (type == SPARE_FTL_CXT && ftl_meta[0] <= minusn)
            {
                minusn = ftl_meta[0];
                ctrl = i;
                cxtv = v;
            }
            lasttype[i] = type;
        }
        used[i] = v;
    }
    /* Clean: the newest block ends in a context. Otherwise keep the
     * newest context anywhere, for its fields. */
    clean = newblk < 3 && lasttype[newblk] == SPARE_FTL_CXT;
    if (clean)
    {
        ctrl = newblk;
        cxtv = used[newblk] - 1;
    }
    if (ctrl == 3 || ftl_read_vpage(ftl_ctrl[ctrl], cxtv) != SPARE_FTL_CXT)
        return -2;
    memcpy(&ftl_cxt, ftl_buffer, sizeof(ftl_cxt));
    ftl_cxt.usn = ftl_meta[0];
    ftl_ctrl_idx = ctrl;
    ftl_ctrl_v = used[ctrl];
    ftl_ctrl_good = ctrl;
    return clean;
}

/* Read npages pages of a context pointer list into table, entries bytes */
static int ftl_read_list(const uint32_t *list, uint32_t npages, int type,
                         void *table, uint32_t bytes)
{
    uint32_t i, vpn;

    for (i = 0; i < npages; i++)
    {
        vpn = list[i];
        if (vpn / sbpages >= nsuperblocks
            || ftl_read_vpage(vpn / sbpages, vpn % sbpages) != type
            || (ftl_meta[1] & 0xffff) != i)
            return -1;
        memcpy((uint8_t *)table + i * NAND_PAGE_SIZE, ftl_buffer,
               MIN(bytes - i * NAND_PAGE_SIZE, NAND_PAGE_SIZE));
    }
    return 0;
}

/* The rest of a clean load: map, open logs and free pool, all as the
 * context records them */
static int ftl_load_clean(void)
{
    static uint16_t table[FTL_LOG_PAGES * (NAND_PAGE_SIZE / 2)];
    uint32_t i, sb;

    if (ftl_read_list(ftl_cxt.mappages,
                      (usersb * 2 + NAND_PAGE_SIZE - 1) / NAND_PAGE_SIZE,
                      SPARE_FTL_MAP,
                      ftl_map, usersb * 2))
        return -3;
    for (i = 0; i < usersb; i++)
        if (ftl_map[i] >= nsuperblocks)
            return -4;

    /* One offset table of sbpages entries per log slot, in slot order */
    if (ftl_read_list(ftl_cxt.logpages,
                      (sbpages * FTL_MAX_LOGS * 2 + NAND_PAGE_SIZE - 1)
                      / NAND_PAGE_SIZE,
                      SPARE_FTL_LOG, table, sbpages * FTL_MAX_LOGS * 2))
        return -5;
    ftl_nlogs = 0;
    for (i = 0; i < FTL_MAX_LOGS; i++)
    {
        const struct ftl_cxt_log *c = &ftl_cxt.logs[i];
        const uint16_t *offsets = table + i * sbpages;
        struct ftl_log *log;
        uint32_t idx;

        if (c->sb == 0xffff)
            continue;
        if (c->sb >= nsuperblocks || c->lblock >= usersb
            || c->pagesused > sbpages || c->pagescurrent > sbpages)
            return -6;
        for (idx = 0; idx < sbpages; idx++)
            if (offsets[idx] != FTL_NO_PAGE && offsets[idx] >= sbpages)
                return -6;
        log = &ftl_logs[ftl_nlogs++];
        log->usn = c->usn;
        log->sb = c->sb;
        log->lblock = c->lblock;
        log->pagesused = c->pagesused;
        log->pagescurrent = c->pagescurrent;
        log->issequential = c->issequential;
        memcpy(log->offsets, offsets, sbpages * 2);
    }

    if (ftl_cxt.freecount > FTL_POOL_SIZE
        || ftl_cxt.nextfreeidx >= FTL_POOL_SIZE)
        return -7;
    for (i = 0; i < ftl_cxt.freecount; i++)
    {
        sb = ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE];
        if (sb >= nsuperblocks)
            return -7;
    }
    return 0;
}

/* ---- Apple's _FTLRestore -------------------------------------------- */
/* Rebuilds the map, the open logs and the free pool from the medium alone,
 * when the newest control page is not a context. Every phase follows the
 * decode of _FTLRestore (0x08069a4c) in osos 1.1.3; the letters are the
 * phases of that decode. Nothing is written here: the caller erases the
 * hole blocks and the pool and commits (phases D, H and J) when the mount
 * is writable. */

#define RS_LOG      0x40
#define RS_DATA     0x41
#define RS_CTRL     0x42
#define RS_FREE     0x48

struct ftl_rs
{
    uint16_t lblock;
    uint8_t state;
    uint32_t usn;
};
static struct ftl_rs ftl_rs[FTL_MAX_SB];
static uint8_t ftl_hole[FTL_MAX_USERSB / 8 + 1];    /* map given an unerased
                                                     * free block */
static uint32_t ftl_nholes;

static bool ftl_is_hole(uint32_t lblock)
{
    return ftl_hole[lblock / 8] & (1 << (lblock % 8));
}

/* Apple's 0x0807c0d8: a log whose every readable page sits at its own
 * index can stand as the data block */
static bool ftl_log_in_place(uint32_t sb)
{
    uint32_t v;

    int type;

    for (v = 0; v < sbpages; v++)
    {
        type = ftl_read_vpage(sb, v);
        if (type >= 0 && type != SPARE_ERASED && ftl_meta[0] % sbpages != v)
            return false;
    }
    return true;
}

/* Apple's _ScanLogSection: every page of the block, the last copy of each
 * index winning */
static void ftl_scan_log(struct ftl_log *log)
{
    uint32_t v, idx;
    int type;

    memset(log->offsets, 0xff, sbpages * sizeof(log->offsets[0]));
    log->pagescurrent = 0;
    for (v = 0; v < sbpages; v++)
    {
        type = ftl_read_vpage(log->sb, v);
        if (type < 0 || type == SPARE_ERASED)
            continue;
        idx = ftl_meta[0] % sbpages;
        if (log->offsets[idx] == FTL_NO_PAGE)
            log->pagescurrent++;
        log->offsets[idx] = v;
    }
}

static int ftl_restore(void)
{
    static struct ftl_log slots[FTL_MAX_LOGS + 1], cand;
    uint32_t sb, i, lblock, maxusn = 0, nslots = FTL_MAX_LOGS + 1, n;
    uint32_t page0usn, f;
    int type;

    logf("ftl: restoring from the medium");
    memset(ftl_rs, 0, sizeof(ftl_rs));
    memset(ftl_hole, 0, sizeof(ftl_hole));
    ftl_nholes = 0;

    /* A: the control blocks; the newest by page 0 holds the next commit's
     * predecessor, and the next commit starts a fresh block */
    page0usn = 0xffffffff;
    for (i = 0; i < 3; i++)
    {
        if (ftl_ctrl[i] >= nsuperblocks)
            return -30;
        ftl_rs[ftl_ctrl[i]].state = RS_CTRL;
        if (ftl_read_vpage(ftl_ctrl[i], 0) >= 0 && ftl_meta[0] < page0usn)
        {
            page0usn = ftl_meta[0];
            ftl_ctrl_idx = i;
        }
    }
    ftl_cxt.usn = page0usn;
    ftl_ctrl_v = sbpages;

    /* B: classify every other superblock by its last page, or by page 0
     * when that is erased or unreadable; closed blocks go into the map,
     * the higher usn winning */
    memset(ftl_map, 0xff, sizeof(ftl_map));
    for (sb = 0; sb < nsuperblocks; sb++)
    {
        struct ftl_rs *r = &ftl_rs[sb];

        if (r->state == RS_CTRL)
            continue;
        r->state = RS_FREE;
        type = ftl_read_vpage(sb, sbpages - 1);
        if (type < 0 || type == SPARE_ERASED)
        {
            type = ftl_read_vpage(sb, 0);
            if (type != SPARE_DATA)
                continue;
            r->state = RS_LOG;
        }
        else if (type == SPARE_DATA_LAST)
            r->state = RS_DATA;
        else if (type == SPARE_DATA)
            r->state = RS_LOG;
        else
            continue;
        lblock = ftl_meta[0] / sbpages;
        if (lblock >= usersb)
        {
            r->state = RS_FREE;
            continue;
        }
        r->lblock = lblock;
        r->usn = ftl_meta[1];
        if (r->usn > maxusn)
            maxusn = r->usn;
        if (r->state != RS_DATA)
            continue;
        if (ftl_map[lblock] != 0xffff)
        {
            if (ftl_rs[ftl_map[lblock]].usn >= r->usn)
            {
                r->state = RS_FREE;
                continue;
            }
            ftl_rs[ftl_map[lblock]].state = RS_FREE;
        }
        ftl_map[lblock] = sb;
    }
    ftl_cxt.maxusn = maxusn;

    /* C: place the logs, one per logical block */
    for (i = 0; i < nslots; i++)
        slots[i].lblock = 0xffff;
    for (sb = 0; sb < nsuperblocks; sb++)
    {
        struct ftl_rs *r = &ftl_rs[sb];
        uint32_t datausn = 0;

        if (r->state != RS_LOG)
            continue;
        lblock = r->lblock;
        if (ftl_map[lblock] == 0xffff)
        {
            if (ftl_log_in_place(sb))
            {
                ftl_map[lblock] = sb;
                r->state = RS_DATA;
                continue;
            }
        }
        else
            datausn = ftl_rs[ftl_map[lblock]].usn;
        if (r->usn < datausn)
        {
            r->state = RS_FREE;
            continue;
        }
        for (i = 0; i < nslots; i++)
        {
            struct ftl_log *s = &slots[i];

            if (s->lblock == 0xffff)
            {
                s->lblock = lblock;
                s->sb = sb;
                s->usn = r->usn;
                break;
            }
            if (s->lblock != lblock)
                continue;
            if (r->usn != maxusn && s->usn != maxusn)
            {
                /* Neither was being written last: the newer one stays */
                if (s->usn >= r->usn)
                    r->state = RS_FREE;
                else
                {
                    ftl_rs[s->sb].state = RS_FREE;
                    s->sb = sb;
                    s->usn = r->usn;
                }
            }
            else
            {
                /* One was being written last. It stays only if it holds
                 * every page the other does; otherwise it is a partial
                 * copy and the other still has the data. */
                const struct ftl_log *newer, *older;
                uint32_t idx, loserusn;

                cand.sb = sb;
                cand.usn = r->usn;
                ftl_scan_log(&cand);
                ftl_scan_log(s);
                newer = r->usn == maxusn ? &cand : s;
                older = newer == &cand ? s : &cand;
                for (idx = 0; idx < sbpages; idx++)
                    if (older->offsets[idx] != FTL_NO_PAGE
                        && newer->offsets[idx] == FTL_NO_PAGE)
                        break;
                loserusn = idx == sbpages ? older->usn : newer->usn;
                if (loserusn == r->usn)
                    r->state = RS_FREE;
                else
                {
                    ftl_rs[s->sb].state = RS_FREE;
                    s->sb = sb;
                    s->usn = r->usn;
                }
            }
            break;
        }
        if (i == nslots)
        {
            /* More distinct log lblocks than Whimory allows. Keep the
             * highest-usn occupants; treat this one as free. The previous
             * nano2g-style FTL left many 512-page hyperblocks that look
             * like logs once they are read as 1024-page superblocks. */
            uint32_t worst = 0;

            for (i = 1; i < nslots; i++)
                if (slots[i].usn < slots[worst].usn)
                    worst = i;
            if (r->usn <= slots[worst].usn)
            {
                r->state = RS_FREE;
                continue;
            }
            ftl_rs[slots[worst].sb].state = RS_FREE;
            slots[worst].lblock = lblock;
            slots[worst].sb = sb;
            slots[worst].usn = r->usn;
        }
    }
    /* An eighteenth log: drop the one being written last - which may be the
     * eighteenth itself - or fail */
    if (slots[FTL_MAX_LOGS].lblock != 0xffff)
    {
        for (i = 0; i <= FTL_MAX_LOGS; i++)
            if (slots[i].usn == maxusn)
                break;
        if (i > FTL_MAX_LOGS)
        {
            /* Newest page is not one of these logs (closed data, or extra
             * logs from a previous FTL). Drop the oldest. */
            i = 0;
            for (n = 1; n <= FTL_MAX_LOGS; n++)
                if (slots[n].usn < slots[i].usn)
                    i = n;
        }
        ftl_rs[slots[i].sb].state = RS_FREE;
        if (i < FTL_MAX_LOGS)
            slots[i] = slots[FTL_MAX_LOGS];
        slots[FTL_MAX_LOGS].lblock = 0xffff;
    }

    /* D: a logical block with no data block gets the next free one */
    f = 0;
    for (lblock = 0; lblock < usersb; lblock++)
    {
        if (ftl_map[lblock] != 0xffff)
            continue;
        while (f < nsuperblocks && ftl_rs[f].state != RS_FREE)
            f++;
        if (f == nsuperblocks)
            return -33;
        ftl_rs[f].state = RS_DATA;
        ftl_rs[f].lblock = lblock;
        ftl_map[lblock] = f;
        ftl_hole[lblock / 8] |= 1 << (lblock % 8);
        ftl_nholes++;
    }

    /* E: the free pool is everything left, after one slot per log */
    ftl_nlogs = 0;
    for (i = 0; i < FTL_MAX_LOGS; i++)
        if (slots[i].lblock != 0xffff)
            ftl_logs[ftl_nlogs++] = slots[i];
    n = 0;
    for (i = 0; i < ftl_nlogs; i++)
        ftl_cxt.blockpool[i] = 0xffff;
    for (sb = 0; sb < nsuperblocks && ftl_nlogs + n < FTL_POOL_SIZE; sb++)
        if (ftl_rs[sb].state == RS_FREE)
            ftl_cxt.blockpool[ftl_nlogs + n++] = sb;
    ftl_cxt.freecount = n;
    ftl_cxt.nextfreeidx = ftl_nlogs % FTL_POOL_SIZE;
    if (ftl_nlogs + n != FTL_POOL_SIZE)
    {
        logf("ftl: restore found %u logs and %lu free blocks", ftl_nlogs,
             (unsigned long)n);
        return -34;
    }

    /* G: the newest erase counter, read counter and stats pages, walking the
     * control blocks newest first and each one backwards */
    {
        static uint32_t seen[2 * FTL_CXT_PAGELIST + 1];
        uint32_t k, blk, order[3], nec;

        memset(seen, 0, sizeof(seen));
        nec = (nsuperblocks * 2 + NAND_PAGE_SIZE - 1) / NAND_PAGE_SIZE;
        for (k = 0; k < 3; k++)
            order[k] = (ftl_ctrl_idx + 3 - k) % 3;
        for (k = 0; k < 3; k++)
        {
            blk = ftl_ctrl[order[k]];
            for (i = sbpages; i-- > 0; )
            {
                uint32_t idx, slot;

                type = ftl_read_vpage(blk, i);
                idx = ftl_meta[1] & 0xffff;
                if (type == SPARE_FTL_ERASECTR && idx < nec)
                    slot = idx;
                else if (type == SPARE_FTL_READCOUNT && idx < nec)
                    slot = FTL_CXT_PAGELIST + idx;
                else if (type == SPARE_FTL_STATS)
                    slot = 2 * FTL_CXT_PAGELIST;
                else
                    continue;
                if (seen[slot])
                    continue;
                seen[slot] = 1;
                if (type == SPARE_FTL_ERASECTR)
                    ftl_cxt.ecpages[idx] = ftl_vpn(blk, i);
                else if (type == SPARE_FTL_READCOUNT)
                    ftl_cxt.rcpages[idx] = ftl_vpn(blk, i);
                else
                {
                    ftl_cxt.statspage = ftl_vpn(blk, i);
                    ftl_cxt.statsflag = 0xffffffff;
                }
            }
        }
    }

    /* I: what each log holds */
    for (i = 0; i < ftl_nlogs; i++)
    {
        ftl_scan_log(&ftl_logs[i]);
        ftl_logs[i].pagesused = sbpages;
        ftl_logs[i].issequential = 0;
    }
    ftl_cxt.cleanflag = 0;
    return 0;
}

/* Which superblock and page hold lpn: the newest log that has it, else the
 * block map */
static int ftl_resolve(uint32_t lpn, uint32_t *sb, uint32_t *v)
{
    uint32_t lblock = lpn / sbpages, idx = lpn % sbpages, i;
    uint32_t bestusn = 0;
    bool found = false;

    if (lblock >= usersb)
        return -1;
    *sb = ftl_map[lblock];
    *v = idx;
    for (i = 0; i < ftl_nlogs; i++)
    {
        const struct ftl_log *log = &ftl_logs[i];
        if (log->lblock == lblock && log->offsets[idx] != FTL_NO_PAGE
            && (!found || log->usn > bestusn))
        {
            *sb = log->sb;
            *v = log->offsets[idx];
            bestusn = log->usn;
            found = true;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Write path                                                         */
/*                                                                    */
/* A write goes to a log superblock for its logical block. Small      */
/* writes append pages to the log; a write covering a whole logical   */
/* block goes straight into a fresh superblock. A log is folded back  */
/* into the block map when it fills, when its slot is needed, and at  */
/* ftl_sync(); a commit records any log still open.                   */
/*                                                                    */
/* Commits are in exactly Apple's format, so its own firmware still   */
/* mounts the result.                                                 */
/* ------------------------------------------------------------------ */

/* Read one of the context's pointer lists: npages pages of type, holding
 * entries 16-bit table entries between them */
static int ftl_read_side(const uint32_t *list, uint32_t npages, uint32_t type,
                         uint16_t *table, uint32_t entries)
{
    uint32_t per = NAND_PAGE_SIZE / 2;
    uint32_t i, vpn, n;

    for (i = 0; i < npages; i++)
    {
        vpn = list[i];
        if (vpn / sbpages >= nsuperblocks
            || ftl_read_vpage(vpn / sbpages, vpn % sbpages) != (int)type
            || (ftl_meta[1] & 0xffff) != i)
            return -1;
        n = MIN(entries - i * per, per);
        memcpy(table + i * per, ftl_buffer, n * 2);
    }
    return 0;
}

/* Pages one commit occupies, from the current geometry */
static void ftl_set_page_counts(void)
{
    ftl_nmappages = (usersb * 2 + NAND_PAGE_SIZE - 1) / NAND_PAGE_SIZE;
    ftl_necpages = (nsuperblocks * 2 + NAND_PAGE_SIZE - 1) / NAND_PAGE_SIZE;
    ftl_nrcpages = ftl_necpages;
    ftl_nlogpages = (sbpages * FTL_MAX_LOGS * 2 + NAND_PAGE_SIZE - 1)
                    / NAND_PAGE_SIZE;
}

static uint32_t ftl_sb_used[FTL_SIDE_ENTRIES / 32 + 1];

/* Mark a superblock in use. False if it is out of range or already is. */
static bool ftl_claim_sb(uint32_t sb)
{
    uint32_t bit = 1u << (sb % 32);

    if (sb >= nsuperblocks || (ftl_sb_used[sb / 32] & bit))
        return false;
    ftl_sb_used[sb / 32] |= bit;
    return true;
}

/* Every superblock holds one thing: a logical block, a log, the FTL
 * control or a free pool slot. A free slot that is also in use would be
 * erased under live data on its first allocation. */
static bool ftl_pool_consistent(void)
{
    uint32_t i;

    memset(ftl_sb_used, 0, sizeof(ftl_sb_used));
    for (i = 0; i < usersb; i++)
        if (!ftl_claim_sb(ftl_map[i]))
        {
            logf("ftl: lblock %lu maps to sb %u", (unsigned long)i,
                 ftl_map[i]);
            return false;
        }
    for (i = 0; i < 3; i++)
        if (!ftl_claim_sb(ftl_ctrl[i]))
        {
            logf("ftl: control block %u is sb %u", (unsigned)i, ftl_ctrl[i]);
            return false;
        }
    for (i = 0; i < ftl_nlogs; i++)
        if (!ftl_claim_sb(ftl_logs[i].sb))
        {
            logf("ftl: log %u is sb %u", (unsigned)i, ftl_logs[i].sb);
            return false;
        }
    for (i = 0; i < ftl_cxt.freecount; i++)
    {
        uint32_t sb =
            ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE];

        if (!ftl_claim_sb(sb))
        {
            logf("ftl: free slot %u is sb %lu", (unsigned)i,
                 (unsigned long)sb);
            return false;
        }
    }
    return true;
}

/* Load the rest of what a commit has to reproduce: the erase and read
 * counters and the stats page, and check that the context and the medium
 * agree. Returns 0 when writes may be enabled, or which check failed; a
 * failure is not fatal - the FTL stays mounted read-only. */
static int ftl_open_write_state(void)
{
    uint32_t vpn;

    ftl_writable = false;
    ftl_set_page_counts();

    if (nsuperblocks - usersb != 3 + FTL_POOL_SIZE)
    {
        logf("ftl: %lu spare superblocks, expected %d",
             (unsigned long)(nsuperblocks - usersb - 3), FTL_POOL_SIZE);
        return 1;
    }
    if (ftl_necpages > FTL_SIDE_PAGES || ftl_nlogpages > FTL_LOG_PAGES
        || nsuperblocks > FTL_SIDE_ENTRIES)
        return 2;

    /* Lost counter tables read as zero, as _FTLRestore leaves them */
    if (ftl_read_side(ftl_cxt.ecpages, ftl_necpages, SPARE_FTL_ERASECTR,
                      ftl_erasectr, nsuperblocks))
    {
        logf("ftl: erase counters lost");
        memset(ftl_erasectr, 0, sizeof(ftl_erasectr));
    }
    if (ftl_read_side(ftl_cxt.rcpages, ftl_nrcpages, SPARE_FTL_READCOUNT,
                      ftl_readcount, nsuperblocks))
    {
        logf("ftl: read counters lost");
        memset(ftl_readcount, 0, sizeof(ftl_readcount));
    }

    /* The stats page is optional; an all-0xff one is harmless */
    memset(ftl_stats, 0xff, sizeof(ftl_stats));
    vpn = ftl_cxt.statspage;
    if (ftl_cxt.statsflag == 0xffffffff && vpn / sbpages < nsuperblocks
        && ftl_read_vpage(vpn / sbpages, vpn % sbpages) == SPARE_FTL_STATS)
        memcpy(ftl_stats, ftl_buffer, NAND_PAGE_SIZE);

    /* A spare superblock is free or a log; more than 20 of them means the
     * context and the medium disagree, and we stay read-only */
    if (ftl_cxt.freecount + ftl_nlogs > FTL_POOL_SIZE)
    {
        logf("ftl: %lu free + %u logs > %d", (unsigned long)ftl_cxt.freecount,
             ftl_nlogs, FTL_POOL_SIZE);
        return 3;
    }
    if (!ftl_pool_consistent())
        return 4;
    ftl_writable = true;
    return 0;
}

/* ---- VFL write side: blocks that have gone bad --------------------- */
/* What Apple's VSVFL does when a block goes bad (_ReplaceBadBlock,
 * the scrub list, VSVFL_Erase, VSVFL_MarkWriteFailBlock, _StoreVFLCxt):
 *  - a block that fails a program may still hold pages the FTL needs, so it
 *    only goes on its chip enable's scrub list; the FTL writes elsewhere
 *    and the block is replaced the next time its virtual block is erased.
 *  - a block that fails an erase is replaced at once and the erase retried
 *    on the replacement, three times in all.
 *  - every change is stored as a new VFL context of that chip enable.
 * A replacement takes the next free slot of the pool from the block's own
 * way, then from the other ways, least used first. Only a failure the chip
 * reports (NAND_OP_FAILED) is treated as a bad block; anything else from
 * the driver, an empty pool or a context that will not store ends writing
 * for this session (ftl_write_failed()). */

static void ftl_vfl_update_checksum(struct ftl_vfl_cxt *cxt)
{
    uint32_t sum, x;

    ftl_vfl_calculate_checksum(cxt, &sum, &x);
    cxt->checksum1 = sum;
    cxt->checksum2 = x;
}

/* Program the context as the next 8-page group of its active ring block
 * and read the copies back, as _WriteVFLCxtToFlash does: the update counts
 * move first, so a group that fails is never reused, and at least five
 * copies must read back intact. */
static int ftl_vfl_write_cxt(uint32_t bank)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    struct nand_write w[8];
    uint32_t meta[NAND_META_WORDS], page, i, bad = 0;
    int rc;

    if (cxt->vflcxtblocks[cxt->usnblock] >= geo->blocks)
        return -1;
    cxt->updatecount--;
    cxt->usn = ++ftl_vfl_usn;
    page = cxt->vflcxtblocks[cxt->usnblock] * geo->pagesperblock
         + cxt->usnpage;
    cxt->usnpage += 8;
    ftl_vfl_update_checksum(cxt);

    memset(ftl_vfl_page, 0xff, sizeof(ftl_vfl_page));
    memcpy(ftl_vfl_page, cxt, sizeof(*cxt));
    meta[0] = cxt->updatecount;
    meta[1] = 0xffffffff;
    meta[2] = 0xffff0000 | (SPARE_VFL_CXT << 8);
    for (i = 0; i < 8; i++)
    {
        w[i].bank = bank;
        w[i].page = page + i;
        w[i].buf = ftl_vfl_page;
        w[i].meta = meta;
    }
    if (nand_write_pages(w, 8, false, NULL))
        return -1;

    for (i = 0; i < 8; i++)
    {
        rc = nand_read_page(bank, page + i, ftl_buffer, ftl_meta);
        if (rc < 0 || rc == NAND_ECC_FAILED
            || memcmp(ftl_buffer, cxt, sizeof(*cxt))
            || ftl_meta[0] != cxt->updatecount
            || (ftl_meta[2] & 0xffff) != (SPARE_VFL_CXT << 8))
            bad++;
    }
    return bad < 4 ? 0 : -1;
}

/* Store a bank's context, as _StoreVFLCxt does: in the active ring block
 * while it has a group left, else (or if that fails) in the next ring
 * block that erases, from its first page. */
static int ftl_vfl_store_cxt(uint32_t bank)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t i, t, start = cxt->usnblock;

    if (start >= 4)
        return -1;
    if (cxt->usnpage + 8u <= geo->pagesperblock && !ftl_vfl_write_cxt(bank))
        return 0;
    for (i = (start + 1) & 3; i != start; i = (i + 1) & 3)
    {
        if (cxt->vflcxtblocks[i] >= geo->blocks)
            continue;
        for (t = 0; t < 4; t++)
            if (!nand_erase_block(bank, cxt->vflcxtblocks[i]))
                break;
        if (t == 4)
            continue;
        cxt->usnblock = i;
        cxt->usnpage = 0;
        if (!ftl_vfl_write_cxt(bank))
            return 0;
    }
    logf("ftl: VFL context of bank %lu not stored", (unsigned long)bank);
    return -1;
}

/* Take a free pool slot of a bank to stand in for block, which belongs to
 * way way, as _ReplaceBadBlock does. A slot that already stood in for it
 * has gone bad itself and is retired. Does not store the context. */
static bool ftl_vfl_replace(uint32_t bank, uint32_t way, uint32_t block)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t per = geo->blocks / ftl_ways - ftl_vfl_cxt[0].poolstart;
    uint32_t order[4], i, k, w, t;

    for (i = 0; i < pool_slots; i++)
        if (cxt->poolmap[i] == block)
            cxt->poolmap[i] = FTL_POOL_BAD;

    /* Its own way first, then every way by how little of it is used */
    order[0] = way;
    for (k = 0; k <= ftl_ways; k++)
    {
        if (k == 1)
        {
            for (i = 0; i < ftl_ways; i++)
                order[i] = i;
            for (i = 0; i + 1 < ftl_ways; i++)
                for (t = i + 1; t < ftl_ways; t++)
                    if (cxt->cursor[order[t]] < cxt->cursor[order[i]])
                    {
                        w = order[t];
                        order[t] = order[i];
                        order[i] = w;
                    }
        }
        w = order[k ? k - 1 : 0];
        for (; cxt->cursor[w] < per; cxt->cursor[w]++)
            if (cxt->poolmap[w * per + cxt->cursor[w]] == FTL_POOL_FREE)
            {
                cxt->poolmap[w * per + cxt->cursor[w]++] = block;
                logf("ftl: bank %lu block %lu replaced by pool slot %lu",
                     (unsigned long)bank, (unsigned long)block,
                     (unsigned long)(w * per + cxt->cursor[w] - 1));
                return true;
            }
    }
    logf("ftl: bank %lu has no free pool slot", (unsigned long)bank);
    return false;
}

static int ftl_vfl_scrub_index(uint32_t bank, uint32_t block)
{
    const struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    int i;

    for (i = 0; i < cxt->scrubcount && i < 20; i++)
        if (cxt->scrub[i] == block)
            return i;
    return -1;
}

/* A program on bank failed in superblock sb: put that bank's blocks of it
 * on the scrub list, as VSVFL_MarkWriteFailBlock and _AddBlockToScrubList
 * do, storing the context for each block added. */
static int ftl_vfl_mark_write_fail(uint32_t bank, uint32_t sb)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t vb, block;

    for (vb = bank; vb < ftl_vbanks; vb += geo->banks)
    {
        block = ftl_vfl_c2p(vb, sb);
        if (ftl_vfl_scrub_index(bank, block) >= 0)
            continue;
        if (cxt->scrubcount >= 20)
            return -1;
        cxt->scrub[cxt->scrubcount++] = block;
        if (ftl_vfl_store_cxt(bank))
            return -1;
        logf("ftl: bank %lu block %lu failed a program, scrubbing",
             (unsigned long)bank, (unsigned long)block);
    }
    return 0;
}

/* A block that has failed is no use for writing any more: stop writing for
 * this session. Nothing is committed, and the next mount rebuilds from the
 * medium as after a power cut. The UI is told with
 * SYS_STORAGE_WRITE_FAILED. */
static void ftl_write_failed(const char *what, uint32_t bank, uint32_t block)
{
    (void)what;
    (void)bank;
    (void)block;
    if (!ftl_writable)
        return;
    logf("ftl: %s failed on bank %lu block %lu, now read-only", what,
         (unsigned long)bank, (unsigned long)block);
    ftl_writable = false;
    nand_set_writable(false);
#ifndef BOOTLOADER
    button_queue_post(SYS_STORAGE_WRITE_FAILED, 0);
#endif
}

/* Erase one virtual block, as VSVFL_Erase does it a bank at a time: a
 * block on the scrub list is replaced first, and a block that fails is
 * replaced and the erase tried again, three times in all. */
static int ftl_vfl_erase(uint32_t vb, uint32_t vblock)
{
    uint32_t t, bank = vb % geo->banks, way = vb / geo->banks;
    uint32_t block = ftl_vfl_c2p(vb, vblock);
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    int i = ftl_vfl_scrub_index(bank, block), rc = 0;

    if (i >= 0)
    {
        if (!ftl_vfl_replace(bank, way, block))
            goto failed;
        /* _RemoveBlockFromScrubList, which leaves the old last entry */
        cxt->scrub[i] = 0;
        if (cxt->scrubcount != 1)
            cxt->scrub[i] = cxt->scrub[cxt->scrubcount - 1];
        cxt->scrubcount--;
        if (ftl_vfl_store_cxt(bank))
            goto failed;
    }
    for (t = 0; t < 3; t++)
    {
        rc = nand_erase_block(bank, ftl_vfl_phys_block(vb, vblock));
        if (!rc)
            return 0;
        if (rc != NAND_OP_FAILED)
            break;
        cxt->erasefail++;
        if (!ftl_vfl_replace(bank, way, block) || ftl_vfl_store_cxt(bank))
            break;
    }
failed:
    ftl_write_failed("erase", bank, block);
    return -1;
}

/* Erase every physical block of a superblock and count it */
static int ftl_erase_sb(uint32_t sb)
{
    uint32_t bank, plane;
    int rc;

    if (sb >= nsuperblocks)
        return -1;
    for (bank = 0; bank < ftl_vbanks; bank++)
        for (plane = 0; plane < geo->planes; plane++)
        {
            rc = ftl_vfl_erase(bank, sb * geo->planes + plane);
            if (rc)
            {
                logf("ftl: erase of sb %lu bank %lu failed (%d)",
                     (unsigned long)sb, (unsigned long)bank, rc);
                return -1;
            }
        }
    if (ftl_erasectr[sb] != 0xffff)
        ftl_erasectr[sb]++;
    return 0;
}

/* The usn a user data page is written with, as Apple's FTL_Write numbers
 * them: one counter, advanced whenever a write starts a new superblock, so
 * every page written until the next such start shares it */
static uint32_t ftl_data_usn(uint32_t v)
{
    if (v == 0)
        ftl_cxt.maxusn++;
    return ftl_cxt.maxusn;
}

/* ---- page runs -------------------------------------------------------- */
/* Pages are programmed in runs, as Apple's FTL does it: the callers gather
 * consecutive pages of one superblock with ftl_run_add(), and ftl_run_write()
 * hands them to the controller the way VFL_WriteMultiplePagesInVb does, so
 * that the chips program in parallel and whole rows two planes at a time. */

/* Queue page v of superblock sb. Pages must follow on from the ones already
 * queued, in the same superblock. buf must stay put until the run is
 * written. */
static void ftl_run_add(uint32_t sb, uint32_t v, const void *buf,
                        uint32_t word0, uint32_t word1, uint32_t type)
{
    uint32_t *meta = ftl_run_meta[ftl_run_n];

    if (!ftl_run_n)
    {
        ftl_run_sb = sb;
        ftl_run_v = v;
    }
    meta[0] = word0;
    meta[1] = word1;
    meta[2] = 0xffff00ff | (type << 8);
    ftl_run[ftl_run_n].buf = buf;
    ftl_run[ftl_run_n].meta = meta;
    ftl_run_n++;
}

/* Program n queued pages from the first'th in one driver call. With rows,
 * n is a whole number of rows; they go two-plane, in the bank-major order
 * m1fmssWriteSequentialPages uses, when the chip allows it and no block of
 * the superblock is remapped - a spare is not the other plane's twin. On
 * failure, *failbank and *failplanes say which blocks were being
 * programmed. */
static int ftl_run_program(uint32_t first, uint32_t n, bool rows,
                           uint32_t *failbank, uint32_t *failplanes)
{
    static struct nand_write w[FTL_MAX_SBPAGES];
    static uint16_t vs[FTL_MAX_SBPAGES];
    uint32_t perrow = ftl_vbanks * geo->planes;
    uint32_t i, idx, bank, plane;
    bool twoplane = rows && geo->twoplane && geo->planes == 2;
    int rc;

    for (bank = 0; twoplane && bank < ftl_vbanks; bank++)
        for (plane = 0; plane < geo->planes; plane++)
            if (ftl_vfl_phys_block(bank, ftl_run_sb * geo->planes + plane)
                != ftl_run_sb * geo->planes + plane)
                twoplane = false;

    for (i = 0; i < n; i++)
    {
        idx = first + i;
        if (twoplane)
        {
            /* Entry k of a row is bank k / planes, plane k % planes */
            uint32_t k = i % perrow;

            idx = first + i - k + (k % geo->planes) * ftl_vbanks
                + k / geo->planes;
        }
        w[i] = ftl_run[idx];
        vs[i] = ftl_run_v + idx;
        ftl_vpage_phys(ftl_run_sb, vs[i], &w[i].bank, &w[i].page);
    }

    *failbank = w[0].bank;
    rc = nand_write_pages(w, n, twoplane, failbank);
    if (rc)
    {
        *failplanes = 0;
        for (i = 0; i < n; i++)
            if (w[i].bank == *failbank)
                *failplanes |= 1 << ((vs[i] / ftl_vbanks) % geo->planes);
    }
    return rc;
}

/* Program the queued run: pages up to the next row boundary on their own,
 * whole rows together while more than one row remains, and the rest on
 * their own, exactly as VFL_WriteMultiplePagesInVb splits it. If the chip
 * refuses a page, its bank's blocks of the superblock go on the scrub list
 * and -1 is returned with the FTL still writable: the caller writes the
 * pages somewhere else, as Apple's FTL does, and the superblock's next
 * erase replaces the block. */
static int ftl_run_write(void)
{
    uint32_t perrow = ftl_vbanks * geo->planes;
    uint32_t n = ftl_run_n, done = 0, len, bank = 0, planes = 0, plane;
    int rc = 0;

    ftl_run_n = 0;
    if (!n)
        return 0;
    len = MIN(perrow - ftl_run_v % perrow, n);
    if (len != perrow)
    {
        rc = ftl_run_program(0, len, false, &bank, &planes);
        done = len;
    }
    if (!rc && n - done > perrow)
    {
        len = (n - done) / perrow * perrow;
        rc = ftl_run_program(done, len, true, &bank, &planes);
        done += len;
    }
    if (!rc && n > done)
        rc = ftl_run_program(done, n - done, false, &bank, &planes);
    if (rc)
    {
        if (rc == NAND_OP_FAILED && !ftl_vfl_mark_write_fail(bank, ftl_run_sb))
            return -1;
        for (plane = 0; plane + 1 < geo->planes; plane++)
            if (planes & (1 << plane))
                break;
        ftl_write_failed("program", bank,
                         ftl_vfl_phys_block(bank,
                                            ftl_run_sb * geo->planes + plane));
        return -1;
    }
    return 0;
}

/* Program one page on its own */
static int ftl_write_vpage(uint32_t sb, uint32_t v, const void *buf,
                           uint32_t word0, uint32_t word1, uint32_t type)
{
    ftl_run_add(sb, v, buf, word0, word1, type);
    return ftl_run_write();
}

/* Take a block off the free ring, least-worn first. Apple's _GetFreeVb picks
 * the entry with the lowest erase count and swaps it to the head of the ring
 * before returning it; doing the same spreads wear over the pool instead of
 * cycling through it blindly, which matters because a block reclaimed from
 * cold data can join the pool thousands of cycles behind the rest. */
static int ftl_pool_alloc(uint32_t *sb, bool worn_first)
{
    uint32_t i, idx, best = 0;
    uint32_t bestec = worn_first ? 0 : 0xffffffff;
    bool found = false;
    uint16_t t;

    if (!ftl_cxt.freecount)
        return -1;
    for (i = 0; i < ftl_cxt.freecount; i++)
    {
        uint32_t ec;

        idx = (ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE;
        if (ftl_cxt.blockpool[idx] >= nsuperblocks)
            continue;
        ec = ftl_erasectr[ftl_cxt.blockpool[idx]];
        if (!found || (worn_first ? ec > bestec : ec < bestec))
        {
            bestec = ec;
            best = idx;
            found = true;
        }
    }
    if (!found)
        return -1;

    idx = ftl_cxt.nextfreeidx;
    t = ftl_cxt.blockpool[idx];
    ftl_cxt.blockpool[idx] = ftl_cxt.blockpool[best];
    ftl_cxt.blockpool[best] = t;
    *sb = ftl_cxt.blockpool[idx];

    /* Pool blocks are erased when released, and the whole pool again by a
     * restore, as Apple's _SetFreeVb and _FTLRestore do */
    ftl_cxt.nextfreeidx = (ftl_cxt.nextfreeidx + 1) % FTL_POOL_SIZE;
    ftl_cxt.freecount--;
    return 0;
}

static int ftl_pool_release(uint32_t sb)
{
    if (ftl_cxt.freecount >= FTL_POOL_SIZE)
    {
        logf("ftl: releasing sb %lu with a full pool", (unsigned long)sb);
        return -1;
    }
    if (ftl_erase_sb(sb))
        return -1;
    ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + ftl_cxt.freecount)
                      % FTL_POOL_SIZE] = sb;
    ftl_cxt.freecount++;
    return 0;
}

static struct ftl_log *ftl_get_log_entry(uint32_t lblock);

/* The current contents of lpn, for copying into a merged block. A sector
 * that was never written reads as 0xff. */
static int ftl_read_current(uint32_t lpn, uint8_t *dst)
{
    uint32_t sb, v, bank, page, meta[NAND_META_WORDS];
    int rc;

    if (ftl_resolve(lpn, &sb, &v))
        return -1;
    if (ftl_nholes && sb == ftl_map[lpn / sbpages]
        && ftl_is_hole(lpn / sbpages))
    {
        memset(dst, 0xff, NAND_PAGE_SIZE);
        return 0;
    }
    ftl_vpage_phys(sb, v, &bank, &page);
    rc = nand_read_page(bank, page, dst, meta);
    if (rc < 0 || rc == NAND_ECC_FAILED)
        return -1;
    if (meta_type(meta) == SPARE_ERASED)
    {
        memset(dst, 0xff, NAND_PAGE_SIZE);
        return 0;
    }
    if (!meta_is_data(meta) || meta[0] != lpn)
    {
        memset(dst, 0xff, NAND_PAGE_SIZE);
        return 0;
    }
    return 0;
}

/* Read one prepared part of a run. Apple retries a failed multi-page read
 * page by page; keep the BootROM-derived single-page path for that retry. */
struct ftl_read_dbg ftl_read_dbg;

static int ftl_read_prepared(uint32_t first, uint32_t n,
                             const uint32_t *lpns)
{
    uint32_t i;
    int rc = nand_read_pages(ftl_read_run + first, n);
    bool failed = rc != 0;

    if (failed)
    {
        ftl_read_dbg.multi_fail++;
        ftl_read_dbg.multi_rc = rc;
    }
    for (i = first; i < first + n; i++)
    {
        struct nand_read *r = &ftl_read_run[i];
        uint32_t *meta = ftl_read_meta[i];

        if (failed || r->ecc == NAND_ECC_FAILED)
        {
            if (!failed)
                ftl_read_dbg.ecc_fail++;
            r->ecc = nand_read_page(r->bank, r->page, r->buf, meta);
            if (r->ecc < 0 || r->ecc == NAND_ECC_FAILED)
            {
                ftl_read_dbg.page_fail++;
                ftl_read_dbg.page_rc = r->ecc;
                ftl_read_dbg.lpn = lpns[i];
                return -1;
            }
        }
        if (meta_type(meta) != SPARE_ERASED
            && (!meta_is_data(meta) || meta[0] != lpns[i]))
        {
            ftl_read_dbg.stale++;
            ftl_read_dbg.lpn = lpns[i];
            ftl_read_dbg.meta0 = meta[0];
        }
        /* Corrected data is safe to return: the controller has repaired it.
         * Apple's FTL also queues the block on its refresh list (context
         * 0x3b4) to be copied; that is maintenance, not needed to read the
         * data. A commit keeps Apple's list as it was, and Apple's
         * _HandleRefreshBlock drops entries whose block has moved since. */
        if (meta_type(meta) == SPARE_ERASED)
            memset(r->buf, 0xff, NAND_PAGE_SIZE);
        else if (!meta_is_data(meta) || meta[0] != lpns[i])
        {
            /* Leftover pages from a previous FTL (different lpn / type).
             * Treat as erased so a format does not turn every USB READ
             * of unwritten space into a medium error. */
            memset(r->buf, 0xff, NAND_PAGE_SIZE);
        }
    }
    return 0;
}

/* Read current logical pages into consecutive buffers. With a log Apple
 * uses one scattered VFL request. Without one, VFL_ReadMultiplePagesInVb
 * splits at a four-bank row: an unaligned head, whole rows only while more
 * than one row remains, then the final row/tail. */
static int ftl_read_pages(uint32_t lpn, uint32_t n, uint8_t *dst)
{
    static uint32_t lpns[FTL_MAX_SBPAGES];
    uint32_t i, nr = 0, sb, v, bank, page, head, middle;
    uint32_t lblock = lpn / sbpages;
    bool scattered = ftl_get_log_entry(lblock) != NULL;

    if (!n || n > FTL_MAX_SBPAGES || lblock != (lpn + n - 1) / sbpages)
        return -1;
    for (i = 0; i < n; i++)
    {
        if (ftl_resolve(lpn + i, &sb, &v))
            return -1;
        if (ftl_nholes && sb == ftl_map[lblock] && ftl_is_hole(lblock))
        {
            memset(dst + i * NAND_PAGE_SIZE, 0xff, NAND_PAGE_SIZE);
            continue;
        }
        ftl_vpage_phys(sb, v, &bank, &page);
        ftl_read_run[nr].bank = bank;
        ftl_read_run[nr].page = page;
        ftl_read_run[nr].buf = dst + i * NAND_PAGE_SIZE;
        ftl_read_run[nr].meta = ftl_read_meta[nr];
        lpns[nr++] = lpn + i;
    }
    if (!nr)
        return 0;
    if (scattered)
        return ftl_read_prepared(0, nr, lpns);

    v = lpn % sbpages;
    head = MIN(nr, ftl_vbanks - v % ftl_vbanks);
    if (head == ftl_vbanks)
        head = 0;
    if (head && ftl_read_prepared(0, head, lpns))
        return -1;
    i = head;
    middle = nr - i > ftl_vbanks
           ? ((nr - i - 1) / ftl_vbanks) * ftl_vbanks : 0;
    if (middle && ftl_read_prepared(i, middle, lpns))
        return -1;
    i += middle;
    return i == nr ? 0 : ftl_read_prepared(i, nr - i, lpns);
}

/* Drop the log blocks of a logical block, returning them to the pool */
static int ftl_release_logs(uint32_t lblock)
{
    unsigned int i = 0;
    int ret = 0;

    while (i < ftl_nlogs)
    {
        if (ftl_logs[i].lblock != lblock)
        {
            i++;
            continue;
        }
        if (ftl_pool_release(ftl_logs[i].sb))
            ret = -1;
        ftl_logs[i] = ftl_logs[--ftl_nlogs];
    }
    return ret;
}

/* Does src() have new contents for every page of this logical block? Then
 * the block can be written outright and no log is needed - the one case
 * where there is no write amplification at all. */
static bool ftl_block_complete(uint32_t lblock, ftl_write_src src, void *ctx)
{
    uint32_t v;

    for (v = 0; v < sbpages; v++)
        if (!src(lblock * sbpages + v, ctx))
            return false;
    return true;
}

/* Write every page of a logical block into sb, taking new contents from
 * src() where it has them and the block's current contents where it does
 * not. New contents for the whole block go in one run, as FTL_Write writes
 * them; a copy goes an eighth of the block at a time, read and then
 * written, as _DoSimpleMerge does. */
static int ftl_fill_block(uint32_t lblock, uint32_t sb, ftl_write_src src,
                          void *ctx)
{
    bool whole = src && ftl_block_complete(lblock, src, ctx);
    uint32_t v, lpn, n = 0;

    for (v = 0; v < sbpages; v++)
    {
        const uint8_t *new_data;

        lpn = lblock * sbpages + v;
        new_data = src ? src(lpn, ctx) : NULL;
        if (!new_data)
        {
            new_data = ftl_copybuf + n * NAND_PAGE_SIZE;
            if (!src && n == 0
                && ftl_read_pages(lpn, MIN(sbpages - v, sbpages / 8),
                                  ftl_copybuf))
            {
                ftl_run_n = 0;
                return -1;
            }
            if (src && ftl_read_current(lpn,
                                        ftl_copybuf + n * NAND_PAGE_SIZE))
            {
                ftl_run_n = 0;
                return -1;
            }
        }
        ftl_run_add(sb, v, new_data, lpn, ftl_data_usn(v),
                    v == sbpages - 1 ? SPARE_DATA_LAST : SPARE_DATA);
        if (!whole && ++n == sbpages / 8)
        {
            if (ftl_run_write())
                return -1;
            n = 0;
        }
    }
    return ftl_run_write();
}

static int ftl_merge(uint32_t lblock, ftl_write_src src, void *ctx,
                     bool worn_first)
{
    uint32_t sb, old;
    int attempt;

    if (lblock >= usersb)
        return -1;
    old = ftl_map[lblock];

    /* Up to four attempts, each in a different superblock, as Apple's
     * _DoSimpleMerge retries. Releasing a superblock that refused a program
     * erases it, which replaces the failed block. */
    for (attempt = 0; attempt < 4; attempt++)
    {
        if (ftl_pool_alloc(&sb, worn_first))
        {
            logf("ftl: free pool empty");
            return -1;
        }
        if (!ftl_fill_block(lblock, sb, src, ctx))
            break;
        if (!ftl_writable)
            return -1;
        logf("ftl: lblock %lu did not take, trying another superblock",
             (unsigned long)lblock);
        if (ftl_pool_release(sb))
            return -1;
    }
    if (attempt == 4)
    {
        logf("ftl: lblock %lu failed in four superblocks",
             (unsigned long)lblock);
        return -1;
    }

    /* Only now is the new block complete enough to point at */
    ftl_map[lblock] = sb;
    if (ftl_is_hole(lblock))
    {
        ftl_hole[lblock / 8] &= (uint8_t)~(1 << (lblock % 8));
        if (ftl_nholes)
            ftl_nholes--;
    }
    if (ftl_release_logs(lblock) || ftl_pool_release(old))
        return -1;
    return 0;
}

/* Does src() have anything for this logical block? */
static bool ftl_block_dirty(uint32_t lblock, ftl_write_src src, void *ctx)
{
    uint32_t v;

    for (v = 0; v < sbpages; v++)
        if (src(lblock * sbpages + v, ctx))
            return true;
    return false;
}

/* ---- log blocks --------------------------------------------------- */
/* Ported from ftl-nano2g.c, which implements the same Whimory scheme. Small
 * writes append pages to a log superblock instead of rewriting the whole
 * logical block; the log is folded back only when it fills, when its slot is
 * needed, or at a commit. */

static struct ftl_log *ftl_get_log_entry(uint32_t lblock)
{
    unsigned int i;

    for (i = 0; i < ftl_nlogs; i++)
        if (ftl_logs[i].lblock == lblock)
            return &ftl_logs[i];
    return NULL;
}

/* Forget a log entry without touching its superblock */
static void ftl_drop_log(struct ftl_log *log)
{
    *log = ftl_logs[--ftl_nlogs];
}

/* Fold a log into a freshly allocated data block, whatever is in it. Reading
 * goes through ftl_resolve(), so the log's pages win where it has them. */
static int ftl_commit_scattered(struct ftl_log *log)
{
    /* ftl_merge() releases every log of this logical block for us */
    return ftl_merge(log->lblock, NULL, NULL, false);
}

/* A log written strictly in order is already a valid data block apart from
 * the pages nobody has written yet: fill those in from the block it
 * replaces and adopt it, saving a whole block copy and one erase. */
static int ftl_commit_sequential(struct ftl_log *log)
{
    uint32_t lblock = log->lblock, sb = log->sb, old, v, n, i;

    if (!log->issequential || log->pagescurrent != log->pagesused)
        return ftl_commit_scattered(log);
    for (v = log->pagesused; v < sbpages; v++)
        if (log->offsets[v] != FTL_NO_PAGE)
            return ftl_commit_scattered(log);

    /* An eighth of the block at a time, read and then written, as
     * _DoCopyMerge does */
    for (v = log->pagesused; v < sbpages; v += n)
    {
        n = MIN(sbpages - v, sbpages / 8);
        if (ftl_read_pages(lblock * sbpages + v, n, ftl_copybuf))
            return ftl_commit_scattered(log);
        for (i = 0; i < n; i++)
        {
            const uint8_t *buf = ftl_copybuf + i * NAND_PAGE_SIZE;

            ftl_run_add(sb, v + i, buf, lblock * sbpages + v + i,
                        ftl_data_usn(v + i),
                        v + i == sbpages - 1 ? SPARE_DATA_LAST : SPARE_DATA);
        }
        if (ftl_run_write())
            return ftl_commit_scattered(log);
        for (i = 0; i < n; i++)
            log->offsets[v + i] = v + i;
        log->pagesused += n;
        log->pagescurrent += n;
    }

    old = ftl_map[lblock];
    ftl_map[lblock] = sb;
    ftl_drop_log(log);
    return ftl_pool_release(old);
}

/* Copy the live pages of a log, in index order, into a fresh superblock and
 * release the old one, as Apple's _DoMoveMerge and ftl_compact_scattered() in
 * ftl-nano2g.c do. A log that has filled up mostly with superseded copies of
 * a few sectors then costs only its live pages, not a whole block. If power
 * is cut part way, the partial copy holds fewer pages than the log it was
 * copying, and the mount keeps the log. */
static int ftl_compact_scattered(struct ftl_log *log)
{
    static uint16_t offsets[FTL_MAX_SBPAGES];
    static uint32_t lpns[FTL_COPY_PAGES];
    uint32_t sb, idx, v = 0, n = 0, lpn, bank, page, i;

    if (!log->pagescurrent)
    {
        sb = log->sb;
        ftl_drop_log(log);
        return ftl_pool_release(sb);
    }
    if (ftl_pool_alloc(&sb, false))
        return -1;
    for (idx = 0; idx < sbpages; idx++)
    {
        offsets[idx] = FTL_NO_PAGE;
        if (log->offsets[idx] != FTL_NO_PAGE)
        {
            lpn = log->lblock * sbpages + idx;
            ftl_vpage_phys(log->sb, log->offsets[idx], &bank, &page);
            ftl_read_run[n].bank = bank;
            ftl_read_run[n].page = page;
            ftl_read_run[n].buf = ftl_copybuf + n * NAND_PAGE_SIZE;
            ftl_read_run[n].meta = ftl_read_meta[n];
            lpns[n] = lpn;
            offsets[idx] = v + n;
            n++;
        }
        /* Up to an eighth of a block of live pages at a time, as
         * _DoMoveMerge gathers them */
        if (n == sbpages / 8 || (n && idx == sbpages - 1))
        {
            if (ftl_read_prepared(0, n, lpns))
                break;
            for (i = 0; i < n; i++)
            {
                const uint8_t *buf = ftl_copybuf + i * NAND_PAGE_SIZE;
                ftl_run_add(sb, v, buf, lpns[i], ftl_data_usn(v),
                            SPARE_DATA);
                v++;
            }
            if (ftl_run_write())
                break;
            n = 0;
        }
    }
    if (idx < sbpages)
    {
        /* Fold it instead; the log itself is untouched */
        if (ftl_pool_release(sb))
            return -1;
        return ftl_commit_scattered(log);
    }
    lpn = log->sb;                      /* the old superblock, to release */
    memcpy(log->offsets, offsets, sizeof(log->offsets));
    log->sb = sb;
    log->pagesused = v;
    log->pagescurrent = v;
    log->issequential = 1;
    for (idx = 0; idx < v; idx++)
        if (log->offsets[idx] != idx)
            log->issequential = 0;
    return ftl_pool_release(lpn);
}

/* Free a log slot, as Apple's _Merge and ftl_remove_scattered_block() in
 * ftl-nano2g.c do. With no entry given, fold the oldest - the one whose
 * pages have been sitting unmerged longest. A given log that is at most
 * half live is compacted rather than folded. */
static int ftl_remove_scattered_block(struct ftl_log *log)
{
    unsigned int i;

    ftl_cxt.swapcounter++;
    if (!log)
    {
        uint32_t age = 0xffffffff;

        for (i = 0; i < ftl_nlogs; i++)
            if (ftl_logs[i].usn <= age)
            {
                age = ftl_logs[i].usn;
                log = &ftl_logs[i];
            }
        if (!log)
            return -1;
    }
    else if (log->pagescurrent <= sbpages / 2)
        return ftl_compact_scattered(log);
    if (log->issequential)
        return ftl_commit_sequential(log);
    return ftl_commit_scattered(log);
}

/* A log for this logical block, making room if need be. */
static struct ftl_log *ftl_allocate_log_entry(uint32_t lblock)
{
    struct ftl_log *log = ftl_get_log_entry(lblock);
    uint32_t sb, i;

    if (log)
        goto used;
    if (ftl_nlogs == FTL_MAX_LOGS && ftl_remove_scattered_block(NULL))
        return NULL;
    /* Keep enough of the pool back for a fold to be possible: Apple's
     * _PrepareLog merges a victim when only three free blocks remain */
    if (ftl_cxt.freecount <= 3 && ftl_remove_scattered_block(NULL))
        return NULL;
    if (ftl_nlogs == FTL_MAX_LOGS || ftl_pool_alloc(&sb, false))
        return NULL;

    log = &ftl_logs[ftl_nlogs++];
    log->sb = sb;
    log->lblock = lblock;
    log->pagesused = 0;
    log->pagescurrent = 0;
    log->issequential = 1;
    memset(log->offsets, 0xff, sizeof(log->offsets));
used:
    /* Its recency, for choosing a victim, as _PrepareLog keeps it */
    log->usn = ftl_cxt.maxusn - 1;
    if (!log->usn)
        for (i = 0; i < ftl_nlogs; i++)
            ftl_logs[i].usn = 0;
    return log;
}

/* Pages of a log, counting those queued but not yet programmed */
static uint32_t ftl_log_used(const struct ftl_log *log)
{
    return log->pagesused + ftl_queued_n;
}

/* Append one page to a log, as ftl_write() in ftl-nano2g.c does. The page
 * is only queued: FTL_Write hands everything it appends to a log to the VFL
 * in one run, and ftl_log_flush() must be called before anything else
 * touches the flash. The log itself is updated only when the run has been
 * programmed, as FTL_Write does. */
static void ftl_log_append(struct ftl_log *log, uint32_t idx,
                          const void *data)
{
    uint32_t v = ftl_log_used(log);
    uint32_t lpn = log->lblock * sbpages + idx;
    uint32_t type = SPARE_DATA;

    if (!ftl_queued_n)
    {
        ftl_queued_current = log->pagescurrent;
        ftl_queued_sequential = log->issequential;
    }
    if (v == sbpages - 1 && ftl_queued_sequential)
        type = SPARE_DATA_LAST;
    ftl_run_add(log->sb, v, data, lpn, ftl_data_usn(v), type);
    ftl_queued_idx[ftl_queued_n++] = idx;
    if (log->offsets[idx] == FTL_NO_PAGE)
        ftl_queued_current++;
    if (v + 1 != ftl_queued_current || v != idx)
        ftl_queued_sequential = false;
}

/* Program what ftl_log_append() queued for a log and enter the pages into
 * it. If the chip refuses a page, the log is closed without them, as
 * FTL_Write closes a log whose program failed: nothing is appended after a
 * page that may be half programmed, and the retry folds the log away, which
 * erases and so replaces the failed block. */
static int ftl_log_flush(struct ftl_log *log)
{
    int rc = ftl_run_write();
    uint32_t i;

    if (rc)
    {
        log->pagesused = sbpages;
        log->issequential = 0;
    }
    else
    {
        for (i = 0; i < ftl_queued_n; i++)
            log->offsets[ftl_queued_idx[i]] = log->pagesused + i;
        log->pagesused += ftl_queued_n;
        if (ftl_queued_n)
        {
            log->pagescurrent = ftl_queued_current;
            log->issequential = ftl_queued_sequential;
        }
    }
    ftl_queued_n = 0;
    return rc;
}

/* Static wear levelling. Allocation alone only spreads wear over blocks that
 * are already being written; a block holding data nobody rewrites keeps its
 * low erase count for ever while the working set grinds down. When the free
 * pool has worn well ahead of the least-worn mapped block, move that block's
 * data into the *most* worn free one, so the cold block joins the pool.
 * One relocation per call; callers ration calls with swapcounter. */
#define FTL_WEAR_SPREAD 64

static int ftl_wear_level(void)
{
    uint32_t i, sb, coldlb = usersb, coldec = 0xffff, poolmax = 0;

    if (!ftl_cxt.freecount)
        return 0;
    for (i = 0; i < ftl_cxt.freecount; i++)
    {
        sb = ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE];
        if (sb < nsuperblocks && ftl_erasectr[sb] > poolmax)
            poolmax = ftl_erasectr[sb];
    }
    for (i = 0; i < usersb; i++)
        if (ftl_map[i] < nsuperblocks && ftl_erasectr[ftl_map[i]] < coldec)
        {
            coldec = ftl_erasectr[ftl_map[i]];
            coldlb = i;
        }
    if (coldlb == usersb || poolmax <= coldec + FTL_WEAR_SPREAD)
        return 0;

    logf("ftl: wear levelling lblock %lu (ec %lu) vs pool max %lu",
         (unsigned long)coldlb, (unsigned long)coldec,
         (unsigned long)poolmax);
    return ftl_merge(coldlb, NULL, NULL, true);
}

/* Reserve n consecutive pages in a control block. A full block is erased and
 * reused in place, exactly as Apple's _NextCtrlPage does - no pool block and
 * no VFL commit is needed for this. */
static int ftl_ctrl_reserve(uint32_t n, uint32_t *sb, uint32_t *v)
{
    if (n > sbpages)
        return -1;
    if (ftl_ctrl_v + n > sbpages)
    {
        uint32_t next = (ftl_ctrl_idx + 1) % 3;

        /* Only after failed programs can the ring come round to the block
         * holding the newest context; erasing it would leave none */
        if (next == ftl_ctrl_good)
        {
            logf("ftl: no control block left to write");
            return -1;
        }
        if (ftl_ctrl[next] >= nsuperblocks || ftl_erase_sb(ftl_ctrl[next]))
            return -1;
        ftl_ctrl_idx = next;
        ftl_ctrl_v = 0;
    }
    *sb = ftl_ctrl[ftl_ctrl_idx];
    *v = ftl_ctrl_v;
    ftl_ctrl_v += n;
    return 0;
}

/* Mark the context dirty before the first write after a commit, as
 * _FTLMarkCxtNotValid does. A mount, ours or Apple's, then finds a 0x4f page
 * after the context and runs the restore, instead of trusting a context that
 * predates the writes. */
static int ftl_mark_dirty(void)
{
    uint32_t sb, v;

    int tries;

    if (!ftl_cxt.cleanflag)
        return 0;                       /* already marked */
    /* Three tries; after a failed program the rest of that control block
     * is skipped, as _FTLMarkCxtNotValid does */
    for (tries = 0; tries < 3; tries++)
    {
        if (ftl_ctrl_reserve(1, &sb, &v))
            return -1;
        memset(ftl_buffer, 0xff, NAND_PAGE_SIZE);
        if (!ftl_write_vpage(sb, v, ftl_buffer, ftl_cxt.usn - 1, 0xffffffff,
                             SPARE_FTL_MOUNT))
        {
            ftl_cxt.cleanflag = 0;
            return 0;
        }
        if (!ftl_writable)
            return -1;
        ftl_ctrl_v = sbpages;
    }
    return -1;
}

/* Write the 25-page commit: erase counters, read counters, map, log tables,
 * stats, then the context, all carrying the same usn, in Apple's order, as
 * _StoreFTLCxt does. The open logs are recorded in the context's log slots
 * and the offset tables, so a commit need not fold them. Returns -2 when
 * the chip refused a page. */
static int ftl_commit_once(void)
{
    static uint16_t table[FTL_LOG_PAGES * (NAND_PAGE_SIZE / 2)];
    uint32_t per = NAND_PAGE_SIZE / 2;
    uint32_t npages, usn, sb, v0, i;
    uint32_t ecv, rcv, mapv, logv, statsv, cxtv;

    npages = FTL_COMMIT_PAGES(ftl_necpages, ftl_nrcpages, ftl_nmappages,
                              ftl_nlogpages);
    if (npages > sbpages || npages > FTL_COPY_PAGES)
        return -1;

    if (ftl_ctrl_reserve(npages, &sb, &v0))
        return -1;
    usn = ftl_cxt.usn - npages;

    ecv = v0;
    rcv = ecv + ftl_necpages;
    mapv = rcv + ftl_nrcpages;
    logv = mapv + ftl_nmappages;
    statsv = logv + ftl_nlogpages;
    cxtv = statsv + 1;

    /* The whole commit goes to the flash as one run, as _StoreFTLCxt writes
     * it; the table pages are staged in ftl_copybuf until then */
    for (i = 0; i < ftl_necpages; i++)
    {
        uint8_t *buf = ftl_copybuf + (ecv - v0 + i) * NAND_PAGE_SIZE;

        memset(buf, 0xff, NAND_PAGE_SIZE);
        memcpy(buf, ftl_erasectr + i * per,
               MIN(nsuperblocks - i * per, per) * 2);
        ftl_run_add(sb, ecv + i, buf, usn, 0xffff0000 | i,
                    SPARE_FTL_ERASECTR);
        ftl_cxt.ecpages[i] = ftl_vpn(sb, ecv + i);
    }
    for (i = 0; i < ftl_nrcpages; i++)
    {
        uint8_t *buf = ftl_copybuf + (rcv - v0 + i) * NAND_PAGE_SIZE;

        memset(buf, 0xff, NAND_PAGE_SIZE);
        memcpy(buf, ftl_readcount + i * per,
               MIN(nsuperblocks - i * per, per) * 2);
        ftl_run_add(sb, rcv + i, buf, usn, 0xffff0000 | i,
                    SPARE_FTL_READCOUNT);
        ftl_cxt.rcpages[i] = ftl_vpn(sb, rcv + i);
    }
    for (i = 0; i < ftl_nmappages; i++)
    {
        uint8_t *buf = ftl_copybuf + (mapv - v0 + i) * NAND_PAGE_SIZE;

        memset(buf, 0xff, NAND_PAGE_SIZE);
        memcpy(buf, ftl_map + i * per, MIN(usersb - i * per, per) * 2);
        ftl_run_add(sb, mapv + i, buf, usn, 0xffff0000 | i, SPARE_FTL_MAP);
        ftl_cxt.mappages[i] = ftl_vpn(sb, mapv + i);
    }
    /* One offset table per log slot, in slot order */
    memset(table, 0xff, sizeof(table));
    for (i = 0; i < ftl_nlogs; i++)
        memcpy(table + i * sbpages, ftl_logs[i].offsets, sbpages * 2);
    for (i = 0; i < ftl_nlogpages; i++)
    {
        ftl_run_add(sb, logv + i, table + i * per, usn, 0xffff0000 | i,
                    SPARE_FTL_LOG);
        ftl_cxt.logpages[i] = ftl_vpn(sb, logv + i);
    }
    ftl_run_add(sb, statsv, ftl_stats, usn, 0xffff0000, SPARE_FTL_STATS);
    ftl_cxt.statspage = ftl_vpn(sb, statsv);
    ftl_cxt.statsflag = 0xffffffff;
    ftl_cxt.cleanflag = 1;              /* everything is in the map again */

    /* The context itself */
    ftl_cxt.usn = usn;
    for (i = 0; i < 3; i++)
        ftl_cxt.ctrlblocks[i] = ftl_ctrl[i];
    ftl_cxt.ctrlpage = ftl_vpn(sb, cxtv);
    for (i = 0; i < FTL_CXT_LOGS; i++)
    {
        struct ftl_cxt_log *c = &ftl_cxt.logs[i];

        if (i < ftl_nlogs && i < FTL_MAX_LOGS)
        {
            c->usn = ftl_logs[i].usn;
            c->sb = ftl_logs[i].sb;
            c->lblock = ftl_logs[i].lblock;
            c->pagesused = ftl_logs[i].pagesused;
            c->pagescurrent = ftl_logs[i].pagescurrent;
            c->issequential = ftl_logs[i].issequential;
        }
        else
        {
            c->sb = 0xffff;
            c->lblock = 0xffff;
        }
    }
    /* The context fills 0x800 bytes of its page; the rest of the page is
     * programmed as 0xff */
    {
        uint8_t *buf = ftl_copybuf + (cxtv - v0) * NAND_PAGE_SIZE;

        memset(buf, 0xff, NAND_PAGE_SIZE);
        memcpy(buf, &ftl_cxt, sizeof(ftl_cxt));
        ftl_run_add(sb, cxtv, buf, usn, 0xffffffff, SPARE_FTL_CXT);
    }
    if (ftl_run_write())
    {
        ftl_cxt.cleanflag = 0;
        return -2;
    }

    ftl_cxt.usn = usn;
    return 0;
}

/* Up to five tries, each failed one skipping the rest of its control
 * block, as FTL_Flush stores the context */
static int ftl_commit(void)
{
    int tries, rc = -1;

    for (tries = 0; tries < 5; tries++)
    {
        rc = ftl_commit_once();
        if (!rc)
        {
            ftl_ctrl_good = ftl_ctrl_idx;
            return 0;
        }
        if (rc != -2 || !ftl_writable)
            break;
        logf("ftl: commit did not take, trying the next control block");
        ftl_ctrl_v = sbpages;
    }
    return -1;
}

/* src for a plain contiguous write */
struct ftl_range
{
    uint32_t first;
    uint32_t count;
    const uint8_t *buf;
};

static const void *ftl_range_src(uint32_t lpn, void *ctx)
{
    const struct ftl_range *r = ctx;

    if (lpn < r->first || lpn - r->first >= r->count)
        return NULL;
    return r->buf + (size_t)(lpn - r->first) * NAND_PAGE_SIZE;
}

/* Append the pages src() has for lblock to its log, folding the log away
 * whenever it fills. Returns 0, -6 or -7 when no log could be had (writing
 * then ends), or -8 when the chip refused a page and the log was closed
 * (ftl_log_flush()). */
static int ftl_write_log(uint32_t lblock, ftl_write_src src, void *ctx)
{
    struct ftl_log *log = ftl_allocate_log_entry(lblock);
    uint32_t idx;

    if (!log)
        return -6;
    for (idx = 0; idx < sbpages; idx++)
    {
        const void *data = src(lblock * sbpages + idx, ctx);

        if (!data)
            continue;
        if (ftl_log_used(log) == sbpages)
        {
            /* Full: fold it away and start another */
            if (ftl_log_flush(log))
                return -8;
            if (ftl_remove_scattered_block(log))
                return -7;
            log = ftl_allocate_log_entry(lblock);
            if (!log)
                return -6;
        }
        ftl_log_append(log, idx, data);
    }
    if (ftl_log_flush(log))
        return -8;
    if (log->pagesused == sbpages && ftl_remove_scattered_block(log))
        return -7;
    return 0;
}

/* Write the pages src() has new contents for, in every logical block
 * between first and last. Blocks it has nothing for are left alone. */
static int ftl_write_merge(uint32_t first, uint32_t last, ftl_write_src src,
                           void *ctx)
{
    uint32_t lblock;
    int attempt, ret = 0;

    if (!ftl_mounted || !ftl_writable || !src)
        return -1;
    if (last < first || last >= usersb * sbpages)
        return -2;

    mutex_lock(&ftl_mtx);
    for (lblock = first / sbpages; lblock <= last / sbpages; lblock++)
    {
        if (!ftl_block_dirty(lblock, src, ctx))
            continue;

        /* Anything we leave in a log block is invisible to the committed
         * map, so the context has to say so before a page is written. Once
         * per block rather than once per call: the commit below leaves the
         * context clean again, and the blocks after it need marking too. */
        if (ftl_mark_dirty())
        {
            ftl_writable = false;
            ret = -5;
            break;
        }

        /* A whole block at once needs no log and costs no amplification */
        if (ftl_block_complete(lblock, src, ctx))
        {
            if (ftl_merge(lblock, src, ctx, false))
            {
                /* The map still points at the old block, so nothing is
                 * lost; but our pool accounting may no longer match the
                 * medium. */
                ftl_writable = false;
                ret = -3;
                break;
            }
            continue;
        }

        /* A program the chip refused closed the log; the next attempt
         * folds it away and writes the pages again in a fresh one */
        for (attempt = 0; attempt < 4; attempt++)
        {
            ret = ftl_write_log(lblock, src, ctx);
            if (ret != -8 || !ftl_writable)
                break;
            logf("ftl: lblock %lu log did not take, retrying",
                 (unsigned long)lblock);
        }
        if (ret)
        {
            ftl_writable = false;
            break;
        }
    }
    /* Static wear levelling on the budget Apple's FTL_Write and
     * ftl-nano2g.c's ftl_write() use: every merge adds one, and at 300 one
     * cold block is moved. There is no commit here - like Apple's firmware,
     * ftl_sync() alone commits, and an unclean mount recovers the rest. */
    if (!ret && ftl_cxt.swapcounter >= 300)
    {
        ftl_cxt.swapcounter -= 20;
        if (ftl_wear_level())
        {
            ftl_writable = false;
            ret = -4;
        }
    }
    mutex_unlock(&ftl_mtx);
    return ret;
}

int ftl_write(uint32_t sector, uint32_t count, const void *buffer)
{
    struct ftl_range r;

    if (!ftl_mounted || !ftl_writable)
        return -1;
    if (!count)
        return 0;
    if (sector + count > usersb * sbpages || sector + count < sector)
        return -2;
    r.first = sector;
    r.count = count;
    r.buf = buffer;
    return ftl_write_merge(sector, sector + count - 1, ftl_range_src, &r);
}

/* The commit sequence, with ftl_mtx already held. */
static int ftl_sync_locked(void)
{
    int ret = 0;

    /* Fold every log block away - the ones we filled this session and any
     * left by whoever wrote the NAND last - so the commit can say there are
     * none and the free pool is whole again, as Apple's full FTL_Flush and
     * ftl-nano2g.c's ftl_sync() do. A log that was written in order is
     * adopted outright rather than copied. */
    if (ftl_cxt.swapcounter >= 20)
    {
        ftl_cxt.swapcounter -= 20;
        if (ftl_wear_level())
        {
            ftl_writable = false;
            return -6;
        }
    }
    while (ftl_nlogs)
    {
        ftl_cxt.swapcounter++;
        if (ftl_commit_sequential(&ftl_logs[0]))
        {
            ftl_writable = false;
            return -2;
        }
    }
    if (ftl_commit())
    {
        ftl_writable = false;
        ret = -4;
    }
    return ret;
}

int ftl_sync(void)
{
    int ret;

    if (!ftl_mounted)
        return -1;
    /* Nothing written since the last commit: nothing to do. Shutdown calls
     * this every time, and ftl-nano2g.c returns early the same way. A
     * read-only mount cannot have written anything either, as with the
     * Nano 2G's FTL_READONLY. */
    if (ftl_cxt.cleanflag || ftl_readonly)
        return 0;
    if (!ftl_writable)
        return -1;
    mutex_lock(&ftl_mtx);
    ret = ftl_sync_locked();
    mutex_unlock(&ftl_mtx);
    return ret;
}

int ftl_read(uint32_t sector, uint32_t count, void *buffer)
{
    uint8_t *buf = buffer;
    uint32_t n;
    int ret = 0;

    if (!ftl_mounted)
        return -1;
    if (!count)
        return 0;
    if (sector + count > usersb * sbpages || sector + count < sector)
        return -2;

    mutex_lock(&ftl_mtx);
    while (count)
    {
        n = MIN(count, sbpages - sector % sbpages);
        if (ftl_read_pages(sector, n, buf))
        {
            ret = -3;
            break;
        }
        sector += n;
        count -= n;
        buf += n * NAND_PAGE_SIZE;
    }
    mutex_unlock(&ftl_mtx);
    return ret;
}

int ftl_is_writable(void)
{
    return ftl_writable ? 1 : 0;
}

uint32_t ftl_num_sectors(void)
{
    return ftl_mounted ? usersb * sbpages : 0;
}

#ifdef BOOTLOADER
int ftl_get_free_block(uint32_t *sb, uint32_t *bank, uint32_t *block,
                       uint32_t *freecount)
{
    uint32_t i;

    if (!ftl_mounted || !ftl_cxt.freecount
        || !sb || !bank || !block || !freecount)
        return -1;

    for (i = 0; i < ftl_cxt.freecount; i++)
    {
        uint32_t idx = (ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE;
        uint32_t candidate = ftl_cxt.blockpool[idx];
        uint32_t b = ftl_vbanks;

        if (candidate >= nsuperblocks)
            continue;
        /* Prefer a high-numbered bank and an unremapped physical block.
         * The caller still verifies every page is erased before use. */
        while (b--)
            if (ftl_vfl_phys_block(b, candidate)
                == ftl_vfl_c2p(b, candidate))
            {
                *sb = candidate;
                *bank = b % geo->banks;
                *block = ftl_vfl_c2p(b, candidate);
                *freecount = ftl_cxt.freecount;
                return 0;
            }
    }
    return -2;
}

int ftl_is_free_block(uint32_t bank, uint32_t block)
{
    uint32_t i, vb;

    if (!ftl_mounted || bank >= geo->banks || block >= geo->blocks)
        return 0;
    for (i = 0; i < ftl_cxt.freecount; i++)
    {
        uint32_t idx = (ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE;
        uint32_t sb = ftl_cxt.blockpool[idx];

        for (vb = bank; sb < nsuperblocks && vb < ftl_vbanks;
             vb += geo->banks)
            if (ftl_vfl_c2p(vb, sb) == block
                && ftl_vfl_phys_block(vb, sb) == block)
                return 1;
    }
    return 0;
}

int ftl_boot_block_reserved(uint32_t bank, uint16_t *poolstart,
                            int *in_ring, int *vblock0_remapped)
{
    const struct ftl_vfl_cxt *cxt;
    uint32_t i, slots;

    if (poolstart)
        *poolstart = 0;
    if (in_ring)
        *in_ring = 0;
    if (vblock0_remapped)
        *vblock0_remapped = 0;
    if (!ftl_vfl_ok || !geo || bank >= geo->banks)
        return -1;
    cxt = &ftl_vfl_cxt[bank];
    if (poolstart)
        *poolstart = cxt->poolstart;
    if (in_ring)
        for (i = 0; i < 4; i++)
            if (cxt->vflcxtblocks[i] == 0)
                *in_ring = 1;
    slots = cxt->poolstart * ftl_ways < geo->blocks
            ? geo->blocks - cxt->poolstart * ftl_ways : 0;
    if (slots > 820)
        slots = 820;
    if (vblock0_remapped)
        for (i = 0; i < slots; i++)
            if (cxt->poolmap[i] == 0)
                *vblock0_remapped = 1;
    return 0;
}
#endif

/* Programming this generation is not implemented. */
int ftl_format(void)
{
    return -1;
#if 0
    uint32_t i, next, bank;
    uint16_t oldctrl[3];
    bool vfl_ctrl_changed = false;

    if (!geo || !ftl_vfl_ok)
        return -1;
    if (nsuperblocks - usersb != 3 + FTL_POOL_SIZE)
        return -2;

    mutex_lock(&ftl_mtx);

    memcpy(oldctrl, ftl_vfl_cxt[0].ftlctrlblocks, sizeof(oldctrl));
    memcpy(ftl_ctrl, oldctrl, sizeof(ftl_ctrl));
    for (i = 0; i < 3; i++)
        if (ftl_ctrl[i] >= nsuperblocks)
        {
            ftl_ctrl[0] = usersb;
            ftl_ctrl[1] = usersb + 1;
            ftl_ctrl[2] = usersb + 2;
            vfl_ctrl_changed = true;
            break;
        }

    memset(&ftl_cxt, 0, sizeof(ftl_cxt));
    memset(ftl_map, 0xff, sizeof(ftl_map));
    memset(ftl_erasectr, 0, sizeof(ftl_erasectr));
    memset(ftl_readcount, 0, sizeof(ftl_readcount));
    memset(ftl_stats, 0xff, sizeof(ftl_stats));
    ftl_nlogs = 0;
    ftl_nholes = 0;
    memset(ftl_hole, 0, sizeof(ftl_hole));

    next = 0;
    for (i = 0; i < usersb; i++)
    {
        while (next < nsuperblocks
               && (next == ftl_ctrl[0] || next == ftl_ctrl[1]
                   || next == ftl_ctrl[2]))
            next++;
        if (next >= nsuperblocks)
        {
            mutex_unlock(&ftl_mtx);
            return -3;
        }
        ftl_map[i] = next++;
    }
    for (i = 0; i < usersb; i++)
        ftl_hole[i / 8] |= (uint8_t)(1 << (i % 8));
    ftl_nholes = usersb;

    ftl_cxt.usn = 0xffffff00;
    ftl_cxt.maxusn = 0;
    ftl_cxt.nextfreeidx = 0;
    ftl_cxt.freecount = 0;
    ftl_cxt.swapcounter = 0;
    ftl_cxt.cleanflag = 1;
    for (; next < nsuperblocks && ftl_cxt.freecount < FTL_POOL_SIZE; next++)
        if (next != ftl_ctrl[0] && next != ftl_ctrl[1] && next != ftl_ctrl[2])
            ftl_cxt.blockpool[ftl_cxt.freecount++] = next;
    if (ftl_cxt.freecount != FTL_POOL_SIZE)
    {
        mutex_unlock(&ftl_mtx);
        return -4;
    }
    for (i = 0; i < FTL_CXT_LOGS; i++)
    {
        ftl_cxt.logs[i].sb = 0xffff;
        ftl_cxt.logs[i].lblock = 0xffff;
    }

    ftl_set_page_counts();
    ftl_ctrl_idx = 0;
    ftl_ctrl_v = 0;
    ftl_run_n = 0;
    ftl_writable = true;
    ftl_readonly = false;

    for (i = 0; i < 3; i++)
        if (ftl_erase_sb(ftl_ctrl[i]))
        {
            ftl_writable = false;
            mutex_unlock(&ftl_mtx);
            return -5;
        }
    for (i = 0; i < ftl_cxt.freecount; i++)
        if (ftl_erase_sb(ftl_cxt.blockpool[i]))
        {
            ftl_writable = false;
            mutex_unlock(&ftl_mtx);
            return -8;
        }

    if (ftl_commit())
    {
        ftl_writable = false;
        mutex_unlock(&ftl_mtx);
        return -6;
    }

    if (vfl_ctrl_changed)
        for (bank = 0; bank < geo->banks; bank++)
            if (ftl_vfl_commit_cxt(bank))
            {
                mutex_unlock(&ftl_mtx);
                return -7;
            }

    ftl_mounted = true;
    mutex_unlock(&ftl_mtx);
    return 0;
#endif
}

/* The rest of _FTLRestore, which writes: erase the blocks given to holes
 * (phase D) and the free pool (H), then commit (J). A used volume has a
 * hole for every logical block that has never been written, so a large
 * count is normal. Erasing them all takes minutes and must not fail the
 * mount: on failure the FTL stays mounted and refuses further writes. */
static int ftl_finish_restore(void)
{
    uint32_t i, lblock;
    int rc = 0;

    for (lblock = 0; lblock < usersb && !rc; lblock++)
        if (ftl_is_hole(lblock))
            rc = ftl_erase_sb(ftl_map[lblock]);
    ftl_nholes = 0;
    for (i = 0; i < ftl_cxt.freecount && !rc; i++)
        rc = ftl_erase_sb(ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + i)
                                            % FTL_POOL_SIZE]);
    if (!rc)
        rc = ftl_commit();
    if (rc)
    {
        logf("ftl: restore could not be written (%d)", rc);
        ftl_writable = false;
        ftl_readonly = true;
        nand_set_writable(false);
    }
    return rc;
}

#ifdef N4G_BL_FTL_WRITE
/* Bootloader DFU file copy. ftl_init() left the mount read-only; this turns
 * writes on after the user has confirmed, and runs the restore tail when
 * the mount was rebuilt. */
int ftl_arm_writes(void)
{
    if (!ftl_mounted)
        return -1;
    if (ftl_read_dbg.writecheck != 0)
        return -(int)ftl_read_dbg.writecheck;
    if (ftl_read_dbg.restored && ftl_nholes > 64)
    {
        ftl_read_dbg.writecheck = 5;
        return -5;
    }
    ftl_writable = true;
    ftl_readonly = false;
    nand_set_writable(true);
    if (ftl_read_dbg.restored)
        return ftl_finish_restore();
    return 0;
}
#endif

int ftl_init(void)
{
    const struct nand_geometry *g;
    const struct ftl_vfl_cxt *c0;
    struct ftl_vfl_cxt *newest;
    uint32_t bank;
    bool restored;
    int rc;

    ftl_mounted = false;
    ftl_vfl_ok = false;
    mutex_init(&ftl_mtx);
    ftl_nholes = 0;

    g = nand_get_geometry();
    if (!g || g->banks == 0 || g->banks > FTL_MAX_BANKS)
        return -1;
    ftl_geo = *g;
    /* VSVFL addresses each way of a chip enable as one virtual bank */
    ftl_geo.planes = 1;
    ftl_geo.twoplane = false;
    geo = &ftl_geo;

    ftl_vfl_usn = 0;
    newest = &ftl_vfl_cxt[0];
    for (bank = 0; bank < geo->banks; bank++)
    {
        if (ftl_vfl_open_bank(bank))
        {
            logf("ftl: no VFL context on bank %lu", (unsigned long)bank);
            return -3;
        }
        if (ftl_vfl_cxt[bank].usn >= ftl_vfl_usn)
        {
            ftl_vfl_usn = ftl_vfl_cxt[bank].usn;
            newest = &ftl_vfl_cxt[bank];
        }
    }
    /* As VSVFL_Open: every bank takes the FTL's fields from the newest
     * context, so that a context stored later on any bank carries them */
    for (bank = 0; bank < geo->banks; bank++)
    {
        struct ftl_vfl_cxt *c = &ftl_vfl_cxt[bank];

        if (c == newest)
            continue;
        memcpy(c->ftlctrlblocks, newest->ftlctrlblocks,
               sizeof(c->ftlctrlblocks));
        c->poolstart = newest->poolstart;
        c->usableblocks = newest->usableblocks;
        c->ftltype = newest->ftltype;
        memcpy(c->field_6ca, newest->field_6ca, sizeof(c->field_6ca));
        ftl_vfl_update_checksum(c);
    }
    ftl_vfl_ok = true;

    /* Apple's FTL takes VFL info 0x70 (usable blocks) less 23 as its
     * logical blocks, and one block of every virtual bank per superblock.
     * The ways follow from the vendor type, as VSVFL_GetWaysForVendor
     * reads its low half. */
    c0 = &ftl_vfl_cxt[0];
    ftl_vendor = c0->vendortype[0] | (uint32_t)c0->vendortype[1] << 16;
    switch (ftl_vendor & 0xffff)
    {
        case 0x01: ftl_ways = 1; break;
        case 0x13: ftl_ways = 4; break;
        case 0x10: case 0x11: case 0x12: case 0x14: ftl_ways = 2; break;
        default: ftl_ways = 0; break;
    }
    ftl_vbanks = geo->banks * ftl_ways;
    for (bank = 0; bank < geo->banks; bank++)
    {
        const struct ftl_vfl_cxt *c = &ftl_vfl_cxt[bank];

        if (!ftl_ways
            || (c->vendortype[0] | (uint32_t)c->vendortype[1] << 16)
                != ftl_vendor
            || c->usableblocks != c0->usableblocks
            || c->poolstart < c->usableblocks
            || c->poolstart >= geo->blocks / ftl_ways)
        {
            logf("ftl: bank %lu VSVFL layout not handled",
                 (unsigned long)bank);
            return -4;
        }
    }
    nsuperblocks = c0->usableblocks;
    pool_slots = MIN(geo->blocks - c0->poolstart * ftl_ways, 820u);
    sbpages = geo->pagesperblock * ftl_vbanks;
    if (nsuperblocks <= FTL_RESERVED_SB)
        return -2;
    usersb = nsuperblocks - FTL_RESERVED_SB;
    if (nsuperblocks > FTL_MAX_SB || usersb > FTL_MAX_USERSB
        || sbpages > FTL_MAX_SBPAGES)
        return -2;

    /* Apple's FTL_Open: the context if it describes the medium, else
     * _FTLRestore */
    rc = ftl_load_cxt();
    if (rc < 0)
    {
        logf("ftl: no FTL context (%d)", rc);
        return -10 + rc;
    }
    restored = !rc;
    rc = restored ? ftl_restore() : ftl_load_clean();
    if (rc)
    {
        logf("ftl: %s failed (%d)", restored ? "restore" : "load", rc);
        return -20 + rc;
    }
    ftl_read_dbg.writecheck = ftl_open_write_state();
    ftl_read_dbg.restored = restored;
    ftl_read_dbg.nlogs = ftl_nlogs;
    ftl_read_dbg.nholes = ftl_nholes;
    ftl_read_dbg.cxtusn = ftl_cxt.usn;
#if defined(N4G_FTL_WRITE) && !defined(BOOTLOADER)
    /* Pages are programmed by fmss/program.s in the page format Apple's
     * firmware writes. Pool blocks are taken as erased, so a restore
     * whose tail below does not run (too many holes) leaves the mount
     * read-only. */
    if (ftl_writable && restored && ftl_nholes > 64)
    {
        logf("ftl: restore left %lu holes, read-only",
             (unsigned long)ftl_nholes);
        ftl_writable = false;
        ftl_read_dbg.writecheck = 5;
    }
#else
    /* Without N4G_FTL_WRITE the mount is read-only, and the bootloader
     * stays read-only until ftl_arm_writes() (the DFU file-copy image).
     * Nothing below erases the hole blocks or the free pool. */
    ftl_writable = false;
#endif
    ftl_readonly = !ftl_writable;
    nand_set_writable(ftl_writable);

    /* The rest of _FTLRestore, which writes. A large hole count is normal
     * and must not be erased here. */
    if (restored && ftl_writable && ftl_nholes <= 64)
        ftl_finish_restore();

    ftl_mounted = true;
    return 0;
}
