/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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
#ifndef _TREE_H_
#define _TREE_H_

#include <stdbool.h>
#include <applimits.h>
#include <file.h>
#include "icon.h"

struct entry {
    short attr; /* FAT attributes + file type flags */
    unsigned long time_write; /* Last write time */
    char *name;
};


#define BROWSE_SELECTONLY   0x0001  /* exit on selecting a file */
#define BROWSE_NO_CONTEXT   0x0002  /* disable context menu */
#define BROWSE_SELECTED     0x0100  /* this bit is set if user selected item */

struct tree_context;

struct browse_context {
    int dirfilter;
    unsigned flags;             /* ored BROWSE_* */
    bool (*callback_show_item)(char *name, int attr, struct tree_context *tc);
                                /* callback function to determine to show/hide
                                   the item for custom browser */
    char *title;                /* title of the browser. if set to NULL,
                                   directory name is used. */
    enum themable_icons icon;   /* title icon */
    const char *root;           /* full path of start directory */
    const char *selected;       /* name of selected file in the root */
    char *buf;                  /* buffer to store selected file */
    size_t bufsize;             /* size of the buffer */
};

/* browser context for file or db */
struct tree_context {
    /* The directory we are browsing */
    char currdir[MAX_PATH];
    /* the number of directories we have crossed from / */
    int dirlevel;
    /* The currently selected file/id3dbitem index (old dircursor+dirfile) */
    int selected_item;
    /* The selected item in each directory crossed
     * (used when we want to return back to a previouws directory)*/
    int selected_item_history[MAX_DIR_LEVELS];

    int firstpos; /* which dir entry is on first
                     position in dir buffer */
    int pos_history[MAX_DIR_LEVELS];

    int *dirfilter; /* file use */
    int filesindir; /* The number of files in the dircache */
    int dirsindir; /* file use */
    int dirlength; /* total number of entries in dir, incl. those not loaded */
#ifdef HAVE_TAGCACHE
    int table_history[MAX_DIR_LEVELS]; /* db use */
    int extra_history[MAX_DIR_LEVELS]; /* db use */
    int currtable; /* db use */
    int currextra; /* db use */
#endif
    /* A big buffer with plenty of entry structs,
     * contains all files and dirs in the current
     * dir (with filters applied) */
    void* dircache;
    int dircache_size;
    char* name_buffer;
    int name_buffer_size;
    int dentry_size;
    bool dirfull;
    int sort_dir; /* directory sort order */
    struct browse_context *browse;
};

void tree_drawlists(void);
void tree_mem_init(void) INIT_ATTR;
void tree_gui_init(void) INIT_ATTR;
char* get_current_file(char* buffer, size_t buffer_len);
void set_dirfilter(int l_dirfilter);
void set_current_file(const char *path);
void browse_context_init(struct browse_context *browse,
                         int dirfilter, unsigned flags,
                         char *title, enum themable_icons icon,
                         const char *root, const char *selected);
int rockbox_browse(struct browse_context *browse);
bool create_playlist(void);
void resume_directory(const char *dir);
#ifdef WIN32
/* it takes an int on windows */
#define getcwd_size_t int
#else
#define getcwd_size_t size_t
#endif
char *getcwd(char *buf, getcwd_size_t size);
void reload_directory(void);
bool check_rockboxdir(void);
struct tree_context* tree_get_context(void);
void tree_flush(void);
void tree_restore(void);

bool bookmark_play(char* resume_file, int index, int offset, int seed,
                   char *filename);

extern struct gui_synclist tree_lists;
#endif
