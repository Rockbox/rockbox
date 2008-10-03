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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "config.h"
#include "menu.h"
#include "root_menu.h"
#include "lang.h"
#include "settings.h"
#include "screens.h"
#include "kernel.h"
#include "debug.h"
#include "misc.h"
#include "rolo.h"
#include "powermgmt.h"
#include "power.h"
#include "talk.h"
#include "audio.h"
#include "hotswap.h"
#include "backdrop.h"

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "splash.h"
#include "buttonbar.h"
#include "action.h"
#include "yesno.h"

#include "tree.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#include "gwps-common.h"
#include "bookmark.h"
#include "playlist.h"
#include "tagtree.h"
#include "menus/exported_menus.h"
#ifdef HAVE_RTC_ALARM
#include "rtc.h"
#endif
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif

struct root_items {
    int (*function)(void* param);
    void* param;
    const struct menu_item_ex *context_menu;
};
static int last_screen = GO_TO_ROOT; /* unfortunatly needed so we can resume
                                        or goto current track based on previous
                                        screen */
static int browser(void* param)
{
    int ret_val;
#ifdef HAVE_TAGCACHE
    struct tree_context* tc = tree_get_context();
#endif
    int filter = SHOW_SUPPORTED;
    char folder[MAX_PATH] = "/";
    /* stuff needed to remember position in file browser */
    static char last_folder[MAX_PATH] = "/";
    /* and stuff for the database browser */
#ifdef HAVE_TAGCACHE
    static int last_db_dirlevel = 0, last_db_selection = 0;
#endif
    
    switch ((intptr_t)param)
    {
        case GO_TO_FILEBROWSER:
            filter = global_settings.dirfilter;
            if (global_settings.browse_current && 
                    last_screen == GO_TO_WPS &&
                    wps_state.current_track_path[0] != '\0')
            {
                strcpy(folder, wps_state.current_track_path);
            }
#ifdef HAVE_HOTSWAP /* quick hack to stop crashing if you try entering 
                     the browser from the menu when you were in the card
                     and it was removed */
            else if (strchr(last_folder, '<') && (card_detect() == false))
                strcpy(folder, "/");
#endif
            else
                strcpy(folder, last_folder);
        break;
#ifdef HAVE_TAGCACHE
        case GO_TO_DBBROWSER:
            if (!tagcache_is_usable())
            {
                bool reinit_attempted = false;

                /* Now display progress until it's ready or the user exits */
                while(!tagcache_is_usable())
                {
                    gui_syncstatusbar_draw(&statusbars, false);
                    struct tagcache_stat *stat = tagcache_get_stat();                
    
                    /* Allow user to exit */
                    if (action_userabort(HZ/2))
                        break;

                    /* Maybe just needs to reboot due to delayed commit */
                    if (stat->commit_delayed)
                    {
                        splash(HZ*2, ID2P(LANG_PLEASE_REBOOT));
                        break;
                    }

                    /* Check if ready status is known */
                    if (!stat->readyvalid)
                    {
                        splash(0, str(LANG_TAGCACHE_BUSY));
                        continue;
                    }
               
                    /* Re-init if required */
                    if (!reinit_attempted && !stat->ready && 
                        stat->processed_entries == 0 && stat->commit_step == 0)
                    {
                        /* Prompt the user */
                        reinit_attempted = true;
                        static const char *lines[]={
                            ID2P(LANG_TAGCACHE_BUSY), ID2P(LANG_TAGCACHE_FORCE_UPDATE)};
                        static const struct text_message message={lines, 2};
                        if(gui_syncyesno_run(&message, NULL, NULL) == YESNO_NO)
                            break;
                        int i;
                        FOR_NB_SCREENS(i)
                            screens[i].clear_display();

                        /* Start initialisation */
                        tagcache_rebuild();
                    }

                    /* Display building progress */
                    static long talked_tick = 0;
                    if(global_settings.talk_menu &&
                       (talked_tick == 0
                        || TIME_AFTER(current_tick, talked_tick+7*HZ)))
                    {
                        talked_tick = current_tick;
                        if (stat->commit_step > 0)
                        {
                            talk_id(LANG_TAGCACHE_INIT, false);
                            talk_number(stat->commit_step, true);
                            talk_id(VOICE_OF, true);
                            talk_number(tagcache_get_max_commit_step(), true);
                        } else if(stat->processed_entries)
                        {
                            talk_number(stat->processed_entries, false);
                            talk_id(LANG_BUILDING_DATABASE, true);
                        }
                    }
                    if (stat->commit_step > 0)
                    {
                        splashf(0, "%s [%d/%d]",
                            str(LANG_TAGCACHE_INIT), stat->commit_step, 
                            tagcache_get_max_commit_step());
                    }
                    else
                    {
                        splashf(0, str(LANG_BUILDING_DATABASE),
                                   stat->processed_entries);
                    }
                }
            }
            if (!tagcache_is_usable())
                return GO_TO_PREVIOUS;
            filter = SHOW_ID3DB;
            tc->dirlevel = last_db_dirlevel;
            tc->selected_item = last_db_selection;
        break;
#endif
        case GO_TO_BROWSEPLUGINS:
            filter = SHOW_PLUGINS;
            strncpy(folder, PLUGIN_DIR, MAX_PATH);
        break;
    }
    ret_val = rockbox_browse(folder, filter);
    switch ((intptr_t)param)
    {
        case GO_TO_FILEBROWSER:
            get_current_file(last_folder, MAX_PATH);
        break;
#ifdef HAVE_TAGCACHE
        case GO_TO_DBBROWSER:
            last_db_dirlevel = tc->dirlevel;
            last_db_selection = tc->selected_item;
        break;
#endif
    }
    return ret_val;
}  

static int menu(void* param)
{
    (void)param;
    return do_menu(NULL, 0, NULL, false);
    
}
#ifdef HAVE_RECORDING
static int recscrn(void* param)
{
    (void)param;
    recording_screen(false);
    return GO_TO_ROOT;
}
#endif
static int wpsscrn(void* param)
{
    int ret_val = GO_TO_PREVIOUS;
    (void)param;
    if (audio_status())
    {
        talk_shutup();
        ret_val = gui_wps_show();
    }
    else if ( global_status.resume_index != -1 )
    {
        DEBUGF("Resume index %X offset %lX\n",
               global_status.resume_index,
               (unsigned long)global_status.resume_offset);
        if (playlist_resume() != -1)
        {
            playlist_start(global_status.resume_index,
                global_status.resume_offset);
            ret_val = gui_wps_show();
        }
    }
    else
    {
        splash(HZ*2, ID2P(LANG_NOTHING_TO_RESUME));
    }
#if LCD_DEPTH > 1
    show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    show_remote_main_backdrop();
#endif
    return ret_val;
}
#if CONFIG_TUNER
static int radio(void* param)
{
    (void)param;
    radio_screen();
    return GO_TO_ROOT;
}
#endif

static int load_bmarks(void* param)
{
    (void)param;
    if(bookmark_mrb_load())
        return GO_TO_WPS;
    return GO_TO_PREVIOUS;
}
static int plugins_menu(void* param)
{
    (void)param;
    MENUITEM_STRINGLIST(plugins_menu_items, ID2P(LANG_PLUGINS), NULL,
                        ID2P(LANG_PLUGIN_GAMES),
                        ID2P(LANG_PLUGIN_APPS), ID2P(LANG_PLUGIN_DEMOS));
    char *folder;
    int retval = GO_TO_PREVIOUS;
    int selection = 0, current = 0;
    while (retval == GO_TO_PREVIOUS)
    {
        selection = do_menu(&plugins_menu_items, &current, NULL, false);
        switch (selection)
        {
            case 0:
                folder = PLUGIN_GAMES_DIR;
                break;
            case 1:
                folder = PLUGIN_APPS_DIR;
                break;
            case 2:
                folder = PLUGIN_DEMOS_DIR;
                break;
            default:
                return selection;
        }
        retval = rockbox_browse(folder, SHOW_PLUGINS);
    }
    return retval;
}

/* These are all static const'd from apps/menus/ *.c
   so little hack so we can use them */
extern struct menu_item_ex 
        file_menu, 
#ifdef HAVE_TAGCACHE
        tagcache_menu,
#endif
        manage_settings,
        recording_settings_menu,
        radio_settings_menu,
        bookmark_settings_menu,
        system_menu;
static const struct root_items items[] = {
    [GO_TO_FILEBROWSER] =   { browser, (void*)GO_TO_FILEBROWSER, &file_menu},
#ifdef HAVE_TAGCACHE
    [GO_TO_DBBROWSER] =     { browser, (void*)GO_TO_DBBROWSER, &tagcache_menu },
#endif
    [GO_TO_WPS] =           { wpsscrn, NULL, &playback_settings },
    [GO_TO_MAINMENU] =      { menu, NULL, &manage_settings },
    
#ifdef HAVE_RECORDING
    [GO_TO_RECSCREEN] =     {  recscrn, NULL, &recording_settings_menu },
#endif
    
#if CONFIG_TUNER
    [GO_TO_FM] =            { radio, NULL, &radio_settings_menu },
#endif
    
    [GO_TO_RECENTBMARKS] =  { load_bmarks, NULL, &bookmark_settings_menu }, 
    [GO_TO_BROWSEPLUGINS] = { plugins_menu, NULL, NULL }, 
    
};
static const int nb_items = sizeof(items)/sizeof(*items);

static int item_callback(int action, const struct menu_item_ex *this_item) ;

MENUITEM_RETURNVALUE(file_browser, ID2P(LANG_DIR_BROWSER), GO_TO_FILEBROWSER,
                        NULL, Icon_file_view_menu);
#ifdef HAVE_TAGCACHE
MENUITEM_RETURNVALUE(db_browser, ID2P(LANG_TAGCACHE), GO_TO_DBBROWSER, 
                        NULL, Icon_Audio);
#endif
MENUITEM_RETURNVALUE(rocks_browser, ID2P(LANG_PLUGINS), GO_TO_BROWSEPLUGINS, 
                        NULL, Icon_Plugin);
static char *get_wps_item_name(int selected_item, void * data, char *buffer)
{
    (void)selected_item; (void)data; (void)buffer;
    if (audio_status())
        return ID2P(LANG_NOW_PLAYING);
    return ID2P(LANG_RESUME_PLAYBACK);
}
MENUITEM_RETURNVALUE_DYNTEXT(wps_item, GO_TO_WPS, NULL, get_wps_item_name, 
                                NULL, NULL, Icon_Playback_menu);
#ifdef HAVE_RECORDING
MENUITEM_RETURNVALUE(rec, ID2P(LANG_RECORDING), GO_TO_RECSCREEN,  
                        NULL, Icon_Recording);
#endif
#if CONFIG_TUNER
MENUITEM_RETURNVALUE(fm, ID2P(LANG_FM_RADIO), GO_TO_FM,  
                        item_callback, Icon_Radio_screen);
#endif
MENUITEM_RETURNVALUE(menu_, ID2P(LANG_SETTINGS), GO_TO_MAINMENU,  
                        NULL, Icon_Submenu_Entered);
MENUITEM_RETURNVALUE(bookmarks, ID2P(LANG_BOOKMARK_MENU_RECENT_BOOKMARKS),
                        GO_TO_RECENTBMARKS,  item_callback, 
                        Icon_Bookmark);
#ifdef HAVE_LCD_CHARCELLS
static int do_shutdown(void)
{
#if CONFIG_CHARGING
    if (charger_inserted())
        charging_splash();
    else
#endif
        sys_poweroff();
    return 0;
}
MENUITEM_FUNCTION(do_shutdown_item, 0, ID2P(LANG_SHUTDOWN),
                  do_shutdown, NULL, NULL, Icon_NOICON);
#endif
MAKE_MENU(root_menu_, ID2P(LANG_ROCKBOX_TITLE),
            item_callback, Icon_Rockbox,
            &bookmarks, &file_browser, 
#ifdef HAVE_TAGCACHE
            &db_browser,
#endif
            &wps_item, &menu_, 
#ifdef HAVE_RECORDING
            &rec, 
#endif
#if CONFIG_TUNER
            &fm,
#endif
            &playlist_options, &rocks_browser,  &info_menu

#ifdef HAVE_LCD_CHARCELLS
            ,&do_shutdown_item
#endif
        );

static int item_callback(int action, const struct menu_item_ex *this_item) 
{
    switch (action)
    {
        case ACTION_TREE_STOP:
            return ACTION_REDRAW;
        case ACTION_REQUEST_MENUITEM:
#if CONFIG_TUNER
            if (this_item == &fm)
            {
                if (radio_hardware_present() == 0)
                    return ACTION_EXIT_MENUITEM;
            }
            else 
#endif
                if (this_item == &bookmarks)
            {
                if (global_settings.usemrb == 0)
                    return ACTION_EXIT_MENUITEM;
            }
        break;
    }
    return action;
}
static int get_selection(int last_screen)
{
    unsigned int i;
    for(i=0; i< sizeof(root_menu__)/sizeof(*root_menu__); i++)
    {
        if (((root_menu__[i]->flags&MENU_TYPE_MASK) == MT_RETURN_VALUE) && 
            (root_menu__[i]->value == last_screen))
        {
            return i;
        }
    }
    return 0;
}

static inline int load_screen(int screen)
{
    /* set the global_status.last_screen before entering,
        if we dont we will always return to the wrong screen on boot */
    int old_previous = last_screen;
    int ret_val;
    if (screen <= GO_TO_ROOT)
        return screen;
    if (screen == old_previous)
        old_previous = GO_TO_ROOT;
    global_status.last_screen = (char)screen;
    status_save();
    ret_val = items[screen].function(items[screen].param);
    last_screen = screen;
    if (ret_val == GO_TO_PREVIOUS)
        last_screen = old_previous;
    return ret_val;
}
static int load_context_screen(int selection)
{
    const struct menu_item_ex *context_menu = NULL;
    if ((root_menu__[selection]->flags&MENU_TYPE_MASK) == MT_RETURN_VALUE)
    {
        int item = root_menu__[selection]->value;
        context_menu = items[item].context_menu;
    }
    /* special cases */
    else if (root_menu__[selection] == &info_menu)
    {
        context_menu = &system_menu;
    }
    
    if (context_menu)
        return do_menu(context_menu, NULL, NULL, false);
    else
        return GO_TO_PREVIOUS;
}

static int previous_music = GO_TO_WPS;

void previous_music_is_wps(void)
{
    previous_music = GO_TO_WPS;
}

void root_menu(void)
{
    int previous_browser = GO_TO_FILEBROWSER;
    int next_screen = GO_TO_ROOT;
    int selected = 0;

    if (global_status.last_screen == (char)-1)
        global_status.last_screen = GO_TO_ROOT;
    
    if (global_settings.start_in_screen == 0)
        next_screen = (int)global_status.last_screen;
    else next_screen = global_settings.start_in_screen - 2;
    
#ifdef HAVE_RTC_ALARM
    if ( rtc_check_alarm_started(true) ) 
    {
        rtc_enable_alarm(false);
        next_screen = GO_TO_WPS;
#if CONFIG_TUNER
        if (global_settings.alarm_wake_up_screen == ALARM_START_FM)
            next_screen = GO_TO_FM;
#endif
#ifdef HAVE_RECORDING
        if (global_settings.alarm_wake_up_screen == ALARM_START_REC)
        {
            recording_start_automatic = true;
            next_screen = GO_TO_RECSCREEN;
        }
#endif
    }
#endif /* HAVE_RTC_ALARM */

#ifdef HAVE_HEADPHONE_DETECTION
    if (next_screen == GO_TO_WPS && 
        (global_settings.unplug_autoresume && !headphones_inserted() ))
            next_screen = GO_TO_ROOT;
#endif

    while (true)
    {
        switch (next_screen)
        {
            case MENU_ATTACHED_USB:
            case MENU_SELECTED_EXIT:
                /* fall through */
            case GO_TO_ROOT:
                if (last_screen != GO_TO_ROOT)
                    selected = get_selection(last_screen);
                next_screen = do_menu(&root_menu_, &selected, NULL, false);
                if (next_screen != GO_TO_PREVIOUS)
                    last_screen = GO_TO_ROOT;
                break;

            case GO_TO_PREVIOUS:
                next_screen = last_screen;
                break;

            case GO_TO_PREVIOUS_BROWSER:
                next_screen = previous_browser;
                break;

            case GO_TO_PREVIOUS_MUSIC:
                next_screen = previous_music;
                break;
            case GO_TO_ROOTITEM_CONTEXT:
                next_screen = load_context_screen(selected);
                break;
            default:
                if (next_screen == GO_TO_FILEBROWSER 
#ifdef HAVE_TAGCACHE
                    || next_screen == GO_TO_DBBROWSER
#endif
                   )
                    previous_browser = next_screen;
                if (next_screen == GO_TO_WPS 
#if CONFIG_TUNER
                    || next_screen == GO_TO_FM
#endif
                   )
                    previous_music = next_screen;
                next_screen = load_screen(next_screen);
                break;
        } /* switch() */
    }
    return;
}
