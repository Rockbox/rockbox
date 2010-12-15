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
#include "filefuncs.h"
#include "onplay.h"
#include "playlist.h"
#include "settings.h"
#include "splash.h"
#include "tree.h"
#include "yesno.h"
#include "filetypes.h"
#include "debug.h"
#include "playlist_catalog.h"
#include "talk.h"

/* Use for recursive directory search */
struct add_track_context {
    int fd;
    int count;
};

/* keep track of most recently used playlist */
static char most_recent_playlist[MAX_PATH];

/* directory where our playlists our stored */
static char playlist_dir[MAX_PATH];
static int  playlist_dir_length;
static bool playlist_dir_exists = false;

/* Retrieve playlist directory from config file and verify it exists */
static int initialize_catalog(void)
{
    static bool initialized = false;

    if (!initialized)
    {
        bool default_dir = true;

        /* directory config is of the format: "dir: /path/to/dir" */
        if (global_settings.playlist_catalog_dir[0] &&
            strcmp(global_settings.playlist_catalog_dir,
                   PLAYLIST_CATALOG_DEFAULT_DIR))
        {
            strcpy(playlist_dir, global_settings.playlist_catalog_dir);
            default_dir = false;
        }

        /* fall back to default directory if no or invalid config */
        if (default_dir)
        {
            strcpy(playlist_dir, PLAYLIST_CATALOG_DEFAULT_DIR);
            if (!dir_exists(playlist_dir))
                mkdir(playlist_dir);
        }

        playlist_dir_length = strlen(playlist_dir);

        if (dir_exists(playlist_dir))
        {
            playlist_dir_exists = true;
            memset(most_recent_playlist, 0, sizeof(most_recent_playlist));
            initialized = true;
        }
    }

    if (!playlist_dir_exists)
    {
        if (mkdir(playlist_dir) < 0) {
            splashf(HZ*2, ID2P(LANG_CATALOG_NO_DIRECTORY), playlist_dir);
            return -1;
        }
        else {
            playlist_dir_exists = true;
            memset(most_recent_playlist, 0, sizeof(most_recent_playlist));
            initialized = true;
        }
    }

    return 0;
}

/* Display all playlists in catalog.  Selected "playlist" is returned.
   If "view" mode is set then we're not adding anything into playlist. */
static int display_playlists(char* playlist, bool view)
{
    struct browse_context browse;
    char selected_playlist[MAX_PATH];
    int result = -1;

    browse_context_init(&browse, SHOW_M3U,
                        BROWSE_SELECTONLY|(view? 0: BROWSE_NO_CONTEXT),
                        str(LANG_CATALOG), NOICON,
                        playlist_dir, most_recent_playlist);

    browse.buf = selected_playlist;
    browse.bufsize = sizeof(selected_playlist);

    rockbox_browse(&browse);

    if (browse.flags & BROWSE_SELECTED)
    {
        strlcpy(most_recent_playlist, selected_playlist+playlist_dir_length+1,
            sizeof(most_recent_playlist));

        if (view)
        {
            char *filename = strrchr(selected_playlist, '/')+1;
            /* In view mode, selecting a playlist starts playback */
            ft_play_playlist(selected_playlist, playlist_dir, filename);
            result = 0;
        }
        else
        {
            result = 0;
            strlcpy(playlist, selected_playlist, MAX_PATH);
        }
    }

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
static int add_to_playlist(const char* playlist, bool new_playlist,
                           const char* sel, int sel_attr)
{
    int fd;
    int result = -1;

    if (new_playlist)
        fd = open_utf8(playlist, O_CREAT|O_WRONLY|O_TRUNC);
    else
        fd = open(playlist, O_CREAT|O_WRONLY|O_APPEND, 0666);

    if(fd < 0)
        return result;

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
        bool recurse = false;
        const char *lines[] = {
            ID2P(LANG_RECURSE_DIRECTORY_QUESTION), sel};
        const struct text_message message={lines, 2};
        struct add_track_context context;

        if (global_settings.recursive_dir_insert != RECURSE_ASK)
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
static bool in_cat_viewer = false;
bool catalog_view_playlists(void)
{
    bool retval = true;
    if (in_cat_viewer)
        return false;

    if (initialize_catalog() == -1)
        return false;

    in_cat_viewer = true;
    retval = (display_playlists(NULL, true) != -1);
    in_cat_viewer = false;
    return retval;
}

static bool in_add_to_playlist = false;
bool catalog_add_to_a_playlist(const char* sel, int sel_attr,
                               bool new_playlist, char *m3u8name)
{
    int result;
    char playlist[MAX_PATH];
    if (in_add_to_playlist)
        return false;

    if (initialize_catalog() == -1)
        return false;

    if (new_playlist)
    {
        size_t len;
        if (m3u8name == NULL)
        {
            snprintf(playlist, MAX_PATH, "%s/", playlist_dir);
            if (kbd_input(playlist, MAX_PATH))
                return false;
        }
        else
            strcpy(playlist, m3u8name);

        len = strlen(playlist);

        if(len > 4 && !strcasecmp(&playlist[len-4], ".m3u"))
            strcat(playlist, "8");
        else if(len <= 5 || strcasecmp(&playlist[len-5], ".m3u8"))
            strcat(playlist, ".m3u8");
    }
    else
    {
        in_add_to_playlist = true;
        result = display_playlists(playlist, false);
        in_add_to_playlist = false;

        if (result == -1)
            return false;
    }

    if (add_to_playlist(playlist, new_playlist, sel, sel_attr) == 0)
        return true;
    else
        return false;
}
