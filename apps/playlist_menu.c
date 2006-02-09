/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>

#include "menu.h"
#include "file.h"
#include "keyboard.h"
#include "playlist.h"
#include "tree.h"
#include "settings.h"
#include "playlist_viewer.h"
#include "talk.h"
#include "lang.h"
#include "playlist_menu.h"

static bool save_playlist(void)
{
    save_playlist_screen(NULL);
    return false;
}

static bool recurse_directory(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_ON) },
        { STR(LANG_RESUME_SETTING_ASK)},
    };

    return set_option( (char *)str(LANG_RECURSE_DIRECTORY),
                       &global_settings.recursive_dir_insert, INT, names, 3,
                       NULL );
}

static bool warnon_option(void)
{
    return set_bool(str(LANG_WARN_ERASEDYNPLAYLIST_MENU),
        &global_settings.warnon_erase_dynplaylist);
}

bool playlist_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_CREATE_PLAYLIST),       create_playlist   },
        { ID2P(LANG_VIEW_DYNAMIC_PLAYLIST), playlist_viewer   },
        { ID2P(LANG_SAVE_DYNAMIC_PLAYLIST), save_playlist     },
        { ID2P(LANG_RECURSE_DIRECTORY),     recurse_directory },
        { ID2P(LANG_WARN_ERASEDYNPLAYLIST_MENU), warnon_option},
    };

    m = menu_init( items, sizeof items / sizeof(struct menu_item), NULL,
                   NULL, NULL, NULL );
    result = menu_run(m);
    menu_exit(m);
    return result;
}

int save_playlist_screen(struct playlist_info* playlist)
{
    char* filename;
    char temp[MAX_PATH+1];
    int len;

    filename = playlist_get_name(playlist, temp, sizeof(temp));

    if (!filename || (len=strlen(filename)) <= 4 ||
        strcasecmp(&filename[len-4], ".m3u"))
        filename = DEFAULT_DYNAMIC_PLAYLIST_NAME;

    if (!kbd_input(filename, sizeof(filename)))
    {
        playlist_save(playlist, filename);

        /* reload in case playlist was saved to cwd */
        reload_directory();
    }

    return 0;
}
