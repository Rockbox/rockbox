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
#include "mp3_playback.h"
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
            case BUTTON_PLAY:
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
    struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "2s", -1 },
        { "4s", -1 },
        { "8s", -1 }
    };
    return set_option(str(LANG_DECAY), &global_settings.avc, INT,
                      names, 4, set_avc);
}

static bool recsource(void)
{
    struct opt_items names[] = {
        { STR(LANG_RECORDING_SRC_MIC) },
        { STR(LANG_RECORDING_SRC_LINE) },
        { STR(LANG_RECORDING_SRC_DIGITAL) }
    };
    return set_option(str(LANG_RECORDING_SOURCE),
                      &global_settings.rec_source, INT,
                      names, 3, NULL );
}

static bool recfrequency(void)
{
    struct opt_items names[] = {
        { "44.1kHz", -1 },
        { "48kHz", -1 },
        { "32kHz", -1 },
        { "22.05kHz", -1 },
        { "24kHz", -1 },
        { "16kHz", -1 }
    };
    return set_option(str(LANG_RECORDING_FREQUENCY),
                      &global_settings.rec_frequency, INT,
                      names, 6, NULL );
}

static bool recchannels(void)
{
    struct opt_items names[] = {
        { STR(LANG_CHANNEL_STEREO) },
        { STR(LANG_CHANNEL_MONO) }
    };
    return set_option(str(LANG_RECORDING_CHANNELS),
                      &global_settings.rec_channels, INT,
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
    struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "00:05" , -1 },
        { "00:10" , -1 },
        { "00:15" , -1 },
        { "00:30" , -1 },
        { "01:00" , -1 },
        { "02:00" , -1 },
        { "04:00" , -1 },
        { "06:00" , -1 },
        { "08:00" , -1 },
        { "10:00" , -1 },
        { "12:00" , -1 },
        { "18:00" , -1 },
        { "24:00" , -1 }
    };
    return set_option(str(LANG_RECORD_TIMESPLIT),
                      &global_settings.rec_timesplit, INT,
                      names, 14, NULL );
}

static bool recprerecord(void)
{
    struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "1s", -1 },
        { "2s", -1 },
        { "3s", -1 },
        { "4s", -1 },
        { "5s", -1 },
        { "6s", -1 },
        { "7s", -1 },
        { "8s", -1 },
        { "9s", -1 },
        { "10s", -1 },
        { "11s", -1 },
        { "12s", -1 },
        { "13s", -1 },
        { "14s", -1 },
        { "15s", -1 },
        { "16s", -1 },
        { "17s", -1 },
        { "18s", -1 },
        { "19s", -1 },
        { "10s", -1 },
        { "21s", -1 },
        { "22s", -1 },
        { "23s", -1 },
        { "24s", -1 },
        { "25s", -1 },
        { "26s", -1 },
        { "27s", -1 },
        { "28s", -1 },
        { "29s", -1 },
        { "30s", -1 }
    };
    return set_option(str(LANG_RECORD_PRERECORD_TIME),
                      &global_settings.rec_prerecord_time, INT,
                      names, 31, NULL );
}

static bool recdirectory(void)
{
    struct opt_items names[] = {
        { rec_base_directory, -1 },
        { STR(LANG_RECORD_CURRENT_DIR) }
    };
    return set_option(str(LANG_RECORD_DIRECTORY),
                      &global_settings.rec_directory, INT,
                      names, 2, NULL );
}

#endif /* HAVE_MAS3587F */

static void set_chanconf(int val)
{
    mpeg_sound_set(SOUND_CHANNELS, val);
}

static bool chanconf(void)
{
    struct opt_items names[] = {
        { STR(LANG_CHANNEL_STEREO) },
#ifdef HAVE_LCD_CHARCELLS
        { STR(LANG_CHANNEL_STEREO_NARROW_PLAYER) },
#else
        { STR(LANG_CHANNEL_STEREO_NARROW_RECORDER) },
#endif
        { STR(LANG_CHANNEL_MONO) },
        { STR(LANG_CHANNEL_LEFT) },
        { STR(LANG_CHANNEL_RIGHT) },
        { STR(LANG_CHANNEL_KARAOKE) }, 
        { STR(LANG_CHANNEL_STEREO_WIDE) }
    };
    return set_option(str(LANG_CHANNEL), &global_settings.channel_config, INT,
                      names, 7, set_chanconf );
}

bool sound_menu(void)
{
    int m;
    bool result;
    struct menu_items items[] = {
        { STR(LANG_VOLUME), volume },
        { STR(LANG_BASS), bass },
        { STR(LANG_TREBLE), treble },
        { STR(LANG_BALANCE), balance },
        { STR(LANG_CHANNEL_MENU), chanconf },
#ifdef HAVE_MAS3587F
        { STR(LANG_LOUDNESS), loudness },
        { STR(LANG_BBOOST), bass_boost },
        { STR(LANG_AUTOVOL), avc }
#endif
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items), NULL );
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#ifdef HAVE_MAS3587F
bool recording_menu(bool no_source)
{
    int m;
    int i = 0;
    struct menu_items menu[8];
    bool result;

    menu[i].desc = str(LANG_RECORDING_QUALITY);
    menu[i].voice_id = LANG_RECORDING_QUALITY;
    menu[i++].function = recquality;
    menu[i].desc = str(LANG_RECORDING_FREQUENCY);
    menu[i].voice_id = LANG_RECORDING_FREQUENCY;
    menu[i++].function = recfrequency;
    if(!no_source) {
        menu[i].desc = str(LANG_RECORDING_SOURCE);
        menu[i].voice_id = LANG_RECORDING_SOURCE;
        menu[i++].function = recsource;
    }
    menu[i].desc = str(LANG_RECORDING_CHANNELS);
    menu[i].voice_id = LANG_RECORDING_CHANNELS;
    menu[i++].function = recchannels;
    menu[i].desc = str(LANG_RECORDING_EDITABLE);
    menu[i].voice_id = LANG_RECORDING_EDITABLE;
    menu[i++].function = receditable;
    menu[i].desc = str(LANG_RECORD_TIMESPLIT);
    menu[i].voice_id = LANG_RECORD_TIMESPLIT;
    menu[i++].function = rectimesplit;
    menu[i].desc = str(LANG_RECORD_PRERECORD_TIME);
    menu[i].voice_id = LANG_RECORD_PRERECORD_TIME;
    menu[i++].function = recprerecord;
    menu[i].desc = str(LANG_RECORD_DIRECTORY);
    menu[i].voice_id = LANG_RECORD_DIRECTORY;
    menu[i++].function = recdirectory;
        
    m=menu_init( menu, i, NULL );
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif
