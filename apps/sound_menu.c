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
#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include "lcd.h"
#include "menu.h"
#include "sound_menu.h"
#include "mpeg.h"
#include "button.h"
#include "kernel.h"
#include "sprintf.h"

typedef void (*settingfunc)(int);
enum { Volume, Bass, Treble, numsettings };

static void soundsetting(int setting)
{
    static int savedsettings[numsettings] = { DEFAULT_VOLUME_SETTING,
                                              DEFAULT_BASS_SETTING,
                                              DEFAULT_TREBLE_SETTING};
    static const char* names[] = { "Volume", "Bass", "Treble" };
    static settingfunc funcs[] = { mpeg_volume, mpeg_bass, mpeg_treble };

    int value = savedsettings[setting];
    char buf[32];
    bool done = false;

    lcd_clear_display();
    snprintf(buf,sizeof buf,"[%s]",names[setting]);
    lcd_puts(0,0,buf);

    while ( !done ) {
        snprintf(buf,sizeof buf,"%d %% ",value);
        lcd_puts(0,1,buf);
        lcd_update();

        switch ( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_RIGHT:
#endif
                value += 10;
                if ( value >= 100 )
                    value = 100;
                (funcs[setting])(value);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_LEFT:
#endif
                value -= 10;
                if ( value <= 0 )
                    value = 0;
                (funcs[setting])(value);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                savedsettings[setting] = value;
                done = true;
                break;
        }
    }
}

static void volume(void)
{
    soundsetting(Volume);
}

static void bass(void)
{
    soundsetting(Bass);
};

static void treble(void)
{
    soundsetting(Treble);
}

void sound_menu(void)
{
    int m;
    struct menu_items items[] = {
        { Volume, "Volume", volume },
        { Bass,   "Bass",   bass },
        { Treble, "Treble", treble }
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}

