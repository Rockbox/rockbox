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
#include "talk.h"
#include "misc.h"

static const char* const fmt[] =
{
    "",                 /* no decimals */
    "%d.%d %s  ",       /* 1 decimal */
    "%d.%02d %s  "      /* 2 decimals */
};

bool set_sound(const char* string,
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
    const char* unit;
    char str[32];
    int talkunit = UNIT_INT;
    int steps;
    int button;

    unit = mpeg_sound_unit(setting);
    numdec = mpeg_sound_numdecimals(setting);
    steps = mpeg_sound_steps(setting);
    min = mpeg_sound_min(setting);
    max = mpeg_sound_max(setting);
    if (*unit == 'd') /* crude reconstruction */
        talkunit = UNIT_DB;
    else if (*unit == '%')
        talkunit = UNIT_PERCENT;
    else if (*unit == 'H')
         talkunit = UNIT_HERTZ;
    
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
            if (global_settings.talk_menu)
                talk_value(val, talkunit, false); /* speak it */
        }
        lcd_puts(0,1,str);
        status_draw(true);
        lcd_update();

        changed = false;
        button = button_get_w_tmo(HZ/2);
        switch( button ) {
            case SETTINGS_INC:
            case SETTINGS_INC | BUTTON_REPEAT:
                (*variable)+=steps;
                if(*variable > max )
                    *variable = max;
                changed = true;
                break;

            case SETTINGS_DEC:
            case SETTINGS_DEC | BUTTON_REPEAT:
                (*variable)-=steps;
                if(*variable < min )
                    *variable = min;
                changed = true;
                break;

            case SETTINGS_OK:
            case SETTINGS_CANCEL:
#ifdef SETTINGS_OK2
            case SETTINGS_OK2:
#endif
#ifdef SETTINGS_CANCEL2
            case SETTINGS_CANCEL2:
#endif
                done = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
        if (changed) {
            mpeg_sound_set(setting, *variable);
#if CONFIG_HWCODEC == MAS3507D
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
}

static bool treble(void)
{
    return set_sound(str(LANG_TREBLE), &global_settings.treble, SOUND_TREBLE);
}

#if CONFIG_HWCODEC == MAS3587F
static bool loudness(void)
{
    return set_sound(str(LANG_LOUDNESS), &global_settings.loudness, 
                     SOUND_LOUDNESS);
}

static bool mdb_strength(void)
{
    return set_sound(str(LANG_MDB_STRENGTH), &global_settings.mdb_strength, 
                     SOUND_MDB_STRENGTH);
}

static bool mdb_harmonics(void)
{
    return set_sound(str(LANG_MDB_HARMONICS), &global_settings.mdb_harmonics, 
                     SOUND_MDB_HARMONICS);
}

static bool mdb_center(void)
{
    return set_sound(str(LANG_MDB_CENTER), &global_settings.mdb_center, 
                     SOUND_MDB_CENTER);
}

static bool mdb_shape(void)
{
    return set_sound(str(LANG_MDB_SHAPE), &global_settings.mdb_shape,
                     SOUND_MDB_SHAPE);
}

static void set_mdb_enable(bool value)
{
    mpeg_sound_set(SOUND_MDB_ENABLE, (int)value);
}

static bool mdb_enable(void)
{
    return set_bool_options(str(LANG_MDB_ENABLE),
                            &global_settings.mdb_enable, 
                            STR(LANG_SET_BOOL_YES), 
                            STR(LANG_SET_BOOL_NO), 
                            set_mdb_enable);
}

static void set_superbass(bool value)
{
    mpeg_sound_set(SOUND_SUPERBASS, (int)value);
}

static bool superbass(void)
{
    return set_bool_options(str(LANG_SUPERBASS),
                            &global_settings.superbass, 
                            STR(LANG_SET_BOOL_YES), 
                            STR(LANG_SET_BOOL_NO), 
                            set_superbass);
}

static void set_avc(int val)
{
    mpeg_sound_set(SOUND_AVC, val);
}

static bool avc(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "20ms", TALK_ID(20, UNIT_MS) },
        { "2s", TALK_ID(2, UNIT_SEC) },
        { "4s", TALK_ID(4, UNIT_SEC) },
        { "8s", TALK_ID(8, UNIT_SEC) }
    };
    return set_option(str(LANG_DECAY), &global_settings.avc, INT,
                      names, 5, set_avc);
}

static bool recsource(void)
{
    static const struct opt_items names[] = {
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
    static const struct opt_items names[] = {
        { "44.1kHz", TALK_ID(44, UNIT_KHZ) },
        { "48kHz", TALK_ID(48, UNIT_KHZ) },
        { "32kHz", TALK_ID(32, UNIT_KHZ) },
        { "22.05kHz", TALK_ID(22, UNIT_KHZ) },
        { "24kHz", TALK_ID(24, UNIT_KHZ) },
        { "16kHz", TALK_ID(16, UNIT_KHZ) }
    };
    return set_option(str(LANG_RECORDING_FREQUENCY),
                      &global_settings.rec_frequency, INT,
                      names, 6, NULL );
}

static bool recchannels(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CHANNEL_STEREO) },
        { STR(LANG_CHANNEL_MONO) }
    };
    return set_option(str(LANG_RECORDING_CHANNELS),
                      &global_settings.rec_channels, INT,
                      names, 2, NULL );
}

static bool recquality(void)
{
    return set_int(str(LANG_RECORDING_QUALITY), "", UNIT_INT,
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
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "00:05" , TALK_ID(5, UNIT_MIN) },
        { "00:10" , TALK_ID(10, UNIT_MIN) },
        { "00:15" , TALK_ID(15, UNIT_MIN) },
        { "00:30" , TALK_ID(30, UNIT_MIN) },
        { "01:00" , TALK_ID(1, UNIT_HOUR) },
        { "02:00" , TALK_ID(2, UNIT_HOUR) },
        { "04:00" , TALK_ID(4, UNIT_HOUR) },
        { "06:00" , TALK_ID(6, UNIT_HOUR) },
        { "08:00" , TALK_ID(8, UNIT_HOUR) },
        { "10:00" , TALK_ID(10, UNIT_HOUR) },
        { "12:00" , TALK_ID(12, UNIT_HOUR) },
        { "18:00" , TALK_ID(18, UNIT_HOUR) },
        { "24:00" , TALK_ID(24, UNIT_HOUR) }
    };
    return set_option(str(LANG_RECORD_TIMESPLIT),
                      &global_settings.rec_timesplit, INT,
                      names, 14, NULL );
}

static bool recprerecord(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "1s", TALK_ID(1, UNIT_SEC) },
        { "2s", TALK_ID(2, UNIT_SEC) },
        { "3s", TALK_ID(3, UNIT_SEC) },
        { "4s", TALK_ID(4, UNIT_SEC) },
        { "5s", TALK_ID(5, UNIT_SEC) },
        { "6s", TALK_ID(6, UNIT_SEC) },
        { "7s", TALK_ID(7, UNIT_SEC) },
        { "8s", TALK_ID(8, UNIT_SEC) },
        { "9s", TALK_ID(9, UNIT_SEC) },
        { "10s", TALK_ID(10, UNIT_SEC) },
        { "11s", TALK_ID(11, UNIT_SEC) },
        { "12s", TALK_ID(12, UNIT_SEC) },
        { "13s", TALK_ID(13, UNIT_SEC) },
        { "14s", TALK_ID(14, UNIT_SEC) },
        { "15s", TALK_ID(15, UNIT_SEC) },
        { "16s", TALK_ID(16, UNIT_SEC) },
        { "17s", TALK_ID(17, UNIT_SEC) },
        { "18s", TALK_ID(18, UNIT_SEC) },
        { "19s", TALK_ID(19, UNIT_SEC) },
        { "20s", TALK_ID(20, UNIT_SEC) },
        { "21s", TALK_ID(21, UNIT_SEC) },
        { "22s", TALK_ID(22, UNIT_SEC) },
        { "23s", TALK_ID(23, UNIT_SEC) },
        { "24s", TALK_ID(24, UNIT_SEC) },
        { "25s", TALK_ID(25, UNIT_SEC) },
        { "26s", TALK_ID(26, UNIT_SEC) },
        { "27s", TALK_ID(27, UNIT_SEC) },
        { "28s", TALK_ID(28, UNIT_SEC) },
        { "29s", TALK_ID(29, UNIT_SEC) },
        { "30s", TALK_ID(30, UNIT_SEC) }
    };
    return set_option(str(LANG_RECORD_PRERECORD_TIME),
                      &global_settings.rec_prerecord_time, INT,
                      names, 31, NULL );
}

static bool recdirectory(void)
{
    static const struct opt_items names[] = {
        { rec_base_directory, -1 },
        { STR(LANG_RECORD_CURRENT_DIR) }
    };
    return set_option(str(LANG_RECORD_DIRECTORY),
                      &global_settings.rec_directory, INT,
                      names, 2, NULL );
}

#endif /* MAS3587F */

static void set_chanconf(int val)
{
    mpeg_sound_set(SOUND_CHANNELS, val);
}

static bool chanconf(void)
{
    static const struct opt_items names[] = {
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
    static const struct menu_item items[] = {
        { ID2P(LANG_VOLUME), volume },
        { ID2P(LANG_BASS), bass },
        { ID2P(LANG_TREBLE), treble },
        { ID2P(LANG_BALANCE), balance },
        { ID2P(LANG_CHANNEL_MENU), chanconf },
#if CONFIG_HWCODEC == MAS3587F
        { ID2P(LANG_LOUDNESS), loudness },
        { ID2P(LANG_AUTOVOL), avc },
        { ID2P(LANG_SUPERBASS), superbass },
        { ID2P(LANG_MDB_ENABLE), mdb_enable },
        { ID2P(LANG_MDB_STRENGTH), mdb_strength },
        { ID2P(LANG_MDB_HARMONICS), mdb_harmonics },
        { ID2P(LANG_MDB_CENTER), mdb_center },
        { ID2P(LANG_MDB_SHAPE), mdb_shape },
#endif
    };
    
    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

#if CONFIG_HWCODEC == MAS3587F
bool recording_menu(bool no_source)
{
    int m;
    int i = 0;
    struct menu_item items[8];
    bool result;

    items[i].desc = ID2P(LANG_RECORDING_QUALITY);
    items[i++].function = recquality;
    items[i].desc = ID2P(LANG_RECORDING_FREQUENCY);
    items[i++].function = recfrequency;
    if(!no_source) {
        items[i].desc = ID2P(LANG_RECORDING_SOURCE);
        items[i++].function = recsource;
    }
    items[i].desc = ID2P(LANG_RECORDING_CHANNELS);
    items[i++].function = recchannels;
    items[i].desc = ID2P(LANG_RECORDING_EDITABLE);
    items[i++].function = receditable;
    items[i].desc = ID2P(LANG_RECORD_TIMESPLIT);
    items[i++].function = rectimesplit;
    items[i].desc = ID2P(LANG_RECORD_PRERECORD_TIME);
    items[i++].function = recprerecord;
    items[i].desc = ID2P(LANG_RECORD_DIRECTORY);
    items[i++].function = recdirectory;

    m=menu_init( items, i, NULL, NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif
