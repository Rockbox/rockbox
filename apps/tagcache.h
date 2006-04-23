/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _TAGCACHE_H
#define _TAGCACHE_H

#include "id3.h"

enum tag_type { tag_artist = 0, tag_album, tag_genre, tag_title,
    tag_filename, tag_composer, tag_year, tag_tracknumber,
    tag_bitrate, tag_length };

#define TAG_COUNT 10

#ifdef HAVE_DIRCACHE
#define HAVE_TC_RAMCACHE 1
#endif

/* Allow a little drift to the filename ordering (should not be too high/low). */
#define POS_HISTORY_COUNT 4

/* How much to pre-load entries while committing to prevent seeking. */
#define IDX_BUF_DEPTH 64

/* Tag Cache Header version 'TCHxx'. Increment when changing internal structures. */
#define TAGCACHE_MAGIC  0x54434804

/* How much to allocate extra space for ramcache. */
#define TAGCACHE_RESERVE 32768

/* How many entries we can create in one tag file (for sorting). */
#define TAGFILE_MAX_ENTRIES  10000

/** 
 * Define how long one entry must be at least (longer -> less memory at commit).
 * Must be at least 4 bytes in length for correct alignment. 
 */
#define TAGFILE_ENTRY_CHUNK_LENGTH   8

/* Used to guess the necessary buffer size at commit. */
#define TAGFILE_ENTRY_AVG_LENGTH   16

/* How many entries to fetch to the seek table at once while searching. */
#define SEEK_LIST_SIZE 50

/* Always strict align entries for best performance and binary compatability. */
#define TAGCACHE_STRICT_ALIGN 1

#define TAGCACHE_MAX_FILTERS 3
#define TAGCACHE_MAX_CLAUSES 10

/* Tag database files. */
#define TAGCACHE_FILE_TEMP    ROCKBOX_DIR "/tagcache_tmp.tcd"
#define TAGCACHE_FILE_MASTER  ROCKBOX_DIR "/tagcache_idx.tcd"
#define TAGCACHE_FILE_INDEX   ROCKBOX_DIR "/tagcache_%d.tcd"

/* Flags */
#define FLAG_DELETED    0x0001

enum clause { clause_none, clause_is, clause_gt, clause_gteq, clause_lt, 
    clause_lteq, clause_contains, clause_begins_with, clause_ends_with  };
enum modifiers { clause_mod_none, clause_mod_not };

struct tagcache_stat {
    bool initialized;        /* Is tagcache currently busy? */
    bool ramcache;           /* Is tagcache loaded in ram? */
    bool commit_delayed;     /* Has commit been delayed until next reboot? */
    int  commit_step;        /* Commit progress */
    int  ramcache_allocated; /* Has ram been allocated for ramcache? */
    int  ramcache_used;      /* How much ram has been really used */
    int  progress;           /* Current progress of disk scan */
    int  processed_entries;  /* Scanned disk entries so far */
};

struct tagcache_search_clause
{
    int tag;
    int type;
    bool numeric;
    bool input;
    long numeric_data;
    char str[32];
};

struct tagcache_search {
    /* For internal use only. */
    int fd, masterfd;
    int idxfd[TAG_COUNT];
    long seek_list[SEEK_LIST_SIZE];
    long filter_tag[TAGCACHE_MAX_FILTERS];
    long filter_seek[TAGCACHE_MAX_FILTERS];
    int filter_count;
    struct tagcache_search_clause *clause[TAGCACHE_MAX_CLAUSES];
    int clause_count;
    int seek_list_count;
    int seek_pos;
    int idx_id;
    long position;
    int entry_count;
    bool valid;

    /* Exported variables. */
    bool ramsearch;
    int type;
    char *result;
    int result_len;
    long result_seek;
};

bool tagcache_is_numeric_tag(int type);
bool tagcache_is_unique_tag(int type);
bool tagcache_is_sorted_tag(int type);
bool tagcache_search(struct tagcache_search *tcs, int tag);
bool tagcache_search_add_filter(struct tagcache_search *tcs,
                                int tag, int seek);
bool tagcache_search_add_clause(struct tagcache_search *tcs,
                                struct tagcache_search_clause *clause);
bool tagcache_get_next(struct tagcache_search *tcs);
bool tagcache_retrieve(struct tagcache_search *tcs, int idxid, 
                       char *buf, long size);
void tagcache_search_finish(struct tagcache_search *tcs);
long tagcache_get_numeric(const struct tagcache_search *tcs, int tag);

struct tagcache_stat* tagcache_get_stat(void);
int tagcache_get_commit_step(void);

#ifdef HAVE_TC_RAMCACHE
bool tagcache_is_ramcache(void);
bool tagcache_fill_tags(struct mp3entry *id3, const char *filename);
#endif
void tagcache_init(void);
bool tagcache_is_initialized(void);
void tagcache_start_scan(void);
void tagcache_stop_scan(void);
bool tagcache_force_update(void);

#endif
