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
#include "string.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "powermgmt.h"
#include "menu.h"
#include "misc.h"
#include "exported_menus.h"
#include "tree.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#include "bookmark.h"
#include "yesno.h"
#include "keyboard.h"
#include "screens.h"
#include "plugin.h"
#include "talk.h"
#include "buffer.h"
#include "splash.h"
#include "debug_menu.h"
#if defined(SIMULATOR) && defined(ROCKBOX_HAS_LOGF)
#include "logfdisp.h"
#endif
#include "version.h"
#include "time.h"



static struct browse_folder_info config = {ROCKBOX_DIR, SHOW_CFG};

/***********************************/
/*    MANAGE SETTINGS MENU        */

static int reset_settings(void)
{
    unsigned char *lines[]={ID2P(LANG_RESET_ASK)};
    unsigned char *yes_lines[]={
        str(LANG_SETTINGS),
        ID2P(LANG_RESET_DONE_CLEAR)
    };
    unsigned char *no_lines[]={yes_lines[0], ID2P(LANG_CANCEL)};
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
            return 1;
    }
    return 0;
}
static int write_settings_file(void* param)
{
    return settings_save_config((intptr_t)param);
}

MENUITEM_FUNCTION(browse_configs, MENU_FUNC_USEPARAM, ID2P(LANG_CUSTOM_CFG), 
        browse_folder, (void*)&config, NULL, Icon_NOICON);
MENUITEM_FUNCTION(save_settings_item, MENU_FUNC_USEPARAM, ID2P(LANG_SAVE_SETTINGS), 
        write_settings_file, (void*)SETTINGS_SAVE_ALL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(save_theme_item, MENU_FUNC_USEPARAM, ID2P(LANG_SAVE_THEME), 
        write_settings_file, (void*)SETTINGS_SAVE_THEME, NULL, Icon_NOICON);
MENUITEM_FUNCTION(reset_settings_item, 0, ID2P(LANG_RESET),
                  reset_settings, NULL, NULL, Icon_NOICON);

MAKE_MENU(manage_settings, ID2P(LANG_MANAGE_MENU), NULL, Icon_Config,
          &browse_configs, &reset_settings_item,
          &save_settings_item, &save_theme_item);
/*    MANAGE SETTINGS MENU        */
/**********************************/

/***********************************/
/*      INFO MENU                  */

static bool show_credits(void)
{
    if (plugin_load(VIEWERS_DIR "/credits.rock",NULL) != PLUGIN_OK)
    {
        /* show the rockbox logo and version untill a button is pressed */
        show_logo();
        get_action(CONTEXT_STD, TIMEOUT_BLOCK);
    }
    return false;
}

#ifdef HAVE_LCD_CHARCELLS
#define SIZE_FMT "%s%s"
#else
#define SIZE_FMT "%s %s"
#endif

static bool show_info(void)
{
    char s[64], s1[32];
    unsigned long size, free;
    long buflen = ((audiobufend - audiobuf) * 2) / 2097;  /* avoid overflow */
    int key;
    int i;
    bool done = false;
    bool new_info = true;
#ifdef HAVE_MULTIVOLUME
    char s2[32];
    unsigned long size2, free2;
#endif
#ifdef HAVE_LCD_CHARCELLS
    int page = 0;
#endif

    const unsigned char *kbyte_units[] = {
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };
#if defined(HAVE_LCD_BITMAP)
    FOR_NB_SCREENS(i)
        screens[i].setmargins(0, 0);
#endif
    while (!done)
    {
        int y=0;

        if (new_info)
        {
            fat_size( IF_MV2(0,) &size, &free );
#ifdef HAVE_MULTIVOLUME
            if (fat_ismounted(1))
                fat_size( 1, &size2, &free2 );
            else
                size2 = 0;
#endif

            if (global_settings.talk_menu)
            {   
                /* say whatever is reasonable, no real connection to the screen */
                bool enqueue = false; /* enqueue all but the first */
                if (battery_level() >= 0)
                {
                    talk_id(LANG_BATTERY_TIME, enqueue);
                    enqueue = true;
                    talk_value(battery_level(), UNIT_PERCENT, true);
#if CONFIG_CHARGING >= CHARGING_MONITOR
                    if (charge_state == CHARGING)
                        talk_id(LANG_BATTERY_CHARGE, true);
#if CONFIG_CHARGING == CHARGING_CONTROL
                    else if (charge_state == TOPOFF)
                        talk_id(LANG_BATTERY_TOPOFF_CHARGE, true);
#endif
                    else if (charge_state == TRICKLE)
                        talk_id(LANG_BATTERY_TRICKLE_CHARGE, true);
#endif
                }

                talk_id(LANG_DISK_FREE_INFO, enqueue);
#ifdef HAVE_MULTIVOLUME
                talk_id(LANG_DISK_NAME_INTERNAL, true);
                output_dyn_value(NULL, 0, free, kbyte_units, true);
                if (size2)
                {
                    talk_id(LANG_DISK_NAME_MMC, true);
                    output_dyn_value(NULL, 0, free2, kbyte_units, true);
                }
#else
                output_dyn_value(NULL, 0, free, kbyte_units, true);
#endif
#if CONFIG_RTC
                talk_date_time(get_time(), true);
#endif
            new_info = false;
        }
}
        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();
#ifdef HAVE_LCD_BITMAP
            screens[i].puts(0, y, str(LANG_ROCKBOX_INFO));
#endif
        }
#ifdef HAVE_LCD_BITMAP
        y++;
#endif
#ifdef HAVE_LCD_CHARCELLS
        if (page == 0)
#endif
        {
            snprintf(s, sizeof(s), "%s: %s", str(LANG_VERSION), appsversion);
            FOR_NB_SCREENS(i)
                screens[i].puts_scroll(0, y, (unsigned char *)s);
#ifdef HAVE_LCD_BITMAP
            y += 2;
#endif
        }

#ifdef HAVE_LCD_CHARCELLS
        if (page == 1)
#endif
        {
            int integer = buflen / 1000;
            int decimal = buflen % 1000;

            snprintf(s, sizeof(s), (char *)str(LANG_BUFFER_STAT),
                     integer, decimal);

            FOR_NB_SCREENS(i)
                screens[i].puts_scroll(0, y, (unsigned char *)s);
            y++;
#if CONFIG_CHARGING == CHARGING_CONTROL
            if (charge_state == CHARGING)
                snprintf(s, sizeof(s), (char *)str(LANG_BATTERY_CHARGE));
            else if (charge_state == TOPOFF)
                snprintf(s, sizeof(s), (char *)str(LANG_BATTERY_TOPOFF_CHARGE));
            else if (charge_state == TRICKLE)
                snprintf(s, sizeof(s), (char *)str(LANG_BATTERY_TRICKLE_CHARGE));
            else
#endif
            if (battery_level() >= 0)
                snprintf(s, sizeof(s), (char *)str(LANG_BATTERY_TIME), battery_level(),
                         battery_time() / 60, battery_time() % 60);
            else
                strncpy(s, "(n/a)", sizeof(s));
            FOR_NB_SCREENS(i)
                screens[i].puts_scroll(0, y, (unsigned char *)s); 
            y++;
        }

#ifdef HAVE_LCD_CHARCELLS
        if (page == 2)
#endif
        {
#ifdef HAVE_MULTIVOLUME
            output_dyn_value(s1, sizeof s1, free, kbyte_units, true);
            output_dyn_value(s2, sizeof s2, size, kbyte_units, true);
            snprintf(s, sizeof s, "%s %s/%s", str(LANG_DISK_NAME_INTERNAL),
                     s1, s2);
            FOR_NB_SCREENS(i)
                screens[i].puts_scroll(0, y, (unsigned char *)s);
            y++;

            if (size2) {
                output_dyn_value(s1, sizeof s1, free2, kbyte_units, true);
                output_dyn_value(s2, sizeof s2, size2, kbyte_units, true);
                snprintf(s, sizeof s, "%s %s/%s", str(LANG_DISK_NAME_MMC),
                         s1, s2);
                FOR_NB_SCREENS(i)
                    screens[i].puts_scroll(0, y, (unsigned char *)s);
                y++;
            }
#else
            output_dyn_value(s1, sizeof s1, size, kbyte_units, true);
            snprintf(s, sizeof s, SIZE_FMT, str(LANG_DISK_SIZE_INFO), s1);
            FOR_NB_SCREENS(i)
                screens[i].puts_scroll(0, y, (unsigned char *)s);
            y++;
            output_dyn_value(s1, sizeof s1, free, kbyte_units, true);
            snprintf(s, sizeof s, SIZE_FMT, str(LANG_DISK_FREE_INFO), s1);
            FOR_NB_SCREENS(i)
                screens[i].puts_scroll(0, y, (unsigned char *)s);
            y++;
#endif
        }

        FOR_NB_SCREENS(i)
                screens[i].update();

        /* Wait for a key to be pushed */
        key = get_action(CONTEXT_MAINMENU,HZ*5);
        switch(key) {

            case ACTION_STD_CANCEL:
                done = true;
                break;

#ifdef HAVE_LCD_CHARCELLS
            case ACTION_STD_NEXT:
                page = (page+1)%3;
                break;
            case ACTION_STD_PREV:
                page--;
                if (page < 0)
                    page = 2;
                break;
#endif

#ifndef SIMULATOR
            case ACTION_STD_OK:
                gui_syncsplash(0, str(LANG_SCANNING_DISK));
                fat_recalc_free(IF_MV(0));
#ifdef HAVE_MULTIVOLUME
                if (fat_ismounted(1))
                    fat_recalc_free(1);
#endif
                new_info = true;
                break;
#endif

            default:
                if (default_event_handler(key) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
    return false;
}
MENUITEM_FUNCTION(show_info_item, 0, ID2P(LANG_ROCKBOX_INFO),
                   (menu_function)show_info, NULL, NULL, Icon_NOICON);


/* sleep Menu */
static void sleep_timer_formatter(char* buffer, size_t buffer_size, int value,
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
                   &sleep_timer_set, -5, 300, 0, sleep_timer_formatter);
}

MENUITEM_FUNCTION(sleep_timer_call, 0, ID2P(LANG_SLEEP_TIMER), sleep_timer,
                    NULL, NULL, Icon_Menu_setting); /* make it look like a 
                                                                setting to the user */
MENUITEM_FUNCTION(show_credits_item, 0, ID2P(LANG_VERSION),
                   (menu_function)show_credits, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(show_runtime_item, 0, ID2P(LANG_RUNNING_TIME),
                   (menu_function)view_runtime, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(debug_menu_item, 0, ID2P(LANG_DEBUG),
                   (menu_function)debug_menu, NULL, NULL, Icon_NOICON);

MAKE_MENU(info_menu, ID2P(LANG_SYSTEM), 0, Icon_Questionmark,
          &show_info_item, &show_credits_item, &show_runtime_item, 
          &sleep_timer_call, &debug_menu_item);
/*      INFO MENU                  */
/***********************************/

/***********************************/
/*    MAIN MENU                    */


#ifdef HAVE_LCD_CHARCELLS
int mainmenu_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            status_set_param(true);
            break;
        case ACTION_EXIT_MENUITEM:
            status_set_param(false);
            break;
    }
    return action;
}
#else
#define mainmenu_callback NULL
#endif
MAKE_MENU(main_menu_, ID2P(LANG_SETTINGS), mainmenu_callback,
        Icon_Submenu_Entered,
        &sound_settings,
        &settings_menu_item, &theme_menu,
#ifdef HAVE_RECORDING
        &recording_settings,
#endif
        &manage_settings,
        );
/*    MAIN MENU                    */
/***********************************/

