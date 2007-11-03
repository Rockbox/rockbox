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

static const struct browse_folder_info config = {ROCKBOX_DIR, SHOW_CFG};

/***********************************/
/*    MANAGE SETTINGS MENU        */

static int reset_settings(void)
{
    const unsigned char *lines[]={ID2P(LANG_RESET_ASK)};
    const unsigned char *yes_lines[]={
        str(LANG_SETTINGS),
        ID2P(LANG_RESET_DONE_CLEAR)
    };
    const unsigned char *no_lines[]={yes_lines[0], ID2P(LANG_CANCEL)};
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
#if CONFIG_RTC
    INFO_DATE,
    INFO_TIME,
#endif
    INFO_COUNT
};


static char* info_getname(int selected_item, void *data, char *buffer)
{
    struct info_data *info = (struct info_data*)data;
#if CONFIG_RTC
    struct tm *tm;
#endif
    const unsigned char *kbyte_units[] =
    {
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };
    char s1[32];
#ifdef HAVE_MULTIVOLUME
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
            snprintf(buffer, MAX_PATH, "%s: %s", 
                     str(LANG_VERSION), appsversion);
            break;
#if CONFIG_RTC
        case INFO_TIME:
            tm = get_time();
            snprintf(buffer, MAX_PATH, "%02d:%02d:%02d %s", 
                global_settings.timeformat == 0 ? tm->tm_hour : tm->tm_hour-12,
                tm->tm_min, 
                tm->tm_sec, 
                global_settings.timeformat == 0 ? "" : tm->tm_hour>11 ? "P" : "A");
            break;
        case INFO_DATE:
            tm = get_time();
            snprintf(buffer, MAX_PATH, "%s %d %d", 
                str(LANG_MONTH_JANUARY + tm->tm_mon),
                tm->tm_mday,
                tm->tm_year+1900);
            break;
#endif
        case INFO_BUFFER: /* buffer */
        {
            long buflen = ((audiobufend - audiobuf) * 2) / 2097;  /* avoid overflow */
            int integer = buflen / 1000;
            int decimal = buflen % 1000;
            snprintf(buffer, MAX_PATH, (char *)str(LANG_BUFFER_STAT),
                     integer, decimal);
        }
        break;
        case INFO_BATTERY: /* battery */
#if CONFIG_CHARGING == CHARGING_SIMPLE
            if (charger_input_state == CHARGER)
                snprintf(buffer, MAX_PATH, (char *)str(LANG_BATTERY_CHARGE));
            else
#elif CONFIG_CHARGING >= CHARGING_MONITOR
            if (charge_state == CHARGING)
                snprintf(buffer, MAX_PATH, (char *)str(LANG_BATTERY_CHARGE));
            else
#if CONFIG_CHARGING == CHARGING_CONTROL
            if (charge_state == TOPOFF)
                snprintf(buffer, MAX_PATH, (char *)str(LANG_BATTERY_TOPOFF_CHARGE));
            else
#endif
            if (charge_state == TRICKLE)
                snprintf(buffer, MAX_PATH, (char *)str(LANG_BATTERY_TRICKLE_CHARGE));
            else
#endif
            if (battery_level() >= 0)
                snprintf(buffer, MAX_PATH, (char *)str(LANG_BATTERY_TIME), battery_level(),
                         battery_time() / 60, battery_time() % 60);
            else
                strcpy(buffer, "(n/a)");
            break;
        case INFO_DISK1: /* disk usage 1 */
#ifdef HAVE_MULTIVOLUME
            output_dyn_value(s1, sizeof s1, info->free, kbyte_units, true);
            output_dyn_value(s2, sizeof s2, info->size, kbyte_units, true);
            snprintf(buffer, MAX_PATH, "%s %s/%s", str(LANG_DISK_NAME_INTERNAL),
                     s1, s2);
#else
            output_dyn_value(s1, sizeof s1, info->free, kbyte_units, true);
            snprintf(buffer, MAX_PATH, SIZE_FMT, str(LANG_DISK_FREE_INFO), s1);
#endif
            break;
        case INFO_DISK2: /* disk usage 2 */
#ifdef HAVE_MULTIVOLUME
            if (info->size2)
            {
                output_dyn_value(s1, sizeof s1, info->free2, kbyte_units, true);
                output_dyn_value(s2, sizeof s2, info->size2, kbyte_units, true);
                snprintf(buffer, MAX_PATH, "%s %s/%s", str(LANG_DISK_NAME_MMC),
                         s1, s2);
            }
            else 
            {
                snprintf(buffer, MAX_PATH, "%s %s", str(LANG_DISK_NAME_MMC),
                         str(LANG_NOT_PRESENT));
            }
#else
            output_dyn_value(s1, sizeof s1, info->size, kbyte_units, true);
            snprintf(buffer, MAX_PATH, SIZE_FMT, str(LANG_DISK_SIZE_INFO), s1);
#endif
            break;
    }
    return buffer;
}

static int info_speak_item(int selected_item, void * data)
{
    struct info_data *info = (struct info_data*)data;
    const unsigned char *kbyte_units[] = {
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };
    switch (selected_item)
    {
        case INFO_VERSION: /* version */
            talk_id(LANG_VERSION, false);
            talk_spell(appsversion, true);
            break;
#if CONFIG_RTC
        case INFO_TIME: 
            talk_id(VOICE_CURRENT_TIME, false);
            talk_time(get_time(), true);
            break;
        case INFO_DATE:
            talk_date(get_time(), true);
            break;
#endif
        case INFO_BUFFER: /* buffer */
        {
            talk_id(LANG_BUFFER_STAT, false);
            long buflen = ((audiobufend - audiobuf) * 2) / 2097;  /* avoid overflow */
            output_dyn_value(NULL, 0, buflen, kbyte_units, true);
            break;
        }
        case INFO_BATTERY: /* battery */
#if CONFIG_CHARGING == CHARGING_SIMPLE
            if (charger_input_state == CHARGER)
                talk_id(LANG_BATTERY_CHARGE, true);
            else
#elif CONFIG_CHARGING >= CHARGING_MONITOR
            if (charge_state == CHARGING)
                talk_id(LANG_BATTERY_CHARGE, true);
            else
#if CONFIG_CHARGING == CHARGING_CONTROL
            if (charge_state == TOPOFF)
                talk_id(LANG_BATTERY_TOPOFF_CHARGE, true);
            else
#endif
            if (charge_state == TRICKLE)
                talk_id(LANG_BATTERY_TRICKLE_CHARGE, true);
            else
#endif
            if (battery_level() >= 0)
            {
                talk_id(LANG_BATTERY_TIME, false);
                talk_value(battery_level(), UNIT_PERCENT, true);
                talk_value(battery_time() *60, UNIT_TIME, true);
            }
            else talk_id(VOICE_BLANK, false);
            break;
        case INFO_DISK1: /* disk 1 */
#ifdef HAVE_MULTIVOLUME
            talk_id(LANG_DISK_NAME_INTERNAL, false);
            talk_id(LANG_DISK_FREE_INFO, true);
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
    if ((action == ACTION_STD_OK)
#ifdef HAVE_MULTIVOLUME
        || action == SYS_HOTSWAP_INSERTED
        || action == SYS_HOTSWAP_EXTRACTED
#endif
        )
    {
#ifndef SIMULATOR
        struct info_data *info = (struct info_data *)lists->gui_list[SCREEN_MAIN].data;
        info->new_data = true;
        gui_syncsplash(0, ID2P(LANG_SCANNING_DISK));
        fat_recalc_free(IF_MV(0));
#ifdef HAVE_MULTIVOLUME
        if (fat_ismounted(1))
            fat_recalc_free(1);
#endif

#else

        (void) lists;
#endif
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
    info.get_name = info_getname;
    if(global_settings.talk_menu)
         info.get_talk = info_speak_item;
    info.action_callback = info_action_callback;
    return simplelist_show_list(&info);
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

