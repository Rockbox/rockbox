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
#include "system.h"
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
#include "font.h"
#include "widgets.h"
#endif
#include "lang.h"
#include "sprintf.h"
#include "talk.h"
#include "misc.h"
#include "sound.h"
#if CONFIG_HWCODEC == MAS3587F
#include "peakmeter.h"
#include "mas.h"
#endif

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

    unit = sound_unit(setting);
    numdec = sound_numdecimals(setting);
    steps = sound_steps(setting);
    min = sound_min(setting);
    max = sound_max(setting);
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
            val = sound_val2phys(setting, *variable);
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
        if (changed)
            sound_set(setting, *variable);
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

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
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
    sound_set(SOUND_MDB_ENABLE, (int)value);
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
    sound_set(SOUND_SUPERBASS, (int)value);
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
    sound_set(SOUND_AVC, val);
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
#endif

#ifdef HAVE_RECORDING
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
        { "01:14" , TALK_ID(74, UNIT_MIN) },
        { "01:20" , TALK_ID(80, UNIT_MIN) },
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
                      names, 16, NULL );
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

static bool reconstartup(void)
{
    return set_bool(str(LANG_RECORD_STARTUP),
                    &global_settings.rec_startup);
}

#endif /* MAS3587F */

static void set_chanconf(int val)
{
    sound_set(SOUND_CHANNELS, val);
}

static bool chanconf(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CHANNEL_STEREO) },
        { STR(LANG_CHANNEL_MONO) },
        { STR(LANG_CHANNEL_CUSTOM) },
        { STR(LANG_CHANNEL_LEFT) },
        { STR(LANG_CHANNEL_RIGHT) },
        { STR(LANG_CHANNEL_KARAOKE) }
    };
    return set_option(str(LANG_CHANNEL), &global_settings.channel_config, INT,
                      names, 6, set_chanconf );
}

static bool stereo_width(void)
{
    return set_sound(str(LANG_STEREO_WIDTH), &global_settings.stereo_width,
                     SOUND_STEREO_WIDTH);
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
        { ID2P(LANG_STEREO_WIDTH), stereo_width },
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
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

#ifdef HAVE_RECORDING
enum trigger_menu_option
{
    TRIGGER_MODE,
    PRERECORD_TIME,
    START_THRESHOLD,
    START_DURATION,
    STOP_THRESHOLD,
    STOP_POSTREC,
    STOP_GAP,
    TRIG_OPTION_COUNT,
};

#if !defined(SIMULATOR) && CONFIG_HWCODEC == MAS3587F
static char* create_thres_str(int threshold)
{
    static char retval[6];
    if (threshold < 0) {
        if (threshold < -88) {
            snprintf (retval, sizeof retval, "%s", str(LANG_DB_INF));
        } else {
            snprintf (retval, sizeof retval, "%ddb", threshold + 1);
        }
    } else {
        snprintf (retval, sizeof retval, "%d%%", threshold);
    }
    return retval;
}

#define INF_DB (-89)
static void change_threshold(int *threshold, int change)
{
    if (global_settings.peak_meter_dbfs) {
        if (*threshold >= 0) {
            int db = (calc_db(*threshold * MAX_PEAK / 100) - 9000) / 100;
            *threshold = db;
        }
        *threshold += change;
        if (*threshold > -1) {
            *threshold = INF_DB;
        } else if (*threshold < INF_DB) {
            *threshold = -1;
        }
    } else {
        if (*threshold < 0) {
            *threshold = peak_meter_db2sample(*threshold * 100) * 100 / MAX_PEAK;
        }
        *threshold += change;
        if (*threshold > 100) {
            *threshold = 0;
        } else if (*threshold < 0) {
            *threshold = 100;
        }
    }
}

/* Variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define TRIG_CANCEL BUTTON_OFF
#define TRIG_ACCEPT BUTTON_PLAY
#define TRIG_RESET_SIM BUTTON_F2

#elif CONFIG_KEYPAD == ONDIO_PAD
#define TRIG_CANCEL BUTTON_OFF
#define TRIG_ACCEPT BUTTON_MENU
#endif

/**
 * Displays a menu for editing the trigger settings.
 */
bool rectrigger(void)
{
    int exit_request = false;
    enum trigger_menu_option selected = TRIGGER_MODE;
    bool retval = false;
    int old_x_margin, old_y_margin;

#define TRIGGER_MODE_COUNT 3
    static const unsigned char *trigger_modes[] = {
        ID2P(LANG_OFF),
        ID2P(LANG_RECORD_TRIG_NOREARM),
        ID2P(LANG_RECORD_TRIG_REARM)
    };

#define PRERECORD_TIMES_COUNT 31
    static const unsigned char *prerecord_times[] = {
        ID2P(LANG_OFF),"1s","2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s",
        "10s", "11s", "12s", "13s", "14s", "15s", "16s", "17s", "18s", "19s",
        "20s", "21s", "22s", "23s", "24s", "25s", "26s", "27s", "28s", "29s",
        "30s"
    };

    static const unsigned char *option_name[] = {
        [TRIGGER_MODE] =    ID2P(LANG_RECORD_TRIGGER_MODE),
        [PRERECORD_TIME] =  ID2P(LANG_RECORD_PRERECORD_TIME),
        [START_THRESHOLD] = ID2P(LANG_RECORD_START_THRESHOLD),
        [START_DURATION] =  ID2P(LANG_RECORD_MIN_DURATION),
        [STOP_THRESHOLD] =  ID2P(LANG_RECORD_STOP_THRESHOLD),
        [STOP_POSTREC] =    ID2P(LANG_RECORD_STOP_POSTREC),
        [STOP_GAP] =        ID2P(LANG_RECORD_STOP_GAP)
    };

    int old_start_thres = global_settings.rec_start_thres;
    int old_start_duration = global_settings.rec_start_duration;
    int old_prerecord_time = global_settings.rec_prerecord_time;
    int old_stop_thres = global_settings.rec_stop_thres;
    int old_stop_postrec = global_settings.rec_stop_postrec;
    int old_stop_gap = global_settings.rec_stop_gap;
    int old_trigger_mode = global_settings.rec_trigger_mode;

    int offset = 0;
    int option_lines;
    int w, h;

    /* restart trigger with new values */
    settings_apply_trigger();
    peak_meter_trigger (global_settings.rec_trigger_mode != TRIG_MODE_OFF);

    lcd_clear_display();

    old_x_margin = lcd_getxmargin();
    old_y_margin = lcd_getymargin();
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);

    lcd_getstringsize("M", &w, &h);

    // two lines are reserved for peak meter and trigger status
    option_lines = (LCD_HEIGHT/h) - (global_settings.statusbar ? 1:0) - 2;

    while (!exit_request) {
        int stat_height = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
        int button, i;
        const char *str;
        char option_value[TRIG_OPTION_COUNT][7];

        snprintf(
            option_value[TRIGGER_MODE],
            sizeof option_value[TRIGGER_MODE],
            "%s",
            P2STR(trigger_modes[global_settings.rec_trigger_mode]));

        snprintf (
            option_value[PRERECORD_TIME],
            sizeof option_value[PRERECORD_TIME],
            "%s",
            P2STR(prerecord_times[global_settings.rec_prerecord_time]));

        /* due to value range shift (peak_meter_define_trigger) -1 is 0db */
        if (global_settings.rec_start_thres == -1) {
            str = str(LANG_OFF);
        } else {
            str = create_thres_str(global_settings.rec_start_thres);
        }
        snprintf(
            option_value[START_THRESHOLD],
            sizeof option_value[START_THRESHOLD],
            "%s",
            str);

        snprintf(
            option_value[START_DURATION],
            sizeof option_value[START_DURATION],
            "%s",
            trig_durations[global_settings.rec_start_duration]);


        if (global_settings.rec_stop_thres <= INF_DB) {
            str = str(LANG_OFF);
        } else {
            str = create_thres_str(global_settings.rec_stop_thres);
        }
        snprintf(
            option_value[STOP_THRESHOLD],
            sizeof option_value[STOP_THRESHOLD],
            "%s",
            str);

        snprintf(
            option_value[STOP_POSTREC],
            sizeof option_value[STOP_POSTREC],
            "%s",
            trig_durations[global_settings.rec_stop_postrec]);

        snprintf(
            option_value[STOP_GAP],
            sizeof option_value[STOP_GAP],
            "%s",
            trig_durations[global_settings.rec_stop_gap]);

        lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        lcd_fillrect(0, stat_height, LCD_WIDTH, LCD_HEIGHT - stat_height);
        lcd_set_drawmode(DRMODE_SOLID);
        status_draw(true);

        /* reselect FONT_SYSFONT as status_draw has changed the font */
        /*lcd_setfont(FONT_SYSFIXED);*/

        for (i = 0; i < option_lines; i++) {
            int x, y;

            str = P2STR(option_name[i + offset]);
            lcd_putsxy(5, stat_height + i * h, str);

            str = option_value[i + offset];
            lcd_getstringsize(str, &w, &h);
            y = stat_height + i * h;
            x = LCD_WIDTH - w;
            lcd_putsxy(x, y, str);
            if ((int)selected == (i + offset)) {
                lcd_set_drawmode(DRMODE_COMPLEMENT);
                lcd_fillrect(x, y, w, h);
                lcd_set_drawmode(DRMODE_SOLID);
            }
        }

        scrollbar(0, stat_height,
            4, LCD_HEIGHT - 16 - stat_height,
            TRIG_OPTION_COUNT, offset, offset + option_lines,
            VERTICAL);

        peak_meter_draw_trig(0, LCD_HEIGHT - 8 - TRIG_HEIGHT);

        button = peak_meter_draw_get_btn(0, LCD_HEIGHT - 8, LCD_WIDTH, 8);

        lcd_update();

        switch (button) {
            case TRIG_CANCEL:
                splash(50, true, str(LANG_RESET_DONE_CANCEL));
                global_settings.rec_start_thres = old_start_thres;
                global_settings.rec_start_duration = old_start_duration;
                global_settings.rec_prerecord_time = old_prerecord_time;
                global_settings.rec_stop_thres = old_stop_thres;
                global_settings.rec_stop_postrec = old_stop_postrec;
                global_settings.rec_stop_gap = old_stop_gap;
                global_settings.rec_trigger_mode = old_trigger_mode;
                exit_request = true;
                break;

            case TRIG_ACCEPT:
                exit_request = true;
                break;

            case BUTTON_UP:
                selected += TRIG_OPTION_COUNT - 1;
                selected %= TRIG_OPTION_COUNT;
                offset = MIN(offset, (int)selected);
                offset = MAX(offset, (int)selected - option_lines + 1);
                break;

            case BUTTON_DOWN:
                selected ++;
                selected %= TRIG_OPTION_COUNT;
                offset = MIN(offset, (int)selected);
                offset = MAX(offset, (int)selected - option_lines + 1);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                switch (selected) {
                    case TRIGGER_MODE:
                        global_settings.rec_trigger_mode ++;
                        global_settings.rec_trigger_mode %= TRIGGER_MODE_COUNT;
                        break;

                    case PRERECORD_TIME:
                        global_settings.rec_prerecord_time ++;
                        global_settings.rec_prerecord_time %= PRERECORD_TIMES_COUNT;
                        break;

                    case START_THRESHOLD:
                        change_threshold(&global_settings.rec_start_thres, 1);
                        break;

                    case START_DURATION:
                        global_settings.rec_start_duration ++;
                        global_settings.rec_start_duration %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_THRESHOLD:
                        change_threshold(&global_settings.rec_stop_thres, 1);
                        break;

                    case STOP_POSTREC:
                        global_settings.rec_stop_postrec ++;
                        global_settings.rec_stop_postrec %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_GAP:
                        global_settings.rec_stop_gap ++;
                        global_settings.rec_stop_gap %= TRIG_DURATION_COUNT;
                        break;

                    case TRIG_OPTION_COUNT:
                        // avoid compiler warnings
                        break;
                }
                peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
                settings_apply_trigger();
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                switch (selected) {
                    case TRIGGER_MODE:
                        global_settings.rec_trigger_mode+=TRIGGER_MODE_COUNT-1;
                        global_settings.rec_trigger_mode %= TRIGGER_MODE_COUNT;
                        break;

                    case PRERECORD_TIME:
                        global_settings.rec_prerecord_time += PRERECORD_TIMES_COUNT - 1;
                        global_settings.rec_prerecord_time %= PRERECORD_TIMES_COUNT;
                        break;

                    case START_THRESHOLD:
                        change_threshold(&global_settings.rec_start_thres, -1);
                        break;

                    case START_DURATION:
                        global_settings.rec_start_duration += TRIG_DURATION_COUNT-1;
                        global_settings.rec_start_duration %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_THRESHOLD:
                        change_threshold(&global_settings.rec_stop_thres, -1);
                        break;

                    case STOP_POSTREC:
                        global_settings.rec_stop_postrec +=
                            TRIG_DURATION_COUNT - 1;
                        global_settings.rec_stop_postrec %=
                            TRIG_DURATION_COUNT;
                        break;

                    case STOP_GAP:
                        global_settings.rec_stop_gap +=
                            TRIG_DURATION_COUNT - 1;
                        global_settings.rec_stop_gap %= TRIG_DURATION_COUNT;
                        break;

                    case TRIG_OPTION_COUNT:
                        // avoid compiler warnings
                        break;
                }
                
                if (global_settings.rec_trigger_mode == TRIG_OFF) {
                    peak_meter_trigger(true);
                } else {
                    /* restart trigger with new values */
                    settings_apply_trigger();
                }
                break;

#ifdef TRIG_RESET_SIM
            case TRIG_RESET_SIM:
                peak_meter_trigger(true);
                break;
#endif

            case SYS_USB_CONNECTED:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    retval = true;
                    exit_request = true;
                }
                break;
        }
    }

    peak_meter_trigger(false);
    lcd_setfont(FONT_UI);
    lcd_setmargins(old_x_margin, old_y_margin);
    return retval;
}
#endif

bool recording_menu(bool no_source)
{
    int m;
    int i = 0;
    struct menu_item items[10];
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
    items[i].desc = ID2P(LANG_RECORD_STARTUP);
    items[i++].function = reconstartup;
#if !defined(SIMULATOR) && CONFIG_HWCODEC == MAS3587F
    items[i].desc = str(LANG_RECORD_TRIGGER);
    items[i++].function = rectrigger;
#endif

    m=menu_init( items, i, NULL, NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif
