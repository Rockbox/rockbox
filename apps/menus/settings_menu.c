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
#include <string.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "sound_menu.h"
#include "exported_menus.h"
#include "tree.h"
#include "tagtree.h"
#include "usb.h"
#include "splash.h"
#include "talk.h"
#include "sprintf.h"
#include "powermgmt.h"
#ifdef HAVE_ALARM_MOD
#include "alarm_menu.h"
#endif

/***********************************/
/*    TAGCACHE MENU                */
#ifdef HAVE_TAGCACHE
#ifdef HAVE_TC_RAMCACHE
MENUITEM_SETTING(tagcache_ram, &global_settings.tagcache_ram, NULL);
#endif
MENUITEM_SETTING(tagcache_autoupdate, &global_settings.tagcache_autoupdate, NULL);
MENUITEM_FUNCTION(tc_init, ID2P(LANG_TAGCACHE_FORCE_UPDATE),
                    (int(*)(void))tagcache_rebuild, NULL, NOICON);
MENUITEM_FUNCTION(tc_update, ID2P(LANG_TAGCACHE_UPDATE),
                    (int(*)(void))tagcache_update, NULL, NOICON);
MENUITEM_SETTING(runtimedb, &global_settings.runtimedb, NULL);
MENUITEM_FUNCTION(tc_export, ID2P(LANG_TAGCACHE_EXPORT),
                    (int(*)(void))tagtree_export, NULL, NOICON);
MENUITEM_FUNCTION(tc_import, ID2P(LANG_TAGCACHE_IMPORT),
                    (int(*)(void))tagtree_import, NULL, NOICON);
MAKE_MENU(tagcache_menu, ID2P(LANG_TAGCACHE), 0, NOICON,
#ifdef HAVE_TC_RAMCACHE
                &tagcache_ram,
#endif
                &tagcache_autoupdate, &tc_init, &tc_update, &runtimedb,
                &tc_export, &tc_import);
#endif /* HAVE_TAGCACHE */
/*    TAGCACHE MENU                */
/***********************************/

/***********************************/
/*    FILE VIEW MENU               */
static int fileview_callback(int action,const struct menu_item_ex *this_item);
MENUITEM_SETTING(sort_case, &global_settings.sort_case, NULL);
MENUITEM_SETTING(sort_dir, &global_settings.sort_dir, fileview_callback);
MENUITEM_SETTING(sort_file, &global_settings.sort_file, fileview_callback);
MENUITEM_SETTING(dirfilter, &global_settings.dirfilter, NULL);
MENUITEM_SETTING(browse_current, &global_settings.browse_current, NULL);
MENUITEM_SETTING(show_path_in_browser, &global_settings.show_path_in_browser, NULL);
static int fileview_callback(int action,const struct menu_item_ex *this_item)
{
    static int oldval;
    int *variable = this_item->variable;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM: /* on entering an item */
            oldval = *variable;
            break;
        case ACTION_EXIT_MENUITEM: /* on exit */
            if (*variable != oldval)
                reload_directory(); /* force reload if this has changed */
            break;
    }
    return action;
}

MAKE_MENU(file_menu, ID2P(LANG_FILE), 0, NOICON,
                &sort_case, &sort_dir, &sort_file,
                &dirfilter, &browse_current, &show_path_in_browser,
#ifdef HAVE_TAGCACHE
                &tagcache_menu
#endif
        );
/*    FILE VIEW MENU               */
/***********************************/


/***********************************/
/*    SYSTEM MENU                  */

/* Battery */
#ifndef SIMULATOR
MENUITEM_SETTING(battery_capacity, &global_settings.battery_capacity, NULL);
MENUITEM_SETTING(battery_type, &global_settings.battery_type, NULL);
#ifdef HAVE_USB_POWER
#ifdef CONFIG_CHARGING
static int usbcharging_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            usb_charging_enable(global_settings.usb_charging);
            break;
    }
    return action;
}
MENUITEM_SETTING(usb_charging, &global_settings.usb_charging, usbcharging_callback);
#endif
#endif
MAKE_MENU(battery_menu, ID2P(LANG_BATTERY_MENU), 0, NOICON,
          &battery_capacity,
#if BATTERY_TYPES_COUNT > 1
            &battery_type,
#endif
#ifdef HAVE_USB_POWER
#ifdef CONFIG_CHARGING
            &usb_charging,
#endif
#endif
         );
#endif /* SIMULATOR */
/* Disk */
#ifndef HAVE_MMC
MENUITEM_SETTING(disk_spindown, &global_settings.disk_spindown, NULL);
#ifdef HAVE_DIRCACHE
static int dircache_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            switch (global_settings.dircache)
            {
                case true:
                    if (!dircache_is_enabled())
                        gui_syncsplash(HZ*2, true, str(LANG_PLEASE_REBOOT));
                    break;
                case false:
                    if (dircache_is_enabled())
                        dircache_disable();
                    break;
            }
            break;
    }
    return action;
}
MENUITEM_SETTING(dircache, &global_settings.dircache, dircache_callback);
#endif
MAKE_MENU(disk_menu, ID2P(LANG_DISK_MENU), 0, NOICON,
          &disk_spindown,
#ifdef HAVE_DIRCACHE
            &dircache,
#endif
         );
#endif

/* Time & Date */
#ifdef CONFIG_RTC
static int timedate_set(void)
{
    struct tm tm;
    int result;

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

    result = (int)set_time_screen(str(LANG_TIME), &tm);

    if(tm.tm_year != -1) {
        set_time(&tm);
    }
    return result;
}

MENUITEM_FUNCTION(time_set, ID2P(LANG_TIME), timedate_set, NULL, NOICON);
MENUITEM_SETTING(timeformat, &global_settings.timeformat, NULL);
MAKE_MENU(time_menu, ID2P(LANG_TIME_MENU), 0, NOICON, &time_set, &timeformat);
#endif

/* System menu */
MENUITEM_SETTING(poweroff, &global_settings.poweroff, NULL);

/* sleep Menu */
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

static int sleep_timer(void)
{
    int minutes = (get_sleep_timer() + 59) / 60; /* round up */
    return (int)set_int(str(LANG_SLEEP_TIMER), "", UNIT_MIN, &minutes,
                   &sleep_timer_set, 5, 0, 300, sleep_timer_formatter);
}

MENUITEM_FUNCTION(sleep_timer_call, ID2P(LANG_SLEEP_TIMER), sleep_timer,
                    NULL, bitmap_icons_6x8[Icon_Menu_setting]); /* make it look like a 
                                                                setting to the user */
#ifdef HAVE_ALARM_MOD
MENUITEM_FUNCTION(alarm_screen_call, ID2P(LANG_ALARM_MOD_ALARM_MENU),
                   (menu_function)alarm_screen, NULL, NOICON);
#endif

/* Limits menu */
MENUITEM_SETTING(max_files_in_dir, &global_settings.max_files_in_dir, NULL);
MENUITEM_SETTING(max_files_in_playlist, &global_settings.max_files_in_playlist, NULL);
MAKE_MENU(limits_menu, ID2P(LANG_LIMITS_MENU), 0, NOICON,
           &max_files_in_dir, &max_files_in_playlist);

#if CONFIG_CODEC == MAS3507D
void dac_line_in(bool enable);
static int linein_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
#ifndef SIMULATOR
            dac_line_in(global_settings.line_in);
#endif
            break;
    }
    return action;
}
MENUITEM_SETTING(line_in, &global_settings.line_in, linein_callback);
#endif
#ifdef CONFIG_CHARGING
MENUITEM_SETTING(car_adapter_mode, &global_settings.car_adapter_mode, NULL);
#endif

MAKE_MENU(system_menu, ID2P(LANG_SYSTEM), 
          0, bitmap_icons_6x8[Icon_System_menu],
#ifndef SIMULATOR
            &battery_menu,
#endif
#ifndef HAVE_MMC
            &disk_menu,
#endif
#ifdef CONFIG_RTC
            &time_menu,
#endif
            &poweroff,
            &sleep_timer_call,
#ifdef HAVE_ALARM_MOD
            &alarm_screen_call,
#endif
            &limits_menu,
#if CONFIG_CODEC == MAS3507D
            &line_in,
#endif
#ifdef CONFIG_CHARGING
            &car_adapter_mode,
#endif
         );

/*    SYSTEM MENU                  */
/***********************************/


/***********************************/
/*    BOOKMARK MENU                */
static int bmark_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            if(global_settings.autocreatebookmark ==  BOOKMARK_RECENT_ONLY_YES ||
               global_settings.autocreatebookmark ==  BOOKMARK_RECENT_ONLY_ASK)
            {
                if(global_settings.usemrb == BOOKMARK_NO)
                    global_settings.usemrb = BOOKMARK_YES;
        
            }
            break;
    }
    return action;
}
MENUITEM_SETTING(autocreatebookmark,
                 &global_settings.autocreatebookmark, bmark_callback);
MENUITEM_SETTING(autoloadbookmark, &global_settings.autoloadbookmark, NULL);
MENUITEM_SETTING(usemrb, &global_settings.usemrb, NULL);
MAKE_MENU(bookmark_settings_menu, ID2P(LANG_BOOKMARK_SETTINGS), 0,
          bitmap_icons_6x8[Icon_Bookmark],
          &autocreatebookmark, &autoloadbookmark, &usemrb);
/*    BOOKMARK MENU                */
/***********************************/

/***********************************/
/*    VOICE MENU                   */
static int talk_callback(int action,const struct menu_item_ex *this_item);
MENUITEM_SETTING(talk_menu, &global_settings.talk_menu, NULL);
MENUITEM_SETTING(talk_dir, &global_settings.talk_dir, talk_callback);
MENUITEM_SETTING(talk_file_item, &global_settings.talk_file, talk_callback);
static int talk_callback(int action,const struct menu_item_ex *this_item)
{
    static int oldval = 0;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            oldval = global_settings.talk_file;
            break;
        case ACTION_EXIT_MENUITEM:
#if CONFIG_CODEC == SWCODEC
            audio_set_crossfade(global_settings.crossfade);
#endif
            if (this_item == &talk_dir)
                break;
            if (oldval != 3 && global_settings.talk_file == 3)
            {   /* force reload if newly talking thumbnails,
                because the clip presence is cached only if enabled */
                reload_directory();
            }
            break;
    }
    return action;
}
MAKE_MENU(voice_settings_menu, ID2P(LANG_VOICE), 0, bitmap_icons_6x8[Icon_Voice],
          &talk_menu, &talk_dir, &talk_file_item);
/*    VOICE MENU                   */
/***********************************/

/***********************************/
/*    SETTINGS MENU                */
static int language_browse(void)
{
    return (int)rockbox_browse(LANG_DIR, SHOW_LNG);
}
MENUITEM_FUNCTION(browse_langs, ID2P(LANG_LANGUAGE), language_browse,
                    NULL, bitmap_icons_6x8[Icon_Language]);

MAKE_MENU(settings_menu_item, ID2P(LANG_GENERAL_SETTINGS), 0,
          bitmap_icons_6x8[Icon_General_settings_menu],
          &playback_menu_item, &file_menu, &display_menu, &system_menu,
          &bookmark_settings_menu, &browse_langs, &voice_settings_menu );
/*    SETTINGS MENU                */
/***********************************/
