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
#include "fileop.h"
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

static struct selected_file
{
    char buf[MAX_PATH];
    const char *path;
    int attr;
    int context;
} selected_file;

static struct clipboard
{
    char path[MAX_PATH];    /* Clipped file's path */
    unsigned int attr;      /* Clipped file's attributes */
    unsigned int flags;     /* Operation type flags */
} clipboard;

/* set selected file (doesn't touch buffer) */
static void selected_file_set(int context, const char *path, int attr)
{
    selected_file.path     = path;
    selected_file.attr     = attr;
    selected_file.context  = context;
}

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
        pop_current_activity_without_refresh();          /* when called from ctxt menu */

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
    if (action == ACTION_REQUEST_MENUITEM)
    {
        /* hide loading bookmarks menu if no bookmarks exist */
        if (this_item == &bookmark_load_menu_item)
        {
            if (!bookmark_exists())
                return ACTION_EXIT_MENUITEM;
        }
    }
    else if (action == ACTION_EXIT_MENUITEM)
        settings_save();

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

#ifdef HAVE_ALBUMART
static void view_album_art(void)
{
    plugin_load(VIEWERS_DIR"/imageviewer.rock", NULL);
}
#endif

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
#define PL_NONE    0x00
#define PL_QUEUE   0x01
#define PL_REPLACE 0x02
struct add_to_pl_param
{
    int8_t position;
    uint8_t flags;
};

static struct add_to_pl_param addtopl_insert           = {PLAYLIST_INSERT, PL_NONE};
static struct add_to_pl_param addtopl_insert_first     = {PLAYLIST_INSERT_FIRST, PL_NONE};
static struct add_to_pl_param addtopl_insert_last      = {PLAYLIST_INSERT_LAST, PL_NONE};
static struct add_to_pl_param addtopl_insert_shuf      = {PLAYLIST_INSERT_SHUFFLED, PL_NONE};
static struct add_to_pl_param addtopl_insert_last_shuf = {PLAYLIST_INSERT_LAST_SHUFFLED, PL_NONE};

static struct add_to_pl_param addtopl_queue            = {PLAYLIST_INSERT, PL_QUEUE};
static struct add_to_pl_param addtopl_queue_first      = {PLAYLIST_INSERT_FIRST, PL_QUEUE};
static struct add_to_pl_param addtopl_queue_last       = {PLAYLIST_INSERT_LAST, PL_QUEUE};
static struct add_to_pl_param addtopl_queue_shuf       = {PLAYLIST_INSERT_SHUFFLED, PL_QUEUE};
static struct add_to_pl_param addtopl_queue_last_shuf  = {PLAYLIST_INSERT_LAST_SHUFFLED, PL_QUEUE};

static struct add_to_pl_param addtopl_replace          = {PLAYLIST_INSERT, PL_REPLACE};
static struct add_to_pl_param addtopl_replace_shuffled = {PLAYLIST_INSERT_LAST_SHUFFLED, PL_REPLACE};

static void op_playlist_insert_selected(int position, bool queue)
{
#ifdef HAVE_TAGCACHE
    if (selected_file.context == CONTEXT_STD && ctx_current_playlist_insert != NULL)
    {
        ctx_current_playlist_insert(position, queue, false);
        return;
    }
#endif
    if ((selected_file.attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO)
        playlist_insert_track(NULL, selected_file.path, position, queue, true);
    else if ((selected_file.attr & FILE_ATTR_MASK) == FILE_ATTR_M3U)
        playlist_insert_playlist(NULL, selected_file.path, position, queue);
    else if (selected_file.attr & ATTR_DIRECTORY)
    {
#ifdef HAVE_TAGCACHE
        if (selected_file.context == CONTEXT_ID3DB)
        {
            tagtree_current_playlist_insert(position, queue);
            return;
        }
#endif
        bool recurse = (global_settings.recursive_dir_insert == RECURSE_ON);
        if (global_settings.recursive_dir_insert == RECURSE_ASK)
        {

            const char *lines[] = {
                ID2P(LANG_RECURSE_DIRECTORY_QUESTION),
                selected_file.path
            };
            const struct text_message message={lines, 2};
            /* Ask if user wants to recurse directory */
            recurse = (gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES);
        }

        playlist_insert_directory(NULL, selected_file.path, position, queue,
                                  recurse == RECURSE_ON);
    }
}

/* CONTEXT_[TREE|ID3DB|STD] playlist options */
static int add_to_playlist(void* arg)
{
    struct add_to_pl_param* param = arg;
    int position = param->position;
    bool new_playlist = (param->flags & PL_REPLACE) == PL_REPLACE;
    bool queue = (param->flags & PL_QUEUE) == PL_QUEUE;

    /* warn if replacing the playlist */
    if (new_playlist && !warn_on_pl_erase())
        return 1;

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

    op_playlist_insert_selected(position, queue);

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
    return 0;
}

static bool view_playlist(void)
{
    bool result;

    result = playlist_viewer_ex(selected_file.path, NULL);

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
    int sel_file_attr = (selected_file.attr & FILE_ATTR_MASK);

    switch (action)
    {
    case ACTION_REQUEST_MENUITEM:
        if (this_item == &tree_playlist_menu)
        {
            if (sel_file_attr != FILE_ATTR_AUDIO &&
                sel_file_attr != FILE_ATTR_M3U &&
                (selected_file.attr & ATTR_DIRECTORY) == 0)
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

            if ((param->flags & PL_QUEUE) == PL_QUEUE)
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

                if (sel_file_attr != FILE_ATTR_M3U &&
                    (selected_file.attr & ATTR_DIRECTORY) == 0)
                    return ACTION_EXIT_MENUITEM;
            }

            if ((param->flags & PL_REPLACE) != PL_REPLACE)
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
    ctx_current_playlist_insert = playlist_insert_cb;
    selected_file_set(CONTEXT_STD, path, attr);
    in_queue_submenu = false;
    do_menu(&tree_playlist_menu, NULL, NULL, false);
}

/* playlist catalog options */
static bool cat_add_to_a_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file.path, selected_file.attr,
                                     false, NULL, ctx_add_to_playlist);
}

static bool cat_add_to_a_new_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file.path, selected_file.attr,
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
    ctx_add_to_playlist = add_to_pl_cb;
    selected_file_set(CONTEXT_STD, track_name, attr);
    do_menu(&cat_playlist_menu, NULL, NULL, false);
}

static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    if (!selected_file.path ||
        (((selected_file.attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO) &&
         ((selected_file.attr & FILE_ATTR_MASK) != FILE_ATTR_M3U) &&
         ((selected_file.attr & ATTR_DIRECTORY) == 0)))
    {
        return ACTION_EXIT_MENUITEM;
    }

    if (action == ACTION_REQUEST_MENUITEM)
    {
        if ((audio_status() & AUDIO_STATUS_PLAY)
           || selected_file.context != CONTEXT_WPS)
        {
            return action;
        }
        return ACTION_EXIT_MENUITEM;
    }
    return action;
}

static void splash_cancelled(void)
{
    clear_screen_buffer(true);
    splash(HZ, ID2P(LANG_CANCEL));
}

static void splash_failed(int lang_what, int err)
{
    cond_talk_ids_fq(lang_what, LANG_FAILED);
    clear_screen_buffer(true);
    splashf(HZ*2, "%s %s (%d)", str(lang_what), str(LANG_FAILED), err);
}

static bool clipboard_cut(void)
{
    return clipboard_clip(&clipboard, selected_file.path, selected_file.attr,
                          PASTE_CUT);
}

static bool clipboard_copy(void)
{
    return clipboard_clip(&clipboard, selected_file.path, selected_file.attr,
                          PASTE_COPY);
}

/* Paste the clipboard to the current directory */
static int clipboard_paste(void)
{
    if (!clipboard.path[0])
        return 1;

    int rc = copy_move_fileobject(clipboard.path, getcwd(NULL, 0), clipboard.flags);

    switch (rc)
    {
    case FORC_CANCELLED:
        splash_cancelled();
        /* Fallthrough */
    case FORC_SUCCESS:
        onplay_result = ONPLAY_RELOAD_DIR;
        /* Fallthrough */
    case FORC_NOOP:
        clipboard_clear_selection(&clipboard);
        /* Fallthrough */
    case FORC_NOOVERWRT:
        break;
    default:
        if (rc < FORC_SUCCESS) {
            splash_failed(LANG_PASTE, rc);
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
    if (action == ACTION_REQUEST_MENUITEM)
    {
        if (!selected_file.path || !global_settings.runtimedb || !tagcache_is_usable())
            return ACTION_EXIT_MENUITEM;
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
    if (action == ACTION_REQUEST_MENUITEM)
    {
        if (!selected_file.path || !id3 || !id3->cuesheet)
            return ACTION_EXIT_MENUITEM;
    }
    return action;
}
MENUITEM_FUNCTION(view_cue_item, 0, ID2P(LANG_BROWSE_CUESHEET),
                  view_cue, view_cue_item_callback, Icon_NOICON);


static int browse_id3_wrapper(void)
{
    if (get_current_activity() == ACTIVITY_CONTEXTMENU)  /* get rid of parent activity */
        pop_current_activity_without_refresh();          /* when called from ctxt menu */

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
#ifdef HAVE_ALBUMART
MENUITEM_FUNCTION(view_album_art_item, 0, ID2P(LANG_VIEW_ALBUMART),
                  view_album_art, NULL, Icon_NOICON);
#endif

static int clipboard_delete_selected_fileobject(void)
{
    int rc = delete_fileobject(selected_file.path);
    if (rc < FORC_SUCCESS) {
        splash_failed(LANG_DELETE, rc);
    } else if (rc == FORC_CANCELLED) {
        splash_cancelled();
    }
    if (rc != FORC_NOOP) {
        /* Could have failed after some but not all needed changes; reload */
        onplay_result = ONPLAY_RELOAD_DIR;
    }
    return 1;
}

static void show_result(int rc, int lang_what)
{
    if (rc < FORC_SUCCESS) {
        splash_failed(lang_what, rc);
    } else if (rc == FORC_CANCELLED) {
        /* splash_cancelled(); kbd_input() splashes it */
    } else if (rc == FORC_SUCCESS) {
        onplay_result = ONPLAY_RELOAD_DIR;
    }
}

static int clipboard_create_dir(void)
{
    int rc = create_dir();

    show_result(rc, LANG_CREATE_DIR);

    return 1;
}

static int clipboard_rename_selected_file(void)
{
    int rc = rename_file(selected_file.path);

    show_result(rc, LANG_RENAME);

    return 1;
}

/* CONTEXT_[TREE|ID3DB] items */
static int clipboard_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list);

MENUITEM_FUNCTION(rename_file_item, 0, ID2P(LANG_RENAME),
                  clipboard_rename_selected_file, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_cut_item, 0, ID2P(LANG_CUT),
                  clipboard_cut, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_copy_item, 0, ID2P(LANG_COPY),
                  clipboard_copy, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_paste_item, 0, ID2P(LANG_PASTE),
                  clipboard_paste, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_file_item, 0, ID2P(LANG_DELETE),
                  clipboard_delete_selected_fileobject, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_dir_item, 0, ID2P(LANG_DELETE_DIR),
                 clipboard_delete_selected_fileobject, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(create_dir_item, 0, ID2P(LANG_CREATE_DIR),
                  clipboard_create_dir, clipboard_callback, Icon_NOICON);

/* other items */
static bool list_viewers(void)
{
    int ret = filetype_list_viewers(selected_file.path);
    if (ret == PLUGIN_USB_CONNECTED)
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}

#ifdef HAVE_TAGCACHE
static bool prepare_database_sel(void *param)
{
    if (selected_file.context == CONTEXT_ID3DB &&
        (selected_file.attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
    {
        if (!strcmp(param, "properties"))
            strmemccpy(selected_file.buf, MAKE_ACT_STR(ACTIVITY_DATABASEBROWSER),
                       sizeof(selected_file.buf));
        else if (!tagtree_get_subentry_filename(selected_file.buf, MAX_PATH))
        {
            onplay_result = ONPLAY_RELOAD_DIR;
            return false;
        }

        selected_file.path = selected_file.buf;
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

    if (get_current_activity() == ACTIVITY_CONTEXTMENU)  /* get rid of parent activity */
        pop_current_activity_without_refresh();          /* when called from ctxt menu */

    int ret = filetype_load_plugin((const char*)param, selected_file.path);
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
    shortcuts_add(SHORTCUT_BROWSER, selected_file.path);
    return false;
}
MENUITEM_FUNCTION(add_to_faves_item, 0, ID2P(LANG_ADD_TO_FAVES),
                  onplay_add_to_shortcuts,
                  clipboard_callback, Icon_NOICON);

static void set_dir_helper(char* dirnamebuf, size_t bufsz)
{
    path_append(dirnamebuf, selected_file.path, PA_SEP_HARD, bufsz);
    settings_save();
}

#if LCD_DEPTH > 1
static bool set_backdrop(void)
{
    set_dir_helper(global_settings.backdrop_file,
                   sizeof(global_settings.backdrop_file));

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
    set_dir_helper(global_settings.rec_directory,
                   sizeof(global_settings.rec_directory));
    return false;
}
MENUITEM_FUNCTION(set_recdir_item, 0, ID2P(LANG_RECORDING_DIR),
                  set_recdir, clipboard_callback, Icon_Recording);
#endif
static bool set_startdir(void)
{
    set_dir_helper(global_settings.start_directory,
                   sizeof(global_settings.start_directory));
    return false;
}
MENUITEM_FUNCTION(set_startdir_item, 0, ID2P(LANG_START_DIR),
                  set_startdir, clipboard_callback, Icon_file_view_menu);

static bool set_catalogdir(void)
{
    catalog_set_directory(selected_file.path);
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_catalogdir_item, 0, ID2P(LANG_PLAYLIST_DIR),
                  set_catalogdir, clipboard_callback, Icon_Playlist);

#ifdef HAVE_TAGCACHE
static bool set_databasedir(void)
{
    struct tagcache_stat *tc_stat = tagcache_get_stat();
    if (strcasecmp(selected_file.path, tc_stat->db_path))
    {
        splash(HZ, ID2P(LANG_PLEASE_REBOOT));
    }

    set_dir_helper(global_settings.tagcache_db_path,
                   sizeof(global_settings.tagcache_db_path));
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
            if ((selected_file.attr & ATTR_VOLUME) &&
                (this_item == &rename_file_item ||
                 this_item == &delete_dir_item ||
                 this_item == &clipboard_cut_item ||
                 this_item == &list_viewers_item))
                return ACTION_EXIT_MENUITEM;
#endif
#ifdef HAVE_TAGCACHE
            if (selected_file.context == CONTEXT_ID3DB)
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
            else if (this_item == &create_dir_item &&
                     *tree_get_context()->dirfilter <= NUM_FILTER_MODES)
            {
                return action;
            }
            else if (selected_file.path)
            {
                /* requires an actual file */
                if (this_item == &rename_file_item ||
                    this_item == &clipboard_cut_item ||
                    this_item == &clipboard_copy_item ||
                    (this_item == &track_info_item &&
                        (selected_file.attr & FILE_ATTR_MASK) == FILE_ATTR_AUDIO) ||
                    (this_item == &properties_item &&
                        (selected_file.attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO) ||
                    this_item == &add_to_faves_item)
                {
                    return action;
                }
                else if ((selected_file.attr & ATTR_DIRECTORY))
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
                    char *suffix = strrchr(selected_file.path, '.');
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
#ifdef HAVE_ALBUMART
           &view_album_art_item,
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
                if ((selected_file.attr & FILE_ATTR_MASK) == FILE_ATTR_M3U &&
                        selected_file.context == CONTEXT_TREE)
                    return action;
            }
            return ACTION_EXIT_MENUITEM;
            break;
        case ACTION_EXIT_MENUITEM:
            return ACTION_EXIT_AFTER_THIS_MENUITEM;
            break;
        default:
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
    if (selected_file.attr & ATTR_VOLUME)
        return false;
#endif

#ifdef HAVE_TAGCACHE
    if (selected_file.context == CONTEXT_ID3DB &&
        (selected_file.attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
        return false;
#endif

    return clipboard_delete_selected_fileobject();
}

static bool hotkey_open_with(void)
{
    /* only open files */
    if (selected_file.attr & ATTR_DIRECTORY)
        return false;
#ifdef HAVE_MULTIVOLUME
    if (selected_file.attr & ATTR_VOLUME)
        return false;
#endif
    return list_viewers();
}

static int hotkey_tree_pl_insert_shuffled(void)
{
    if ((audio_status() & AUDIO_STATUS_PLAY) ||
        (selected_file.attr & ATTR_DIRECTORY) ||
        ((selected_file.attr & FILE_ATTR_MASK) == FILE_ATTR_M3U))
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
    if (filetype_load_plugin((const char*)param, selected_file.path) == PLUGIN_GOTO_WPS)
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

int onplay(char* file, int attr, int from_context, bool hotkey, int customaction)
{
    const struct menu_item_ex *menu;
    onplay_result = ONPLAY_OK;
    ctx_current_playlist_insert = NULL;
    selected_file_set(from_context, NULL, attr);

#ifdef HAVE_TAGCACHE
    if (from_context == CONTEXT_ID3DB &&
        (attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO)
    {
        ctx_add_to_playlist = tagtree_add_to_playlist;
        if (file != NULL)
        {
            /* add a leading slash so that catalog_add_to_a_playlist
               later prefills the name when creating a new playlist */
            snprintf(selected_file.buf, MAX_PATH, "/%s", file);
            selected_file.path = selected_file.buf;
        }
    }
   else
#endif
    {
        ctx_add_to_playlist = NULL;
        if (file != NULL)
        {
            strmemccpy(selected_file.buf, file, MAX_PATH);
            selected_file.path = selected_file.buf;
        }

    }
    int menu_selection;

#ifdef HAVE_HOTKEY
    if (hotkey)
        return execute_hotkey(from_context == CONTEXT_WPS);
#else
    (void)hotkey;
#endif
    if (customaction == ONPLAY_CUSTOMACTION_SHUFFLE_SONGS)
    {
        int returnCode = add_to_playlist(&addtopl_replace_shuffled);
        if (returnCode == 1)
            // User did not want to erase his current playlist, so let's show again the database main menu
            return ONPLAY_RELOAD_DIR;
        return ONPLAY_START_PLAY;
    }

    push_current_activity(ACTIVITY_CONTEXTMENU);
    if (from_context == CONTEXT_WPS)
        menu = &wps_onplay_menu;
    else
        menu = &tree_onplay_menu;
    menu_selection = do_menu(menu, NULL, NULL, false);

    if (get_current_activity() == ACTIVITY_CONTEXTMENU) /* Activity may have been      */
        pop_current_activity();                         /* popped already by menu item */


    if (menu_selection == GO_TO_WPS)
        return ONPLAY_START_PLAY;
    if (menu_selection == GO_TO_ROOT)
        return ONPLAY_MAINMENU;
    if (menu_selection == GO_TO_MAINMENU)
        return ONPLAY_MAINMENU;
    if (menu_selection == GO_TO_PLAYLIST_VIEWER)
        return ONPLAY_PLAYLIST;
    if (menu_selection == GO_TO_PLUGIN)
        return ONPLAY_PLUGIN;

    return onplay_result;
}

int get_onplay_context(void)
{
    return selected_file.context;
}
