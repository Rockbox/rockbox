/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "options.h"

#ifdef HAVE_LCD_BITMAP
#ifdef USE_GAMES

#include <stdio.h>
#include <stdbool.h>
#include "menu.h"
#include "games_menu.h"
#include "lang.h"
#include "plugin.h"

static bool tetris(void)
{
    if (plugin_load("/.rockbox/rocks/tetris.rock",NULL)==PLUGIN_USB_CONNECTED)
        return true;
    return false;
}

static bool sokoban(void)
{
    if (plugin_load("/.rockbox/rocks/sokoban.rock",NULL)==PLUGIN_USB_CONNECTED)
        return true;
    return false;
}

static bool wormlet(void)
{
    if (plugin_load("/.rockbox/rocks/wormlet.rock",NULL)==PLUGIN_USB_CONNECTED)
        return true;
    return false;
}

bool games_menu(void)
{
    int m;
    bool result;

    struct menu_items items[] = {
        { str(LANG_TETRIS),  tetris  },
        { str(LANG_SOKOBAN), sokoban },
        { str(LANG_WORMLET), wormlet },
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#endif
#endif
