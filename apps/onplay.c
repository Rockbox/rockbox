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
#include "dir.h"
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
#include "plugin.h"
#include "bookmark.h"
#include "action.h"
#include "splash.h"
#include "yesno.h"
#include "menus/exported_menus.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif
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

static int context;
static char* selected_file = NULL;
static int selected_file_attr = 0;
static int onplay_result = ONPLAY_OK;
static char clipboard_selection[MAX_PATH];
static int clipboard_selection_attr = 0;
static bool clipboard_is_copy = false;

/* redefine MAKE_MENU so the MENU_EXITAFTERTHISMENU flag can be added easily */
#define MAKE_ONPLAYMENU( name, str, callback, icon, ... )               \
    static const struct menu_item_ex *name##_[]  = {__VA_ARGS__};       \
    static const struct menu_callback_with_desc name##__ = {callback,str,icon};\
    static const struct menu_item_ex name =                             \
        {MT_MENU|MENU_HAS_DESC|MENU_EXITAFTERTHISMENU|                  \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { (void*)name##_},{.callback_and_desc = & name##__}};

/* ----------------------------------------------------------------------- */
/* Displays the bookmark menu options for the user to decide.  This is an  */
/* interface function.                                                     */
/* ----------------------------------------------------------------------- */

static int bookmark_menu_callback(int action,
                                  const struct menu_item_ex *this_item);
MENUITEM_FUNCTION(bookmark_create_menu_item, 0,
                  ID2P(LANG_BOOKMARK_MENU_CREATE),
                  bookmark_create_menu, NULL, NULL, Icon_Bookmark);
MENUITEM_FUNCTION(bookmark_load_menu_item, 0,
                  ID2P(LANG_BOOKMARK_MENU_LIST),
                  bookmark_load_menu, NULL,
                  bookmark_menu_callback, Icon_Bookmark);
MAKE_ONPLAYMENU(bookmark_menu, ID2P(LANG_BOOKMARK_MENU),
                bookmark_menu_callback, Icon_Bookmark,
                &bookmark_create_menu_item, &bookmark_load_menu_item);
static int bookmark_menu_callback(int action,
                                  const struct menu_item_ex *this_item)
{
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (this_item == &bookmark_load_menu_item)
            {
                if (bookmark_exist() == 0)
                    return ACTION_EXIT_MENUITEM;
            }
            /* hide the bookmark menu if there is no playback */
            else if ((audio_status() & AUDIO_STATUS_PLAY) == 0)
                return ACTION_EXIT_MENUITEM;
            break;
#ifdef HAVE_LCD_CHARCELLS
        case ACTION_ENTER_MENUITEM:
            status_set_param(true);
            break;
#endif
        case ACTION_EXIT_MENUITEM:
#ifdef HAVE_LCD_CHARCELLS
            status_set_param(false);
#endif
            settings_save();
            break;
    }
    return action;
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
MAKE_ONPLAYMENU( wps_playlist_menu, ID2P(LANG_PLAYLIST),
                 NULL, Icon_Playlist,
                 &view_cur_playlist, &search_playlist_item,
                 &playlist_save_item, &reshuffle_item
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
        playlist_start(0,0);
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
                                        const struct menu_item_ex* this_item)
{
    (void)this_item;
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
                                 const struct menu_item_ex *this_item);

/* insert items */
MENUITEM_FUNCTION(i_pl_item, MENU_FUNC_USEPARAM | MENU_FUNC_HOTKEYABLE,
                  ID2P(LANG_INSERT),
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

MAKE_ONPLAYMENU( tree_playlist_menu, ID2P(LANG_PLAYLIST),
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
                                 const struct menu_item_ex *this_item)
{
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

static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item);
MENUITEM_FUNCTION(cat_view_lists, 0, ID2P(LANG_CATALOG_VIEW),
                  catalog_view_playlists, 0,
                  cat_playlist_callback, Icon_Playlist);
MENUITEM_FUNCTION(cat_add_to_list, 0, ID2P(LANG_CATALOG_ADD_TO),
                  cat_add_to_a_playlist, 0, NULL, Icon_Playlist);
MENUITEM_FUNCTION(cat_add_to_new, 0, ID2P(LANG_CATALOG_ADD_TO_NEW),
                  cat_add_to_a_new_playlist, 0, NULL, Icon_Playlist);
MAKE_ONPLAYMENU(cat_playlist_menu, ID2P(LANG_CATALOG),
                cat_playlist_callback, Icon_Playlist,
                &cat_view_lists, &cat_add_to_list, &cat_add_to_new);

static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item)
{
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
            if (this_item == &cat_view_lists)
            {
                return action;
            }
            else if ((audio_status() & AUDIO_STATUS_PLAY) ||
                     context != CONTEXT_WPS)
            {
                return action;
            }
            else
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}

#ifdef HAVE_LCD_BITMAP
static void draw_slider(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        struct viewport vp;
        int slider_height = 2*screens[i].getcharheight();
        viewport_set_defaults(&vp, i);
        screens[i].set_viewport(&vp);
        show_busy_slider(&screens[i], 1, vp.height - slider_height,
                         vp.width-2, slider_height-1);
        screens[i].update_viewport();
        screens[i].set_viewport(NULL);
    }
}
#else
#define draw_slider()
#endif

/* helper function to remove a non-empty directory */
static int remove_dir(char* dirname, int len)
{
    int result = 0;
    DIR* dir;
    int dirlen = strlen(dirname);

    dir = opendir(dirname);
    if (!dir)
        return -1; /* open error */

    while(true)
    {
        struct dirent* entry;
        /* walk through the directory content */
        entry = readdir(dir);
        if (!entry)
            break;

        dirname[dirlen] ='\0';
        /* inform the user which dir we're deleting */
        splash(0, dirname);

        /* append name to current directory */
        snprintf(dirname+dirlen, len-dirlen, "/%s", entry->d_name);
        if (entry->attribute & ATTR_DIRECTORY)
        {   /* remove a subdirectory */
            if (!strcmp((char *)entry->d_name, ".") ||
                !strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            result = remove_dir(dirname, len); /* recursion */
            if (result)
                break; /* or better continue, delete what we can? */
        }
        else
        {   /* remove a file */
            draw_slider();
            result = remove(dirname);
        }
        if(ACTION_STD_CANCEL == get_action(CONTEXT_STD,TIMEOUT_NOBLOCK))
        {
            splash(HZ, ID2P(LANG_CANCEL));
            result = -1;
            break;
        }
    }
    closedir(dir);

    if (!result)
    {   /* remove the now empty directory */
        dirname[dirlen] = '\0'; /* terminate to original length */

        result = rmdir(dirname);
    }

    return result;
}


/* share code for file and directory deletion, saves space */
static bool delete_file_dir(void)
{
    char file_to_delete[MAX_PATH];
    strcpy(file_to_delete, selected_file);

    const char *lines[]={
        ID2P(LANG_REALLY_DELETE),
        file_to_delete
    };
    const char *yes_lines[]={
        ID2P(LANG_DELETING),
        file_to_delete
    };

    const struct text_message message={lines, 2};
    const struct text_message yes_message={yes_lines, 2};

    if(gui_syncyesno_run(&message, &yes_message, NULL)!=YESNO_YES)
        return false;

    splash(0, str(LANG_DELETING));

    int res;
    if (selected_file_attr & ATTR_DIRECTORY) /* true if directory */
    {
        char pathname[MAX_PATH]; /* space to go deep */
        cpu_boost(true);
        strlcpy(pathname, file_to_delete, sizeof(pathname));
        res = remove_dir(pathname, sizeof(pathname));
        cpu_boost(false);
    }
    else
        res = remove(file_to_delete);

    if (!res)
        onplay_result = ONPLAY_RELOAD_DIR;

    return (res == 0);
}

static bool rename_file(void)
{
    char newname[MAX_PATH];
    char* ptr = strrchr(selected_file, '/') + 1;
    int pathlen = (ptr - selected_file);
    strlcpy(newname, selected_file, sizeof(newname));
    if (!kbd_input(newname + pathlen, (sizeof newname)-pathlen)) {
        if (!strlen(newname + pathlen) ||
            (rename(selected_file, newname) < 0)) {
            cond_talk_ids_fq(LANG_RENAME, LANG_FAILED);
            splashf(HZ*2, "%s %s", str(LANG_RENAME), str(LANG_FAILED));
        }
        else
            onplay_result = ONPLAY_RELOAD_DIR;
    }

    return false;
}

static bool create_dir(void)
{
    char dirname[MAX_PATH];
    char *cwd;
    int rc;
    int pathlen;

    cwd = getcwd(NULL, 0);
    memset(dirname, 0, sizeof dirname);

    snprintf(dirname, sizeof dirname, "%s/", cwd[1] ? cwd : "");

    pathlen = strlen(dirname);
    rc = kbd_input(dirname + pathlen, (sizeof dirname)-pathlen);
    if (rc < 0)
        return false;

    rc = mkdir(dirname);
    if (rc < 0) {
        cond_talk_ids_fq(LANG_CREATE_DIR, LANG_FAILED);
        splashf(HZ, (unsigned char *)"%s %s", str(LANG_CREATE_DIR),
                                              str(LANG_FAILED));
    } else {
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return true;
}

/* Store the current selection in the clipboard */
static bool clipboard_clip(bool copy)
{
    clipboard_selection[0] = 0;
    strlcpy(clipboard_selection, selected_file, sizeof(clipboard_selection));
    clipboard_selection_attr = selected_file_attr;
    clipboard_is_copy = copy;

    return true;
}

static bool clipboard_cut(void)
{
    return clipboard_clip(false);
}

static bool clipboard_copy(void)
{
    return clipboard_clip(true);
}

/* Paste a file to a new directory. Will overwrite always. */
static bool clipboard_pastefile(const char *src, const char *target, bool copy)
{
    int src_fd, target_fd;
    size_t buffersize;
    ssize_t size, bytesread, byteswritten;
    char *buffer;
    bool result = false;

    if (copy) {
        /* See if we can get the plugin buffer for the file copy buffer */
        buffer = (char *) plugin_get_buffer(&buffersize);
        if (buffer == NULL || buffersize < 512) {
            /* Not large enough, try for a disk sector worth of stack
               instead */
            buffersize = 512;
            buffer = (char *) __builtin_alloca(buffersize);
        }

        if (buffer == NULL) {
            return false;
        }

        buffersize &= ~0x1ff;  /* Round buffer size to multiple of sector
                                  size */

        src_fd = open(src, O_RDONLY);

        if (src_fd >= 0) {
            target_fd = creat(target, 0666);

            if (target_fd >= 0) {
                result = true;

                size = filesize(src_fd);

                if (size == -1) {
                    result = false;
                }

                while(size > 0) {
                    bytesread = read(src_fd, buffer, buffersize);

                    if (bytesread == -1) {
                        result = false;
                        break;
                    }

                    size -= bytesread;

                    while(bytesread > 0) {
                        byteswritten = write(target_fd, buffer, bytesread);

                        if (byteswritten < 0) {
                            result = false;
                            size = 0;
                            break;
                        }

                        bytesread -= byteswritten;
                        draw_slider();
                    }
                }

                close(target_fd);

                /* Copy failed. Cleanup. */
                if (!result) {
                    remove(target);
                }
            }

            close(src_fd);
        }
    } else {
        result = rename(src, target) == 0;
#ifdef HAVE_MULTIVOLUME
        if (!result) {
            if (errno == EXDEV) {
                /* Failed because cross volume rename doesn't work. Copy
                   instead */
                result = clipboard_pastefile(src, target, true);

                if (result) {
                    result = remove(src) == 0;
                }
            }
        }
#endif
    }

    return result;
}

/* Paste a directory to a new location. Designed to be called by
   clipboard_paste */
static bool clipboard_pastedirectory(char *src, int srclen, char *target,
                                     int targetlen, bool copy)
{
    DIR *srcdir;
    int srcdirlen = strlen(src);
    int targetdirlen = strlen(target);
    bool result = true;

    if (!file_exists(target)) {
        if (!copy) {
            /* Just move the directory */
            result = rename(src, target) == 0;

#ifdef HAVE_MULTIVOLUME
            if (!result && errno == EXDEV) {
                /* Try a copy as we're going across devices */
                result = clipboard_pastedirectory(src, srclen, target,
                    targetlen, true);

                /* If it worked, remove the source directory */
                if (result) {
                    remove_dir(src, srclen);
                }
            }
#endif
            return result;
        } else {
            /* Make a directory to copy things to */
            result = mkdir(target) == 0;
        }
    }

    /* Check if something went wrong already */
    if (!result) {
        return result;
    }

    srcdir = opendir(src);
    if (!srcdir) {
        return false;
    }

    /* This loop will exit as soon as there's a problem */
    while(result)
    {
        struct dirent* entry;
        /* walk through the directory content */
        entry = readdir(srcdir);
        if (!entry)
            break;

        /* append name to current directory */
        snprintf(src+srcdirlen, srclen-srcdirlen, "/%s", entry->d_name);
        snprintf(target+targetdirlen, targetlen-targetdirlen, "/%s",
            entry->d_name);

        DEBUGF("Copy %s to %s\n", src, target);

        if (entry->attribute & ATTR_DIRECTORY)
        {   /* copy/move a subdirectory */
            if (!strcmp((char *)entry->d_name, ".") ||
                !strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            result = clipboard_pastedirectory(src, srclen, target, targetlen,
                copy); /* recursion */
        }
        else
        {   /* copy/move a file */
            draw_slider();
            result = clipboard_pastefile(src, target, copy);
        }
    }

    closedir(srcdir);

    if (result) {
        src[srcdirlen] = '\0'; /* terminate to original length */
        target[targetdirlen] = '\0'; /* terminate to original length */
    }

    return result;
}

/* Paste the clipboard to the current directory */
static bool clipboard_paste(void)
{
    char target[MAX_PATH];
    char *cwd, *nameptr;
    bool success;

    static const char *lines[]={ID2P(LANG_REALLY_OVERWRITE)};
    static const struct text_message message={lines, 1};

    /* Get the name of the current directory */
    cwd = getcwd(NULL, 0);

    /* Figure out the name of the selection */
    nameptr = strrchr(clipboard_selection, '/');

    /* Final target is current directory plus name of selection  */
    snprintf(target, sizeof(target), "%s%s", cwd[1] ? cwd : "", nameptr);

    /* If the target existed but they choose not to overwite, exit */
    if (file_exists(target) &&
        (gui_syncyesno_run(&message, NULL, NULL) == YESNO_NO)) {
        return false;
    }

    if (clipboard_is_copy) {
        splash(0, ID2P(LANG_COPYING));
    }
    else
    {
        splash(0, ID2P(LANG_MOVING));
    }

    /* Now figure out what we're doing */
    cpu_boost(true);
    if (clipboard_selection_attr & ATTR_DIRECTORY) {
        /* Recursion. Set up external stack */
        char srcpath[MAX_PATH];
        char targetpath[MAX_PATH];
        if (!strncmp(clipboard_selection, target, strlen(clipboard_selection)))
        {
            /* Do not allow the user to paste a directory into a dir they are
               copying */
            success = 0;
        }
        else
        {
            strlcpy(srcpath, clipboard_selection, sizeof(srcpath));
            strlcpy(targetpath, target, sizeof(targetpath));

            success = clipboard_pastedirectory(srcpath, sizeof(srcpath),
                             target, sizeof(targetpath), clipboard_is_copy);

            if (success && !clipboard_is_copy)
            {
                strlcpy(srcpath, clipboard_selection, sizeof(srcpath));
                remove_dir(srcpath, sizeof(srcpath));
            }
        }
    } else {
        success = clipboard_pastefile(clipboard_selection, target,
            clipboard_is_copy);
    }
    cpu_boost(false);

    /* Did it work? */
    if (success) {
        /* Reset everything */
        clipboard_selection[0] = 0;
        clipboard_selection_attr = 0;
        clipboard_is_copy = false;

        /* Force reload of the current directory */
        onplay_result = ONPLAY_RELOAD_DIR;
    } else {
        cond_talk_ids_fq(LANG_PASTE, LANG_FAILED);
        splashf(HZ, (unsigned char *)"%s %s", str(LANG_PASTE),
                                              str(LANG_FAILED));
    }

    return true;
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
static int ratingitem_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
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

static bool view_cue(void)
{
    struct mp3entry* id3 = audio_current_track();
    if(id3 && id3->cuesheet)
    {
        browse_cuesheet(id3->cuesheet);
    }
    return false;
}
static int view_cue_item_callback(int action,
                                  const struct menu_item_ex *this_item)
{
    (void)this_item;
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

/* CONTEXT_WPS items */
MENUITEM_FUNCTION(browse_id3_item, MENU_FUNC_HOTKEYABLE,
                  ID2P(LANG_MENU_SHOW_ID3_INFO),
                  browse_id3, NULL, NULL, Icon_NOICON);
#ifdef HAVE_PITCHSCREEN
MENUITEM_FUNCTION(pitch_screen_item, MENU_FUNC_HOTKEYABLE,
                  ID2P(LANG_PITCH),
                  gui_syncpitchscreen_run, NULL, NULL, Icon_Audio);
#endif

/* CONTEXT_[TREE|ID3DB] items */
static int clipboard_callback(int action,const struct menu_item_ex *this_item);
MENUITEM_FUNCTION(rename_file_item, 0, ID2P(LANG_RENAME),
                  rename_file, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_cut_item, 0, ID2P(LANG_CUT),
                  clipboard_cut, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_copy_item, 0, ID2P(LANG_COPY),
                  clipboard_copy, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(clipboard_paste_item, 0, ID2P(LANG_PASTE),
                  clipboard_paste, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_file_item, MENU_FUNC_HOTKEYABLE, ID2P(LANG_DELETE),
                  delete_file_dir, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_dir_item, MENU_FUNC_HOTKEYABLE, ID2P(LANG_DELETE_DIR),
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

MENUITEM_FUNCTION(list_viewers_item, MENU_FUNC_HOTKEYABLE,
                  ID2P(LANG_ONPLAY_OPEN_WITH),
                  list_viewers, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(properties_item, MENU_FUNC_USEPARAM, ID2P(LANG_PROPERTIES),
                  onplay_load_plugin, (void *)"properties",
                  clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(add_to_faves_item, MENU_FUNC_USEPARAM, ID2P(LANG_ADD_TO_FAVES),
                  onplay_load_plugin, (void *)"shortcuts_append",
                  clipboard_callback, Icon_NOICON);

#if LCD_DEPTH > 1
static bool set_backdrop(void)
{
    /* load the image */
    if(sb_set_backdrop(SCREEN_MAIN, selected_file)) {
        splash(HZ, str(LANG_BACKDROP_LOADED));
        set_file(selected_file, (char *)global_settings.backdrop_file,
            MAX_FILENAME);
        return true;
    } else {
        splash(HZ, str(LANG_BACKDROP_FAILED));
        return false;
    }
    return true;
}
MENUITEM_FUNCTION(set_backdrop_item, 0, ID2P(LANG_SET_AS_BACKDROP),
                  set_backdrop, NULL, clipboard_callback, Icon_NOICON);
#endif
#ifdef HAVE_RECORDING
static bool set_recdir(void)
{
    strlcpy(global_settings.rec_directory, selected_file, MAX_FILENAME+1);
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_recdir_item, 0, ID2P(LANG_SET_AS_REC_DIR),
                  set_recdir, NULL, clipboard_callback, Icon_Recording);
#endif

static int clipboard_callback(int action,const struct menu_item_ex *this_item)
{
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
#ifdef HAVE_MULTIVOLUME
            if ((selected_file_attr & FAT_ATTR_VOLUME) &&
                (this_item == &rename_file_item ||
                 this_item == &delete_dir_item ||
                 this_item == &clipboard_cut_item) )
                return ACTION_EXIT_MENUITEM;
            /* no rename+delete for volumes */
            if ((selected_file_attr & ATTR_VOLUME) &&
                 (this_item == &delete_file_item ||
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
                return (clipboard_selection[0] != 0) ?
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
                    if (this_item == &delete_dir_item
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

static int onplaymenu_callback(int action,const struct menu_item_ex *this_item);
/* used when onplay() is called in the CONTEXT_WPS context */
MAKE_ONPLAYMENU( wps_onplay_menu, ID2P(LANG_ONPLAY_MENU_TITLE),
           onplaymenu_callback, Icon_Audio,
           &wps_playlist_menu, &cat_playlist_menu,
           &sound_settings, &playback_settings,
#ifdef HAVE_TAGCACHE
           &rating_item,
#endif
           &bookmark_menu, &browse_id3_item, &list_viewers_item,
           &delete_file_item, &view_cue_item,
#ifdef HAVE_PITCHSCREEN
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
           &add_to_faves_item,
         );
static int onplaymenu_callback(int action,const struct menu_item_ex *this_item)
{
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
    if ((selected_file_attr & FAT_ATTR_VOLUME) ||
        (selected_file_attr & ATTR_VOLUME))
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

extern const struct menu_item_ex *selected_menu_item;
extern bool hotkey_settable_menu;

#define HOTKEY_ACTION_MASK 0x0FF /* Mask to apply to get the action (enum)           */
#define HOTKEY_CTX_WPS     0x100 /* Mask to apply to check whether it's for WPS      */
#define HOTKEY_CTX_TREE    0x200 /* Mask to apply to check whether it's for the tree */

/* Any desired hotkey functions go here... */
enum hotkey_action {
    HOTKEY_OFF = 0,
    HOTKEY_VIEW_PLAYLIST = 1,
    HOTKEY_SHOW_TRACK_INFO,
    HOTKEY_PITCHSCREEN,
    HOTKEY_OPEN_WITH,
    HOTKEY_DELETE,
    HOTKEY_INSERT,
};

struct hotkey_assignment {
    int item;               /* Bit or'd hotkey_action and HOTKEY_CTX_x   */
    struct menu_func func;  /* Function to run if this entry is selected */
    int return_code;        /* What to return after the function is run  */
    const struct menu_item_ex *menu_addr; /* Must have non-dynamic text, */
        /* i.e. have the flag MENU_HAS_DESC. E.g. be a MENUITEM_FUNCTION */
        /* For all possibilities see menu.h.                             */
};

#define HOTKEY_FUNC(func, param) {{(void *)func}, param}

/* ... and here.  Order is not important. */
static struct hotkey_assignment hotkey_items[] = {
    { HOTKEY_VIEW_PLAYLIST  | HOTKEY_CTX_WPS,
            HOTKEY_FUNC(NULL, NULL),
            ONPLAY_PLAYLIST,    &view_cur_playlist },
    { HOTKEY_SHOW_TRACK_INFO| HOTKEY_CTX_WPS,
            HOTKEY_FUNC(browse_id3, NULL),
            ONPLAY_RELOAD_DIR,  &browse_id3_item },
#ifdef HAVE_PITCHSCREEN
    { HOTKEY_PITCHSCREEN    | HOTKEY_CTX_WPS,
            HOTKEY_FUNC(gui_syncpitchscreen_run, NULL),
            ONPLAY_RELOAD_DIR,  &pitch_screen_item },
#endif
    { HOTKEY_OPEN_WITH      | HOTKEY_CTX_WPS | HOTKEY_CTX_TREE,
            HOTKEY_FUNC(open_with, NULL),
            ONPLAY_RELOAD_DIR,  &list_viewers_item },
    { HOTKEY_DELETE         | HOTKEY_CTX_WPS | HOTKEY_CTX_TREE,
            HOTKEY_FUNC(delete_item, NULL),
            ONPLAY_RELOAD_DIR,  &delete_file_item },
    { HOTKEY_DELETE         | HOTKEY_CTX_TREE,
            HOTKEY_FUNC(delete_item, NULL),
            ONPLAY_RELOAD_DIR,  &delete_dir_item },
    { HOTKEY_INSERT         | HOTKEY_CTX_TREE,
            HOTKEY_FUNC(playlist_insert_func, (intptr_t*)PLAYLIST_INSERT),
            ONPLAY_START_PLAY,  &i_pl_item },
};

static const int num_hotkey_items = sizeof(hotkey_items) / sizeof(hotkey_items[0]);

/* Return the language ID for the input function */
const char* get_hotkey_desc(int hk_func)
{
    int i;
    for (i = 0; i < num_hotkey_items; i++)
    {
        if ((hotkey_items[i].item & HOTKEY_ACTION_MASK) == hk_func)
            return P2STR(hotkey_items[i].menu_addr->callback_and_desc->desc);
    }
    
    return str(LANG_HOTKEY_NOT_SET);
}

/* Execute the hotkey function, if listed for this screen */
static int execute_hotkey(bool is_wps)
{
    int i;
    struct hotkey_assignment *this_item;
    const int context = is_wps ? HOTKEY_CTX_WPS : HOTKEY_CTX_TREE;
    const int this_hotkey = (is_wps ? global_settings.hotkey_wps :
        global_settings.hotkey_tree);
    
    /* search assignment struct for a match for the hotkey setting */
    for (i = 0; i < num_hotkey_items; i++)
    {
        this_item = &hotkey_items[i];
        if ((this_item->item & context) &&
            ((this_item->item & HOTKEY_ACTION_MASK) == this_hotkey))
        {
            /* run the associated function (with optional param), if any */
            const struct menu_func func = this_item->func;
            if (func.function != NULL)
            {
                if (func.param != NULL)
                    (*func.function_w_param)(func.param);
                else
                    (*func.function)();
            }
            /* return with the associated code */
            return this_item->return_code;
        }
    }
    
    /* no valid hotkey set */
    splash(HZ, ID2P(LANG_HOTKEY_NOT_SET));
    return ONPLAY_RELOAD_DIR;
}

/* Set the hotkey to the current context menu function, if listed */
static void set_hotkey(bool is_wps)
{
    int i;
    struct hotkey_assignment *this_item;
    const int context = is_wps ? HOTKEY_CTX_WPS : HOTKEY_CTX_TREE;
    int *hk_func = is_wps ? &global_settings.hotkey_wps :
                            &global_settings.hotkey_tree;
    int this_hk;
    char *this_desc;
    bool match_found = false;
    
    /* search assignment struct for a function that matches the current menu item */
    for (i = 0; i < num_hotkey_items; i++)
    {
        this_item = &hotkey_items[i];
        if ((this_item->item & context) &&
            (this_item->menu_addr == selected_menu_item))
        {
            this_hk = this_item->item & HOTKEY_ACTION_MASK;
            this_desc = P2STR((selected_menu_item->callback_and_desc)->desc);
            match_found = true;
            break;
        }
    }
    
    /* ignore the hotkey if no match found or no change to setting */
    if (!match_found || (this_hk == *hk_func)) return;
    
    char   line1_buf[100],
           line2_buf[100];
    char  *line1 = line1_buf,
          *line2 = line2_buf;
    char **line1_ptr = &line1,
         **line2_ptr = &line2;
    const struct text_message     message={(const char **)line1_ptr, 1};
    const struct text_message yes_message={(const char **)line2_ptr, 1};

    snprintf(line1, sizeof(line1_buf), str(LANG_SET_HOTKEY_QUESTION), this_desc);
    snprintf(line2, sizeof(line2_buf), str(LANG_HOTKEY_ASSIGNED), this_desc);

    /* confirm the hotkey setting change */
    if(gui_syncyesno_run(&message, &yes_message, NULL)==YESNO_YES)
    {                    
        /* store the hotkey settings */
        *hk_func = this_hk;
        settings_save();
    }
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
    hotkey_settable_menu = true;
#else
    (void)hotkey;
#endif
    if (context == CONTEXT_WPS)
        menu = &wps_onplay_menu;
    else
        menu = &tree_onplay_menu;
    menu_selection = do_menu(menu, NULL, NULL, false);
#ifdef HAVE_HOTKEY
    hotkey_settable_menu = false;
    switch (menu_selection)
    {
        case MENU_SELECTED_HOTKEY:
            set_hotkey(context == CONTEXT_WPS);
            return ONPLAY_RELOAD_DIR;
#else
    switch (menu_selection)
    {
#endif
        case GO_TO_WPS:
            return ONPLAY_START_PLAY;
        case GO_TO_ROOT:
        case GO_TO_MAINMENU:
            return ONPLAY_MAINMENU;
        case GO_TO_PLAYLIST_VIEWER:
            return ONPLAY_PLAYLIST;
        default:
            return onplay_result;
    }
}
