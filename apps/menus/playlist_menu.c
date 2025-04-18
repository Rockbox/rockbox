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
#include "general.h"
#include "pathfuncs.h"

/* load a screen to save the playlist passed in (or current playlist if NULL is passed) */
int save_playlist_screen(struct playlist_info* playlist)
{

    char directoryonly[MAX_PATH+3];

    char temp[MAX_PATH+1], *p;
    int len;
    bool audio_stopped = !(audio_status() & AUDIO_STATUS_PLAY);

    /* After a reboot, we may need to resume the playlist
       first, or it will not contain any indices when
       user selects "Save Current Playlist" from playlist
       catalogue's context menu in root menu */
    if (!playlist && audio_stopped && !playlist_get_current()->started)
    {
        if (playlist_resume() == -1)
       {
            splash(HZ, ID2P(LANG_CATALOG_NO_PLAYLISTS));
            return 0;
        }
    }


    catalog_get_directory(directoryonly, sizeof(directoryonly));
    playlist_get_name(playlist, temp, sizeof(temp)-1);

    len = strlen(temp);

    if (len <= 1) /* root or dynamic playlist */
        create_numbered_filename(temp, directoryonly, PLAYLIST_UNTITLED_PREFIX, ".m3u8",
                                 1 IF_CNFN_NUM_(, NULL));
    else if (temp[len - 1] == PATH_SEPCH) /* dir playlists other than root  */
    {
        temp[len - 1] = '\0';

        if ((p = strrchr(temp, PATH_SEPCH))) /* use last path component as playlist name */
        {
            strlcat(directoryonly, p, sizeof(directoryonly));
            strlcat(directoryonly, ".m3u8", sizeof(directoryonly));
            strmemccpy(temp, directoryonly, sizeof(temp));
        }
        else
            create_numbered_filename(temp, directoryonly, PLAYLIST_UNTITLED_PREFIX, ".m3u8",
                                     1 IF_CNFN_NUM_(, NULL));
    }
    else if (!playlist) /* current playlist loaded from a playlist file */
    {
        if (!file_exists(temp))
        {
            splashf(HZ*2, ID2P(LANG_CATALOG_NO_DIRECTORY), temp);
            return 0;
        }
    }

    if (catalog_pick_new_playlist_name(temp, sizeof(temp),
                                       playlist ? playlist->filename :
                                       playlist_get_current()->filename))
    {
        playlist_save(playlist, temp);

        /* reload in case playlist was saved to cwd */
        reload_directory();
    } else {
        return 1; /* cancelled out of name selection */
    }

    return 0;
}

static int playlist_view_(void)
{
    playlist_viewer_ex(NULL, NULL);
    FOR_NB_SCREENS(i) /* Playlist Viewer defers skin updates when popping its activity */
        skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_ALL);
    return 0;
}
MENUITEM_FUNCTION(create_playlist_item, 0, ID2P(LANG_CREATE_PLAYLIST),
                  create_playlist, NULL, Icon_NOICON);
MENUITEM_FUNCTION(view_cur_playlist, 0,
                  ID2P(LANG_VIEW_DYNAMIC_PLAYLIST),
                  playlist_view_, NULL, Icon_NOICON);
MENUITEM_FUNCTION_W_PARAM(save_playlist, 0, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),
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
                  clear_catalog_directory, NULL, Icon_file_view_menu);

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
MAKE_MENU(currentplaylist_settings_menu, ID2P(LANG_CURRENT_PLAYLIST),
          NULL, Icon_Playlist,
          &warn_on_erase,
          &keep_current_track_on_replace,
          &show_shuffled_adding_options,
          &show_queue_options);

MENUITEM_SETTING(sort_playlists, &global_settings.sort_playlists, NULL);
MAKE_MENU(playlist_settings, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &sort_playlists, &viewer_settings_menu, &recursive_dir_insert,
          &currentplaylist_settings_menu);
MAKE_MENU(playlist_options, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &view_cur_playlist, &save_playlist,
          &create_playlist_item, &clear_catalog_directory_item);
