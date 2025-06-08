/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include <dir.h>
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
#include "string-extra.h"
#include "lang.h"
#include "menu.h"
#include "misc.h"
#include "open_plugin.h"
#include "tree.h"
#include "splash.h"
#include "pathfuncs.h"
#include "filetypes.h"
#include "shortcuts.h"
#include "onplay.h"
#include "screens.h"
#include "talk.h"
#include "yesno.h"

#define MAX_SHORTCUT_NAME 64
#define SHORTCUTS_HDR "[shortcut]"
#define SHORTCUTS_FILENAME ROCKBOX_DIR "/shortcuts.txt"
static const char * const type_strings[SHORTCUT_TYPE_COUNT] = {
    [SHORTCUT_SETTING] = "setting",
    [SHORTCUT_SETTING_APPLY] = "apply",
    [SHORTCUT_DEBUGITEM] = "debug",
    [SHORTCUT_BROWSER] = "browse",
    [SHORTCUT_PLAYLISTMENU] = "playlist menu",
    [SHORTCUT_SEPARATOR] = "separator",
    [SHORTCUT_SHUTDOWN] = "shutdown",
    [SHORTCUT_REBOOT] = "reboot",
    [SHORTCUT_TIME] = "time",
    [SHORTCUT_FILE] = "file",
};

struct shortcut {
    enum shortcut_type type;
    int8_t icon;
    char name[MAX_SHORTCUT_NAME];
    char talk_clip[MAX_PATH];
    const struct settings_list *setting;
    union {
        char path[MAX_PATH];
        struct {
#if CONFIG_RTC
            bool talktime;
#endif
            int sleep_timeout;
        } timedata;
    } u;
};

#define SHORTCUTS_PER_HANDLE 4
struct shortcut_handle {
    struct shortcut shortcuts[SHORTCUTS_PER_HANDLE];
    int next_handle;
};
static int first_handle = 0;
static int shortcut_count = 0;

static int buflib_move_lock;
static int move_callback(int handle, void *current, void *new)
{
    (void)handle;
    (void)current;
    (void)new;

    if (buflib_move_lock > 0)
        return BUFLIB_CB_CANNOT_MOVE;

    return BUFLIB_CB_OK;
}
static struct buflib_callbacks shortcut_ops = {
    .move_callback = move_callback,
};

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

static struct shortcut_handle * alloc_first_sc_handle(void)
{
    struct shortcut_handle *h = NULL;
#if 0 /* all paths are guarded, compiler doesn't recognize INIT_ATTR callers */
    if (first_handle != 0)
        return h; /* No re-init allowed */
#endif
    first_handle = core_alloc_ex(sizeof(struct shortcut_handle), &shortcut_ops);
    if (first_handle > 0)
    {
        h = core_get_data(first_handle);
        h->next_handle = 0;
    }

    return h;
}

static struct shortcut* get_shortcut(int index, struct shortcut *fail)
{
    int handle_count, handle_index;
    int current_handle = first_handle;
    struct shortcut_handle *h;

    if (first_handle == 0)
    {
        h = alloc_first_sc_handle();
        if (!h)
            return fail;
        current_handle = first_handle;
    }

    handle_count = index/SHORTCUTS_PER_HANDLE + 1;
    handle_index = index%SHORTCUTS_PER_HANDLE;
    do {
        h = core_get_data(current_handle);
        current_handle = h->next_handle;
        handle_count--;
        if(handle_count <= 0)
            return &h->shortcuts[handle_index];
    } while (current_handle > 0);

    if (handle_index == 0)
    {
        /* prevent invalidation of 'h' during compaction */
        ++buflib_move_lock;
        h->next_handle = core_alloc_ex(sizeof(struct shortcut_handle), &shortcut_ops);
        --buflib_move_lock;
        if (h->next_handle <= 0)
            return fail;
        h = core_get_data(h->next_handle);
        h->next_handle = 0;
    }
    return &h->shortcuts[handle_index];
}

static void remove_shortcut(int index)
{
    int this = index, next = index + 1;
    struct shortcut *prev = get_shortcut(this, NULL);

    while (next <= shortcut_count)
    {
        struct shortcut *sc = get_shortcut(next, NULL);
        memcpy(prev, sc, sizeof(struct shortcut));
        next++;
        prev = sc;
    }
    shortcut_count--;
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
        case SHORTCUT_SETTING_APPLY:
        case SHORTCUT_SETTING:
            return sc->setting != NULL;
        case SHORTCUT_TIME:
        case SHORTCUT_DEBUGITEM:
        case SHORTCUT_SEPARATOR:
        case SHORTCUT_SHUTDOWN:
        case SHORTCUT_REBOOT:
        default:
            break;
    }
    return true;
}

static void init_shortcut(struct shortcut* sc)
{
    memset(sc, 0, sizeof(*sc));
    sc->type = SHORTCUT_UNDEFINED;
    sc->icon = Icon_NOICON;
}

static int first_idx_to_writeback = -1;
static bool overwrite_shortcuts = false;
static void shortcuts_ata_idle_callback(void)
{
    int fd;

    int current_idx = first_idx_to_writeback;
    int append = overwrite_shortcuts ? O_TRUNC : O_APPEND;

    if (first_idx_to_writeback < 0)
        return;
    fd = open(SHORTCUTS_FILENAME, append|O_RDWR|O_CREAT, 0644);
    if (fd < 0)
        return;

    /* ensure pointer returned by get_shortcut()
       survives the yield() of write() */
    ++buflib_move_lock;

    while (current_idx < shortcut_count)
    {
        struct shortcut* sc = get_shortcut(current_idx++, NULL);
        const char *type;

        if (!sc)
            break;
        type = type_strings[sc->type];

        fdprintf(fd, SHORTCUTS_HDR "\ntype: %s\ndata: ", type);

        if (sc->type == SHORTCUT_TIME)
        {
#if CONFIG_RTC
            if (sc->u.timedata.talktime)
                write(fd, "talk", 4);
            else
#endif
            {
                write(fd, "sleep", 5);
                if(sc->u.timedata.sleep_timeout >= 0)
                    fdprintf(fd, " %d", sc->u.timedata.sleep_timeout);
            }
        }
        else if (sc->type == SHORTCUT_SETTING_APPLY)
        {
            fdprintf(fd, "%s: %s", sc->setting->cfg_name, sc->u.path);
        }
        else if (sc->type == SHORTCUT_SETTING)
        {
            write(fd, sc->setting->cfg_name, strlen(sc->setting->cfg_name));
        }
        else
            write(fd, sc->u.path, strlen(sc->u.path));

        /* write name:, icon:, talkclip: */
        fdprintf(fd, "\nname: %s\nicon: %d\ntalkclip: %s\n\n",
                 sc->name, sc->icon, sc->talk_clip);
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
    --buflib_move_lock;
    first_idx_to_writeback = -1;
}

void shortcuts_add(enum shortcut_type type, const char* value)
{
    struct shortcut* sc = get_shortcut(shortcut_count++, NULL);
    if (!sc)
        return;
    init_shortcut(sc);
    sc->type = type;
    if (type == SHORTCUT_SETTING || type == SHORTCUT_SETTING_APPLY)
    {
        sc->setting = (void*)value;
        /* write the current value, will be ignored for SHORTCUT_SETTING */
        cfg_to_string(sc->setting, sc->u.path,  sizeof(sc->u.path));
    }
    else
        strmemccpy(sc->u.path, value, MAX_PATH);

    if (first_idx_to_writeback < 0)
        first_idx_to_writeback = shortcut_count - 1;
    overwrite_shortcuts = false;
    register_storage_idle_func(shortcuts_ata_idle_callback);
}

static int readline_cb(int n, char *buf, void *parameters)
{
    (void)n;
    (void)parameters;
    struct shortcut **param = (struct shortcut**)parameters;
    struct shortcut* sc = *param;
    char *name, *value;

    buf = skip_whitespace(buf);

    if (!strcasecmp(buf, SHORTCUTS_HDR))
    {
        if (sc && verify_shortcut(sc))
            shortcut_count++;
        sc = get_shortcut(shortcut_count, NULL);
        if (!sc)
            return 1;
        init_shortcut(sc);
        *param = sc;
    }
    else if (sc && settings_parseline(buf, &name, &value))
    {
        static const char * const nm_options[] = {"type", "name", "data",
                                                  "icon", "talkclip", NULL};
        int nm_op = string_option(name, nm_options, false);

        if (nm_op == 0) /*type*/
        {
            int t = 0;
            for (t=0; t<SHORTCUT_TYPE_COUNT && sc->type == SHORTCUT_UNDEFINED; t++)
                if (!strcmp(value, type_strings[t]))
                    sc->type = t;
        }
        else if (nm_op == 1) /*name*/
        {
            strmemccpy(sc->name, value, MAX_SHORTCUT_NAME);
        }
        else if (nm_op == 2) /*data*/
        {
            switch (sc->type)
            {
                case SHORTCUT_UNDEFINED:
                case SHORTCUT_TYPE_COUNT:
                    *param = NULL;
                    break;
                case SHORTCUT_BROWSER:
                {
                    char *p = strmemccpy(sc->u.path, value, MAX_PATH);
                    if (p && dir_exists(value))
                    {
                        /* ensure ending slash */
                        *p = '\0';
                        if (*(p-2) != '/')
                            *(p-1) = '/';
                    }
                    break;
                }
                case SHORTCUT_FILE:
                case SHORTCUT_DEBUGITEM:
                case SHORTCUT_PLAYLISTMENU:
                {
                    strmemccpy(sc->u.path, value, MAX_PATH);
                    break;
                }
                case SHORTCUT_SETTING_APPLY:
                case SHORTCUT_SETTING:
                    /* can handle 'name: value' pair for either type */
                    if (settings_parseline(value, &name, &value))
                    {
                        sc->setting = find_setting_by_cfgname(name);
                        strmemccpy(sc->u.path, value, MAX_PATH);
                    }
                    else /* force SHORTCUT_SETTING, no 'name: value' pair */
                    {
                        sc->type = SHORTCUT_SETTING;
                        sc->setting = find_setting_by_cfgname(value);
                    }
                    break;
                case SHORTCUT_TIME:
#if CONFIG_RTC
                    sc->u.timedata.talktime = false;
                    if (!strcasecmp(value, "talk"))
                        sc->u.timedata.talktime = true;
                    else
#endif
                    if (!strncasecmp(value, "sleep", sizeof("sleep")-1))
                    {
                        /* 'sleep' may appear alone or followed by number after a space */
                        if (strlen(value) > sizeof("sleep")) /* sizeof 1 larger (+space chr..) */
                            sc->u.timedata.sleep_timeout = atoi(&value[sizeof("sleep")-1]);
                        else
                            sc->u.timedata.sleep_timeout = -1;
                    }
                    else
                        sc->type = SHORTCUT_UNDEFINED; /* error */
                    break;
                case SHORTCUT_SEPARATOR:
                case SHORTCUT_SHUTDOWN:
                case SHORTCUT_REBOOT:
                    break;
            }
        }
        else if (nm_op == 3) /*icon*/
        {
            if (!strcmp(value, "filetype") && sc->type != SHORTCUT_SETTING
                && sc->type != SHORTCUT_SETTING_APPLY && sc->u.path[0])
            {
                sc->icon = filetype_get_icon(filetype_get_attr(sc->u.path));
            }
            else
            {
                sc->icon = atoi(value);
            }
        }
        else if (nm_op == 4) /*talkclip*/
        {
            strmemccpy(sc->talk_clip, value, MAX_PATH);
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
    h = alloc_first_sc_handle();
    if (!h) {
        close(fd);
        return;
    }

    /* we enter readline_cb() multiple times with a buffer
       obtained when we encounter a SHORTCUTS_HDR ("[shortcut]") section.
       fast_readline() might yield() -> protect buffer */
    ++buflib_move_lock;

    fast_readline(fd, buf, sizeof buf, &param, readline_cb);
    close(fd);
    if (param && verify_shortcut(param))
        shortcut_count++;

    /* invalidate at scope end since "param" contains
       the shortcut pointer */
    --buflib_move_lock;
}

static const char * shortcut_menu_get_name(int selected_item, void * data,
                                           char * buffer, size_t buffer_len)
{
    (void)data;
    struct shortcut *sc = get_shortcut(selected_item, NULL);
    if (!sc)
        return "";
    const char *ret = sc->u.path;

    if (sc->type == SHORTCUT_SETTING && sc->name[0] == '\0')
        return P2STR(ID2P(sc->setting->lang_id));
    else if (sc->type == SHORTCUT_SETTING_APPLY && sc->name[0] == '\0')
    {
        snprintf(buffer, buffer_len, "%s (%s)",
                 P2STR(ID2P(sc->setting->lang_id)), sc->u.path);
        return buffer;
    }
    else if (sc->type == SHORTCUT_SEPARATOR)
        return sc->name;
    else if (sc->type == SHORTCUT_TIME)
    {
        if (sc->u.timedata.sleep_timeout < 0
#if CONFIG_RTC
            && !sc->u.timedata.talktime
#endif
        ) /* String representation for toggling sleep timer */
            return string_sleeptimer(buffer, buffer_len);

        if (sc->name[0])
            return sc->name;

#if CONFIG_RTC
        if (sc->u.timedata.talktime)
            return P2STR(ID2P(LANG_CURRENT_TIME));
        else
#endif
        {
            format_sleeptimer(sc->name, sizeof(sc->name),
                              sc->u.timedata.sleep_timeout, NULL);
            snprintf(buffer, buffer_len, "%s (%s)",
                     P2STR(ID2P(LANG_SLEEP_TIMER)),
                     sc->name[0] ? sc->name : P2STR(ID2P(LANG_OFF)));
            sc->name[0] = '\0';
            return buffer;
        }

    }
    else if ((sc->type == SHORTCUT_SHUTDOWN || sc->type == SHORTCUT_REBOOT) &&
             sc->name[0] == '\0')
    {
        /* No translation support as only soft_shutdown has LANG_SHUTDOWN defined */
        return type_strings[sc->type];
    }
    else if (sc->type == SHORTCUT_BROWSER && sc->name[0] == '\0' && (sc->u.path)[0] != '\0')
    {
        char* pos;
        if (path_basename(sc->u.path, (const char **)&pos) > 0)
        {
            if (snprintf(buffer, buffer_len, "%s (%s)", pos, sc->u.path) < (int)buffer_len)
                return buffer;
        }
    }

    return sc->name[0] ? sc->name : ret;
}

static int shortcut_menu_speak_item(int selected_item, void * data)
{
    (void)data;
    struct shortcut *sc = get_shortcut(selected_item, NULL);
    if (sc)
    {
        if (sc->talk_clip[0])
        {
            talk_file(NULL, NULL, sc->talk_clip, NULL, NULL, false);
        }
        else
        {
            switch (sc->type)
            {
            case SHORTCUT_BROWSER:
                {
                    DIR* dir;
                    struct dirent* entry;
                    char* slash = strrchr(sc->u.path, PATH_SEPCH);
                    char* filename = slash + 1;
                    if (slash && *filename != '\0')
                    {
                        *slash = '\0'; /* terminate the path to open the directory */
                        dir = opendir(sc->u.path);
                        *slash = PATH_SEPCH; /* restore fullpath */
                        if (dir)
                        {
                            while (0 != (entry = readdir(dir)))
                            {
                                if (!strcmp(entry->d_name, filename))
                                {
                                    struct dirinfo info = dir_get_info(dir, entry);

                                    if (info.attribute & ATTR_DIRECTORY)
                                        talk_dir_or_spell(sc->u.path, NULL, false);
                                    else
                                        talk_file_or_spell(NULL, sc->u.path, NULL, false);

                                    closedir(dir);
                                    return 0;
                                }
                            }
                            closedir(dir);
                        }
                    }
                    else
                    {
                        talk_dir_or_spell(sc->u.path, NULL, false);
                        break;
                    }
                    talk_spell(sc->u.path, false);
                }
                break;
            case SHORTCUT_FILE:
            case SHORTCUT_PLAYLISTMENU:
                talk_file_or_spell(NULL, sc->u.path, NULL, false);
                break;
            case SHORTCUT_SETTING_APPLY:
            case SHORTCUT_SETTING:
                talk_id(sc->setting->lang_id, false);
                if (sc->type == SHORTCUT_SETTING_APPLY)
                    talk_spell(sc->u.path, true);
                break;

            case SHORTCUT_TIME:
#if CONFIG_RTC
                if (sc->u.timedata.talktime)
                    talk_timedate();
                else
#endif
                if (sc->name[0] && sc->u.timedata.sleep_timeout >= 0)
                    talk_spell(sc->name, false);
                else
                    talk_sleeptimer(sc->u.timedata.sleep_timeout);
                break;
            case SHORTCUT_SHUTDOWN:
            case SHORTCUT_REBOOT:
                if (!sc->name[0])
                {
                    talk_spell(type_strings[sc->type], false);
                    break;
                }
                /* fall-through */
            default:
                talk_spell(sc->name[0] ? sc->name : sc->u.path, false);
                break;
            }
        }
    }
    return 0;
}

static int shortcut_menu_get_action(int action, struct gui_synclist *lists)
{
    (void)lists;
    if (action == ACTION_STD_CONTEXT)
    {
        int selection = gui_synclist_get_sel_pos(lists);

        if (confirm_delete_yesno("") != YESNO_YES)
        {
            gui_synclist_set_title(lists, lists->title, lists->title_icon);
            shortcut_menu_speak_item(selection, NULL);
            return ACTION_REDRAW;
        }

        remove_shortcut(selection);
        gui_synclist_set_nb_items(lists, shortcut_count);
        gui_synclist_set_title(lists, lists->title, lists->title_icon);
        if (selection >= shortcut_count)
            gui_synclist_select_item(lists, shortcut_count - 1);
        first_idx_to_writeback = 0;
        overwrite_shortcuts = true;
        shortcuts_ata_idle_callback();

        if (shortcut_count > 0)
        {
            shortcut_menu_speak_item(gui_synclist_get_sel_pos(lists), NULL);
            return ACTION_REDRAW;
        }

        return ACTION_STD_CANCEL;
    }

    if (action == ACTION_STD_OK
     || action == ACTION_STD_MENU
     || action == ACTION_STD_QUICKSCREEN)
    {
        return ACTION_STD_CANCEL;
    }

    return action;
}

static enum themable_icons shortcut_menu_get_icon(int selected_item, void * data)
{
    static const int8_t type_icons[SHORTCUT_TYPE_COUNT] = {
        [SHORTCUT_SETTING] = Icon_Menu_setting,
        [SHORTCUT_SETTING_APPLY] = Icon_Queued,
        [SHORTCUT_DEBUGITEM] = Icon_Menu_functioncall,
        [SHORTCUT_BROWSER] = Icon_Folder,
        [SHORTCUT_PLAYLISTMENU] = Icon_Playlist,
        [SHORTCUT_SEPARATOR] = Icon_NOICON,
        [SHORTCUT_SHUTDOWN] = Icon_System_menu,
        [SHORTCUT_REBOOT] = Icon_System_menu,
        [SHORTCUT_TIME] = Icon_Menu_functioncall,
        [SHORTCUT_FILE] = Icon_NOICON,
    };

    (void)data;
    int icon;
    struct shortcut *sc = get_shortcut(selected_item, NULL);
    if (!sc)
        return Icon_NOICON;
    if (sc->icon == Icon_NOICON)
    {
        if (sc->type == SHORTCUT_BROWSER || sc->type == SHORTCUT_FILE)
        {
            icon = filetype_get_icon(filetype_get_attr(sc->u.path));
            if (icon > 0)
                return icon;
        }
        /* excluding SHORTCUT_UNDEFINED (-1) */
        if (sc->type >= 0 && sc->type < SHORTCUT_TYPE_COUNT)
            return type_icons[sc->type];
    }
    return sc->icon;
}

static void apply_new_setting(const struct settings_list *setting)
{
    settings_apply(false);
    if ((setting->flags & (F_THEMESETTING|F_NEEDAPPLY)) == (F_THEMESETTING|F_NEEDAPPLY))
    {
            settings_apply_skins();
    }
    if (setting->setting == &global_settings.sleeptimer_duration && get_sleep_timer())
    {
        set_sleeptimer_duration(global_settings.sleeptimer_duration);
    }
}

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
    list.title_icon = Icon_Bookmark;

    if (shortcut_count == 0)
    {
        splash(HZ, ID2P(LANG_NO_FILES));
        return GO_TO_PREVIOUS;
    }
    push_current_activity(ACTIVITY_SHORTCUTSMENU);

    /* prevent buffer moves while the menu is active.
       Playing talk files or other I/O actions call yield() */
    ++buflib_move_lock;

    while (done == GO_TO_PREVIOUS)
    {
        list.count = shortcut_count; /* in case shortcut was deleted */
        list.title = P2STR(ID2P(LANG_SHORTCUTS)); /* in case language changed */
        list.get_icon = global_settings.show_icons ? shortcut_menu_get_icon : NULL;
        list.get_talk = global_settings.talk_menu ? shortcut_menu_speak_item : NULL;

        if (simplelist_show_list(&list))
            break; /* some error happened?! */

        if (list.selection == -1)
            break;
        else
        {
            sc = get_shortcut(list.selection, NULL);

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
                        onplay_show_playlist_menu(sc->u.path,
                                                  dir_exists(sc->u.path) ? ATTR_DIRECTORY :
                                                  filetype_get_attr(sc->u.path),
                                                  NULL);
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
                    if(open_plugin_add_path(ID2P(LANG_SHORTCUTS), sc->u.path, NULL) != 0)
                    {
                        done = GO_TO_PLUGIN;
                        break;
                    }
                    struct browse_context browse = {
                        .dirfilter = global_settings.dirfilter,
                        .icon = Icon_NOICON,
                        .root = sc->u.path,
                    };
                    if (sc->type == SHORTCUT_FILE)
                        browse.flags |= BROWSE_RUNFILE;
                    done = rockbox_browse(&browse);

                }
                break;
                case SHORTCUT_SETTING_APPLY:
                {
                    bool theme_changed;
                    string_to_cfg(sc->setting->cfg_name, sc->u.path, &theme_changed);
                    settings_save();
                    apply_new_setting(sc->setting);
                    break;
                }
                case SHORTCUT_SETTING:
                {
                    do_setting_screen(sc->setting,
                        sc->name[0] ? sc->name : P2STR(ID2P(sc->setting->lang_id)),NULL);
                    apply_new_setting(sc->setting);
                    break;
                }
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
                case SHORTCUT_REBOOT:
#if CONFIG_CHARGING
                    if (charger_inserted())
                        charging_splash();
                    else
#endif
                        sys_reboot();
                    break;
                case SHORTCUT_TIME:
#if CONFIG_RTC
                  if (!sc->u.timedata.talktime)
#endif
                    {
                        char timer_buf[10];
                        if (sc->u.timedata.sleep_timeout >= 0)
                        {
                            set_sleeptimer_duration(sc->u.timedata.sleep_timeout);
                            splashf(HZ, "%s (%s)", str(LANG_SLEEP_TIMER),
                                    format_sleeptimer(timer_buf, sizeof(timer_buf),
                                                          sc->u.timedata.sleep_timeout,
                                                          NULL));
                        }
                        else
                            toggle_sleeptimer();
                    }
                    break;
                case SHORTCUT_UNDEFINED:
                default:
                    break;
            }
        }
    }
    if (GO_TO_PLUGIN == done)
        pop_current_activity_without_refresh();
    else
        pop_current_activity();
    --buflib_move_lock;

    return done;
}
