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

static bool flipit(void) 
{
    if (plugin_load("/.rockbox/rocks/flipit.rock",NULL)==PLUGIN_USB_CONNECTED)
        return true;
    return false;
}

static bool othelo(void) 
{
    if (plugin_load("/.rockbox/rocks/othelo.rock",NULL)==PLUGIN_USB_CONNECTED)
        return true;
    return false;
}

static bool sliding_puzzle(void) 
{
    if (plugin_load("/.rockbox/rocks/sliding_puzzle.rock",
                    NULL)==PLUGIN_USB_CONNECTED)
        return true;
    return false;
}

static bool star(void) 
{
    if (plugin_load("/.rockbox/rocks/star.rock",NULL)==PLUGIN_USB_CONNECTED)
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
        { str(LANG_FLIPIT),  flipit  },
        { str(LANG_OTHELO), othelo },
        { str(LANG_SLIDING_PUZZLE), sliding_puzzle },
        { str(LANG_STAR), star },
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#endif
#endif
