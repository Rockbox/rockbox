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

static int context;
static const char *selected_file = NULL;
static int selected_file_attr = 0;
static int onplay_result = ONPLAY_OK;
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
    if (strlcpy(clip->path, path, sizeof (clip->path))
            < sizeof (clip->path)) {
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

static int bookmark_menu_callback(int action,
                                  const struct menu_item_ex *this_item,
                                  struct gui_synclist *this_list);
MENUITEM_FUNCTION(bookmark_create_menu_item, 0,
                  ID2P(LANG_BOOKMARK_MENU_CREATE),
                  bookmark_create_menu, NULL,
                  bookmark_menu_callback, Icon_Bookmark);
MENUITEM_FUNCTION(bookmark_load_menu_item, 0,
                  ID2P(LANG_BOOKMARK_MENU_LIST),
                  bookmark_load_menu, NULL,
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

/* playing_time screen context */
struct playing_time_info {
    int curr_playing; /* index of currently playing track in playlist */
    int nb_tracks; /* how many tracks in playlist */
    /* seconds before and after current position, and total.  Datatype
       allows for values up to 68years.  If I had kept it in ms
       though, it would have overflowed at 24days, which takes
       something like 8.5GB at 32kbps, and so we could conceivably
       have playlists lasting longer than that. */
    long secs_bef, secs_aft, secs_ttl;
    long trk_secs_bef, trk_secs_aft, trk_secs_ttl;
    /* kilobytes played before and after current pos, and total.
       Kilobytes because bytes would overflow. Data type range is up
       to 2TB. */
    long kbs_bef, kbs_aft, kbs_ttl;
};

/* list callback for playing_time screen */
static const char * playing_time_get_or_speak_info(int selected_item, void * data,
                                                   char *buf, size_t buffer_len,
                                                   bool say_it)
{
    struct playing_time_info *pti = (struct playing_time_info *)data;
    switch(selected_item) {
    case 0: { /* elapsed and total time */
        char timestr1[25], timestr2[25];
        format_time_auto(timestr1, sizeof(timestr1), pti->secs_bef,
                         UNIT_SEC, false);
        format_time_auto(timestr2, sizeof(timestr2), pti->secs_ttl,
                         UNIT_SEC, false);
        long elapsed_perc; /* percentage of duration elapsed */
        if (pti->secs_ttl == 0)
            elapsed_perc = 0;
        else if (pti->secs_ttl <= 0xFFFFFF)
            elapsed_perc = pti->secs_bef *100 / pti->secs_ttl;
        else /* sacrifice some precision to avoid overflow */
            elapsed_perc = (pti->secs_bef>>7) *100 /(pti->secs_ttl>>7);
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_ELAPSED),
                 timestr1, timestr2, elapsed_perc);
        if (say_it)
            talk_ids(false, LANG_PLAYTIME_ELAPSED,
                     TALK_ID(pti->secs_bef, UNIT_TIME),
                     VOICE_OF,
                     TALK_ID(pti->secs_ttl, UNIT_TIME),
                     VOICE_PAUSE,
                     TALK_ID(elapsed_perc, UNIT_PERCENT));
        break;
    }
    case 1: { /* playlist remaining time */
        char timestr[25];
        format_time_auto(timestr, sizeof(timestr), pti->secs_aft,
			 UNIT_SEC, false);
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_REMAINING),
                 timestr);
        if (say_it)
          talk_ids(false, LANG_PLAYTIME_REMAINING,
                     TALK_ID(pti->secs_aft, UNIT_TIME));
        break;
    }
    case 2: { /* track elapsed and duration */
        char timestr1[25], timestr2[25];
        format_time_auto(timestr1, sizeof(timestr1), pti->trk_secs_bef,
			 UNIT_SEC, false);
        format_time_auto(timestr2, sizeof(timestr2), pti->trk_secs_ttl,
			 UNIT_SEC, false);
        long elapsed_perc; /* percentage of duration elapsed */
        if (pti->trk_secs_ttl == 0)
            elapsed_perc = 0;
        else if (pti->trk_secs_ttl <= 0xFFFFFF)
            elapsed_perc = pti->trk_secs_bef *100 / pti->trk_secs_ttl;
        else /* sacrifice some precision to avoid overflow */
            elapsed_perc = (pti->trk_secs_bef>>7) *100 /(pti->trk_secs_ttl>>7);
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_TRK_ELAPSED),
                 timestr1, timestr2, elapsed_perc);
        if (say_it)
            talk_ids(false, LANG_PLAYTIME_TRK_ELAPSED,
                     TALK_ID(pti->trk_secs_bef, UNIT_TIME),
                     VOICE_OF,
                     TALK_ID(pti->trk_secs_ttl, UNIT_TIME),
                     VOICE_PAUSE,
                     TALK_ID(elapsed_perc, UNIT_PERCENT));
        break;
    }
    case 3: { /* track remaining time */
        char timestr[25];
        format_time_auto(timestr, sizeof(timestr), pti->trk_secs_aft,
			 UNIT_SEC, false);
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_TRK_REMAINING),
                 timestr);
        if (say_it)
          talk_ids(false, LANG_PLAYTIME_TRK_REMAINING,
                     TALK_ID(pti->trk_secs_aft, UNIT_TIME));
        break;
    }
    case 4: { /* track index */
        int track_perc = (pti->curr_playing+1) *100 / pti->nb_tracks;
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_TRACK),
                 pti->curr_playing + 1, pti->nb_tracks, track_perc);
        if (say_it)
          talk_ids(false, LANG_PLAYTIME_TRACK,
                     TALK_ID(pti->curr_playing+1, UNIT_INT),
                     VOICE_OF,
                     TALK_ID(pti->nb_tracks, UNIT_INT),
                     VOICE_PAUSE,
                     TALK_ID(track_perc, UNIT_PERCENT));
        break;
    }
    case 5: { /* storage size */
        char str1[10], str2[10], str3[10];
        output_dyn_value(str1, sizeof(str1), pti->kbs_ttl, kibyte_units, 3, true);
        output_dyn_value(str2, sizeof(str2), pti->kbs_bef, kibyte_units, 3, true);
        output_dyn_value(str3, sizeof(str3), pti->kbs_aft, kibyte_units, 3, true);
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_STORAGE),
                 str1,str2,str3);
        if (say_it) {
            talk_id(LANG_PLAYTIME_STORAGE, false);
            output_dyn_value(NULL, 0, pti->kbs_ttl, kibyte_units, 3, true);
            talk_ids(true, VOICE_PAUSE, VOICE_PLAYTIME_DONE);
            output_dyn_value(NULL, 0, pti->kbs_bef, kibyte_units, 3, true);
            talk_id(LANG_PLAYTIME_REMAINING, true);
            output_dyn_value(NULL, 0, pti->kbs_aft, kibyte_units, 3, true);
        }
        break;
    }
    case 6: { /* Average track file size */
        char str[10];
        long avg_track_size = pti->kbs_ttl /pti->nb_tracks;
        output_dyn_value(str, sizeof(str), avg_track_size, kibyte_units, 3, true);
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_AVG_TRACK_SIZE),
                 str);
        if (say_it) {
            talk_id(LANG_PLAYTIME_AVG_TRACK_SIZE, false);
            output_dyn_value(NULL, 0, avg_track_size, kibyte_units, 3, true);
        }
        break;
    }
    case 7: { /* Average bitrate */
        /* Convert power of 2 kilobytes to power of 10 kilobits */
        long avg_bitrate = pti->kbs_ttl / pti->secs_ttl *1024 *8 /1000;
        snprintf(buf, buffer_len, str(LANG_PLAYTIME_AVG_BITRATE),
                 avg_bitrate);
        if (say_it)
          talk_ids(false, LANG_PLAYTIME_AVG_BITRATE,
                   TALK_ID(avg_bitrate, UNIT_KBIT));
        break;
    }
    }
    return buf;
}

static const char * playing_time_get_info(int selected_item, void * data,
                                          char *buffer, size_t buffer_len)
{
    return playing_time_get_or_speak_info(selected_item, data,
                                          buffer, buffer_len, false);
}

static int playing_time_speak_info(int selected_item, void * data)
{
    static char buffer[MAX_PATH];
    playing_time_get_or_speak_info(selected_item, data,
                                   buffer, MAX_PATH, true);
    return 0;
}

/* playing time screen: shows total and elapsed playlist duration and
   other stats */
static bool playing_time(void)
{
    unsigned long talked_tick = current_tick;
    struct playing_time_info pti;
    struct playlist_track_info pltrack;
    struct mp3entry id3;
    int i, fd, ret;

    pti.nb_tracks = playlist_amount();
    playlist_get_resume_info(&pti.curr_playing);
    struct mp3entry *curr_id3 = audio_current_track();
    if (pti.curr_playing == -1 || !curr_id3)
        return false;
    pti.secs_bef = pti.trk_secs_bef = curr_id3->elapsed/1000;
    pti.secs_aft = pti.trk_secs_aft
        = (curr_id3->length -curr_id3->elapsed)/1000;
    pti.kbs_bef = curr_id3->offset/1024;
    pti.kbs_aft = (curr_id3->filesize -curr_id3->offset)/1024;

    splash(0, ID2P(LANG_WAIT));

    /* Go through each file in the playlist and get its stats. For
       huge playlists this can take a while... The reference position
       is the position at the moment this function was invoked,
       although playback continues forward. */
    for (i = 0; i < pti.nb_tracks; i++) {
        /* Show a splash while we are loading. */
        splashf(0, str(LANG_LOADING_PERCENT),
                i*100/pti.nb_tracks, str(LANG_OFF_ABORT));
        /* Voice equivalent */
        if (TIME_AFTER(current_tick, talked_tick+5*HZ)) {
            talked_tick = current_tick;
            talk_ids(false, LANG_LOADING_PERCENT,
                     TALK_ID(i*100/pti.nb_tracks, UNIT_PERCENT));
        }
        if (action_userabort(TIMEOUT_NOBLOCK))
            goto exit;

        if (i == pti.curr_playing)
            continue;

        if (playlist_get_track_info(NULL, i, &pltrack) < 0)
            goto error;
        if ((fd = open(pltrack.filename, O_RDONLY)) < 0)
            goto error;
        ret = get_metadata(&id3, fd, pltrack.filename);
        close(fd);
        if (!ret)
            goto error;

        if (i < pti.curr_playing) {
            pti.secs_bef += id3.length/1000;
            pti.kbs_bef += id3.filesize/1024;
        } else {
            pti.secs_aft += id3.length/1000;
            pti.kbs_aft += id3.filesize/1024;
        }
    }

    pti.secs_ttl = pti.secs_bef +pti.secs_aft;
    pti.trk_secs_ttl = pti.trk_secs_bef +pti.trk_secs_aft;
    pti.kbs_ttl = pti.kbs_bef +pti.kbs_aft;

    struct gui_synclist pt_lists;
    int key;

    gui_synclist_init(&pt_lists, &playing_time_get_info, &pti, true, 1, NULL);
    if (global_settings.talk_menu)
        gui_synclist_set_voice_callback(&pt_lists, playing_time_speak_info);
    gui_synclist_set_nb_items(&pt_lists, 8);
    gui_synclist_draw(&pt_lists);
/*    gui_syncstatusbar_draw(&statusbars, true); */
    gui_synclist_speak_item(&pt_lists);
    while (true) {
/*        gui_syncstatusbar_draw(&statusbars, false); */
        if (list_do_action(CONTEXT_LIST, HZ/2,
                          &pt_lists, &key, LIST_WRAP_UNLESS_HELD) == 0
           && key!=ACTION_NONE && key!=ACTION_UNKNOWN)
        {
            talk_force_shutup();
            return(default_event_handler(key) == SYS_USB_CONNECTED);
        }
    }
 error:
    splash(HZ, ID2P(LANG_PLAYTIME_ERROR));
 exit:
    return false;
}


/* CONTEXT_WPS playlist options */
static bool shuffle_playlist(void)
{
    playlist_sort(NULL, true);
    playlist_randomise(NULL, current_tick, true);

    return false;
}
static bool save_playlist(void)
{
    save_playlist_screen(NULL);
    return false;
}

extern struct menu_item_ex view_cur_playlist; /* from playlist_menu.c */
MENUITEM_FUNCTION(search_playlist_item, 0, ID2P(LANG_SEARCH_IN_PLAYLIST),
                  search_playlist, NULL, NULL, Icon_Playlist);
MENUITEM_FUNCTION(playlist_save_item, 0, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),
                  save_playlist, NULL, NULL, Icon_Playlist);
MENUITEM_FUNCTION(reshuffle_item, 0, ID2P(LANG_SHUFFLE_PLAYLIST),
                  shuffle_playlist, NULL, NULL, Icon_Playlist);
MENUITEM_FUNCTION(playing_time_item, 0, ID2P(LANG_PLAYING_TIME),
                  playing_time, NULL, NULL, Icon_Playlist);
MAKE_ONPLAYMENU( wps_playlist_menu, ID2P(LANG_PLAYLIST),
                 NULL, Icon_Playlist,
                 &view_cur_playlist, &search_playlist_item,
                 &playlist_save_item, &reshuffle_item, &playing_time_item
               );

/* CONTEXT_[TREE|ID3DB] playlist options */
static bool add_to_playlist(int position, bool queue)
{
    bool new_playlist = !(audio_status() & AUDIO_STATUS_PLAY);
    const char *lines[] = {
        ID2P(LANG_RECURSE_DIRECTORY_QUESTION),
        selected_file
    };
    const struct text_message message={lines, 2};

    splash(0, ID2P(LANG_WAIT));

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
    if (context == CONTEXT_ID3DB)
    {
        tagtree_insert_selection_playlist(position, queue);
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

    return false;
}

static bool view_playlist(void)
{
    bool was_playing = audio_status() & AUDIO_STATUS_PLAY;
    bool result;

    result = playlist_viewer_ex(selected_file);

    if (!was_playing && (audio_status() & AUDIO_STATUS_PLAY) &&
        onplay_result == ONPLAY_OK)
        /* playlist was started from viewer */
        onplay_result = ONPLAY_START_PLAY;

    return result;
}

static int playlist_insert_func(void *param)
{
    if (((intptr_t)param == PLAYLIST_REPLACE) && !warn_on_pl_erase())
        return 0;
    add_to_playlist((intptr_t)param, false);
    return 0;
}

static int playlist_queue_func(void *param)
{
    add_to_playlist((intptr_t)param, true);
    return 0;
}

static int treeplaylist_wplayback_callback(int action,
                                        const struct menu_item_ex* this_item,
                                        struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (audio_status() & AUDIO_STATUS_PLAY)
                return action;
            else
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}

static int treeplaylist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list);

/* insert items */
MENUITEM_FUNCTION(i_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT,
                  NULL, Icon_Playlist);
MENUITEM_FUNCTION(i_first_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT_FIRST),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT_FIRST,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_last_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT_LAST),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT_LAST,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_shuf_pl_item, MENU_FUNC_USEPARAM,
                  ID2P(LANG_INSERT_SHUFFLED), playlist_insert_func,
                  (intptr_t*)PLAYLIST_INSERT_SHUFFLED,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_last_shuf_pl_item, MENU_FUNC_USEPARAM,
                  ID2P(LANG_INSERT_LAST_SHUFFLED), playlist_insert_func,
                  (intptr_t*)PLAYLIST_INSERT_LAST_SHUFFLED,
                  treeplaylist_callback, Icon_Playlist);
/* queue items */
MENUITEM_FUNCTION(q_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_QUEUE),
                  playlist_queue_func, (intptr_t*)PLAYLIST_INSERT,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(q_first_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_QUEUE_FIRST),
                  playlist_queue_func, (intptr_t*)PLAYLIST_INSERT_FIRST,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(q_last_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_QUEUE_LAST),
                  playlist_queue_func, (intptr_t*)PLAYLIST_INSERT_LAST,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(q_shuf_pl_item, MENU_FUNC_USEPARAM,
                  ID2P(LANG_QUEUE_SHUFFLED), playlist_queue_func,
                  (intptr_t*)PLAYLIST_INSERT_SHUFFLED,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(q_last_shuf_pl_item, MENU_FUNC_USEPARAM,
                  ID2P(LANG_QUEUE_LAST_SHUFFLED), playlist_queue_func,
                  (intptr_t*)PLAYLIST_INSERT_LAST_SHUFFLED,
                  treeplaylist_callback, Icon_Playlist);
/* replace playlist */
MENUITEM_FUNCTION(replace_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_REPLACE),
                  playlist_insert_func, (intptr_t*)PLAYLIST_REPLACE,
                  treeplaylist_wplayback_callback, Icon_Playlist);

/* others */
MENUITEM_FUNCTION(view_playlist_item, 0, ID2P(LANG_VIEW),
                  view_playlist, NULL,
                  treeplaylist_callback, Icon_Playlist);

MAKE_ONPLAYMENU( tree_playlist_menu, ID2P(LANG_CURRENT_PLAYLIST),
                 treeplaylist_callback, Icon_Playlist,

                 /* view */
                 &view_playlist_item,

                 /* insert */
                 &i_pl_item, &i_first_pl_item, &i_last_pl_item,
                 &i_shuf_pl_item, &i_last_shuf_pl_item,
                 /* queue */

                 &q_pl_item, &q_first_pl_item, &q_last_pl_item,
                 &q_shuf_pl_item, &q_last_shuf_pl_item,

                 /* replace */
                 &replace_pl_item
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
                if (((selected_file_attr & FILE_ATTR_MASK) ==
                        FILE_ATTR_AUDIO) ||
                    ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U)||
                     (selected_file_attr & ATTR_DIRECTORY))
                {
                    return action;
                }
            }
            else if (this_item == &view_playlist_item)
            {
                if ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U &&
                        context == CONTEXT_TREE)
                {
                    return action;
                }
            }
            else if (this_item == &i_shuf_pl_item)
            {
                if ((audio_status() & AUDIO_STATUS_PLAY) ||
                    (selected_file_attr & ATTR_DIRECTORY) ||
                    ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U))
                {
                    return action;
                }
            }
            else if (this_item == &i_last_shuf_pl_item ||
                     this_item == &q_last_shuf_pl_item)
            {
                if ((playlist_amount() > 0) &&
                    (audio_status() & AUDIO_STATUS_PLAY) &&
                    ((selected_file_attr & ATTR_DIRECTORY) ||
                    ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U)))
                {
                    return action;
                }
            }
            return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}

void onplay_show_playlist_menu(char* path)
{
    selected_file = path;
    if (dir_exists(path))
        selected_file_attr = ATTR_DIRECTORY;
    else
        selected_file_attr = filetype_get_attr(path);
    do_menu(&tree_playlist_menu, NULL, NULL, false);
}

/* playlist catalog options */
static bool cat_add_to_a_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file, selected_file_attr,
                                     false, NULL);
}

static bool cat_add_to_a_new_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file, selected_file_attr,
                                     true, NULL);
}
static int clipboard_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list);

static bool set_catalogdir(void)
{
    catalog_set_directory(selected_file);
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_catalogdir_item, 0, ID2P(LANG_SET_AS_PLAYLISTCAT_DIR),
                  set_catalogdir, NULL, clipboard_callback, Icon_Playlist);

static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item,
                                 struct gui_synclist *this_list);

MENUITEM_FUNCTION(cat_add_to_list, 0, ID2P(LANG_CATALOG_ADD_TO),
                  cat_add_to_a_playlist, 0, NULL, Icon_Playlist);
MENUITEM_FUNCTION(cat_add_to_new, 0, ID2P(LANG_CATALOG_ADD_TO_NEW),
                  cat_add_to_a_new_playlist, 0, NULL, Icon_Playlist);
MAKE_ONPLAYMENU(cat_playlist_menu, ID2P(LANG_CATALOG),
                cat_playlist_callback, Icon_Playlist,
                &cat_add_to_list, &cat_add_to_new, &set_catalogdir_item);

void onplay_show_playlist_cat_menu(char* track_name)
{
    selected_file = track_name;
    selected_file_attr = FILE_ATTR_AUDIO;
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
#ifdef HAVE_TAGCACHE
    if (context == CONTEXT_ID3DB &&
        ((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO))
    {
        return ACTION_EXIT_MENUITEM;
    }
#endif

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

static int confirm_overwrite(void)
{
    static const char *lines[] = { ID2P(LANG_REALLY_OVERWRITE) };
    static const struct text_message message = { lines, 1 };
    return gui_syncyesno_run(&message, NULL, NULL);
}

static int confirm_delete(const char *file)
{
    const char *lines[] = { ID2P(LANG_REALLY_DELETE), file };
    const char *yes_lines[] = { ID2P(LANG_DELETING), file };
    const struct text_message message = { lines, 2 };
    const struct text_message yes_message = { yes_lines, 2 };
    return gui_syncyesno_run(&message, &yes_message, NULL);
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
    if (confirm_delete(selected_file) != YESNO_YES) {
        return 1;
    }

    clear_display(true);
    splash(HZ/2, str(LANG_DELETING));

    int rc = -1;

    if (selected_file_attr & ATTR_DIRECTORY) { /* true if directory */
        struct dirrecurse_params parm;
        parm.append = strlcpy(parm.path, selected_file, sizeof (parm.path));

        if (parm.append < sizeof (parm.path)) {
            cpu_boost(true);
            rc = remove_dir(&parm);
            cpu_boost(false);
        }
    } else {
        rc = remove(selected_file);
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

    if (strlcpy(newname, selection, sizeof (newname)) >= sizeof (newname)) {
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
            if (confirm_overwrite() == YESNO_NO) {
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
    case OPRC_SUCCESS:
        onplay_result = ONPLAY_RELOAD_DIR;
    case OPRC_NOOP:
        clipboard_clear_selection(&clipboard);
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
                  set_rating_inline, NULL,
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
                  view_cue, NULL, view_cue_item_callback, Icon_NOICON);


static int browse_id3_wrapper(void)
{
    if (browse_id3())
        return GO_TO_ROOT;
    return GO_TO_PREVIOUS;
}

/* CONTEXT_WPS items */
MENUITEM_FUNCTION(browse_id3_item, MENU_FUNC_CHECK_RETVAL, ID2P(LANG_MENU_SHOW_ID3_INFO),
                  browse_id3_wrapper, NULL, NULL, Icon_NOICON);
#ifdef HAVE_PITCHCONTROL
MENUITEM_FUNCTION(pitch_screen_item, 0, ID2P(LANG_PITCH),
                  gui_syncpitchscreen_run, NULL, NULL, Icon_Audio);
#endif

/* CONTEXT_[TREE|ID3DB] items */
static int clipboard_callback(int action,
                              const struct menu_item_ex *this_item,
                              struct gui_synclist *this_list);

MENUITEM_FUNCTION(rename_file_item, 0, ID2P(LANG_RENAME),
                  rename_file, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_cut_item, 0, ID2P(LANG_CUT),
                  clipboard_cut, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_copy_item, 0, ID2P(LANG_COPY),
                  clipboard_copy, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_paste_item, 0, ID2P(LANG_PASTE),
                  clipboard_paste, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_file_item, 0, ID2P(LANG_DELETE),
                  delete_file_dir, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_dir_item, 0, ID2P(LANG_DELETE_DIR),
                  delete_file_dir, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(create_dir_item, 0, ID2P(LANG_CREATE_DIR),
                  create_dir, NULL, clipboard_callback, Icon_NOICON);

/* other items */
static bool list_viewers(void)
{
    int ret = filetype_list_viewers(selected_file);
    if (ret == PLUGIN_USB_CONNECTED)
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}

static bool onplay_load_plugin(void *param)
{
    int ret = filetype_load_plugin((const char*)param, selected_file);
    if (ret == PLUGIN_USB_CONNECTED)
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}

MENUITEM_FUNCTION(list_viewers_item, 0, ID2P(LANG_ONPLAY_OPEN_WITH),
                  list_viewers, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(properties_item, MENU_FUNC_USEPARAM, ID2P(LANG_PROPERTIES),
                  onplay_load_plugin, (void *)"properties",
                  clipboard_callback, Icon_NOICON);
static bool onplay_add_to_shortcuts(void)
{
    shortcuts_add(SHORTCUT_BROWSER, selected_file);
    return false;
}
MENUITEM_FUNCTION(add_to_faves_item, 0, ID2P(LANG_ADD_TO_FAVES),
                  onplay_add_to_shortcuts, NULL,
                  clipboard_callback, Icon_NOICON);

#if LCD_DEPTH > 1
static bool set_backdrop(void)
{
    strlcpy(global_settings.backdrop_file, selected_file,
            sizeof(global_settings.backdrop_file));
    settings_save();
    skin_backdrop_load_setting();
    skin_backdrop_show(sb_get_backdrop(SCREEN_MAIN));
    return true;
}
MENUITEM_FUNCTION(set_backdrop_item, 0, ID2P(LANG_SET_AS_BACKDROP),
                  set_backdrop, NULL, clipboard_callback, Icon_NOICON);
#endif
#ifdef HAVE_RECORDING
static bool set_recdir(void)
{
    strlcpy(global_settings.rec_directory, selected_file,
            sizeof(global_settings.rec_directory));
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_recdir_item, 0, ID2P(LANG_SET_AS_REC_DIR),
                  set_recdir, NULL, clipboard_callback, Icon_Recording);
#endif
static bool set_startdir(void)
{
    snprintf(global_settings.start_directory,
             sizeof(global_settings.start_directory),
             "%s/", selected_file);
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_startdir_item, 0, ID2P(LANG_SET_AS_START_DIR),
                  set_startdir, NULL, clipboard_callback, Icon_file_view_menu);

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
                if (((selected_file_attr & FILE_ATTR_MASK) ==
                        FILE_ATTR_AUDIO) &&
                    this_item == &properties_item)
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
                    this_item == &properties_item ||
                    this_item == &add_to_faves_item)
                {
                    return action;
                }
                else if ((selected_file_attr & ATTR_DIRECTORY))
                {
                    /* only for directories */
                    if (this_item == &delete_dir_item ||
                        this_item == &set_startdir_item ||
                        this_item == &set_catalogdir_item
#ifdef HAVE_RECORDING
                     || this_item == &set_recdir_item
#endif
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
/* used when onplay() is not called in the CONTEXT_WPS context */
MAKE_ONPLAYMENU( tree_onplay_menu, ID2P(LANG_ONPLAY_MENU_TITLE),
           onplaymenu_callback, Icon_file_view_menu,
           &tree_playlist_menu, &cat_playlist_menu,
           &rename_file_item, &clipboard_cut_item, &clipboard_copy_item,
           &clipboard_paste_item, &delete_file_item, &delete_dir_item,
#if LCD_DEPTH > 1
           &set_backdrop_item,
#endif
           &list_viewers_item, &create_dir_item, &properties_item,
#ifdef HAVE_RECORDING
           &set_recdir_item,
#endif
           &set_startdir_item, &add_to_faves_item, &file_menu,
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
        case ACTION_EXIT_MENUITEM:
            return ACTION_EXIT_AFTER_THIS_MENUITEM;
            break;
    }
    return action;
}

#ifdef HAVE_HOTKEY
/* direct function calls, no need for menu callbacks */
static bool delete_item(void)
{
#ifdef HAVE_MULTIVOLUME
    /* no delete for volumes */
    if (selected_file_attr & ATTR_VOLUME)
        return false;
#endif
    return delete_file_dir();
}

static bool open_with(void)
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

static int playlist_insert_shuffled(void)
{
    if ((audio_status() & AUDIO_STATUS_PLAY) ||
        (selected_file_attr & ATTR_DIRECTORY) ||
        ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U))
    {
        playlist_insert_func((intptr_t*)PLAYLIST_INSERT_SHUFFLED);
        return ONPLAY_START_PLAY;
    }

    return ONPLAY_RELOAD_DIR;
}

static void hotkey_run_plugin(void)
{
    open_plugin_run(ID2P(LANG_HOTKEY_WPS));
}

struct hotkey_assignment {
    int action;             /* hotkey_action */
    int lang_id;            /* Language ID */
    struct menu_func func;  /* Function to run if this entry is selected */
    int return_code;        /* What to return after the function is run  */
};

#define HOTKEY_FUNC(func, param) {{(void *)func}, param}

/* Any desired hotkey functions go here, in the enum in onplay.h,
   and in the settings menu in settings_list.c.  The order here
   is not important. */
static struct hotkey_assignment hotkey_items[] = {
    { HOTKEY_VIEW_PLAYLIST,     LANG_VIEW_DYNAMIC_PLAYLIST,
            HOTKEY_FUNC(NULL, NULL),
            ONPLAY_PLAYLIST },
    { HOTKEY_SHOW_TRACK_INFO,   LANG_MENU_SHOW_ID3_INFO,
            HOTKEY_FUNC(browse_id3, NULL),
            ONPLAY_RELOAD_DIR },
#ifdef HAVE_PITCHCONTROL
    { HOTKEY_PITCHSCREEN,       LANG_PITCH,
            HOTKEY_FUNC(gui_syncpitchscreen_run, NULL),
            ONPLAY_RELOAD_DIR },
#endif
    { HOTKEY_OPEN_WITH,         LANG_ONPLAY_OPEN_WITH,
            HOTKEY_FUNC(open_with, NULL),
            ONPLAY_RELOAD_DIR },
    { HOTKEY_DELETE,            LANG_DELETE,
            HOTKEY_FUNC(delete_item, NULL),
            ONPLAY_RELOAD_DIR },
    { HOTKEY_INSERT,            LANG_INSERT,
            HOTKEY_FUNC(playlist_insert_func, (intptr_t*)PLAYLIST_INSERT),
            ONPLAY_RELOAD_DIR },
    { HOTKEY_INSERT_SHUFFLED,   LANG_INSERT_SHUFFLED,
            HOTKEY_FUNC(playlist_insert_shuffled, NULL),
            ONPLAY_RELOAD_DIR },
    { HOTKEY_PLUGIN, LANG_OPEN_PLUGIN,
            HOTKEY_FUNC(hotkey_run_plugin, NULL),
            ONPLAY_OK },
    { HOTKEY_BOOKMARK,   LANG_BOOKMARK_MENU_CREATE,
            HOTKEY_FUNC(bookmark_create_menu, NULL),
            ONPLAY_OK },
};

/* Return the language ID for this action */
int get_hotkey_lang_id(int action)
{
    int i = ARRAYLEN(hotkey_items);
    while (i--)
    {
        if (hotkey_items[i].action == action)
            return hotkey_items[i].lang_id;
    }

    return LANG_OFF;
}

/* Execute the hotkey function, if listed */
static int execute_hotkey(bool is_wps)
{
    int i = ARRAYLEN(hotkey_items);
    struct hotkey_assignment *this_item;
    const int action = (is_wps ? global_settings.hotkey_wps :
        global_settings.hotkey_tree);

    /* search assignment struct for a match for the hotkey setting */
    while (i--)
    {
        this_item = &hotkey_items[i];
        if (this_item->action == action)
        {
            /* run the associated function (with optional param), if any */
            const struct menu_func func = this_item->func;
            int func_return = ONPLAY_RELOAD_DIR;
            if (func.function != NULL)
            {
                if (func.param != NULL)
                    func_return = (*func.function_w_param)(func.param);
                else
                    func_return = (*func.function)();
            }
            /* return with the associated code */
            const int return_code = this_item->return_code;
            /* ONPLAY_OK here means to use the function return code */
            if (return_code == ONPLAY_OK)
                return func_return;
            return return_code;
        }
    }

    /* no valid hotkey set, ignore hotkey */
    return ONPLAY_RELOAD_DIR;
}
#endif /* HOTKEY */

int onplay(char* file, int attr, int from, bool hotkey)
{
    const struct menu_item_ex *menu;
    onplay_result = ONPLAY_OK;
    context = from;
    selected_file = file;
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
    pop_current_activity();

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
