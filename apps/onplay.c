/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "debug.h"
#include "lcd.h"
#include "file.h"
#include "audio.h"
#include "menu.h"
#include "lang.h"
#include "playlist.h"
#include "button.h"
#include "kernel.h"
#include "keyboard.h"
#include "mp3data.h"
#include "metadata.h"
#include "screens.h"
#include "tree.h"
#include "settings.h"
#include "playlist_viewer.h"
#include "talk.h"
#include "onplay.h"
#include "filetypes.h"
#include "open_plugin.h"
#include "plugin.h"
#include "bookmark.h"
#include "action.h"
#include "splash.h"
#include "yesno.h"
#include "menus/exported_menus.h"
#include "icons.h"
#include "sound_menu.h"
#include "playlist_menu.h"
#include "playlist_catalog.h"
#ifdef HAVE_TAGCACHE
#include "tagtree.h"
#endif
#include "cuesheet.h"
#include "statusbar-skinned.h"
#include "pitchscreen.h"
#include "viewport.h"
#include "pathfuncs.h"
#include "shortcuts.h"
#include "misc.h"

static int context;
static const char *selected_file = NULL;
static char selected_file_path[MAX_PATH];
static int selected_file_attr = 0;
static int onplay_result = ONPLAY_OK;
static bool in_queue_submenu = false;
static bool (*ctx_current_playlist_insert)(int position, bool queue, bool create_new);
static int (*ctx_add_to_playlist)(const char* playlist, bool new_playlist);
extern struct menu_item_ex file_menu; /* settings_menu.c  */

/* redefine MAKE_MENU so the MENU_EXITAFTERTHISMENU flag can be added easily */
#define MAKE_ONPLAYMENU( name, str, callback, icon, ... )               \
    static const struct menu_item_ex *name##_[]  = {__VA_ARGS__};       \
    static const struct menu_callback_with_desc name##__ = {callback,str,icon};\
    static const struct menu_item_ex name =                             \
        {MT_MENU|MENU_HAS_DESC|MENU_EXITAFTERTHISMENU|                  \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { (void*)name##_},{.callback_and_desc = & name##__}};

/* Used for directory move, copy and delete */
struct dirrecurse_params
{
    char path[MAX_PATH];    /* Buffer for full path */
    size_t append;          /* Append position in 'path' for stack push */
};

enum clipboard_op_flags
{
    PASTE_CUT       = 0x00, /* Is a move (cut) operation (default) */
    PASTE_COPY      = 0x01, /* Is a copy operation */
    PASTE_OVERWRITE = 0x02, /* Overwrite destination */
    PASTE_EXDEV     = 0x04, /* Actually copy/move across volumes */
};

/* result codec of various onplay operations */
enum onplay_result_code
{
    /* Anything < 0 is failure */
    OPRC_SUCCESS   = 0,     /* All operations completed successfully */
    OPRC_NOOP      = 1,     /* Operation didn't need to do anything */
    OPRC_CANCELLED = 2,     /* Operation was cancelled by user */
    OPRC_NOOVERWRT = 3,
};

static struct clipboard
{
    char path[MAX_PATH];    /* Clipped file's path */
    unsigned int attr;      /* Clipped file's attributes */
    unsigned int flags;     /* Operation type flags */
} clipboard;

/* Empty the clipboard */
static void clipboard_clear_selection(struct clipboard *clip)
{
    clip->path[0] = '\0';
    clip->attr    = 0;
    clip->flags   = 0;
}

/* Store the selection in the clipboard */
static bool clipboard_clip(struct clipboard *clip, const char *path,
                           unsigned int attr, unsigned int flags)
{
    /* if it fits it clips */
    if (strmemccpy(clip->path, path, sizeof (clip->path)) != NULL)
    {
        clip->attr = attr;
        clip->flags = flags;
        return true;
    }
    else {
        clipboard_clear_selection(clip);
        return false;
    }
}

/* ----------------------------------------------------------------------- */
/* Displays the bookmark menu options for the user to decide.  This is an  */
/* interface function.                                                     */
/* ----------------------------------------------------------------------- */


static int bookmark_load_menu_wrapper(void)
{
    if (get_current_activity() == ACTIVITY_CONTEXTMENU)  /* get rid of parent activity */
        pop_current_activity_without_refresh(); /* when called from ctxt menu */

    return bookmark_load_menu();
}

static int bookmark_menu_callback(int action,
                                  const struct menu_item_ex *this_item,
                                  struct gui_synclist *this_list);
MENUITEM_FUNCTION(bookmark_create_menu_item, 0,
                  ID2P(LANG_BOOKMARK_MENU_CREATE),
                  bookmark_create_menu,
                  bookmark_menu_callback, Icon_Bookmark);
MENUITEM_FUNCTION(bookmark_load_menu_item, 0,
                  ID2P(LANG_BOOKMARK_MENU_LIST),
                  bookmark_load_menu_wrapper,
                  bookmark_menu_callback, Icon_Bookmark);
MAKE_ONPLAYMENU(bookmark_menu, ID2P(LANG_BOOKMARK_MENU),
                bookmark_menu_callback, Icon_Bookmark,
                &bookmark_create_menu_item, &bookmark_load_menu_item);
static int bookmark_menu_callback(int action,
                                  const struct menu_item_ex *this_item,
                                  struct gui_synclist *this_list)
{
    (void) this_list;
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            /* hide create bookmark option if bookmarking isn't currently possible (no track playing, queued tracks...) */
            if (this_item == &bookmark_create_menu_item)
            {
                if (!bookmark_is_bookmarkable_state())
                    return ACTION_EXIT_MENUITEM;
            }
            /* hide loading bookmarks menu if no bookmarks exist */
            else if (this_item == &bookmark_load_menu_item)
            {
                if (!bookmark_exists())
                    return ACTION_EXIT_MENUITEM;
            }
            /* hide the bookmark menu if bookmarks can't be loaded or created */
            else if (!bookmark_is_bookmarkable_state() && !bookmark_exists())
                return ACTION_EXIT_MENUITEM;
            break;
        case ACTION_EXIT_MENUITEM:
            settings_save();
            break;
    }
    return action;
}



/* CONTEXT_WPS playlist options */
static bool shuffle_playlist(void)
{
    if (!warn_on_pl_erase())
        return false;
    playlist_sort(NULL, true);
    playlist_randomise(NULL, current_tick, true);
    playlist_set_modified(NULL, true);

    return false;
}

static bool save_playlist(void)
{
    /* save_playlist_screen should load the newly saved playlist and resume */
    save_playlist_screen(NULL);
    return false;
}

static int wps_view_cur_playlist(void)
{
    if (get_current_activity() == ACTIVITY_CONTEXTMENU)  /* get rid of parent activity */
        pop_current_activity_without_refresh(); /* when called from ctxt menu */

    playlist_viewer_ex(NULL, NULL);

    return 0;
}

static void playing_time(void)
{
    plugin_load(PLUGIN_APPS_DIR"/playing_time.rock", NULL);
}

MENUITEM_FUNCTION(wps_view_cur_playlist_item, 0, ID2P(LANG_VIEW_DYNAMIC_PLAYLIST),
                  wps_view_cur_playlist, NULL, Icon_NOICON);
MENUITEM_FUNCTION(search_playlist_item, 0, ID2P(LANG_SEARCH_IN_PLAYLIST),
                  search_playlist, NULL, Icon_Playlist);
MENUITEM_FUNCTION(playlist_save_item, 0, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),
                  save_playlist, NULL, Icon_Playlist);
MENUITEM_FUNCTION(reshuffle_item, 0, ID2P(LANG_SHUFFLE_PLAYLIST),
                  shuffle_playlist, NULL, Icon_Playlist);
MENUITEM_FUNCTION(playing_time_item, 0, ID2P(LANG_PLAYING_TIME),
                  playing_time, NULL, Icon_Playlist);
MAKE_ONPLAYMENU( wps_playlist_menu, ID2P(LANG_CURRENT_PLAYLIST),
                 NULL, Icon_Playlist,
                 &wps_view_cur_playlist_item, &search_playlist_item,
                 &playlist_save_item, &reshuffle_item, &playing_time_item
               );

/* argument for add_to_playlist (for use by menu callbacks) */
struct add_to_pl_param
{
    int8_t position;
    unsigned int queue: 1;
    unsigned int replace: 1;
};

static struct add_to_pl_param addtopl_insert           = {PLAYLIST_INSERT, 0, 0};
static struct add_to_pl_param addtopl_insert_first     = {PLAYLIST_INSERT_FIRST, 0, 0};
static struct add_to_pl_param addtopl_insert_last      = {PLAYLIST_INSERT_LAST, 0, 0};
static struct add_to_pl_param addtopl_insert_shuf      = {PLAYLIST_INSERT_SHUFFLED, 0, 0};
static struct add_to_pl_param addtopl_insert_last_shuf = {PLAYLIST_INSERT_LAST_SHUFFLED, 0, 0};

static struct add_to_pl_param addtopl_queue            = {PLAYLIST_INSERT, 1, 0};
static struct add_to_pl_param addtopl_queue_first      = {PLAYLIST_INSERT_FIRST, 1, 0};
static struct add_to_pl_param addtopl_queue_last       = {PLAYLIST_INSERT_LAST, 1, 0};
static struct add_to_pl_param addtopl_queue_shuf       = {PLAYLIST_INSERT_SHUFFLED, 1, 0};
static struct add_to_pl_param addtopl_queue_last_shuf  = {PLAYLIST_INSERT_LAST_SHUFFLED, 1, 0};

static struct add_to_pl_param addtopl_replace          = {PLAYLIST_INSERT, 0, 1};
static struct add_to_pl_param addtopl_replace_shuffled = {PLAYLIST_INSERT_LAST_SHUFFLED, 0, 1};

/* CONTEXT_[TREE|ID3DB|STD] playlist options */
static int add_to_playlist(void* arg)
{
    struct add_to_pl_param* param = arg;
    int position = param->position;
    bool new_playlist = !!param->replace;
    bool queue = !!param->queue;

    /* warn if replacing the playlist */
    if (new_playlist && !warn_on_pl_erase())
        return 0;

    const char *lines[] = {
        ID2P(LANG_RECURSE_DIRECTORY_QUESTION),
        selected_file
    };
    const struct text_message message={lines, 2};

    splash(0, ID2P(LANG_WAIT));

    if (new_playlist && global_settings.keep_current_track_on_replace_playlist)
    {
        if (audio_status() & AUDIO_STATUS_PLAY)
        {
            playlist_remove_all_tracks(NULL);
            new_playlist = false;
        }
    }

    if (new_playlist)
        playlist_create(NULL, NULL);

    /* always set seed before inserting shuffled */
    if (position == PLAYLIST_INSERT_SHUFFLED ||
        position == PLAYLIST_INSERT_LAST_SHUFFLED)
    {
        srand(current_tick);
        if (position == PLAYLIST_INSERT_LAST_SHUFFLED)
            playlist_set_last_shuffled_start();
    }

#ifdef HAVE_TAGCACHE
    if ((context == CONTEXT_ID3DB) && (selected_file_attr & ATTR_DIRECTORY))
    {
        tagtree_current_playlist_insert(position, queue);
    }
    else if (context == CONTEXT_STD && ctx_current_playlist_insert != NULL)
    {
        ctx_current_playlist_insert(position, queue, false);
    }
    else
#endif
    {
        if ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO)
            playlist_insert_track(NULL, selected_file, position, queue, true);
        else if (selected_file_attr & ATTR_DIRECTORY)
        {
            bool recurse = false;

            if (global_settings.recursive_dir_insert != RECURSE_ASK)
                recurse = (bool)global_settings.recursive_dir_insert;
            else
            {
                /* Ask if user wants to recurse directory */
                recurse = (gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES);
            }

            playlist_insert_directory(NULL, selected_file, position, queue,
                                      recurse);
        }
        else if ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U)
            playlist_insert_playlist(NULL, selected_file, position, queue);
    }

    if (new_playlist && (playlist_amount() > 0))
    {
        /* nothing is currently playing so begin playing what we just
           inserted */
        if (global_settings.playlist_shuffle)
            playlist_shuffle(current_tick, -1);
        playlist_start(0, 0, 0);
        onplay_result = ONPLAY_START_PLAY;
    }

    playlist_set_modified(NULL, true);
    return false;
}

static bool view_playlist(void)
{
    bool result;

    result = playlist_viewer_ex(selected_file, NULL);

    if (result == PLAYLIST_VIEWER_OK &&
        onplay_result == ONPLAY_OK)
        /* playlist was started from viewer */
        onplay_result = ONPLAY_START_PLAY;

    return result;
}

static int treeplaylist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list);

/* insert items */
MENUITEM_FUNCTION_W_PARAM(i_pl_item, 0, ID2P(LANG_ADD),
                  add_to_playlist, &addtopl_insert,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(i_first_pl_item, 0, ID2P(LANG_PLAY_NEXT),
                  add_to_playlist, &addtopl_insert_first,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(i_last_pl_item, 0, ID2P(LANG_PLAY_LAST),
                  add_to_playlist, &addtopl_insert_last,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(i_shuf_pl_item, 0, ID2P(LANG_ADD_SHUFFLED),
                  add_to_playlist, &addtopl_insert_shuf,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(i_last_shuf_pl_item, 0, ID2P(LANG_PLAY_LAST_SHUFFLED),
                  add_to_playlist, &addtopl_insert_last_shuf,
                  treeplaylist_callback, Icon_Playlist);
/* queue items */
MENUITEM_FUNCTION_W_PARAM(q_pl_item, 0, ID2P(LANG_QUEUE),
                  add_to_playlist, &addtopl_queue,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(q_first_pl_item, 0, ID2P(LANG_QUEUE_FIRST),
                  add_to_playlist, &addtopl_queue_first,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(q_last_pl_item, 0, ID2P(LANG_QUEUE_LAST),
                  add_to_playlist, &addtopl_queue_last,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(q_shuf_pl_item, 0, ID2P(LANG_QUEUE_SHUFFLED),
                  add_to_playlist, &addtopl_queue_shuf,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION_W_PARAM(q_last_shuf_pl_item, 0, ID2P(LANG_QUEUE_LAST_SHUFFLED),
                  add_to_playlist, &addtopl_queue_last_shuf,
                  treeplaylist_callback, Icon_Playlist);

/* queue submenu */
MAKE_ONPLAYMENU(queue_menu, ID2P(LANG_QUEUE_MENU),
                treeplaylist_callback, Icon_Playlist,
                &q_first_pl_item,
                &q_pl_item,
                &q_shuf_pl_item,
                &q_last_pl_item,
                &q_last_shuf_pl_item);

/* replace playlist */
MENUITEM_FUNCTION_W_PARAM(replace_pl_item, 0, ID2P(LANG_PLAY),
                  add_to_playlist, &addtopl_replace,
                  treeplaylist_callback, Icon_Playlist);

MENUITEM_FUNCTION_W_PARAM(replace_shuf_pl_item, 0, ID2P(LANG_PLAY_SHUFFLED),
                  add_to_playlist, &addtopl_replace_shuffled,
                  treeplaylist_callback, Icon_Playlist);

MAKE_ONPLAYMENU(tree_playlist_menu, ID2P(LANG_PLAYING_NEXT),
                treeplaylist_callback, Icon_Playlist,

                /* insert */
                &i_first_pl_item,
                &i_pl_item,
                &i_last_pl_item,
                &i_shuf_pl_item,
                &i_last_shuf_pl_item,

                /* queue */
                &q_first_pl_item,
                &q_pl_item,
                &q_last_pl_item,
                &q_shuf_pl_item,
                &q_last_shuf_pl_item,

                /* Queue submenu */
                &queue_menu,

                /* replace */
                &replace_pl_item,
                &replace_shuf_pl_item
               );
static int treeplaylist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list)
{
    (void)this_list;
    switch (action)
    {
    case ACTION_REQUEST_MENUITEM:
        if (this_item == &tree_playlist_menu)
        {
            if ((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO &&
                (selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_M3U &&
                (selected_file_attr & ATTR_DIRECTORY) == 0)
                return ACTION_EXIT_MENUITEM;
        }
        else if (this_item == &queue_menu)
        {
            if (global_settings.show_queue_options != QUEUE_SHOW_IN_SUBMENU)
                return ACTION_EXIT_MENUITEM;

            /* queueing options only work during playback */
            if (!(audio_status() & AUDIO_STATUS_PLAY))
                return ACTION_EXIT_MENUITEM;
        }
        else if ((this_item->flags & MENU_TYPE_MASK) == MT_FUNCTION_CALL_W_PARAM &&
                 this_item->function_param->function_w_param == add_to_playlist)
        {
            struct add_to_pl_param *param = this_item->function_param->param;

            if (param->queue)
            {
                if (global_settings.show_queue_options != QUEUE_SHOW_AT_TOPLEVEL &&
                    !in_queue_submenu)
                    return ACTION_EXIT_MENUITEM;
            }

            if (param->position == PLAYLIST_INSERT_SHUFFLED ||
                param->position == PLAYLIST_INSERT_LAST_SHUFFLED)
            {
                if (!global_settings.show_shuffled_adding_options)
                    return ACTION_EXIT_MENUITEM;

                if ((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_M3U &&
                    (selected_file_attr & ATTR_DIRECTORY) == 0)
                    return ACTION_EXIT_MENUITEM;
            }

            if (!param->replace)
            {
                if (!(audio_status() & AUDIO_STATUS_PLAY))
                    return ACTION_EXIT_MENUITEM;
            }
        }

        break;

    case ACTION_ENTER_MENUITEM:
        in_queue_submenu = this_item == &queue_menu;
        break;
    }

    return action;
}

void onplay_show_playlist_menu(const char* path, int attr, void (*playlist_insert_cb))
{
    context = CONTEXT_STD;
    ctx_current_playlist_insert = playlist_insert_cb;
    selected_file = path;
    selected_file_attr = attr;
    in_queue_submenu = false;
    do_menu(&tree_playlist_menu, NULL, NULL, false);
}

/* playlist catalog options */
static bool cat_add_to_a_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file, selected_file_attr,
                                     false, NULL, ctx_add_to_playlist);
}

static bool cat_add_to_a_new_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file, selected_file_attr,
                                     true, NULL, ctx_add_to_playlist);
}

static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list);

MENUITEM_FUNCTION(cat_add_to_list, 0, ID2P(LANG_ADD_TO_EXISTING_PL),
                  cat_add_to_a_playlist, NULL, Icon_Playlist);
MENUITEM_FUNCTION(cat_add_to_new, 0, ID2P(LANG_CATALOG_ADD_TO_NEW),
                  cat_add_to_a_new_playlist, NULL, Icon_Playlist);
MAKE_ONPLAYMENU(cat_playlist_menu, ID2P(LANG_ADD_TO_PL),
                cat_playlist_callback, Icon_Playlist,
                &cat_add_to_list, &cat_add_to_new);

void onplay_show_playlist_cat_menu(const char* track_name, int attr, void (*add_to_pl_cb))
{
    context = CONTEXT_STD;
    ctx_add_to_playlist = add_to_pl_cb;
    selected_file = track_name;
    selected_file_attr = attr;
    do_menu(&cat_playlist_menu, NULL, NULL, false);
}

static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    if (!selected_file ||
        (((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO) &&
         ((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_M3U) &&
         ((selected_file_attr & ATTR_DIRECTORY) == 0)))
    {
        return ACTION_EXIT_MENUITEM;
    }

    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if ((audio_status() & AUDIO_STATUS_PLAY) || context != CONTEXT_WPS)
            {
                return action;
            }
            else
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}

static void draw_slider(void)
{
    struct viewport *last_vp;
    FOR_NB_SCREENS(i)
    {
        struct viewport vp;
        int slider_height = 2*screens[i].getcharheight();
        viewport_set_defaults(&vp, i);
        last_vp = screens[i].set_viewport(&vp);
        show_busy_slider(&screens[i], 1, vp.height - slider_height,
                         vp.width-2, slider_height-1);
        screens[i].update_viewport();
        screens[i].set_viewport(last_vp);
    }
}

static void clear_display(bool update)
{
    struct viewport vp;
    struct viewport *last_vp;
    FOR_NB_SCREENS(i)
    {
        struct screen * screen = &screens[i];
        viewport_set_defaults(&vp, screen->screen_type);
        last_vp = screen->set_viewport(&vp);
        screen->clear_viewport();
        if (update) {
            screen->update_viewport();
        }
        screen->set_viewport(last_vp);
    }
}

static void splash_path(const char *path)
{
    clear_display(false);
    path_basename(path, &path);
    splash(0, path);
    draw_slider();
}

/* Splashes the path and checks the keys */
static bool poll_cancel_action(const char *path)
{
    splash_path(path);
    return ACTION_STD_CANCEL == get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
}

static bool check_new_name(const char *basename)
{
    /* at least prevent escapes out of the base directory from keyboard-
       entered filenames; the file code should reject other invalidities */
    return *basename != '\0' && !strchr(basename, PATH_SEPCH) &&
           !is_dotdir_name(basename);
}

static void splash_cancelled(void)
{
    clear_display(true);
    splash(HZ, ID2P(LANG_CANCEL));
}

static void splash_failed(int lang_what)
{
    cond_talk_ids_fq(lang_what, LANG_FAILED);
    clear_display(true);
    splashf(HZ*2, "%s %s", str(lang_what), str(LANG_FAILED));
}

/* helper function to remove a non-empty directory */
static int remove_dir(struct dirrecurse_params *parm)
{
    DIR *dir = opendir(parm->path);
    if (!dir) {
        return -1; /* open error */
    }

    size_t append = parm->append;
    int rc = OPRC_SUCCESS;

    /* walk through the directory content */
    while (rc == OPRC_SUCCESS) {
        errno = 0; /* distinguish failure from eod */
        struct dirent *entry = readdir(dir);
        if (!entry) {
            if (errno) {
                rc = -1;
            }
            break;
        }

        struct dirinfo info = dir_get_info(dir, entry);
        if ((info.attribute & ATTR_DIRECTORY) &&
            is_dotdir_name(entry->d_name)) {
            continue; /* skip these */
        }

        /* append name to current directory */
        parm->append = append + path_append(&parm->path[append],
                                            PA_SEP_HARD, entry->d_name,
                                            sizeof (parm->path) - append);
        if (parm->append >= sizeof (parm->path)) {
            rc = -1;
            break; /* no space left in buffer */
        }

        if (info.attribute & ATTR_DIRECTORY) {
            /* remove a subdirectory */
            rc = remove_dir(parm);
        } else {
            /* remove a file */
            if (poll_cancel_action(parm->path)) {
                rc = OPRC_CANCELLED;
                break;
            }

            rc = remove(parm->path);
        }

        /* Remove basename we added above */
        parm->path[append] = '\0';
    }

    closedir(dir);

    if (rc == 0) {
        /* remove the now empty directory */
        if (poll_cancel_action(parm->path)) {
            rc = OPRC_CANCELLED;
        } else {
            rc = rmdir(parm->path);
        }
    }

    return rc;
}

/* share code for file and directory deletion, saves space */
static int delete_file_dir(void)
{
    const char *to_delete=selected_file;
    const int   to_delete_attr=selected_file_attr;
    if (confirm_delete_yesno(to_delete) != YESNO_YES) {
        return 1;
    }

    clear_display(true);
    splash(HZ/2, str(LANG_DELETING));

    int rc = -1;

    if (to_delete_attr & ATTR_DIRECTORY) { /* true if directory */
        struct dirrecurse_params parm;
        parm.append = strlcpy(parm.path, to_delete, sizeof (parm.path));

        if (parm.append < sizeof (parm.path)) {
            cpu_boost(true);
            rc = remove_dir(&parm);
            cpu_boost(false);
        }
    } else {
        rc = remove(to_delete);
    }

    if (rc < OPRC_SUCCESS) {
        splash_failed(LANG_DELETE);
    } else if (rc == OPRC_CANCELLED) {
        splash_cancelled();
    }

    if (rc != OPRC_NOOP) {
        /* Could have failed after some but not all needed changes; reload */
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return 1;
}

static int rename_file(void)
{
    int rc = -1;
    char newname[MAX_PATH];
    const char *oldbase, *selection = selected_file;

    path_basename(selection, &oldbase);
    size_t pathlen = oldbase - selection;
    char *newbase = newname + pathlen;

    if (strmemccpy(newname, selection, sizeof (newname)) == NULL) {
        /* Too long */
    } else if (kbd_input(newbase, sizeof (newname) - pathlen, NULL) < 0) {
        rc = OPRC_CANCELLED;
    } else if (!strcmp(oldbase, newbase)) {
        rc = OPRC_NOOP; /* No change at all */
    } else if (check_new_name(newbase)) {
        switch (relate(selection, newname))
        {
        case RELATE_DIFFERENT:
            if (file_exists(newname)) {
                break; /* don't overwrite */
            }
            /* Fall-through */
        case RELATE_SAME:
            rc = rename(selection, newname);
            break;
        case RELATE_PREFIX:
        default:
            break;
        }
    }

    if (rc < OPRC_SUCCESS) {
        splash_failed(LANG_RENAME);
    } else if (rc == OPRC_CANCELLED) {
        /* splash_cancelled(); kbd_input() splashes it */
    } else if (rc == OPRC_SUCCESS) {
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return 1;
}

static int create_dir(void)
{
    int rc = -1;
    char dirname[MAX_PATH];
    size_t pathlen = path_append(dirname, getcwd(NULL, 0), PA_SEP_HARD,
                                 sizeof (dirname));
    char *basename = dirname + pathlen;

    if (pathlen >= sizeof (dirname)) {
        /* Too long */
    } else if (kbd_input(basename, sizeof (dirname) - pathlen, NULL) < 0) {
        rc = OPRC_CANCELLED;
    } else if (check_new_name(basename)) {
        rc = mkdir(dirname);
    }

    if (rc < OPRC_SUCCESS) {
        splash_failed(LANG_CREATE_DIR);
    } else if (rc == OPRC_CANCELLED) {
        /* splash_cancelled(); kbd_input() splashes it */
    } else if (rc == OPRC_SUCCESS) {
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return 1;
}

/* Paste a file */
static int clipboard_pastefile(const char *src, const char *target,
                               unsigned int flags)
{
    int rc = -1;

    while (!(flags & (PASTE_COPY | PASTE_EXDEV))) {
        if ((flags & PASTE_OVERWRITE) || !file_exists(target)) {
            /* Rename and possibly overwrite the file */
            if (poll_cancel_action(src)) {
                rc = OPRC_CANCELLED;
            } else {
                rc = rename(src, target);
            }

        #ifdef HAVE_MULTIVOLUME
            if (rc < 0 && errno == EXDEV) {
                /* Failed because cross volume rename doesn't work; force
                   a move instead */
                flags |= PASTE_EXDEV;
                break;
            }
        #endif /* HAVE_MULTIVOLUME */
        }

        return rc;
    }

    /* See if we can get the plugin buffer for the file copy buffer */
    size_t buffersize;
    char *buffer = (char *) plugin_get_buffer(&buffersize);
    if (buffer == NULL || buffersize < 512) {
        /* Not large enough, try for a disk sector worth of stack
           instead */
        buffersize = 512;
        buffer = (char *)alloca(buffersize);
    }

    if (buffer == NULL) {
        return -1;
    }

    buffersize &= ~0x1ff;  /* Round buffer size to multiple of sector
                              size */

    int src_fd = open(src, O_RDONLY);
    if (src_fd >= 0) {
        int oflag = O_WRONLY|O_CREAT;

        if (!(flags & PASTE_OVERWRITE)) {
            oflag |= O_EXCL;
        }

        int target_fd = open(target, oflag, 0666);
        if (target_fd >= 0) {
            off_t total_size = 0;
            off_t next_cancel_test = 0; /* No excessive button polling */

            rc = OPRC_SUCCESS;

            while (rc == OPRC_SUCCESS) {
                if (total_size >= next_cancel_test) {
                    next_cancel_test = total_size + 0x10000;
                    if (poll_cancel_action(src)) {
                       rc = OPRC_CANCELLED;
                       break;
                    }
                }

                ssize_t bytesread = read(src_fd, buffer, buffersize);
                if (bytesread <= 0) {
                    if (bytesread < 0) {
                        rc = -1;
                    }
                    /* else eof on buffer boundary; nothing to write */
                    break;
                }

                ssize_t byteswritten = write(target_fd, buffer, bytesread);
                if (byteswritten < bytesread) {
                    /* Some I/O error */
                    rc = -1;
                    break;
                }

                total_size += byteswritten;

                if (bytesread < (ssize_t)buffersize) {
                    /* EOF with trailing bytes */
                    break;
                }
            }

            if (rc == OPRC_SUCCESS) {
                /* If overwriting, set the correct length if original was
                   longer */
                rc = ftruncate(target_fd, total_size);
            }

            close(target_fd);

            if (rc != OPRC_SUCCESS) {
                /* Copy failed. Cleanup. */
                remove(target);
            }
        }

        close(src_fd);
    }

    if (rc == OPRC_SUCCESS && !(flags & PASTE_COPY)) {
        /* Remove the source file */
        rc = remove(src);
    }

    return rc;
}

/* Paste a directory */
static int clipboard_pastedirectory(struct dirrecurse_params *src,
                                    struct dirrecurse_params *target,
                                    unsigned int flags)
{
    int rc = -1;

    while (!(flags & (PASTE_COPY | PASTE_EXDEV))) {
        if ((flags & PASTE_OVERWRITE) || !file_exists(target->path)) {
            /* Just try to move the directory */
            if (poll_cancel_action(src->path)) {
                rc = OPRC_CANCELLED;
            } else {
                rc = rename(src->path, target->path);
            }

            if (rc < 0) {
                int errnum = errno;
                if (errnum == ENOTEMPTY && (flags & PASTE_OVERWRITE)) {
                    /* Directory is not empty thus rename() will not do a quick
                       overwrite */
                    break;
                }
            #ifdef HAVE_MULTIVOLUME
                else if (errnum == EXDEV) {
                    /* Failed because cross volume rename doesn't work; force
                       a move instead */
                    flags |= PASTE_EXDEV;
                    break;
                }
            #endif /* HAVE_MULTIVOLUME */
            }
        }

        return rc;
    }

    DIR *srcdir = opendir(src->path);

    if (srcdir) {
        /* Make a directory to copy things to */
        rc = mkdir(target->path);
        if (rc < 0 && errno == EEXIST && (flags & PASTE_OVERWRITE)) {
            /* Exists and overwrite was approved */
            rc = OPRC_SUCCESS;
        }
    }

    size_t srcap = src->append, targetap = target->append;

    /* Walk through the directory content; this loop will exit as soon as
       there's a problem */
    while (rc == OPRC_SUCCESS) {
        errno = 0; /* Distinguish failure from eod */
        struct dirent *entry = readdir(srcdir);
        if (!entry) {
            if (errno) {
                rc = -1;
            }
            break;
        }

        struct dirinfo info = dir_get_info(srcdir, entry);
        if ((info.attribute & ATTR_DIRECTORY) &&
            is_dotdir_name(entry->d_name)) {
            continue; /* Skip these */
        }

        /* Append names to current directories */
        src->append = srcap +
            path_append(&src->path[srcap], PA_SEP_HARD, entry->d_name,
                        sizeof(src->path) - srcap);

        target->append = targetap +
            path_append(&target->path[targetap], PA_SEP_HARD, entry->d_name,
                        sizeof (target->path) - targetap);

        if (src->append >= sizeof (src->path) ||
            target->append >= sizeof (target->path)) {
            rc = -1; /* No space left in buffer */
            break;
        }

        if (poll_cancel_action(src->path)) {
            rc = OPRC_CANCELLED;
            break;
        }

        DEBUGF("Copy %s to %s\n", src->path, target->path);

        if (info.attribute & ATTR_DIRECTORY) {
            /* Copy/move a subdirectory */
            rc = clipboard_pastedirectory(src, target, flags); /* recursion */
        } else {
            /* Copy/move a file */
            rc = clipboard_pastefile(src->path, target->path, flags);
        }

        /* Remove basenames we added above */
        src->path[srcap] = target->path[targetap] = '\0';
    }

    if (rc == OPRC_SUCCESS && !(flags & PASTE_COPY)) {
        /* Remove the now empty directory */
        rc = rmdir(src->path);
    }

    closedir(srcdir);
    return rc;
}

static bool clipboard_cut(void)
{
    return clipboard_clip(&clipboard, selected_file, selected_file_attr,
                          PASTE_CUT);
}

static bool clipboard_copy(void)
{
    return clipboard_clip(&clipboard, selected_file, selected_file_attr,
                          PASTE_COPY);
}

/* Paste the clipboard to the current directory */
static int clipboard_paste(void)
{
    if (!clipboard.path[0])
        return 1;

    int rc = -1;

    struct dirrecurse_params src, target;
    unsigned int flags = clipboard.flags;

    /* Figure out the name of the selection */
    const char *nameptr;
    path_basename(clipboard.path, &nameptr);

    /* Final target is current directory plus name of selection  */
    target.append = path_append(target.path, getcwd(NULL, 0),
                                nameptr, sizeof (target.path));

    switch (target.append < sizeof (target.path) ?
                relate(clipboard.path, target.path) : -1)
    {
    case RELATE_SAME:
        rc = OPRC_NOOP;
        break;

    case RELATE_DIFFERENT:
        if (file_exists(target.path)) {
            /* If user chooses not to overwrite, cancel */
            if (confirm_overwrite_yesno() == YESNO_NO) {
                rc = OPRC_NOOVERWRT;
                break;
            }

            flags |= PASTE_OVERWRITE;
        }

        clear_display(true);
        splash(HZ/2, (flags & PASTE_COPY) ? ID2P(LANG_COPYING) :
                                            ID2P(LANG_MOVING));

        /* Now figure out what we're doing */
        cpu_boost(true);

        if (clipboard.attr & ATTR_DIRECTORY) {
            /* Copy or move a subdirectory */
            src.append = strlcpy(src.path, clipboard.path,
                                 sizeof (src.path));
            if (src.append < sizeof (src.path)) {
                rc = clipboard_pastedirectory(&src, &target, flags);
            }
        } else {
            /* Copy or move a file */
            rc = clipboard_pastefile(clipboard.path, target.path, flags);
        }

        cpu_boost(false);
        break;

    case RELATE_PREFIX:
    default: /* Some other relation / failure */
        break;
    }

    clear_display(true);

    switch (rc)
    {
    case OPRC_CANCELLED:
        splash_cancelled();
        /* Fallthrough */
    case OPRC_SUCCESS:
        onplay_result = ONPLAY_RELOAD_DIR;
        /* Fallthrough */
    case OPRC_NOOP:
        clipboard_clear_selection(&clipboard);
        /* Fallthrough */
    case OPRC_NOOVERWRT:
        break;
    default:
        if (rc < OPRC_SUCCESS) {
            splash_failed(LANG_PASTE);
            onplay_result = ONPLAY_RELOAD_DIR;
        }
    }

    return 1;
}

#ifdef HAVE_TAGCACHE
static int set_rating_inline(void)
{
    struct mp3entry* id3 = audio_current_track();
    if (id3 && id3->tagcache_idx && global_settings.runtimedb)
    {
        set_int_ex(str(LANG_MENU_SET_RATING), "", UNIT_INT, (void*)(&id3->rating),
                   NULL, 1, 0, 10, NULL, NULL);
        tagcache_update_numeric(id3->tagcache_idx-1, tag_rating, id3->rating);
    }
    else
        splash(HZ*2, ID2P(LANG_ID3_NO_INFO));
    return 0;
}
static int ratingitem_callback(int action,
                               const struct menu_item_ex *this_item,
                               struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (!selected_file || !global_settings.runtimedb ||
                !tagcache_is_usable())
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}
MENUITEM_FUNCTION(rating_item, 0, ID2P(LANG_MENU_SET_RATING),
                  set_rating_inline,
                  ratingitem_callback, Icon_Questionmark);
#endif
MENUITEM_RETURNVALUE(plugin_item, ID2P(LANG_OPEN_PLUGIN),
                  GO_TO_PLUGIN, NULL, Icon_Plugin);

static bool view_cue(void)
{
    struct mp3entry* id3 = audio_current_track();
    if (id3 && id3->cuesheet)
    {
        browse_cuesheet(id3->cuesheet);
    }
    return false;
}
static int view_cue_item_callback(int action,
                                  const struct menu_item_ex *this_item,
                                  struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    struct mp3entry* id3 = audio_current_track();
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (!selected_file
                || !id3 || !id3->cuesheet)
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}
MENUITEM_FUNCTION(view_cue_item, 0, ID2P(LANG_BROWSE_CUESHEET),
                  view_cue, view_cue_item_callback, Icon_NOICON);


static int browse_id3_wrapper(void)
{
    if (get_current_activity() == ACTIVITY_CONTEXTMENU)  /* get rid of parent activity */
        pop_current_activity_without_refresh(); /* when called from ctxt menu */

    if (browse_id3(audio_current_track(),
            playlist_get_display_index(),
            playlist_amount(), NULL, 1))
        return GO_TO_ROOT;
    return GO_TO_PREVIOUS;
}

/* CONTEXT_WPS items */
MENUITEM_FUNCTION(browse_id3_item, MENU_FUNC_CHECK_RETVAL, ID2P(LANG_MENU_SHOW_ID3_INFO),
                  browse_id3_wrapper, NULL, Icon_NOICON);
#ifdef HAVE_PITCHCONTROL
MENUITEM_FUNCTION(pitch_screen_item, 0, ID2P(LANG_PITCH),
                  gui_syncpitchscreen_run, NULL, Icon_Audio);
#endif

/* CONTEXT_[TREE|ID3DB] items */
static int clipboard_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list);

MENUITEM_FUNCTION(rename_file_item, 0, ID2P(LANG_RENAME),
                  rename_file, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_cut_item, 0, ID2P(LANG_CUT),
                  clipboard_cut, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_copy_item, 0, ID2P(LANG_COPY),
                  clipboard_copy, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_paste_item, 0, ID2P(LANG_PASTE),
                  clipboard_paste, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_file_item, 0, ID2P(LANG_DELETE),
                  delete_file_dir, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_dir_item, 0, ID2P(LANG_DELETE_DIR),
                  delete_file_dir, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(create_dir_item, 0, ID2P(LANG_CREATE_DIR),
                  create_dir, clipboard_callback, Icon_NOICON);

/* other items */
static bool list_viewers(void)
{
    int ret = filetype_list_viewers(selected_file);
    if (ret == PLUGIN_USB_CONNECTED)
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}

#ifdef HAVE_TAGCACHE
static bool prepare_database_sel(void *param)
{
    if (context == CONTEXT_ID3DB &&
        (selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
    {
        if (!strcmp(param, "properties"))
            strmemccpy(selected_file_path, MAKE_ACT_STR(ACTIVITY_DATABASEBROWSER),
                       sizeof(selected_file_path));
        else if (!tagtree_get_subentry_filename(selected_file_path, MAX_PATH))
        {
            onplay_result = ONPLAY_RELOAD_DIR;
            return false;
        }

        selected_file = selected_file_path;
    }
    return true;
}
#endif

static bool onplay_load_plugin(void *param)
{
#ifdef HAVE_TAGCACHE
    if (!prepare_database_sel(param))
        return false;
#endif
    int ret = filetype_load_plugin((const char*)param, selected_file);
    if (ret == PLUGIN_USB_CONNECTED)
        onplay_result = ONPLAY_RELOAD_DIR;
    else if (ret == PLUGIN_GOTO_PLUGIN)
        onplay_result = ONPLAY_PLUGIN;
    else if (ret == PLUGIN_GOTO_WPS)
        onplay_result = ONPLAY_START_PLAY;
    return false;
}

MENUITEM_FUNCTION(list_viewers_item, 0, ID2P(LANG_ONPLAY_OPEN_WITH),
                  list_viewers, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION_W_PARAM(properties_item, 0, ID2P(LANG_PROPERTIES),
                  onplay_load_plugin, (void *)"properties",
                  clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION_W_PARAM(track_info_item, 0, ID2P(LANG_MENU_SHOW_ID3_INFO),
                  onplay_load_plugin, (void *)"properties",
                  clipboard_callback, Icon_NOICON);
#ifdef HAVE_TAGCACHE
MENUITEM_FUNCTION_W_PARAM(pictureflow_item, 0, ID2P(LANG_ONPLAY_PICTUREFLOW),
                  onplay_load_plugin, (void *)"pictureflow",
                  clipboard_callback, Icon_NOICON);
#endif
static bool onplay_add_to_shortcuts(void)
{
    shortcuts_add(SHORTCUT_BROWSER, selected_file);
    return false;
}
MENUITEM_FUNCTION(add_to_faves_item, 0, ID2P(LANG_ADD_TO_FAVES),
                  onplay_add_to_shortcuts,
                  clipboard_callback, Icon_NOICON);

#if LCD_DEPTH > 1
static bool set_backdrop(void)
{
    path_append(global_settings.backdrop_file, selected_file,
                PA_SEP_HARD, sizeof(global_settings.backdrop_file));
    settings_save();
    skin_backdrop_load_setting();
    skin_backdrop_show(sb_get_backdrop(SCREEN_MAIN));
    return true;
}
MENUITEM_FUNCTION(set_backdrop_item, 0, ID2P(LANG_SET_AS_BACKDROP),
                  set_backdrop, clipboard_callback, Icon_NOICON);
#endif
#ifdef HAVE_RECORDING
static bool set_recdir(void)
{
    path_append(global_settings.rec_directory, selected_file,
                PA_SEP_HARD, sizeof(global_settings.rec_directory));
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_recdir_item, 0, ID2P(LANG_RECORDING_DIR),
                  set_recdir, clipboard_callback, Icon_Recording);
#endif
static bool set_startdir(void)
{
    path_append(global_settings.start_directory, selected_file,
                PA_SEP_HARD, sizeof(global_settings.start_directory));

    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_startdir_item, 0, ID2P(LANG_START_DIR),
                  set_startdir, clipboard_callback, Icon_file_view_menu);

static bool set_catalogdir(void)
{
    catalog_set_directory(selected_file);
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_catalogdir_item, 0, ID2P(LANG_PLAYLIST_DIR),
                  set_catalogdir, clipboard_callback, Icon_Playlist);

#ifdef HAVE_TAGCACHE
static bool set_databasedir(void)
{
    path_append(global_settings.tagcache_db_path, selected_file,
                PA_SEP_SOFT, sizeof(global_settings.tagcache_db_path));

    struct tagcache_stat *tc_stat = tagcache_get_stat();
    if (strcasecmp(global_settings.tagcache_db_path, tc_stat->db_path))
    {
        splash(HZ, ID2P(LANG_PLEASE_REBOOT));
    }

    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_databasedir_item, 0, ID2P(LANG_DATABASE_DIR),
                  set_databasedir, clipboard_callback, Icon_Audio);
#endif

MAKE_ONPLAYMENU(set_as_dir_menu, ID2P(LANG_SET_AS),
                clipboard_callback, Icon_NOICON,
                &set_catalogdir_item,
#ifdef HAVE_TAGCACHE
                &set_databasedir_item,
#endif
#ifdef HAVE_RECORDING
                &set_recdir_item,
#endif
                &set_startdir_item);

static int clipboard_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list)
{
    (void)this_list;
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
#ifdef HAVE_MULTIVOLUME
            /* no rename+delete for volumes */
            if ((selected_file_attr & ATTR_VOLUME) &&
                (this_item == &rename_file_item ||
                 this_item == &delete_dir_item ||
                 this_item == &clipboard_cut_item ||
                 this_item == &list_viewers_item))
                return ACTION_EXIT_MENUITEM;
#endif
#ifdef HAVE_TAGCACHE
            if (context == CONTEXT_ID3DB)
            {
                if (this_item == &track_info_item ||
                    this_item == &pictureflow_item)
                    return action;
                return ACTION_EXIT_MENUITEM;
            }
#endif
            if (this_item == &clipboard_paste_item)
            {  /* visible if there is something to paste */
                return (clipboard.path[0] != 0) ?
                                    action : ACTION_EXIT_MENUITEM;
            }
            else if (this_item == &create_dir_item)
            {
                /* always visible */
                return action;
            }
            else if (selected_file)
            {
                /* requires an actual file */
                if (this_item == &rename_file_item ||
                    this_item == &clipboard_cut_item ||
                    this_item == &clipboard_copy_item ||
                    (this_item == &track_info_item &&
                        (selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO) ||
                    (this_item == &properties_item &&
                        (selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO) ||
                    this_item == &add_to_faves_item)
                {
                    return action;
                }
                else if ((selected_file_attr & ATTR_DIRECTORY))
                {
                    /* only for directories */
                    if (this_item == &delete_dir_item ||
                        this_item == &set_startdir_item ||
                        this_item == &set_catalogdir_item ||
#ifdef HAVE_TAGCACHE
                        this_item == &set_databasedir_item ||
#endif
#ifdef HAVE_RECORDING
                        this_item == &set_recdir_item ||
#endif
                        this_item == &set_as_dir_menu
                        )
                        return action;
                }
                else if (this_item == &delete_file_item ||
                         this_item == &list_viewers_item)
                {
                    /* only for files */
                    return action;
                }
#if LCD_DEPTH > 1
                else if (this_item == &set_backdrop_item)
                {
                    char *suffix = strrchr(selected_file, '.');
                    if (suffix)
                    {
                        if (strcasecmp(suffix, ".bmp") == 0)
                        {
                            return action;
                        }
                    }
                }
#endif
            }
            return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}

static int onplaymenu_callback(int action,
                               const struct menu_item_ex *this_item,
                               struct gui_synclist *this_list);

/* used when onplay() is called in the CONTEXT_WPS context */
MAKE_ONPLAYMENU( wps_onplay_menu, ID2P(LANG_ONPLAY_MENU_TITLE),
           onplaymenu_callback, Icon_Audio,
           &wps_playlist_menu, &cat_playlist_menu,
           &sound_settings, &playback_settings,
#ifdef HAVE_TAGCACHE
           &rating_item,
#endif
           &bookmark_menu,
           &plugin_item,
           &browse_id3_item, &list_viewers_item,
           &delete_file_item, &view_cue_item,
#ifdef HAVE_PITCHCONTROL
           &pitch_screen_item,
#endif
         );

MENUITEM_FUNCTION(view_playlist_item, 0, ID2P(LANG_VIEW),
                  view_playlist,
                  onplaymenu_callback, Icon_Playlist);

/* used when onplay() is not called in the CONTEXT_WPS context */
MAKE_ONPLAYMENU( tree_onplay_menu, ID2P(LANG_ONPLAY_MENU_TITLE),
           onplaymenu_callback, Icon_file_view_menu,
           &view_playlist_item, &tree_playlist_menu, &cat_playlist_menu,
           &rename_file_item, &clipboard_cut_item, &clipboard_copy_item,
           &clipboard_paste_item, &delete_file_item, &delete_dir_item,
           &list_viewers_item, &create_dir_item, &properties_item, &track_info_item,
#ifdef HAVE_TAGCACHE
           &pictureflow_item,
#endif
#if LCD_DEPTH > 1
           &set_backdrop_item,
#endif
           &add_to_faves_item, &set_as_dir_menu, &file_menu,
         );
static int onplaymenu_callback(int action,
                               const struct menu_item_ex *this_item,
                               struct gui_synclist *this_list)
{
    (void)this_list;
    switch (action)
    {
        case ACTION_TREE_STOP:
            if (this_item == &wps_onplay_menu)
            {
                list_stop_handler();
                return ACTION_STD_CANCEL;
            }
            break;
        case ACTION_REQUEST_MENUITEM:
            if (this_item == &view_playlist_item)
            {
                if ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U &&
                        context == CONTEXT_TREE)
                    return action;
            }
            return ACTION_EXIT_MENUITEM;
            break;
        case ACTION_EXIT_MENUITEM:
            return ACTION_EXIT_AFTER_THIS_MENUITEM;
            break;
    }
    return action;
}

#ifdef HAVE_HOTKEY
/* direct function calls, no need for menu callbacks */
static bool hotkey_delete_item(void)
{
#ifdef HAVE_MULTIVOLUME
    /* no delete for volumes */
    if (selected_file_attr & ATTR_VOLUME)
        return false;
#endif

#ifdef HAVE_TAGCACHE
    if (context == CONTEXT_ID3DB &&
        (selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
        return false;
#endif

    return delete_file_dir();
}

static bool hotkey_open_with(void)
{
    /* only open files */
    if (selected_file_attr & ATTR_DIRECTORY)
        return false;
#ifdef HAVE_MULTIVOLUME
    if (selected_file_attr & ATTR_VOLUME)
        return false;
#endif
    return list_viewers();
}

static int hotkey_tree_pl_insert_shuffled(void)
{
    if ((audio_status() & AUDIO_STATUS_PLAY) ||
        (selected_file_attr & ATTR_DIRECTORY) ||
        ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U))
    {
        add_to_playlist(&addtopl_insert_shuf);
    }
    return ONPLAY_RELOAD_DIR;
}

static int hotkey_tree_run_plugin(void *param)
{
#ifdef HAVE_TAGCACHE
    if (!prepare_database_sel(param))
        return ONPLAY_RELOAD_DIR;
#endif
    if (filetype_load_plugin((const char*)param, selected_file) == PLUGIN_GOTO_WPS)
        return ONPLAY_START_PLAY;

    return ONPLAY_RELOAD_DIR;
}

static int hotkey_wps_run_plugin(void)
{
    open_plugin_run(ID2P(LANG_HOTKEY_WPS));
    return ONPLAY_OK;
}
#define HOTKEY_FUNC(func, param) {{(void *)func}, param}

/* Any desired hotkey functions go here, in the enum in onplay.h,
   and in the settings menu in settings_list.c.  The order here
   is not important. */
static const struct hotkey_assignment hotkey_items[] = {
 [0]{ .action = HOTKEY_OFF,
      .lang_id = LANG_OFF,
      .func = HOTKEY_FUNC(NULL,NULL),
      .return_code = ONPLAY_RELOAD_DIR,
      .flags = HOTKEY_FLAG_WPS | HOTKEY_FLAG_TREE },
    { .action = HOTKEY_VIEW_PLAYLIST,
      .lang_id = LANG_VIEW_DYNAMIC_PLAYLIST,
      .func = HOTKEY_FUNC(NULL, NULL),
      .return_code = ONPLAY_PLAYLIST,
      .flags = HOTKEY_FLAG_WPS },
    { .action = HOTKEY_SHOW_TRACK_INFO,
      .lang_id = LANG_MENU_SHOW_ID3_INFO,
      .func = HOTKEY_FUNC(browse_id3_wrapper, NULL),
      .return_code = ONPLAY_RELOAD_DIR,
      .flags = HOTKEY_FLAG_WPS },
#ifdef HAVE_PITCHCONTROL
    { .action = HOTKEY_PITCHSCREEN,
      .lang_id = LANG_PITCH,
      .func = HOTKEY_FUNC(gui_syncpitchscreen_run, NULL),
      .return_code = ONPLAY_RELOAD_DIR,
      .flags = HOTKEY_FLAG_WPS | HOTKEY_FLAG_NOSBS },
#endif
    { .action = HOTKEY_OPEN_WITH,
      .lang_id = LANG_ONPLAY_OPEN_WITH,
      .func = HOTKEY_FUNC(hotkey_open_with, NULL),
      .return_code = ONPLAY_RELOAD_DIR,
      .flags = HOTKEY_FLAG_WPS | HOTKEY_FLAG_TREE },
    { .action = HOTKEY_DELETE,
      .lang_id = LANG_DELETE,
      .func = HOTKEY_FUNC(hotkey_delete_item, NULL),
      .return_code = ONPLAY_RELOAD_DIR,
      .flags = HOTKEY_FLAG_WPS | HOTKEY_FLAG_TREE },
    { .action = HOTKEY_INSERT,
      .lang_id = LANG_ADD,
      .func = HOTKEY_FUNC(add_to_playlist, (intptr_t*)&addtopl_insert),
      .return_code = ONPLAY_RELOAD_DIR,
      .flags = HOTKEY_FLAG_TREE },
    { .action = HOTKEY_INSERT_SHUFFLED,
      .lang_id = LANG_ADD_SHUFFLED,
      .func = HOTKEY_FUNC(hotkey_tree_pl_insert_shuffled, NULL),
      .return_code = ONPLAY_FUNC_RETURN,
      .flags = HOTKEY_FLAG_TREE },
    { .action = HOTKEY_PLUGIN,
      .lang_id = LANG_OPEN_PLUGIN,
      .func = HOTKEY_FUNC(hotkey_wps_run_plugin, NULL),
      .return_code = ONPLAY_FUNC_RETURN,
      .flags = HOTKEY_FLAG_WPS | HOTKEY_FLAG_NOSBS },
    { .action = HOTKEY_BOOKMARK,
      .lang_id = LANG_BOOKMARK_MENU_CREATE,
      .func = HOTKEY_FUNC(bookmark_create_menu, NULL),
      .return_code = ONPLAY_OK,
      .flags = HOTKEY_FLAG_WPS | HOTKEY_FLAG_NOSBS },
    { .action = HOTKEY_BOOKMARK_LIST,
      .lang_id = LANG_BOOKMARK_MENU_LIST,
      .func = HOTKEY_FUNC(bookmark_load_menu, NULL),
      .return_code = ONPLAY_START_PLAY,
      .flags = HOTKEY_FLAG_WPS },
    { .action = HOTKEY_PROPERTIES,
      .lang_id = LANG_PROPERTIES,
      .func = HOTKEY_FUNC(hotkey_tree_run_plugin, (void *)"properties"),
      .return_code = ONPLAY_FUNC_RETURN,
      .flags = HOTKEY_FLAG_TREE },
#ifdef HAVE_TAGCACHE
    { .action = HOTKEY_PICTUREFLOW,
      .lang_id = LANG_ONPLAY_PICTUREFLOW,
      .func = HOTKEY_FUNC(hotkey_tree_run_plugin, (void *)"pictureflow"),
      .return_code = ONPLAY_FUNC_RETURN,
      .flags = HOTKEY_FLAG_TREE },
#endif
};

const struct hotkey_assignment *get_hotkey(int action)
{
    for (size_t i = ARRAYLEN(hotkey_items) - 1; i < ARRAYLEN(hotkey_items); i--)
    {
        if (hotkey_items[i].action == action)
            return &hotkey_items[i];
    }
    return &hotkey_items[0]; /* no valid hotkey set, return HOTKEY_OFF*/
}

/* Execute the hotkey function, if listed */
static int execute_hotkey(bool is_wps)
{
    const int action = (is_wps ? global_settings.hotkey_wps :
                                 global_settings.hotkey_tree);

    /* search assignment struct for a match for the hotkey setting */
    const struct hotkey_assignment *this_item = get_hotkey(action);

    /* run the associated function (with optional param), if any */
    const struct menu_func_param func = this_item->func;

    int func_return = ONPLAY_RELOAD_DIR;
    if (func.function != NULL)
    {
        if (func.param != NULL)
            func_return = (*func.function_w_param)(func.param);
        else
            func_return = (*func.function)();
    }
    const int return_code = this_item->return_code;

    if (return_code == ONPLAY_FUNC_RETURN)
        return func_return;  /* Use value returned by function */
    return return_code;      /* or return the associated value */
}
#endif /* HOTKEY */

int onplay(char* file, int attr, int from, bool hotkey)
{
    const struct menu_item_ex *menu;
    onplay_result = ONPLAY_OK;
    context = from;
    ctx_current_playlist_insert = NULL;
    selected_file = NULL;
#ifdef HAVE_TAGCACHE
    if (context == CONTEXT_ID3DB &&
        (attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
    {
        ctx_add_to_playlist = tagtree_add_to_playlist;
        if (file != NULL)
        {
            /* add a leading slash so that catalog_add_to_a_playlist
               later prefills the name when creating a new playlist */
            snprintf(selected_file_path, MAX_PATH, "/%s", file);
            selected_file = selected_file_path;
        }
    }
   else
#endif
    {
        ctx_add_to_playlist = NULL;
        if (file != NULL)
        {
            strmemccpy(selected_file_path, file, MAX_PATH);
            selected_file = selected_file_path;
        }

    }
    selected_file_attr = attr;
    int menu_selection;
#ifdef HAVE_HOTKEY
    if (hotkey)
        return execute_hotkey(context == CONTEXT_WPS);
#else
    (void)hotkey;
#endif

    push_current_activity(ACTIVITY_CONTEXTMENU);
    if (context == CONTEXT_WPS)
        menu = &wps_onplay_menu;
    else
        menu = &tree_onplay_menu;
    menu_selection = do_menu(menu, NULL, NULL, false);

    if (get_current_activity() == ACTIVITY_CONTEXTMENU) /* Activity may have been      */
        pop_current_activity();     /* popped already by menu item */

    switch (menu_selection)
    {
        case GO_TO_WPS:
            return ONPLAY_START_PLAY;
        case GO_TO_ROOT:
        case GO_TO_MAINMENU:
            return ONPLAY_MAINMENU;
        case GO_TO_PLAYLIST_VIEWER:
            return ONPLAY_PLAYLIST;
        case GO_TO_PLUGIN:
            return ONPLAY_PLUGIN;
        default:
            return onplay_result;
    }
}

int get_onplay_context(void)
{
    return context;
}

