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
#include "lcd.h"
#include "menu.h"
#include "games_menu.h"
#include "button.h"
#include "kernel.h"
#include "sprintf.h"

#include "sokoban.h"
#include "wormlet.h"
extern Menu tetris(void);
extern Menu snake(void);

Menu games_menu(void)
{
    int m;
    Menu result;

    struct menu_items items[] = {
        { "Tetris",  tetris  },
        { "Sokoban", sokoban },
	{ "Wormlet", wormlet },
        { "Snake",   snake   } 
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#endif
#endif
