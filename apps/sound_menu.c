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
#include "kernel.h"
#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "mpeg.h"
#include "settings.h"
#include "status.h"
#include "screens.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif
#include "lang.h"
#include "sprintf.h"

static char *fmt[] =
{
    "",                 /* no decimals */
    "%d.%d %s  ",       /* 1 decimal */
    "%d.%02d %s  "      /* 2 decimals */
};

bool set_sound(char* string, 
               int* variable,
               int setting)
{
    bool done = false;
    bool changed = true;
    int min, max;
    int val;
    int numdec;
    int integer;
    int dec;
    char* unit;
    char str[32];

    unit = mpeg_sound_unit(setting);
    numdec = mpeg_sound_numdecimals(setting);
    min = mpeg_sound_min(setting);
    max = mpeg_sound_max(setting);
    
#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts_scroll(0,0,string);

    while (!done) {
        if (changed) {
            val = mpeg_val2phys(setting, *variable);
            if(numdec)
            {
                integer = val / (10 * numdec);
                dec = val % (10 * numdec);
                snprintf(str,sizeof str, fmt[numdec], integer, dec, unit);
            }
            else
            {
                snprintf(str,sizeof str,"%d %s  ", val, unit);
            }
        }
        lcd_puts(0,1,str);
        status_draw(true);
        lcd_update();

        changed = false;
        switch( button_get_w_tmo(HZ/2) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                (*variable)++;
                if(*variable > max )
                    *variable = max;
                changed = true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#endif
                (*variable)--;
                if(*variable < min )
                    *variable = min;
                changed = true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                return true;
        }
        if (changed) {
            mpeg_sound_set(setting, *variable);
#ifdef HAVE_MAS3507D
            if(setting == SOUND_BALANCE)
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
#endif
        }
    }
    lcd_stop_scroll();
    return false;
}

static bool volume(void)
{
    return set_sound(str(LANG_VOLUME), &global_settings.volume, SOUND_VOLUME);
}

static bool balance(void)
{
    return set_sound(str(LANG_BALANCE), &global_settings.balance,
                     SOUND_BALANCE);
}

static bool bass(void)
{
    return set_sound(str(LANG_BASS), &global_settings.bass, SOUND_BASS);
};

static bool treble(void)
{
    return set_sound(str(LANG_TREBLE), &global_settings.treble, SOUND_TREBLE);
}

#ifdef HAVE_MAS3587F
static bool loudness(void)
{
    return set_sound(str(LANG_LOUDNESS), &global_settings.loudness, 
                     SOUND_LOUDNESS);
};

static bool bass_boost(void)
{
    return set_sound(str(LANG_BBOOST), &global_settings.bass_boost, 
                     SOUND_SUPERBASS);
};

static void set_avc(int val)
{
    mpeg_sound_set(SOUND_AVC, val);
}

static bool avc(void)
{
    char* names[] = { str(LANG_OFF), "2s", "4s", "8s" };
    return set_option(str(LANG_DECAY), &global_settings.avc, 
                      names, 4, set_avc);
}

static bool recsource(void)
{
    char *names[] = {str(LANG_RECORDING_SRC_MIC), str(LANG_RECORDING_SRC_LINE),
                     str(LANG_RECORDING_SRC_DIGITAL) };
    return set_option(str(LANG_RECORDING_SOURCE),
                      &global_settings.rec_source,
                      names, 3, NULL );
}

static bool recfrequency(void)
{
    char *names[] = {"44.1kHz", "48kHz", "32kHz",
                     "22.05kHz", "24kHz", "16kHz"};

    return set_option(str(LANG_RECORDING_FREQUENCY),
                      &global_settings.rec_frequency,
                      names, 6, NULL );
}

static bool recchannels(void)
{
    char *names[] = {str(LANG_CHANNEL_STEREO), str(LANG_CHANNEL_MONO)};

    return set_option(str(LANG_RECORDING_CHANNELS),
                      &global_settings.rec_channels,
                      names, 2, NULL );
}

static bool recquality(void)
{
    return set_int(str(LANG_RECORDING_QUALITY), "",
                   &global_settings.rec_quality, 
                   NULL, 1, 0, 7 );
}

static bool receditable(void)
{
    return set_bool(str(LANG_RECORDING_EDITABLE),
                    &global_settings.rec_editable);
}

static bool rectimesplit(void)
{
    char *names[] = {str(LANG_OFF), "00:05","00:10","00:15",
                     "00:30","01:00","02:00","04:00"};

    return set_option(str(LANG_RECORD_TIMESPLIT),
                      &global_settings.rec_timesplit,
                      names, 8, NULL );
}

#endif /* HAVE_MAS3587F */

static void set_chanconf(int val)
{
    mpeg_sound_set(SOUND_CHANNELS, val);
}

static bool chanconf(void)
{
    char *names[] = {str(LANG_CHANNEL_STEREO),
#ifdef HAVE_LCD_CHARCELLS
                     str(LANG_CHANNEL_STEREO_NARROW_PLAYER),
#else
                     str(LANG_CHANNEL_STEREO_NARROW_RECORDER),
#endif
                     str(LANG_CHANNEL_MONO),
                     str(LANG_CHANNEL_LEFT), str(LANG_CHANNEL_RIGHT),
                     str(LANG_CHANNEL_KARAOKE), str(LANG_CHANNEL_STEREO_WIDE) };
    return set_option(str(LANG_CHANNEL), &global_settings.channel_config,
                      names, 7, set_chanconf );
}

bool sound_menu(void)
{
    int m;
    bool result;
    struct menu_items items[] = {
        { str(LANG_VOLUME), volume },
        { str(LANG_BASS), bass },
        { str(LANG_TREBLE), treble },
        { str(LANG_BALANCE), balance },
        { str(LANG_CHANNEL_MENU), chanconf },
#ifdef HAVE_MAS3587F
        { str(LANG_LOUDNESS), loudness },
        { str(LANG_BBOOST), bass_boost },
        { str(LANG_AUTOVOL), avc }
#endif
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#ifdef HAVE_MAS3587F
bool recording_menu(void)
{
    int m;
    bool result;
    struct menu_items items[] = {
        { str(LANG_RECORDING_QUALITY), recquality },
        { str(LANG_RECORDING_FREQUENCY), recfrequency },
        { str(LANG_RECORDING_SOURCE), recsource },
        { str(LANG_RECORDING_CHANNELS), recchannels },
        { str(LANG_RECORDING_EDITABLE), receditable },
        { str(LANG_RECORD_TIMESPLIT), rectimesplit },
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif
