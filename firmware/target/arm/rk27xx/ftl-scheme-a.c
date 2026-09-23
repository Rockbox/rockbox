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

/* THE SCHEME A FLASH TRANSLATION LAYER
 *
 * Many rk2705/rk2706 players store everything - their firmware and the
 * user's music - on NAND flash managed by a block-mapping FTL, called Scheme
 * A here to tell it from the log-structured layout later firmware uses. This
 * file reads and writes that format exactly as the original firmware does,
 * so a device keeps working with its original firmware after Rockbox has
 * written to it. Where the original firmware's behaviour is odd, it is
 * reproduced on purpose and marked as such.
 *
 *
 * VOLUMES
 *
 * The FTL presents one linear space of 512-byte logical sectors, split into
 * two volumes:
 *
 *   SYS    the first sectors. The original firmware keeps itself here
 *          (BASE.RKW and its resources) as a FAT volume of its own.
 *   USER   everything after SYS: the music storage, the drive the original
 *          firmware exports over USB.
 *
 * Where SYS ends is not in the FTL's own tables: the boot area's ID block
 * records it (see ftl-rk27xx.c), and the caller passes it in.
 *
 *
 * BLOCKS AND ZONES
 *
 * The unit of mapping is the SUPER-BLOCK - on a two-plane chip, a physical
 * block from each plane, used as one (flash-rk27xx.h). Blocks are grouped in
 * ZONES of 256; a 4096-block chip has 16. Mapping never crosses a zone: a
 * logical block always lives somewhere in its own zone, so a zone can be
 * described by one byte per block.
 *
 * The first blocks of the chip are the BOOT AREA: the bootloader and the ID
 * blocks the boot ROM looks for. The FTL does not use them. The zone they are
 * in is the SYSTEM ZONE (zone 0 on every device seen), and the number of boot
 * blocks in it is the SYSTEM OFFSET.
 *
 *
 * THE ZONE TABLE
 *
 * Each zone is described by a 512-byte table. Its first 256 bytes are block
 * positions within the zone:
 *
 *   [0 .. n-1]            logical block i is stored at position table[i];
 *                         n is the zone's block count (see below)
 *   [base - 1]            system zone only: the exchange-record block
 *   [base .. base+5]      the exchange pool: six erased blocks, one per
 *                         exchange slot, that writes go to
 *   [base + 6]            the spare block power-loss recovery merges into
 *   [0xfe], [0xff]        the zone's two remap-log blocks: mirror, primary
 *
 * Positions not listed are bad or unused. The table is a permutation - no
 * block appears twice - and writing only ever swaps entries.
 *
 * In the system zone every index above except the log entries' and the
 * format flag's counts DOWN from the top shifted by the system offset: the
 * top entries are taken by the boot blocks. So the primary log block of the
 * system zone is at table[0xff - offset], its pool at table[base - offset].
 *
 * The BASE, the first pool entry, depends on the firmware build that
 * formatted the device - 0xf7 on the rk2705 build, 0xf1 on the Samsung
 * YP-CP3 - as does the system offset. Neither is stored anywhere. Both are
 * recovered from the structure at mount (probe_layout), and a device whose
 * layout cannot be confirmed is mounted read-only: writing with the wrong
 * base would erase blocks holding data.
 *
 * Bytes 0x100..0x10b are the SWAP LOG: one (old, new) position pair per
 * exchange slot, recording the last swap that slot made. In the system
 * zone's table, byte 0xff (unshifted) is the FORMAT FLAG - 2 for a formatted
 * device - and the ten entries below the exchange record, table[base - 11 ..
 * base - 2], are the device's REPLACEMENT BLOCKS for bad-block relocation.
 *
 *
 * THE REMAP LOG
 *
 * A zone's table is never rewritten in place. Every version is appended, one
 * sector per page, to the zone's remap-log block; the newest programmed page
 * is the current table. The first page of a log block carries the block type
 * 0x52 in its metadata, which is how a mount finds it. Every version is
 * written twice, to the primary and to the mirror log block, so that
 * recycling a full log - erasing both and starting again at page 0 - cannot
 * lose the zone: an interrupted recycle leaves one of them intact, and the
 * mount rewrites the other from it.
 *
 *
 * THE EXCHANGE RECORD
 *
 * One block of the system zone holds device-wide state, one version per
 * page: byte z of the newest page is zone z's BLOCK COUNT - how many logical
 * blocks it holds - and from byte 0x40, ten little-endian 16-bit entries
 * list the blocks that have been replaced after failing. The block counts
 * give the capacity: every logical block is a super-block of data.
 *
 *
 * WRITING
 *
 * NAND is erased a block at a time and programmed a page at a time, only
 * once between erases. A write to a logical block therefore goes to a
 * different, erased physical block - an EXCHANGE BLOCK - through one of six
 * exchange slots per zone:
 *
 *   1. open: swap the logical block's table entry with the slot's pool
 *      entry, record the swap in the swap log, erase the new block, and
 *      publish the table - all before any data is written
 *   2. copy forward the pages before the first one being written, from the
 *      old block; then write pages in order, the slot's cursor advancing
 *   3. close: copy forward the rest of the old block
 *
 * The old block is now in the pool, to be erased when a later write needs
 * it. A full exchange block is a finished write. Pages are staged in three
 * page-sized RAM buffers, so sectors written one at a time reach the flash a
 * page at a time; ftl_a_sync() writes them out.
 *
 *
 * POWER LOSS
 *
 * There is no journal. Because the table is published before the data, a
 * write cut short leaves a published table pointing at an exchange block
 * that is not full, and a swap-log entry whose old and new differ. At every
 * writable mount, recovery finds such slots, finds the last page programmed
 * in the exchange block, and merges those pages with the rest of the old
 * block into the zone's spare block, then publishes the table with the
 * swap undone. Everything before the final publish can be repeated, so
 * losing power during recovery only means recovering again.
 *
 * Whatever is still in the RAM buffers when power fails is lost.
 *
 *
 * BAD BLOCKS
 *
 * A block is bad when metadata byte 0 of its first page is not 0xff. When a
 * program or erase fails, the block is marked bad and replaced by the next
 * of the ten replacement blocks, the replacement recorded in the exchange
 * record. Blocks that are bad from the factory are simply never in a table.
 *
 *
 * METADATA
 *
 * Of each sector's spare bytes the FTL uses three:
 *
 *   [0]  0xff = good block (first page only)
 *   [1]  0x00 = this page is programmed - written by the flash layer on every
 *        program, so an erased page reads 0xff
 *   [2]  block type: 0x52 remap-log block; 0x69 marks the boot area's ID
 *        blocks
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flash-rk27xx.h"
#include "ftl-scheme-a.h"

/* ---- layout ---- */

#define ZONE_BLOCKS         256     /* blocks per zone */
#define MAX_ZONES           64
#define EXCH_SLOTS          6       /* exchange slots per zone */
#define REPLACEMENT_SLOTS   10      /* bad-block replacements, device-wide */
#define WRITE_CACHE_PAGES   3       /* page staging buffers */

#define TABLE_SIZE          512
#define TBL_LOG_PRIMARY     0xff    /* position of the primary log block */
#define TBL_LOG_MIRROR      0xfe    /* ... and of its mirror */
#define TBL_SWAP_LOG        0x100   /* EXCH_SLOTS (old, new) pairs */
#define TBL_FORMAT_FLAG     0xff    /* system zone only, never shifted */

/* In the exchange record: the replaced-block list */
#define XREC_REPLACED       0x40

/* The original firmware's reserve base, and the one used before a mount has
 * derived the device's own */
#define DEFAULT_RESERVE_BASE 0xf7

/* The format flag of a formatted device */
#define FORMAT_FLAG         2

/* ---- metadata ---- */

#define META_BAD            0       /* [0]: 0xff = good block */
#define META_PROGRAMMED     1       /* [1]: 0x00 once the page is programmed */
#define META_TYPE           2       /* [2]: block type */
#define META_ERASED         0xff
#define META_TYPE_LOG       0x52

static const uint8_t log_block_meta[FLASH_META_SIZE] =
    { 0xff, 0xff, META_TYPE_LOG };
static const uint8_t bad_block_meta[FLASH_META_SIZE] =
    { 0x00, 0xff, 0xff };

#define NO_ZONE             0xffff
#define NO_REPLACEMENT      0xffff  /* an unused replaced-block entry */
#define NO_LBA              0xffffffff

/* ---- state ---- */

/* An exchange slot: a logical block being rewritten into an exchange block. */
struct exch_slot
{
    uint32_t cursor;    /* next sector to write in the exchange block */
    uint32_t end;       /* the sector after the exchange block */
    uint32_t src;       /* first sector of the block being replaced */
    uint8_t  age;       /* EXCH_SLOTS - 1 = used last, 0 = reused next */
    bool     open;
};

struct zone
{
    uint8_t  log_block;         /* position of the primary remap-log block */
    uint16_t log_page;          /* page of the newest table version in it */
    uint8_t  block_count;       /* logical blocks in the zone */
    struct exch_slot exch[EXCH_SLOTS];
};

/* A page being assembled in RAM. */
struct write_cache
{
    uint8_t  buf[FLASH_MAX_SEC_PER_PAGE * FLASH_SECTOR_SIZE];
    bool     valid;
    uint8_t  age;               /* as exch_slot.age */
    uint32_t next_lba;          /* the page holds [page start, next_lba) */
    struct exch_slot *slot;     /* the exchange slot the page is for */
};

static struct
{
    const struct flash_geometry *geo;
    struct ftl_a_config config;
    bool mounted;

    /* the layout of this device */
    uint8_t  reserve_base;
    uint16_t boot_blocks;           /* (system zone << 8) | system offset */
    uint32_t logical_sectors;       /* both volumes */
    uint16_t replaced[REPLACEMENT_SLOTS];
    struct zone zones[MAX_ZONES];

    /* what the last mount found */
    enum ftl_a_error error;
    enum ftl_a_confidence confidence;
    bool     write_protect;
    uint8_t  base_fits;
    uint8_t  format_flag;

    /* the table of cached_zone, the one most recently used */
    uint16_t cached_zone;
    uint8_t  table[TABLE_SIZE];

    struct write_cache cache[WRITE_CACHE_PAGES];
} ftl =
{
    .reserve_base  = DEFAULT_RESERVE_BASE,
    .write_protect = true,
};

/* Scratch sector for the exchange record and for the system zone's table,
 * shared by relocate_block() and replaced_block(); neither needs it across a
 * call to the other. */
static uint8_t scratch[TABLE_SIZE];

/* ---- geometry helpers ---- */

static uint8_t sec_per_page_raw(void)
{
    return ftl.geo->sec_per_page_raw;
}

static uint8_t sec_per_page(void)
{
    return ftl.geo->sec_per_page;
}

static uint16_t sec_per_block(void)
{
    return ftl.geo->sec_per_block;
}

static uint8_t system_zone(void)
{
    return (uint8_t)(ftl.boot_blocks >> 8);
}

static uint8_t system_offset(void)
{
    return (uint8_t)ftl.boot_blocks;
}

/* How far the table indices of `zone` are shifted down by boot blocks. */
static uint8_t table_shift(uint16_t zone)
{
    uint8_t shift = 0;

    if (zone <= system_zone())
    {
        shift = system_offset();
    }
    return shift;
}

/* Table index of the exchange record (meaningful in the system zone). */
static uint8_t xrec_index(void)
{
    return (uint8_t)(ftl.reserve_base - 1 - system_offset());
}

/* First sector of block `blk`, (zone << 8) | position. Past the end of the
 * chip it returns the sector past the end, which the flash layer refuses. */
static uint32_t block_sector(uint16_t blk)
{
    uint32_t sec = ftl.geo->total_sectors;

    if (blk < ftl.geo->total_blocks)
    {
        sec = (uint32_t)blk * sec_per_block();
    }
    return sec;
}

/* Sector of the system zone's current table. */
static uint32_t system_table_sector(void)
{
    const struct zone *sz = &ftl.zones[system_zone()];

    return block_sector((uint16_t)(sz->log_block + (system_zone() << 8))) +
           (uint32_t)sec_per_page_raw() * sz->log_page;
}

static void set_write_protect(bool protect)
{
    ftl.write_protect = protect;
    flash_set_writable(!protect);
}

static void reset_state(void)
{
    int i, j;

    for (i = 0; i < WRITE_CACHE_PAGES; i++)
    {
        ftl.cache[i].valid = false;
        ftl.cache[i].age = (uint8_t)i;
        ftl.cache[i].next_lba = NO_LBA;
    }

    ftl.cached_zone = NO_ZONE;

    for (i = 0; i < MAX_ZONES; i++)
    {
        for (j = 0; j < EXCH_SLOTS; j++)
        {
            ftl.zones[i].exch[j].open = false;
            ftl.zones[i].exch[j].age = (uint8_t)j;
        }
    }
}

/* Sectors on a volume. USER is whatever SYS leaves. */
static uint32_t volume_sectors(int volume)
{
    uint32_t sectors = 0;

    if (volume == FTL_A_VOL_SYS)
    {
        sectors = ftl.config.sys_sectors;
    }
    else if (volume == FTL_A_VOL_USER)
    {
        sectors = ftl.logical_sectors - ftl.config.sys_sectors;
    }
    return sectors;
}

/* ---- logical addresses ---- */

/* Split a logical sector into its zone, logical block in the zone and sector
 * in the block. Zones hold different numbers of logical blocks, so this
 * walks the block counts. */
static void locate(uint32_t lba, uint16_t *zone, uint8_t *block,
                   uint16_t *sector)
{
    uint32_t blk = lba / sec_per_block();
    uint16_t z;

    *sector = (uint16_t)(lba % sec_per_block());

    for (z = 0; z < MAX_ZONES && blk >= ftl.zones[z].block_count; z++)
    {
        blk -= ftl.zones[z].block_count;
    }

    *block = (uint8_t)blk;
    *zone = z;
}

/* First logical sector of `zone`. A zone past the last holding any blocks
 * returns the capacity. */
static uint32_t zone_first_lba(uint16_t zone)
{
    uint32_t lba = 0;
    uint16_t z = 0;

    while (z != zone && z < MAX_ZONES && lba < ftl.logical_sectors)
    {
        lba += (uint32_t)ftl.zones[z].block_count * sec_per_block();
        z++;
    }
    return lba;
}

/* ---- bad blocks ---- */

/* Is the block containing `sec` marked bad? On a two-plane part both planes'
 * markers count - but, as the original firmware does, only the first read's
 * status decides whether they are consulted at all. */
static bool block_is_bad(uint32_t sec)
{
    uint32_t addr = sec - sec % sec_per_block();
    uint8_t meta[FLASH_META_SIZE];
    uint8_t marker = 1;

    if (flash_read(addr, NULL, meta, 1) == 0)
    {
        marker = meta[META_BAD];
        if (ftl.geo->planes > 1)
        {
            flash_read(addr + sec_per_page_raw(), NULL, meta, 1);
            marker &= meta[META_BAD];
        }
    }
    return marker != META_ERASED;
}

static void mark_block_bad(uint32_t sec)
{
    uint32_t row = sec - sec % sec_per_block();

    flash_erase(row);
    flash_program(row, NULL, bad_block_meta, 1);

    if (ftl.geo->planes > 1)
    {
        row += sec_per_page_raw();
        flash_program(row, NULL, bad_block_meta, 1);
    }
}

/* The block that `blk` has been replaced with, following replacements of
 * replacements; `blk` itself when it has none. */
static uint32_t replaced_block(uint32_t blk)
{
    uint32_t result = blk;
    int i;

    for (i = 0; i < REPLACEMENT_SLOTS && ftl.replaced[i] != NO_REPLACEMENT; i++)
    {
        if (ftl.replaced[i] == (uint16_t)result)
        {
            flash_read(system_table_sector(), scratch, NULL, 1);
            result = scratch[xrec_index() + i - REPLACEMENT_SLOTS] +
                     (system_zone() << 8);
        }
    }
    return result;
}

/* Replace the block containing `sec` after a program or erase failed in it,
 * and return the sector in the replacement that corresponds to `sec`.
 *
 * With `data`, a write of `n` sectors at `sec` is carried over: the sectors
 * of the block before `sec` are copied, then the data programmed. The
 * replacement is recorded in the exchange record, any exchange slot writing
 * into the failed block is moved to the replacement, and the failed block is
 * marked bad. When every replacement is used up, `sec` itself is returned.
 *
 * The original firmware records replacements but, reading the exchange
 * record back at mount, looks for them on a page it never writes them to:
 * they are forgotten at every reboot. That is kept - see mount_scan(). */
static uint32_t relocate_block(uint32_t sec, const void *data, uint16_t n)
{
    uint32_t result = sec;
    uint32_t in_block = sec % sec_per_block();
    uint16_t failed_blk;
    uint32_t xrec;
    int slot;

    if (ftl.replaced[REPLACEMENT_SLOTS - 1] == NO_REPLACEMENT)
    {
        failed_blk = (uint16_t)((sec < ftl.geo->total_sectors) ?
                                sec / sec_per_block() : ftl.geo->total_blocks);

        flash_read(system_table_sector(), scratch, NULL, 1);
        xrec = block_sector((uint16_t)(scratch[xrec_index()] +
                                       (system_zone() << 8)));

        for (slot = 0; slot < REPLACEMENT_SLOTS; slot++)
        {
            uint32_t dest;
            bool retargeted = false;
            int z, k;

            if (ftl.replaced[slot] != NO_REPLACEMENT)
            {
                continue;
            }

            dest = block_sector((uint16_t)((system_zone() << 8) +
                       scratch[xrec_index() + slot - REPLACEMENT_SLOTS]));

            if (data != NULL)
            {
                flash_copy(sec - in_block, dest, (uint16_t)in_block);
                dest += in_block;

                if (n != 0)
                {
                    if (ftl.geo->planes > 1 && n != sec_per_page_raw())
                    {
                        flash_program_page(dest, data, NULL);
                        dest += sec_per_page();
                    }
                    else
                    {
                        flash_program(dest, data, NULL, sec_per_page_raw());
                        dest += sec_per_page_raw();
                    }
                }
            }

            /* append the new list to the exchange record */
            flash_read(xrec + slot * sec_per_page_raw(), scratch, NULL, 1);
            scratch[XREC_REPLACED + slot * 2]     = (uint8_t)failed_blk;
            scratch[XREC_REPLACED + slot * 2 + 1] = (uint8_t)(failed_blk >> 8);
            flash_program(xrec + (slot + 1) * sec_per_page_raw(), scratch,
                          NULL, 1);

            /* an exchange slot writing into the failed block follows it */
            for (z = 0; z < MAX_ZONES && !retargeted; z++)
            {
                for (k = 0; k < EXCH_SLOTS && !retargeted; k++)
                {
                    struct exch_slot *s = &ftl.zones[z].exch[k];
                    uint32_t block_start = s->cursor & ~(sec_per_block() - 1);

                    if (s->open && block_start <= sec && s->end > sec)
                    {
                        uint32_t dest_start = dest & ~(sec_per_block() - 1);

                        s->cursor = dest_start + s->cursor % sec_per_block();
                        s->end    = dest_start + sec_per_block();
                        retargeted = true;
                    }
                }
            }

            mark_block_bad(sec);
            ftl.replaced[slot] = failed_blk;
            result = dest;
            break;
        }
    }
    return result;
}

/* ---- the zone table ---- */

/* Bring `zone`'s current table into ftl.table. */
static void load_table(uint16_t zone)
{
    if (ftl.cached_zone != zone)
    {
        struct zone *z = &ftl.zones[zone];

        ftl.cached_zone = zone;
        flash_read(block_sector((uint16_t)((zone << 8) + z->log_block)) +
                       z->log_page * sec_per_page_raw(),
                   ftl.table, NULL, 1);
    }
}

/* Where logical sector `lba` is committed: its block per the table, ignoring
 * any exchange block it is being rewritten into. */
static uint32_t map_committed(uint32_t lba)
{
    uint16_t zone, sector;
    uint8_t block;
    uint32_t blk;

    locate(lba, &zone, &block, &sector);
    load_table(zone);

    blk = replaced_block((zone << 8) + ftl.table[block]);
    return block_sector((uint16_t)blk) | sector;
}

/* Where logical sector `lba` can be read now. A sector in a block being
 * rewritten reads from the block being replaced until the exchange block's
 * cursor has passed it; past the cursor the exchange block is erased. */
static uint32_t map_current(uint32_t lba)
{
    uint16_t zone, sector;
    uint8_t block;
    uint32_t addr;
    int i;

    locate(lba, &zone, &block, &sector);
    load_table(zone);

    addr = replaced_block((zone << 8) + ftl.table[block]);
    addr = block_sector((uint16_t)addr) | sector;

    for (i = 0; i < EXCH_SLOTS; i++)
    {
        struct exch_slot *s = &ftl.zones[zone].exch[i];

        if (s->open && s->cursor <= addr && s->end > addr)
        {
            addr = s->src + addr % sec_per_block();
            break;
        }
    }
    return addr;
}

/* Append ftl.table as the new version of the table of the zone holding
 * `lba`, to both log blocks. A full log is erased and restarted. */
static void publish_table(uint32_t lba)
{
    uint16_t zone, sector;
    uint8_t block;
    struct zone *z;
    uint32_t primary, mirror;

    locate(lba, &zone, &block, &sector);
    z = &ftl.zones[zone];

    ftl.cached_zone = zone;
    z->log_page++;

    mirror  = (zone << 8)
              + ftl.table[(uint8_t)(TBL_LOG_MIRROR - table_shift(zone))];
    primary = replaced_block(z->log_block + (zone << 8));
    mirror  = replaced_block(mirror);

    primary = block_sector((uint16_t)primary);
    mirror  = block_sector((uint16_t)mirror);

    if (z->log_page < sec_per_block() / sec_per_page_raw())
    {
        uint32_t page = z->log_page * sec_per_page_raw();

        flash_program(page + primary, ftl.table, NULL, 1);
        flash_program(page + mirror, ftl.table, NULL, 1);
    }
    else
    {
        z->log_page = 0;

        if (flash_erase(primary) != 0)
        {
            primary = relocate_block(primary, NULL, 0);
        }
        flash_program(primary, ftl.table, log_block_meta, 1);

        if (flash_erase(mirror) != 0)
        {
            mirror = relocate_block(mirror, NULL, 0);
        }
        flash_program(mirror, ftl.table, log_block_meta, 1);
    }
}

/* ---- exchange slots ---- */

/* Finish the rewrite a slot is doing: copy the rest of the old block into
 * the exchange block, and free the slot. */
static void exch_close(struct exch_slot *s)
{
    if (s->open)
    {
        if (s->cursor != s->end)
        {
            uint32_t from = s->src + s->cursor % sec_per_block();
            uint16_t count = (uint16_t)(s->end - s->cursor);

            if (flash_copy(from, s->cursor, count) != 0)
            {
                uint32_t dest = relocate_block(s->cursor, NULL, 0);

                if (dest != s->cursor)
                {
                    flash_copy(from, dest, count);
                }
            }
        }
        s->open = false;
    }
}

/* Start rewriting the logical block holding `lba`: close any slot already
 * writing into its current block, take the least recently used slot, swap
 * the slot's pool block in, erase it, and publish the table. Returns the
 * slot's index.
 *
 * The slot ages form a permutation of 0 .. EXCH_SLOTS-1, so there always is
 * a slot of age 0 to take. */
static int exch_open(uint32_t lba)
{
    uint16_t zone, sector;
    uint8_t block, pool, fresh;
    struct exch_slot *s = NULL;
    uint32_t addr;
    int i;

    locate(lba, &zone, &block, &sector);

    for (i = 0; i < EXCH_SLOTS; i++)
    {
        s = &ftl.zones[zone].exch[i];

        if (s->open)
        {
            addr = map_committed(lba) & ~(sec_per_block() - 1);
            if ((s->cursor & ~(sec_per_block() - 1)) == addr)
            {
                exch_close(s);
            }
        }
    }

    for (i = 0; i < EXCH_SLOTS; i++)
    {
        s = &ftl.zones[zone].exch[i];

        if (!s->open || s->age == 0)
        {
            exch_close(s);
            break;
        }
    }

    /* swap the pool block in; the swap log records (old, new) */
    pool = (uint8_t)(ftl.reserve_base - table_shift(zone) + i);
    fresh = ftl.table[pool];
    ftl.table[pool] = ftl.table[block];
    ftl.table[TBL_SWAP_LOG + i * 2]     = ftl.table[block];
    ftl.table[TBL_SWAP_LOG + i * 2 + 1] = fresh;
    ftl.table[block] = fresh;

    addr = replaced_block((zone << 8) + fresh);
    addr = block_sector((uint16_t)addr);

    if (flash_erase(addr) != 0)
    {
        addr = relocate_block(addr, NULL, 0);
    }

    s->cursor = addr;
    s->end = addr + sec_per_block();
    s->open = true;

    publish_table(lba);

    return i;
}

/* The exchange slot sector `lba` - the first of a page - is written through,
 * opening one if needed. Pages are written in order: if the slot's cursor is
 * behind `lba`, the pages in between are copied forward from the old block
 * first. */
static struct exch_slot *exch_for_write(uint32_t lba)
{
    uint16_t zone, sector;
    uint8_t block;
    uint32_t addr, in_block;
    struct exch_slot *s = NULL;
    int i;

    locate(lba, &zone, &block, &sector);
    addr = map_committed(lba);

    for (i = 0; i < EXCH_SLOTS; i++)
    {
        s = &ftl.zones[zone].exch[i];

        if (!s->open || s->cursor > addr || s->end <= addr)
        {
            continue;
        }

        if (s->cursor != addr)
        {
            uint16_t count = (uint16_t)(addr - s->cursor);

            in_block = s->cursor % sec_per_block();
            if (flash_copy(s->src + in_block, s->cursor, count) != 0)
            {
                uint32_t dest = relocate_block(s->cursor, NULL, 0);

                if (dest != s->cursor)
                {
                    flash_copy(s->src + in_block, dest, count);
                    addr = addr - s->cursor + dest;
                }
            }
            s->cursor = addr;
        }
        break;
    }

    if (i >= EXCH_SLOTS)
    {
        i = exch_open(lba);
        s = &ftl.zones[zone].exch[i];

        in_block = addr % sec_per_block();
        s->src = addr - in_block;

        if (in_block != 0)
        {
            if (flash_copy(s->src, s->cursor, (uint16_t)in_block) != 0)
            {
                uint32_t dest = relocate_block(s->cursor, NULL, 0);

                if (dest != s->cursor)
                {
                    flash_copy(s->src, dest, (uint16_t)in_block);
                    s->cursor = dest;
                }
            }
        }

        s->cursor += in_block;
    }

    for (i = 0; i < EXCH_SLOTS; i++)
    {
        struct exch_slot *other = &ftl.zones[zone].exch[i];

        if (other->age > s->age)
        {
            other->age--;
        }
    }
    s->age = EXCH_SLOTS - 1;

    return s;
}

/* ---- power-loss recovery ---- */

/* The number of sectors of an exchange block that hold data: everything up
 * to the last programmed page. A last page that does not read back cleanly
 * was cut short by the power loss and is not counted.
 *
 * That back-off computes page & ~sec_per_page, clearing a single bit, where
 * aligning down to a page would need page & ~(sec_per_page - 1). It is the
 * original firmware's arithmetic, kept so that recovery merges exactly the
 * pages the original firmware would. */
static int32_t exch_written(uint32_t src)
{
    uint8_t meta[FLASH_META_SIZE];
    int32_t page = (int16_t)(sec_per_block() - sec_per_page_raw());
    bool done = false;

    while (!done)
    {
        flash_read(src + page, NULL, meta, 1);

        if (meta[META_PROGRAMMED] != 0)
        {
            page = (int16_t)(page - sec_per_page_raw());
            done = page < 0;
        }
        else
        {
            if (flash_read(src + page, NULL, NULL, sec_per_page_raw()) != 0)
            {
                page = (int16_t)((page & ~sec_per_page()) - sec_per_page_raw());
            }
            done = true;
        }
    }

    return (int16_t)(page + sec_per_page_raw());
}

/* Finish, in every zone, the writes a power loss interrupted - see POWER
 * LOSS above. For each swap-log entry still open, the written pages of the
 * exchange block and the rest of the old block are merged into the zone's
 * spare block, which takes the logical block's place; the exchange block
 * returns to the pool and the old block becomes the new spare. */
static void recover_interrupted(void)
{
    uint32_t capacity = volume_sectors(FTL_A_VOL_SYS)
                        + volume_sectors(FTL_A_VOL_USER);
    uint32_t zone_blk = 0;
    bool done = false;

    while (!done && zone_blk < ftl.geo->total_blocks)
    {
        uint16_t zone = (uint16_t)(zone_blk >> 8);
        int slot;

        if (zone_blk + ZONE_BLOCKS <= ftl.boot_blocks)
        {
            zone_blk += ZONE_BLOCKS;        /* all boot area */
            continue;
        }

        if (zone_first_lba(zone) >= capacity)
        {
            done = true;                    /* no zone after this holds data */
            continue;
        }

        map_current(zone_first_lba(zone));     /* loads the zone's table */

        for (slot = 0; slot < EXCH_SLOTS; slot++)
        {
            uint8_t old = ftl.table[TBL_SWAP_LOG + slot * 2];
            uint8_t new = ftl.table[TBL_SWAP_LOG + slot * 2 + 1];
            uint8_t pool, spare, spare_pos;
            uint32_t exch, dest, orig, n;
            int32_t written;
            uint8_t at;

            if (old == new)
            {
                continue;                   /* closed */
            }

            exch = block_sector((uint16_t)replaced_block(zone_blk + new));
            written = exch_written(exch);

            if (sec_per_block() <= written)
            {
                continue;                   /* full: the write completed */
            }

            spare = (uint8_t)(ftl.reserve_base + EXCH_SLOTS
                              - table_shift(zone));
            pool  = (uint8_t)(ftl.reserve_base - table_shift(zone));

            /* undo the swap: the spare block takes the logical block's
             * place, the exchange block goes back to the pool, the old
             * block becomes the spare */
            spare_pos = ftl.table[spare];
            for (at = 0; at < pool && ftl.table[at] != new; at++)
            {
            }
            ftl.table[pool + slot] = ftl.table[at];
            ftl.table[at] = spare_pos;
            ftl.table[spare] = old;

            ftl.table[TBL_SWAP_LOG + slot * 2]     = 0xff;
            ftl.table[TBL_SWAP_LOG + slot * 2 + 1] = 0xff;

            dest = block_sector((uint16_t)replaced_block(zone_blk + spare_pos));
            orig = block_sector((uint16_t)replaced_block(zone_blk + old));

            flash_erase(dest);

            /* the written pages, where they read back; the old block's
             * where they do not */
            for (n = 0; n < (uint32_t)written; n += sec_per_page_raw())
            {
                if (flash_read(exch + n, NULL, NULL, sec_per_page_raw()) != 0)
                {
                    flash_copy(orig + n, dest + n, sec_per_page_raw());
                }
                else
                {
                    flash_copy(exch + n, dest + n, sec_per_page_raw());
                }
            }
            flash_copy(orig + written, dest + written,
                       (uint16_t)(sec_per_block() - written));

            publish_table(zone_first_lba(zone));
        }

        zone_blk += ZONE_BLOCKS;
    }
}

/* ---- recognising the layout ----
 *
 * The reserve base and the system offset differ between the firmware builds
 * that format devices, and neither is recorded. Both follow from the
 * structure:
 *
 *   system offset  the two log blocks are always the top two table entries,
 *                  shifted down by the offset in the system zone - so the
 *                  offset is 0xff less the index of the primary log block's
 *                  own position in its table
 *   base           opening a slot i parks the displaced block at
 *                  table[base + i] and records it in the swap log, so a used
 *                  slot gives base = index of its old block - i; and, needing
 *                  no write history, the system zone's table[base - 1] must
 *                  name an exchange record whose block counts are all at
 *                  most base, peaking at exactly base
 *
 * The second test decides, the first is only a hint. Only a unique answer,
 * cross-checked against every zone's log blocks, makes the device writable:
 * a wrong base or offset would publish tables that erase live data. */

/* Find a zone's log blocks - metadata type 0x52 - and the newest programmed
 * page of the first. Returns how many were found: 2 normally, 1 when a
 * power loss during a log recycle left one erased, 0 for none. */
static int probe_find_log(uint16_t zone_base, uint8_t *pos_a, uint8_t *pos_b,
                          uint16_t *newest)
{
    uint8_t meta[FLASH_META_SIZE];
    uint16_t pages = sec_per_block() / sec_per_page_raw();
    int pos, i, found = 0;

    *pos_a = 0xff;
    *pos_b = 0xff;
    *newest = 0;

    for (pos = 0xff; pos >= 0 && found < 2; pos--)
    {
        uint32_t sec = block_sector((uint16_t)(zone_base + pos));

        if (flash_read(sec, NULL, meta, 1) == 0
            && meta[META_TYPE] == META_TYPE_LOG)
        {
            if (found == 0)
            {
                *pos_a = (uint8_t)pos;
            }
            else
            {
                *pos_b = (uint8_t)pos;
            }
            found++;
        }
    }

    if (found > 0)
    {
        uint32_t base = block_sector((uint16_t)(zone_base + *pos_a));

        for (i = 0; i < (int)pages; i++)
        {
            if (flash_read(base + (uint32_t)i * sec_per_page_raw(),
                           NULL, meta, 1) != 0)
            {
                break;
            }
            if (meta[META_PROGRAMMED] != META_ERASED)
            {
                *newest = (uint16_t)i;
            }
        }
    }
    return found;
}

/* The base the swap log points to. -1 with no evidence, -2 if slots
 * disagree. Only values that occur once in the table count: a formatted
 * system zone repeats some. */
static int probe_base(const uint8_t *tbl, int *votes)
{
    int i, j, base = -1;
    bool conflict = false;

    *votes = 0;
    for (i = 0; i < EXCH_SLOTS && !conflict; i++)
    {
        uint8_t old = tbl[TBL_SWAP_LOG + i * 2];
        uint8_t new = tbl[TBL_SWAP_LOG + i * 2 + 1];
        int at = -1, count = 0;

        if (old == new)
        {
            continue;                       /* unused slot */
        }

        for (j = 0; j < ZONE_BLOCKS; j++)
        {
            if (tbl[j] == old)
            {
                count++;
                at = j;
            }
        }
        if (count != 1 || at - i < 0)
        {
            continue;
        }

        if (base >= 0 && base != at - i)
        {
            conflict = true;
        }
        else
        {
            base = at - i;
            (*votes)++;
        }
    }

    if (conflict)
    {
        base = -2;
    }
    else if (*votes == 0)
    {
        base = -1;
    }
    return base;
}

/* The system offset from where the log blocks sit. -1 if they do not sit as
 * the top two entries of some shift. */
static int probe_sys_offset(const uint8_t *tbl, uint8_t log_primary,
                            uint8_t log_mirror)
{
    int i, at = -1, count = 0, offset = -1;

    for (i = 0; i < ZONE_BLOCKS; i++)
    {
        if (tbl[i] == log_primary)
        {
            count++;
            at = i;
        }
    }
    if (count == 1 && at >= 1 && tbl[at - 1] == log_mirror)
    {
        offset = TBL_LOG_PRIMARY - at;
    }
    return offset;
}

/* Does `base` make the system zone's exchange record a real one? Its block
 * counts must all be at most base, and reach it:
 *
 *     CP3     d9 f1 f1 f1 f1 ... f1        base f1
 *     rk2705  df f6 f6 f7 f6 f7 ... f7 f4  base f7
 *
 * A wrong base names some other block, whose bytes are not that.
 * The newest page is found as mount_scan() finds it, including ignoring the
 * read status: erased pages fail ECC, and only byte 0 decides. */
static bool probe_base_fits(uint8_t base, const uint8_t *sys_tbl,
                            uint8_t sys_zone, uint8_t sys_offset,
                            uint16_t nzones, uint8_t *page)
{
    uint32_t addr;
    int idx, z, max = -1;
    bool fits = true;

    addr = block_sector((uint16_t)(sys_tbl[(uint8_t)(base - 1 - sys_offset)] +
                                   (sys_zone << 8)));

    for (idx = REPLACEMENT_SLOTS - 1; ; idx--)
    {
        flash_read(addr + (uint32_t)idx * sec_per_page_raw(), page, NULL, 1);
        if (page[0] != META_ERASED || idx == 0)
        {
            break;
        }
    }

    for (z = 0; z < nzones && fits; z++)
    {
        if (page[z] > base)
        {
            fits = false;
        }
        else if (page[z] > max)
        {
            max = page[z];
        }
    }
    return fits && max == base;
}

/* Read zone z's table version `newest` from log block `log_blk`. */
static int probe_read_table(uint16_t z, uint8_t log_blk, uint16_t newest,
                            uint8_t *tbl)
{
    return flash_read(block_sector((uint16_t)((z << 8) + log_blk)) +
                          (uint32_t)newest * sec_per_page_raw(),
                      tbl, NULL, 1);
}

/* With one log block left, could the system offset be `so`? The top `so`
 * entries of the system zone's table are the boot blocks, recorded as
 * positions below `so` (formatting zeroes them, so they repeat), and the
 * lost log block's position cannot be one of them. */
static bool probe_layout_fits(const uint8_t *tbl, int so,
                              bool survivor_is_primary)
{
    int j;
    bool fits = true;
    uint8_t other;

    for (j = ZONE_BLOCKS - so; j <= TBL_LOG_PRIMARY; j++)
    {
        if (tbl[j] >= so)
        {
            fits = false;
        }
    }

    other = survivor_is_primary ? tbl[TBL_LOG_MIRROR - so]
                                : tbl[TBL_LOG_PRIMARY - so];
    return fits && other >= so;
}

/* How many bases fit if the system zone is `sz` with offset `so`; *found
 * gets the last that did. The pool and the spare must fit under the log
 * blocks, and table[base - 1 - so] must be a real entry. */
static int probe_base_search(const uint8_t *sys_tbl, uint8_t sz, uint8_t so,
                             uint16_t nzones, int *found)
{
    static uint8_t page[TABLE_SIZE];
    int b, fits = 0;

    for (b = so + 1; b + EXCH_SLOTS < TBL_LOG_MIRROR; b++)
    {
        if (probe_base_fits((uint8_t)b, sys_tbl, sz, so, nzones, page))
        {
            fits++;
            *found = b;
        }
    }
    return fits;
}

/* Recognise the layout of `nzones` zones - see probe_layout(). */
static bool probe_zones(uint16_t nzones)
{
    static uint8_t tbl[TABLE_SIZE];
    uint8_t  log_a[MAX_ZONES], log_b[MAX_ZONES];
    uint8_t  nlog[MAX_ZONES];           /* log blocks found: 0, 1 or 2 */
    uint16_t newest[MAX_ZONES];
    int16_t  zbase[MAX_ZONES];
    int16_t  zoffset[MAX_ZONES];
    int16_t  zcand[MAX_ZONES][2];       /* one-log zones: offset candidates */
    uint16_t z;
    int base_hi = -1, sys_zone = -1, i, confirm = 0, disagree = 0;
    int found = -1;
    bool usable = false;

    for (z = 0; z < nzones; z++)
    {
        int votes;

        zbase[z] = -1;
        zoffset[z] = -1;
        zcand[z][0] = zcand[z][1] = -1;

        nlog[z] = (uint8_t)probe_find_log((uint16_t)(z << 8), &log_a[z],
                                          &log_b[z], &newest[z]);
        if (nlog[z] == 0)
        {
            continue;
        }
        if (probe_read_table(z, log_a[z], newest[z], tbl) != 0)
        {
            nlog[z] = 0;
            continue;
        }

        zbase[z] = (int16_t)probe_base(tbl, &votes);

        if (nlog[z] == 2)
        {
            zoffset[z] = (int16_t)probe_sys_offset(tbl, log_a[z], log_b[z]);
        }
        else
        {
            /* One survivor: the primary at table[0xff - offset] or the
             * mirror at table[0xfe - offset]; settled below. */
            int j, at = -1, count = 0;

            for (j = 0; j < ZONE_BLOCKS; j++)
            {
                if (tbl[j] == log_a[z])
                {
                    count++;
                    at = j;
                }
            }
            if (count == 1)
            {
                zcand[z][0] = (int16_t)(TBL_LOG_PRIMARY - at);
                zcand[z][1] = (int16_t)((at <= TBL_LOG_MIRROR)
                                        ? TBL_LOG_MIRROR - at : -1);
            }
        }

        if (zbase[z] >= 0 && zbase[z] > base_hi)
        {
            base_hi = zbase[z];
        }
    }

    /* the offset is only visible in the system zone; elsewhere it is 0 */
    for (z = 0; z < nzones; z++)
    {
        if (zoffset[z] > 0 && (int)z > sys_zone)
        {
            sys_zone = (int)z;
        }
    }

    if (sys_zone >= 0)
    {
        if (probe_read_table((uint16_t)sys_zone, log_a[sys_zone],
                             newest[sys_zone], tbl) == 0)
        {
            ftl.base_fits = (uint8_t)probe_base_search(
                tbl, (uint8_t)sys_zone, (uint8_t)zoffset[sys_zone],
                nzones, &found);
        }
    }
    else
    {
        /* No zone shows an offset: it is 0, or the system zone is one that
         * lost a log block. Try each one-log candidate; only a unique
         * winner counts. */
        int wins = 0, win_z = -1, win_so = -1, win_base = -1;

        for (z = 0; z < nzones; z++)
        {
            for (i = 0; i < 2; i++)
            {
                int f = -1, so = zcand[z][i];

                if (nlog[z] != 1 || so <= 0)
                {
                    continue;
                }
                if (probe_read_table(z, log_a[z], newest[z], tbl) != 0)
                {
                    continue;
                }
                if (!probe_layout_fits(tbl, so, i == 0))
                {
                    continue;
                }
                if (probe_base_search(tbl, (uint8_t)z, (uint8_t)so,
                                      nzones, &f) == 1)
                {
                    wins++;
                    win_z = z;
                    win_so = so;
                    win_base = f;
                }
            }
        }

        if (wins == 1)
        {
            sys_zone = win_z;
            zoffset[win_z] = (int16_t)win_so;
            ftl.base_fits = 1;
            found = win_base;
        }
        else if (wins == 0 && nlog[0] &&
                 probe_read_table(0, log_a[0], newest[0], tbl) == 0)
        {
            ftl.base_fits = (uint8_t)probe_base_search(tbl, 0, 0, nzones,
                                                       &found);
        }
    }

    if (sys_zone < 0)
    {
        ftl.boot_blocks = 0;
    }
    else
    {
        ftl.boot_blocks = (uint16_t)((sys_zone << 8)
                                     | (uint8_t)zoffset[sys_zone]);
    }

    if (ftl.base_fits == 1)
    {
        ftl.reserve_base = (uint8_t)found;
        usable = true;
    }
    else if (base_hi >= 0)
    {
        ftl.reserve_base = (uint8_t)base_hi;    /* unconfirmed: read-only */
        usable = true;
    }

    if (usable)
    {
        /* Cross-check: every zone's log blocks must sit at the top two
         * entries, as shifted. A zone that lost one confirms if the
         * survivor sits at either. */
        for (z = 0; z < nzones; z++)
        {
            uint8_t shift;
            bool ok;

            if (nlog[z] == 0
                || (nlog[z] == 2 && zbase[z] < 0 && zoffset[z] < 0))
            {
                continue;
            }
            shift = (z <= (ftl.boot_blocks >> 8)) ? (uint8_t)ftl.boot_blocks
                                                  : 0;
            if (probe_read_table(z, log_a[z], newest[z], tbl) != 0)
            {
                continue;
            }
            if (nlog[z] == 2)
            {
                ok = tbl[TBL_LOG_PRIMARY - shift] == log_a[z] &&
                     tbl[TBL_LOG_MIRROR - shift] == log_b[z];
            }
            else
            {
                ok = tbl[TBL_LOG_PRIMARY - shift] == log_a[z] ||
                     tbl[TBL_LOG_MIRROR - shift] == log_a[z];
            }

            if (ok)
            {
                confirm++;
            }
            else
            {
                disagree++;
            }
        }

        /* an offset of 0 is only measured if the system zone itself voted */
        if (disagree || confirm == 0 || ftl.base_fits != 1 ||
            (sys_zone < 0 && zoffset[0] < 0))
        {
            ftl.confidence = FTL_A_CONF_ASSUMED;
        }
        else
        {
            ftl.confidence = FTL_A_CONF_CONFIRMED;
            set_write_protect(ftl.config.read_only);
        }
    }

    return usable;
}

/* Recognise the layout. Returns true if the device is usable - read-only
 * unless the layout was confirmed (ftl.confidence). */
static bool probe_layout(void)
{
    uint16_t nzones = (uint16_t)(ftl.geo->total_blocks >> 8);
    bool usable = false;

    ftl.confidence = FTL_A_CONF_NONE;
    set_write_protect(true);
    ftl.base_fits = 0;

    if (nzones > 0 && nzones <= MAX_ZONES)
    {
        usable = probe_zones(nzones);
    }
    return usable;
}

/* ---- mounting ---- */

/* Find the newest table version in log block `sec`: the last programmed
 * page, by binary search on the "programmed" metadata byte. A page whose
 * neighbour below is programmed but unreadable is taken as cut short, and
 * the search settles one lower. Returns the page count, 0 if nothing was
 * found. */
static uint32_t log_versions(uint32_t sec, uint8_t *buf)
{
    uint32_t pages = sec_per_block() / sec_per_page_raw();
    uint32_t lo = 0, hi = pages - 1, mid, count = 0;
    bool done = false;

    while (lo <= hi && !done)
    {
        mid = (lo + hi) / 2;
        flash_read(sec + mid * sec_per_page_raw(), NULL, buf, 1);

        if (buf[META_PROGRAMMED] != META_ERASED)
        {
            if (mid == pages - 1)
            {
                count = mid + 1;
                done = true;
            }
            else
            {
                lo = mid + 1;
            }
        }
        else if (mid == 0)
        {
            count = 1;
            done = true;
        }
        else
        {
            uint32_t below = mid - 1;
            int rc;

            hi = below;
            rc = flash_read(sec + below * sec_per_page_raw(), NULL, buf, 1);
            if (buf[META_PROGRAMMED] != META_ERASED)
            {
                count = (rc == 0) ? mid : mid - 1;
                done = true;
            }
        }
    }
    return count;
}

/* Load every zone's log position, repairing log mirrors that disagree, then
 * the device state from the system zone's table and the exchange record.
 * Runs after probe_layout(). */
static enum ftl_a_error mount_scan(void)
{
    static uint8_t buf[TABLE_SIZE];
    enum ftl_a_error err = FTL_A_OK;
    uint8_t off_primary = (uint8_t)(TBL_LOG_PRIMARY - system_offset());
    uint8_t off_mirror  = (uint8_t)(TBL_LOG_MIRROR - system_offset());
    uint32_t zone_blk;
    uint32_t addr;
    int i;

    for (zone_blk = 0; zone_blk < ftl.geo->total_blocks && err == FTL_A_OK;
         zone_blk += ZONE_BLOCKS)
    {
        uint8_t log_pos[2]  = { 0xff, 0xff };
        uint8_t log_page[2] = { 0xff, 0xff };
        uint8_t found = 0;
        int pos = 0xff;
        uint16_t zone = (uint16_t)(zone_blk >> 8);
        struct zone *z;

        if (zone_blk + ZONE_BLOCKS <= ftl.boot_blocks)
        {
            continue;                       /* all boot area */
        }

        /* the log blocks, from the top; positions 0-2 are never searched */
        while (pos > 2)
        {
            uint32_t sec = block_sector((uint16_t)(zone_blk + pos));

            if (!block_is_bad(sec))
            {
                flash_read(sec, NULL, buf, 1);
                if (buf[META_TYPE] == META_TYPE_LOG)
                {
                    log_pos[found] = (uint8_t)pos;
                    log_page[found] = (uint8_t)(log_versions(sec, buf) - 1);
                    found++;
                    if (found >= 2)
                    {
                        break;
                    }
                }
            }
            pos--;
        }

        if (pos <= 2 && found == 0)
        {
            err = FTL_A_ERR_NO_LOG_BLOCK;
            continue;
        }

        z = &ftl.zones[zone];
        z->log_block = log_pos[0];

        if (log_page[0] == log_page[1])
        {
            z->log_page = log_page[0];
            continue;
        }

        /* The two logs disagree: a power loss during an append or a
         * recycle. Rewrite both from the newest version. With one log left,
         * where the two belong comes from that table. */
        if (log_pos[1] == 0xff)
        {
            addr = block_sector((uint16_t)(zone_blk + log_pos[0])) +
                   log_page[0] * sec_per_page_raw();
        }
        else
        {
            addr = block_sector((uint16_t)(zone_blk + log_pos[1])) +
                   log_page[1] * sec_per_page_raw();
        }
        flash_read(addr, buf, NULL, 1);

        if (log_pos[1] == 0xff)
        {
            if (zone <= system_zone())
            {
                log_pos[0] = buf[off_primary];
                log_pos[1] = buf[off_mirror];
            }
            else
            {
                log_pos[0] = buf[TBL_LOG_PRIMARY];
                log_pos[1] = buf[TBL_LOG_MIRROR];
            }
            z->log_block = log_pos[0];
        }

        /* A read-only mount cannot repair, and half a repair is worse than
         * none - a refused erase leaves log_page pointing at a page never
         * written, and every lookup in the zone reads garbage. */
        if (ftl.write_protect)
        {
            err = FTL_A_ERR_NEEDS_REPAIR;
            continue;
        }

        addr = block_sector((uint16_t)(zone_blk + log_pos[0]));
        if (flash_erase(addr) != 0)
        {
            addr = relocate_block(addr, NULL, 0);
        }
        flash_program(addr, buf, log_block_meta, 1);

        addr = block_sector((uint16_t)(zone_blk + log_pos[1]));
        if (flash_erase(addr) != 0)
        {
            addr = relocate_block(addr, NULL, 0);
        }
        flash_program(addr, buf, log_block_meta, 1);

        z->log_page = 0;
    }

    if (err == FTL_A_OK)
    {
        uint32_t idx = REPLACEMENT_SLOTS - 1;
        uint32_t nzones = ftl.geo->total_blocks >> 8;
        uint32_t z;

        flash_read(system_table_sector(), buf, NULL, 1);
        ftl.format_flag = buf[TBL_FORMAT_FLAG];

        /* The exchange record's newest page is searched for from page 9
         * down, by its byte 0 - which relocate_block() never writes; so it
         * always settles on page 0, and the replaced-block list is always
         * the one formatting left, i.e. empty. As in the original firmware:
         * a replaced block is found bad again rather than remembered. */
        addr = block_sector((uint16_t)(buf[xrec_index()]
                                       + (system_zone() << 8)));
        for (;;)
        {
            flash_read(addr + idx * sec_per_page_raw(), buf, NULL, 1);
            if (buf[0] != META_ERASED || idx == 0)
            {
                break;
            }
            idx--;
        }

        for (i = 0; i < REPLACEMENT_SLOTS; i++)
        {
            ftl.replaced[i] = (uint16_t)(buf[XREC_REPLACED + i * 2] |
                                         (buf[XREC_REPLACED + i * 2 + 1] << 8));
        }

        for (i = 0; i < MAX_ZONES; i++)
        {
            ftl.zones[i].block_count = buf[i];
        }

        ftl.logical_sectors = 0;
        for (z = 0; z < nzones && z < MAX_ZONES; z++)
        {
            ftl.logical_sectors += ftl.zones[z].block_count;
        }
        ftl.logical_sectors *= sec_per_block();

        if (ftl.logical_sectors == 0)
        {
            err = FTL_A_ERR_ZERO_CAPACITY;
        }
        else
        {
            /* A count of 0xff is an erased page, not a zone: every zone
             * keeps at least its log blocks and pool out of its 256. */
            for (z = 0; z < nzones && z < MAX_ZONES; z++)
            {
                if (ftl.zones[z].block_count == 0xff)
                {
                    err = FTL_A_ERR_COUNTS_ERASED;
                }
            }
        }
    }

    return err;
}

/* ---- reading and writing ---- */

static int read_sectors(int volume, uint32_t lba, uint8_t *p, uint16_t count)
{
    int ret = 0;

    if (volume == FTL_A_VOL_USER)
    {
        lba += volume_sectors(FTL_A_VOL_SYS);
    }

    while (count)
    {
        uint32_t addr = map_current(lba);
        struct write_cache *hit = NULL;
        uint16_t len;
        int i;

        /* a page being assembled holds the newest copy of what it holds */
        for (i = 0; i < WRITE_CACHE_PAGES; i++)
        {
            struct write_cache *c = &ftl.cache[i];

            uint32_t held = c->next_lba & ~(uint32_t)(sec_per_page() - 1);

            if (c->valid && held <= lba && c->next_lba > lba)
            {
                hit = c;
                break;
            }
        }

        if (hit == NULL)
        {
            uint16_t in_page = (uint16_t)(lba % sec_per_page_raw());

            len = sec_per_page_raw() - in_page;
            if (len > count)
            {
                len = count;
            }
            if (flash_read(addr, p, NULL, len) != 0)
            {
                ret = 1;
            }
        }
        else
        {
            uint16_t in_page = (uint16_t)(lba % sec_per_page());

            len = (uint16_t)(hit->next_lba % sec_per_page() - in_page);
            if (len > count)
            {
                len = count;
            }
            memcpy(p, hit->buf + in_page * FLASH_SECTOR_SIZE,
                   (size_t)len * FLASH_SECTOR_SIZE);
        }

        p += (uint32_t)len * FLASH_SECTOR_SIZE;
        count -= len;
        lba += len;
    }

    return ret;
}

/* Write a cached page out: complete it from the block being replaced, and
 * program it at its exchange slot's cursor. A page that holds no sector yet
 * is dropped. */
static void cache_flush(struct write_cache *c)
{
    if (c->valid)
    {
        uint32_t lba = c->next_lba;
        uint32_t addr = map_committed(lba);
        uint8_t spp = sec_per_page(), sppr = sec_per_page_raw();
        uint16_t in_page = (uint16_t)(lba & (spp - 1));

        if (in_page != 0)
        {
            uint32_t page_base = (lba % sec_per_block()) & ~(uint32_t)(spp - 1);
            struct exch_slot *s = c->slot;

            /* the slot may have moved on since; take the current one. The
             * original firmware does not store it back - neither does this,
             * the page is done with it. */
            if ((addr & ~(uint32_t)(spp - 1)) != s->cursor)
            {
                s = exch_for_write(lba & ~(uint32_t)(spp - 1));
            }

            if (in_page <= sppr && ftl.geo->planes > 1)
            {
                flash_read(s->src + page_base + in_page,
                           c->buf + (uint32_t)in_page * FLASH_SECTOR_SIZE, NULL,
                           (uint8_t)(sppr - in_page));
                flash_read(s->src + page_base + sppr,
                           c->buf + (uint32_t)sppr * FLASH_SECTOR_SIZE,
                           NULL, sppr);
            }
            else
            {
                flash_read(s->src + page_base + in_page,
                           c->buf + (uint32_t)in_page * FLASH_SECTOR_SIZE, NULL,
                           (uint8_t)(spp - in_page));
            }

            flash_program_page(s->cursor, c->buf, NULL);
            s->cursor += spp;
        }
        c->valid = false;
    }
}

static void cache_flush_all(void)
{
    int i;

    for (i = 0; i < WRITE_CACHE_PAGES; i++)
    {
        cache_flush(&ftl.cache[i]);
    }
}

/* Stage sectors into a page buffer; a completed page goes to its exchange
 * slot. A buffer is taken - in this order - from one already holding the
 * start of the same page (flushed first unless it ends right where this
 * write begins), from the free ones, or from the least recently used. */
static int write_sectors(int volume, uint32_t lba, const uint8_t *p,
                         uint16_t count)
{
    struct write_cache *slot = NULL;
    uint32_t page_mask = sec_per_page() - 1;
    uint32_t page_lba;
    int i;

    if (volume == FTL_A_VOL_USER)
    {
        lba += volume_sectors(FTL_A_VOL_SYS);
    }

    /* The original firmware resolves the first sector here, before anything
     * else - it wants the chip it is on. Kept: it loads the zone's table,
     * and the flash access of this driver was validated operation for
     * operation against the original's. */
    map_current(lba);

    page_lba = lba & ~page_mask;
    for (i = 0; i < WRITE_CACHE_PAGES && slot == NULL; i++)
    {
        struct write_cache *c = &ftl.cache[i];

        if (c->valid && (c->next_lba & ~page_mask) == page_lba)
        {
            if (c->next_lba != lba)
            {
                cache_flush(c);
            }
            slot = c;
        }
    }

    for (i = 0; i < WRITE_CACHE_PAGES && slot == NULL; i++)
    {
        if (!ftl.cache[i].valid)
        {
            slot = &ftl.cache[i];
        }
    }

    for (i = 0; i < WRITE_CACHE_PAGES && slot == NULL; i++)
    {
        if (ftl.cache[i].age == 0)
        {
            cache_flush(&ftl.cache[i]);
            slot = &ftl.cache[i];
        }
    }

    /* The ages form a permutation, so the least recently used buffer - age
     * 0 - always exists. They have to be updated whichever way the buffer
     * was chosen for that to hold. */
    for (i = 0; i < WRITE_CACHE_PAGES; i++)
    {
        if (ftl.cache[i].age > slot->age)
        {
            ftl.cache[i].age--;
        }
    }
    slot->age = WRITE_CACHE_PAGES - 1;

    /* A page further on in this write may still sit, part written, in
     * another buffer - left there by an earlier write that ended inside it.
     * This write reaches that page through its own buffer, so the old one
     * goes to flash first: otherwise two buffers hold the page, reads find
     * the older, and its later flush programs stale data over the newer.
     * The original firmware lacks this and loses data that way. */
    for (i = 0; i < WRITE_CACHE_PAGES; i++)
    {
        struct write_cache *c = &ftl.cache[i];
        uint32_t page = c->next_lba & ~page_mask;

        if (c != slot && c->valid && page >= page_lba && page < lba + count)
        {
            cache_flush(c);
        }
    }

    while (count)
    {
        uint32_t in_block = lba % sec_per_block();
        struct exch_slot *s = exch_for_write(lba & ~page_mask);

        slot->slot = s;

        if (!slot->valid)
        {
            /* start the page with what precedes this sector in it */
            uint16_t in_page = (uint16_t)(lba & page_mask);
            uint8_t sppr = sec_per_page_raw();

            if (in_page <= sppr)
            {
                flash_read(s->src + (in_block & ~page_mask), slot->buf, NULL,
                           (uint8_t)in_page);
            }
            else
            {
                uint32_t page_base = (s->src + in_block) & ~page_mask;

                flash_read(page_base, slot->buf, NULL, sppr);
                flash_read(page_base + sppr,
                           slot->buf + (uint32_t)sppr * FLASH_SECTOR_SIZE,
                           NULL, (uint8_t)(in_page - sppr));
            }
            slot->valid = true;
        }

        memcpy(slot->buf + (lba & page_mask) * FLASH_SECTOR_SIZE, p,
               FLASH_SECTOR_SIZE);

        if ((page_mask & ~lba) == 0)
        {
            /* the last sector of the page */
            slot->valid = false;
            flash_program_page(s->cursor, slot->buf, NULL);
            s->cursor += sec_per_page();
            if (s->cursor == s->end)
            {
                exch_close(s);
            }
        }

        p += FLASH_SECTOR_SIZE;
        count--;
        lba++;
        slot->next_lba = lba;
    }

    return 0;
}

/* ---- interface ---- */

enum ftl_a_error ftl_a_mount(const struct ftl_a_config *config)
{
    enum ftl_a_error err;

    ftl.geo = flash_get_geometry();
    ftl.config = *config;
    ftl.mounted = false;

    reset_state();

    if (!probe_layout())
    {
        err = FTL_A_ERR_LAYOUT;
    }
    else
    {
        /* the flash layer keeps writes out of the boot area, as the
         * original firmware does - all writes, if the area is implausibly
         * small */
        flash_set_boot_area(ftl.boot_blocks < 3 ? UINT32_MAX :
                            (uint32_t)ftl.boot_blocks * sec_per_block());

        err = mount_scan();
    }

    if (err == FTL_A_OK && ftl.config.sys_sectors >= ftl.geo->total_sectors)
    {
        err = FTL_A_ERR_SYS_TOO_BIG;
    }

    if (err == FTL_A_OK && ftl.format_flag != FORMAT_FLAG)
    {
        /* Another generation's flag, when the caller knows it. The older
         * rk2705 NAND bootloader formats with 1; its write logic is the same
         * as the flag-2 generation's, but writing such media stays the
         * caller's decision. */
        if (ftl.config.alt_format_flag == 0 ||
            ftl.format_flag != ftl.config.alt_format_flag)
        {
            err = FTL_A_ERR_FORMAT_FLAG;
        }
        else if (!ftl.config.alt_format_writable)
        {
            set_write_protect(true);
        }
    }

    if (err == FTL_A_OK)
    {
        /* Recovery writes, and a wrong layout would make it relocate data
         * into the wrong blocks: only on a confirmed one. Leaving an
         * interrupted write unresolved is the safe side. */
        if (!ftl.write_protect)
        {
            recover_interrupted();
        }
        ftl.mounted = true;
    }

    ftl.error = err;
    return err;
}

void ftl_a_get_status(struct ftl_a_status *status)
{
    status->error        = ftl.error;
    status->confidence   = ftl.confidence;
    status->writable     = !ftl.write_protect;
    status->reserve_base = ftl.reserve_base;
    status->sys_zone     = system_zone();
    status->sys_offset   = system_offset();
    status->base_fits    = ftl.base_fits;
    status->format_flag  = ftl.format_flag;
}

uint32_t ftl_a_capacity(int volume)
{
    uint32_t sectors = 0;

    if (ftl.mounted)
    {
        sectors = volume_sectors(volume);
    }
    return sectors;
}

/* Requests are split in pieces the page-sized internals count in 16 bits. */
#define MAX_REQUEST 0x8000

int ftl_a_read(int volume, uint32_t sector, void *buf, uint32_t count)
{
    uint8_t *p = buf;
    int ret = 0;

    if (!ftl.mounted || (volume != FTL_A_VOL_SYS && volume != FTL_A_VOL_USER) ||
        sector + count > volume_sectors(volume))
    {
        ret = 1;
    }

    while (ret == 0 && count > 0)
    {
        uint16_t n = (count > MAX_REQUEST) ? MAX_REQUEST : (uint16_t)count;

        ret = read_sectors(volume, sector, p, n);
        sector += n;
        count -= n;
        p += (size_t)n * FLASH_SECTOR_SIZE;
    }
    return ret;
}

int ftl_a_write(int volume, uint32_t sector, const void *buf, uint32_t count)
{
    const uint8_t *p = buf;
    int ret = 0;

    if (!ftl.mounted || ftl.write_protect ||
        (volume != FTL_A_VOL_SYS && volume != FTL_A_VOL_USER) ||
        sector + count > volume_sectors(volume))
    {
        ret = 1;
    }

    while (ret == 0 && count > 0)
    {
        uint16_t n = (count > MAX_REQUEST) ? MAX_REQUEST : (uint16_t)count;

        ret = write_sectors(volume, sector, p, n);
        sector += n;
        count -= n;
        p += (size_t)n * FLASH_SECTOR_SIZE;
    }
    return ret;
}

void ftl_a_sync(void)
{
    if (ftl.mounted && !ftl.write_protect)
    {
        cache_flush_all();
    }
}
