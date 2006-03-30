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

/* Allow a little drift to the filename ordering. */
#define POS_HISTORY_COUNT 4

/* How many entries we can create in one tag file (for sorting). */
#define TAGFILE_MAX_ENTRIES  20000

#define SEEK_LIST_SIZE 50
#define TAGCACHE_MAX_FILTERS 3

struct tagcache_search {
    /* For internal use only. */
    int fd;
    long seek_list[SEEK_LIST_SIZE];
    long filter_tag[TAGCACHE_MAX_FILTERS];
    long filter_seek[TAGCACHE_MAX_FILTERS];
    int filter_count;
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

bool tagcache_search(struct tagcache_search *tcs, int tag);
bool tagcache_search_add_filter(struct tagcache_search *tcs,
                                int tag, int seek);
bool tagcache_get_next(struct tagcache_search *tcs);
void tagcache_search_finish(struct tagcache_search *tcs);
long tagcache_get_numeric(const struct tagcache_search *tcs, int tag);

int tagcache_get_progress(void);
#ifdef HAVE_TC_RAMCACHE
bool tagcache_is_ramcache(void);
bool tagcache_fill_tags(struct mp3entry *id3, const char *filename);
#endif
void tagcache_init(void);
void tagcache_start_scan(void);
void tagcache_stop_scan(void);
bool tagcache_force_update(void);

#endif
