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
 * The PDB format is documented at:
 *   https://djl-analysis.deepsymmetry.org/rekordbox-export-analysis/exports.html
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

#ifndef _PDB_PARSER_H
#define _PDB_PARSER_H

#include <stdint.h>
#include <stdbool.h>

/* Maximum string length for any PDB field (UTF-8 encoded) */
#define PDB_STR_MAX 512

/* Maximum number of tracks we'll process in a single PDB file.
 * Prevents unbounded memory use on large collections. */
#define PDB_MAX_TRACKS 50000

/* Maximum number of playlists */
#define PDB_MAX_PLAYLISTS 2000

/* Maximum tracks per playlist */
#define PDB_MAX_PLAYLIST_ENTRIES 10000

/*
 * PDB table type identifiers (value in page header's "type" field).
 */
#define PDB_TABLE_TRACKS         0x00
#define PDB_TABLE_GENRES         0x01
#define PDB_TABLE_ARTISTS        0x02
#define PDB_TABLE_ALBUMS         0x03
#define PDB_TABLE_LABELS         0x04
#define PDB_TABLE_KEYS           0x05
#define PDB_TABLE_COLORS         0x06
#define PDB_TABLE_PLAYLIST_TREE  0x07
#define PDB_TABLE_PLAYLIST_ENTRY 0x08
#define PDB_TABLE_ARTWORK        0x0d

/*
 * PDB "strange" page flag: pages with this bit set in page_flags
 * contain only deleted/garbage rows and must be skipped.
 */
#define PDB_PAGE_FLAG_STRANGE    0x40

/*
 * DeviceSQL string type bytes (first byte of the string blob).
 * 0x40 = long ASCII   (2-byte length prefix, then ASCII bytes)
 * 0x90 = long UTF-16LE (2-byte length prefix, then UTF-16LE code units)
 * Other values may exist but are rare; treat as empty string.
 */
#define PDB_STR_TYPE_LONG_ASCII   0x40
#define PDB_STR_TYPE_LONG_UTF16LE 0x90

/* -----------------------------------------------------------------------
 * Parsed record structures.
 * These are populated by the parser callbacks and kept compact to avoid
 * large stack frames. All strings are NUL-terminated UTF-8.
 * ----------------------------------------------------------------------- */

struct pdb_track {
    uint32_t id;
    uint32_t artist_id;
    uint32_t album_id;
    uint32_t genre_id;
    uint32_t key_id;
    uint16_t track_number;
    uint16_t disc_number;
    uint16_t year;
    uint16_t duration;       /* seconds */
    uint32_t bitrate;        /* kbps */
    uint32_t sample_rate;    /* Hz */
    uint8_t  rating;         /* 0-5 (multiply by 51 for 0-255 scale) */

    /* File path: "/Contents/..." relative to export root */
    char     file_path[PDB_STR_MAX];

    /* Metadata strings decoded to UTF-8 */
    char     title[PDB_STR_MAX];
    char     artist[PDB_STR_MAX];    /* resolved from artist_id */
    char     album[PDB_STR_MAX];     /* resolved from album_id  */
    char     genre[PDB_STR_MAX];     /* resolved from genre_id  */
    char     comment[PDB_STR_MAX];
    char     composer[PDB_STR_MAX];
};

struct pdb_playlist_tree_entry {
    uint32_t id;
    uint32_t parent_id;
    uint32_t sort_order;
    bool     is_folder;
    char     name[PDB_STR_MAX];
};

struct pdb_playlist_entry {
    uint32_t playlist_id;
    uint32_t track_id;
    uint32_t entry_index;  /* sort order within the playlist */
};

/* -----------------------------------------------------------------------
 * String lookup table for resolving IDs → names.
 * We build one per relevant table (artists, albums, genres).
 * ----------------------------------------------------------------------- */

#define PDB_LOOKUP_MAX 8192

struct pdb_string_entry {
    uint32_t id;
    char     name[PDB_STR_MAX];
};

struct pdb_string_table {
    struct pdb_string_entry *entries;
    int                      count;
    int                      capacity;
};

/* -----------------------------------------------------------------------
 * Parser state / context.
 * Caller allocates this on the heap (or in the audio buffer).
 * ----------------------------------------------------------------------- */

struct pdb_context {
    int      fd;         /* open file descriptor of export.pdb */
    uint32_t page_size;  /* bytes per page (from file header) */
    uint32_t num_pages;  /* total number of pages in the file */

    /* Table first-page pointers (page index of each table's first page).
     * 0xFFFFFFFF means the table is absent. */
    uint32_t table_first_page[16];

    /* String lookup tables built during the first pass */
    struct pdb_string_table artists;
    struct pdb_string_table albums;
    struct pdb_string_table genres;

    /* Track records (filled during the track pass) */
    struct pdb_track        *tracks;
    int                      track_count;

    /* Playlist tree (folders + playlists) */
    struct pdb_playlist_tree_entry *playlist_tree;
    int                             playlist_tree_count;

    /* Playlist entries (track-to-playlist membership + order) */
    struct pdb_playlist_entry *playlist_entries;
    int                        playlist_entry_count;

    /* Scratch buffer for reading one page at a time (page_size bytes).
     * Must point to a caller-provided buffer of at least page_size bytes. */
    uint8_t *page_buf;
    uint32_t page_buf_size;
};

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/*
 * Open and validate the PDB file header. Fills ctx->page_size,
 * ctx->num_pages, and ctx->table_first_page[].
 *
 * Returns true on success, false on error (bad magic or I/O failure).
 * The caller must have set ctx->fd to an open file descriptor before calling.
 */
bool pdb_open(struct pdb_context *ctx);

/*
 * Close the PDB context and free any allocated lookup tables.
 * Does NOT close ctx->fd — caller owns the file descriptor.
 */
void pdb_close(struct pdb_context *ctx);

/*
 * First pass: build artist, album, and genre lookup tables.
 * Must be called before pdb_parse_tracks().
 *
 * Returns number of entries loaded (sum across all three tables),
 * or -1 on error.
 */
int pdb_build_lookup_tables(struct pdb_context *ctx);

/*
 * Second pass: parse all track rows and populate ctx->tracks[].
 * Resolves artist/album/genre names from lookup tables.
 *
 * max_tracks: caller-provided maximum (use PDB_MAX_TRACKS or less).
 * tracks_buf: caller-provided array of at least max_tracks pdb_track structs.
 *
 * Returns number of tracks parsed, or -1 on error.
 */
int pdb_parse_tracks(struct pdb_context *ctx,
                     struct pdb_track *tracks_buf, int max_tracks);

/*
 * Third pass: parse playlist_tree and playlist_entries tables.
 *
 * tree_buf / tree_max: caller-provided buffer for playlist tree entries.
 * entries_buf / entries_max: caller-provided buffer for membership entries.
 *
 * Returns total entries processed (tree + membership), or -1 on error.
 */
int pdb_parse_playlists(struct pdb_context *ctx,
                        struct pdb_playlist_tree_entry *tree_buf, int tree_max,
                        struct pdb_playlist_entry *entries_buf, int entries_max);

/*
 * Resolve a string ID against one of the lookup tables.
 * Returns the name string, or "" if not found.
 */
const char *pdb_lookup_string(const struct pdb_string_table *tbl, uint32_t id);

#endif /* _PDB_PARSER_H */
