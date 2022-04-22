/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "string-extra.h"
#include "menu.h"
#include "playlist_menu.h"

#include "file.h"
#include "keyboard.h"
#include "playlist.h"
#include "tree.h"
#include "playlist_viewer.h"
#include "talk.h"
#include "playlist_catalog.h"
#include "splash.h"
#include "filetree.h"

/* load a screen to save the playlist passed in (or current playlist if NULL is passed) */
int save_playlist_screen(struct playlist_info* playlist)
{

    char directoryonly[MAX_PATH+3];
    char *filename;

    int resume_index;
    uint32_t resume_elapsed;
    uint32_t resume_offset;

    char temp[MAX_PATH+1], *dot;
    int len;

    playlist_get_name(playlist, temp, sizeof(temp)-1);

    len = strlen(temp);
    dot = strrchr(temp, '.');

    if (!dot && len <= 1)
    {
        snprintf(temp, sizeof(temp), "%s%s",
                 catalog_get_directory(), DEFAULT_DYNAMIC_PLAYLIST_NAME);
    }

    dot = strrchr(temp, '.');
    if (dot) /* remove extension */
       *dot = '\0';

    if (!kbd_input(temp, sizeof(temp), NULL))
    {
        len = strlen(temp);
        if(len > 4 && !strcasecmp(&temp[len-4], ".m3u"))
            strlcat(temp, "8", sizeof(temp));
        else if(len <= 5 || strcasecmp(&temp[len-5], ".m3u8"))
            strlcat(temp, ".m3u8", sizeof(temp));

        playlist_save(playlist, temp, NULL, 0);

        /* reload in case playlist was saved to cwd */
        reload_directory();

        /* only reload newly saved playlist if:
         * playlist is null AND setting is turned on
         *
         * if playlist is null, we should be dealing with the current playlist,
         * and thus we got here from the wps screen. That means we want to reload
         * the current playlist so the user can make bookmarks. */
        if ((global_settings.playlist_reload_after_save == true) &&
            (playlist == NULL))
        {

            /* at least one slash exists in temp */
            if (strrchr(temp, '/') != NULL)
            {
                filename = strrchr(temp, '/') + 1;

                if (temp[0] == '/') /* most common situation - first char is a slash */
                {
                    strcpy(directoryonly, temp);
                    directoryonly[filename - temp] = '\0';
                } else /* there is a slash, but not at the beginning of temp - prepend one */
                {
                    directoryonly[0] = '/';

                    strcpy(directoryonly+1, temp);
                    directoryonly[filename - temp + 1] = '\0';
                }
            } else /* temp doesn't contain any slashes, uncommon? */
            {
                directoryonly[0] = '/';
                directoryonly[1] = '\0';
                filename = temp;
            }

            /* can't trust index from id3 (don't know why), get it from playlist */
            resume_index = playlist_get_current()->index;

            struct mp3entry* id3 = audio_current_track();

            /* record elapsed and offset so they don't change when we load new playlist */
            resume_elapsed = id3->elapsed;
            resume_offset = id3->offset;

            ft_play_playlist(temp, directoryonly, filename, true);
            playlist_start(resume_index, resume_elapsed, resume_offset);
        }
    /* cancelled out of name selection */
    } else {
        return 1;
    }

    return 0;
}

static int playlist_view_(void)
{
    playlist_viewer_ex(NULL);
    return 0;
}
MENUITEM_FUNCTION(create_playlist_item, 0, ID2P(LANG_CREATE_PLAYLIST),
                  create_playlist, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(view_cur_playlist, 0,
                  ID2P(LANG_VIEW_DYNAMIC_PLAYLIST),
                  playlist_view_, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(save_playlist, MENU_FUNC_USEPARAM, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),
                  save_playlist_screen, NULL, NULL, Icon_NOICON);
MENUITEM_SETTING(recursive_dir_insert, &global_settings.recursive_dir_insert, NULL);
static int clear_catalog_directory(void)
{
    catalog_set_directory(NULL);
    settings_save();
    splash(HZ, ID2P(LANG_RESET_DONE_CLEAR));
    return false;
}
MENUITEM_FUNCTION(clear_catalog_directory_item, 0, ID2P(LANG_RESET_PLAYLISTCAT_DIR),
                  clear_catalog_directory, NULL, NULL, Icon_file_view_menu);

/* Playlist viewer settings submenu */
MENUITEM_SETTING(show_icons, &global_settings.playlist_viewer_icons, NULL);
MENUITEM_SETTING(show_indices, &global_settings.playlist_viewer_indices, NULL);
MENUITEM_SETTING(track_display,
                 &global_settings.playlist_viewer_track_display, NULL);
MAKE_MENU(viewer_settings_menu, ID2P(LANG_PLAYLISTVIEWER_SETTINGS),
          NULL, Icon_Playlist,
          &show_icons, &show_indices, &track_display);

/* Current Playlist submenu */
MENUITEM_SETTING(warn_on_erase, &global_settings.warnon_erase_dynplaylist, NULL);
MENUITEM_SETTING(keep_current_track_on_replace, &global_settings.keep_current_track_on_replace_playlist, NULL);
MENUITEM_SETTING(show_shuffled_adding_options, &global_settings.show_shuffled_adding_options, NULL);
MENUITEM_SETTING(show_queue_options, &global_settings.show_queue_options, NULL);
MENUITEM_SETTING(playlist_reload_after_save, &global_settings.playlist_reload_after_save, NULL);
MAKE_MENU(currentplaylist_settings_menu, ID2P(LANG_CURRENT_PLAYLIST),
          NULL, Icon_Playlist,
          &warn_on_erase,
          &keep_current_track_on_replace,
          &show_shuffled_adding_options,
          &show_queue_options,
          &playlist_reload_after_save);

MAKE_MENU(playlist_settings, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &viewer_settings_menu, &recursive_dir_insert, &currentplaylist_settings_menu);
MAKE_MENU(playlist_options, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &create_playlist_item, &view_cur_playlist,
          &save_playlist, &clear_catalog_directory_item);
