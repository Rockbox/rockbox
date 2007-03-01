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
#include "settings_menu.h"
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



struct browse_folder_info {
    const char* dir;
    int show_options;
};
static struct browse_folder_info theme = {THEME_DIR, SHOW_CFG};
static struct browse_folder_info config = {ROCKBOX_DIR, SHOW_CFG};
static int browse_folder(void *param)
{
    const struct browse_folder_info *info =
            (const struct browse_folder_info*)param;
    return rockbox_browse(info->dir, info->show_options);
}

/***********************************/
/*    MANAGE SETTINGS MENU        */

static int reset_settings(void)
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
            return 1;
    }
    return 0;
}
static int write_settings_file(void* param)
{
    return settings_save_config((intptr_t)param);
}

MENUITEM_FUNCTION_WPARAM(browse_configs, ID2P(LANG_CUSTOM_CFG), 
        browse_folder, (void*)&config, NULL, NOICON);
MENUITEM_FUNCTION_WPARAM(save_settings_item, ID2P(LANG_SAVE_SETTINGS), 
        write_settings_file, (void*)SETTINGS_SAVE_ALL, NULL, NOICON);
MENUITEM_FUNCTION_WPARAM(save_theme_item, ID2P(LANG_SAVE_THEME), 
        write_settings_file, (void*)SETTINGS_SAVE_THEME, NULL, NOICON);
MENUITEM_FUNCTION(reset_settings_item,ID2P(LANG_RESET),reset_settings, NULL, NOICON);

MAKE_MENU(manage_settings, ID2P(LANG_MANAGE_MENU), NULL, bitmap_icons_6x8[Icon_Config],
          &browse_configs, &reset_settings_item,
          &save_settings_item, &save_theme_item);
/*    MANAGE SETTINGS MENU        */
/**********************************/

/***********************************/
/*      INFO MENU                  */

static bool show_credits(void)
{
    plugin_load(PLUGIN_DIR "/credits.rock",NULL);
    return false;
}

#ifdef SIMULATOR
extern bool simulate_usb(void);
#endif

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
            {   /* say whatever is reasonable, no real connection to the screen */
                bool enqueue = false; /* enqueue all but the first */
                if (battery_level() >= 0)
                {
                    talk_id(LANG_BATTERY_TIME, enqueue);
                    enqueue = true;
                    talk_value(battery_level(), UNIT_PERCENT, true);
#if CONFIG_CHARGING >= CHARGING_MONITOR
                    if (charge_state == CHARGING)
                        talk_id(LANG_BATTERY_CHARGE, true);               
                    else if (charge_state == TOPOFF)
                        talk_id(LANG_BATTERY_TOPOFF_CHARGE, true);
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

#ifdef CONFIG_RTC
                {
                    struct tm* tm = get_time();
                    talk_id(VOICE_CURRENT_TIME, true);
                    talk_value(tm->tm_hour, UNIT_HOUR, true);
                    talk_value(tm->tm_min, UNIT_MIN, true);
                    talk_value(tm->tm_sec, UNIT_SEC, true);
                    talk_id(LANG_MONTH_JANUARY + tm->tm_mon, true);
                    talk_number(tm->tm_mday, true);
                    talk_number(1900 + tm->tm_year, true);
                }
#endif
            }
            new_info = false;
        }

        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();
#ifdef HAVE_LCD_BITMAP
            screens[i].puts(0, y, str(LANG_ROCKBOX_INFO));
#endif
        }
#ifdef HAVE_LCD_BITMAP
        y += 2;
#endif

#ifdef HAVE_LCD_CHARCELLS
        if (page == 0)
#endif
        {
            int integer = buflen / 1000;
            int decimal = buflen % 1000;

#ifdef HAVE_LCD_CHARCELLS
            snprintf(s, sizeof(s), (char *)str(LANG_BUFFER_STAT_PLAYER),
                     integer, decimal);
#else
            snprintf(s, sizeof(s), (char *)str(LANG_BUFFER_STAT_RECORDER),
                     integer, decimal);
#endif
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
        if (page == 1)
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

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
        FOR_NB_SCREENS(i)
                screens[i].update();
#endif

        /* Wait for a key to be pushed */
        key = get_action(CONTEXT_MAINMENU,HZ*5);
        switch(key) {

            case ACTION_STD_CANCEL:
                done = true;
                break;

#ifdef HAVE_LCD_CHARCELLS
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
                page = (page == 0) ? 1 : 0;
                break;
#endif

#ifndef SIMULATOR
            case ACTION_STD_OK:
                gui_syncsplash(0, true, str(LANG_DIRCACHE_BUILDING));
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
    action_signalscreenchange();
    return false;
}
MENUITEM_FUNCTION(show_info_item, ID2P(LANG_INFO_MENU),
                   (menu_function)show_info, NULL, NOICON);
MENUITEM_FUNCTION(show_credits_item, ID2P(LANG_VERSION),
                   (menu_function)show_credits, NULL, NOICON);
MENUITEM_FUNCTION(show_runtime_item, ID2P(LANG_RUNNING_TIME),
                   (menu_function)view_runtime, NULL, NOICON);

#ifndef SIMULATOR
MENUITEM_FUNCTION(debug_menu_item, ID2P(LANG_DEBUG),
                   (menu_function)debug_menu, NULL, NOICON);
#else
MENUITEM_FUNCTION(simulate_usb_item, ID2P(LANG_USB),
                   (menu_function)simulate_usb, NULL, NOICON);
#ifdef ROCKBOX_HAS_LOGF
MENUITEM_FUNCTION(logfdisplay_item, "logf",(int (*)(void)) logfdisplay, NULL, NOICON);
MENUITEM_FUNCTION(logfdump_item, "logfdump",(int (*)(void)) logfdump, NULL, NOICON);
#endif
#endif

MAKE_MENU(info_menu, ID2P(LANG_INFO), 0, bitmap_icons_6x8[Icon_Questionmark],
          &show_info_item, &show_credits_item, &show_runtime_item,
#ifndef SIMULATOR
         &debug_menu_item
#else
         &simulate_usb_item
#ifdef ROCKBOX_HAS_LOGF
        ,&logfdisplay_item, &logfdump_item
#endif
#endif
        );
/*      INFO MENU                  */
/***********************************/

/***********************************/
/*    MAIN MENU                    */

MENUITEM_FUNCTION_WPARAM(browse_themes, ID2P(LANG_CUSTOM_THEME), 
        browse_folder, (void*)&theme, NULL, bitmap_icons_6x8[Icon_Folder]);

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
MAKE_MENU(main_menu_, ID2P(LANG_SETTINGS_MENU), mainmenu_callback,
        bitmap_icons_6x8[Icon_Submenu_Entered],
        &sound_settings,
        &settings_menu_item, &manage_settings, &browse_themes,
#ifdef HAVE_RECORDING
        &recording_settings,
#endif
        );
/*    MAIN MENU                    */
/***********************************/

