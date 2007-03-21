/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "settings_menu.h"
#include "tree.h"
#include "list.h"
#ifdef HAVE_LCD_BITMAP
#include "backdrop.h"
#include "peakmeter.h"
#endif
#include "talk.h"
#include "color_picker.h"
#include "lcd.h"
#include "lcd-remote.h"


#if CONFIG_BACKLIGHT
int filterfirstkeypress_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM:
            set_backlight_filter_keypress(global_settings.bl_filter_first_keypress);
#ifdef HAVE_REMOTE_LCD
            set_remote_backlight_filter_keypress(
                                global_settings.remote_bl_filter_first_keypress);
#endif
        
            break;
    }
    return action;
}
#endif
#ifdef HAVE_LCD_BITMAP
int flipdisplay_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM:
            button_set_flip(global_settings.flip_display);
            lcd_set_flip(global_settings.flip_display);
#ifdef HAVE_REMOTE_LCD
            lcd_remote_set_flip(global_settings.remote_flip_display);
            lcd_remote_update();
#endif
            break;
    }
    return action;
}
#endif

/***********************************/
/*    LCD MENU                     */
#if CONFIG_BACKLIGHT
MENUITEM_SETTING(backlight_timeout, &global_settings.backlight_timeout, NULL);
MENUITEM_SETTING(backlight_in_plugins, &global_settings.backlight_in_plugins, NULL);
#if CONFIG_CHARGING
MENUITEM_SETTING(backlight_timeout_plugged, 
                &global_settings.backlight_timeout_plugged, NULL);
#endif
#ifdef HAS_BUTTON_HOLD
MENUITEM_SETTING(backlight_on_button_hold, 
                &global_settings.backlight_on_button_hold, NULL);
#endif
MENUITEM_SETTING(caption_backlight, &global_settings.caption_backlight, NULL);
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
MENUITEM_SETTING(backlight_fade_in, &global_settings.backlight_fade_in, NULL);
MENUITEM_SETTING(backlight_fade_out, &global_settings.backlight_fade_out, NULL);
#endif
MENUITEM_SETTING(bl_filter_first_keypress, 
                    &global_settings.bl_filter_first_keypress, 
                    filterfirstkeypress_callback);
#ifdef HAVE_LCD_SLEEP
MENUITEM_SETTING(lcd_sleep_after_backlight_off,
                &global_settings.lcd_sleep_after_backlight_off, NULL);
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
MENUITEM_SETTING(brightness_item, &global_settings.brightness, NULL);
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_LCD_CONTRAST
MENUITEM_SETTING(contrast, &global_settings.contrast, NULL);
#endif
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_LCD_INVERT
MENUITEM_SETTING(invert, &global_settings.invert, NULL);
#endif
#ifdef HAVE_LCD_FLIP
MENUITEM_SETTING(flip_display, &global_settings.flip_display, flipdisplay_callback);
#endif
MENUITEM_SETTING(invert_cursor, &global_settings.invert_cursor, NULL);
#endif /* HAVE_LCD_BITMAP */
#ifdef HAVE_LCD_COLOR
/**
* Menu to clear the backdrop image
 */
static int clear_main_backdrop(void)
{
    global_settings.backdrop_file[0]=0;
    unload_main_backdrop();
    show_main_backdrop();
    settings_save();
    return 0;
}

/**
 * Menu for fore/back colors
 */
static int set_fg_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_FOREGROUND_COLOR),
                     &global_settings.fg_color,global_settings.bg_color);

    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    return res;
}

static int set_bg_color(void)
{
    int res;
    res = (int)set_color(&screens[SCREEN_MAIN],str(LANG_BACKGROUND_COLOR),
                     &global_settings.bg_color,global_settings.fg_color);

    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
    return res;
}
static int reset_color(void)
{
    global_settings.fg_color = LCD_DEFAULT_FG;
    global_settings.bg_color = LCD_DEFAULT_BG;

    screens[SCREEN_MAIN].set_foreground(global_settings.fg_color);
    screens[SCREEN_MAIN].set_background(global_settings.bg_color);
    return 0;
}
MENUITEM_FUNCTION(clear_main_bd, 0, ID2P(LANG_CLEAR_BACKDROP),
                    clear_main_backdrop, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_bg_col, 0, ID2P(LANG_BACKGROUND_COLOR),
                    set_bg_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_fg_col, 0, ID2P(LANG_FOREGROUND_COLOR),
                    set_fg_color, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(reset_colors, 0, ID2P(LANG_RESET_COLORS),
                    reset_color, NULL, NULL, Icon_NOICON);
#endif

/* now the actual menu */
MAKE_MENU(lcd_settings,ID2P(LANG_LCD_MENU),
            NULL, Icon_Display_menu
#if CONFIG_BACKLIGHT
            ,&backlight_timeout
# if CONFIG_CHARGING
            ,&backlight_timeout_plugged
# endif
# ifdef HAS_BUTTON_HOLD
            ,&backlight_on_button_hold
# endif
            ,&caption_backlight, &backlight_in_plugins
# if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
            ,&backlight_fade_in, &backlight_fade_out
# endif
            ,&bl_filter_first_keypress
# ifdef HAVE_LCD_SLEEP
            ,&lcd_sleep_after_backlight_off
# endif
# ifdef HAVE_BACKLIGHT_BRIGHTNESS
            ,&brightness_item
# endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_LCD_CONTRAST
            ,&contrast
#endif
#ifdef HAVE_LCD_BITMAP
# ifdef HAVE_LCD_INVERT
            ,&invert
# endif
# ifdef HAVE_LCD_FLIP
            ,&flip_display
# endif
            ,&invert_cursor
#endif /* HAVE_LCD_BITMAP */
#ifdef HAVE_LCD_COLOR
            ,&clear_main_bd, &set_bg_col, &set_fg_col, &reset_colors
#endif
         );
/*    LCD MENU                    */
/***********************************/


/********************************/
/* Remote LCD settings menu     */
#ifdef HAVE_REMOTE_LCD
MENUITEM_SETTING(remote_backlight_timeout, 
                    &global_settings.remote_backlight_timeout, NULL);

#if CONFIG_CHARGING
MENUITEM_SETTING(remote_backlight_timeout_plugged, 
                    &global_settings.remote_backlight_timeout_plugged, NULL);
#endif

#ifdef HAS_REMOTE_BUTTON_HOLD
MENUITEM_SETTING(remote_backlight_on_button_hold, 
                    &global_settings.remote_backlight_on_button_hold, NULL);
#endif

MENUITEM_SETTING(remote_caption_backlight, 
                    &global_settings.remote_caption_backlight, NULL);
MENUITEM_SETTING(remote_bl_filter_first_keypress, 
                    &global_settings.remote_bl_filter_first_keypress, 
                    filterfirstkeypress_callback);
MENUITEM_SETTING(remote_contrast, 
                    &global_settings.remote_contrast, NULL);
MENUITEM_SETTING(remote_invert, 
                    &global_settings.remote_invert, NULL);
                    
MENUITEM_SETTING(remote_flip_display, 
                    &global_settings.remote_flip_display, flipdisplay_callback);

#ifdef HAVE_REMOTE_LCD_TICKING
int ticking_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM:
            lcd_remote_emireduce(global_settings.remote_reduce_ticking);
            break;
    }
    return action;
}
MENUITEM_SETTING(remote_reduce_ticking, 
                    &global_settings.remote_reduce_ticking, ticking_callback);
#endif

MAKE_MENU(lcd_remote_settings, ID2P(LANG_LCD_REMOTE_MENU),
            NULL, Icon_Remote_Display_menu,
            &remote_backlight_timeout,
#if CONFIG_CHARGING
            &remote_backlight_timeout_plugged,
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
            &remote_backlight_on_button_hold,
#endif
            &remote_caption_backlight, &remote_bl_filter_first_keypress,
            &remote_contrast, &remote_invert, &remote_flip_display
#ifdef HAVE_REMOTE_LCD_TICKING
            ,&remote_reduce_ticking
#endif
         );

#endif /* HAVE_REMOTE_LCD */
/* Remote LCD settings menu     */
/********************************/

/***********************************/
/*    SCROLL MENU                  */
MENUITEM_SETTING_W_TEXT(scroll_speed, &global_settings.scroll_speed,
                         ID2P(LANG_SCROLL), NULL);
MENUITEM_SETTING(scroll_delay, &global_settings.scroll_delay, NULL);
#ifdef HAVE_LCD_BITMAP
MENUITEM_SETTING_W_TEXT(scroll_step, &global_settings.scroll_step,
                        ID2P(LANG_SCROLL_STEP_EXAMPLE), NULL);
#endif
MENUITEM_SETTING(bidir_limit, &global_settings.bidir_limit, NULL);
#ifdef HAVE_REMOTE_LCD
MENUITEM_SETTING_W_TEXT(remote_scroll_speed, &global_settings.remote_scroll_speed,
                         ID2P(LANG_SCROLL), NULL);
MENUITEM_SETTING(remote_scroll_delay, &global_settings.remote_scroll_delay, NULL);
MENUITEM_SETTING_W_TEXT(remote_scroll_step, &global_settings.remote_scroll_step,
                        ID2P(LANG_SCROLL_STEP_EXAMPLE), NULL);
MENUITEM_SETTING(remote_bidir_limit, &global_settings.remote_bidir_limit, NULL);
MAKE_MENU(remote_scroll_sets, ID2P(LANG_REMOTE_SCROLL_SETS), 0, Icon_NOICON,
          &remote_scroll_speed, &remote_scroll_delay,
          &remote_scroll_step, &remote_bidir_limit);
#endif /* HAVE_REMOTE_LCD */
#ifdef HAVE_LCD_CHARCELLS
MENUITEM_SETTING(jump_scroll, &global_settings.jump_scroll, NULL);
MENUITEM_SETTING(jump_scroll_delay, &global_settings.jump_scroll_delay, NULL);
#endif
#ifdef HAVE_LCD_BITMAP
int screenscroll_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM:
            gui_list_screen_scroll_out_of_view(global_settings.offset_out_of_view);
            break;
    }
    return action;
}
MENUITEM_SETTING(offset_out_of_view, &global_settings.offset_out_of_view,
                 screenscroll_callback);
MENUITEM_SETTING(screen_scroll_step, &global_settings.screen_scroll_step, NULL);
#endif
MENUITEM_SETTING(scroll_paginated, &global_settings.scroll_paginated, NULL);

MAKE_MENU(scroll_settings_menu, ID2P(LANG_SCROLL_MENU), 0, Icon_NOICON,
          &scroll_speed, &scroll_delay,
#ifdef HAVE_LCD_BITMAP
          &scroll_step,
#endif
          &bidir_limit,
#ifdef HAVE_REMOTE_LCD
          &remote_scroll_sets,
#endif
#ifdef HAVE_LCD_CHARCELLS
          &jump_scroll, &jump_scroll_delay,
#endif
#ifdef HAVE_LCD_BITMAP
          &offset_out_of_view, &screen_scroll_step,
#endif
          &scroll_paginated
          );
/*    SCROLL MENU                  */
/***********************************/

/***********************************/
/*    BARS MENU                    */
#ifdef HAVE_LCD_BITMAP
MENUITEM_SETTING(scrollbar_item, &global_settings.scrollbar, NULL);
MENUITEM_SETTING(statusbar, &global_settings.statusbar, NULL);
#if CONFIG_KEYPAD == RECORDER_PAD
MENUITEM_SETTING(buttonbar, &global_settings.buttonbar, NULL);
#endif
MENUITEM_SETTING(volume_type, &global_settings.volume_type, NULL);
MENUITEM_SETTING(battery_display, &global_settings.battery_display, NULL);
MAKE_MENU(bars_menu, ID2P(LANG_BARS_MENU), 0, Icon_NOICON,
          &scrollbar_item, &statusbar,
#if CONFIG_KEYPAD == RECORDER_PAD
          &buttonbar,
#endif
          &volume_type, &battery_display);
#endif /* HAVE_LCD_BITMAP */
/*    BARS MENU                    */
/***********************************/


/***********************************/
/*    PEAK METER MENU              */

#ifdef HAVE_LCD_BITMAP
int peakmeter_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM:
            peak_meter_init_times(global_settings.peak_meter_release,
                                    global_settings.peak_meter_hold,
                                    global_settings.peak_meter_clip_hold);
            break;
    }
    return action;
}
MENUITEM_SETTING(peak_meter_hold, 
                 &global_settings.peak_meter_hold, peakmeter_callback);
MENUITEM_SETTING(peak_meter_clip_hold, 
                 &global_settings.peak_meter_clip_hold, peakmeter_callback);

MENUITEM_SETTING(peak_meter_release, 
                 &global_settings.peak_meter_release, peakmeter_callback);
/**
 * Menu to select wether the scale of the meter
 * displays dBfs of linear values.
 */
static int peak_meter_scale(void) {
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
static int peak_meter_min(void) {
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
static int peak_meter_max(void) {
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
MENUITEM_FUNCTION(peak_meter_scale_item, 0, ID2P(LANG_PM_SCALE),
                    peak_meter_scale, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(peak_meter_min_item, 0, ID2P(LANG_PM_MIN), 
                    peak_meter_min, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(peak_meter_max_item, 0, ID2P(LANG_PM_MAX), 
                    peak_meter_max, NULL, NULL, Icon_NOICON);
MAKE_MENU(peak_meter_menu, ID2P(LANG_PM_MENU), NULL, Icon_NOICON,
          &peak_meter_release, &peak_meter_hold, 
          &peak_meter_clip_hold,
          &peak_meter_scale_item, &peak_meter_min_item, &peak_meter_max_item);
#endif /*  HAVE_LCD_BITMAP */
/*    PEAK METER MENU              */
/***********************************/



struct browse_folder_info {
    const char* dir;
    int show_options;
};
#ifdef HAVE_LCD_BITMAP
static struct browse_folder_info fonts = {FONT_DIR, SHOW_FONT};
#endif
static struct browse_folder_info wps = {WPS_DIR, SHOW_WPS};
#ifdef HAVE_REMOTE_LCD
static struct browse_folder_info rwps = {WPS_DIR, SHOW_RWPS};
#endif

static int browse_folder(void *param)
{
    const struct browse_folder_info *info =
        (const struct browse_folder_info*)param;
    return rockbox_browse(info->dir, info->show_options);
}

#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(browse_fonts, MENU_FUNC_USEPARAM, 
        ID2P(LANG_CUSTOM_FONT), 
        browse_folder, (void*)&fonts, NULL, Icon_NOICON);
#endif
MENUITEM_FUNCTION(browse_wps, MENU_FUNC_USEPARAM, 
        ID2P(LANG_WHILE_PLAYING), 
        browse_folder, (void*)&wps, NULL, Icon_NOICON);
#ifdef HAVE_REMOTE_LCD
MENUITEM_FUNCTION(browse_rwps, MENU_FUNC_USEPARAM, 
        ID2P(LANG_REMOTE_WHILE_PLAYING), 
        browse_folder, (void*)&rwps, NULL, Icon_NOICON);
#endif

MENUITEM_SETTING(show_icons, &global_settings.show_icons, NULL);
MENUITEM_SETTING(codepage_setting, &global_settings.default_codepage, NULL);


MAKE_MENU(display_menu, ID2P(LANG_DISPLAY),
            NULL, Icon_Display_menu,
#ifdef HAVE_LCD_BITMAP
            &browse_fonts,
#endif
            &browse_wps,
#ifdef HAVE_REMOTE_LCD
            &browse_rwps,
#endif
            &lcd_settings,
#ifdef HAVE_REMOTE_LCD
            &lcd_remote_settings,
#endif
            &show_icons, &scroll_settings_menu,
#ifdef HAVE_LCD_BITMAP
            &bars_menu, &peak_meter_menu,
#endif
            &codepage_setting,
            );
