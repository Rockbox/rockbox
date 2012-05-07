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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "power.h"
#include "powermgmt.h"
#include "menu.h"
#include "misc.h"
#include "exported_menus.h"
#include "tree.h"
#include "storage.h"
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#include "yesno.h"
#include "keyboard.h"
#include "screens.h"
#include "plugin.h"
#include "talk.h"
#include "splash.h"
#include "debug_menu.h"
#include "version.h"
#include "time.h"
#include "wps.h"
#include "skin_buffer.h"

static const struct browse_folder_info config = {ROCKBOX_DIR, SHOW_CFG};

/***********************************/
/*    MANAGE SETTINGS MENU        */

static int reset_settings(void)
{
    static const char *lines[]={ID2P(LANG_RESET_ASK)};
    static const char *yes_lines[]={
        ID2P(LANG_SETTINGS),
        ID2P(LANG_RESET_DONE_CLEAR)
    };
    static const char *no_lines[]={
        ID2P(LANG_SETTINGS),
        ID2P(LANG_CANCEL)
    };
    static const struct text_message message={lines, 1};
    static const struct text_message yes_message={yes_lines, 2};
    static const struct text_message no_message={no_lines, 2};

    switch(gui_syncyesno_run(&message, &yes_message, &no_message))
    {
        case YESNO_YES:
            settings_reset();
            settings_save();
            settings_apply(true);
            settings_apply_skins();
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
MENUITEM_FUNCTION(save_sound_item, MENU_FUNC_USEPARAM, ID2P(LANG_SAVE_SOUND), 
        write_settings_file, (void*)SETTINGS_SAVE_SOUND, NULL, Icon_NOICON);
MENUITEM_FUNCTION(reset_settings_item, 0, ID2P(LANG_RESET),
                  reset_settings, NULL, NULL, Icon_NOICON);

MAKE_MENU(manage_settings, ID2P(LANG_MANAGE_MENU), NULL, Icon_Config,
          &browse_configs, &reset_settings_item,
          &save_settings_item, &save_sound_item, &save_theme_item);
/*    MANAGE SETTINGS MENU        */
/**********************************/

/***********************************/
/*      INFO MENU                  */


static bool show_credits(void)
{
    char credits[MAX_PATH] = { '\0' };
    snprintf(credits, MAX_PATH, "%s/credits.rock", VIEWERS_DIR);
    if (plugin_load(credits, NULL) != PLUGIN_OK)
    {
        /* show the rockbox logo and version untill a button is pressed */
        show_logo();
        while (IS_SYSEVENT(get_action(CONTEXT_STD, TIMEOUT_BLOCK)))
            ;
    }
    return false;
}

#ifdef HAVE_LCD_CHARCELLS
#define SIZE_FMT "%s%s"
#else
#define SIZE_FMT "%s %s"
#endif
struct info_data 

{
    bool new_data;
    unsigned long size;
    unsigned long free;
#ifdef HAVE_MULTIVOLUME
    unsigned long size2;
    unsigned long free2;
#endif
};
enum infoscreenorder
{
    INFO_BATTERY = 0,
    INFO_DISK1, /* capacity or internal capacity/free on hotswap */
    INFO_DISK2, /* free space or external capacity/free on hotswap */
    INFO_BUFFER,
    INFO_VERSION,
    INFO_COUNT
};

static const char* info_getname(int selected_item, void *data,
                                char *buffer, size_t buffer_len)
{
    struct info_data *info = (struct info_data*)data;
    char s1[32];
#if defined(HAVE_MULTIVOLUME)
    char s2[32];
#endif
    if (info->new_data)
    {
        fat_size(IF_MV2(0,) &info->size, &info->free);
#ifdef HAVE_MULTIVOLUME
        if (fat_ismounted(1))
            fat_size(1, &info->size2, &info->free2);
        else
            info->size2 = 0;
#endif
        info->new_data = false;
    }
    switch (selected_item)
    {
        case INFO_VERSION:
            snprintf(buffer, buffer_len, "%s: %s", 
                     str(LANG_VERSION), rbversion);
            break;

        case INFO_BUFFER: /* buffer */
        {
            long kib = audio_buffer_size() / 1024; /* to KiB */
            output_dyn_value(s1, sizeof(s1), kib, kbyte_units, true);
            snprintf(buffer, buffer_len, "%s %s", str(LANG_BUFFER_STAT), s1);
        }
        break;
        case INFO_BATTERY: /* battery */
#if CONFIG_CHARGING == CHARGING_SIMPLE
            /* Only know if plugged */
            if (charger_inserted())
                return str(LANG_BATTERY_CHARGE);
            else
#elif CONFIG_CHARGING >= CHARGING_MONITOR
#ifdef ARCHOS_RECORDER
            /* Report the particular algorithm state */
            if (charge_state == CHARGING)
                return str(LANG_BATTERY_CHARGE);
            else if (charge_state == TOPOFF)
                return str(LANG_BATTERY_TOPOFF_CHARGE);
            else if (charge_state == TRICKLE)
                return str(LANG_BATTERY_TRICKLE_CHARGE);
            else
#else /* !ARCHOS_RECORDER */
            /* Go by what power management reports */
            if (charging_state())
                return str(LANG_BATTERY_CHARGE);
            else
#endif /* ARCHOS_RECORDER */
#endif /* CONFIG_CHARGING = */
            if (battery_level() >= 0)
                snprintf(buffer, buffer_len, str(LANG_BATTERY_TIME),
                         battery_level(), battery_time() / 60, battery_time() % 60);
            else
                return "Battery n/a"; /* translating worth it? */
            break;
        case INFO_DISK1: /* disk usage 1 */
#ifdef HAVE_MULTIVOLUME
            output_dyn_value(s1, sizeof s1, info->free, kbyte_units, true);
            output_dyn_value(s2, sizeof s2, info->size, kbyte_units, true);
            snprintf(buffer, buffer_len, "%s %s/%s", str(LANG_DISK_NAME_INTERNAL),
                     s1, s2);
#else
            output_dyn_value(s1, sizeof s1, info->free, kbyte_units, true);
            snprintf(buffer, buffer_len, SIZE_FMT, str(LANG_DISK_FREE_INFO), s1);
#endif
            break;
        case INFO_DISK2: /* disk usage 2 */
#ifdef HAVE_MULTIVOLUME
            if (info->size2)
            {
                output_dyn_value(s1, sizeof s1, info->free2, kbyte_units, true);
                output_dyn_value(s2, sizeof s2, info->size2, kbyte_units, true);
                snprintf(buffer, buffer_len, "%s %s/%s", str(LANG_DISK_NAME_MMC),
                         s1, s2);
            }
            else 
            {
                snprintf(buffer, buffer_len, "%s %s", str(LANG_DISK_NAME_MMC),
                         str(LANG_NOT_PRESENT));
            }
#else
            output_dyn_value(s1, sizeof s1, info->size, kbyte_units, true);
            snprintf(buffer, buffer_len, SIZE_FMT, str(LANG_DISK_SIZE_INFO), s1);
#endif
            break;
    }
    return buffer;
}

static int info_speak_item(int selected_item, void * data)
{
    struct info_data *info = (struct info_data*)data;

    switch (selected_item)
    {
        case INFO_VERSION: /* version */
            talk_id(LANG_VERSION, false);
            talk_spell(rbversion, true);
            break;

        case INFO_BUFFER: /* buffer */
        {
            talk_id(LANG_BUFFER_STAT, false);
            long kib = audio_buffer_size() / 1024; /* to KiB */
            output_dyn_value(NULL, 0, kib, kbyte_units, true);
            break;
        }
        case INFO_BATTERY: /* battery */
#if CONFIG_CHARGING == CHARGING_SIMPLE
            /* Only know if plugged */
            if (charger_inserted())
                talk_id(LANG_BATTERY_CHARGE, true);
            else
#elif CONFIG_CHARGING >= CHARGING_MONITOR
#ifdef ARCHOS_RECORDER
            /* Report the particular algorithm state */
            if (charge_state == CHARGING)
                talk_id(LANG_BATTERY_CHARGE, true);
            else if (charge_state == TOPOFF)
                talk_id(LANG_BATTERY_TOPOFF_CHARGE, true);
            else if (charge_state == TRICKLE)
                talk_id(LANG_BATTERY_TRICKLE_CHARGE, true);
            else
#else /* !ARCHOS_RECORDER */
            /* Go by what power management reports */
            if (charging_state())
                talk_id(LANG_BATTERY_CHARGE, true);
            else
#endif /* ARCHOS_RECORDER */
#endif /* CONFIG_CHARGING = */
            if (battery_level() >= 0)
            {
                int time_left = battery_time();
                talk_id(LANG_BATTERY_TIME, false);
                talk_value(battery_level(), UNIT_PERCENT, true);
                if (time_left >= 0)
                    talk_value(time_left * 60, UNIT_TIME, true);
            }
            else talk_id(VOICE_BLANK, false);
            break;
        case INFO_DISK1: /* disk 1 */
#ifdef HAVE_MULTIVOLUME
            talk_ids(false, LANG_DISK_NAME_INTERNAL, LANG_DISK_FREE_INFO);
            output_dyn_value(NULL, 0, info->free, kbyte_units, true);
            talk_id(LANG_DISK_SIZE_INFO, true);
            output_dyn_value(NULL, 0, info->size, kbyte_units, true);
#else
            talk_id(LANG_DISK_FREE_INFO, false);
            output_dyn_value(NULL, 0, info->free, kbyte_units, true);
#endif
            break;
        case INFO_DISK2: /* disk 2 */
#ifdef HAVE_MULTIVOLUME
            talk_id(LANG_DISK_NAME_MMC, false);
            if (info->size2)
            {
                talk_id(LANG_DISK_FREE_INFO, true);
                output_dyn_value(NULL, 0, info->free2, kbyte_units, true);
                talk_id(LANG_DISK_SIZE_INFO, true);
                output_dyn_value(NULL, 0, info->size2, kbyte_units, true);
            }
            else talk_id(LANG_NOT_PRESENT, true);
#else
            talk_id(LANG_DISK_SIZE_INFO, false);
            output_dyn_value(NULL, 0, info->size, kbyte_units, true);
#endif
            break;
            
    }
    return 0;
}

static int info_action_callback(int action, struct gui_synclist *lists)
{
    if (action == ACTION_STD_CANCEL)
        return action;
    else if (action == ACTION_STD_OK
#ifdef HAVE_HOTSWAP
        || action == SYS_FS_CHANGED
#endif
        )
    {
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        struct info_data *info = (struct info_data *)lists->data;
        int i;
        info->new_data = true;
        splash(0, ID2P(LANG_SCANNING_DISK));
        for (i = 0; i < NUM_VOLUMES; i++)
        {
#ifdef HAVE_HOTSWAP
            if (fat_ismounted(i))
#endif
                fat_recalc_free(IF_MV(i));
        }
#else
        (void) lists;
#endif
        gui_synclist_speak_item(lists);
        return ACTION_REDRAW;
    }
    return action;
}
static bool show_info(void)
{
    struct info_data data = {.new_data = true };
    struct simplelist_info info;
    simplelist_info_init(&info, str(LANG_ROCKBOX_INFO), INFO_COUNT, (void*)&data);
    info.hide_selection = !global_settings.talk_menu;
    if (info.hide_selection)
        info.scroll_all = true;
    info.get_name = info_getname;
    if(global_settings.talk_menu)
         info.get_talk = info_speak_item;
    info.action_callback = info_action_callback;
    return simplelist_show_list(&info);
}
MENUITEM_FUNCTION(show_info_item, 0, ID2P(LANG_ROCKBOX_INFO),
                   (menu_function)show_info, NULL, NULL, Icon_NOICON);

#if CONFIG_RTC
int time_screen(void* ignored);
MENUITEM_FUNCTION(timedate_item, MENU_FUNC_CHECK_RETVAL, ID2P(LANG_TIME_MENU),
                    time_screen, NULL,  NULL, Icon_Menu_setting );
#endif

MENUITEM_FUNCTION(show_credits_item, 0, ID2P(LANG_CREDITS),
                   (menu_function)show_credits, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(show_runtime_item, 0, ID2P(LANG_RUNNING_TIME),
                   (menu_function)view_runtime, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(debug_menu_item, 0, ID2P(LANG_DEBUG),
                   (menu_function)debug_menu, NULL, NULL, Icon_NOICON);

MAKE_MENU(info_menu, ID2P(LANG_SYSTEM), 0, Icon_System_menu,
          &show_info_item, &show_credits_item,
          &show_runtime_item, &debug_menu_item);
/*      INFO MENU                  */
/***********************************/

/***********************************/
/*    MAIN MENU                    */


#ifdef HAVE_LCD_CHARCELLS
static int mainmenu_callback(int action,const struct menu_item_ex *this_item)
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
        &playback_settings,
        &settings_menu_item, &theme_menu,
#ifdef HAVE_RECORDING
        &recording_settings,
#endif
#if CONFIG_RTC
        &timedate_item,
#endif
        &manage_settings,
        );
/*    MAIN MENU                    */
/***********************************/

