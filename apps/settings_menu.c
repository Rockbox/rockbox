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

#include <stdio.h>
#include <stdbool.h>
#include "lcd.h"
#include "menu.h"
#include "mpeg.h"
#include "button.h"
#include "kernel.h"
#include "sprintf.h"

#include "settings.h"
#include "settings_menu.h"
#include "backlight.h"
#include "playlist.h"  /* for playlist_shuffle */

static void shuffle(void)
{
    set_bool( "[Shuffle]", &global_settings.playlist_shuffle );
}

static void mp3_filter(void)
{
    set_bool( "[MP3/M3U filter]", &global_settings.mp3filter );
}

static void backlight_timer(void)
{
    set_int( "[Backlight]", "s", &global_settings.backlight, 
             backlight_time, 1, 0, 60 );
    backlight_on();
}

static void scroll_speed(void)
{
    set_int("Scroll speed indicator...", "", &global_settings.scroll_speed, 
            &lcd_scroll_speed, 1, 1, 20 );
}

static void wps_set(void)
{
    char* names[] = { "Id3  ", "File ", "Parse" };
    set_option("[WPS display]", &global_settings.wps_display, names, 3 );
}

void settings_menu(void)
{
    int m;
    struct menu_items items[] = {
        { "Shuffle",         shuffle         }, 
        { "MP3/M3U filter",  mp3_filter      },
        { "Backlight Timer", backlight_timer },
        { "Scroll speed",    scroll_speed    },  
        { "While Playing",   wps_set },
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
    settings_save();
}
