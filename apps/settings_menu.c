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
#include "powermgmt.h"
#include "rtc.h"
#include "ata.h"
#include "tree.h"
#include "screens.h"
#include "talk.h"
#include "timefuncs.h"
#include "misc.h"
#include "abrepeat.h"
#include "power.h"
#include "database.h"
#include "dir.h"
#include "dircache.h"
#include "rbunicode.h"
#include "splash.h"
#include "yesno.h"
#include "list.h"
#include "color_picker.h"

#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
#endif
#include "lang.h"
#if CONFIG_CODEC == MAS3507D
void dac_line_in(bool enable);
#endif
#ifdef HAVE_ALARM_MOD
#include "alarm_menu.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#include "pcm_playback.h"
#include "dsp.h"
#endif

#ifdef HAVE_CHARGING
static bool car_adapter_mode(void)
{
    return set_bool( str(LANG_CAR_ADAPTER_MODE),
                     &global_settings.car_adapter_mode );
}
#endif

/**
 * Menu to set icon visibility
 */
static bool show_icons(void)
{
    return set_bool( (char *)str(LANG_SHOW_ICONS), &global_settings.show_icons );
}

/**
 * Menu to set the option to scroll paginated
 */
static bool scroll_paginated(void)
{
    return set_bool( (char *)str(LANG_SCROLL_PAGINATED), &global_settings.scroll_paginated );
}

#ifdef HAVE_REMOTE_LCD
static bool remote_contrast(void)
{
    return set_int( str(LANG_CONTRAST), "", UNIT_INT,
                    &global_settings.remote_contrast, 
                    lcd_remote_set_contrast, 1, MIN_CONTRAST_SETTING,
                    MAX_CONTRAST_SETTING, NULL );
}

static bool remote_invert(void)
{
     bool rc = set_bool_options((char *)str(LANG_INVERT),
                                &global_settings.remote_invert,
                                (char *)STR(LANG_INVERT_LCD_INVERSE),
                                STR(LANG_INVERT_LCD_NORMAL),
                                lcd_remote_set_invert_display);
     return rc;
}

static bool remote_flip_display(void)
{
    bool rc = set_bool( (char *)str(LANG_FLIP_DISPLAY),
                        &global_settings.remote_flip_display);
    
    lcd_remote_set_flip(global_settings.remote_flip_display);
    lcd_remote_update();

    return rc;
}

#ifdef HAVE_REMOTE_LCD_TICKING
static bool remote_reduce_ticking(void)
{
    bool rc = set_bool( str(LANG_REDUCE_TICKING),
                        &global_settings.remote_reduce_ticking);
    
    lcd_remote_emireduce(global_settings.remote_reduce_ticking);

    return rc;
}
#endif
#endif

#ifdef CONFIG_BACKLIGHT
static const struct opt_items backlight_timeouts[] = {
    { STR(LANG_OFF) },
    { STR(LANG_ON) },
    { (unsigned char *)"1s ", TALK_ID(1, UNIT_SEC) },
    { (unsigned char *)"2s ", TALK_ID(2, UNIT_SEC) },
    { (unsigned char *)"3s ", TALK_ID(3, UNIT_SEC) },
    { (unsigned char *)"4s ", TALK_ID(4, UNIT_SEC) },
    { (unsigned char *)"5s ", TALK_ID(5, UNIT_SEC) },
    { (unsigned char *)"6s ", TALK_ID(6, UNIT_SEC) },
    { (unsigned char *)"7s ", TALK_ID(7, UNIT_SEC) },
    { (unsigned char *)"8s ", TALK_ID(8, UNIT_SEC) },
    { (unsigned char *)"9s ", TALK_ID(9, UNIT_SEC) },
    { (unsigned char *)"10s", TALK_ID(10, UNIT_SEC) },
    { (unsigned char *)"15s", TALK_ID(15, UNIT_SEC) },
    { (unsigned char *)"20s", TALK_ID(20, UNIT_SEC) },
    { (unsigned char *)"25s", TALK_ID(25, UNIT_SEC) },
    { (unsigned char *)"30s", TALK_ID(30, UNIT_SEC) },
    { (unsigned char *)"45s", TALK_ID(45, UNIT_SEC) },
    { (unsigned char *)"60s", TALK_ID(60, UNIT_SEC) },
    { (unsigned char *)"90s", TALK_ID(90, UNIT_SEC) }
};

static bool caption_backlight(void)
{
    return set_bool( (char *)str(LANG_CAPTION_BACKLIGHT),
                     &global_settings.caption_backlight);
}

#ifdef HAVE_CHARGING
static bool backlight_timer_plugged(void)
{
    return set_option((char *)str(LANG_BACKLIGHT_ON_WHEN_CHARGING),
                      &global_settings.backlight_timeout_plugged,
                      INT, backlight_timeouts, 19,
                      backlight_set_timeout_plugged );
}
#endif

static bool backlight_timer(void)
{
    return set_option((char *)str(LANG_BACKLIGHT),
                      &global_settings.backlight_timeout,
                      INT, backlight_timeouts, 19,
                      backlight_set_timeout );
}

#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
static bool backlight_fade_in(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { (unsigned char *)"500ms", TALK_ID(500, UNIT_MS) },
        { (unsigned char *)"1s", TALK_ID(1, UNIT_SEC) },
        { (unsigned char *)"2s", TALK_ID(2, UNIT_SEC) },
    };
    return set_option(str(LANG_BACKLIGHT_FADE_IN),
                      &global_settings.backlight_fade_in,
                      INT, names, 4, backlight_set_fade_in );
}

static bool backlight_fade_out(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { (unsigned char *)"500ms", TALK_ID(500, UNIT_MS) },
        { (unsigned char *)"1s", TALK_ID(1, UNIT_SEC) },
        { (unsigned char *)"2s", TALK_ID(2, UNIT_SEC) },
        { (unsigned char *)"3s", TALK_ID(3, UNIT_SEC) },
        { (unsigned char *)"4s", TALK_ID(4, UNIT_SEC) },
        { (unsigned char *)"5s", TALK_ID(5, UNIT_SEC) },
        { (unsigned char *)"10s", TALK_ID(10, UNIT_SEC) },
    };
    return set_option(str(LANG_BACKLIGHT_FADE_OUT),
                      &global_settings.backlight_fade_out,
                      INT, names, 8, backlight_set_fade_out );
}
#endif
#endif /* CONFIG_BACKLIGHT */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static bool brightness(void)
{
    return set_int( str(LANG_BRIGHTNESS), "", UNIT_INT,
                    &global_settings.brightness, 
                    backlight_set_brightness, 1, MIN_BRIGHTNESS_SETTING,
                    MAX_BRIGHTNESS_SETTING, NULL );
}
#endif

#ifdef HAVE_REMOTE_LCD

static bool remote_backlight_timer(void)
{
    return set_option((char *)str(LANG_BACKLIGHT),
                      &global_settings.remote_backlight_timeout,
                      INT, backlight_timeouts, 19,
                      remote_backlight_set_timeout );
}

#ifdef HAVE_CHARGING
static bool remote_backlight_timer_plugged(void)
{
    return set_option((char *)str(LANG_BACKLIGHT_ON_WHEN_CHARGING),
                      &global_settings.remote_backlight_timeout_plugged,
                      INT, backlight_timeouts, 19,
                      remote_backlight_set_timeout_plugged );
}
#endif

static bool remote_caption_backlight(void)
{
    return set_bool((char *)str(LANG_CAPTION_BACKLIGHT),
                    &global_settings.remote_caption_backlight);
}
#endif /* HAVE_REMOTE_LCD */

static bool contrast(void)
{
    return set_int( str(LANG_CONTRAST), "", UNIT_INT,
                    &global_settings.contrast, 
                    lcd_set_contrast, 1, MIN_CONTRAST_SETTING,
                    MAX_CONTRAST_SETTING, NULL );
}

#ifdef HAVE_LCD_BITMAP

 /**
 * Menu to set LCD Mode (normal/inverse)
 */
static bool invert(void)
{
    bool rc = set_bool_options(str(LANG_INVERT),
                               &global_settings.invert,
                               (char *)STR(LANG_INVERT_LCD_INVERSE),
                               STR(LANG_INVERT_LCD_NORMAL),
                               lcd_set_invert_display);
    return rc;
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

#ifdef HAVE_LCD_COLOR 
/**
 * Menu to clear the backdrop image
 */
static bool clear_main_backdrop(void)
{
    global_settings.backdrop_file[0]=0;
    lcd_set_backdrop(NULL);
    return false;
}

/**
 * Menu for fore/back colors
 */
static bool set_fg_color(void)
{
    bool res;

    res = set_color(&screens[SCREEN_MAIN],str(LANG_FOREGROUND_COLOR),
                    &global_settings.fg_color,global_settings.bg_color);

    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);

    return res;
}

static bool set_bg_color(void)
{
    bool res;

    res = set_color(&screens[SCREEN_MAIN],str(LANG_BACKGROUND_COLOR),
                    &global_settings.bg_color,global_settings.fg_color);

    screens[SCREEN_MAIN].set_background(global_settings.bg_color);

    return res;
}

static bool reset_color(void)
{
    global_settings.fg_color = LCD_DEFAULT_FG;
    global_settings.bg_color = LCD_DEFAULT_BG;
    
    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
    return false;
}
#endif

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
                      NULL, 1, 1, 0x7e, NULL);

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
            &min, NULL, 1, -89, range_max, NULL);

        global_settings.peak_meter_min = - min;
    } 

    /* for linear scale */
    else {
        int min = global_settings.peak_meter_min;

        retval =  set_int(str(LANG_PM_MIN), "%", UNIT_PERCENT,
            &min, NULL, 
            1, 0, global_settings.peak_meter_max - 1, NULL);

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
            &max, NULL, 1, range_min, 0, NULL);

        global_settings.peak_meter_max = - max;

    } 
    
    /* for linear scale */
    else {
        int max = global_settings.peak_meter_max;

        retval =  set_int(str(LANG_PM_MAX), "%", UNIT_PERCENT,
            &max, NULL, 
            1, global_settings.peak_meter_min + 1, 100, NULL);

        global_settings.peak_meter_max = (unsigned char)max;
    }

    settings_apply_pm_range();
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
        { STR(LANG_REPEAT_ONE) },
        { STR(LANG_SHUFFLE) },
#if (AB_REPEAT_ENABLE == 1)
        { STR(LANG_REPEAT_AB) }
#endif
    };
    int old_repeat = global_settings.repeat_mode;

    result = set_option( str(LANG_REPEAT), &global_settings.repeat_mode,
                         INT, names, NUM_REPEAT_MODES, NULL );

    if (old_repeat != global_settings.repeat_mode &&
        (audio_status() & AUDIO_STATUS_PLAY))
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
    return set_bool( str(LANG_RESUME), &global_settings.resume); 
}

#ifdef HAVE_SPDIF_POWER
static bool spdif(void)
{
     bool rc = set_bool_options(str(LANG_SPDIF_ENABLE),
                                &global_settings.spdif_enable,
                                STR(LANG_ON),
                                STR(LANG_OFF),
                                spdif_power_enable);
     return rc;
}
#endif

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

static void sleep_timer_formatter(char* buffer, int buffer_size, int value,
    const char* unit)
{
    int minutes, hours;

    (void) unit;

    if (value) {
        hours = value / 60;
        minutes = value - (hours * 60);
        snprintf(buffer, buffer_size, "%d:%02d", hours, minutes);
    } else {
        snprintf(buffer, buffer_size, "%s", str(LANG_OFF));
    }
}

static void sleep_timer_set(int minutes)
{
    set_sleep_timer(minutes * 60);
}

static bool sleep_timer(void)
{
    int minutes = get_sleep_timer() / 60;

    return set_int(str(LANG_SLEEP_TIMER), "", UNIT_MIN, &minutes,
        &sleep_timer_set, 15, 0, 300, sleep_timer_formatter);
}

static bool scroll_speed(void)
{
    return set_int(str(LANG_SCROLL), "", UNIT_INT,
                   &global_settings.scroll_speed,
                   &lcd_scroll_speed, 1, 0, 15, NULL );
}

static bool scroll_delay(void)
{
    int dummy = global_settings.scroll_delay * (HZ/10);
    int rc = set_int(str(LANG_SCROLL_DELAY), "ms", UNIT_MS,
                     &dummy, 
                     &lcd_scroll_delay, 100, 0, 2500, NULL );
    global_settings.scroll_delay = dummy / (HZ/10);
    return rc;
}

#ifdef HAVE_LCD_BITMAP
static bool screen_scroll(void)
{
    bool rc = set_bool( str(LANG_SCREEN_SCROLL_VIEW), &global_settings.offset_out_of_view);
    gui_list_screen_scroll_out_of_view(global_settings.offset_out_of_view);
    return rc;
}

static bool screen_scroll_step(void)
{
    return set_int(str(LANG_SCREEN_SCROLL_STEP), "pixels", UNIT_PIXEL,
                   &global_settings.screen_scroll_step,
                   &gui_list_screen_scroll_step, 1, 1, LCD_WIDTH, NULL );
}

static bool scroll_step(void)
{
    return set_int(str(LANG_SCROLL_STEP_EXAMPLE), "pixels", UNIT_PIXEL,
                   &global_settings.scroll_step,
                   &lcd_scroll_step, 1, 1, LCD_WIDTH, NULL );
}
#endif

static bool bidir_limit(void)
{
    return set_int(str(LANG_BIDIR_SCROLL), "%", UNIT_PERCENT,
                   &global_settings.bidir_limit, 
                   &lcd_bidir_scroll, 25, 0, 200, NULL );
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
                     &lcd_jump_scroll_delay, 100, 0, 2500, NULL );
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
                   BATTERY_CAPACITY_MAX, NULL );
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

#ifdef CONFIG_RTC
static bool timedate_set(void)
{
    struct tm tm;
    bool result;

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
                   ata_spindown, 1, 3, 254, NULL );
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

#if CONFIG_CODEC == MAS3507D
static bool line_in(void)
{
    bool rc = set_bool(str(LANG_LINE_IN), &global_settings.line_in);
#ifndef SIMULATOR
    dac_line_in(global_settings.line_in);
#endif
    return rc;
}
#endif

static bool max_files_in_dir(void)
{
    return set_int(str(LANG_MAX_FILES_IN_DIR), "", UNIT_INT,
                   &global_settings.max_files_in_dir,
                   NULL, 50, 50, 10000, NULL );
}

static bool max_files_in_playlist(void)
{
    return set_int(str(LANG_MAX_FILES_IN_PLAYLIST), "", UNIT_INT,
                   &global_settings.max_files_in_playlist,
                   NULL, 1000, 1000, 20000, NULL );
}

#if CONFIG_CODEC == SWCODEC
static bool buffer_margin(void)
{
    int ret;
    static const struct opt_items names[] = {
        { "5s", TALK_ID(5, UNIT_SEC) },
        { "15s", TALK_ID(15, UNIT_SEC) },
        { "30s", TALK_ID(30, UNIT_SEC) },
        { "1min", TALK_ID(1, UNIT_MIN) },
        { "2min", TALK_ID(2, UNIT_MIN) },
        { "3min", TALK_ID(3, UNIT_MIN) },
        { "5min", TALK_ID(5, UNIT_MIN) },
        { "10min", TALK_ID(10, UNIT_MIN) }
    };
    
    ret = set_option(str(LANG_MP3BUFFER_MARGIN), &global_settings.buffer_margin,
                        INT, names, 8, NULL);
    audio_set_buffer_margin(global_settings.buffer_margin);
    
    return ret;
}
#else
static bool buffer_margin(void)
{
    return set_int(str(LANG_MP3BUFFER_MARGIN), "s", UNIT_SEC,
                   &global_settings.buffer_margin,
                   audio_set_buffer_margin, 1, 0, 7, NULL );
}
#endif

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

static bool set_party_mode(void)
{
    return set_bool( str(LANG_PARTY_MODE), &global_settings.party_mode );
}

#ifdef CONFIG_BACKLIGHT
static bool set_bl_filter_first_keypress(void)
{
    bool result = set_bool( str(LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS), 
                            &global_settings.bl_filter_first_keypress );
    set_backlight_filter_keypress(global_settings.bl_filter_first_keypress);
    return result;
}
#endif 

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
    return rockbox_browse(WPS_DIR, SHOW_WPS);
}

#ifdef HAVE_REMOTE_LCD
static bool custom_remote_wps_browse(void)
{
    return rockbox_browse(WPS_DIR, SHOW_RWPS);
}
#endif

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

static bool next_folder(void)
{
    return set_bool( str(LANG_NEXT_FOLDER), &global_settings.next_folder );
}

static bool runtimedb(void)
{
    bool rc;
    bool old = global_settings.runtimedb;

    rc = set_bool( str(LANG_RUNTIMEDB_ACTIVE),
                           &global_settings.runtimedb );
    if (old && !global_settings.runtimedb)
        rundb_shutdown();
    if (!old && global_settings.runtimedb)
        rundb_init();

    return rc;
}

static bool codepage_setting(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CODEPAGE_LATIN1)          },
        { STR(LANG_CODEPAGE_GREEK)           },
        { STR(LANG_CODEPAGE_HEBREW)          },
        { STR(LANG_CODEPAGE_RUSSIAN)         },
        { STR(LANG_CODEPAGE_THAI)            },
        { STR(LANG_CODEPAGE_ARABIC)          },
        { STR(LANG_CODEPAGE_TURKISH)         },
        { STR(LANG_CODEPAGE_LATIN_EXTENDED)  },
        { STR(LANG_CODEPAGE_JAPANESE)        },
        { STR(LANG_CODEPAGE_SIMPLIFIED)      },
        { STR(LANG_CODEPAGE_KOREAN)          },
        { STR(LANG_CODEPAGE_TRADITIONAL)     },
        { STR(LANG_CODEPAGE_UTF8)            },
    };
    return set_option(str(LANG_DEFAULT_CODEPAGE),
                      &global_settings.default_codepage,
                      INT, names, 13, set_codepage );
}

#if CONFIG_CODEC == SWCODEC
static bool replaygain(void)
{
    bool result = set_bool(str(LANG_REPLAYGAIN_ENABLE), 
        &global_settings.replaygain);
        
    dsp_set_replaygain(true);
    return result;
}

static bool replaygain_mode(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_TRACK_GAIN) },
        { STR(LANG_ALBUM_GAIN) },
        { STR(LANG_SHUFFLE_GAIN) },
    };
    bool result = set_option(str(LANG_REPLAYGAIN_MODE),
        &global_settings.replaygain_type, INT, names, 3, NULL);

    dsp_set_replaygain(true);
    return result;
}

static bool replaygain_noclip(void)
{
    bool result = set_bool(str(LANG_REPLAYGAIN_NOCLIP), 
        &global_settings.replaygain_noclip);

    dsp_set_replaygain(true);
    return result;
}

void replaygain_preamp_format(char* buffer, int buffer_size, int value, 
    const char* unit)
{
    int v = abs(value);
    
    snprintf(buffer, buffer_size, "%s%d.%d %s", value < 0 ? "-" : "",
        v / 10, v % 10, unit);
}

static bool replaygain_preamp(void)
{
    bool result = set_int(str(LANG_REPLAYGAIN_PREAMP), str(LANG_UNIT_DB), 
        UNIT_DB, &global_settings.replaygain_preamp, NULL, 1, -120, 120, 
        replaygain_preamp_format);

    dsp_set_replaygain(true);
    return result;
}

static bool replaygain_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_REPLAYGAIN_ENABLE), replaygain },
        { ID2P(LANG_REPLAYGAIN_NOCLIP), replaygain_noclip },
        { ID2P(LANG_REPLAYGAIN_MODE), replaygain_mode },
        { ID2P(LANG_REPLAYGAIN_PREAMP), replaygain_preamp },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

static bool crossfade(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_SHUFFLE) },
        { STR(LANG_ALWAYS) },
    };

    bool ret;
    
    ret=set_option( str(LANG_CROSSFADE_ENABLE),
                    &global_settings.crossfade, INT, names, 3, NULL);
    
    audio_set_crossfade(global_settings.crossfade);

    return ret;
}

static bool crossfade_fade_in_delay(void)
{
    bool ret;

    ret = set_int(str(LANG_CROSSFADE_FADE_IN_DELAY), "s", UNIT_SEC,
                   &global_settings.crossfade_fade_in_delay,
                   NULL, 1, 0, 7, NULL );
    audio_set_crossfade(global_settings.crossfade);
    return ret;
}

static bool crossfade_fade_out_delay(void)
{
    bool ret;

    ret = set_int(str(LANG_CROSSFADE_FADE_OUT_DELAY), "s", UNIT_SEC,
                   &global_settings.crossfade_fade_out_delay,
                   NULL, 1, 0, 7, NULL );
    audio_set_crossfade(global_settings.crossfade);
    return ret;
}

static bool crossfade_fade_in_duration(void)
{
    bool ret;

    ret = set_int(str(LANG_CROSSFADE_FADE_IN_DURATION), "s", UNIT_SEC,
                   &global_settings.crossfade_fade_in_duration,
                   NULL, 1, 0, 15, NULL );
    audio_set_crossfade(global_settings.crossfade);
    return ret;
}

static bool crossfade_fade_out_duration(void)
{
    bool ret;

    ret = set_int(str(LANG_CROSSFADE_FADE_OUT_DURATION), "s", UNIT_SEC,
                   &global_settings.crossfade_fade_out_duration,
                   NULL, 1, 0, 15, NULL );
    audio_set_crossfade(global_settings.crossfade);
    return ret;
}

static bool crossfade_fade_out_mixmode(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CROSSFADE) },
        { STR(LANG_MIX) },
    };
    bool ret;
    ret=set_option( str(LANG_CROSSFADE_FADE_OUT_MODE),
                    &global_settings.crossfade_fade_out_mixmode, INT, names, 2, NULL);
                   
    return ret;
}

/**
 * Menu to configure the crossfade settings.
 */
static bool crossfade_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_CROSSFADE_ENABLE), crossfade },
        { ID2P(LANG_CROSSFADE_FADE_IN_DELAY), crossfade_fade_in_delay },
        { ID2P(LANG_CROSSFADE_FADE_IN_DURATION), crossfade_fade_in_duration },
        { ID2P(LANG_CROSSFADE_FADE_OUT_DELAY), crossfade_fade_out_delay },
        { ID2P(LANG_CROSSFADE_FADE_OUT_DURATION), crossfade_fade_out_duration },
        { ID2P(LANG_CROSSFADE_FADE_OUT_MODE), crossfade_fade_out_mixmode },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

static bool beep(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_WEAK) },
        { STR(LANG_MODERATE) },
        { STR(LANG_STRONG) },
    };
    bool ret;
    ret=set_option( str(LANG_BEEP),
                    &global_settings.beep, INT, names, 4, NULL);
        
    return ret;
}
#endif

#ifdef HAVE_DIRCACHE
static bool dircache(void)
{
    bool result = set_bool_options(str(LANG_DIRCACHE_ENABLE),
                                   &global_settings.dircache,
                                   STR(LANG_ON),
                                   STR(LANG_OFF),
                                   NULL);

    if (!dircache_is_enabled() && global_settings.dircache)
        gui_syncsplash(HZ*2, true, str(LANG_DIRCACHE_REBOOT));

    if (!result)
        dircache_disable();
        
    return result;
}

#endif /* HAVE_DIRCACHE */

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
        { ID2P(LANG_PARTY_MODE), set_party_mode },
#if CONFIG_CODEC == SWCODEC
        { ID2P(LANG_CROSSFADE), crossfade_settings_menu },
        { ID2P(LANG_REPLAYGAIN), replaygain_settings_menu },
        { ID2P(LANG_BEEP), beep },
#endif
#ifdef HAVE_SPDIF_POWER
        { ID2P(LANG_SPDIF_ENABLE), spdif },
#endif
        { ID2P(LANG_ID3_ORDER), id3_order },
        { ID2P(LANG_NEXT_FOLDER), next_folder },
        { ID2P(LANG_RUNTIMEDB_ACTIVE), runtimedb },
    };

    bool old_shuffle = global_settings.playlist_shuffle;

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    if ((old_shuffle != global_settings.playlist_shuffle) 
        && (audio_status() & AUDIO_STATUS_PLAY))
    {
#if CONFIG_CODEC == SWCODEC
        dsp_set_replaygain(true);
#endif

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
    unsigned char *lines[]={str(LANG_RESET_ASK_RECORDER)};
    unsigned char *yes_lines[]={
        str(LANG_RESET_DONE_SETTING),
        str(LANG_RESET_DONE_CLEAR)
    };
    unsigned char *no_lines[]={yes_lines[0], str(LANG_RESET_DONE_CANCEL)};
    struct text_message message={(char **)lines, 1};
    struct text_message yes_message={(char **)yes_lines, 2};
    struct text_message no_message={(char **)no_lines, 2};

    switch(gui_syncyesno_run(&message, &yes_message, &no_message))
    {
        case YESNO_YES:
            settings_reset();
            settings_apply();
            break;
        case YESNO_NO:
            break;
        case YESNO_USB:
            return true;
    }
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
        { ID2P(LANG_SCROLL_SPEED),        scroll_speed       },
        { ID2P(LANG_SCROLL_DELAY),        scroll_delay       },
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_SCROLL_STEP),         scroll_step        },
#endif
        { ID2P(LANG_BIDIR_SCROLL),        bidir_limit        },
#ifdef HAVE_LCD_CHARCELLS
        { ID2P(LANG_JUMP_SCROLL),         jump_scroll        },
        { ID2P(LANG_JUMP_SCROLL_DELAY),   jump_scroll_delay  },
#endif
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_SCREEN_SCROLL_VIEW),  screen_scroll      },
        { ID2P(LANG_SCREEN_SCROLL_STEP),  screen_scroll_step },
#endif
        { ID2P(LANG_SCROLL_PAGINATED),    scroll_paginated   },
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
        { ID2P(LANG_BACKLIGHT_ON_WHEN_CHARGING), backlight_timer_plugged },
#endif
        { ID2P(LANG_CAPTION_BACKLIGHT), caption_backlight },
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
        { ID2P(LANG_BACKLIGHT_FADE_IN), backlight_fade_in },
        { ID2P(LANG_BACKLIGHT_FADE_OUT), backlight_fade_out },
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
        { ID2P(LANG_BRIGHTNESS), brightness },
#endif
        { ID2P(LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS), set_bl_filter_first_keypress },
#endif /* CONFIG_BACKLIGHT */
        { ID2P(LANG_CONTRAST),        contrast },
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_INVERT),          invert },
        { ID2P(LANG_FLIP_DISPLAY),    flip_display },
        { ID2P(LANG_INVERT_CURSOR),   invert_cursor },
#endif
#ifdef HAVE_LCD_COLOR
        { ID2P(LANG_CLEAR_BACKDROP),   clear_main_backdrop },
        { ID2P(LANG_BACKGROUND_COLOR), set_bg_color },
        { ID2P(LANG_FOREGROUND_COLOR), set_fg_color },
        { ID2P(LANG_RESET_COLORS),     reset_color },
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
        { ID2P(LANG_BACKLIGHT),       remote_backlight_timer },
#ifdef HAVE_CHARGING
        { ID2P(LANG_BACKLIGHT_ON_WHEN_CHARGING),
                                      remote_backlight_timer_plugged },
#endif
        { ID2P(LANG_CAPTION_BACKLIGHT), remote_caption_backlight },
        { ID2P(LANG_CONTRAST),        remote_contrast },
        { ID2P(LANG_INVERT),          remote_invert },
        { ID2P(LANG_FLIP_DISPLAY),    remote_flip_display },
#ifdef HAVE_REMOTE_LCD_TICKING
        { ID2P(LANG_REDUCE_TICKING),  remote_reduce_ticking },
#endif
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
#ifdef HAVE_REMOTE_LCD
        { ID2P(LANG_REMOTE_WHILE_PLAYING),   custom_remote_wps_browse },
#endif
        { ID2P(LANG_LCD_MENU),        lcd_settings_menu },
#ifdef HAVE_REMOTE_LCD
        { ID2P(LANG_LCD_REMOTE_MENU), lcd_remote_settings_menu },
#endif
        { ID2P(LANG_SCROLL_MENU),     scroll_settings_menu },
#ifdef HAVE_LCD_BITMAP
        { ID2P(LANG_BARS_MENU),       bars_settings_menu },
        { ID2P(LANG_PM_MENU),         peak_meter_menu },
#endif
        { ID2P(LANG_DEFAULT_CODEPAGE),    codepage_setting },
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
#ifdef HAVE_DIRCACHE
        { ID2P(LANG_DIRCACHE_ENABLE),  dircache },
#endif
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif /* !HAVE_MMC */

#ifdef CONFIG_RTC
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

bool manage_settings_menu(void)
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
#ifdef CONFIG_RTC
        { ID2P(LANG_TIME_MENU),        time_settings_menu     },
#endif
        { ID2P(LANG_POWEROFF_IDLE),    poweroff_idle_timer    },
        { ID2P(LANG_SLEEP_TIMER),      sleep_timer            },
#ifdef HAVE_ALARM_MOD
        { ID2P(LANG_ALARM_MOD_ALARM_MENU), alarm_screen       },
#endif
        { ID2P(LANG_LIMITS_MENU),      limits_settings_menu   },
#if CONFIG_CODEC == MAS3507D
        { ID2P(LANG_LINE_IN),          line_in                },
#endif
#ifdef HAVE_CHARGING
        { ID2P(LANG_CAR_ADAPTER_MODE), car_adapter_mode       },
#endif
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
