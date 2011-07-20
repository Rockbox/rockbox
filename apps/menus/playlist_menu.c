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

int save_playlist_screen(struct playlist_info* playlist)
{
    char temp[MAX_PATH+1], *dot;
    int len;
    
    playlist_get_name(playlist, temp, sizeof(temp)-1);
    len = strlen(temp);

    dot = strrchr(temp, '.');
    if (!dot)
    {
        /* folder of some type */
        if (temp[1] != '\0')
            strcpy(&temp[len-1], ".m3u8");
        else
            snprintf(temp, sizeof(temp), "%s%s",
                    catalog_get_directory(), DEFAULT_DYNAMIC_PLAYLIST_NAME);
    }
    else if (len > 4 && !strcasecmp(dot, ".m3u"))
        strcat(temp, "8");

    if (!kbd_input(temp, sizeof(temp)))
    {
        playlist_save(playlist, temp);

        /* reload in case playlist was saved to cwd */
        reload_directory();
    }

    return 0;
}

static int playlist_view_(void)
{
    playlist_viewer_ex(NULL);
    return 0;
}

MENUITEM_FUNCTION(create_playlist_item, 0, ID2P(LANG_CREATE_PLAYLIST), 
                  (int(*)(void))create_playlist, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(view_cur_playlist, 0,
                  ID2P(LANG_VIEW_DYNAMIC_PLAYLIST), 
                  (int(*)(void))playlist_view_, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(save_playlist, MENU_FUNC_USEPARAM, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST), 
                         (int(*)(void*))save_playlist_screen, 
                        NULL, NULL, Icon_NOICON);
MENUITEM_SETTING(recursive_dir_insert, &global_settings.recursive_dir_insert, NULL);
MENUITEM_SETTING(warn_on_erase, &global_settings.warnon_erase_dynplaylist, NULL);
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


MAKE_MENU(playlist_settings, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &viewer_settings_menu, &recursive_dir_insert, &warn_on_erase);
MAKE_MENU(playlist_options, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &create_playlist_item, &view_cur_playlist,
          &save_playlist, &clear_catalog_directory_item);

