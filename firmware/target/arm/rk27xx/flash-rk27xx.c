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

/* NAND access for the rk27xx flash translation layer - see flash-rk27xx.h.
 *
 * THE CONTROLLER
 *
 * Commands and addresses are written to the chip through FLASH_CMD/ADDR, one
 * cycle per write. Data moves a sector at a time between the chip and one of
 * four controller slots - 512 bytes of PAGE_BUF and 16 of SPARE_BUF each -
 * with the BCH engine in the path: on a read it corrects the sector and
 * reports the result in BCHST, on a program it computes the code into the
 * last 13 spare bytes. A transfer is started with FLCTL; the slot is chosen
 * by bits 3-4.
 *
 * A read latches a whole page in the chip, then streams its sectors out in
 * order from the first: reaching sector k of a page means transferring the
 * k before it too.
 *
 * A program sends the column, the sectors, then the confirm command, and
 * reads the chip's status: one program operation per page between erases,
 * as this MLC part allows no more (NOP = 1).
 *
 * The chip's write-protect line is lifted only for the duration of each
 * program or erase. */

#include "config.h"
#include "system.h"
#include "string.h"
#include "rk27xx.h"
#include "nand-target.h"
#include "flash-rk27xx.h"

#define CMD_READ_1ST     0x00
#define CMD_READ_2ND     0x30
#define CMD_PROG_1ST     0x80
#define CMD_PROG_2ND     0x10
#define CMD_ERASE_1ST    0x60
#define CMD_ERASE_2ND    0xD0
#define CMD_STATUS       0x70
#define NAND_STATUS_FAIL 0x01

#define SPARE_SIZE       16             /* spare bytes per sector on the chip */
#define SECTOR_STRIDE    (FLASH_SECTOR_SIZE + SPARE_SIZE)

/* Metadata byte 1 of every programmed sector: "programmed" */
#define META_PROGRAMMED  1

/* Transfer one sector between the chip and a controller slot: ECC on,
 * transfer on, bit 5 (region select), start. A write adds FL_WR, and the
 * BCH engine then encodes (BCH_WR) rather than decodes. */
#define FL_KICK_READ     (FL_COR_EN | FL_XFER_EN | (1<<5) | FL_START)
#define FL_KICK_WRITE    (FL_KICK_READ | FL_WR)

/* Busy limits: the datasheet maxima (tPROG 2.2 ms, tBERS 10 ms) with margin */
#define PROG_TIMEOUT_US  5000
#define ERASE_TIMEOUT_US 20000

static struct flash_geometry geo;
static bool ready;
static bool writable;
static bool meta_passthrough;
static uint32_t ecc_mode;               /* BCHCTL mode bits: 0 or BCH_T14 */
static uint32_t boot_area = UINT32_MAX;
static struct flash_stats stats;

int flash_layer_init(void)
{
    const struct flashspec_t *f = &flash_spec[0];
    int ret = 0;

    ready = false;

    if (f->total_phy_sec == 0 || f->sec_per_page_raw == 0 ||
        f->sec_per_block_raw == 0)
    {
        ret = 1;
    }
    /* the plane interleave below knows one plane or two */
    else if (f->mul_plane != 1 && f->mul_plane != 2)
    {
        ret = 2;
    }
    else if (f->sec_per_page_raw * f->mul_plane > FLASH_MAX_SEC_PER_PAGE)
    {
        ret = 3;
    }
    else
    {
        geo.planes            = f->mul_plane;
        geo.sec_per_page_raw  = f->sec_per_page_raw;
        geo.sec_per_page      = f->sec_per_page;
        geo.sec_per_block_raw = f->sec_per_block_raw;
        geo.sec_per_block     = f->sec_per_block;
        geo.total_blocks      = f->total_bloks;
        geo.total_sectors     = f->total_phy_sec;
        ready = true;
    }
    return ret;
}

const struct flash_geometry *flash_get_geometry(void)
{
    return &geo;
}

void flash_set_writable(bool on)
{
    writable = on;
}

void flash_set_boot_area(uint32_t sectors)
{
    boot_area = sectors;
}

void flash_set_meta_passthrough(bool on)
{
    meta_passthrough = on;
}

int flash_set_ecc(unsigned t)
{
    int ret = 0;

    if (t == 8)
    {
        ecc_mode = 0;
    }
    else if (t == 14)
    {
        ecc_mode = BCH_T14;
    }
    else
    {
        ret = 1;
    }
    return ret;
}

void flash_get_stats(struct flash_stats *out)
{
    *out = stats;
}

/* Map an FTL sector to a raw sector on the chip.
 *
 * On a two-plane part a super-block is a pair of physical blocks, and
 * consecutive pages alternate between them:
 *
 *   super-block  sec / (spb_raw * 2)
 *   within it    page  = L / (spp_raw * 2)
 *                plane = (L % (spp_raw * 2)) / spp_raw
 *                slot  = L % spp_raw
 *   raw = (super_block * 2 + plane) * spb_raw + page * spp_raw + slot
 *
 * On a single-plane part it is the identity. */
static uint32_t sec_to_raw(uint32_t sec)
{
    uint32_t raw = sec;

    if (geo.planes == 2)
    {
        uint32_t spb   = (uint32_t)geo.sec_per_block_raw * 2;
        uint32_t sb    = sec / spb;
        uint32_t l     = sec % spb;
        uint32_t page  = l / ((uint32_t)geo.sec_per_page_raw * 2);
        uint32_t plane = (l % ((uint32_t)geo.sec_per_page_raw * 2))
                         / geo.sec_per_page_raw;
        uint32_t slot  = l % geo.sec_per_page_raw;

        raw = (sb * 2 + plane) * geo.sec_per_block_raw
              + page * geo.sec_per_page_raw + slot;
    }
    return raw;
}

static void wait_flash_ready(void)
{
    while (!(FMCTL & FM_RDY))
    {
    }
}

/* R/B# drops within tWB (100 ns) of a confirm command, so wait that long
 * before polling, or a still-high line reads as done. */
static bool wait_ready_us(uint32_t us)
{
    bool timeout = false;

    udelay(1);
    while (!(FMCTL & FM_RDY) && !timeout)
    {
        if (us-- == 0)
        {
            stats.timeouts++;
            timeout = true;
        }
        else
        {
            udelay(1);
        }
    }
    return timeout;
}

static void send_row(uint32_t row)
{
    FLASH_ADDR(0) = row & 0xff;
    FLASH_ADDR(0) = (row >> 8) & 0xff;
    FLASH_ADDR(0) = (row >> 16) & 0xff;
}

/* Latch raw page `row` in the chip and prepare the ECC engine to decode in
 * `mode` (BCHCTL mode bits). */
static void latch_page(uint32_t row, uint32_t mode)
{
    flash_chip_select(0);
    wait_flash_ready();

    FLASH_CMD(0)  = CMD_READ_1ST;
    FLASH_ADDR(0) = 0x00;
    FLASH_ADDR(0) = 0x00;
    send_row(row);
    FLASH_CMD(0)  = CMD_READ_2ND;

    wait_flash_ready();

    BCHCTL = BCH_RST | mode;
}

/* Transfer the next sector of the latched page through slot `slot` & 3.
 * Returns the BCH status. */
static uint32_t read_next_sector(uint32_t slot, uint8_t *data, uint8_t *meta)
{
    uint32_t st;
    uint32_t buf = slot & 3;

    FLCTL = FL_KICK_READ | (buf << 3);
    while (!(FLCTL & FL_RDY))
    {
    }

    st = BCHST;

    if (data)
    {
        memcpy(data, (const void *)((uintptr_t)&PAGE_BUF + (buf << 9)),
               FLASH_SECTOR_SIZE);
    }
    if (meta)
    {
        memcpy(meta, (const void *)((uintptr_t)&SPARE_BUF + (buf << 4)),
               FLASH_META_SIZE);
    }
    return st;
}

/* Read sectors [first, first + n) of raw page `row`. Returns 1 if any was
 * uncorrectable. */
static int read_raw_run(uint32_t row, uint32_t first, uint32_t n,
                        uint8_t *data, uint8_t *meta, uint32_t mode)
{
    uint32_t j;
    int uncorrectable = 0;

    latch_page(row, mode);

    for (j = 0; j < first + n; j++)
    {
        bool wanted = j >= first;
        uint32_t k = j - first;
        uint8_t *d = (wanted && data) ? data + (size_t)k * FLASH_SECTOR_SIZE
                                      : NULL;
        uint8_t *m = (wanted && meta) ? meta + (size_t)k * FLASH_META_SIZE
                                      : NULL;
        uint32_t st = read_next_sector(j, d, m);

        if (wanted)
        {
            if (BCH_UNCORRECTABLE(st))
            {
                uncorrectable = 1;
            }
            else if (BCH_CORRECTED(st) >= BCH_REFRESH_THRESHOLD)
            {
                stats.refresh_pending++;
            }
        }
    }

    flash_chip_deselect();
    return uncorrectable;
}

int flash_read_raw(uint32_t raw_sec, void *data, void *meta)
{
    int ret = 1;

    if (ready && raw_sec < geo.total_sectors)
    {
        /* the boot area is t=8 on every device seen, whatever the FTL
         * area uses */
        ret = read_raw_run(raw_sec / geo.sec_per_page_raw,
                           raw_sec % geo.sec_per_page_raw, 1, data, meta, 0);
    }
    return ret;
}

int flash_read(uint32_t sec, void *data, void *meta, unsigned n)
{
    uint8_t *d = data;
    uint8_t *m = meta;
    unsigned i;
    int ret = ready ? 0 : 1;

    unsigned run;

    /* One page latch per run of sectors that lie consecutively in one raw
     * page. Latching per sector instead cost an array load per sector and,
     * the controller streaming a page from its first sector, a transfer of
     * every sector before it: 8 loads and 36 transfers for an 8-sector page
     * read sector by sector, against 1 and 8. On a two-plane part a run of
     * FTL sectors stays in one page until it moves on to the other plane. */
    for (i = 0; i < n && ready; i += run)
    {
        uint32_t raw = sec_to_raw(sec + i);
        uint32_t slot = raw % geo.sec_per_page_raw;

        if (raw >= geo.total_sectors)
        {
            ret = 1;
            break;
        }

        run = 1;
        while (i + run < n && slot + run < geo.sec_per_page_raw &&
               sec_to_raw(sec + i + run) == raw + run)
        {
            run++;
        }

        if (read_raw_run(raw / geo.sec_per_page_raw, slot, run,
                         d ? d + (size_t)i * FLASH_SECTOR_SIZE : NULL,
                         m ? m + (size_t)i * FLASH_META_SIZE : NULL, ecc_mode))
        {
            ret = 1;
        }
    }
    return ret;
}

/* ---- writing ---- */

/* A write is refused in t=14 mode too: a t=14 sector is a 538-byte record
 * on the media - 512 data, 3 metadata, 23 parity - where the program path
 * below addresses 528-byte records, and programming in that mode has not
 * been tried. */
static bool write_refused(void)
{
    bool refused = !ready || !writable || ecc_mode != 0;

    if (refused)
    {
        stats.write_refusals++;
    }
    return refused;
}

static uint8_t read_status(void)
{
    FLASH_CMD(0) = CMD_STATUS;
    return FLASH_DATA(0);
}

/* The page and spare buffers are written a word at a time; byte stores are
 * not known to work. src may be unaligned, or NULL for erased content. */
static void put_words(uintptr_t dst, const uint8_t *src, uint32_t len)
{
    volatile uint32_t *d = (volatile uint32_t *)dst;
    uint32_t i;

    for (i = 0; i < len / 4; i++)
    {
        if (src == NULL)
        {
            d[i] = 0xffffffff;
        }
        else
        {
            d[i] = src[4*i] | (src[4*i + 1] << 8) |
                   (src[4*i + 2] << 16) | ((uint32_t)src[4*i + 3] << 24);
        }
    }
}

/* Program `n` sectors of raw page `row`, from sector `first` in it. */
static int prog_raw_run(uint32_t row, uint32_t first, uint32_t n,
                        const uint8_t *data, const uint8_t *meta)
{
    uint32_t col = first * SECTOR_STRIDE;
    uint32_t i;
    int fail;

    flash_chip_select(0);
    FMCTL |= FM_PROTECT;                /* lift WP# for this operation */
    wait_flash_ready();

    FLASH_CMD(0)  = CMD_PROG_1ST;
    FLASH_ADDR(0) = col & 0xff;
    FLASH_ADDR(0) = (col >> 8) & 0xff;
    send_row(row);

    for (i = 0; i < n; i++)
    {
        uint32_t buf = i & 3;
        uint8_t spare[SPARE_SIZE];

        memset(spare, 0xff, sizeof(spare));
        if (meta)
        {
            memcpy(spare, meta + i * FLASH_META_SIZE, FLASH_META_SIZE);
        }
        if (!meta_passthrough)
        {
            spare[META_PROGRAMMED] = 0x00;
        }

        /* a slot is reused every four sectors: its last transfer must be
         * done */
        while (!(FLCTL & FL_RDY))
        {
        }

        put_words((uintptr_t)&PAGE_BUF + (buf << 9),
                  data ? data + (size_t)i * FLASH_SECTOR_SIZE : NULL,
                  FLASH_SECTOR_SIZE);
        put_words((uintptr_t)&SPARE_BUF + (buf << 4), spare, SPARE_SIZE);

        BCHCTL = BCH_WR | BCH_RST;
        FLCTL  = FL_KICK_WRITE | (buf << 3);
    }

    while (!(FLCTL & FL_RDY))
    {
    }

    FLASH_CMD(0) = CMD_PROG_2ND;
    fail = wait_ready_us(PROG_TIMEOUT_US) || (read_status() & NAND_STATUS_FAIL);

    flash_chip_deselect();
    FMCTL &= ~FM_PROTECT;

    if (fail)
    {
        stats.prog_failures++;
    }
    return fail ? 1 : 0;
}

static int erase_raw_block(uint32_t row)
{
    int fail;

    flash_chip_select(0);
    FMCTL |= FM_PROTECT;
    wait_flash_ready();

    FLASH_CMD(0) = CMD_ERASE_1ST;
    send_row(row);
    FLASH_CMD(0) = CMD_ERASE_2ND;

    fail = wait_ready_us(ERASE_TIMEOUT_US)
           || (read_status() & NAND_STATUS_FAIL);

    flash_chip_deselect();
    FMCTL &= ~FM_PROTECT;

    if (fail)
    {
        stats.erase_failures++;
    }
    return fail ? 1 : 0;
}

/* Sectors of the FTL's view are split into one program per raw page - and
 * so per plane. */
int flash_program(uint32_t sec, const void *data, const void *meta, unsigned n)
{
    const uint8_t *d = data;
    const uint8_t *m = meta;
    uint32_t i = 0;
    int ret = 0;

    if (n == 0)
    {
        ret = 0;
    }
    else if (write_refused())
    {
        ret = 1;
    }
    else if (sec < boot_area)
    {
        stats.boot_area_skips++;
    }
    else
    {
        while (i < n && ret == 0)
        {
            uint32_t raw   = sec_to_raw(sec + i);
            uint32_t row   = raw / geo.sec_per_page_raw;
            uint32_t first = raw % geo.sec_per_page_raw;
            uint32_t run   = 1;

            if (raw >= geo.total_sectors)
            {
                ret = 1;
                break;
            }

            /* extend the run while the next sector is the next slot of the
             * same raw page */
            while (i + run < n && first + run < geo.sec_per_page_raw &&
                   sec_to_raw(sec + i + run) == raw + run)
            {
                run++;
            }

            if (prog_raw_run(row, first, run,
                             d ? d + (size_t)i * FLASH_SECTOR_SIZE : NULL,
                             m ? m + (size_t)i * FLASH_META_SIZE : NULL))
            {
                ret = 1;
            }
            i += run;
        }
    }
    return ret;
}

int flash_program_page(uint32_t sec, const void *data, const void *meta)
{
    return flash_program(sec & ~(uint32_t)(geo.sec_per_page - 1), data, meta,
                         geo.sec_per_page);
}

int flash_erase(uint32_t sec)
{
    uint32_t base = sec - sec % geo.sec_per_block;
    uint32_t p;
    int ret = 0;

    if (write_refused())
    {
        ret = 1;
    }
    else if (sec < boot_area)
    {
        stats.boot_area_skips++;
    }
    else if (sec_to_raw(base) >= geo.total_sectors)
    {
        ret = 1;
    }
    else
    {
        for (p = 0; p < geo.planes; p++)
        {
            uint32_t raw = sec_to_raw(base + p * geo.sec_per_page_raw);

            if (erase_raw_block(raw / geo.sec_per_page_raw))
            {
                ret = 1;
            }
        }
    }
    return ret;
}

/* What a copy programs as the destination's metadata */
enum copy_meta
{
    COPY_META_FRESH,            /* as flash_program() with meta NULL */
    COPY_META_KEEP,             /* each sector's own */
    COPY_META_PAGE,             /* the caller's, by position in the page */
};

/* Copy through the ECC engine, never with the chip's internal data move: on
 * this MLC part moved pages accumulate bit errors, and the data area's
 * t=8 is already below the chip's 12-bit minimum. A source sector that
 * fails ECC is copied as read, and counted. */
static int copy_sectors(uint32_t src, uint32_t dst, unsigned n,
                        enum copy_meta how, const uint8_t *page_meta)
{
    static uint8_t buf[FLASH_MAX_SEC_PER_PAGE * FLASH_SECTOR_SIZE]
        __attribute__((aligned(4)));
    static uint8_t meta[FLASH_MAX_SEC_PER_PAGE * FLASH_META_SIZE];
    uint32_t k = 0;
    int ret = 0;

    if (n == 0 || n > geo.sec_per_block)
    {
        ret = 0;
    }
    else if (write_refused())
    {
        ret = 1;
    }
    else
    {
        while (k < n && ret == 0)
        {
            /* one destination raw page at a time */
            uint32_t len = geo.sec_per_page_raw
                           - (dst + k) % geo.sec_per_page_raw;
            uint32_t i = 0;

            if (len > n - k)
            {
                len = n - k;
            }

            /* the source may straddle raw pages if its offset differs */
            while (i < len && ret == 0)
            {
                uint32_t raw   = sec_to_raw(src + k + i);
                uint32_t first = raw % geo.sec_per_page_raw;
                uint32_t run   = 1;

                if (raw >= geo.total_sectors)
                {
                    ret = 1;
                    break;
                }
                while (i + run < len && first + run < geo.sec_per_page_raw &&
                       sec_to_raw(src + k + i + run) == raw + run)
                {
                    run++;
                }

                if (read_raw_run(raw / geo.sec_per_page_raw, first, run,
                                 buf + (size_t)i * FLASH_SECTOR_SIZE,
                                 how == COPY_META_KEEP ?
                                     meta + (size_t)i * FLASH_META_SIZE : NULL,
                                 ecc_mode))
                {
                    stats.copy_uncorrectable++;
                }
                i += run;
            }

            if (how == COPY_META_PAGE)
            {
                for (i = 0; i < len; i++)
                {
                    uint32_t at = (dst + k + i) % geo.sec_per_page;

                    memcpy(meta + i * FLASH_META_SIZE,
                           page_meta + at * FLASH_META_SIZE,
                           FLASH_META_SIZE);
                }
            }

            if (ret == 0
                && flash_program(dst + k, buf,
                                 how == COPY_META_FRESH ? NULL : meta, len))
            {
                ret = 1;
            }
            k += len;
        }
    }
    return ret;
}

int flash_copy(uint32_t src, uint32_t dst, unsigned n)
{
    return copy_sectors(src, dst, n, COPY_META_FRESH, NULL);
}

int flash_copy_meta(uint32_t src, uint32_t dst, unsigned n,
                    const void *page_meta)
{
    return copy_sectors(src, dst, n,
                        page_meta ? COPY_META_PAGE : COPY_META_KEEP,
                        page_meta);
}
