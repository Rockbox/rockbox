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

enum { Shuffle, Backlight, Scroll, Wps, numsettings };

static void shuffle(void)
{
    bool done = false;

    lcd_clear_display();
    lcd_puts(0,0,"[Shuffle]");

    while ( !done ) {
        lcd_puts(0, 1, playlist_shuffle ? "on " : "off");
        lcd_update();

        switch ( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
#endif
                done = true;
                break;

            default:
                playlist_shuffle = !playlist_shuffle;
                break;
        }
    }
}

static void backlight_timer(void)
{
    bool done = false;
    int timer = global_settings.backlight;
    char str[16];

    lcd_clear_display();
    lcd_puts_scroll(0,0,"Backlight");

    while (!done) {
        snprintf(str,sizeof str,"Timeout: %d s  ", timer);
        lcd_puts(0,1,str);
        lcd_update();
        switch( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_RIGHT:
#endif
                timer++;
                if(timer > 60)
                    timer = 60;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_LEFT:
#endif
                timer--;
                if ( timer < 0 )
                    timer = 0;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                global_settings.backlight = timer;
                backlight_on();
                break;
        }
    }
}

static void scroll_speed(void)
{
    bool done=false;
    int speed=10;
    char str[16];

    lcd_clear_display();
    lcd_puts_scroll(0,0,"Scroll speed indicator");

    while (!done) {
        snprintf(str,sizeof str,"Speed: %d ",speed);
        lcd_puts(0,1,str);
        lcd_update();
        lcd_scroll_speed(speed);
        switch( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_RIGHT:
#endif
                speed++;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_LEFT:
#endif
                speed--;
                if ( speed < 1 )
                    speed = 1;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                lcd_stop_scroll();
                break;
        }
    }
}


void wps_set(void)
{
   /* Simple menu for selecting what the display shows during playback */

    bool done = false;
    int itemp = 0;
    char* names[] = { "Id3  ", "File ", "Parse" };

    lcd_clear_display();
    lcd_puts(0,0,"[Display]");

    while (!done) {
        lcd_puts(0,1,names[itemp]);
        lcd_update();

        switch ( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_LEFT:
#endif
                itemp--;
                if (itemp <= 0)
                   itemp = 0;
                break;
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_RIGHT:
#endif
                itemp++;
                if (itemp >= 2)
                   itemp = 2;
                break;
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif            
                done = true;
                break;
            default:
                itemp = 0;
                break;
        }
    }


    global_settings.wps_display = itemp; //savedsettings[itemp];
}

void settings_menu(void)
{
    int m;
    struct menu_items items[] = {
        { Shuffle,   "Shuffle",         shuffle         }, 
        { Backlight, "Backlight Timer", backlight_timer },
        { Scroll,    "Scroll speed",    scroll_speed    },  
        { Wps,       "While Playing",   wps_set },
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}
