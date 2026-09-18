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
 * The log-block handling, the control-block list and the VFL context commit
 * follow firmware/target/arm/s5l8700/ipodnano2g/ftl-nano2g.c, which decodes
 * the same Whimory structures on the Nano 2G. Several functions are ports of
 * their counterparts there.
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
 * FTL for the iPod Nano 3G: Apple's Whimory, in the variant this firmware
 * uses. It is related to the Nano 2G's (ftl-nano2g.c), but the VFL context
 * layout differs and a virtual block groups one to four physical blocks per
 * chip enable, as the chip's mode lays them out.
 *
 *
 * From the bottom up:
 *  - VFL: each bank's blocks form planes units ("planes"); virtual block b
 *    of a unit is a physical block placed by the chip's mode
 *    (ftl_unit_block()), unless that bank's VFL context remaps it. The
 *    context holds one table of vflspares slots per unit. Slot k holds the
 *    physical block it replaces (0xfff0 free, 0xffff unusable) and stands
 *    for the reserved virtual block nsuperblocks + k % vflspares of unit
 *    k / vflspares, as Apple's VFL numbers them.
 *  - Superblocks: superblock s is virtual block s of every unit of every
 *    bank. Its pages are numbered v = page * banks * planes + plane * banks
 *    + bank.
 *  - FTL: logical page lpn is at index lpn % sbpages of logical block
 *    lpn / sbpages. The block map, committed in the FTL control blocks, says
 *    which superblock holds each logical block. Pages written since a block
 *    was last merged are in log superblocks, which the context and its log
 *    offset tables record at every commit; a page present in a log is newer
 *    than the one the map points at, and ftl_resolve() prefers it.
 *
 * The write path follows the original firmware's own FTL: writes append to
 * logs, a merge folds or compacts them when they fill or their slot is
 * needed, and ftl_sync() folds every log and commits, as the firmware's
 * flush does. Nothing else commits, so a mount that finds writes after the
 * last commit rebuilds the tables from the medium, as the firmware's
 * restore does. Pages go to the flash in runs, the way the firmware hands
 * them to the VFL.
 * A block that will not take a write or an erase is replaced from the VFL's
 * reserved spares.
 */

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "panic.h"
#include "storage.h"
#include "nand-target.h"
#include "ftl-target.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define FTL_MAX_BANKS       4
/* Sized for the largest chip in nand_chip_table[]: the 8GB parts give 3895
 * superblocks and 3872 logical blocks. The 4GB one needs half of that. */
#define FTL_MAX_SB          4096    /* superblocks in the virtual area */
#define FTL_MAX_USERSB      4096    /* logical blocks */
#define FTL_MAX_SBPAGES     1024    /* pages per superblock */
#define FTL_MAX_LOGS        17      /* Whimory keeps 0x11 log blocks */
#define FTL_MAP_PAGES       (FTL_MAX_USERSB * 2 / NAND_PAGE_SIZE)
#define FTL_VFL_SCANBLOCKS  200     /* where to look for a VFL context */
#define FTL_NO_PAGE         0xffff
#define FTL_VFL_FREE        0xfff0  /* a reserved block nothing uses yet */
#define FTL_VFL_BAD         0xffff  /* a reserved block that is itself bad */
#define FTL_VFL_PENDING     20      /* Apple's pending-bad list length */

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
 * found it, so a commit of ours preserves what the original firmware
 * expects. Field offsets were derived from the firmware's own context load
 * and store, and the FTL_CHECK assertions below pin every one of them. */
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
/* The page counts above are at the smallest page size. A table is read and
 * written a whole page at a time, so its buffer is rounded up to the
 * largest. */
#define FTL_WHOLE_PAGES(bytes) (((bytes) + NAND_MAX_PAGE_SIZE - 1) \
                                / NAND_MAX_PAGE_SIZE * NAND_MAX_PAGE_SIZE)
#define FTL_SIDE_BYTES      FTL_WHOLE_PAGES(FTL_SIDE_ENTRIES * 2)
#define FTL_LOG_BYTES       FTL_WHOLE_PAGES(FTL_MAX_SBPAGES * FTL_MAX_LOGS * 2)
/* Pages one commit occupies: erase counters, read counters, map, log
 * tables, stats, then the context itself */
#define FTL_COMMIT_PAGES(ec, rc, map, log) ((ec) + (rc) + (map) + (log) + 2)

/* VFL context, one per bank: nano2g's header, then per-plane remap tables */
struct ftl_vfl_cxt
{
    uint32_t usn;                   /* cross-bank update sequence number */
    uint16_t ftlctrlblocks[3];      /* superblocks holding FTL control pages */
    uint16_t field_a;
    uint32_t updatecount;           /* decrementing, also in the spare */
    uint16_t activecxtblock;
    uint16_t nextcxtpage;
    uint16_t badcount;              /* slots marked 0xffff, i.e. spares that
                                     * are themselves bad - not a count of
                                     * remapped blocks (measured) */
    uint16_t field_16[3];
    uint16_t usedcount[4];          /* slots taken in each unit's table */
    uint16_t field_24[4];
    uint16_t remap[820];            /* plane tables, vflspares entries each */
    uint16_t vflcxtblocks[4];       /* physical blocks of the context ring */
    uint16_t pendingcount;          /* blocks awaiting replacement */
    uint16_t pending[FTL_VFL_PENDING]; /* physical blocks that failed a read
                                     * or write, replaced at their next
                                     * erase */
    uint8_t field_6c6[0x12e];
    uint32_t version;
    uint32_t checksum1;
    uint32_t checksum2;
};

#define FTL_CHECK(name, cond) typedef char name[(cond) ? 1 : -1]
FTL_CHECK(ftl_vfl_cxt_size, sizeof(struct ftl_vfl_cxt) == 0x800);
FTL_CHECK(ftl_vfl_cxt_remap, offsetof(struct ftl_vfl_cxt, remap) == 0x2c);
FTL_CHECK(ftl_vfl_cxt_blocks,
          offsetof(struct ftl_vfl_cxt, vflcxtblocks) == 0x694);
FTL_CHECK(ftl_vfl_cxt_pending,
          offsetof(struct ftl_vfl_cxt, pending) == 0x69e);
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

/* Source of replacement page data for ftl_write_merge(): a pointer to a
 * page of data for lpn, or NULL to keep what is there now. */
typedef const void *(*ftl_write_src)(uint32_t lpn, void *ctx);

static const struct nand_geometry *geo;
static uint32_t nsuperblocks;   /* superblocks in the virtual area */
static uint32_t sbpages;        /* pages per superblock */
static uint32_t usersb;         /* logical blocks */
static uint32_t pagesize;       /* bytes per NAND page */
static uint32_t secperpage;     /* NAND_PAGE_SIZE sectors in one page */
static bool ftl_mounted = false;
static struct mutex ftl_mtx;

static struct ftl_vfl_cxt ftl_vfl_cxt[FTL_MAX_BANKS];
static uint16_t ftl_map[FTL_MAX_USERSB];
static uint16_t ftl_ctrl[3];
static struct ftl_log ftl_logs[FTL_MAX_LOGS];
/* A page-sized scratch for the log offset tables, which a clean load reads
 * and a commit writes - never both at once, so they share it */
static uint16_t ftl_logtable[FTL_LOG_BYTES / 2] STORAGE_ALIGN_ATTR;

/*
 * The buffers here and below are sized for the largest chip in the driver's
 * table, since storage is up long before there is an allocator to ask. They
 * come to about 540KB, 2% of this target's 28MB audio buffer. Overlapping
 * the mount-time ones (ftl_rs, the restore's log slots) with ftl_copybuf
 * would save 68KB, and halving the copy batch another 128KB, but the first
 * trades a user's flash against an aliasing invariant and the second slows
 * every merge; 0.4% of the buffer does not pay for either. ftl_copybuf's
 * size is one constant (FTL_COPY_PAGES) for anyone who disagrees.
 */
static unsigned int ftl_nlogs;

static uint8_t ftl_buffer[NAND_MAX_PAGE_SIZE] STORAGE_ALIGN_ATTR;
static uint32_t ftl_meta[NAND_META_WORDS];

/* Write-path state. The committed context page is kept verbatim so that a
 * commit of ours preserves every field we do not understand. */
static bool ftl_writable = false;
static bool ftl_readonly = false;   /* not writable since the mount */
static struct ftl_cxt ftl_cxt STORAGE_ALIGN_ATTR;
static uint16_t ftl_erasectr[FTL_SIDE_BYTES / 2];
static uint16_t ftl_readcount[FTL_SIDE_BYTES / 2];
static uint8_t ftl_stats[NAND_MAX_PAGE_SIZE] STORAGE_ALIGN_ATTR;
/* A 0x800-byte context padded out to a whole page for writing */
static uint8_t ftl_cxtpage[NAND_MAX_PAGE_SIZE] STORAGE_ALIGN_ATTR;
/* A page holding sectors a read or write covers only part of */
static uint8_t ftl_sectorpage[NAND_MAX_PAGE_SIZE] STORAGE_ALIGN_ATTR;
static uint32_t ftl_vfl_usn;    /* cross-bank VFL context update counter */
static uint32_t ftl_ctrl_idx;   /* which of ftl_ctrl[] is being written */
static uint32_t ftl_ctrl_v;     /* next free page in it */
static uint32_t ftl_necpages, ftl_nrcpages, ftl_nmappages, ftl_nlogpages;

/* The run of pages being gathered for one program call (see ftl_run_add) */
static struct nand_write ftl_run[FTL_MAX_SBPAGES];
static uint32_t ftl_run_meta[FTL_MAX_SBPAGES][NAND_META_WORDS];
static uint32_t ftl_run_sb, ftl_run_v, ftl_run_n;
/* Merges read a batch of pages, then write it: an eighth of a superblock
 * at a time, as the firmware's simple, copy and move merges do.
 * The buffer holds that at any page size, since ftl_init() refuses a
 * superblock larger than FTL_MAX_SBPAGES pages of NAND_PAGE_SIZE. */
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

/* The physical block of virtual block vblock of a bank's unit, as the
 * original firmware's per-mode address functions place it */
static uint32_t ftl_unit_block(uint32_t unit, uint32_t vblock)
{
    uint32_t half = geo->blocks / 2;

    switch (geo->layout)
    {
    case NAND_LAYOUT_SINGLE:
        return vblock;
    case NAND_LAYOUT_HALVES:
        return vblock + unit * half;
    case NAND_LAYOUT_BOTH:
        return 2 * vblock + (unit & 1) + ((unit & 2) ? half : 0);
    case NAND_LAYOUT_SPLIT13:
        /* 8320 blocks: two 4096-block regions halved, then 128 blocks in
         * four runs of 32 */
        if (vblock < 4096)
            return vblock + (vblock >= 2048 ? 2048 : 0) + unit * 2048;
        if (vblock < 4128)
            return 8192 + unit * 32 + (vblock - 4096);
        return 8256 + unit * 32 + (vblock - 4128);
    default: /* NAND_LAYOUT_ADJACENT */
        return 2 * vblock + unit;
    }
}

/* The physical block of slot k's reserved block */
static uint32_t ftl_vfl_spare_block(uint32_t k)
{
    return ftl_unit_block(k / geo->vflspares,
                         nsuperblocks + k % geo->vflspares);
}

/* Check that a bank's VFL context is laid out as the chip table row says.
 * Each unit's table is vflspares slots: the physical blocks it has
 * replaced and bad spares, then from its first free slot only free and bad
 * ones, and the rest of the array is zero (measured on the 4GB unit). A
 * wrong reserved count moves those boundaries, so this refuses a row that
 * does not describe the chip rather than mount it with the wrong map. */
static int ftl_vfl_check_cxt(const struct ftl_vfl_cxt *cxt)
{
    uint32_t unit, i, v;
    bool freeseen;

    for (i = 0; i < 4; i++)
        if (cxt->vflcxtblocks[i] >= geo->blocks)
            return -1;
    for (i = 0; i < 3; i++)
        if (cxt->ftlctrlblocks[i] >= nsuperblocks)
            return -1;
    for (unit = 0; unit < geo->planes; unit++)
    {
        const uint16_t *table = &cxt->remap[unit * geo->vflspares];

        if (cxt->usedcount[unit] > geo->vflspares)
            return -1;
        freeseen = false;
        for (i = 0; i < geo->vflspares; i++)
        {
            v = table[i];
            if (v == FTL_VFL_FREE)
                freeseen = true;
            else if (v != FTL_VFL_BAD && (freeseen || v >= geo->blocks))
                return -1;
        }
    }
    for (i = geo->planes * geo->vflspares; i < ARRAYLEN(cxt->remap); i++)
        if (cxt->remap[i])
            return -1;
    return 0;
}

/* Where virtual block vblock of a unit really is: its own physical block,
 * or the reserved block a slot of any unit replaced it with - a unit whose
 * table is full borrows another's, so Apple's lookup scans them all */
static uint32_t ftl_vfl_phys_block(uint32_t bank, uint32_t unit,
                                   uint32_t vblock)
{
    const uint16_t *remap = ftl_vfl_cxt[bank].remap;
    uint32_t block = ftl_unit_block(unit, vblock);
    uint32_t k;

    for (k = 0; k < geo->planes * geo->vflspares; k++)
        if (remap[k] == block)
            return ftl_vfl_spare_block(k);
    return block;
}

static void ftl_vpage_phys(uint32_t sb, uint32_t v,
                           uint32_t *bank, uint32_t *page)
{
    uint32_t plane = (v / geo->banks) % geo->planes;

    *bank = v % geo->banks;
    *page = ftl_vfl_phys_block(*bank, plane, sb) * geo->pagesperblock
          + v / (geo->banks * geo->planes);
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

/* Find the FTL context, as the firmware's context load does: the newest control
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
        memcpy((uint8_t *)table + i * pagesize, ftl_buffer,
               MIN(bytes - i * pagesize, pagesize));
    }
    return 0;
}

/* The rest of a clean load: map, open logs and free pool, all as the
 * context records them */
static int ftl_load_clean(void)
{
    uint32_t i, sb;

    if (ftl_read_list(ftl_cxt.mappages,
                      (usersb * 2 + pagesize - 1) / pagesize,
                      SPARE_FTL_MAP,
                      ftl_map, usersb * 2))
        return -3;
    for (i = 0; i < usersb; i++)
        if (ftl_map[i] >= nsuperblocks)
            return -4;

    /* One offset table of sbpages entries per log slot, in slot order */
    if (ftl_read_list(ftl_cxt.logpages,
                      (sbpages * FTL_MAX_LOGS * 2 + pagesize - 1)
                      / pagesize,
                      SPARE_FTL_LOG, ftl_logtable, sbpages * FTL_MAX_LOGS * 2))
        return -5;
    ftl_nlogs = 0;
    for (i = 0; i < FTL_MAX_LOGS; i++)
    {
        const struct ftl_cxt_log *c = &ftl_cxt.logs[i];
        const uint16_t *offsets = ftl_logtable + i * sbpages;
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

/* ---- Rebuilding the tables from the medium --------------------------- */
/* Rebuilds the map, the open logs and the free pool from the medium alone,
 * when the newest control page is not a context. Every phase follows the
 * decode of the original firmware's restore; the letters are that decode's
 * phases, and each one is a ftl_restore_*() of its own below, called in
 * order by ftl_restore(). The letter is on the call and on the function, so
 * the correspondence with the decode notes stays readable. Nothing is
 * written here: the caller erases the hole blocks and the pool and commits
 * (phases D, H and J) when the mount is writable. */

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

/* The firmware's promotion step: a log whose every readable page sits at
 * its own index can stand as the data block */
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

/* the firmware's log scan: every page of the block, the last copy of each
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

/* Phase C hands one slot to each log it keeps, and phase E drains the slots
 * into ftl_logs[]. The array is static because struct ftl_log is about 2KB
 * and eighteen of them will not fit on the mount's stack; it is live only
 * between ftl_restore_place_logs() and ftl_restore_build_pool(). */
#define FTL_RS_SLOTS    (FTL_MAX_LOGS + 1)
static struct ftl_log ftl_rs_slots[FTL_RS_SLOTS];

/* phase A: the control blocks; the newest by page 0 holds the next commit's
 * predecessor, and the next commit starts a fresh block */
static int ftl_restore_ctrl_blocks(void)
{
    uint32_t i, page0usn = 0xffffffff;

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
    return 0;
}

/* phase B: classify every other superblock by its last page, or by page 0
 * when that is erased or unreadable; closed blocks go into the map, the
 * higher usn winning. Returns the highest user-data usn, which phase C
 * needs and which the context records. */
static uint32_t ftl_restore_classify(void)
{
    uint32_t sb, lblock, maxusn = 0;
    int type;

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
    return maxusn;
}

/* phase C: place the logs, one per logical block */
static int ftl_restore_place_logs(uint32_t maxusn)
{
    /* Static for the same reason as ftl_rs_slots[]: one struct ftl_log is
     * about 2KB, too much to put on the stack. Phase C alone uses it. */
    static struct ftl_log cand;
    uint32_t sb, i, lblock;

    for (i = 0; i < FTL_RS_SLOTS; i++)
        ftl_rs_slots[i].lblock = 0xffff;
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
        for (i = 0; i < FTL_RS_SLOTS; i++)
        {
            struct ftl_log *s = &ftl_rs_slots[i];

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
        if (i == FTL_RS_SLOTS)
            return -31;
    }
    /* An eighteenth log: drop the one being written last - which may be the
     * eighteenth itself - or fail */
    if (ftl_rs_slots[FTL_MAX_LOGS].lblock != 0xffff)
    {
        for (i = 0; i <= FTL_MAX_LOGS; i++)
            if (ftl_rs_slots[i].usn == maxusn)
                break;
        if (i > FTL_MAX_LOGS)
        {
            logf("ftl: restore found more than %d logs", FTL_MAX_LOGS);
            return -32;
        }
        ftl_rs[ftl_rs_slots[i].sb].state = RS_FREE;
        if (i < FTL_MAX_LOGS)
            ftl_rs_slots[i] = ftl_rs_slots[FTL_MAX_LOGS];
        ftl_rs_slots[FTL_MAX_LOGS].lblock = 0xffff;
    }
    return 0;
}

/* phase D: a logical block with no data block gets the next free one. The
 * blocks handed out here are the holes ftl_is_hole() reports; the caller
 * erases them when the mount is writable (phase H). */
static int ftl_restore_fill_holes(void)
{
    uint32_t lblock, f = 0;

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
    return 0;
}

/* phase E: the free pool is everything left, after one slot per log */
static int ftl_restore_build_pool(void)
{
    uint32_t sb, i, n;

    ftl_nlogs = 0;
    for (i = 0; i < FTL_MAX_LOGS; i++)
        if (ftl_rs_slots[i].lblock != 0xffff)
            ftl_logs[ftl_nlogs++] = ftl_rs_slots[i];
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
    return 0;
}

/* phase G: the newest erase counter, read counter and stats pages, walking
 * the control blocks newest first and each one backwards */
static void ftl_restore_side_pages(void)
{
    /* One flag per side-table slot. Static only to keep it off the mount's
     * stack; nothing outside this phase reads it. */
    static uint32_t seen[2 * FTL_CXT_PAGELIST + 1];
    uint32_t i, k, blk, order[3], nec;
    int type;

    memset(seen, 0, sizeof(seen));
    nec = (nsuperblocks * 2 + pagesize - 1) / pagesize;
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

/* phase I: what each log holds */
static void ftl_restore_scan_logs(void)
{
    uint32_t i;

    for (i = 0; i < ftl_nlogs; i++)
    {
        ftl_scan_log(&ftl_logs[i]);
        ftl_logs[i].pagesused = sbpages;
        ftl_logs[i].issequential = 0;
    }
}

static int ftl_restore(void)
{
    uint32_t maxusn;
    int ret;

    logf("ftl: restoring from the medium");
    memset(ftl_rs, 0, sizeof(ftl_rs));
    memset(ftl_hole, 0, sizeof(ftl_hole));
    ftl_nholes = 0;

    ret = ftl_restore_ctrl_blocks();            /* phase A */
    if (ret)
        return ret;
    maxusn = ftl_restore_classify();            /* phase B */
    ret = ftl_restore_place_logs(maxusn);       /* phase C */
    if (ret)
        return ret;
    ret = ftl_restore_fill_holes();             /* phase D */
    if (ret)
        return ret;
    ret = ftl_restore_build_pool();             /* phase E */
    if (ret)
        return ret;
    ftl_restore_side_pages();                   /* phase G */
    ftl_restore_scan_logs();                    /* phase I */
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
    uint32_t per = pagesize / 2;
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

/* Load the rest of what a commit has to reproduce: the erase and read
 * counters and the stats page. Failure is not fatal - the FTL stays mounted
 * read-only. */
static void ftl_open_write_state(void)
{
    uint32_t vpn;

    ftl_writable = false;
    ftl_nmappages = (usersb * 2 + pagesize - 1) / pagesize;
    ftl_necpages = (nsuperblocks * 2 + pagesize - 1) / pagesize;
    ftl_nrcpages = ftl_necpages;
    ftl_nlogpages = (sbpages * FTL_MAX_LOGS * 2 + pagesize - 1) / pagesize;

    if (ftl_necpages > FTL_SIDE_PAGES || ftl_nlogpages > FTL_LOG_PAGES
        || nsuperblocks > FTL_SIDE_ENTRIES)
        return;

    /* Lost counter tables read as zero, as the firmware's restore leaves
     * them */
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
        memcpy(ftl_stats, ftl_buffer, pagesize);

    /* A spare superblock is free or a log; more than 20 of them means the
     * context and the medium disagree, and we stay read-only */
    if (ftl_cxt.freecount + ftl_nlogs > FTL_POOL_SIZE)
    {
        logf("ftl: %lu free + %u logs > %d", (unsigned long)ftl_cxt.freecount,
             ftl_nlogs, FTL_POOL_SIZE);
        return;
    }
    ftl_writable = true;
}

/* ---- VFL write side: replacing blocks that have gone bad ----------- */
/* Ported from ftl-nano2g.c (ftl_vfl_store_cxt, ftl_vfl_commit_cxt,
 * ftl_vfl_remap_block). The Nano 3G keeps one remap table per plane rather
 * than a single table plus a bad-block bitmap, and has no bbt to maintain,
 * so the table update differs; everything else is the same scheme. */

/* Write 8 copies of a bank's VFL context; it counts as stored if at least 5
 * read back intact, as Apple's _StoreVFLCxt requires. */
static int ftl_vfl_store_cxt(uint32_t bank)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t meta[NAND_META_WORDS];
    uint32_t i, block, first, good = 0;

    first = cxt->nextcxtpage;
    cxt->updatecount--;
    cxt->usn = ++ftl_vfl_usn;
    cxt->nextcxtpage += 8;
    ftl_vfl_calculate_checksum(cxt, &cxt->checksum1, &cxt->checksum2);

    if (cxt->activecxtblock >= 4)
        return -1;
    block = cxt->vflcxtblocks[cxt->activecxtblock];
    if (block >= geo->blocks || cxt->nextcxtpage > geo->pagesperblock)
        return -1;

    /* Eight copies in page order, as Apple's _WriteVFLCxtPage writes them:
     * pages of an MLC block must be programmed in order. The first failed
     * program abandons this block. */
    meta[0] = cxt->updatecount;
    meta[1] = 0xffffffff;
    meta[2] = 0xffff0000 | (SPARE_VFL_CXT << 8);   /* byte 8 is 0 here */
    memset(ftl_cxtpage, 0xff, pagesize);
    memcpy(ftl_cxtpage, cxt, sizeof(*cxt));
    for (i = 0; i < 8; i++)
        if (nand_write_page(bank, block * geo->pagesperblock + first + i,
                            ftl_cxtpage, meta))
            return -1;
    for (i = 0; i < 8; i++)
    {
        if (ftl_raw_read(bank, block * geo->pagesperblock + first + i)
                != SPARE_VFL_CXT
            || (ftl_meta[2] & 0xff) != 0
            || ftl_meta[0] != cxt->updatecount
            || memcmp(ftl_buffer, cxt, offsetof(struct ftl_vfl_cxt,
                                                checksum1)))
            continue;
        good++;
    }
    /* Apple accepts a commit with at most three bad copies */
    return good >= 5 ? 0 : -1;
}

/* Commit a bank's VFL context, moving to another block of the context ring
 * if the current one is full or will not take it. */
static int ftl_vfl_commit_cxt(uint32_t bank)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t start = cxt->activecxtblock, i, j;

    /* The mount takes the control block list from whichever bank's context
     * is newest, and this commit is about to make one of them newest, so
     * every bank's copy has to agree first. Same as nano2g's
     * ftl_store_ctrl_block_list(). */
    for (i = 0; i < geo->banks; i++)
        memcpy(ftl_vfl_cxt[i].ftlctrlblocks, ftl_ctrl, sizeof(ftl_ctrl));

    if ((uint32_t)cxt->nextcxtpage + 8 <= geo->pagesperblock
        && !ftl_vfl_store_cxt(bank))
        return 0;

    i = start;
    while (1)
    {
        i = (i + 1) & 3;
        if (i == start)
            break;
        if (cxt->vflcxtblocks[i] >= geo->blocks)
            continue;
        for (j = 0; j < 4; j++)
            if (!nand_erase_block(bank, cxt->vflcxtblocks[i]))
                break;
        if (j == 4)
            continue;
        cxt->activecxtblock = i;
        cxt->nextcxtpage = 0;
        if (!ftl_vfl_store_cxt(bank))
            return 0;
    }
    /* The RAM remap table has already changed. Continuing after a failed
     * commit would use mappings that cannot survive a reboot. Stop here,
     * as ftl_vfl_commit_cxt() does on the Nano 2G. */
    panicf("VFL: Failed to commit VFL CXT on bank %lu!",
           (unsigned long)bank);
    return -1;
}

/* Take a physical block off a context's pending-bad list */
static void ftl_vfl_unpend(struct ftl_vfl_cxt *cxt, uint32_t block)
{
    uint32_t i, j;

    for (i = 0; i < cxt->pendingcount && i < FTL_VFL_PENDING; )
        if (cxt->pending[i] == block)
        {
            for (j = i; j + 1 < cxt->pendingcount && j + 1 < FTL_VFL_PENDING;
                 j++)
                cxt->pending[j] = cxt->pending[j + 1];
            cxt->pendingcount--;
        }
        else
            i++;
}

/* Replace a virtual block that has gone bad with one of the VFL's reserved
 * spares, and commit so the replacement survives a remount. The unit's own
 * table is used first, then any other with a free slot. */
static int ftl_vfl_remap_block(uint32_t bank, uint32_t unit, uint32_t vblock)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t block = ftl_unit_block(unit, vblock);
    uint32_t n = geo->planes * geo->vflspares;
    uint32_t i, k, slot = n, newblock;

    for (i = 0; i < n && slot == n; i++)
    {
        k = (unit * geo->vflspares + i) % n;
        if (cxt->remap[k] == FTL_VFL_FREE)
            slot = k;
    }
    if (slot == n)
    {
        logf("ftl: bank %lu is out of spare blocks", (unsigned long)bank);
        return -1;
    }
    newblock = ftl_vfl_spare_block(slot);

    for (i = 0; i < 9; i++)
        if (!nand_erase_block(bank, newblock))
            break;
    if (i == 9)
    {
        cxt->remap[slot] = FTL_VFL_BAD;     /* the spare itself is no good */
        cxt->badcount++;
        ftl_vfl_commit_cxt(bank);
        return -1;
    }
    /* Replaced now, so no longer pending: Apple's list holds the block as
     * it was addressed, found before its spare is retired below */
    ftl_vfl_unpend(cxt, ftl_vfl_phys_block(bank, unit, vblock));
    /* A spare that was already tried for this block and failed is unusable */
    for (k = 0; k < n; k++)
        if (cxt->remap[k] == block)
        {
            cxt->remap[k] = FTL_VFL_BAD;
            cxt->badcount++;
        }
    cxt->remap[slot] = block;
    cxt->usedcount[slot / geo->vflspares]++;
    logf("ftl: remapped bank %lu block %lu to spare %lu",
         (unsigned long)bank, (unsigned long)block, (unsigned long)newblock);
    return ftl_vfl_commit_cxt(bank);
}

/* Erase one virtual block, retrying and then replacing it if it will not
 * take an erase. nano2g tries three times before giving up on a block. A
 * block on the pending-bad list, put there by Apple's firmware after a
 * failed read or write, is replaced first, as the firmware's VFL erase does. */
static int ftl_vfl_erase(uint32_t bank, uint32_t unit, uint32_t vblock)
{
    const struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t block = ftl_vfl_phys_block(bank, unit, vblock);
    uint32_t i;

    for (i = 0; i < cxt->pendingcount && i < FTL_VFL_PENDING; i++)
        if (cxt->pending[i] == block)
        {
            if (ftl_vfl_remap_block(bank, unit, vblock))
                return -1;
            break;
        }

    for (i = 0; i < 3; i++)
        if (!nand_erase_block(bank, ftl_vfl_phys_block(bank, unit, vblock)))
            return 0;
    logf("ftl: erase of bank %lu unit %lu vblock %lu failed, remapping",
         (unsigned long)bank, (unsigned long)unit, (unsigned long)vblock);
    if (ftl_vfl_remap_block(bank, unit, vblock))
        return -1;
    for (i = 0; i < 3; i++)
        if (!nand_erase_block(bank, ftl_vfl_phys_block(bank, unit, vblock)))
            return 0;
    return -1;
}

/* Erase every physical block of a superblock and count it */
static int ftl_erase_sb(uint32_t sb)
{
    uint32_t bank, plane;
    int rc;

    if (sb >= nsuperblocks)
        return -1;
    for (bank = 0; bank < geo->banks; bank++)
        for (plane = 0; plane < geo->planes; plane++)
        {
            rc = ftl_vfl_erase(bank, plane, sb);
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

/* The usn a user data page is written with, as the firmware's write path
 * numbers them: one counter, advanced whenever a write starts a new
 * superblock, so
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
 * hands them to the controller the way the firmware's multi-page VFL write
 * does, so
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
 * the firmware's sequential write uses, when the chip allows it and no
 * block of the superblock is remapped - a spare is not the other plane's
 * twin. On
 * failure, *failbank and *failplanes say which blocks were being
 * programmed. */
static int ftl_run_program(uint32_t first, uint32_t n, bool rows,
                           uint32_t *failbank, uint32_t *failplanes)
{
    static struct nand_write w[FTL_MAX_SBPAGES];
    static uint16_t vs[FTL_MAX_SBPAGES];
    uint32_t perrow = geo->banks * geo->planes;
    uint32_t i, idx, bank, plane;
    bool twoplane = rows && geo->twoplane;
    int rc;

    for (bank = 0; twoplane && bank < geo->banks; bank++)
        for (plane = 0; plane < geo->planes; plane++)
            if (ftl_vfl_phys_block(bank, plane, ftl_run_sb)
                != ftl_unit_block(plane, ftl_run_sb))
                twoplane = false;

    for (i = 0; i < n; i++)
    {
        idx = first + i;
        if (twoplane)
        {
            /* Entry k of a row is bank k / planes, plane k % planes */
            uint32_t k = i % perrow;

            idx = first + i - k + (k % geo->planes) * geo->banks
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
                *failplanes |= 1 << ((vs[i] / geo->banks) % geo->planes);
    }
    return rc;
}

/* Program the queued run: pages up to the next row boundary on their own,
 * whole rows together while more than one row remains, and the rest on
 * their own, exactly as the firmware's multi-page VFL write splits it. */
static int ftl_run_write(bool remap_on_fail)
{
    uint32_t perrow = geo->banks * geo->planes;
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
        /* Only a fresh merge destination can be discarded and rebuilt.
         * Live logs and control blocks must retain their earlier pages. */
        if (!remap_on_fail)
            panicf("FTL: Write error: %lu %lu!",
                   (unsigned long)ftl_run_sb, (unsigned long)ftl_run_v);
        logf("ftl: write to sb %lu bank %lu failed (%d), remapping",
             (unsigned long)ftl_run_sb, (unsigned long)bank, rc);
        /* The blocks will not take programs any more. Replace them from the
         * VFL's spares now; whatever was already written to the superblock
         * is lost, so the caller has to start it again. */
        for (plane = 0; plane < geo->planes; plane++)
            if (planes & (1 << plane))
                ftl_vfl_remap_block(bank, plane, ftl_run_sb);
        return -1;
    }
    return 0;
}

/* Program one page on its own */
static int ftl_write_vpage(uint32_t sb, uint32_t v, const void *buf,
                           uint32_t word0, uint32_t word1, uint32_t type,
                           bool remap_on_fail)
{
    ftl_run_add(sb, v, buf, word0, word1, type);
    return ftl_run_write(remap_on_fail);
}

/* Take a block off the free ring, least-worn first. The firmware's free-block
 * take picks
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
     * restore, as the firmware's free-block return and its restore do */
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
        memset(dst, 0xff, pagesize);
        return 0;
    }
    ftl_vpage_phys(sb, v, &bank, &page);
    rc = nand_read_page(bank, page, dst, meta);
    if (rc < 0 || rc == NAND_ECC_FAILED)
        return -1;
    if (meta_type(meta) == SPARE_ERASED)
    {
        memset(dst, 0xff, pagesize);
        return 0;
    }
    if (!meta_is_data(meta) || meta[0] != lpn)
    {
        logf("ftl: merge read lpn %lx from sb %lu v %lu: type %lx lpn %lx",
             (unsigned long)lpn, (unsigned long)sb, (unsigned long)v,
             (unsigned long)meta_type(meta), (unsigned long)meta[0]);
        return -1;
    }
    return 0;
}

/* Read one prepared part of a run. Apple retries a failed multi-page read
 * page by page; keep the BootROM-derived single-page path for that retry. */
static int ftl_read_prepared(uint32_t first, uint32_t n,
                             const uint32_t *lpns)
{
    uint32_t i;
    int failed = nand_read_pages(ftl_read_run + first, n) != 0;

    for (i = first; i < first + n; i++)
    {
        struct nand_read *r = &ftl_read_run[i];
        uint32_t *meta = ftl_read_meta[i];

        if (failed || r->ecc == NAND_ECC_FAILED)
        {
            r->ecc = nand_read_page(r->bank, r->page, r->buf, meta);
            if (r->ecc < 0 || r->ecc == NAND_ECC_FAILED)
                return -1;
        }
        /* Corrected data is safe to return: the controller has repaired it.
         * TODO: Implement Apple's refresh list so persistently corrected
         * blocks are proactively rewritten. This is media maintenance, not
         * a loss of ECC protection compared with the earlier read path. */
        if (meta_type(meta) == SPARE_ERASED)
            memset(r->buf, 0xff, pagesize);
        else if (!meta_is_data(meta) || meta[0] != lpns[i])
        {
            logf("ftl: lpn %lx: spare says %lx type %lx",
                 (unsigned long)lpns[i], (unsigned long)meta[0],
                 (unsigned long)meta_type(meta));
            return -1;
        }
    }
    return 0;
}

/* Read current logical pages into consecutive buffers. With a log the
 * firmware uses one scattered VFL request. Without one, its multi-page VFL
 * read splits at a four-bank row: an unaligned head, whole rows only while
 * more than one row remains, then the final row/tail. */
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
            memset(dst + i * pagesize, 0xff, pagesize);
            continue;
        }
        ftl_vpage_phys(sb, v, &bank, &page);
        ftl_read_run[nr].bank = bank;
        ftl_read_run[nr].page = page;
        ftl_read_run[nr].buf = dst + i * pagesize;
        ftl_read_run[nr].meta = ftl_read_meta[nr];
        lpns[nr++] = lpn + i;
    }
    if (!nr)
        return 0;
    if (scattered)
        return ftl_read_prepared(0, nr, lpns);

    v = lpn % sbpages;
    head = MIN(nr, geo->banks - v % geo->banks);
    if (head == geo->banks)
        head = 0;
    if (head && ftl_read_prepared(0, head, lpns))
        return -1;
    i = head;
    middle = nr - i > geo->banks
           ? ((nr - i - 1) / geo->banks) * geo->banks : 0;
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
 * not. New contents for the whole block go in one run, as the firmware's write
 * path writes
 * them; a copy goes an eighth of the block at a time, read and then
 * written, as the firmware's simple merge does. */
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
            new_data = ftl_copybuf + n * pagesize;
            if (!src && n == 0
                && ftl_read_pages(lpn, MIN(sbpages - v, sbpages / 8),
                                  ftl_copybuf))
            {
                ftl_run_n = 0;
                return -1;
            }
            if (src && ftl_read_current(lpn, ftl_copybuf + n * pagesize))
            {
                ftl_run_n = 0;
                return -1;
            }
        }
        ftl_run_add(sb, v, new_data, lpn, ftl_data_usn(v),
                    v == sbpages - 1 ? SPARE_DATA_LAST : SPARE_DATA);
        if (!whole && ++n == sbpages / 8)
        {
            if (ftl_run_write(true))
                return -1;
            n = 0;
        }
    }
    return ftl_run_write(true);
}

static int ftl_merge(uint32_t lblock, ftl_write_src src, void *ctx,
                     bool worn_first)
{
    uint32_t sb, old;
    int attempt;

    if (lblock >= usersb)
        return -1;
    old = ftl_map[lblock];

    /* Up to four attempts, each in a different superblock. A block that
     * refused the write has been remapped by ftl_run_write() by the time
     * we get here, so the next attempt gets fresh silicon.
     * ftl_commit_scattered() in ftl-nano2g.c retries the same way. */
    for (attempt = 0; attempt < 4; attempt++)
    {
        if (ftl_pool_alloc(&sb, worn_first))
        {
            logf("ftl: free pool empty");
            return -1;
        }
        if (!ftl_fill_block(lblock, sb, src, ctx))
            break;
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
     * the firmware's copy merge does */
    for (v = log->pagesused; v < sbpages; v += n)
    {
        n = MIN(sbpages - v, sbpages / 8);
        if (ftl_read_pages(lblock * sbpages + v, n, ftl_copybuf))
            return ftl_commit_scattered(log);
        for (i = 0; i < n; i++)
        {
            const uint8_t *buf = ftl_copybuf + i * pagesize;

            ftl_run_add(sb, v + i, buf, lblock * sbpages + v + i,
                        ftl_data_usn(v + i),
                        v + i == sbpages - 1 ? SPARE_DATA_LAST : SPARE_DATA);
        }
        if (ftl_run_write(false))
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
 * release the old one, as the firmware's move merge and
 * ftl_compact_scattered() in ftl-nano2g.c do. A log that has filled up
 * mostly with superseded copies of
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
            ftl_read_run[n].buf = ftl_copybuf + n * pagesize;
            ftl_read_run[n].meta = ftl_read_meta[n];
            lpns[n] = lpn;
            offsets[idx] = v + n;
            n++;
        }
        /* Up to an eighth of a block of live pages at a time, as
         * the firmware's move merge gathers them */
        if (n == sbpages / 8 || (n && idx == sbpages - 1))
        {
            if (ftl_read_prepared(0, n, lpns))
                break;
            for (i = 0; i < n; i++)
            {
                const uint8_t *buf = ftl_copybuf + i * pagesize;
                ftl_run_add(sb, v, buf, lpns[i], ftl_data_usn(v),
                            SPARE_DATA);
                v++;
            }
            if (ftl_run_write(true))
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

/* Free a log slot, as the firmware's merge and ftl_remove_scattered_block() in
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
     * the firmware's log preparation merges a victim when only three free
     * blocks remain */
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
    /* Its recency, for choosing a victim, as the firmware's log preparation
     * keeps it */
    log->usn = ftl_cxt.maxusn - 1;
    if (!log->usn)
        for (i = 0; i < ftl_nlogs; i++)
            ftl_logs[i].usn = 0;
    return log;
}

/* Append one page to a log, as ftl_write() in ftl-nano2g.c does. The page
 * is only queued: the firmware's write path hands everything it appends to a
 * log to the VFL
 * in one run, and ftl_run_write() must be called before anything else
 * touches the flash. */
static void ftl_log_append(struct ftl_log *log, uint32_t idx,
                          const void *data)
{
    uint32_t v = log->pagesused;
    uint32_t lpn = log->lblock * sbpages + idx;
    uint32_t type = SPARE_DATA;

    if (v == sbpages - 1 && log->issequential)
        type = SPARE_DATA_LAST;
    ftl_run_add(log->sb, v, data, lpn, ftl_data_usn(v), type);
    log->pagesused++;
    if (log->offsets[idx] == FTL_NO_PAGE)
        log->pagescurrent++;
    log->offsets[idx] = v;
    if (log->pagesused != log->pagescurrent || log->offsets[idx] != idx)
        log->issequential = 0;
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
 * reused in place, exactly as the firmware does - no pool block and
 * no VFL commit is needed for this. */
static int ftl_ctrl_reserve(uint32_t n, uint32_t *sb, uint32_t *v)
{
    if (n > sbpages)
        return -1;
    if (ftl_ctrl_v + n > sbpages)
    {
        uint32_t next = (ftl_ctrl_idx + 1) % 3;

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
 * the firmware's mark-context-invalid step does. A mount, ours or the
 * firmware's, then finds a 0x4f page
 * after the context and runs the restore, instead of trusting a context that
 * predates the writes. */
static int ftl_mark_dirty(void)
{
    uint32_t sb, v;

    if (!ftl_cxt.cleanflag)
        return 0;                       /* already marked */
    if (ftl_ctrl_reserve(1, &sb, &v))
        return -1;
    memset(ftl_buffer, 0xff, pagesize);
    if (ftl_write_vpage(sb, v, ftl_buffer, ftl_cxt.usn - 1, 0xffffffff,
                        SPARE_FTL_MOUNT, false))
        return -1;
    ftl_cxt.cleanflag = 0;
    return 0;
}

/* Write the 25-page commit: erase counters, read counters, map, log tables,
 * stats, then the context, all carrying the same usn, in Apple's order, as
 * the firmware's context store does. The open logs are recorded in the
 * context's log slots
 * and the offset tables, so a commit need not fold them. */
static int ftl_commit(void)
{
    uint32_t per = pagesize / 2;
    uint32_t npages, usn, sb, v0, i;
    uint32_t ecv, rcv, mapv, logv, statsv, cxtv;

    npages = FTL_COMMIT_PAGES(ftl_necpages, ftl_nrcpages, ftl_nmappages,
                              ftl_nlogpages);
    if (npages > sbpages || npages * pagesize > sizeof(ftl_copybuf))
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

    /* The whole commit goes to the flash as one run, as the firmware's context
     * store writes
     * it; the table pages are staged in ftl_copybuf until then */
    for (i = 0; i < ftl_necpages; i++)
    {
        uint8_t *buf = ftl_copybuf + (ecv - v0 + i) * pagesize;

        memset(buf, 0xff, pagesize);
        memcpy(buf, ftl_erasectr + i * per,
               MIN(nsuperblocks - i * per, per) * 2);
        ftl_run_add(sb, ecv + i, buf, usn, 0xffff0000 | i,
                    SPARE_FTL_ERASECTR);
        ftl_cxt.ecpages[i] = ftl_vpn(sb, ecv + i);
    }
    for (i = 0; i < ftl_nrcpages; i++)
    {
        uint8_t *buf = ftl_copybuf + (rcv - v0 + i) * pagesize;

        memset(buf, 0xff, pagesize);
        memcpy(buf, ftl_readcount + i * per,
               MIN(nsuperblocks - i * per, per) * 2);
        ftl_run_add(sb, rcv + i, buf, usn, 0xffff0000 | i,
                    SPARE_FTL_READCOUNT);
        ftl_cxt.rcpages[i] = ftl_vpn(sb, rcv + i);
    }
    for (i = 0; i < ftl_nmappages; i++)
    {
        uint8_t *buf = ftl_copybuf + (mapv - v0 + i) * pagesize;

        memset(buf, 0xff, pagesize);
        memcpy(buf, ftl_map + i * per, MIN(usersb - i * per, per) * 2);
        ftl_run_add(sb, mapv + i, buf, usn, 0xffff0000 | i, SPARE_FTL_MAP);
        ftl_cxt.mappages[i] = ftl_vpn(sb, mapv + i);
    }
    /* One offset table per log slot, in slot order */
    memset(ftl_logtable, 0xff, sizeof(ftl_logtable));
    for (i = 0; i < ftl_nlogs; i++)
        memcpy(ftl_logtable + i * sbpages, ftl_logs[i].offsets, sbpages * 2);
    for (i = 0; i < ftl_nlogpages; i++)
    {
        ftl_run_add(sb, logv + i, ftl_logtable + i * per, usn, 0xffff0000 | i,
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
    memset(ftl_cxtpage, 0xff, pagesize);
    memcpy(ftl_cxtpage, &ftl_cxt, sizeof(ftl_cxt));
    ftl_run_add(sb, cxtv, ftl_cxtpage, usn, 0xffffffff, SPARE_FTL_CXT);
    if (ftl_run_write(false))
        return -1;

    ftl_cxt.usn = usn;
    return 0;
}

/* src for a plain contiguous write of sectors. On pages larger than a
 * sector, a page the write covers only part of comes from part, holding
 * the page's current contents with the new sectors laid over them. */
struct ftl_range
{
    uint32_t first;             /* sectors */
    uint32_t count;
    const uint8_t *buf;
    uint32_t partlpn;           /* the page in part, or 0xffffffff */
    const uint8_t *part;
};

static const void *ftl_range_src(uint32_t lpn, void *ctx)
{
    const struct ftl_range *r = ctx;
    uint64_t sector = (uint64_t)lpn * secperpage;

    if (lpn == r->partlpn)
        return r->part;
    if (sector < r->first || sector - r->first >= r->count)
        return NULL;
    return r->buf + (size_t)(sector - r->first) * NAND_PAGE_SIZE;
}

/* The single failure path of the write side. Every failure below leaves the
 * medium itself consistent - the committed map still points at whatever was
 * there - but our pool or log accounting may no longer match it, so the
 * mount drops to read-only and the caller is told which step failed.
 * Returns the code, so a failure site is one statement. */
static int ftl_write_fail(int code)
{
    ftl_writable = false;
    return code;
}

/* One logical block of ftl_write_merge(): mark the context dirty, then
 * either rewrite the block whole or append the changed pages to a log. */
static int ftl_write_block(uint32_t lblock, ftl_write_src src, void *ctx)
{
    struct ftl_log *log;
    uint32_t idx;
    int ret = 0;

    /* Anything we leave in a log block is invisible to the committed map,
     * so the context has to say so before a page is written. Once per block
     * rather than once per call: the commit below leaves the context clean
     * again, and the blocks after it need marking too. */
    if (ftl_mark_dirty())
        return ftl_write_fail(-5);

    /* A whole block at once needs no log and costs no amplification */
    if (ftl_block_complete(lblock, src, ctx))
    {
        if (ftl_merge(lblock, src, ctx, false))
            return ftl_write_fail(-3);
        return 0;
    }

    log = ftl_allocate_log_entry(lblock);
    if (!log)
        return ftl_write_fail(-6);
    for (idx = 0; idx < sbpages && !ret; idx++)
    {
        const void *data = src(lblock * sbpages + idx, ctx);

        if (!data)
            continue;
        if (log->pagesused == sbpages)
        {
            /* Full: fold it away and start another. These two cannot
             * return straight away - the run queued above still has to be
             * flushed below before we leave. */
            ftl_run_write(false);
            if (ftl_remove_scattered_block(log))
            {
                ret = ftl_write_fail(-7);
                break;
            }
            log = ftl_allocate_log_entry(lblock);
            if (!log)
            {
                ret = ftl_write_fail(-6);
                break;
            }
        }
        ftl_log_append(log, idx, data);
    }
    /* A failed program panics: the log's earlier pages cannot be
     * rebuilt, as for a single page in ftl-nano2g.c */
    ftl_run_write(false);
    if (ret)
        return ret;
    if (log->pagesused == sbpages && ftl_remove_scattered_block(log))
        return ftl_write_fail(-7);
    return 0;
}

/* Write the pages src() has new contents for, in every logical block
 * between first and last. Blocks it has nothing for are left alone. */
static int ftl_write_merge(uint32_t first, uint32_t last, ftl_write_src src,
                           void *ctx)
{
    uint32_t lblock;
    int ret = 0;

    if (!ftl_mounted || !ftl_writable || !src)
        return -1;
    if (last < first || last >= usersb * sbpages)
        return -2;

    mutex_lock(&ftl_mtx);
    for (lblock = first / sbpages; lblock <= last / sbpages; lblock++)
    {
        if (!ftl_block_dirty(lblock, src, ctx))
            continue;
        ret = ftl_write_block(lblock, src, ctx);
        if (ret)
            break;
    }
    /* Static wear levelling on the budget the firmware's write path and
     * ftl-nano2g.c's ftl_write() use: every merge adds one, and at 300 one
     * cold block is moved. There is no commit here - like Apple's firmware,
     * ftl_sync() alone commits, and an unclean mount recovers the rest. */
    if (!ret && ftl_cxt.swapcounter >= 300)
    {
        ftl_cxt.swapcounter -= 20;
        if (ftl_wear_level())
            ret = ftl_write_fail(-4);
    }
    mutex_unlock(&ftl_mtx);
    return ret;
}

/* Write sectors that all lie in the page lpn and do not fill it: the rest
 * of the page is read back and written with them */
static int ftl_write_part(uint32_t lpn, uint32_t sector, uint32_t count,
                          const uint8_t *buf)
{
    struct ftl_range r = { sector, count, buf, lpn, ftl_sectorpage };
    int rc;

    mutex_lock(&ftl_mtx);
    rc = ftl_read_pages(lpn, 1, ftl_sectorpage);
    mutex_unlock(&ftl_mtx);
    if (rc)
        return -3;
    memcpy(ftl_sectorpage + (sector - lpn * secperpage) * NAND_PAGE_SIZE,
           buf, count * NAND_PAGE_SIZE);
    return ftl_write_merge(lpn, lpn, ftl_range_src, &r);
}

int ftl_write(uint32_t sector, uint32_t count, const void *buffer)
{
    struct ftl_range r = { sector, count, buffer, 0xffffffff, NULL };
    const uint8_t *buf = buffer;
    uint32_t n, first, last;
    int rc;

    if (!count)
        return 0;
    if (sector + count > ftl_num_sectors() || sector + count < sector)
        return -2;

    /* A page only partly covered at either end, then the whole pages */
    if (sector % secperpage)
    {
        n = MIN(count, secperpage - sector % secperpage);
        rc = ftl_write_part(sector / secperpage, sector, n, buf);
        if (rc)
            return rc;
        sector += n;
        count -= n;
        buf += n * NAND_PAGE_SIZE;
    }
    if (count % secperpage)
    {
        n = count % secperpage;
        rc = ftl_write_part((sector + count - n) / secperpage,
                            sector + count - n, n,
                            buf + (count - n) * NAND_PAGE_SIZE);
        if (rc)
            return rc;
        count -= n;
    }
    if (!count)
        return 0;
    first = sector / secperpage;
    last = (sector + count) / secperpage - 1;
    r.first = sector;
    r.count = count;
    r.buf = buf;
    return ftl_write_merge(first, last, ftl_range_src, &r);
}

/* The commit sequence, with ftl_mtx already held. */
static int ftl_sync_locked(void)
{
    int ret = 0;

    /* Fold every log block away - the ones we filled this session and any
     * left by whoever wrote the NAND last - so the commit can say there are
     * none and the free pool is whole again, as the firmware's full flush and
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
    uint32_t n, lpn, off;
    int ret = 0;

    if (!ftl_mounted)
        return -1;
    if (!count)
        return 0;
    if (sector + count > ftl_num_sectors() || sector + count < sector)
        return -2;

    mutex_lock(&ftl_mtx);
    while (count)
    {
        lpn = sector / secperpage;
        off = sector % secperpage;
        if (off || count < secperpage)
        {
            /* Part of a page, through a page-sized buffer */
            n = MIN(count, secperpage - off);
            if (ftl_read_pages(lpn, 1, ftl_sectorpage))
            {
                ret = -3;
                break;
            }
            memcpy(buf, ftl_sectorpage + off * NAND_PAGE_SIZE,
                   n * NAND_PAGE_SIZE);
        }
        else
        {
            n = MIN(count / secperpage, sbpages - lpn % sbpages);
            if (ftl_read_pages(lpn, n, buf))
            {
                ret = -3;
                break;
            }
            n *= secperpage;
        }
        sector += n;
        count -= n;
        buf += n * NAND_PAGE_SIZE;
    }
    mutex_unlock(&ftl_mtx);
    return ret;
}

bool ftl_readonly_mount(void)
{
    /* Anything but a mounted, writable FTL takes no writes: a predicate
     * that guards them answers the safe way when nothing is mounted. */
    return !ftl_mounted || !ftl_writable;
}

uint32_t ftl_num_sectors(void)
{
    return ftl_mounted ? usersb * sbpages * secperpage : 0;
}

/* The geometry the mount works in, from the chip table row */
static int ftl_setup_geometry(void)
{
    geo = nand_get_geometry();
    if (!geo || geo->banks == 0 || geo->banks > FTL_MAX_BANKS)
        return -1;
    if (geo->layout < NAND_LAYOUT_SINGLE || geo->layout > NAND_LAYOUT_SPLIT13
        || geo->planes < 1 || geo->planes > 4
        || geo->pagesize < NAND_PAGE_SIZE
        || geo->pagesize > NAND_MAX_PAGE_SIZE
        || geo->pagesize % NAND_PAGE_SIZE)
        return -4;
    pagesize = geo->pagesize;
    secperpage = pagesize / NAND_PAGE_SIZE;
    /* Apple's VFL reserves each unit's top vflspares virtual blocks; below
     * them are the user blocks, 3 FTL control blocks and a pool of 20 */
    nsuperblocks = geo->blocks / geo->planes - geo->vflspares;
    sbpages = geo->pagesperblock * geo->planes * geo->banks;
    usersb = geo->userblocks / geo->planes;
    if (nsuperblocks > FTL_MAX_SB || usersb > FTL_MAX_USERSB
        || usersb > nsuperblocks || sbpages > FTL_MAX_SBPAGES
        || sbpages * pagesize > FTL_MAX_SBPAGES * NAND_PAGE_SIZE
        || geo->planes * geo->vflspares > 820)
        return -2;
    return 0;
}

int ftl_init(void)
{
    uint32_t bank;
    bool restored;
    int rc;

    ftl_mounted = false;
    mutex_init(&ftl_mtx);
    ftl_nholes = 0;

    rc = ftl_setup_geometry();
    if (rc)
        return rc;

    for (bank = 0; bank < geo->banks; bank++)
    {
        if (ftl_vfl_open_bank(bank))
        {
            logf("ftl: no VFL context on bank %lu", (unsigned long)bank);
            return -3;
        }
        if (ftl_vfl_check_cxt(&ftl_vfl_cxt[bank]))
        {
            logf("ftl: VFL context on bank %lu does not fit the chip",
                 (unsigned long)bank);
            return -5;
        }
    }

    for (bank = 0; bank < geo->banks; bank++)
        if (ftl_vfl_cxt[bank].usn > ftl_vfl_usn)
            ftl_vfl_usn = ftl_vfl_cxt[bank].usn;

    /* Apple's FTL_Open: the context if it describes the medium, else
     * the restore */
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
    ftl_open_write_state();
#ifdef BOOTLOADER
    /* Bootloaders don't need write access, as on the Nano 2G */
    ftl_writable = false;
#endif
    /* A chip whose row has not been proven on hardware is only read */
    if (!geo->validated)
        ftl_writable = false;
    ftl_readonly = !ftl_writable;

    /* The rest of the restore, which writes: erase the blocks given to
     * holes (phase D) and the free pool (H), then commit (J) */
    if (restored && ftl_writable)
    {
        uint32_t i, lblock;

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
        }
    }

    ftl_mounted = true;
    return 0;
}
