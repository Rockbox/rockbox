
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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "playlist_menu.h"

#include "menu.h"
#include "file.h"
#include "keyboard.h"
#include "playlist.h"
#include "tree.h"
#include "playlist_viewer.h"
#include "talk.h"
#include "playlist_catalog.h"

int save_playlist_screen(struct playlist_info* playlist)
{
    char temp[MAX_PATH+1];
    int len;

    playlist_get_name(playlist, temp, sizeof(temp));
    len = strlen(temp);

    if (len > 4 && !strcasecmp(&temp[len-4], ".m3u"))
        strcat(temp, "8");
    
    if (len <= 5 || strcasecmp(&temp[len-5], ".m3u8"))
        strcpy(temp, DEFAULT_DYNAMIC_PLAYLIST_NAME);

    if (!kbd_input(temp, sizeof(temp)))
    {
        playlist_save(playlist, temp);

        /* reload in case playlist was saved to cwd */
        reload_directory();
    }

    return 0;
}
MENUITEM_FUNCTION(create_playlist_item, 0, ID2P(LANG_CREATE_PLAYLIST), 
                  (int(*)(void))create_playlist, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(view_playlist, 0, ID2P(LANG_VIEW_DYNAMIC_PLAYLIST), 
                  (int(*)(void))playlist_viewer, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(save_playlist, MENU_FUNC_USEPARAM, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST), 
                         (int(*)(void*))save_playlist_screen, 
                        NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(catalog, 0, ID2P(LANG_CATALOG_VIEW), 
                  (int(*)(void))catalog_view_playlists,
                   NULL, NULL, Icon_NOICON);
MENUITEM_SETTING(recursive_dir_insert, &global_settings.recursive_dir_insert, NULL);
MENUITEM_SETTING(warn_on_erase, &global_settings.warnon_erase_dynplaylist, NULL);

MAKE_MENU(playlist_settings, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &recursive_dir_insert, &warn_on_erase);
MAKE_MENU(playlist_options, ID2P(LANG_PLAYLISTS), NULL,
          Icon_Playlist,
          &create_playlist_item, &view_playlist, &save_playlist, &catalog);

