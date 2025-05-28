/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Sebastian Henriksen, Hardeep Sidhu
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "string-extra.h"
#include "action.h"
#include "dir.h"
#include "file.h"
#include "filetree.h"
#include "kernel.h"
#include "keyboard.h"
#include "lang.h"
#include "list.h"
#include "misc.h"
#include "pathfuncs.h"
#include "onplay.h"
#include "playlist.h"
#include "settings.h"
#include "rbpaths.h"
#include "splash.h"
#include "tree.h"
#include "yesno.h"
#include "filetypes.h"
#include "debug.h"
#include "playlist_catalog.h"
#include "talk.h"
#include "playlist_viewer.h"
#include "bookmark.h"
#include "root_menu.h"
#include "general.h"

/* Use for recursive directory search */
struct add_track_context {
    int fd;
    int count;
};

enum catbrowse_status_flags{
    CATBROWSE_NOTHING = 0,
    CATBROWSE_CATVIEW,
    CATBROWSE_PLAYLIST
};

/* keep track of most recently used playlist */
static char most_recent_playlist[MAX_PATH];
/* we need playlist_dir_length for easy removal of playlist dir prefix */
static size_t playlist_dir_length;
/* keep track of what browser(s) are current to prevent reentry */
static int browser_status = CATBROWSE_NOTHING;

static size_t get_directory(char* dirbuf, size_t dirbuf_sz)
{
    const char *pl_dir = PLAYLIST_CATALOG_DEFAULT_DIR;

    /* directory config is of the format: "dir: /path/to/dir" */
    if (global_settings.playlist_catalog_dir[0] != '\0')
    {
        pl_dir = global_settings.playlist_catalog_dir;
    }

    return path_append(dirbuf, pl_dir, PA_SEP_SOFT, dirbuf_sz);
}

/* Retrieve playlist directory from config file and verify it exists
 * attempts to create directory
 * catalog dir is returned in dirbuf */
static int initialize_catalog_buf(char* dirbuf, size_t dirbuf_sz)
{
    playlist_dir_length = get_directory(dirbuf, dirbuf_sz);
    if (playlist_dir_length >= dirbuf_sz)
    {
        return -2;
    }

    if (!dir_exists(dirbuf))
    {
        if (mkdir(dirbuf) < 0) {
            if (global_settings.talk_menu) {
                talk_id(LANG_CATALOG_NO_DIRECTORY, true);
                talk_dir_or_spell(dirbuf, NULL, true);
                talk_force_enqueue_next();
            }
            /* (voiced above) */
            splashf(HZ*2, str(LANG_CATALOG_NO_DIRECTORY), dirbuf);
            return -1;
        }
        else {
            memset(most_recent_playlist, 0, sizeof(most_recent_playlist));
        }
    }

    return 0;
}

/* Retrieve playlist directory from config file and verify it exists
 * attempts to create directory
 * Don't inline as we want the stack to be freed ASAP */
static NO_INLINE int initialize_catalog(void)
{
    char playlist_dir[MAX_PATH];

    return initialize_catalog_buf(playlist_dir, sizeof(playlist_dir));
}

void catalog_set_directory(const char* directory)
{
    if (directory == NULL)
    {
        global_settings.playlist_catalog_dir[0] = '\0';
    }
    else
    {
        path_append(global_settings.playlist_catalog_dir, directory,
                    PA_SEP_SOFT, sizeof(global_settings.playlist_catalog_dir));
    }
    initialize_catalog();
}

void catalog_get_directory(char* dirbuf, size_t dirbuf_sz)
{
    if (initialize_catalog_buf(dirbuf, dirbuf_sz) < 0)
    {
        dirbuf[0] = '\0';
        return;
    }
}

/* Display all playlists in catalog.  Selected "playlist" is returned.
 * If status is CATBROWSE_CATVIEW then we're not adding anything into playlist */
static int display_playlists(char* playlist, enum catbrowse_status_flags status)
{
    static bool reopen_last_playlist = false;
    static int most_recent_selection = 0;
    int result = -1;
    char selected_playlist[MAX_PATH];
    selected_playlist[0] = '\0';

    browser_status |= status;
    bool view = (status == CATBROWSE_CATVIEW);

    struct browse_context browse = {
        .dirfilter = SHOW_M3U,
        .flags = BROWSE_SELECTONLY | (view ? 0 : BROWSE_NO_CONTEXT_MENU),
        .title = str(LANG_PLAYLISTS),
        .icon = Icon_Playlist,
        .root = selected_playlist,
        .selected = &most_recent_playlist[playlist_dir_length + 1],
        .buf = selected_playlist,
        .bufsize = sizeof(selected_playlist),
    };

restart:
    /* set / restore the root directory for the browser */
    catalog_get_directory(selected_playlist, sizeof(selected_playlist));
    browse.flags &= ~BROWSE_SELECTED;

    if (view && reopen_last_playlist)
    {
        switch (playlist_viewer_ex(most_recent_playlist, &most_recent_selection))
        {
            case PLAYLIST_VIEWER_OK:
            {
                result = 0;
                break;
            }
            case PLAYLIST_VIEWER_CANCEL:
            {
                reopen_last_playlist = false;
                goto restart;
            }
            case PLAYLIST_VIEWER_USB:
            case PLAYLIST_VIEWER_MAINMENU:
            /* Fall through */
            default:
                break;
        }
    }
    else /* browse playlist dir */
    {
        int browse_ret = rockbox_browse(&browse);
        if (browse_ret == GO_TO_WPS
            || (view && browse_ret == GO_TO_PREVIOUS_MUSIC))
            result = 0;
    }

    if (browse.flags & BROWSE_SELECTED) /* User picked a playlist */
    {
        if (strcmp(most_recent_playlist, selected_playlist)) /* isn't most recent one */
        {
            path_append(most_recent_playlist, selected_playlist,
                        PA_SEP_SOFT, sizeof(most_recent_playlist));
            most_recent_selection = 0;
            reopen_last_playlist = false;
        }

        if (view) /* display playlist contents or resume bookmark */
        {

            int res = bookmark_autoload(selected_playlist);
            if (res == BOOKMARK_DO_RESUME)
               result = 0;
            else if (res == BOOKMARK_CANCEL)
                goto restart;
            else
            {
                switch (playlist_viewer_ex(selected_playlist, &most_recent_selection)) {
                    case PLAYLIST_VIEWER_OK:
                    {
                        reopen_last_playlist = true;
                        result = 0;
                        break;
                    }
                    case PLAYLIST_VIEWER_CANCEL:
                    {
                        goto restart;
                    }
                    case PLAYLIST_VIEWER_USB:
                    case PLAYLIST_VIEWER_MAINMENU:
                    /* Fall through */
                    default:
                        reopen_last_playlist = true;
                        break;
                }
            }
        }
        else /* we're just adding something to a playlist */
        {
            result = 0;
            strmemccpy(playlist, selected_playlist, MAX_PATH);
        }
    }
    browser_status &= ~status;
    return result;
}

/* display number of tracks inserted into playlists.  Used for directory
   insert */
static void display_insert_count(int count)
{
    static long talked_tick = 0;
    if(global_settings.talk_menu && count &&
        (talked_tick == 0 || TIME_AFTER(current_tick, talked_tick+5*HZ)))
    {
        talked_tick = current_tick;
        talk_number(count, false);
        talk_id(LANG_PLAYLIST_INSERT_COUNT, true);
    }
    /* (voiced above) */
    splashf(0, str(LANG_PLAYLIST_INSERT_COUNT), count, str(LANG_OFF_ABORT));
}

/* Add specified track into playlist.  Callback from directory insert */
static int add_track_to_playlist(char* filename, void* context)
{
    struct add_track_context* c = (struct add_track_context*) context;

    if (fdprintf(c->fd, "%s\n", filename) <= 0)
        return -1;

    (c->count)++;

    if (((c->count)%PLAYLIST_DISPLAY_COUNT) == 0)
        display_insert_count(c->count);

    return 0;
}

/* Add "sel" file into specified "playlist".  How to insert depends on type
   of file */
int catalog_insert_into(const char* playlist, bool new_playlist,
                        const char* sel, int sel_attr)
{
    int fd;
    int result = -1;

    if (new_playlist)
        fd = open_utf8(playlist, O_CREAT|O_WRONLY|O_TRUNC);
    else
        fd = open(playlist, O_CREAT|O_WRONLY|O_APPEND, 0666);

    if(fd < 0)
    {
        splash(HZ*2, ID2P(LANG_FAILED));
        return result;
    }
    /* In case we're in the playlist directory */
    reload_directory();

    if ((sel_attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO)
    {
        /* append the selected file */
        if (fdprintf(fd, "%s\n", sel) > 0)
            result = 0;
    }
    else if ((sel_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U)
    {
        /* append playlist */
        int f, fs, i;
        char buf[1024];

        if(strcasecmp(playlist, sel) == 0)
            goto exit;

        f = open_utf8(sel, O_RDONLY);
        if (f < 0)
            goto exit;

        i = lseek(f, 0, SEEK_CUR);
        fs = filesize(f);
        while (i < fs)
        {
            int n;

            n = read(f, buf, sizeof(buf));
            if (n < 0)
                break;

            if (write(fd, buf, n) < 0)
                break;

            i += n;
        }

        if (i >= fs)
            result = 0;

        close(f);
    }
    else if (sel_attr & ATTR_DIRECTORY)
    {
        /* search directory for tracks and append to playlist */
        bool recurse;
        const char *lines[] = {
            ID2P(LANG_RECURSE_DIRECTORY_QUESTION), sel};
        const struct text_message message={lines, 2};
        struct add_track_context context;


        if (sel[1] == '\0' && sel[0] == PATH_ROOTCHR)
            recurse = true;
        else if (global_settings.recursive_dir_insert != RECURSE_ASK)
            recurse = (bool)global_settings.recursive_dir_insert;
        else
        {
            /* Ask if user wants to recurse directory */
            recurse = (gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES);
        }

        context.fd = fd;
        context.count = 0;

        display_insert_count(0);

        result = playlist_directory_tracksearch(sel, recurse,
            add_track_to_playlist, &context);

        display_insert_count(context.count);
    }

exit:
    close(fd);
    return result;
}

bool catalog_view_playlists(void)
{
    if ((browser_status & CATBROWSE_CATVIEW) == CATBROWSE_CATVIEW)
        return false;

    if (initialize_catalog() < 0)
        return false;

    return (display_playlists(NULL, CATBROWSE_CATVIEW) >= 0);
}

static void apply_playlist_extension(char* buf, size_t buf_size)
{
    size_t len = strlen(buf);
    if(len > 4 && !strcasecmp(&buf[len-4], ".m3u"))
        strlcat(buf, "8", buf_size);
    else if(len <= 5 || strcasecmp(&buf[len-5], ".m3u8"))
        strlcat(buf, ".m3u8", buf_size);
}

static int remove_extension(char* path)
{
    char *dot = strrchr(path, '.');
    if (dot)
       *dot = '\0';

    return 0;
}


bool catalog_pick_new_playlist_name(char *pl_name, size_t buf_size,
                                    const char* curr_pl_name)
{
    char bmark_file[MAX_PATH + 7], *p;
    bool do_save = false;
    while (!do_save && !remove_extension(pl_name) &&
           !kbd_input(pl_name, buf_size - 7, NULL))
    {
        if (*pl_name == PATH_SEPCH) /* Require absolute filenames */
        {
            p = strrchr(pl_name, PATH_SEPCH);
            do_save = *(p + 1);   /* Disallow empty filename */

            /* prevent illegal characters */
            fix_path_part(p, 1, strlen(p + 1));

            /* Create dir if necessary */
            if (do_save && p != (char *) pl_name)
            {
                *p = '\0';
                do_save = dir_exists(pl_name) || mkdir(pl_name) == 0;
                if (!do_save)
                    splashf(HZ*2, ID2P(LANG_CATALOG_NO_DIRECTORY), pl_name);
                *p = PATH_SEPCH;
            }
        }
        apply_playlist_extension(pl_name, buf_size);

        /* warn before overwriting existing (different) playlist */
        if (do_save && (!curr_pl_name || strcmp(curr_pl_name, pl_name)))
        {
            if (file_exists(pl_name))
                do_save = yesno_pop(ID2P(LANG_REALLY_OVERWRITE));

            if (do_save) /* delete bookmark file unrelated to new playlist */
            {
                snprintf(bmark_file, sizeof(bmark_file), "%s.bmark", pl_name);
                if (file_exists(bmark_file))
                    remove(bmark_file);
            }
        }
    }
    return do_save;
}

static int (*ctx_add_to_playlist)(const char* playlist, bool new_playlist);
bool catalog_add_to_a_playlist(const char* sel, int sel_attr,
                               bool new_playlist, char *m3u8name,
                               void (*add_to_pl_cb))
{
    int result;
    char playlist[MAX_PATH + 7]; /* room for /.m3u8\0*/
    size_t basename_start;
    if ((browser_status & CATBROWSE_PLAYLIST) == CATBROWSE_PLAYLIST)
        return false;

    if (initialize_catalog_buf(playlist, sizeof(playlist)) < 0)
        return false;

    if (new_playlist)
    {
        if (m3u8name == NULL)
        {
            const char *name;
            /* If sel is empty, root, or playlist directory  we use 'all' */
            if (!sel || !strcmp(sel, "/") || !strcmp(sel, playlist))
            {
                sel = "/";
                name = "/all";
            }
            else /*If sel is a folder, we prefill the text field with its name*/
                name = strrchr(sel, '/');

            if (name == NULL || ((sel_attr & ATTR_DIRECTORY) != ATTR_DIRECTORY))
                create_numbered_filename(playlist, playlist, PLAYLIST_UNTITLED_PREFIX,
                                         ".m3u8", 1 IF_CNFN_NUM_(, NULL));
            else
            {
                basename_start = strlen(playlist) + 1;
                strlcat(playlist, name, sizeof(playlist));
                fix_path_part(playlist, basename_start,
                              sizeof(playlist) - 1 - basename_start) ;
                apply_playlist_extension(playlist, sizeof(playlist));
            }
        }
        else
            strmemccpy(playlist, m3u8name, sizeof(playlist));

        if (!catalog_pick_new_playlist_name(playlist, sizeof(playlist), NULL))
            return false;
    }
    else
    {
        if (display_playlists(playlist, CATBROWSE_PLAYLIST) < 0)
            return false;
    }

    if (add_to_pl_cb != NULL)
    {
        ctx_add_to_playlist = add_to_pl_cb;
        result = ctx_add_to_playlist(playlist, new_playlist);
    }
    else
        result = catalog_insert_into(playlist, new_playlist, sel, sel_attr);

    return (result == 0);
}
