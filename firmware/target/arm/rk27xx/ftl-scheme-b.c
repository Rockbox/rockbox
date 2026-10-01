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

/* THE SCHEME B FLASH TRANSLATION LAYER
 *
 * Later rk27xx players - the HiFiMAN HM-601 among them - keep their NAND in
 * a different format from Scheme A's (ftl-scheme-a.c): one where every
 * block says what it holds, so the whole mapping is rebuilt by scanning at
 * every mount and there is no table to keep consistent. This file reads and
 * writes that format as the original firmware does, so a device keeps
 * working with its original firmware after Rockbox has written to it.
 *
 * It is a reimplementation from reverse engineering. The format and the
 * behaviour were worked out by analysing the machine code of the HM-601's
 * NAND bootloader and of a compiled Rockchip FTL object from the rk2808
 * platform, which handles the same format, and checked against dumps of
 * the media; no source code was used. Where the two binaries differ, the
 * HM-601's behaviour is followed and the place is marked. "The original"
 * below means that observed behaviour. Where it is odd, it is reproduced on
 * purpose and marked as such: the order in which blocks are used and pages
 * cached is part of what is checked against it.
 *
 *
 * BLOCKS
 *
 * The unit of mapping is the SUPER-BLOCK - on a two-plane chip, a physical
 * block from each plane, used as one (flash-rk27xx.h) - and the logical
 * space is a run of logical blocks of that size. The blocks before
 * config.first_block are the boot area. Then come the FTL's system blocks,
 * the last of them named by the bad-block table, then the data area.
 *
 *
 * METADATA
 *
 * The original firmware sees 16 spare bytes per sector and keeps one 16-bit
 * FIELD in the first two of each; of a page, the field of sector k is field
 * k. rk27 media holds the first three bytes of each sector's spare, so a
 * field is metadata bytes 0-1, and byte 2 is used only where the original
 * stores 32-bit values. Field 0 of every page is its TAG:
 *
 *   0xf200  data           1 version, 2 logical block, 3 source block,
 *                          4 the block itself
 *   0xf100  cache journal  1 journal version, 2-3 first sector of the page
 *                          (32 bits: field 2 and byte 2 of sector 2 hold the
 *                          low 24, field 3 the high 16), 4-5 the 32-bit mask
 *                          of sectors the page holds
 *   0xf000  bad-block table
 *   0xf300  power-loss record
 *   0xf800  page remap     not used on the devices seen; such blocks are
 *                          freed
 *   0xffff  erased
 *
 * A data block carries its header on every page, so the last page written
 * can be found by binary search.
 *
 *
 * THE MOUNT
 *
 * The bad-block table (two mirror blocks at the start of the FTL's area)
 * gives the bad blocks, the logical block count, the format version and the
 * system blocks. Then page 0 of every block of the data area is read: a data
 * block claims its logical block, and of two claimants the newer version
 * wins. The (source, destination) pair of the winner is remembered: a write
 * may have been under way. A destination block that is not full is an
 * EXCHANGE BLOCK still being written; it is reopened, its unwritten tail
 * still to come from the source. Blocks claimed by nothing are free.
 *
 * Versions are 16 bits and compared allowing for wrap-around, as the HM-601
 * does; the rk2808 object compares 24 bits including the tag's low byte,
 * which agrees on every device seen.
 *
 *
 * WRITING
 *
 * A write to a logical block goes to an erased block - an exchange block:
 * the pages before the write are copied from the old block, the new pages
 * written after them, and the exchange stays open for following pages
 * until it is full or its slot is needed; closing it copies the rest of the
 * old block. Up to config.exch_blocks are open at once, reused least
 * recently used first. The old block is then free, erased when it is next
 * used.
 *
 * A copy puts on every page it programs one header built from the source
 * block's page 0 - version + 1, the source block, the destination block -
 * as the HM-601 does (the rk2808 object keeps each page's own spare).
 *
 *
 * THE WRITE CACHE
 *
 * Writes of less than a page, and of single pages, go to a cache of 16
 * pages in RAM, each with a mask of the sectors it holds. Every change to a
 * cached page is also written to the CACHE JOURNAL - up to 16 blocks of
 * 0xf100 pages - so nothing is held only in RAM: the mount replays the
 * newest 16 distinct pages of the journal into the cache. A page leaves the
 * cache through an exchange block, the gaps in it read from the flash first,
 * and its journal entry is then cancelled by a copy with an empty mask.
 *
 *
 * NOT IMPLEMENTED
 *
 * - The low-level format: media without a bad-block table does not mount.
 * - The page remap (0xf800) and the paging of the remap table to flash for
 *   more than 32768 logical blocks; neither is used on the devices seen.
 * - Bad-block replacement after a failed program or erase, and refreshing
 *   blocks the ECC reports decaying.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flash-rk27xx.h"
#include "ftl-scheme-b.h"

/* ---- layout ---- */

#define MAX_BLOCKS          8192    /* super-blocks on the chip */
#define MAX_LOGICAL_BLOCKS  8192
#define MAX_EXCH_BLOCKS     32      /* as many as the rk2808 object keeps */
#define CACHE_PAGES         16
#define JOURNAL_BLOCKS      16
#define BBT_SCAN_BLOCKS     50      /* blocks the bad-block table may be in */
#define BBT_MAX             0x400
#define FREE_QUEUE_MIN      0x80
#define FREE_QUEUE_MAX      0x400
#define RCV_PAIRS           0x400   /* (source, destination) pairs kept */
#define RECHECK_PAGES       6       /* last pages of an exchange read back */
#define SEEN_MAX            2048    /* journal pages a mount looks at */
#define MIN_SEC_PER_PAGE    8       /* a journal page needs fields 0-7 */

#define PAGE_BYTES          (FLASH_MAX_SEC_PER_PAGE * FLASH_SECTOR_SIZE)
#define PAGE_META_BYTES     (FLASH_MAX_SEC_PER_PAGE * FLASH_META_SIZE)

/* The original's write path decides between the cache and an exchange
 * block on fixed sector counts, whatever the page size */
#define CACHE_FIRST_MAX     16      /* a write up to this long keeps its first
                                     * page in the cache */
#define CACHE_WRITE_MAX     15      /* ... and its whole pages too */

/* ---- metadata ---- */

#define TAG_TABLE           0xf000
#define TAG_CACHE           0xf100
#define TAG_DATA            0xf200
#define TAG_POWER           0xf300
#define TAG_PAGE_REMAP      0xf800
#define TAG_KIND            0xff00
#define FIELD_ERASED        0xffff

/* data page */
#define F_TAG               0
#define F_VER               1
#define F_LBLOCK            2
#define F_SRC               3
#define F_SELF              4
/* cache journal page */
#define F_LBA_LO            2
#define F_LBA_HI            3
#define F_MASK_LO           4
#define F_MASK_HI           5
#define JOURNAL_FIELDS      8
/* bad-block table page */
#define F_LOGICAL           2
#define F_MIRROR_A          4
#define F_MIRROR_B          5
#define F_POWER_BLOCK       6
#define F_LAST_SYS_BLOCK    7
#define BBT_FIELDS          8
#define FORMAT_VERSION_MAJOR 2

/* power-loss record: three little-endian words of the page's first sector */
#define PWR_MODE_DONE       0x55530000
#define PWR_MODE_COPY       0x55531111

#define DATA_HEADER_FIELDS  5
#define HEADER_PAD          0xff    /* what the unused metadata bytes get */

#define NO_SLOT             0xff
#define NO_JOURNAL          0xffffffff

/* system blocks named by the bad-block table */
#define SYS_MIRROR_A        0
#define SYS_MIRROR_B        1
#define SYS_POWER           2
#define SYS_LAST            3
#define SYS_BLOCKS          4

/* ---- state ---- */

/* A logical block being rewritten into an exchange block */
struct exch_block
{
    uint32_t start;     /* next sector to write in the exchange block */
    uint32_t end;       /* the sector after the exchange block */
    uint32_t src;       /* first sector of the block being replaced */
    uint16_t ver;       /* version of the exchange block's header */
    uint8_t  age;       /* exch_count - 1 = used most recently */
    bool     open;
};

/* A page of the write cache */
struct cache_page
{
    uint32_t lba;       /* first sector of the page */
    uint32_t mask;      /* the sectors held */
    uint8_t  age;       /* 15 = used most recently */
    bool     valid;
};

/* The blocks waiting to be used, oldest first. A block is only ever in it
 * once, and one queued when it is full is dropped - so a mount that finds
 * more free blocks than the queue holds leaves the rest unused until the
 * next mount, as the original does. */
struct free_queue
{
    uint16_t max;
    uint16_t front;
    uint16_t rear;
    uint16_t count;
    uint16_t blk[FREE_QUEUE_MAX];
};

static struct
{
    const struct flash_geometry *geo;
    struct ftl_b_config config;
    bool     mounted;
    bool     writable;
    enum ftl_b_error error;

    uint16_t total_blocks;
    uint16_t logical_blocks;
    uint16_t format_version;
    uint16_t sys_block[SYS_BLOCKS];
    uint16_t bbt_page;          /* sector of the newest table page, in block */
    uint16_t bad_count;
    uint16_t bad[BBT_MAX];

    uint16_t remap[MAX_LOGICAL_BLOCKS];     /* 0 = not mapped */
    struct free_queue free;
    struct exch_block exch[MAX_EXCH_BLOCKS];

    /* the cache journal: its blocks, oldest first, the one being written
     * and the next sector in it */
    uint16_t journal_blk[JOURNAL_BLOCKS];
    int8_t   journal_slot;      /* -1 = no journal yet */
    uint32_t journal_page;      /* NO_JOURNAL = the block is full */
    uint16_t journal_ver;

    struct cache_page cache[CACHE_PAGES];
    /* the valid cache pages by ascending LBA, NO_SLOT-terminated */
    uint8_t  order[CACHE_PAGES + 1];
} ftl;

static uint8_t cache_buf[CACHE_PAGES][PAGE_BYTES] __attribute__((aligned(4)));

/* One page of scratch: the bad-block table as read, the journal pages a
 * mount has seen. */
static uint8_t page_buf[PAGE_BYTES] __attribute__((aligned(4)));

/* ---- geometry helpers ---- */

static uint8_t sec_per_page(void)
{
    return ftl.geo->sec_per_page;
}

static uint16_t sec_per_block(void)
{
    return ftl.geo->sec_per_block;
}

static uint32_t block_sector(uint16_t blk)
{
    return (uint32_t)blk * sec_per_block();
}

static uint16_t sector_block(uint32_t sec)
{
    return (uint16_t)(sec / sec_per_block());
}

/* The sectors of a page from sector `sec`: every bit up to the page size */
static uint32_t page_mask(uint32_t n)
{
    return n >= 32 ? 0xffffffff : ((uint32_t)1 << n) - 1;
}

/* ---- metadata helpers ---- */

static uint16_t field(const uint8_t *meta, unsigned k)
{
    return (uint16_t)(meta[k * FLASH_META_SIZE]
                      | (meta[k * FLASH_META_SIZE + 1] << 8));
}

static void set_field(uint8_t *meta, unsigned k, uint16_t v)
{
    meta[k * FLASH_META_SIZE]     = (uint8_t)v;
    meta[k * FLASH_META_SIZE + 1] = (uint8_t)(v >> 8);
}

/* The original stores a 32-bit word in the slot: the low three bytes reach
 * the media. */
static void set_word(uint8_t *meta, unsigned k, uint32_t v)
{
    meta[k * FLASH_META_SIZE]     = (uint8_t)v;
    meta[k * FLASH_META_SIZE + 1] = (uint8_t)(v >> 8);
    meta[k * FLASH_META_SIZE + 2] = (uint8_t)(v >> 16);
}

/* v is a later version than `than`. The HM-601's comparison: plain, or with
 * both shifted half the range, so that a wrapped version still wins. */
static bool ver_newer(uint16_t v, uint16_t than)
{
    return than < v || (uint16_t)(than + 0x8000) < (uint16_t)(v + 0x8000);
}

/* ---- free blocks ---- */

static void queue_in(uint16_t blk)
{
    struct free_queue *q = &ftl.free;
    bool queued = q->count == q->max;
    uint16_t i;

    for (i = 0; i < q->count && !queued; i++)
    {
        queued = q->blk[(q->front + i) % q->max] == blk;
    }
    if (!queued)
    {
        q->blk[q->rear] = blk;
        q->count++;
        q->rear = (uint16_t)((q->rear + 1) % q->max);
    }
}

/* Returns 0 when there is none: block 0 is the boot area's. */
static uint16_t queue_out(void)
{
    struct free_queue *q = &ftl.free;
    uint16_t blk = 0;

    if (q->count > 0)
    {
        q->count--;
        blk = q->blk[q->front];
        q->front = (uint16_t)((q->front + 1) % q->max);
    }
    return blk;
}

static bool block_is_bad(uint16_t blk)
{
    bool bad = false;
    uint16_t i;

    for (i = 0; i < ftl.bad_count && !bad; i++)
    {
        bad = ftl.bad[i] == blk;
    }
    return bad;
}

/* ---- mapping ---- */

/* Map logical block `lb` to `blk`, unless blk is 0 or past the chip. Returns
 * the previous mapping; 0 for a logical block out of range. */
static uint16_t remap_set(uint16_t lb, uint16_t blk)
{
    uint16_t old = 0;

    if (lb < ftl.logical_blocks)
    {
        old = ftl.remap[lb];
        if (blk != 0 && blk < ftl.total_blocks)
        {
            ftl.remap[lb] = blk;
        }
    }
    return old;
}

/* The logical block stored in the block holding sector `sec`;
 * logical_blocks if none. */
static uint16_t remap_owner(uint32_t sec)
{
    uint16_t blk = sector_block(sec);
    uint16_t lb;

    for (lb = 0; lb < ftl.logical_blocks && ftl.remap[lb] != blk; lb++)
    {
    }
    return lb;
}

/* The sector logical sector `lba` is mapped to. A logical block never
 * written is given a free block here - even to be read, as the original
 * does; its content is then whatever that block holds. With `current`, a
 * sector of an exchange block not yet written is found in the source block
 * instead. */
static uint32_t remap_sector(uint32_t lba, bool current)
{
    uint16_t lb = sector_block(lba);
    uint16_t blk = remap_set(lb, 0);
    uint32_t sec;
    uint8_t i;

    if (blk == 0)
    {
        blk = queue_out();
        remap_set(lb, blk);
    }
    sec = block_sector(blk) + lba % sec_per_block();

    for (i = 0; current && i < ftl.config.exch_blocks; i++)
    {
        const struct exch_block *e = &ftl.exch[i];

        if (e->open && e->start <= sec && sec < e->end)
        {
            sec = e->src + sec % sec_per_block();
            break;
        }
    }
    return sec;
}

/* ---- copying ---- */

/* Copy n sectors as the HM-601 does: if the source block is a data block,
 * every page programmed gets the header of the source's page 0 with the
 * version advanced and the source and destination blocks filled in; any
 * other block is copied with each sector's own metadata. */
static void copy_sectors(uint32_t src, uint32_t dst, uint32_t n)
{
    uint8_t meta[PAGE_META_BYTES];

    if (n > 0)
    {
        flash_read(src - src % sec_per_block(), NULL, meta, sec_per_page());

        if (field(meta, F_TAG) == TAG_DATA)
        {
            set_field(meta, F_VER, (uint16_t)(field(meta, F_VER) + 1));
            set_field(meta, F_SRC, sector_block(src));
            set_field(meta, F_SELF, sector_block(dst));
            flash_copy_meta(src, dst, n, meta);
        }
        else
        {
            flash_copy_meta(src, dst, n, NULL);
        }
    }
}

/* ---- exchange blocks ---- */

static void exch_close(struct exch_block *e)
{
    if (e->open)
    {
        copy_sectors(e->src + e->start % sec_per_block(), e->start,
                     e->end - e->start);
        queue_in(sector_block(e->src));
        e->open = false;
    }
}

/* Open an exchange block for the logical block mapped at `sec`, in the slot
 * of an exchange already on that block, else a free slot, else the least
 * recently used. */
static struct exch_block *exch_open(uint32_t lba, uint32_t sec)
{
    struct exch_block *e = NULL;
    uint8_t meta[FLASH_META_SIZE];
    uint32_t offset = sec % sec_per_block();
    uint16_t blk;
    uint8_t i;

    for (i = 0; i < ftl.config.exch_blocks && e == NULL; i++)
    {
        if (ftl.exch[i].open
            && sector_block(ftl.exch[i].start) == sector_block(sec))
        {
            e = &ftl.exch[i];
            exch_close(e);
        }
    }
    for (i = 0; i < ftl.config.exch_blocks && e == NULL; i++)
    {
        if (!ftl.exch[i].open)
        {
            e = &ftl.exch[i];
        }
    }
    for (i = 0; i < ftl.config.exch_blocks && e == NULL; i++)
    {
        if (ftl.exch[i].age == 0)
        {
            e = &ftl.exch[i];
            exch_close(e);
        }
    }

    /* the version follows the block being replaced: field 1 of its second
     * sector (HM-601: 16 bits, where the rk2808 object lets it reach 2^16) */
    flash_read(sec - offset + 1, NULL, meta, 1);
    e->ver = (uint16_t)(field(meta, 0) + 1);

    blk = queue_out();
    remap_set(sector_block(lba), blk);
    e->start = block_sector(blk);
    e->end = e->start + sec_per_block();
    e->open = true;
    flash_erase(e->start);

    e->src = sec - offset;
    copy_sectors(e->src, e->start, offset);
    e->start += offset;
    return e;
}

/* The exchange block to write the page at `lba` to: the one writing that
 * block, if the page is not behind its cursor - pages skipped are copied
 * forward - or a new one. */
static struct exch_block *exch_for_write(uint32_t lba)
{
    uint32_t sec = remap_sector(lba, false);
    struct exch_block *e = NULL;
    uint8_t i;

    for (i = 0; i < ftl.config.exch_blocks && e == NULL; i++)
    {
        struct exch_block *x = &ftl.exch[i];

        if (x->open && x->start <= sec && sec < x->end)
        {
            if (x->start < sec)
            {
                copy_sectors(x->src + x->start % sec_per_block(), x->start,
                             sec - x->start);
                x->start = sec;
            }
            e = x;
        }
    }
    if (e == NULL)
    {
        e = exch_open(lba, sec);
    }

    for (i = 0; i < ftl.config.exch_blocks; i++)
    {
        if (e->age < ftl.exch[i].age)
        {
            ftl.exch[i].age--;
        }
    }
    e->age = (uint8_t)(ftl.config.exch_blocks - 1);
    return e;
}

/* The page at `lba` is the next one an exchange block writes */
static bool exch_at(uint32_t lba)
{
    uint32_t sec = remap_sector(lba, false);
    bool at = false;
    uint8_t i;

    for (i = 0; i < ftl.config.exch_blocks && !at; i++)
    {
        at = ftl.exch[i].open && ftl.exch[i].start == sec
             && sec < ftl.exch[i].end;
    }
    return at;
}

/* Write one page, at a page-aligned `lba`, through an exchange block */
static void write_page(uint32_t lba, const void *data)
{
    struct exch_block *e = exch_for_write(lba);
    uint8_t meta[PAGE_META_BYTES];

    memset(meta, HEADER_PAD, sizeof(meta));
    set_field(meta, F_TAG, TAG_DATA);       /* HM-601: no version bits in it */
    set_field(meta, F_VER, e->ver);
    set_field(meta, F_LBLOCK, sector_block(lba));
    set_field(meta, F_SRC, sector_block(e->src));
    set_field(meta, F_SELF, sector_block(e->start));

    flash_program(e->start, data, meta, sec_per_page());
    e->start += sec_per_page();
    if (e->start == e->end)
    {
        exch_close(e);
    }
}

/* ---- the write cache ---- */

static uint8_t cache_index(const struct cache_page *c)
{
    return (uint8_t)(c - ftl.cache);
}

static uint8_t *cache_data(const struct cache_page *c)
{
    return cache_buf[cache_index(c)];
}

/* The valid cache page holding `lba`; CACHE_PAGES if none */
static uint8_t cache_match(uint32_t lba)
{
    uint32_t page = lba - lba % sec_per_page();
    uint8_t i;

    for (i = 0;
         i < CACHE_PAGES && !(ftl.cache[i].valid && ftl.cache[i].lba == page);
         i++)
    {
    }
    return i;
}

static uint8_t cache_first_free(void)
{
    uint8_t i;

    for (i = 0; i < CACHE_PAGES && ftl.cache[i].valid; i++)
    {
    }
    return i;
}

static void journal_new_block(void)
{
    uint16_t blk = queue_out();

    ftl.journal_blk[ftl.journal_slot] = blk;
    flash_erase(block_sector(blk));
}

static void journal_program(const struct cache_page *c)
{
    uint8_t meta[PAGE_META_BYTES];

    memset(meta, 0xff, sizeof(meta));
    set_word(meta, F_TAG, TAG_CACHE);       /* HM-601: no version bits in it */
    set_word(meta, F_VER, ftl.journal_ver);
    set_word(meta, F_LBA_LO, c->lba);
    set_word(meta, F_LBA_HI, c->lba >> 16);
    set_word(meta, F_MASK_LO, c->mask);
    set_word(meta, F_MASK_HI, c->mask >> 16);
    set_word(meta, 6, 0);
    set_word(meta, 7, 0);

    /* the whole page buffer, the sectors the mask leaves out included */
    flash_program(block_sector(ftl.journal_blk[ftl.journal_slot])
                  + ftl.journal_page, cache_data(c), meta, sec_per_page());
    ftl.journal_page += sec_per_page();
}

/* Record cache page `c` in the journal. When the journal has used all its
 * blocks it starts again from a new first block, freeing the others, and
 * records every cached page there - `c` among them. */
static void cache_write(struct cache_page *c)
{
    uint8_t i;

    if (c->valid)
    {
        if (ftl.journal_slot < 0)
        {
            ftl.journal_slot = 0;
            ftl.journal_page = 0;
            journal_new_block();
            journal_program(c);
        }
        else if (ftl.journal_page < sec_per_block())
        {
            journal_program(c);
        }
        else
        {
            ftl.journal_ver++;
            ftl.journal_page = 0;
            ftl.journal_slot =
                (int8_t)((ftl.journal_slot + 1) % JOURNAL_BLOCKS);

            if (ftl.journal_slot != 0)
            {
                journal_new_block();
                journal_program(c);
            }
            else
            {
                for (i = 0; i < JOURNAL_BLOCKS; i++)
                {
                    queue_in(ftl.journal_blk[i]);
                    ftl.journal_blk[i] = 0;
                }
                journal_new_block();

                /* recursion one level deep: a fresh block has room */
                for (i = 0; i < CACHE_PAGES && ftl.order[i] != NO_SLOT; i++)
                {
                    cache_write(&ftl.cache[ftl.order[i]]);
                }
            }
        }
    }
}

/* Drop cache page `c`, journalling it with an empty mask */
static void cache_invalidate(struct cache_page *c)
{
    uint8_t idx = cache_index(c);
    uint8_t i, k;

    for (i = 0; i < CACHE_PAGES; i++)
    {
        if (ftl.order[i] == idx)
        {
            c->mask = 0;
            cache_write(c);
            c->valid = false;
            for (k = i; k < CACHE_PAGES; k++)
            {
                ftl.order[k] = ftl.order[k + 1];
            }
            break;
        }
    }
}

static int read_sectors(uint32_t lba, uint8_t *buf, uint32_t count);

/* Write cache page `c` to its block - the sectors it does not hold read
 * from the flash first - and drop it. */
static void cache_flush(struct cache_page *c)
{
    uint8_t *data = cache_data(c);
    uint8_t i;

    if (c->valid)
    {
        for (i = 0; i < sec_per_page(); i++)
        {
            if (!(c->mask & ((uint32_t)1 << i)))
            {
                read_sectors(c->lba + i,
                             data + (size_t)i * FLASH_SECTOR_SIZE, 1);
            }
        }
        write_page(c->lba, data);
        cache_invalidate(c);
    }
}

/* Make room in a full cache. A run of more than three consecutive pages -
 * a sequential write - is flushed, from the first page an exchange block is
 * waiting for, or all but its last page. Otherwise, or if that freed no
 * more than two, the least recently used page goes, with the pages that
 * follow it consecutively. Returns the slot to use. */
static uint8_t cache_evict(void)
{
    uint8_t best = 0, best_start = 0, cur = 0, cur_start = 0;
    uint8_t slot = CACHE_PAGES;
    bool lru = true;
    uint8_t j, k, n, p;

    for (j = 0; j < CACHE_PAGES && ftl.order[j + 1] != NO_SLOT; j++)
    {
        if (ftl.cache[ftl.order[j]].lba + sec_per_page()
            == ftl.cache[ftl.order[j + 1]].lba)
        {
            if (cur == 0)
            {
                cur_start = j;
            }
            cur++;
        }
        else
        {
            if (best < cur)
            {
                best_start = cur_start;
                best = cur;
            }
            cur = 0;
        }
    }
    if (best < cur)
    {
        best_start = cur_start;
        best = cur;
    }

    /* best counts the steps of the run: it holds best + 1 pages. Each page
     * flushed leaves the order, the next taking its position. */
    if (best + 1 > 3)
    {
        for (k = 0;
             k < best && !exch_at(ftl.cache[ftl.order[best_start + k]].lba);
             k++)
        {
        }
        if (k < best)
        {
            for (n = (uint8_t)(best - k); n > 0; n--)
            {
                cache_flush(&ftl.cache[ftl.order[best_start + k]]);
            }
            lru = best + 1 - k <= 3;
        }
        else
        {
            for (n = best; n > 0; n--)
            {
                cache_flush(&ftl.cache[ftl.order[best_start]]);
            }
            lru = false;
        }
    }

    if (lru)
    {
        slot = 0;
        for (j = 1; j < CACHE_PAGES; j++)
        {
            if (ftl.cache[j].age < ftl.cache[slot].age)
            {
                slot = j;
            }
        }
        for (p = 0; p < CACHE_PAGES && ftl.order[p] != slot; p++)
        {
        }
        while (p < CACHE_PAGES && ftl.order[p] != NO_SLOT)
        {
            uint8_t this = ftl.order[p], next = ftl.order[p + 1];
            bool chained = next != NO_SLOT &&
                ftl.cache[this].lba + sec_per_page() == ftl.cache[next].lba;

            cache_flush(&ftl.cache[this]);
            if (!chained)
            {
                break;
            }
        }
    }
    else
    {
        slot = cache_first_free();
    }
    return slot;
}

/* The cache page for `lba`, opened if not cached */
static struct cache_page *cache_open(uint32_t lba)
{
    uint8_t slot = cache_match(lba);
    bool hit = slot < CACHE_PAGES;
    struct cache_page *c;
    uint8_t i, j;

    if (!hit)
    {
        slot = cache_first_free();
        if (slot == CACHE_PAGES)
        {
            slot = cache_evict();
        }
    }

    c = &ftl.cache[slot];
    c->valid = true;
    c->lba = lba - lba % sec_per_page();

    if (!hit)
    {
        for (j = 0; j < CACHE_PAGES && ftl.order[j] != NO_SLOT &&
                    !(c->lba < ftl.cache[ftl.order[j]].lba); j++)
        {
        }
        if (j < CACHE_PAGES && ftl.order[j] != NO_SLOT)
        {
            for (i = CACHE_PAGES - 1; i >= j && i < CACHE_PAGES; i--)
            {
                ftl.order[i + 1] = ftl.order[i];
            }
        }
        ftl.order[j] = slot;
    }

    for (i = 0; i < CACHE_PAGES; i++)
    {
        if (c->age < ftl.cache[i].age)
        {
            ftl.cache[i].age--;
        }
    }
    c->age = CACHE_PAGES - 1;
    return c;
}

/* ---- reading ---- */

static int read_sectors(uint32_t lba, uint8_t *buf, uint32_t count)
{
    int ret = 0;

    while (count > 0)
    {
        uint32_t sec = remap_sector(lba, true);
        uint32_t off = lba % sec_per_page();
        uint32_t len = sec_per_page() - off;
        uint32_t page = lba - off;
        uint8_t k;

        if (len > count)
        {
            len = count;
        }
        if (flash_read(sec, buf, NULL, len))
        {
            ret = 1;
        }

        /* the cache has the newest copy of what it holds */
        for (k = 0; k < CACHE_PAGES && ftl.order[k] != NO_SLOT; k++)
        {
            const struct cache_page *c = &ftl.cache[ftl.order[k]];

            if (c->lba == page)
            {
                uint32_t i;

                for (i = 0; i < len && c->mask != 0; i++)
                {
                    if (c->mask & ((uint32_t)1 << (off + i)))
                    {
                        memcpy(buf + i * FLASH_SECTOR_SIZE,
                               cache_data(c) + (off + i) * FLASH_SECTOR_SIZE,
                               FLASH_SECTOR_SIZE);
                    }
                }
                break;
            }
        }

        lba += len;
        buf += len * FLASH_SECTOR_SIZE;
        count -= len;
    }
    return ret;
}

/* ---- writing ---- */

/* Put `n` sectors from `buf` into cache page `c`, from sector `off` */
static void cache_put(struct cache_page *c, uint32_t off, const uint8_t *buf,
                      uint32_t n)
{
    memcpy(cache_data(c) + off * FLASH_SECTOR_SIZE, buf, n * FLASH_SECTOR_SIZE);
    c->mask |= page_mask(n) << off;
}

static void write_sectors(uint32_t lba, const uint8_t *buf, uint32_t count)
{
    uint32_t spp = sec_per_page();
    uint32_t off = lba % spp;
    uint32_t left = count;
    bool done = false;
    uint8_t k;

    if (off != 0)
    {
        uint32_t n = spp - off;
        struct cache_page *c;

        if (n > count)
        {
            n = count;
        }
        c = cache_open(lba);
        cache_put(c, off, buf, n);
        buf += n * FLASH_SECTOR_SIZE;
        lba += n;
        left -= n;
        if (count <= CACHE_FIRST_MAX)
        {
            cache_write(c);
        }
        else
        {
            cache_flush(c);
        }
    }

    while (!done)
    {
        if (left < spp)
        {
            if (left > 0)
            {
                struct cache_page *c = cache_open(lba);

                cache_put(c, 0, buf, left);
                cache_write(c);
            }
            done = true;
        }
        else
        {
            /* odd but original: on the length of the whole request */
            if (count <= CACHE_WRITE_MAX && !exch_at(lba))
            {
                struct cache_page *c = cache_open(lba);

                cache_put(c, 0, buf, spp);
                cache_write(c);
            }
            else
            {
                /* the page before this one goes first, so a sequential
                 * write continues the exchange block; odd but original:
                 * the first in the order is never looked at */
                for (k = CACHE_PAGES - 1; k > 0; k--)
                {
                    uint8_t i = ftl.order[k];

                    if (i != NO_SLOT && ftl.cache[i].lba + spp == lba)
                    {
                        cache_flush(&ftl.cache[i]);
                        break;
                    }
                }
                k = cache_match(lba);
                if (k < CACHE_PAGES)
                {
                    cache_invalidate(&ftl.cache[k]);
                }
                write_page(lba, buf);
            }
            buf += spp * FLASH_SECTOR_SIZE;
            lba += spp;
            left -= spp;
        }
    }
}

/* ---- mount ---- */

/* Read the bad-block table: the newest page of the two mirror blocks among
 * the first BBT_SCAN_BLOCKS of the FTL's area. Mirrors left at different
 * pages are rewritten from the newer, as the original does on a writable
 * mount. */
static enum ftl_b_error read_bbt(void)
{
    enum ftl_b_error err = FTL_B_OK;
    uint32_t addr[2] = { UINT32_MAX, UINT32_MAX };
    uint8_t meta[PAGE_META_BYTES];
    uint32_t spb = sec_per_block();
    uint32_t pick, off0, off1;
    int found = -1;
    uint16_t blk, i;

    for (blk = ftl.config.first_block;
         blk < ftl.config.first_block + BBT_SCAN_BLOCKS
         && blk < ftl.total_blocks && found < 1;
         blk++)
    {
        uint32_t off;

        for (off = 0; off < spb; off += sec_per_page())
        {
            if (flash_read(block_sector(blk) + off, NULL, meta, 1) == 0)
            {
                if (field(meta, F_TAG) != TAG_TABLE)
                {
                    break;
                }
                if (off == 0)
                {
                    found++;
                }
                if (found >= 0)
                {
                    addr[found] = block_sector(blk) + off;
                }
            }
        }
    }

    if (found < 0)
    {
        err = FTL_B_ERR_NO_TABLE;
    }
    else
    {
        off0 = addr[0] % spb;
        off1 = addr[1] % spb;
        pick = addr[0];
        if (off0 != off1 && addr[1] != UINT32_MAX
            && (addr[0] == UINT32_MAX || off1 >= off0))
        {
            pick = addr[1];
        }

        flash_read(pick, page_buf, meta, sec_per_page());

        if (off0 != off1)
        {
            uint16_t a = field(meta, F_MIRROR_A), b = field(meta, F_MIRROR_B);

            if (sector_block(pick) == a)
            {
                addr[0] = block_sector(b);
                addr[1] = block_sector(a);
            }
            else
            {
                addr[1] = block_sector(b);
                addr[0] = block_sector(a);
            }

            if (a < ftl.config.first_block || b < ftl.config.first_block ||
                a >= ftl.total_blocks || b >= ftl.total_blocks)
            {
                err = FTL_B_ERR_TABLE;
            }
            else if (ftl.writable)
            {
                flash_erase(addr[0]);
                flash_program(addr[0], page_buf, meta, sec_per_page());
                flash_erase(addr[1]);
                flash_program(addr[1], page_buf, meta, sec_per_page());
            }
        }
    }

    if (err == FTL_B_OK)
    {
        const uint8_t *tbl = page_buf;

        ftl.format_version = field(meta, F_VER);
        ftl.sys_block[SYS_POWER] = field(meta, F_POWER_BLOCK);
        ftl.sys_block[SYS_LAST] = field(meta, F_LAST_SYS_BLOCK);
        ftl.logical_blocks = field(meta, F_LOGICAL);

        for (i = 0;
             i < BBT_MAX && (tbl[2 * i] | (tbl[2 * i + 1] << 8)) != 0xffff;
             i++)
        {
            ftl.bad[i] = (uint16_t)(tbl[2 * i] | (tbl[2 * i + 1] << 8));
        }
        ftl.bad_count = i;

        ftl.sys_block[SYS_MIRROR_A] =
            sector_block(addr[1] < addr[0] ? addr[1] : addr[0]);
        ftl.sys_block[SYS_MIRROR_B] =
            sector_block(addr[1] < addr[0] ? addr[0] : addr[1]);
        ftl.bbt_page = (uint16_t)(addr[0] % spb);

        if ((ftl.format_version >> 8) != FORMAT_VERSION_MAJOR)
        {
            err = FTL_B_ERR_VERSION;
        }
        else if (ftl.logical_blocks > MAX_LOGICAL_BLOCKS ||
                 ftl.sys_block[SYS_LAST] >= ftl.total_blocks)
        {
            err = FTL_B_ERR_TOO_BIG;
        }
    }
    return err;
}

/* Finish a refresh of a system block that power loss interrupted: its
 * record says the block's content waits in a temporary block. Unlike the
 * original, which also replays its "done" record - a copy of block 0 onto
 * itself - only an unfinished copy is redone. */
static void power_loss_recover(void)
{
    uint32_t base = block_sector(ftl.sys_block[SYS_POWER]);
    uint8_t meta[FLASH_META_SIZE];
    uint32_t mode = 0, src = 0, dst = 0;
    bool any = false;
    uint32_t off;

    for (off = 0; off < sec_per_block(); off += sec_per_page())
    {
        if (flash_read(base + off, page_buf, meta, 1) == 0)
        {
            if (field(meta, 0) != TAG_POWER)
            {
                break;
            }
            mode = page_buf[0] | page_buf[1] << 8 | page_buf[2] << 16
                   | (uint32_t)page_buf[3] << 24;
            src  = page_buf[4] | page_buf[5] << 8 | page_buf[6] << 16
                   | (uint32_t)page_buf[7] << 24;
            dst  = page_buf[8] | page_buf[9] << 8 | page_buf[10] << 16
                   | (uint32_t)page_buf[11] << 24;
            any = true;
        }
    }

    if (any && ftl.writable)
    {
        if (mode == PWR_MODE_COPY && src < ftl.geo->total_sectors
            && dst < ftl.geo->total_sectors)
        {
            flash_erase(dst);
            copy_sectors(src, dst, sec_per_block());
        }
        flash_erase(base);
    }
}

/* The mount's working set lives in the cache buffers, free until the
 * journal is replayed into them */
#define SCAN_USED       ((uint32_t *)cache_buf[0])
#define SCAN_PAIRS      ((uint16_t *)cache_buf[1])

static void mark_used(uint16_t blk, bool used)
{
    if (blk < ftl.total_blocks)
    {
        if (used)
        {
            SCAN_USED[blk / 32] |= (uint32_t)1 << (blk % 32);
        }
        else
        {
            SCAN_USED[blk / 32] &= ~((uint32_t)1 << (blk % 32));
        }
    }
}

static bool is_used(uint16_t blk)
{
    return (SCAN_USED[blk / 32] >> (blk % 32)) & 1;
}

/* Remember the (source, destination) of the logical block now stored in
 * `blk`, replacing the pair of the block it displaced, `old` */
static void add_pair(uint16_t *npairs, uint16_t old, uint16_t src, uint16_t dst)
{
    uint16_t k;

    for (k = 0; k < RCV_PAIRS && SCAN_PAIRS[2 * k + 1] != old; k++)
    {
    }
    if (k == RCV_PAIRS && *npairs < RCV_PAIRS)
    {
        k = (*npairs)++;
    }
    if (k < RCV_PAIRS)
    {
        SCAN_PAIRS[2 * k] = src;
        SCAN_PAIRS[2 * k + 1] = dst;
    }
}

/* A data block claiming logical block `lb`: the newer of it and the block
 * claiming it already wins */
static void scan_data_block(uint16_t blk, const uint8_t *meta, uint16_t *npairs)
{
    uint16_t lb = field(meta, F_LBLOCK);
    uint16_t old = ftl.remap[lb];

    if (old == 0)
    {
        mark_used(blk, true);
        ftl.remap[lb] = blk;
    }
    else
    {
        uint8_t old_meta[DATA_HEADER_FIELDS * FLASH_META_SIZE];
        uint16_t src, dst;

        flash_read(block_sector(old), NULL, old_meta, DATA_HEADER_FIELDS);

        if (ver_newer(field(meta, F_VER), field(old_meta, F_VER)))
        {
            mark_used(old, false);
            mark_used(blk, true);
            ftl.remap[lb] = blk;
            src = field(meta, F_SRC);
            dst = field(meta, F_SELF);
        }
        else
        {
            src = field(old_meta, F_SRC);
            dst = field(old_meta, F_SELF);
        }
        add_pair(npairs, old, src, dst);
    }
}

/* A journal block: keep the JOURNAL_BLOCKS newest, oldest first, in
 * journal_blk[] with their versions in vers[]. vers[0] == 0 is taken for a
 * free first entry. */
static void scan_journal_block(uint16_t blk, uint16_t ver, uint16_t *vers,
                               uint8_t *count)
{
    int i = JOURNAL_BLOCKS - 1;
    int k;

    while (i > JOURNAL_BLOCKS - 1 - *count && !ver_newer(ver, vers[i]))
    {
        i--;
    }

    if (i > JOURNAL_BLOCKS - 1 - *count)
    {
        if (vers[0] == 0 && *count != JOURNAL_BLOCKS)
        {
            (*count)++;
        }
        else
        {
            mark_used(ftl.journal_blk[0], false);
            queue_in(ftl.journal_blk[0]);
        }
        for (k = 0; k < i; k++)
        {
            vers[k] = vers[k + 1];
            ftl.journal_blk[k] = ftl.journal_blk[k + 1];
        }
        vers[i] = ver;
        ftl.journal_blk[i] = blk;
        mark_used(blk, true);
    }
    else if (i < 0)
    {
        queue_in(blk);
    }
    else
    {
        vers[i] = ver;
        ftl.journal_blk[i] = blk;
        mark_used(blk, true);
        (*count)++;
    }
}

/* An exchange the mount found: `dst` was being written with the content of
 * `src`. If it is not full, reopen it - or, if one of its last pages cannot
 * be read, finish it into a new block now, taking unreadable pages from the
 * source. When it is full, the exchange finished and its source is free. */
static void recover_exchange(uint16_t src_blk, uint16_t dst_blk, uint8_t *nexch)
{
    uint32_t spp = sec_per_page();
    uint32_t dst = block_sector(dst_blk), src = block_sector(src_blk);
    int last = sec_per_block() / spp - 1;
    int lo = 0, hi = last;
    uint8_t meta[2 * FLASH_META_SIZE];
    uint16_t ver = 0;
    uint32_t written, off;
    uint16_t lb;

    flash_read(dst + spp * last, NULL, meta, 1);
    if (field(meta, F_TAG) == FIELD_ERASED)
    {
        while (lo <= hi)
        {
            int mid = (lo + hi) / 2;

            flash_read(dst + spp * mid, NULL, meta, 2);
            if (field(meta, F_TAG) == FIELD_ERASED)
            {
                hi = mid - 1;
            }
            else
            {
                ver = field(meta, F_VER);
                lo = mid + 1;
            }
        }
    }
    written = spp * (hi + 1);

    if (written < sec_per_block())
    {
        off = written > RECHECK_PAGES * spp ? written - RECHECK_PAGES * spp : 0;
        while (off < written && flash_read(dst + off, NULL, NULL, spp) == 0)
        {
            off += spp;
        }

        if (off < written && ftl.writable)
        {
            uint16_t nb = queue_out();
            uint32_t to;

            if (nb == 0)
            {
                nb = ftl.sys_block[SYS_POWER];
            }
            to = block_sector(nb);
            remap_set(remap_owner(dst), nb);
            flash_erase(to);
            copy_sectors(dst, to, off);
            for (; off < written; off += spp)
            {
                if (flash_read(dst + off, NULL, NULL, spp) == 0)
                {
                    copy_sectors(dst + off, to + off, spp);
                }
                else
                {
                    copy_sectors(src + off, to + off, spp);
                }
            }
            copy_sectors(src + written, to + written,
                         sec_per_block() - written);
            queue_in(dst_blk);
            queue_in(src_blk);

            /* odd but original: the block stays mapped to the power-loss
             * block, which is then erased */
            if (nb == ftl.sys_block[SYS_POWER])
            {
                copy_sectors(to, block_sector(queue_out()), sec_per_block());
                flash_erase(to);
            }
        }
        else if (*nexch < ftl.config.exch_blocks)
        {
            struct exch_block *e = &ftl.exch[*nexch];

            e->src = src;
            e->start = dst + written;
            e->end = dst + sec_per_block();
            e->ver = ver;
            e->age = *nexch;
            e->open = true;
            (*nexch)++;
        }
    }
    else if (src_blk < ftl.total_blocks)
    {
        /* finished: its source is free unless something maps it. The
         * original would queue a source past the chip too. */
        for (lb = 0; lb < ftl.logical_blocks && ftl.remap[lb] != src_blk; lb++)
        {
        }
        if (lb == ftl.logical_blocks)
        {
            queue_in(src_blk);
        }
    }
}

/* Replay the journal into the cache: newest block first, newest page
 * first, each page once, until the cache is full. A page with an empty
 * mask cancels the older entries of its page. */
static void replay_journal(void)
{
    uint32_t *seen = (uint32_t *)page_buf;
    uint8_t meta[4 * FLASH_META_SIZE];
    uint8_t restored = 0;
    int i;

    ftl.journal_page = NO_JOURNAL;
    ftl.journal_slot = -1;
    memset(page_buf, 0xff, sizeof(page_buf));

    for (i = JOURNAL_BLOCKS - 1; i >= 0; i--)
    {
        int32_t off;

        if (ftl.journal_blk[i] == 0)
        {
            continue;
        }
        if (ftl.journal_slot < 0)
        {
            ftl.journal_slot = (int8_t)i;
        }

        for (off = sec_per_block() - sec_per_page();
             off >= 0 && restored < CACHE_PAGES;
             off -= sec_per_page())
        {
            uint32_t sec = block_sector(ftl.journal_blk[i]) + off;
            uint32_t lba, mask;
            uint16_t k;

            if (flash_read(sec + F_LBA_LO, NULL, meta, 4) != 0)
            {
                continue;
            }
            /* byte 2 of the LBA word is never 0xff in a written page */
            if (meta[2] == 0xff)
            {
                ftl.journal_page = (uint32_t)off;
                continue;
            }

            lba  = field(meta, 0) | (uint32_t)field(meta, 1) << 16;
            mask = field(meta, 2) | (uint32_t)field(meta, 3) << 16;

            for (k = 0;
                 k < SEEN_MAX && seen[k] != 0xffffffff && seen[k] != lba;
                 k++)
            {
            }
            if (k < SEEN_MAX && seen[k] == 0xffffffff)
            {
                if (mask != 0)
                {
                    struct cache_page *c = cache_open(lba);

                    restored++;
                    c->mask = mask;
                    flash_read(sec, cache_data(c), NULL, sec_per_page());
                }
                seen[k] = lba;
            }
        }
    }
}

static enum ftl_b_error mount_scan(void)
{
    enum ftl_b_error err = FTL_B_OK;
    uint16_t vers[JOURNAL_BLOCKS];
    uint8_t meta[DATA_HEADER_FIELDS * FLASH_META_SIZE];
    uint16_t npairs = 0;
    uint8_t njournal = 0, nexch = 0;
    uint16_t blk, k;
    uint8_t i;

    ftl.total_blocks = (uint16_t)(ftl.geo->total_sectors / sec_per_block());
    ftl.free.max = ftl.total_blocks >> 5;
    if (ftl.free.max < FREE_QUEUE_MIN)
    {
        ftl.free.max = FREE_QUEUE_MIN;
    }
    if (ftl.free.max > FREE_QUEUE_MAX)
    {
        ftl.free.max = FREE_QUEUE_MAX;
    }
    ftl.free.front = ftl.free.rear = ftl.free.count = 0;

    for (i = 0; i < MAX_EXCH_BLOCKS; i++)
    {
        ftl.exch[i].open = false;
        ftl.exch[i].age = i;
    }
    for (i = 0; i < CACHE_PAGES; i++)
    {
        ftl.cache[i].valid = false;
        ftl.cache[i].age = i;
        ftl.cache[i].mask = 0;
        ftl.cache[i].lba = 0;
        ftl.order[i] = NO_SLOT;
    }
    ftl.order[CACHE_PAGES] = NO_SLOT;

    err = read_bbt();

    if (err == FTL_B_OK)
    {
        memset(SCAN_USED, 0, (ftl.total_blocks + 31) / 32 * 4);
        memset(SCAN_PAIRS, 0, RCV_PAIRS * 2 * sizeof(uint16_t));
        memset(ftl.remap, 0, sizeof(ftl.remap));
        memset(ftl.journal_blk, 0, sizeof(ftl.journal_blk));
        memset(vers, 0, sizeof(vers));

        power_loss_recover();

        for (blk = ftl.sys_block[SYS_LAST] + 1; blk < ftl.total_blocks; blk++)
        {
            uint16_t tag;

            if (block_is_bad(blk))
            {
                mark_used(blk, true);
                continue;
            }
            if (flash_read(block_sector(blk), NULL, meta,
                           DATA_HEADER_FIELDS) != 0)
            {
                continue;
            }

            tag = field(meta, F_TAG);
            if ((tag & TAG_KIND) == TAG_CACHE)
            {
                scan_journal_block(blk, field(meta, F_VER), vers, &njournal);
            }
            else if (tag == TAG_PAGE_REMAP)
            {
                queue_in(blk);
            }
            else if ((tag & TAG_KIND) == TAG_DATA
                     && field(meta, F_LBLOCK) < ftl.logical_blocks)
            {
                scan_data_block(blk, meta, &npairs);
            }
        }

        ftl.journal_ver = vers[JOURNAL_BLOCKS - 1];
        if (ftl.journal_blk[0] == 0)
        {
            /* close the gaps: oldest first from entry 0 */
            for (i = 0; i < JOURNAL_BLOCKS && ftl.journal_blk[i] == 0; i++)
            {
            }
            for (k = 0; i < JOURNAL_BLOCKS; k++, i++)
            {
                ftl.journal_blk[k] = ftl.journal_blk[i];
                ftl.journal_blk[i] = 0;
            }
        }

        /* both blocks of an exchange stay out of the free queue */
        for (k = 0; k < npairs; k++)
        {
            mark_used(SCAN_PAIRS[2 * k], true);
            mark_used(SCAN_PAIRS[2 * k + 1], true);
        }
        for (blk = ftl.sys_block[SYS_LAST] + 1; blk < ftl.total_blocks; blk++)
        {
            if (!is_used(blk))
            {
                queue_in(blk);
            }
        }

        for (k = 0; k < RCV_PAIRS; k++)
        {
            uint16_t src = SCAN_PAIRS[2 * k], dst = SCAN_PAIRS[2 * k + 1];

            /* the original reads past the chip for a destination that names
             * no block; such a pair is skipped. A source past the chip -
             * 0xffff in the blocks of a firmware image - only matters if the
             * destination is not full. */
            if (dst != 0 && dst < ftl.total_blocks)
            {
                recover_exchange(src, dst, &nexch);
            }
        }

        replay_journal();
    }
    return err;
}

/* ---- interface ---- */

enum ftl_b_error ftl_b_mount(const struct ftl_b_config *config)
{
    enum ftl_b_error err = FTL_B_OK;

    ftl.mounted = false;
    ftl.config = *config;
    ftl.geo = flash_get_geometry();
    ftl.writable = !config->read_only;

    if (ftl.geo->sec_per_page < MIN_SEC_PER_PAGE ||
        ftl.geo->sec_per_page > FLASH_MAX_SEC_PER_PAGE ||
        ftl.geo->sec_per_block == 0 ||
        ftl.geo->total_sectors / ftl.geo->sec_per_block > MAX_BLOCKS)
    {
        err = FTL_B_ERR_GEOMETRY;
    }
    else if (config->exch_blocks == 0
             || config->exch_blocks > MAX_EXCH_BLOCKS
             || config->first_block == 0)
    {
        err = FTL_B_ERR_CONFIG;
    }
    else
    {
        flash_set_meta_passthrough(true);
        flash_set_boot_area(block_sector(config->first_block));
        flash_set_writable(ftl.writable);

        err = mount_scan();
        ftl.mounted = err == FTL_B_OK;
    }

    if (!ftl.mounted)
    {
        ftl.writable = false;
        flash_set_writable(false);
    }
    ftl.error = err;
    return err;
}

void ftl_b_get_status(struct ftl_b_status *status)
{
    uint8_t i;

    memset(status, 0, sizeof(*status));
    status->error = ftl.error;
    status->writable = ftl.writable;
    status->format_version = ftl.format_version;
    status->logical_blocks = ftl.logical_blocks;
    status->bad_blocks = ftl.bad_count;
    status->free_blocks = ftl.free.count;
    for (i = 0; ftl.mounted && i < ftl.config.exch_blocks; i++)
    {
        status->open_exch += ftl.exch[i].open;
    }
    for (i = 0; ftl.mounted && i < CACHE_PAGES; i++)
    {
        status->cached_pages += ftl.cache[i].valid;
    }
}

uint32_t ftl_b_capacity(void)
{
    uint32_t sectors = 0;

    if (ftl.mounted)
    {
        sectors = (uint32_t)ftl.logical_blocks * sec_per_block();
    }
    return sectors;
}

static bool range_valid(uint32_t sector, uint32_t count)
{
    uint32_t cap = ftl_b_capacity();

    return sector < cap && count <= cap - sector;
}

int ftl_b_read(uint32_t sector, void *buf, uint32_t count)
{
    int ret = 1;

    if (range_valid(sector, count))
    {
        ret = read_sectors(sector, buf, count);
    }
    return ret;
}

int ftl_b_write(uint32_t sector, const void *buf, uint32_t count)
{
    int ret = 1;

    if (ftl.writable && range_valid(sector, count))
    {
        write_sectors(sector, buf, count);
        ret = 0;
    }
    return ret;
}

void ftl_b_sync(void)
{
}
