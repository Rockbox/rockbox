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
#include <string.h>
#include "action.h"
#include "dir.h"
#include "file.h"
#include "filetree.h"
#include "kernel.h"
#include "keyboard.h"
#include "lang.h"
#include "list.h"
#include "misc.h"
#include "onplay.h"
#include "playlist.h"
#include "settings.h"
#include "splash.h"
#include "sprintf.h"
#include "tree.h"
#include "yesno.h"
#include "filetypes.h"
#include "debug.h"
#include "playlist_catalog.h"
#include "statusbar.h"
#include "talk.h"

#define MAX_PLAYLISTS 400

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
        if (global_settings.playlist_catalog_dir[0])
        {
            strcpy(playlist_dir, global_settings.playlist_catalog_dir);
            default_dir = false;
        }

        /* fall back to default directory if no or invalid config */
        if (default_dir)
            strncpy(playlist_dir, PLAYLIST_CATALOG_DEFAULT_DIR,
                sizeof(playlist_dir));

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
/* Use the filetree functions to retrieve the list of playlists in the
   directory */
static int create_playlist_list(char** playlists, int num_items,
                                int* num_playlists)
{
    int result = -1;
    int num_files = 0;
    int index = 0;
    int i;
    bool most_recent = false;
    struct entry *files;
    struct tree_context* tc = tree_get_context();
    int dirfilter = *(tc->dirfilter);

    *num_playlists = 0;

    /* use the tree browser dircache to load only playlists */
    *(tc->dirfilter) = SHOW_PLAYLIST;
    
    if (ft_load(tc, playlist_dir) < 0)
    {
        splashf(HZ*2, ID2P(LANG_CATALOG_NO_DIRECTORY), playlist_dir);
        goto exit;
    }
    
    files = (struct entry*) tc->dircache;
    num_files = tc->filesindir;
    
    /* we've overwritten the dircache so tree browser will need to be
       reloaded */
    reload_directory();
    
    /* if it exists, most recent playlist will always be index 0 */
    if (most_recent_playlist[0] != '\0')
    {
        index = 1;
        most_recent = true;
    }

    for (i=0; i<num_files && index<num_items; i++)
    {
        if (files[i].attr & FILE_ATTR_M3U)
        {
            if (most_recent && !strncmp(files[i].name, most_recent_playlist,
                sizeof(most_recent_playlist)))
            {
                playlists[0] = files[i].name;
                most_recent = false;
            }
            else
            {
                playlists[index] = files[i].name;
                index++;
            }
        }
    }

    *num_playlists = index;

    /* we couldn't find the most recent playlist, shift all playlists up */
    if (most_recent)
    {
        for (i=0; i<index-1; i++)
            playlists[i] = playlists[i+1];

        (*num_playlists)--;

        most_recent_playlist[0] = '\0';
    }

    result = 0;

exit:
    *(tc->dirfilter) = dirfilter;
    return result;
}

/* Callback for gui_synclist */
static char* playlist_callback_name(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    char** playlists = (char**) data;

    strncpy(buffer, playlists[selected_item], buffer_len);

    if (buffer[0] != '.' && !(global_settings.show_filename_ext == 1
        || (global_settings.show_filename_ext == 3
            && global_settings.dirfilter == 0)))
    {
        char* dot = strrchr(buffer, '.');

        if (dot != NULL)
        {
            *dot = '\0';
        }
    }

    return buffer;
}

static int playlist_callback_voice(int selected_item, void* data)
{
    char** playlists = (char**) data;
    talk_file_or_spell(playlist_dir, playlists[selected_item], NULL, false);
    return 0;
}

/* Display all playlists in catalog.  Selected "playlist" is returned.
   If "view" mode is set then we're not adding anything into playlist. */
static int display_playlists(char* playlist, bool view)
{
    int result = -1;
    int num_playlists = 0;
    bool exit = false;
    char temp_buf[MAX_PATH];
    char* playlists[MAX_PLAYLISTS];
    struct gui_synclist playlist_lists;

    if (create_playlist_list(playlists, sizeof(playlists),
            &num_playlists) != 0)
        return -1;

    if (num_playlists <= 0)
    {
        splash(HZ*2, ID2P(LANG_CATALOG_NO_PLAYLISTS));
        return -1;
    }

    if (!playlist)
        playlist = temp_buf;

    gui_synclist_init(&playlist_lists, playlist_callback_name, playlists,
                       false, 1, NULL);
    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&playlist_lists,
                                        playlist_callback_voice);
    gui_synclist_set_nb_items(&playlist_lists, num_playlists);
    gui_synclist_draw(&playlist_lists);
    gui_synclist_speak_item(&playlist_lists);

    while (!exit)
    {
        int button;
        char* sel_file;
        list_do_action(CONTEXT_LIST,HZ/2,
                       &playlist_lists, &button,LIST_WRAP_UNLESS_HELD);
        sel_file = playlists[gui_synclist_get_sel_pos(&playlist_lists)];

        switch (button)
        {
            case ACTION_STD_CANCEL:
                exit = true;
                break;

            case ACTION_STD_OK:
                snprintf(playlist, MAX_PATH, "%s/%s", playlist_dir, sel_file);

                if (view)
                {
                    /* In view mode, selecting a playlist starts playback */
                    ft_play_playlist(playlist, playlist_dir, sel_file);
                }

                result = 0;
                exit = true;
                break;

            case ACTION_STD_CONTEXT:
                /* context menu only available in view mode */
                if (view)
                {
                    snprintf(playlist, MAX_PATH, "%s/%s", playlist_dir,
                        sel_file);

                    if (onplay(playlist, FILE_ATTR_M3U,
                            CONTEXT_TREE) != ONPLAY_OK)
                    {
                        result = 0;
                        exit = true;
                    }
                    else
                    {
                        gui_synclist_draw(&playlist_lists);
                        gui_synclist_speak_item(&playlist_lists);
                    }
                }
                break;

            case ACTION_NONE:
                gui_syncstatusbar_draw(&statusbars, false);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    result = -1;
                    exit = true;
                }
                break;
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
    int flags = O_CREAT|O_WRONLY;
    
    if (new_playlist)
        flags |= O_TRUNC;
    else
        flags |= O_APPEND;
    
    fd = open(playlist, flags);
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
        
        f = open(sel, O_RDONLY);
        if (f < 0)
            goto exit;
        
        fs = filesize(f);
        
        for (i=0; i<fs;)
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

bool catalog_view_playlists(void)
{
    if (initialize_catalog() == -1)
        return false;

    if (display_playlists(NULL, true) == -1)
        return false;

    return true;
}

bool catalog_add_to_a_playlist(const char* sel, int sel_attr,
                               bool new_playlist, char *m3u8name)
{
    char playlist[MAX_PATH];
    
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
        if (display_playlists(playlist, false) == -1)
            return false;
    }

    if (add_to_playlist(playlist, new_playlist, sel, sel_attr) == 0)
    {
        strncpy(most_recent_playlist, playlist+playlist_dir_length+1,
            sizeof(most_recent_playlist));
        return true;
    }
    else
        return false;
}
