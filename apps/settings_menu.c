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
#include "dir.h"
#include "dircache.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#include "tagtree.h"
#endif
#include "rbunicode.h"
#include "splash.h"
#include "yesno.h"
#include "list.h"
#include "color_picker.h"
#include "scrobbler.h"

#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
#endif
#include "lang.h"
#if CONFIG_CODEC == MAS3507D
void dac_line_in(bool enable);
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#include "pcm_playback.h"
#include "dsp.h"
#endif

#if LCD_DEPTH > 1
#include "backdrop.h"
#endif
#include "menus/exported_menus.h"


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
                    lcd_remote_set_contrast, 1, MIN_REMOTE_CONTRAST_SETTING,
                    MAX_REMOTE_CONTRAST_SETTING, NULL );
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

#ifdef CONFIG_CHARGING
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

#ifdef HAS_BUTTON_HOLD
static bool backlight_on_button_hold(void)
{
    static const struct opt_items names[3] = {
        { STR(LANG_BACKLIGHT_ON_BUTTON_HOLD_NORMAL) },
        { STR(LANG_OFF) },
        { STR(LANG_ON) },
    };
    return set_option(str(LANG_BACKLIGHT_ON_BUTTON_HOLD),
                      &global_settings.backlight_on_button_hold,
                      INT, names, 3,
                      backlight_set_on_button_hold);
}
#endif /* HAS_BUTTON_HOLD */

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

#ifdef HAVE_LCD_SLEEP
static bool lcd_sleep_after_backlight_off(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_ALWAYS) },
        { STR(LANG_NEVER) },
        { (unsigned char *)"5s", TALK_ID(5, UNIT_SEC) },
        { (unsigned char *)"10s", TALK_ID(10, UNIT_SEC) },
        { (unsigned char *)"15s", TALK_ID(15, UNIT_SEC) },
        { (unsigned char *)"20s", TALK_ID(20, UNIT_SEC) },
        { (unsigned char *)"30s", TALK_ID(30, UNIT_SEC) },
        { (unsigned char *)"45s", TALK_ID(45, UNIT_SEC) },
        { (unsigned char *)"60s", TALK_ID(60, UNIT_SEC) },
        { (unsigned char *)"90s", TALK_ID(90, UNIT_SEC) },
    };

    return set_option(str(LANG_LCD_SLEEP_AFTER_BACKLIGHT_OFF),
                      &global_settings.lcd_sleep_after_backlight_off,
                      INT, names, 10,
                      lcd_set_sleep_after_backlight_off );
}
#endif /* HAVE_LCD_SLEEP */
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

#ifdef CONFIG_CHARGING
static bool remote_backlight_timer_plugged(void)
{
    return set_option((char *)str(LANG_BACKLIGHT_ON_WHEN_CHARGING),
                      &global_settings.remote_backlight_timeout_plugged,
                      INT, backlight_timeouts, 19,
                      remote_backlight_set_timeout_plugged );
}
#endif /* HAVE_REMOTE_LCD */

static bool remote_caption_backlight(void)
{
    return set_bool((char *)str(LANG_CAPTION_BACKLIGHT),
                    &global_settings.remote_caption_backlight);
}

#ifdef HAS_REMOTE_BUTTON_HOLD
static bool remote_backlight_on_button_hold(void)
{
    static const struct opt_items names[3] = {
        { STR(LANG_BACKLIGHT_ON_BUTTON_HOLD_NORMAL) },
        { STR(LANG_OFF) },
        { STR(LANG_ON) },
    };
    return set_option(str(LANG_BACKLIGHT_ON_BUTTON_HOLD),
                      &global_settings.remote_backlight_on_button_hold,
                      INT, names, 3,
                      remote_backlight_set_on_button_hold);
}
#endif /* HAS_BUTTON_HOLD */

#endif /* HAVE_REMOTE_LCD */

#ifdef HAVE_LCD_CONTRAST
static bool contrast(void)
{
    return set_int( str(LANG_CONTRAST), "", UNIT_INT,
                    &global_settings.contrast,
                    lcd_set_contrast, 1, MIN_CONTRAST_SETTING,
                    MAX_CONTRAST_SETTING, NULL );
}
#endif /* HAVE_LCD_CONTRAST */

#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_LCD_INVERT
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
#endif /* HAVE_LCD_INVERT */

#ifdef HAVE_LCD_FLIP
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
#endif /* HAVE_LCD_FLIP */

/**
 * Menu to set Line Selector Type (Pointer/Bar)
 */
static bool invert_cursor(void)
{
    bool type = global_settings.invert_cursor;
    bool rc = set_bool_options(str(LANG_INVERT_CURSOR),
                            &type,
                            STR(LANG_INVERT_CURSOR_BAR),
                            STR(LANG_INVERT_CURSOR_POINTER),
                            NULL);
    global_settings.invert_cursor = type;
    return rc;
}

#if LCD_DEPTH > 1
/**
 * Menu to clear the backdrop image
 */
static bool clear_main_backdrop(void)
{
    global_settings.backdrop_file[0]=0;
    unload_main_backdrop();
    show_main_backdrop();
    return false;
}
#endif

#ifdef HAVE_USB_POWER
#ifdef CONFIG_CHARGING
#include "usb.h"
#endif
#endif

#ifdef HAVE_LCD_COLOR
/**
 * Menu for fore/back colors
 */
static bool set_fg_color(void)
{
    bool res;

    res = set_color(NULL,str(LANG_FOREGROUND_COLOR),
                    &global_settings.fg_color,global_settings.bg_color);

    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);

    return res;
}

static bool set_bg_color(void)
{
    bool res;

    res = set_color(NULL,str(LANG_BACKGROUND_COLOR),
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

#ifdef HAVE_REMOTE_LCD
static bool remote_scroll_speed(void)
{
    return set_int(str(LANG_SCROLL), "", UNIT_INT,
                   &global_settings.remote_scroll_speed,
                   &lcd_remote_scroll_speed, 1, 0, 15, NULL );
}

static bool remote_scroll_step(void)
{
    return set_int(str(LANG_SCROLL_STEP_EXAMPLE), str(LANG_PIXELS), UNIT_PIXEL,
                   &global_settings.remote_scroll_step,
                   &lcd_remote_scroll_step, 1, 1, LCD_WIDTH, NULL );
}

static bool remote_scroll_delay(void)
{
    int dummy = global_settings.remote_scroll_delay * (HZ/10);
    int rc = set_int(str(LANG_SCROLL_DELAY), "ms", UNIT_MS,
                     &dummy,
                     &lcd_remote_scroll_delay, 100, 0, 2500, NULL );
    global_settings.remote_scroll_delay = dummy / (HZ/10);
    return rc;
}

static bool remote_bidir_limit(void)
{
    return set_int(str(LANG_BIDIR_SCROLL), "%", UNIT_PERCENT,
                   &global_settings.remote_bidir_limit,
                   &lcd_remote_bidir_scroll, 25, 0, 200, NULL );
}

#endif

#ifdef HAVE_LCD_BITMAP
static bool screen_scroll(void)
{
    bool rc = set_bool( str(LANG_SCREEN_SCROLL_VIEW), &global_settings.offset_out_of_view);
    gui_list_screen_scroll_out_of_view(global_settings.offset_out_of_view);
    return rc;
}

static bool screen_scroll_step(void)
{
    return set_int(str(LANG_SCREEN_SCROLL_STEP), str(LANG_PIXELS), UNIT_PIXEL,
                   &global_settings.screen_scroll_step,
                   &gui_list_screen_scroll_step, 1, 1, LCD_WIDTH, NULL );
}

static bool scroll_step(void)
{
    return set_int(str(LANG_SCROLL_STEP_EXAMPLE), str(LANG_PIXELS), UNIT_PIXEL,
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


#ifdef CONFIG_BACKLIGHT
static bool set_bl_filter_first_keypress(void)
{
    bool result = set_bool( str(LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS),
                            &global_settings.bl_filter_first_keypress );
    set_backlight_filter_keypress(global_settings.bl_filter_first_keypress);
    return result;
}
#ifdef HAVE_REMOTE_LCD
static bool set_remote_bl_filter_first_keypress(void)
{
    bool result = set_bool( str(LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS),
                            &global_settings.remote_bl_filter_first_keypress );
    set_remote_backlight_filter_keypress(global_settings.remote_bl_filter_first_keypress);
    return result;
}
#endif
#endif

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

#ifdef HAVE_LCD_BITMAP
static bool font_browse(void)
{
    return rockbox_browse(FONT_DIR, SHOW_FONT);
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

static bool codepage_setting(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CODEPAGE_LATIN1)          },
        { STR(LANG_CODEPAGE_GREEK)           },
        { STR(LANG_CODEPAGE_HEBREW)          },
        { STR(LANG_CODEPAGE_CYRILLIC)        },
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
            settings_save();
            break;
        case YESNO_NO:
            break;
        case YESNO_USB:
            return true;
    }
    return false;
}


#ifdef HAVE_REMOTE_LCD
static bool remote_scroll_sets(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SCROLL_SPEED),        remote_scroll_speed },
        { ID2P(LANG_SCROLL_DELAY),        remote_scroll_delay },
        { ID2P(LANG_SCROLL_STEP),         remote_scroll_step  },
        { ID2P(LANG_BIDIR_SCROLL),        remote_bidir_limit  },
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
#endif

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
#ifdef HAVE_REMOTE_LCD
        { ID2P(LANG_REMOTE_SCROLL_SETS),  remote_scroll_sets },
#endif
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
#ifdef CONFIG_CHARGING
        { ID2P(LANG_BACKLIGHT_ON_WHEN_CHARGING), backlight_timer_plugged },
#endif
#ifdef HAS_BUTTON_HOLD
        { ID2P(LANG_BACKLIGHT_ON_BUTTON_HOLD), backlight_on_button_hold },
#endif
        { ID2P(LANG_CAPTION_BACKLIGHT), caption_backlight },
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
        { ID2P(LANG_BACKLIGHT_FADE_IN), backlight_fade_in },
        { ID2P(LANG_BACKLIGHT_FADE_OUT), backlight_fade_out },
#endif
        { ID2P(LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS), set_bl_filter_first_keypress },
#ifdef HAVE_LCD_SLEEP
        { ID2P(LANG_LCD_SLEEP_AFTER_BACKLIGHT_OFF), lcd_sleep_after_backlight_off },
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
        { ID2P(LANG_BRIGHTNESS),      brightness },
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_LCD_CONTRAST
        { ID2P(LANG_CONTRAST),        contrast },
#endif
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_LCD_INVERT
        { ID2P(LANG_INVERT),          invert },
#endif
#ifdef HAVE_LCD_FLIP
        { ID2P(LANG_FLIP_DISPLAY),    flip_display },
#endif
        { ID2P(LANG_INVERT_CURSOR),   invert_cursor },
#endif
#if LCD_DEPTH > 1
        { ID2P(LANG_CLEAR_BACKDROP),   clear_main_backdrop },
#endif
#ifdef HAVE_LCD_COLOR
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
#ifdef CONFIG_CHARGING
        { ID2P(LANG_BACKLIGHT_ON_WHEN_CHARGING),
                                      remote_backlight_timer_plugged },
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
        { ID2P(LANG_BACKLIGHT_ON_BUTTON_HOLD), remote_backlight_on_button_hold },
#endif
        { ID2P(LANG_CAPTION_BACKLIGHT), remote_caption_backlight },
        { ID2P(LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS), set_remote_bl_filter_first_keypress },
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


bool display_settings_menu(void)
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
static bool manage_settings_write_config(void)
{
	return settings_save_config(SETTINGS_SAVE_ALL);
}
static bool manage_settings_write_theme(void)
{
	return settings_save_config(SETTINGS_SAVE_THEME);
}

bool manage_settings_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_CUSTOM_CFG),      custom_cfg_browse },
        { ID2P(LANG_RESET),           reset_settings },
        { ID2P(LANG_SAVE_SETTINGS),   manage_settings_write_config},
        { ID2P(LANG_SAVE_THEME),      manage_settings_write_theme},
    };

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}

