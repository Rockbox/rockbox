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
#include <string.h>

#include "lcd.h"
#include "menu.h"
#include "mpeg.h"
#include "audio.h"
#include "button.h"
#include "kernel.h"
#include "thread.h"
#include "sprintf.h"
#include "settings.h"
#include "settings_menu.h"
#include "backlight.h"
#include "playlist.h"           /* for playlist_shuffle */
#include "fat.h"                /* For dotfile settings */
#include "sleeptimer.h"
#include "powermgmt.h"
#include "rtc.h"
#include "ata.h"
#include "tree.h"
#include "screens.h"
#include "talk.h"
#include "timefuncs.h"
#include "misc.h"
#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
#endif
#include "lang.h"
#if CONFIG_HWCODEC == MAS3507D
void dac_line_in(bool enable);
#endif
#ifdef HAVE_ALARM_MOD
#include "alarm_menu.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#ifdef HAVE_CHARGING
static bool car_adapter_mode(void)
{
    return set_bool_options( str(LANG_CAR_ADAPTER_MODE),
                             &global_settings.car_adapter_mode,
                             STR(LANG_SET_BOOL_YES),
                             STR(LANG_SET_BOOL_NO),
                             NULL);
}
#endif

static bool contrast(void)
{
    return set_int( str(LANG_CONTRAST), "", UNIT_INT,
                    &global_settings.contrast, 
                    lcd_set_contrast, 1, MIN_CONTRAST_SETTING,
                    MAX_CONTRAST_SETTING );
}

#ifdef HAVE_REMOTE_LCD
static bool remote_contrast(void)
{
    return set_int( str(LANG_CONTRAST), "", UNIT_INT,
                    &global_settings.remote_contrast, 
                    lcd_remote_set_contrast, 1, MIN_CONTRAST_SETTING,
                    MAX_CONTRAST_SETTING );
}

static bool remote_invert(void)
{
     bool rc = set_bool_options(str(LANG_INVERT),
                                &global_settings.remote_invert,
                                STR(LANG_INVERT_LCD_INVERSE),
                                STR(LANG_INVERT_LCD_NORMAL),
                                lcd_remote_set_invert_display);
     return rc;
}
#endif

#ifdef CONFIG_BACKLIGHT
static bool caption_backlight(void)
{
    bool rc = set_bool( str(LANG_CAPTION_BACKLIGHT),
                        &global_settings.caption_backlight);

    return rc;
}
#endif

/**
 * Menu to set icon visibility
 */
static bool show_icons(void)
{
    return set_bool( str(LANG_SHOW_ICONS), &global_settings.show_icons );
}

#ifdef HAVE_LCD_BITMAP

 /**
 * Menu to set LCD Mode (normal/inverse)
 */
static bool invert(void)
{
     bool rc = set_bool_options(str(LANG_INVERT),
                                &global_settings.invert,
                                STR(LANG_INVERT_LCD_INVERSE),
                                STR(LANG_INVERT_LCD_NORMAL),
                                lcd_set_invert_display);
     return rc;
}

/**
 * Menu to set Line Selector Type (Pointer/Bar)
 */
static bool invert_cursor(void)
{
        return set_bool_options(str(LANG_INVERT_CURSOR),
                            &global_settings.invert_cursor,
                            STR(LANG_INVERT_CURSOR_BAR),
                            STR(LANG_INVERT_CURSOR_POINTER),
                            NULL);
}

/**
 * Menu to turn the display+buttons by 180 degrees
 */
static bool flip_display(void)
{
    bool rc = set_bool( str(LANG_FLIP_DISPLAY),
                        &global_settings.flip_display);
    
    button_set_flip(global_settings.flip_display);
    lcd_set_flip(global_settings.flip_display);

    return rc;
}

/**
 * Menu to configure the battery display on status bar
 */
static bool battery_display(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_DISPLAY_GRAPHIC) }, 
        { STR(LANG_DISPLAY_NUMERIC) }
    };
    return set_option( str(LANG_BATTERY_DISPLAY), 
                       &global_settings.battery_display, INT, names, 2, NULL);
}

/**
 * Menu to configure the volume display on status bar
 */
static bool volume_type(void)
{
    static const struct opt_items names[] = { 
        { STR(LANG_DISPLAY_GRAPHIC) }, 
        { STR(LANG_DISPLAY_NUMERIC) }
    };
    return set_option( str(LANG_VOLUME_DISPLAY), &global_settings.volume_type,
                       INT, names, 2, NULL);
}

#ifdef PM_DEBUG
static bool peak_meter_fps_menu(void) {
    bool retval = false;
    retval = set_int( "Refresh rate", "/s", UNIT_PER_SEC,
             &peak_meter_fps,
             NULL, 1, 5, 40);
    return retval;
}
#endif /* PM_DEBUG */

/**
 * Menu to set the hold time of normal peaks.
 */
static bool peak_meter_hold(void) {
    bool retval = false;
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "200 ms " , TALK_ID(200, UNIT_MS) },
        { "300 ms " , TALK_ID(300, UNIT_MS) },
        { "500 ms " , TALK_ID(500, UNIT_MS) },
        { "1 s" , TALK_ID(1, UNIT_SEC) },
        { "2 s" , TALK_ID(2, UNIT_SEC) },
        { "3 s" , TALK_ID(3, UNIT_SEC) },
        { "4 s" , TALK_ID(4, UNIT_SEC) },
        { "5 s" , TALK_ID(5, UNIT_SEC) },
        { "6 s" , TALK_ID(6, UNIT_SEC) },
        { "7 s" , TALK_ID(7, UNIT_SEC) },
        { "8 s" , TALK_ID(8, UNIT_SEC) },
        { "9 s" , TALK_ID(9, UNIT_SEC) },
        { "10 s" , TALK_ID(10, UNIT_SEC) },
        { "15 s" , TALK_ID(15, UNIT_SEC) },
        { "20 s" , TALK_ID(20, UNIT_SEC) },
        { "30 s" , TALK_ID(30, UNIT_SEC) },
        { "1 min" , TALK_ID(1, UNIT_MIN) }
    };
    retval = set_option( str(LANG_PM_PEAK_HOLD),
                         &global_settings.peak_meter_hold, INT, names,
                         18, NULL);

    peak_meter_init_times(global_settings.peak_meter_release,
        global_settings.peak_meter_hold, 
        global_settings.peak_meter_clip_hold);

    return retval;
}

/**
 * Menu to set the hold time of clips.
 */
static bool peak_meter_clip_hold(void) {
    bool retval = false;

    static const struct opt_items names[] = {
        { STR(LANG_PM_ETERNAL) },
        { "1s " , TALK_ID(1, UNIT_SEC) },
        { "2s " , TALK_ID(2, UNIT_SEC) },
        { "3s " , TALK_ID(3, UNIT_SEC) },
        { "4s " , TALK_ID(4, UNIT_SEC) },
        { "5s " , TALK_ID(5, UNIT_SEC) },
        { "6s " , TALK_ID(6, UNIT_SEC) },
        { "7s " , TALK_ID(7, UNIT_SEC) },
        { "8s " , TALK_ID(8, UNIT_SEC) },
        { "9s " , TALK_ID(9, UNIT_SEC) },
        { "10s" , TALK_ID(10, UNIT_SEC) },
        { "15s" , TALK_ID(15, UNIT_SEC) },
        { "20s" , TALK_ID(20, UNIT_SEC) },
        { "25s" , TALK_ID(25, UNIT_SEC) },
        { "30s" , TALK_ID(30, UNIT_SEC) },
        { "45s" , TALK_ID(45, UNIT_SEC) },
        { "60s" , TALK_ID(60, UNIT_SEC) },
        { "90s" , TALK_ID(90, UNIT_SEC) },
        { "2min" , TALK_ID(2, UNIT_MIN) },
        { "3min" , TALK_ID(3, UNIT_MIN) },
        { "5min" , TALK_ID(5, UNIT_MIN) },
        { "10min" , TALK_ID(10, UNIT_MIN) },
        { "20min" , TALK_ID(20, UNIT_MIN) },
        { "45min" , TALK_ID(45, UNIT_MIN) },
        { "90min" , TALK_ID(90, UNIT_MIN) }
    };
    retval = set_option( str(LANG_PM_CLIP_HOLD), 
                         &global_settings.peak_meter_clip_hold, INT, names,
                         25, peak_meter_set_clip_hold);

    peak_meter_init_times(global_settings.peak_meter_release,
        global_settings.peak_meter_hold, 
        global_settings.peak_meter_clip_hold);

    return retval;
}

/**
 * Menu to set the release time of the peak meter.
 */
static bool peak_meter_release(void)  {
    bool retval = false;

    /* The range of peak_meter_release is restricted so that it
       fits into a 7 bit number. The 8th bit is used for storing
       something else in the rtc ram.
       Also, the max value is 0x7e, since the RTC value 0xff is reserved */
    retval = set_int( str(LANG_PM_RELEASE), str(LANG_PM_UNITS_PER_READ),
                      LANG_PM_UNITS_PER_READ,
                      &global_settings.peak_meter_release,
                      NULL, 1, 1, 0x7e);

    peak_meter_init_times(global_settings.peak_meter_release,
        global_settings.peak_meter_hold, 
        global_settings.peak_meter_clip_hold);

    return retval;
}

/**
 * Menu to select wether the scale of the meter 
 * displays dBfs of linear values.
 */
static bool peak_meter_scale(void) {
    bool retval = false;
    bool use_dbfs = global_settings.peak_meter_dbfs;
    retval = set_bool_options(str(LANG_PM_SCALE), 
        &use_dbfs, 
        STR(LANG_PM_DBFS), STR(LANG_PM_LINEAR),
        NULL);

    /* has the user really changed the scale? */
    if (use_dbfs != global_settings.peak_meter_dbfs) {

        /* store the change */
        global_settings.peak_meter_dbfs = use_dbfs;
        peak_meter_set_use_dbfs(use_dbfs);

        /* If the user changed the scale mode the meaning of
           peak_meter_min (peak_meter_max) has changed. Thus we have
           to convert the values stored in global_settings. */
        if (use_dbfs) {

            /* we only store -dBfs */
            global_settings.peak_meter_min = -peak_meter_get_min() / 100;
            global_settings.peak_meter_max = -peak_meter_get_max() / 100;
        } else {
            int max;
            
            /* linear percent */
            global_settings.peak_meter_min = peak_meter_get_min();

            /* converting dBfs -> percent results in a precision loss.
               I assume that the user doesn't bother that conversion
               dBfs <-> percent isn't symmetrical for odd values but that
               he wants 0 dBfs == 100%. Thus I 'correct' the percent value
               resulting from dBfs -> percent manually here */
            max = peak_meter_get_max();
            global_settings.peak_meter_max = max < 99 ? max : 100;
        }
        settings_apply_pm_range();
    }
    return retval;
}

/**
 * Adjust the min value of the value range that
 * the peak meter shall visualize.
 */
static bool peak_meter_min(void) {
    bool retval = false;
    if (global_settings.peak_meter_dbfs) {

        /* for dBfs scale */
        int range_max = -global_settings.peak_meter_max;
        int min = -global_settings.peak_meter_min;

        retval =  set_int(str(LANG_PM_MIN), str(LANG_PM_DBFS), UNIT_DB,
            &min, NULL, 1, -89, range_max);

        global_settings.peak_meter_min = - min;
    } 

    /* for linear scale */
    else {
        int min = global_settings.peak_meter_min;

        retval =  set_int(str(LANG_PM_MIN), "%", UNIT_PERCENT,
            &min, NULL, 
            1, 0, global_settings.peak_meter_max - 1);

        global_settings.peak_meter_min = (unsigned char)min;
    }

    settings_apply_pm_range();
    return retval;
}


/**
 * Adjust the max value of the value range that
 * the peak meter shall visualize.
 */
static bool peak_meter_max(void) {
    bool retval = false;
    if (global_settings.peak_meter_dbfs) {

        /* for dBfs scale */
        int range_min = -global_settings.peak_meter_min;
        int max = -global_settings.peak_meter_max;;

        retval =  set_int(str(LANG_PM_MAX), str(LANG_PM_DBFS), UNIT_DB,
            &max, NULL, 1, range_min, 0);

        global_settings.peak_meter_max = - max;

    } 
    
    /* for linear scale */
    else {
        int max = global_settings.peak_meter_max;

        retval =  set_int(str(LANG_PM_MAX), "%", UNIT_PERCENT,
            &max, NULL, 
            1, global_settings.peak_meter_min + 1, 100);

        global_settings.peak_meter_max = (unsigned char)max;
    }

    settings_apply_pm_range();
    return retval;
}

/**
 * Menu to select wether the meter is in
 * precision or in energy saver mode
 */
static bool peak_meter_performance(void) {
    bool retval = false;
    retval = set_bool_options(str(LANG_PM_PERFORMANCE), 
        &global_settings.peak_meter_performance, 
        STR(LANG_PM_HIGH_PERFORMANCE), STR(LANG_PM_ENERGY_SAVER),
        NULL);

    if (global_settings.peak_meter_performance) {
        peak_meter_fps = 25;
    } else {
        peak_meter_fps = 20;
    }
    return retval;
}

/**
 * Menu to configure the peak meter
 */
static bool peak_meter_menu(void) 
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_PM_RELEASE)  , peak_meter_release   },  
        { ID2P(LANG_PM_PEAK_HOLD), peak_meter_hold      },  
        { ID2P(LANG_PM_CLIP_HOLD), peak_meter_clip_hold },
        { ID2P(LANG_PM_PERFORMANCE), peak_meter_performance },
#ifdef PM_DEBUG
        { "Refresh rate" , -1    , peak_meter_fps_menu  },
#endif
        { ID2P(LANG_PM_SCALE)    , peak_meter_scale     },
        { ID2P(LANG_PM_MIN)      , peak_meter_min       },
        { ID2P(LANG_PM_MAX)      , peak_meter_max       },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL );
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif /* HAVE_LCD_BITMAP */

static bool shuffle(void)
{
    return set_bool( str(LANG_SHUFFLE), &global_settings.playlist_shuffle );
}

static bool repeat_mode(void)
{
    bool result;
    static const struct opt_items names[] = { 
        { STR(LANG_OFF) },
        { STR(LANG_REPEAT_ALL) },
        { STR(LANG_REPEAT_ONE) }
    };
    int old_repeat = global_settings.repeat_mode;

    result = set_option( str(LANG_REPEAT), &global_settings.repeat_mode,
                         INT, names, 3, NULL );

    if (old_repeat != global_settings.repeat_mode)
        audio_flush_and_reload_tracks();

    return result;
}

static bool play_selected(void)
{
    return set_bool( str(LANG_PLAY_SELECTED), &global_settings.play_selected );
}

static bool dir_filter(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_FILTER_ALL) },
        { STR(LANG_FILTER_SUPPORTED) },
        { STR(LANG_FILTER_MUSIC) },
        { STR(LANG_FILTER_PLAYLIST) },
        { STR(LANG_FILTER_ID3DB) }
    };
    return set_option( str(LANG_FILTER), &global_settings.dirfilter, INT,
                       names, 5, NULL );
}

static bool sort_case(void)
{
    return set_bool( str(LANG_SORT_CASE), &global_settings.sort_case );
}

static bool sort_file(void)
{
    int oldval = global_settings.sort_file;
    bool ret;
    static const struct opt_items names[] = {
        { STR(LANG_SORT_ALPHA) },
        { STR(LANG_SORT_DATE) },
        { STR(LANG_SORT_DATE_REVERSE) },
        { STR(LANG_SORT_TYPE) }
    };
    ret = set_option( str(LANG_SORT_FILE), &global_settings.sort_file, INT,
                       names, 4, NULL );
    if (global_settings.sort_file != oldval)
        reload_directory(); /* force reload if this has changed */
    return ret;
}

static bool sort_dir(void)
{
    int oldval = global_settings.sort_dir;
    bool ret;
    static const struct opt_items names[] = {
        { STR(LANG_SORT_ALPHA) },
        { STR(LANG_SORT_DATE) },
        { STR(LANG_SORT_DATE_REVERSE) }
    };
    ret = set_option( str(LANG_SORT_DIR), &global_settings.sort_dir, INT,
                       names, 3, NULL );
    if (global_settings.sort_dir != oldval)
        reload_directory(); /* force reload if this has changed */
    return ret;
}

static bool resume(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_SET_BOOL_NO) }, 
        { STR(LANG_RESUME_SETTING_ASK) },
        { STR(LANG_RESUME_SETTING_ASK_ONCE) },
        { STR(LANG_SET_BOOL_YES) }
    };
    return set_option( str(LANG_RESUME), &global_settings.resume, INT,
                       names, 4, NULL );
}

static bool autocreatebookmark(void)
{
    bool retval = false;
    static const struct opt_items names[] = {
        { STR(LANG_SET_BOOL_NO) },
        { STR(LANG_SET_BOOL_YES) },
        { STR(LANG_RESUME_SETTING_ASK) },
        { STR(LANG_BOOKMARK_SETTINGS_RECENT_ONLY_YES) },
        { STR(LANG_BOOKMARK_SETTINGS_RECENT_ONLY_ASK) }
    };

    retval = set_option( str(LANG_BOOKMARK_SETTINGS_AUTOCREATE),
                       &global_settings.autocreatebookmark, INT,
                       names, 5, NULL );
    if(global_settings.autocreatebookmark ==  BOOKMARK_RECENT_ONLY_YES ||
       global_settings.autocreatebookmark ==  BOOKMARK_RECENT_ONLY_ASK)
    {
        if(global_settings.usemrb == BOOKMARK_NO)
            global_settings.usemrb = BOOKMARK_YES;

    }
    return retval;
}

static bool autoloadbookmark(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_SET_BOOL_NO) },
        { STR(LANG_SET_BOOL_YES) },
        { STR(LANG_RESUME_SETTING_ASK) }
    };
    return set_option( str(LANG_BOOKMARK_SETTINGS_AUTOLOAD),
                       &global_settings.autoloadbookmark, INT,
                       names, 3, NULL );
}

static bool useMRB(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_SET_BOOL_NO) },
        { STR(LANG_SET_BOOL_YES) },
        { STR(LANG_BOOKMARK_SETTINGS_UNIQUE_ONLY) }
    };
    return set_option( str(LANG_BOOKMARK_SETTINGS_MAINTAIN_RECENT_BOOKMARKS),
                       &global_settings.usemrb, INT,
                       names, 3, NULL );
}

#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_CHARGING
static bool backlight_on_when_charging(void)
{
    bool result = set_bool(str(LANG_BACKLIGHT_ON_WHEN_CHARGING),
                           &global_settings.backlight_on_when_charging);
    backlight_set_on_when_charging(global_settings.backlight_on_when_charging);
    return result;
}
#endif

static bool backlight_timer(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_ON) },
        { "1s ", TALK_ID(1, UNIT_SEC) },
        { "2s ", TALK_ID(2, UNIT_SEC) },
        { "3s ", TALK_ID(3, UNIT_SEC) },
        { "4s ", TALK_ID(4, UNIT_SEC) },
        { "5s ", TALK_ID(5, UNIT_SEC) },
        { "6s ", TALK_ID(6, UNIT_SEC) },
        { "7s ", TALK_ID(7, UNIT_SEC) },
        { "8s ", TALK_ID(8, UNIT_SEC) },
        { "9s ", TALK_ID(9, UNIT_SEC) },
        { "10s", TALK_ID(10, UNIT_SEC) },
        { "15s", TALK_ID(15, UNIT_SEC) },
        { "20s", TALK_ID(20, UNIT_SEC) },
        { "25s", TALK_ID(25, UNIT_SEC) },
        { "30s", TALK_ID(30, UNIT_SEC) },
        { "45s", TALK_ID(45, UNIT_SEC) },
        { "60s", TALK_ID(60, UNIT_SEC) },
        { "90s", TALK_ID(90, UNIT_SEC) }
    };
    return set_option(str(LANG_BACKLIGHT), &global_settings.backlight_timeout,
                      INT, names, 19, backlight_set_timeout );
}
#endif /* CONFIG_BACKLIGHT */

static bool poweroff_idle_timer(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "1m ", TALK_ID(1, UNIT_MIN) },
        { "2m ", TALK_ID(2, UNIT_MIN) },
        { "3m ", TALK_ID(3, UNIT_MIN) },
        { "4m ", TALK_ID(4, UNIT_MIN) },
        { "5m ", TALK_ID(5, UNIT_MIN) },
        { "6m ", TALK_ID(6, UNIT_MIN) },
        { "7m ", TALK_ID(7, UNIT_MIN) },
        { "8m ", TALK_ID(8, UNIT_MIN) },
        { "9m ", TALK_ID(9, UNIT_MIN) },
        { "10m", TALK_ID(10, UNIT_MIN) },
        { "15m", TALK_ID(15, UNIT_MIN) },
        { "30m", TALK_ID(30, UNIT_MIN) },
        { "45m", TALK_ID(45, UNIT_MIN) },
        { "60m", TALK_ID(60, UNIT_MIN) }
    };
    return set_option(str(LANG_POWEROFF_IDLE), &global_settings.poweroff,
                      INT, names, 15, set_poweroff_timeout);
}

static bool scroll_speed(void)
{
    return set_int(str(LANG_SCROLL), "", UNIT_INT,
                   &global_settings.scroll_speed,
                   &lcd_scroll_speed, 1, 0, 15 );
}


static bool scroll_delay(void)
{
    int dummy = global_settings.scroll_delay * (HZ/10);
    int rc = set_int(str(LANG_SCROLL_DELAY), "ms", UNIT_MS,
                     &dummy, 
                     &lcd_scroll_delay, 100, 0, 2500 );
    global_settings.scroll_delay = dummy / (HZ/10);
    return rc;
}

#ifdef HAVE_LCD_BITMAP
static bool scroll_step(void)
{
    return set_int(str(LANG_SCROLL_STEP_EXAMPLE), "pixels", UNIT_PIXEL,
                   &global_settings.scroll_step,
                   &lcd_scroll_step, 1, 1, LCD_WIDTH );
}
#endif

static bool bidir_limit(void)
{
    return set_int(str(LANG_BIDIR_SCROLL), "%", UNIT_PERCENT,
                   &global_settings.bidir_limit, 
                   &lcd_bidir_scroll, 25, 0, 200 );
}

#ifdef HAVE_LCD_CHARCELLS
static bool jump_scroll(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_ONE_TIME) },
        { "2", TALK_ID(2, UNIT_INT) },
        { "3", TALK_ID(3, UNIT_INT) },
        { "4", TALK_ID(4, UNIT_INT) },
        { STR(LANG_ALWAYS) }
    };
    bool ret;
    ret=set_option(str(LANG_JUMP_SCROLL), &global_settings.jump_scroll,
                   INT, names, 6, lcd_jump_scroll);
    return ret;
}
static bool jump_scroll_delay(void)
{
    int dummy = global_settings.jump_scroll_delay * (HZ/10);
    int rc = set_int(str(LANG_JUMP_SCROLL_DELAY), "ms", UNIT_MS,
                     &dummy, 
                     &lcd_jump_scroll_delay, 100, 0, 2500 );
    global_settings.jump_scroll_delay = dummy / (HZ/10);
    return rc;
}
#endif

#ifndef SIMULATOR
/**
 * Menu to set the battery capacity
 */
static bool battery_capacity(void)
{
    return set_int(str(LANG_BATTERY_CAPACITY), "mAh", UNIT_MAH,
                   &global_settings.battery_capacity, 
                   &set_battery_capacity, 50, BATTERY_CAPACITY_MIN,
                   BATTERY_CAPACITY_MAX );
}

#if BATTERY_TYPES_COUNT > 1
static bool battery_type(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_BATTERY_TYPE_ALKALINE) },
        { STR(LANG_BATTERY_TYPE_NIMH) }
    };

    return set_option(str(LANG_BATTERY_TYPE), &global_settings.battery_type,
                      INT, names, 2, set_battery_type);
}
#endif
#endif

#ifdef HAVE_RTC
static bool timedate_set(void)
{
    struct tm tm;
    int timedate[8];
    bool result;

    timedate[0] = rtc_read(0x03); /* hour   */
    timedate[1] = rtc_read(0x02); /* minute */
    timedate[2] = rtc_read(0x01); /* second */
    timedate[3] = rtc_read(0x07); /* year   */
    timedate[4] = rtc_read(0x06); /* month  */
    timedate[5] = rtc_read(0x05); /* day    */

    /* Make a local copy of the time struct */
    memcpy(&tm, get_time(), sizeof(struct tm));

    /* do some range checks */
    /* This prevents problems with time/date setting after a power loss */
    if (!valid_time(&tm))
    {
        /* hour   */
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_mday = 1;
        tm.tm_mon = 0;
        tm.tm_wday = 1;
        tm.tm_year = 100;
    }

    result = set_time_screen(str(LANG_TIME), &tm);

    if(tm.tm_year != -1) {
        set_time(&tm);
    }
    return result;
}

static bool timeformat_set(void)
{
    static const struct opt_items names[] = { 
        { STR(LANG_24_HOUR_CLOCK) },
        { STR(LANG_12_HOUR_CLOCK) }
    };
    return set_option(str(LANG_TIMEFORMAT), &global_settings.timeformat, 
                      INT, names, 2, NULL);
}
#endif

#ifndef HAVE_MMC
static bool spindown(void)
{
    return set_int(str(LANG_SPINDOWN), "s", UNIT_SEC,
                   &global_settings.disk_spindown,
                   ata_spindown, 1, 3, 254 );
}

#ifdef HAVE_ATA_POWER_OFF
static bool poweroff(void)
{
    bool rc = set_bool(str(LANG_POWEROFF), &global_settings.disk_poweroff);
    ata_poweroff(global_settings.disk_poweroff);
    return rc;
}
#endif /* HAVE_ATA_POWEROFF */
#endif /* !HAVE_MMC */

#if CONFIG_HWCODEC == MAS3507D
static bool line_in(void)
{
    bool rc = set_bool(str(LANG_LINE_IN), &global_settings.line_in);
    dac_line_in(global_settings.line_in);
    return rc;
}
#endif

static bool max_files_in_dir(void)
{
    return set_int(str(LANG_MAX_FILES_IN_DIR), "", UNIT_INT,
                   &global_settings.max_files_in_dir,
                   NULL, 50, 50, 10000 );
}

static bool max_files_in_playlist(void)
{
    return set_int(str(LANG_MAX_FILES_IN_PLAYLIST), "", UNIT_INT,
                   &global_settings.max_files_in_playlist,
                   NULL, 1000, 1000, 20000 );
}

static bool buffer_margin(void)
{
    return set_int(str(LANG_MP3BUFFER_MARGIN), "s", UNIT_SEC,
                   &global_settings.buffer_margin,
                   audio_set_buffer_margin, 1, 0, 7 );
}

static bool ff_rewind_min_step(void)
{ 
    static const struct opt_items names[] = {
        { "1s", TALK_ID(1, UNIT_SEC) },
        { "2s", TALK_ID(2, UNIT_SEC) },
        { "3s", TALK_ID(3, UNIT_SEC) },
        { "4s", TALK_ID(4, UNIT_SEC) },
        { "5s", TALK_ID(5, UNIT_SEC) },
        { "6s", TALK_ID(6, UNIT_SEC) },
        { "8s", TALK_ID(8, UNIT_SEC) },
        { "10s", TALK_ID(10, UNIT_SEC) },
        { "15s", TALK_ID(15, UNIT_SEC) },
        { "20s", TALK_ID(20, UNIT_SEC) },
        { "25s", TALK_ID(25, UNIT_SEC) },
        { "30s", TALK_ID(30, UNIT_SEC) },
        { "45s", TALK_ID(45, UNIT_SEC) },
        { "60s", TALK_ID(60, UNIT_SEC) }
    };
    return set_option(str(LANG_FFRW_STEP), &global_settings.ff_rewind_min_step,
                      INT, names, 14, NULL ); 
} 

static bool set_fade_on_stop(void)
{
    return set_bool( str(LANG_FADE_ON_STOP), &global_settings.fade_on_stop );
}


static bool ff_rewind_accel(void) 
{ 
    static const struct opt_items names[] = {
        { STR(LANG_OFF) }, 
        { "2x/1s", TALK_ID(1, UNIT_SEC) },
        { "2x/2s", TALK_ID(2, UNIT_SEC) },
        { "2x/3s", TALK_ID(3, UNIT_SEC) },
        { "2x/4s", TALK_ID(4, UNIT_SEC) },
        { "2x/5s", TALK_ID(5, UNIT_SEC) },
        { "2x/6s", TALK_ID(6, UNIT_SEC) },
        { "2x/7s", TALK_ID(7, UNIT_SEC) },
        { "2x/8s", TALK_ID(8, UNIT_SEC) },
        { "2x/9s", TALK_ID(9, UNIT_SEC) },
        { "2x/10s", TALK_ID(10, UNIT_SEC) },
        { "2x/11s", TALK_ID(11, UNIT_SEC) },
        { "2x/12s", TALK_ID(12, UNIT_SEC) },
        { "2x/13s", TALK_ID(13, UNIT_SEC) },
        { "2x/14s", TALK_ID(14, UNIT_SEC) },
        { "2x/15s", TALK_ID(15, UNIT_SEC) }
    };
    return set_option(str(LANG_FFRW_ACCEL), &global_settings.ff_rewind_accel, 
                      INT, names, 16, NULL ); 
} 

static bool browse_current(void)
{
    return set_bool( str(LANG_FOLLOW), &global_settings.browse_current );
}

static bool custom_wps_browse(void)
{
    return rockbox_browse(ROCKBOX_DIR, SHOW_WPS);
}

static bool custom_cfg_browse(void)
{
    return rockbox_browse(ROCKBOX_DIR, SHOW_CFG);
}

static bool language_browse(void)
{
    return rockbox_browse(ROCKBOX_DIR LANG_DIR, SHOW_LNG);
}

static bool voice_menus(void)
{
    bool ret;
    bool temp = global_settings.talk_menu;
    /* work on a temp variable first, avoid "life" disabling */
    ret = set_bool( str(LANG_VOICE_MENU), &temp );
    global_settings.talk_menu = temp;
    return ret;
}

/* this is used 2 times below, so it saves memory to put it in global scope */
static const struct opt_items voice_names[] = {
    { STR(LANG_OFF) }, 
    { STR(LANG_VOICE_NUMBER) },
    { STR(LANG_VOICE_SPELL) },
    { STR(LANG_VOICE_DIR_HOVER) }
};

static bool voice_dirs(void)
{
    return set_option( str(LANG_VOICE_DIR), 
                       &global_settings.talk_dir, INT, voice_names, 4, NULL);
}

static bool voice_files(void)
{
    int oldval = global_settings.talk_file;
    bool ret;
    ret = set_option( str(LANG_VOICE_FILE), 
                       &global_settings.talk_file, INT, voice_names, 4, NULL);
    if (oldval != 3 && global_settings.talk_file == 3)
    {   /* force reload if newly talking thumbnails, 
           because the clip presence is cached only if enabled */  
        reload_directory();
    }
    return ret;
}

static bool voice_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_VOICE_MENU), voice_menus },
        { ID2P(LANG_VOICE_DIR),  voice_dirs  },
        { ID2P(LANG_VOICE_FILE),  voice_files }
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

#ifdef HAVE_LCD_BITMAP
static bool font_browse(void)
{
    return rockbox_browse(ROCKBOX_DIR FONT_DIR, SHOW_FONT);
}

static bool scroll_bar(void)
{
    return set_bool( str(LANG_SCROLL_BAR), &global_settings.scrollbar );
}

static bool status_bar(void)
{
    return set_bool( str(LANG_STATUS_BAR), &global_settings.statusbar );
}

#if CONFIG_KEYPAD == RECORDER_PAD
static bool button_bar(void)
{
    return set_bool( str(LANG_BUTTON_BAR), &global_settings.buttonbar );
}
#endif /* CONFIG_KEYPAD == RECORDER_PAD */
#endif /* HAVE_LCD_BITMAP */

static bool ff_rewind_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_FFRW_STEP), ff_rewind_min_step },
        { ID2P(LANG_FFRW_ACCEL), ff_rewind_accel },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}

static bool id3_order(void)
{
    return set_bool_options( str(LANG_ID3_ORDER),
                             &global_settings.id3_v1_first,
                             STR(LANG_ID3_V1_FIRST),
                             STR(LANG_ID3_V2_FIRST),
                             mpeg_id3_options);
}

static bool playback_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SHUFFLE), shuffle },
        { ID2P(LANG_REPEAT), repeat_mode },
        { ID2P(LANG_PLAY_SELECTED), play_selected },
        { ID2P(LANG_RESUME), resume },
        { ID2P(LANG_WIND_MENU), ff_rewind_settings_menu },
        { ID2P(LANG_MP3BUFFER_MARGIN), buffer_margin },
        { ID2P(LANG_FADE_ON_STOP), set_fade_on_stop },
        { ID2P(LANG_ID3_ORDER), id3_order },
    };

    bool old_shuffle = global_settings.playlist_shuffle;

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    if (old_shuffle != global_settings.playlist_shuffle)
    {
        if (global_settings.playlist_shuffle)
        {
            playlist_randomise(NULL, current_tick, true);
        }
        else
        {
            playlist_sort(NULL, true);
        }
    }
    return result;
}

static bool bookmark_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_BOOKMARK_SETTINGS_AUTOCREATE), autocreatebookmark},
        { ID2P(LANG_BOOKMARK_SETTINGS_AUTOLOAD), autoloadbookmark},
        { ID2P(LANG_BOOKMARK_SETTINGS_MAINTAIN_RECENT_BOOKMARKS), useMRB},
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
static bool reset_settings(void)
{
    bool done=false;
    int line;
    int button;
 
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    line = 0;
#else
    line = 1;
    lcd_puts(0,0,str(LANG_RESET_ASK_RECORDER));
#endif
    lcd_puts(0,line,str(LANG_RESET_CONFIRM));
    lcd_puts(0,line+1,str(LANG_RESET_CANCEL));

    lcd_update();
     
    while(!done) {
        button = button_get(true);
        switch(button) {
            case SETTINGS_OK:
                settings_reset();
                settings_apply();
                lcd_clear_display();
                lcd_puts(0,1,str(LANG_RESET_DONE_CLEAR));
                done = true;
                break;

            case SETTINGS_CANCEL:
                lcd_clear_display();
                lcd_puts(0,1,str(LANG_RESET_DONE_CANCEL));
                done = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
        }
    }

    lcd_puts(0,0,str(LANG_RESET_DONE_SETTING));
    lcd_update();
    sleep(HZ);
    return false;
}

static bool fileview_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SORT_CASE),    sort_case           },
        { ID2P(LANG_SORT_DIR),     sort_dir            },
        { ID2P(LANG_SORT_FILE),    sort_file           },
        { ID2P(LANG_FILTER),       dir_filter          },
        { ID2P(LANG_FOLLOW),       browse_current      },
        { ID2P(LANG_SHOW_ICONS),   show_icons          },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}


static bool scroll_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SCROLL_SPEED),     scroll_speed    },
        { ID2P(LANG_SCROLL_DELAY),    scroll_delay    },  
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_SCROLL_STEP),     scroll_step     },  
#endif
        { ID2P(LANG_BIDIR_SCROLL),    bidir_limit    },
#ifdef HAVE_LCD_CHARCELLS
        { ID2P(LANG_JUMP_SCROLL),    jump_scroll    },
        { ID2P(LANG_JUMP_SCROLL_DELAY),    jump_scroll_delay    },
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

static bool lcd_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
#ifdef CONFIG_BACKLIGHT
        { ID2P(LANG_BACKLIGHT),       backlight_timer },
#ifdef HAVE_CHARGING
        { ID2P(LANG_BACKLIGHT_ON_WHEN_CHARGING), backlight_on_when_charging },
#endif
        { ID2P(LANG_CAPTION_BACKLIGHT), caption_backlight },
#endif /* CONFIG_BACKLIGHT */
        { ID2P(LANG_CONTRAST),        contrast },
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_INVERT),          invert },
        { ID2P(LANG_FLIP_DISPLAY),    flip_display },
        { ID2P(LANG_INVERT_CURSOR),   invert_cursor },
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

#ifdef HAVE_REMOTE_LCD
static bool lcd_remote_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_CONTRAST),        remote_contrast },
        { ID2P(LANG_INVERT),          remote_invert },
/*        { ID2P(LANG_FLIP_DISPLAY),    remote_flip_display },
        { ID2P(LANG_INVERT_CURSOR),   invert_cursor },*/
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif

#ifdef HAVE_LCD_BITMAP
static bool bars_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SCROLL_BAR),      scroll_bar },
        { ID2P(LANG_STATUS_BAR),      status_bar },
#if CONFIG_KEYPAD == RECORDER_PAD
        { ID2P(LANG_BUTTON_BAR),      button_bar },
#endif
        { ID2P(LANG_VOLUME_DISPLAY),  volume_type },
        { ID2P(LANG_BATTERY_DISPLAY), battery_display },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif


static bool display_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_CUSTOM_FONT),     font_browse },
#endif
        { ID2P(LANG_WHILE_PLAYING),   custom_wps_browse },
        { ID2P(LANG_LCD_MENU),        lcd_settings_menu },
#ifdef HAVE_REMOTE_LCD
        { ID2P(LANG_LCD_REMOTE_MENU), lcd_remote_settings_menu },
#endif
        { ID2P(LANG_SCROLL_MENU),     scroll_settings_menu },
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_BARS_MENU),       bars_settings_menu },
        { ID2P(LANG_PM_MENU),         peak_meter_menu },
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}


static bool firmware_browse(void)
{
    return rockbox_browse(ROCKBOX_DIR, SHOW_MOD);
}

static bool battery_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
#ifndef SIMULATOR
        { ID2P(LANG_BATTERY_CAPACITY), battery_capacity },
#if BATTERY_TYPES_COUNT > 1
        { ID2P(LANG_BATTERY_TYPE),     battery_type },
#endif
#else
#ifndef HAVE_CHARGE_CTRL
        { "Dummy", NULL }, /* to have an entry at all, in the simulator */
#endif
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

#ifndef HAVE_MMC
static bool disk_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SPINDOWN),    spindown        },
#ifdef HAVE_ATA_POWER_OFF
        { ID2P(LANG_POWEROFF),    poweroff        },
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif /* !HAVE_MMC */

#ifdef HAVE_RTC
static bool time_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_TIME),        timedate_set    },
        { ID2P(LANG_TIMEFORMAT),  timeformat_set  },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif

static bool manage_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_CUSTOM_CFG),      custom_cfg_browse },
        { ID2P(LANG_FIRMWARE),        firmware_browse },
        { ID2P(LANG_RESET),           reset_settings },
        { ID2P(LANG_SAVE_SETTINGS),   settings_save_config },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

static bool limits_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_MAX_FILES_IN_DIR),    max_files_in_dir        },
        { ID2P(LANG_MAX_FILES_IN_PLAYLIST),    max_files_in_playlist        },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}


static bool system_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_BATTERY_MENU),     battery_settings_menu },
#ifndef HAVE_MMC
        { ID2P(LANG_DISK_MENU),        disk_settings_menu     },
#endif
#ifdef HAVE_RTC
        { ID2P(LANG_TIME_MENU),        time_settings_menu     },
#endif
        { ID2P(LANG_POWEROFF_IDLE),    poweroff_idle_timer    },
        { ID2P(LANG_SLEEP_TIMER),      sleeptimer_screen      },
#ifdef HAVE_ALARM_MOD
        { ID2P(LANG_ALARM_MOD_ALARM_MENU), alarm_screen       },
#endif
        { ID2P(LANG_LIMITS_MENU),      limits_settings_menu   },
#if CONFIG_HWCODEC == MAS3507D
        { ID2P(LANG_LINE_IN),          line_in                },
#endif
#ifdef HAVE_CHARGING
        { ID2P(LANG_CAR_ADAPTER_MODE), car_adapter_mode       },
#endif
        { ID2P(LANG_MANAGE_MENU),      manage_settings_menu   },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

bool settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_PLAYBACK),         playback_settings_menu },
        { ID2P(LANG_FILE),             fileview_settings_menu },
        { ID2P(LANG_DISPLAY),          display_settings_menu  },
        { ID2P(LANG_SYSTEM),           system_settings_menu   },
        { ID2P(LANG_BOOKMARK_SETTINGS),bookmark_settings_menu },
        { ID2P(LANG_LANGUAGE),         language_browse        },
        { ID2P(LANG_VOICE),            voice_menu             },
    };
    
    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
