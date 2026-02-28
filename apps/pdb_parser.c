/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Parser for Pioneer Rekordbox export.pdb database files.
 *
 * Binary format reference:
 *   https://djl-analysis.deepsymmetry.org/rekordbox-export-analysis/exports.html
 *
 * All multi-byte integers in PDB are little-endian.
 *
 * File layout:
 *   - File header (28 bytes): magic(4), unknown(4), page_size(4),
 *     num_pages(4), unknown(4), sequence(4), unknown(4)
 *     followed by an array of table_pointer structs (one per table type).
 *   - Pages: each page_size bytes, zero-indexed.
 *
 * Page layout:
 *   - 0x00: zeros(4)
 *   - 0x04: page_index(4)      -- 0-based index of this page
 *   - 0x08: type(4)            -- table type (PDB_TABLE_*)
 *   - 0x0C: next_page(4)       -- next page in chain (0xFFFFFFFF = none)
 *   - 0x10: unknown1(4)
 *   - 0x14: num_rows_small(2)  -- lower bits of row count
 *   - 0x16: num_rows_large(2)  -- for pages with > 127 rows
 *   - 0x18: page_flags(1)      -- bit 6 = "strange" (garbage page, skip)
 *   - 0x19: free_size(2)
 *   - 0x1B: used_size(2)
 *   - 0x1D: unknown2(4)
 *   - 0x21: heap starts here
 *   - Row index: at end of page, grows backwards.
 *     Groups of up to 16 rows. Each group = presence_mask(2) +
 *     16 x row_offset(2). Row offsets are relative to heap start (0x28).
 *
 * Row count encoding (num_rows_small / num_rows_large):
 *   - actual row count = (num_rows_small & 0x1FFF) * 2  [if bit 14 set]
 *                      = (num_rows_small & 0x1FFF)       [otherwise]
 *   See: https://djl-analysis.deepsymmetry.org/rekordbox-export-analysis/exports.html#_pages
 *
 * Copyright (C) 2024 The Rockbox Project
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

#include "pdb_parser.h"

#include <string.h>
#include <stdlib.h>

#ifndef __PCTOOL__
#include "system.h"
#include "file.h"
#include "logf.h"
#include "rbunicode.h"
#else
/* Host-side stubs */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#define logf(fmt, ...) fprintf(stderr, "[pdb] " fmt "\n", ##__VA_ARGS__)
/* Provide utf16LEdecode for the host tool */
static unsigned char *utf16LEdecode(const unsigned char *utf16,
                                    unsigned char *utf8, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        unsigned int ucs = (unsigned int)utf16[0] | ((unsigned int)utf16[1] << 8);
        utf16 += 2;
        if (ucs < 0x80) {
            *utf8++ = (unsigned char)ucs;
        } else if (ucs < 0x800) {
            *utf8++ = 0xC0 | (ucs >> 6);
            *utf8++ = 0x80 | (ucs & 0x3F);
        } else {
            *utf8++ = 0xE0 | (ucs >> 12);
            *utf8++ = 0x80 | ((ucs >> 6) & 0x3F);
            *utf8++ = 0x80 | (ucs & 0x3F);
        }
    }
    *utf8 = '\0';
    return utf8;
}
#endif /* __PCTOOL__ */

/* -----------------------------------------------------------------------
 * Internal helpers for little-endian reads from a byte buffer.
 * ----------------------------------------------------------------------- */

static inline uint8_t read_u8(const uint8_t *buf, uint32_t off)
{
    return buf[off];
}

static inline uint16_t read_u16(const uint8_t *buf, uint32_t off)
{
    return (uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8);
}

static inline uint32_t read_u32(const uint8_t *buf, uint32_t off)
{
    return (uint32_t)buf[off]
         | ((uint32_t)buf[off + 1] << 8)
         | ((uint32_t)buf[off + 2] << 16)
         | ((uint32_t)buf[off + 3] << 24);
}

/* -----------------------------------------------------------------------
 * PDB file header constants.
 *
 * The file starts with:
 *   0x00: magic (4 bytes) — 0x00000000 (yes, four zero bytes)
 *   0x04: unknown (4 bytes)
 *   0x08: page_size (4 bytes)
 *   0x0C: num_pages (4 bytes)
 *   0x10: unknown (4 bytes)
 *   0x14: sequence (4 bytes)
 *   0x18: unknown (4 bytes)
 *
 * Immediately after (at 0x1C) are the table pointer entries. Each entry is:
 *   type(4) + first_page(4) = 8 bytes.
 * There are typically 16 table entries.
 * ----------------------------------------------------------------------- */

#define PDB_FILE_HEADER_SIZE     0x1C
#define PDB_TABLE_ENTRY_SIZE     8
#define PDB_NUM_TABLE_ENTRIES    16

/* Page header field offsets */
#define PDB_PAGE_OFF_ZEROS       0x00
#define PDB_PAGE_OFF_INDEX       0x04
#define PDB_PAGE_OFF_TYPE        0x08
#define PDB_PAGE_OFF_NEXT        0x0C
#define PDB_PAGE_OFF_NUM_ROWS    0x14  /* 2-byte num_rows_small */
#define PDB_PAGE_OFF_NUM_ROWS2   0x16  /* 2-byte num_rows_large */
#define PDB_PAGE_OFF_FLAGS       0x18
#define PDB_PAGE_HEAP_START      0x28  /* first byte of heap */

/* -----------------------------------------------------------------------
 * DeviceSQL string decoding.
 *
 * Strings in the PDB heap are stored as "DeviceSQL" blobs:
 *   byte 0: type
 *     0x40 = long ASCII   (followed by 2-byte length, then ASCII chars)
 *     0x90 = long UTF-16LE (followed by 2-byte length in code units, then UTF-16LE)
 *   Other types are rare; we treat them as empty.
 *
 * String offsets stored in rows are relative to the start of the page heap
 * (i.e., offset 0 in the heap = PDB_PAGE_HEAP_START in the page buffer).
 * ----------------------------------------------------------------------- */

/*
 * Decode a DeviceSQL string from within a page buffer.
 * heap_off: offset from heap start (PDB_PAGE_HEAP_START) of the string blob.
 * page:     the full page buffer.
 * page_size: size of the page buffer.
 * out:      destination buffer (UTF-8, NUL-terminated).
 * out_size: size of destination buffer.
 */
static void decode_devicesql_string(uint32_t heap_off,
                                    const uint8_t *page, uint32_t page_size,
                                    char *out, int out_size)
{
    uint32_t blob_off;
    uint8_t  type_byte;
    uint16_t length;
    const uint8_t *data;

    out[0] = '\0';
    blob_off = PDB_PAGE_HEAP_START + heap_off;
    if (blob_off + 3 > page_size)
        return;

    type_byte = read_u8(page, blob_off);
    length    = read_u16(page, blob_off + 1);
    data      = page + blob_off + 3;

    if (type_byte == PDB_STR_TYPE_LONG_ASCII) {
        /* ASCII: length bytes of ASCII characters */
        if (blob_off + 3 + length > page_size)
            return;
        int copy = (length < (uint16_t)(out_size - 1))
                 ? length : (out_size - 1);
        memcpy(out, data, copy);
        out[copy] = '\0';
    } else if (type_byte == PDB_STR_TYPE_LONG_UTF16LE) {
        /* UTF-16LE: length = number of code units (not bytes) */
        uint32_t byte_len = (uint32_t)length * 2;
        if (blob_off + 3 + byte_len > page_size)
            return;
        /* Each UTF-16 code unit encodes to at most 3 UTF-8 bytes + NUL */
        if ((int)(length * 3 + 1) > out_size)
            length = (uint16_t)((out_size - 1) / 3);
        utf16LEdecode(data, (unsigned char *)out, length);
        out[out_size - 1] = '\0';
    }
    /* Unknown type byte: leave as empty string */
}

/* -----------------------------------------------------------------------
 * Page I/O helpers.
 * ----------------------------------------------------------------------- */

/*
 * Read page number page_idx into ctx->page_buf.
 * Returns true on success.
 */
static bool read_page(struct pdb_context *ctx, uint32_t page_idx)
{
    int32_t offset;

    if (page_idx >= ctx->num_pages) {
        logf("pdb: page %lu out of range (num_pages=%lu)",
             (unsigned long)page_idx, (unsigned long)ctx->num_pages);
        return false;
    }

    offset = (int32_t)(page_idx * ctx->page_size);
    if (lseek(ctx->fd, offset, SEEK_SET) < 0) {
        logf("pdb: seek to page %lu failed", (unsigned long)page_idx);
        return false;
    }

    if (read(ctx->fd, ctx->page_buf, ctx->page_size)
            != (int)ctx->page_size) {
        logf("pdb: read page %lu failed", (unsigned long)page_idx);
        return false;
    }

    return true;
}

/*
 * Decode the effective row count from a page buffer.
 *
 * The page stores two 2-byte fields at offsets 0x14 and 0x16.
 * The high bit (bit 15) of the first field indicates which formula to use:
 *   - bit 15 clear: num_rows = (field0 & 0x1FFF)
 *   - bit 15 set:   num_rows = (field0 & 0x0FFF) | ((field1 & 0x000F) << 12)
 *                              (i.e., 16-bit combined row count)
 *
 * Additionally, the row *offset* count is stored in the upper bits:
 *   num_row_offsets = (field0 >> 13) & 0x07   [3-bit field at bits 13-15]
 *
 * The number of rows listed in the row index (num_row_offsets) may differ
 * from the actual filled row count (num_rows); we iterate over all
 * row index entries and use the presence mask to skip absent ones.
 */
static uint32_t page_row_count(const uint8_t *page)
{
    uint16_t f0 = read_u16(page, PDB_PAGE_OFF_NUM_ROWS);
    uint16_t f1 = read_u16(page, PDB_PAGE_OFF_NUM_ROWS2);

    if (f0 & 0x8000) {
        /* high bit set: combined 16-bit count */
        return (uint32_t)(f0 & 0x0FFF) | ((uint32_t)(f1 & 0x000F) << 12);
    } else {
        return (uint32_t)(f0 & 0x1FFF);
    }
}

/*
 * Return the number of row index entries (groups of up to 16) on the page.
 * This is the value stored in bits 13-15 of the first num_rows field.
 *
 * NOTE: each index entry covers up to 16 rows. A value of N means there
 * are N groups, covering rows 0..N*16-1.
 */
static uint32_t page_num_row_groups(const uint8_t *page)
{
    uint16_t f0 = read_u16(page, PDB_PAGE_OFF_NUM_ROWS);
    /* bits 12:0 contain the count; bits 15:13 the group count */
    return (uint32_t)((f0 >> 13) & 0x07);
}

/*
 * Iterate over all rows in a page, calling the provided callback for each
 * present row.
 *
 * The row index grows backwards from the end of the page. Each group is:
 *   presence_mask(2) + 16 x row_offset(2) = 34 bytes.
 *
 * The group at index 0 (the *last* group written, at the end of the page)
 * covers the highest-numbered rows; groups are in reverse order.
 * Bit i of presence_mask indicates row i within the group is present.
 * Row offsets are relative to heap start (PDB_PAGE_HEAP_START).
 *
 * callback: called with (page_buf, row_heap_offset, page_size, userdata)
 *           for each present row.
 *           Returns false to abort iteration.
 */
typedef bool (*pdb_row_cb)(const uint8_t *page, uint32_t row_heap_off,
                           uint32_t page_size, void *userdata);

static void iterate_page_rows(const uint8_t *page, uint32_t page_size,
                               pdb_row_cb cb, void *userdata)
{
    uint32_t num_groups;
    uint32_t g, bit;
    uint32_t group_off;  /* offset from end of page to this group's presence_mask */
    uint16_t mask;
    uint16_t row_off;

    if (page[PDB_PAGE_OFF_FLAGS] & PDB_PAGE_FLAG_STRANGE)
        return;  /* "strange" page — skip */

    num_groups = page_num_row_groups(page);
    if (num_groups == 0)
        return;

    /* Groups are at the very end of the page, growing backwards.
     * Group 0 is at page[page_size - 34], group 1 at page[page_size - 68], etc.
     * Each group is 2 + 16*2 = 34 bytes. */
    for (g = 0; g < num_groups; g++) {
        group_off = page_size - (g + 1) * 34;
        mask = read_u16(page, group_off);

        for (bit = 0; bit < 16; bit++) {
            if (!(mask & (1 << bit)))
                continue;  /* row absent */

            row_off = read_u16(page, group_off + 2 + bit * 2);
            if (row_off == 0)
                continue;

            /* row_off is relative to heap start */
            if ((uint32_t)PDB_PAGE_HEAP_START + row_off >= page_size)
                continue;

            if (!cb(page, (uint32_t)row_off, page_size, userdata))
                return;  /* callback requested abort */
        }
    }
}

/* -----------------------------------------------------------------------
 * String lookup table management.
 * ----------------------------------------------------------------------- */

/*
 * Add or update an entry in a string lookup table.
 * Returns true on success, false if the table is full.
 */
static bool string_table_add(struct pdb_string_table *tbl,
                              uint32_t id, const char *name)
{
    int i;
    struct pdb_string_entry *e;

    /* Check for existing entry (update in place) */
    for (i = 0; i < tbl->count; i++) {
        if (tbl->entries[i].id == id) {
            strlcpy(tbl->entries[i].name, name, PDB_STR_MAX);
            return true;
        }
    }

    if (tbl->count >= tbl->capacity)
        return false;

    e = &tbl->entries[tbl->count++];
    e->id = id;
    strlcpy(e->name, name, PDB_STR_MAX);
    return true;
}

const char *pdb_lookup_string(const struct pdb_string_table *tbl, uint32_t id)
{
    int i;
    for (i = 0; i < tbl->count; i++) {
        if (tbl->entries[i].id == id)
            return tbl->entries[i].name;
    }
    return "";
}

/* -----------------------------------------------------------------------
 * Row parsers for each table type.
 * ----------------------------------------------------------------------- */

/*
 * Parse an artist or album row.
 * The row format is identical for both:
 *   0x00: subtype(1)     — 0x60/0x80 = "near" (1-byte offsets)
 *                          0x64/0x84 = "far"  (2-byte offsets)
 *   0x01: index_shift(1)
 *   0x02: bitmask(4)
 *   0x06: id(4)
 *   ...
 *   For subtype 0x60 (near artist):
 *     0x0A: name_offset(1)  — 1-byte heap offset
 *   For subtype 0x64 (far artist):
 *     0x0A: name_offset(2)  — 2-byte heap offset
 *
 * Album rows have subtypes 0x80 (near) and 0x84 (far).
 * The id field is always at the same position.
 */
struct name_id_cb_data {
    struct pdb_string_table *tbl;
    uint32_t page_size;
};

static bool parse_artist_row(const uint8_t *page, uint32_t row_off,
                              uint32_t page_size, void *userdata)
{
    struct name_id_cb_data *d = (struct name_id_cb_data *)userdata;
    const uint8_t *row = page + PDB_PAGE_HEAP_START + row_off;
    uint8_t  subtype;
    uint32_t id;
    uint32_t name_heap_off;
    char name[PDB_STR_MAX];

    /* Safety: need at least 12 bytes for the row header */
    if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 12) > page_size)
        return true;

    subtype = read_u8(row, 0x00);
    id      = read_u32(row, 0x06);

    if (subtype == 0x60) {
        /* near: 1-byte name offset at 0x0A */
        if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 11) > page_size)
            return true;
        name_heap_off = read_u8(row, 0x0A);
    } else if (subtype == 0x64) {
        /* far: 2-byte name offset at 0x0A */
        if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 12) > page_size)
            return true;
        name_heap_off = read_u16(row, 0x0A);
    } else {
        return true;  /* unknown subtype, skip */
    }

    decode_devicesql_string(name_heap_off, page, page_size, name, PDB_STR_MAX);
    string_table_add(d->tbl, id, name);
    return true;
}

static bool parse_album_row(const uint8_t *page, uint32_t row_off,
                             uint32_t page_size, void *userdata)
{
    struct name_id_cb_data *d = (struct name_id_cb_data *)userdata;
    const uint8_t *row = page + PDB_PAGE_HEAP_START + row_off;
    uint8_t  subtype;
    uint32_t id;
    uint32_t name_heap_off;
    char name[PDB_STR_MAX];

    if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 12) > page_size)
        return true;

    subtype = read_u8(row, 0x00);
    id      = read_u32(row, 0x06);

    if (subtype == 0x80) {
        /* near album */
        if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 11) > page_size)
            return true;
        name_heap_off = read_u8(row, 0x0A);
    } else if (subtype == 0x84) {
        /* far album */
        if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 12) > page_size)
            return true;
        name_heap_off = read_u16(row, 0x0A);
    } else {
        return true;
    }

    decode_devicesql_string(name_heap_off, page, page_size, name, PDB_STR_MAX);
    string_table_add(d->tbl, id, name);
    return true;
}

/*
 * Parse a genre row.
 * Genre rows use a simpler format:
 *   0x00: subtype(1)
 *   0x01: index_shift(1)
 *   0x02: bitmask(4)
 *   0x06: id(4)
 *   0x0A: name_offset(2)  — 2-byte heap offset
 */
static bool parse_genre_row(const uint8_t *page, uint32_t row_off,
                             uint32_t page_size, void *userdata)
{
    struct name_id_cb_data *d = (struct name_id_cb_data *)userdata;
    const uint8_t *row = page + PDB_PAGE_HEAP_START + row_off;
    uint32_t id;
    uint32_t name_heap_off;
    char name[PDB_STR_MAX];

    if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 12) > page_size)
        return true;

    id            = read_u32(row, 0x06);
    name_heap_off = read_u16(row, 0x0A);

    decode_devicesql_string(name_heap_off, page, page_size, name, PDB_STR_MAX);
    string_table_add(d->tbl, id, name);
    return true;
}

/*
 * Track row layout (Rekordbox 6.x/7.x, little-endian):
 *
 *   0x00: subtype(2)
 *   0x02: index_shift(2)
 *   0x04: bitmask(4)
 *   0x08: sample_rate(4)
 *   0x0C: composer_id(4)
 *   0x10: file_size(4)
 *   0x14: unknown(4)
 *   0x18: unknown(2)
 *   0x1A: unknown(2)
 *   0x1C: artwork_id(4)
 *   0x20: key_id(4)
 *   0x24: orig_artist_id(4)
 *   0x28: label_id(4)
 *   0x2C: remixer_id(4)
 *   0x30: bitrate(4)       -- kbps
 *   0x34: track_number(4)
 *   0x38: tempo(4)         -- BPM * 100
 *   0x3C: genre_id(4)
 *   0x40: album_id(4)
 *   0x44: artist_id(4)
 *   0x48: id(4)
 *   0x4C: disc_number(2)
 *   0x4E: play_count(2)
 *   0x50: year(2)
 *   0x52: sample_depth(2)
 *   0x54: duration(2)      -- seconds
 *   0x56: unknown(2)
 *   0x58: color_id(1)
 *   0x59: rating(1)        -- 0..5 stars * 0x14
 *   0x5A: unknown(2)
 *   0x5C: file_type(2)
 *   0x5E: unknown(2)
 *   0x60...: 21 x uint16 string offsets (heap-relative)
 *
 * String slot indices (0-based within the 21-slot array at 0x60):
 *   0: isrc
 *   1: lyricist
 *   2: title
 *   3: unknown
 *   4: unknown
 *   5: artist        (sometimes also from artist_id)
 *   6: album         (sometimes also from album_id)
 *   7: genre         (sometimes also from genre_id)
 *   8: comment
 *   9: date_added
 *  10: unknown
 *  11: file_path     ("/Contents/...")
 *  12: file_name
 *  13: file_type_str
 *  14: unknown
 *  15: unknown
 *  16: unknown
 *  17: mix_name
 *  18: unknown
 *  19: unknown
 *  20: key
 */

#define TRACK_STR_TITLE       2
#define TRACK_STR_ARTIST      5
#define TRACK_STR_ALBUM       6
#define TRACK_STR_GENRE       7
#define TRACK_STR_COMMENT     8
#define TRACK_STR_FILE_PATH  11
#define TRACK_NUM_STRINGS    21

#define TRACK_ROW_MIN_SIZE   (0x60 + TRACK_NUM_STRINGS * 2)

struct track_parse_cb_data {
    struct pdb_context *ctx;
    struct pdb_track   *tracks;
    int                 max_tracks;
    int                 count;
};

static bool parse_track_row(const uint8_t *page, uint32_t row_off,
                             uint32_t page_size, void *userdata)
{
    struct track_parse_cb_data *d = (struct track_parse_cb_data *)userdata;
    const uint8_t *row = page + PDB_PAGE_HEAP_START + row_off;
    struct pdb_track *t;
    uint16_t str_offs[TRACK_NUM_STRINGS];
    int i;
    uint8_t raw_rating;

    if (d->count >= d->max_tracks)
        return false;  /* no more room */

    /* Safety check for minimum row size */
    if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + TRACK_ROW_MIN_SIZE) > page_size)
        return true;

    t = &d->tracks[d->count];
    memset(t, 0, sizeof(struct pdb_track));

    /* Fixed numeric fields */
    t->sample_rate   = read_u32(row, 0x08);
    t->bitrate       = read_u32(row, 0x30);
    t->track_number  = (uint16_t)read_u32(row, 0x34);
    t->genre_id      = read_u32(row, 0x3C);
    t->album_id      = read_u32(row, 0x40);
    t->artist_id     = read_u32(row, 0x44);
    t->id            = read_u32(row, 0x48);
    t->disc_number   = read_u16(row, 0x4C);
    t->year          = read_u16(row, 0x50);
    t->duration      = read_u16(row, 0x54);
    raw_rating       = read_u8 (row, 0x59);
    /* Rekordbox stores rating as 0x00, 0x14, 0x28, 0x3C, 0x50, 0x64 (0..5 stars) */
    t->rating        = (raw_rating > 0) ? (raw_rating / 0x14) : 0;
    if (t->rating > 5) t->rating = 5;

    /* Read the 21 string offset slots */
    for (i = 0; i < TRACK_NUM_STRINGS; i++)
        str_offs[i] = read_u16(row, 0x60 + i * 2);

    /* Decode string fields from the heap */
    decode_devicesql_string(str_offs[TRACK_STR_TITLE],
                            page, page_size, t->title,   PDB_STR_MAX);
    decode_devicesql_string(str_offs[TRACK_STR_COMMENT],
                            page, page_size, t->comment, PDB_STR_MAX);
    decode_devicesql_string(str_offs[TRACK_STR_FILE_PATH],
                            page, page_size, t->file_path, PDB_STR_MAX);

    /* Prefer embedded string for artist/album/genre; fall back to lookup */
    decode_devicesql_string(str_offs[TRACK_STR_ARTIST],
                            page, page_size, t->artist, PDB_STR_MAX);
    if (t->artist[0] == '\0' && t->artist_id != 0)
        strlcpy(t->artist, pdb_lookup_string(&d->ctx->artists, t->artist_id),
                PDB_STR_MAX);

    decode_devicesql_string(str_offs[TRACK_STR_ALBUM],
                            page, page_size, t->album, PDB_STR_MAX);
    if (t->album[0] == '\0' && t->album_id != 0)
        strlcpy(t->album, pdb_lookup_string(&d->ctx->albums, t->album_id),
                PDB_STR_MAX);

    decode_devicesql_string(str_offs[TRACK_STR_GENRE],
                            page, page_size, t->genre, PDB_STR_MAX);
    if (t->genre[0] == '\0' && t->genre_id != 0)
        strlcpy(t->genre, pdb_lookup_string(&d->ctx->genres, t->genre_id),
                PDB_STR_MAX);

    /* Skip entries with no usable file path */
    if (t->file_path[0] == '\0') {
        logf("pdb: track id=%lu has no file_path, skipping",
             (unsigned long)t->id);
        return true;
    }

    d->count++;
    return true;
}

/*
 * Playlist tree row:
 *   0x00: subtype(1)
 *   0x01: index_shift(1)
 *   0x02: bitmask(4)
 *   0x06: parent_id(4)
 *   0x0A: unknown(4)
 *   0x0E: sort_order(4)
 *   0x12: id(4)
 *   0x16: raw_is_folder(2)   -- 0 = playlist, 1 = folder
 *   0x18: name_offset(2)     -- heap offset to DeviceSQL string
 */
struct playlist_tree_cb_data {
    struct pdb_playlist_tree_entry *buf;
    int max;
    int count;
};

static bool parse_playlist_tree_row(const uint8_t *page, uint32_t row_off,
                                    uint32_t page_size, void *userdata)
{
    struct playlist_tree_cb_data *d = (struct playlist_tree_cb_data *)userdata;
    const uint8_t *row = page + PDB_PAGE_HEAP_START + row_off;
    struct pdb_playlist_tree_entry *e;
    uint16_t name_off;

    if (d->count >= d->max)
        return false;

    if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 0x1A) > page_size)
        return true;

    e = &d->buf[d->count];
    e->parent_id  = read_u32(row, 0x06);
    e->sort_order = read_u32(row, 0x0E);
    e->id         = read_u32(row, 0x12);
    e->is_folder  = (read_u16(row, 0x16) != 0);
    name_off      = read_u16(row, 0x18);

    decode_devicesql_string(name_off, page, page_size, e->name, PDB_STR_MAX);

    if (e->name[0] == '\0')
        strlcpy(e->name, "Unnamed", PDB_STR_MAX);

    d->count++;
    return true;
}

/*
 * Playlist entry row:
 *   0x00: subtype(1)
 *   0x01: index_shift(1)
 *   0x02: bitmask(4)
 *   0x06: entry_index(4)
 *   0x0A: track_id(4)
 *   0x0E: playlist_id(4)
 */
struct playlist_entry_cb_data {
    struct pdb_playlist_entry *buf;
    int max;
    int count;
};

static bool parse_playlist_entry_row(const uint8_t *page, uint32_t row_off,
                                     uint32_t page_size, void *userdata)
{
    struct playlist_entry_cb_data *d = (struct playlist_entry_cb_data *)userdata;
    const uint8_t *row = page + PDB_PAGE_HEAP_START + row_off;
    struct pdb_playlist_entry *e;

    if (d->count >= d->max)
        return false;

    if ((uint32_t)(PDB_PAGE_HEAP_START + row_off + 0x12) > page_size)
        return true;

    e = &d->buf[d->count];
    e->entry_index = read_u32(row, 0x06);
    e->track_id    = read_u32(row, 0x0A);
    e->playlist_id = read_u32(row, 0x0E);

    d->count++;
    return true;
}

/* -----------------------------------------------------------------------
 * Page-chain walker: walk all pages of a given table type, calling
 * the row callback on each non-strange page.
 * ----------------------------------------------------------------------- */

static bool walk_table(struct pdb_context *ctx, uint32_t table_type,
                       pdb_row_cb cb, void *userdata)
{
    uint32_t page_idx;
    uint32_t next_page;

    if (table_type >= 16)
        return false;

    page_idx = ctx->table_first_page[table_type];
    if (page_idx == 0xFFFFFFFF) {
        logf("pdb: table %lu not present", (unsigned long)table_type);
        return true;  /* not an error, table simply absent */
    }

    while (page_idx != 0xFFFFFFFF) {
        if (!read_page(ctx, page_idx)) {
            logf("pdb: failed to read page %lu for table %lu",
                 (unsigned long)page_idx, (unsigned long)table_type);
            return false;
        }

        /* Verify this page belongs to the expected table */
        if (read_u32(ctx->page_buf, PDB_PAGE_OFF_TYPE) != table_type) {
            logf("pdb: page %lu has wrong type (expected %lu, got %lu)",
                 (unsigned long)page_idx, (unsigned long)table_type,
                 (unsigned long)read_u32(ctx->page_buf, PDB_PAGE_OFF_TYPE));
            break;
        }

        iterate_page_rows(ctx->page_buf, ctx->page_size, cb, userdata);

        next_page = read_u32(ctx->page_buf, PDB_PAGE_OFF_NEXT);
        page_idx  = next_page;
    }

    return true;
}

/* -----------------------------------------------------------------------
 * Public API implementation.
 * ----------------------------------------------------------------------- */

bool pdb_open(struct pdb_context *ctx)
{
    uint8_t  header[PDB_FILE_HEADER_SIZE + PDB_NUM_TABLE_ENTRIES * PDB_TABLE_ENTRY_SIZE];
    uint32_t magic;
    int      i;

    /* Read file header + table entries in one shot */
    if (lseek(ctx->fd, 0, SEEK_SET) < 0)
        return false;

    if (read(ctx->fd, header, sizeof(header)) != (int)sizeof(header)) {
        logf("pdb: failed to read file header");
        return false;
    }

    /* Magic is the first 4 bytes — Rekordbox PDB starts with 0x00000000 */
    magic = read_u32(header, 0x00);
    if (magic != 0x00000000) {
        logf("pdb: unexpected magic 0x%08lx (expected 0x00000000)",
             (unsigned long)magic);
        return false;
    }

    ctx->page_size = read_u32(header, 0x08);
    ctx->num_pages = read_u32(header, 0x0C);

    if (ctx->page_size < 0x100 || ctx->page_size > 0x10000) {
        logf("pdb: implausible page_size %lu", (unsigned long)ctx->page_size);
        return false;
    }
    if (ctx->num_pages == 0 || ctx->num_pages > 0x100000) {
        logf("pdb: implausible num_pages %lu", (unsigned long)ctx->num_pages);
        return false;
    }
    if (ctx->page_size > ctx->page_buf_size) {
        logf("pdb: page_size %lu > page_buf_size %lu",
             (unsigned long)ctx->page_size, (unsigned long)ctx->page_buf_size);
        return false;
    }

    /* Initialise all table first-page pointers to "absent" */
    for (i = 0; i < 16; i++)
        ctx->table_first_page[i] = 0xFFFFFFFF;

    /* Parse table pointer entries.
     * Each entry: type(4) + first_page(4). */
    for (i = 0; i < PDB_NUM_TABLE_ENTRIES; i++) {
        uint32_t off  = PDB_FILE_HEADER_SIZE + (uint32_t)i * PDB_TABLE_ENTRY_SIZE;
        uint32_t type = read_u32(header, off);
        uint32_t fp   = read_u32(header, off + 4);

        if (type < 16)
            ctx->table_first_page[type] = fp;
    }

    logf("pdb: page_size=%lu num_pages=%lu",
         (unsigned long)ctx->page_size, (unsigned long)ctx->num_pages);

    return true;
}

void pdb_close(struct pdb_context *ctx)
{
    if (ctx->artists.entries) {
        free(ctx->artists.entries);
        ctx->artists.entries = NULL;
        ctx->artists.count   = 0;
    }
    if (ctx->albums.entries) {
        free(ctx->albums.entries);
        ctx->albums.entries = NULL;
        ctx->albums.count   = 0;
    }
    if (ctx->genres.entries) {
        free(ctx->genres.entries);
        ctx->genres.entries = NULL;
        ctx->genres.count   = 0;
    }
}

int pdb_build_lookup_tables(struct pdb_context *ctx)
{
    struct name_id_cb_data cb_data;
    int total = 0;

    /* Allocate lookup table storage */
    if (!ctx->artists.entries) {
        ctx->artists.entries = (struct pdb_string_entry *)
            malloc(PDB_LOOKUP_MAX * sizeof(struct pdb_string_entry));
        if (!ctx->artists.entries) {
            logf("pdb: out of memory for artists table");
            return -1;
        }
        ctx->artists.count    = 0;
        ctx->artists.capacity = PDB_LOOKUP_MAX;
    }
    if (!ctx->albums.entries) {
        ctx->albums.entries = (struct pdb_string_entry *)
            malloc(PDB_LOOKUP_MAX * sizeof(struct pdb_string_entry));
        if (!ctx->albums.entries) {
            logf("pdb: out of memory for albums table");
            return -1;
        }
        ctx->albums.count    = 0;
        ctx->albums.capacity = PDB_LOOKUP_MAX;
    }
    if (!ctx->genres.entries) {
        ctx->genres.entries = (struct pdb_string_entry *)
            malloc(PDB_LOOKUP_MAX * sizeof(struct pdb_string_entry));
        if (!ctx->genres.entries) {
            logf("pdb: out of memory for genres table");
            return -1;
        }
        ctx->genres.count    = 0;
        ctx->genres.capacity = PDB_LOOKUP_MAX;
    }

    /* Parse artists */
    cb_data.tbl       = &ctx->artists;
    cb_data.page_size = ctx->page_size;
    if (!walk_table(ctx, PDB_TABLE_ARTISTS, parse_artist_row, &cb_data))
        return -1;
    total += ctx->artists.count;
    logf("pdb: loaded %d artists", ctx->artists.count);

    /* Parse albums */
    cb_data.tbl = &ctx->albums;
    if (!walk_table(ctx, PDB_TABLE_ALBUMS, parse_album_row, &cb_data))
        return -1;
    total += ctx->albums.count;
    logf("pdb: loaded %d albums", ctx->albums.count);

    /* Parse genres */
    cb_data.tbl = &ctx->genres;
    if (!walk_table(ctx, PDB_TABLE_GENRES, parse_genre_row, &cb_data))
        return -1;
    total += ctx->genres.count;
    logf("pdb: loaded %d genres", ctx->genres.count);

    return total;
}

int pdb_parse_tracks(struct pdb_context *ctx,
                     struct pdb_track *tracks_buf, int max_tracks)
{
    struct track_parse_cb_data cb_data;

    cb_data.ctx        = ctx;
    cb_data.tracks     = tracks_buf;
    cb_data.max_tracks = max_tracks;
    cb_data.count      = 0;

    if (!walk_table(ctx, PDB_TABLE_TRACKS, parse_track_row, &cb_data))
        return -1;

    logf("pdb: parsed %d tracks", cb_data.count);
    return cb_data.count;
}

int pdb_parse_playlists(struct pdb_context *ctx,
                        struct pdb_playlist_tree_entry *tree_buf, int tree_max,
                        struct pdb_playlist_entry *entries_buf, int entries_max)
{
    struct playlist_tree_cb_data  tree_cb;
    struct playlist_entry_cb_data entry_cb;
    int total;

    tree_cb.buf   = tree_buf;
    tree_cb.max   = tree_max;
    tree_cb.count = 0;
    if (!walk_table(ctx, PDB_TABLE_PLAYLIST_TREE,
                    parse_playlist_tree_row, &tree_cb))
        return -1;
    logf("pdb: parsed %d playlist tree entries", tree_cb.count);

    entry_cb.buf   = entries_buf;
    entry_cb.max   = entries_max;
    entry_cb.count = 0;
    if (!walk_table(ctx, PDB_TABLE_PLAYLIST_ENTRY,
                    parse_playlist_entry_row, &entry_cb))
        return -1;
    logf("pdb: parsed %d playlist member entries", entry_cb.count);

    ctx->playlist_tree         = tree_buf;
    ctx->playlist_tree_count   = tree_cb.count;
    ctx->playlist_entries      = entries_buf;
    ctx->playlist_entry_count  = entry_cb.count;

    total = tree_cb.count + entry_cb.count;
    return total;
}
