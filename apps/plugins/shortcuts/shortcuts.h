/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Bryan Childs
 * Copyright (c) 2007 Alexander Levin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _SHORTCUTS_H
#define _SHORTCUTS_H

#include "plugin.h"

#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_LEN 1 /* strlen(PATH_SEPARATOR) */

#if defined(DEBUG) || defined(SIMULATOR)
#define SC_DEBUG
#endif

#define SHORTCUTS_FILENAME "/shortcuts.link"

extern struct plugin_api* rb;

typedef struct sc_entry_s
{
    char path[MAX_PATH+1];
    char disp[MAX_PATH+1];
    bool explicit_disp;
} sc_entry_t;

typedef struct sc_file_s
{
    sc_entry_t *entries;
    int max_entries; /* Max allowed number of entries */
    int entry_cnt; /* Current number of entries */
    int show_last_segments;
} sc_file_t;


extern void *memory_buf;
extern long memory_bufsize;


extern sc_file_t sc_file;


/* Allocates a chunk of memory (as much as possible) */
void allocate_memory(void **buf, size_t *bufsize);

/* Initializes the file */
void init_sc_file(sc_file_t *file, void *buf, size_t bufsize);

/* Loads shortcuts from the file. Returns true iff succeeded */
bool load_sc_file(sc_file_t *file, char* filename, bool must_exist,
        void *entry_buf, size_t entry_bufsize);

/* Writes contents to the file. File is overwritten. */
bool dump_sc_file(sc_file_t *file, char *filename);

/* Appends the entry to the file. Entry is copied. Returns true iff succeded. */
bool append_entry(sc_file_t *file, sc_entry_t *entry);

/* Removes the specified entry (0-based index). Returns true iff succeded. */
bool remove_entry(sc_file_t *file, int entry_idx);

/* Checks whether the index is a valid one for the file. */
bool is_valid_index(sc_file_t *file, int entry_idx);


#ifdef SC_DEBUG
void print_file(sc_file_t *file);
#endif

#endif
