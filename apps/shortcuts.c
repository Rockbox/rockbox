/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Jonathan Gordon
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
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "powermgmt.h"
#include "power.h"
#include "action.h"
#include "ata_idle_notify.h"
#include "debug_menu.h"
#include "core_alloc.h"
#include "list.h"
#include "settings.h"
#include "settings_list.h"
#include "lang.h"
#include "menu.h"
#include "misc.h"
#include "tree.h"
#include "splash.h"
#include "filefuncs.h"
#include "filetypes.h"
#include "shortcuts.h"
#include "onplay.h"
#include "screens.h"
#include "talk.h"


#define MAX_SHORTCUT_NAME 32
#define SHORTCUTS_FILENAME ROCKBOX_DIR "/shortcuts.txt"
static const char * const type_strings[SHORTCUT_TYPE_COUNT] = {
    [SHORTCUT_SETTING] = "setting",
    [SHORTCUT_FILE] = "file",
    [SHORTCUT_DEBUGITEM] = "debug",
    [SHORTCUT_BROWSER] = "browse",
    [SHORTCUT_PLAYLISTMENU] = "playlist menu",
    [SHORTCUT_SEPARATOR] = "separator",
    [SHORTCUT_SHUTDOWN] = "shutdown",
    [SHORTCUT_TIME] = "time",
};

struct shortcut {
    enum shortcut_type type;
    char name[MAX_SHORTCUT_NAME];
    char talk_clip[MAX_PATH];
    int icon;
    union {
        char path[MAX_PATH];
        const struct settings_list *setting;
        struct {
#if CONFIG_RTC
            bool talktime;
#endif
            int sleep_timeout;
        } timedata;
    } u;
};
#define SHORTCUTS_PER_HANDLE 32
struct shortcut_handle {
    struct shortcut shortcuts[SHORTCUTS_PER_HANDLE];
    int next_handle;
};
static int first_handle = 0;
static int shortcut_count = 0;

static void reset_shortcuts(void)
{
    int current_handle = first_handle;
    struct shortcut_handle *h = NULL;
    while (current_handle > 0)
    {
        int next;
        h = core_get_data(current_handle);
        next = h->next_handle;
        core_free(current_handle);
        current_handle = next;
    }
    first_handle = 0;
    shortcut_count = 0;
}   

static struct shortcut* get_shortcut(int index)
{
    int handle_count, handle_index;
    int current_handle = first_handle;
    struct shortcut_handle *h = NULL;
    
    if (first_handle == 0)
    {
        first_handle = core_alloc("shortcuts_head", sizeof(struct shortcut_handle));
        if (first_handle <= 0)
            return NULL;
        h = core_get_data(first_handle);
        h->next_handle = 0;
        current_handle = first_handle;
    }

    handle_count = index/SHORTCUTS_PER_HANDLE + 1;
    handle_index = index%SHORTCUTS_PER_HANDLE;
    do {
        h = core_get_data(current_handle);
        current_handle = h->next_handle;
        handle_count--;
    } while (handle_count > 0 && current_handle > 0);
    if (handle_count > 0 && handle_index == 0)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "shortcuts_%d", index/SHORTCUTS_PER_HANDLE);
        h->next_handle = core_alloc(buf, sizeof(struct shortcut_handle));
        if (h->next_handle <= 0)
            return NULL;
        h = core_get_data(h->next_handle);
        h->next_handle = 0;
    }
    return &h->shortcuts[handle_index];
}

static bool verify_shortcut(struct shortcut* sc)
{
    switch (sc->type)
    {
        case SHORTCUT_UNDEFINED:
            return false;
        case SHORTCUT_BROWSER:
        case SHORTCUT_FILE:
        case SHORTCUT_PLAYLISTMENU:
            return sc->u.path[0] != '\0';
        case SHORTCUT_SETTING:
            return sc->u.setting != NULL;
        case SHORTCUT_TIME:
            return sc->name[0] != '\0';
        case SHORTCUT_DEBUGITEM:
        case SHORTCUT_SEPARATOR:
        case SHORTCUT_SHUTDOWN:
        default:
            break;
    }
    return true;
}

static void init_shortcut(struct shortcut* sc)
{
    sc->type = SHORTCUT_UNDEFINED;
    sc->name[0] = '\0';
    sc->u.path[0] = '\0';
    sc->talk_clip[0] = '\0';
    sc->icon = Icon_NOICON;
}

static int first_idx_to_writeback = -1;
static void shortcuts_ata_idle_callback(void* data)
{
    (void)data;
    int fd;
    char buf[MAX_PATH];
    int current_idx = first_idx_to_writeback;
    if (first_idx_to_writeback < 0)
        return;
    fd = open(SHORTCUTS_FILENAME, O_APPEND|O_RDWR|O_CREAT, 0644);
    if (fd < 0)
        return;
    while (current_idx < shortcut_count)
    {
        struct shortcut* sc = get_shortcut(current_idx++);
        const char *type;
        int len;
        if (!sc)
            break;
        type = type_strings[sc->type];
        len = snprintf(buf, MAX_PATH, "[shortcut]\ntype: %s\ndata: ", type);
        write(fd, buf, len);
        if (sc->type == SHORTCUT_SETTING)
            write(fd, sc->u.setting->cfg_name, strlen(sc->u.setting->cfg_name));
        else
            write(fd, sc->u.path, strlen(sc->u.path));
        write(fd, "\n\n", 2);
    }
    close(fd);
    if (first_idx_to_writeback == 0)
    {
        /* reload all shortcuts because we appended to the shortcuts file which
         * has not been read yet.
         */
         reset_shortcuts();
         shortcuts_init();
    }
    first_idx_to_writeback = -1;
}

void shortcuts_add(enum shortcut_type type, const char* value)
{
    struct shortcut* sc = get_shortcut(shortcut_count++);
    if (!sc)
        return;
    init_shortcut(sc);
    sc->type = type;
    if (type == SHORTCUT_SETTING)
        sc->u.setting = (void*)value;
    else
        strlcpy(sc->u.path, value, MAX_PATH);
    if (first_idx_to_writeback < 0)
        first_idx_to_writeback = shortcut_count - 1;
    register_storage_idle_func(shortcuts_ata_idle_callback);
}

static int readline_cb(int n, char *buf, void *parameters)
{
    (void)n;
    (void)parameters;
    struct shortcut **param = (struct shortcut**)parameters;
    struct shortcut* sc = *param;
    char *name, *value;

    if (!strcasecmp(skip_whitespace(buf), "[shortcut]"))
    {
        if (sc && verify_shortcut(sc))
            shortcut_count++;
        sc = get_shortcut(shortcut_count);
        if (!sc)
            return 1;
        init_shortcut(sc);
        *param = sc;
    }
    else if (sc && settings_parseline(buf, &name, &value))
    {
        if (!strcmp(name, "type"))
        {
            int t = 0;
            for (t=0; t<SHORTCUT_TYPE_COUNT && sc->type == SHORTCUT_UNDEFINED; t++)
                if (!strcmp(value, type_strings[t]))
                    sc->type = t;
        }
        else if (!strcmp(name, "name"))
        {
            strlcpy(sc->name, value, MAX_SHORTCUT_NAME);
        }
        else if (!strcmp(name, "data"))
        {
            switch (sc->type)
            {
                case SHORTCUT_UNDEFINED:
                case SHORTCUT_TYPE_COUNT:
                    *param = NULL;
                    break;
                case SHORTCUT_BROWSER:
                case SHORTCUT_FILE:
                case SHORTCUT_DEBUGITEM:
                case SHORTCUT_PLAYLISTMENU:
                    strlcpy(sc->u.path, value, MAX_PATH);
                    break;
                case SHORTCUT_SETTING:
                    sc->u.setting = find_setting_by_cfgname(value, NULL);
                    break;
                case SHORTCUT_TIME:
#if CONFIG_RTC
                    sc->u.timedata.talktime = false;
                    if (!strcasecmp(value, "talk"))
                        sc->u.timedata.talktime = true;
                    else
#endif
                    if (!strncasecmp(value, "sleep ", strlen("sleep ")))
                        sc->u.timedata.sleep_timeout = atoi(&value[strlen("sleep ")]);
                    else
                        sc->type = SHORTCUT_UNDEFINED; /* error */
                    break;
                case SHORTCUT_SEPARATOR:
                case SHORTCUT_SHUTDOWN:
                    break;
            }
        }
        else if (!strcmp(name, "icon"))
        {
            if (!strcmp(value, "filetype") && sc->type != SHORTCUT_SETTING && sc->u.path[0])
            {
                sc->icon = filetype_get_icon(filetype_get_attr(sc->u.path));
            }
            else
            {
                sc->icon = atoi(value);
            }
        }
        else if (!strcmp(name, "talkclip"))
        {
            strlcpy(sc->talk_clip, value, MAX_PATH);
        }
    }
    return 0;
}

void shortcuts_init(void)
{
    int fd;
    char buf[512];
    struct shortcut *param = NULL;
    struct shortcut_handle *h;
    shortcut_count = 0;
    fd = open_utf8(SHORTCUTS_FILENAME, O_RDONLY);
    if (fd < 0)
        return;
    first_handle = core_alloc("shortcuts_head", sizeof(struct shortcut_handle));
    if (first_handle <= 0)
        return;
    h = core_get_data(first_handle);
    h->next_handle = 0;
    fast_readline(fd, buf, sizeof buf, &param, readline_cb);
    close(fd);
    if (param && verify_shortcut(param))
        shortcut_count++;
}

static const char * shortcut_menu_get_name(int selected_item, void * data,
                                           char * buffer, size_t buffer_len)
{
    (void)data;
    (void)buffer;
    (void)buffer_len;
    struct shortcut *sc = get_shortcut(selected_item);
    if (!sc)
        return "";
    if (sc->type == SHORTCUT_SETTING)
        return sc->name[0] ? sc->name : P2STR(ID2P(sc->u.setting->lang_id));
    else if (sc->type == SHORTCUT_SEPARATOR || sc->type == SHORTCUT_TIME)
        return sc->name;
    else if (sc->type == SHORTCUT_SHUTDOWN && sc->name[0] == '\0')
    {
        /* No translation support as only soft_shutdown has LANG_SHUTDOWN defined */
        return type_strings[SHORTCUT_SHUTDOWN];
    }
    return sc->name[0] ? sc->name : sc->u.path;
}

static int shortcut_menu_get_action(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (action == ACTION_STD_OK)
        return ACTION_STD_CANCEL;
    return action;
}

static enum themable_icons shortcut_menu_get_icon(int selected_item, void * data)
{
    (void)data;
    struct shortcut *sc = get_shortcut(selected_item);
    if (!sc)
        return Icon_NOICON;
    if (sc->icon == Icon_NOICON)
    {
        switch (sc->type)
        {
            case SHORTCUT_FILE:
                return filetype_get_icon(filetype_get_attr(sc->u.path));
            case SHORTCUT_BROWSER:
                return Icon_Folder;
            case SHORTCUT_SETTING:
                return Icon_Menu_setting;
            case SHORTCUT_DEBUGITEM:
                return Icon_Menu_functioncall;
            case SHORTCUT_PLAYLISTMENU:
                return Icon_Playlist;
            case SHORTCUT_SHUTDOWN:
                return Icon_System_menu;
            case SHORTCUT_TIME:
                return Icon_Menu_functioncall;
            default:
                break;
        }
    }
    return sc->icon;
}

static int shortcut_menu_speak_item(int selected_item, void * data)
{
    (void)data;
    struct shortcut *sc = get_shortcut(selected_item);
    if (sc && sc->talk_clip[0])
        talk_file(NULL, NULL, sc->talk_clip, NULL, NULL, false);
    return 0;
}

void talk_timedate(void);
const char* sleep_timer_formatter(char* buffer, size_t buffer_size,
                                  int value, const char* unit);

int do_shortcut_menu(void *ignored)
{
    (void)ignored;
    struct simplelist_info list;
    struct shortcut *sc;
    int done = GO_TO_PREVIOUS;
    if (first_handle == 0)
        shortcuts_init();
    simplelist_info_init(&list, P2STR(ID2P(LANG_SHORTCUTS)), shortcut_count, NULL);
    list.get_name = shortcut_menu_get_name;
    list.action_callback = shortcut_menu_get_action;
    if (global_settings.show_icons)
        list.get_icon = shortcut_menu_get_icon;
    list.title_icon = Icon_Bookmark;
    if (global_settings.talk_menu)
        list.get_talk = shortcut_menu_speak_item;

    if (shortcut_count == 0)
    {
        splash(HZ, str(LANG_NO_FILES));
        return GO_TO_PREVIOUS;
    }
    push_current_activity(ACTIVITY_SHORTCUTSMENU);

    while (done == GO_TO_PREVIOUS)
    {
        if (simplelist_show_list(&list))
            break; /* some error happened?! */
        if (list.selection == -1)
            break;
        else
        {
            sc = get_shortcut(list.selection);
            if (!sc)
                continue;
            switch (sc->type)
            {
                case SHORTCUT_PLAYLISTMENU:
                    if (!file_exists(sc->u.path))
                    {
                        splash(HZ, ID2P(LANG_NO_FILES));
                        break;
                    }
                    else
                    {
                        onplay_show_playlist_menu(sc->u.path);
                    }
                    break;
                case SHORTCUT_FILE:
                    if (!file_exists(sc->u.path))
                    {
                        splash(HZ, ID2P(LANG_NO_FILES));
                        break;
                    }
                    /* else fall through */
                case SHORTCUT_BROWSER:
                {
                    struct browse_context browse;
                    browse_context_init(&browse, global_settings.dirfilter, 0,
                            NULL, NOICON, sc->u.path, NULL);
                    if (sc->type == SHORTCUT_FILE)
                        browse.flags |= BROWSE_RUNFILE;
                    done = rockbox_browse(&browse);
                }
                break;
                case SHORTCUT_SETTING:
                    do_setting_screen(sc->u.setting,
                            sc->name[0] ? sc->name : P2STR(ID2P(sc->u.setting->lang_id)),NULL);
                    break;
                case SHORTCUT_DEBUGITEM:
                    run_debug_screen(sc->u.path);
                    break;
                case SHORTCUT_SHUTDOWN:
#if CONFIG_CHARGING
                    if (charger_inserted())
                        charging_splash();
                    else
#endif
                        sys_poweroff();
                    break;
                case SHORTCUT_TIME:
#if CONFIG_RTC
                  if (sc->u.timedata.talktime) {
                        talk_timedate();
                        talk_force_enqueue_next();
                  } else
#endif
                    {
                        char timer_buf[10];
                        set_sleep_timer(sc->u.timedata.sleep_timeout * 60);
                        splashf(HZ, "%s (%s)", str(LANG_SLEEP_TIMER), 
                                sleep_timer_formatter(timer_buf, sizeof(timer_buf),
                                                      sc->u.timedata.sleep_timeout, NULL));
                    }
                    break;
                case SHORTCUT_UNDEFINED:
                default:
                    break;
            }
        }
    }
    pop_current_activity();
    return done;
}
