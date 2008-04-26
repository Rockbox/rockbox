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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "sprintf.h"
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
#include "id3.h"
#include "screens.h"
#include "tree.h"
#include "buffer.h"
#include "settings.h"
#include "statusbar.h"
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
#if CONFIG_CODEC == SWCODEC
#include "menus/eq_menu.h"
#endif
#include "playlist_menu.h"
#include "playlist_catalog.h"
#ifdef HAVE_TAGCACHE
#include "tagtree.h"
#endif
#include "cuesheet.h"
#include "backdrop.h"

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
            
#ifdef HAVE_LCD_BITMAP            
static void draw_slider(void);
#else
#define draw_slider()
#endif
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
MAKE_MENU(bookmark_menu, ID2P(LANG_BOOKMARK_MENU), bookmark_menu_callback,
          Icon_Bookmark, &bookmark_create_menu_item, &bookmark_load_menu_item);
static int bookmark_menu_callback(int action,
                                  const struct menu_item_ex *this_item)
{
    (void)this_item;
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

static bool list_viewers(void)
{
    int ret = filetype_list_viewers(selected_file);
    if (ret == PLUGIN_USB_CONNECTED)
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}

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

static bool add_to_playlist(int position, bool queue)
{
    bool new_playlist = !(audio_status() & AUDIO_STATUS_PLAY);
    const char *lines[] = {
        ID2P(LANG_RECURSE_DIRECTORY_QUESTION),
        selected_file
    };
    const struct text_message message={lines, 2};

    gui_syncsplash(0, ID2P(LANG_WAIT));
    
    if (new_playlist)
        playlist_create(NULL, NULL);

    /* always set seed before inserting shuffled */
    if (position == PLAYLIST_INSERT_SHUFFLED)
        srand(current_tick);

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
        gui_syncstatusbar_draw(&statusbars, false);
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

static bool cat_add_to_a_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file, selected_file_attr,
        false);
}

static bool cat_add_to_a_new_playlist(void)
{
    return catalog_add_to_a_playlist(selected_file, selected_file_attr, true);
}


static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item);
MENUITEM_FUNCTION(cat_view_lists, 0, ID2P(LANG_CATALOG_VIEW),
                  catalog_view_playlists, 0, cat_playlist_callback,
                  Icon_Playlist);
MENUITEM_FUNCTION(cat_add_to_list, 0, ID2P(LANG_CATALOG_ADD_TO), 
                  cat_add_to_a_playlist, 0, NULL, Icon_Playlist);
MENUITEM_FUNCTION(cat_add_to_new, 0, ID2P(LANG_CATALOG_ADD_TO_NEW), 
                  cat_add_to_a_new_playlist, 0, NULL, Icon_Playlist);
MAKE_MENU( cat_playlist_menu, ID2P(LANG_CATALOG), cat_playlist_callback,
           Icon_Playlist, &cat_view_lists, 
           &cat_add_to_list, &cat_add_to_new );
        
static int cat_playlist_callback(int action,
                                 const struct menu_item_ex *this_item)
{
    if (((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_AUDIO) &&
          ((selected_file_attr & FILE_ATTR_MASK) != FILE_ATTR_M3U) &&
          ((selected_file_attr & ATTR_DIRECTORY) == 0))
    {
        return ACTION_EXIT_MENUITEM;
    }
    
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (this_item == &cat_view_lists)
            {
                if (context == CONTEXT_WPS)
                    return action;
            }
            else if (selected_file && /* set before calling this menu,
                                         so safe */
                     ((audio_status() & AUDIO_STATUS_PLAY &&
                      context == CONTEXT_WPS) ||
                      context == CONTEXT_TREE))
            {
                return action;
            }
            else
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}


/* CONTEXT_WPS playlist options */
MENUITEM_FUNCTION(playlist_viewer_item, 0, 
                  ID2P(LANG_VIEW_DYNAMIC_PLAYLIST), playlist_viewer,
                  NULL, NULL, Icon_Playlist);
MENUITEM_FUNCTION(search_playlist_item, 0, 
                  ID2P(LANG_SEARCH_IN_PLAYLIST), search_playlist,
                  NULL, NULL, Icon_Playlist);
MENUITEM_FUNCTION(playlist_save_item, 0, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),
                  save_playlist, NULL, NULL, Icon_Playlist);
MENUITEM_FUNCTION(reshuffle_item, 0, ID2P(LANG_SHUFFLE_PLAYLIST),
                  shuffle_playlist, NULL, NULL, Icon_Playlist);
MAKE_ONPLAYMENU( wps_playlist_menu, ID2P(LANG_PLAYLIST), 
                 NULL, Icon_Playlist, 
                 &playlist_viewer_item, &search_playlist_item,
                 &playlist_save_item, &reshuffle_item
               );
               
/* CONTEXT_[TREE|ID3DB] playlist options */
static int playlist_insert_func(void *param)
{
    add_to_playlist((intptr_t)param, false);
    return 0;
}
static int playlist_queue_func(void *param)
{
    add_to_playlist((intptr_t)param, true);
    return 0;
}
static int treeplaylist_wplayback_callback(int action,
                                           const struct menu_item_ex* 
                                                this_item)
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
MENUITEM_FUNCTION(i_pl_item_no_play, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT_LAST,
                  treeplaylist_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_first_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT_FIRST),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT_FIRST,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_last_pl_item, MENU_FUNC_USEPARAM, ID2P(LANG_INSERT_LAST),
                  playlist_insert_func, (intptr_t*)PLAYLIST_INSERT_LAST,
                  treeplaylist_wplayback_callback, Icon_Playlist);
MENUITEM_FUNCTION(i_shuf_pl_item, MENU_FUNC_USEPARAM,
                  ID2P(LANG_INSERT_SHUFFLED), playlist_insert_func,
                  (intptr_t*)PLAYLIST_INSERT_SHUFFLED, treeplaylist_callback,
                  Icon_Playlist);
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
                 &i_pl_item_no_play, &i_pl_item, &i_first_pl_item,
                 &i_last_pl_item, &i_shuf_pl_item,
                 
                 /* queue */
                 &q_pl_item, &q_first_pl_item, &q_last_pl_item,
                 &q_shuf_pl_item,
                 
                 /* replace */
                 &replace_pl_item
               );
static int treeplaylist_callback(int action,
                                 const struct menu_item_ex *this_item)
{
    (void)this_item;
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
                else 
                    return ACTION_EXIT_MENUITEM;
            }
            else if (this_item == &view_playlist_item)
            {
                if ((selected_file_attr & FILE_ATTR_MASK) == FILE_ATTR_M3U &&
                        context == CONTEXT_TREE)
                    return action;
                else 
                    return ACTION_EXIT_MENUITEM;
            }
            else if (this_item == &i_pl_item_no_play)
            {
                if (!(audio_status() & AUDIO_STATUS_PLAY))
                {
                    return action;
                }
                else 
                    return ACTION_EXIT_MENUITEM;
            }
            else if (this_item == &i_shuf_pl_item)
            {
                
                if (audio_status() & AUDIO_STATUS_PLAY)
                {
                    return action;
                }
                else if ((this_item == &i_shuf_pl_item) && 
                        ((selected_file_attr & ATTR_DIRECTORY) ||
                        ((selected_file_attr & FILE_ATTR_MASK) ==
                            FILE_ATTR_M3U)))
                {
                    return action;
                }
                return ACTION_EXIT_MENUITEM;
            }
            break;
    }
    return action;
}

/* helper function to remove a non-empty directory */
static int remove_dir(char* dirname, int len)
{
    int result = 0;
    DIR* dir;
    int dirlen = strlen(dirname);
    int i;

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
        FOR_NB_SCREENS(i){
            screens[i].puts(0,1,dirname);
            screens[i].update();        
        }
        
        /* append name to current directory */
        snprintf(dirname+dirlen, len-dirlen, "/%s", entry->d_name);
        if (entry->attribute & ATTR_DIRECTORY)
        {   /* remove a subdirectory */
            if (!strcmp((char *)entry->d_name, ".") ||
                !strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            /* inform the user which dir we're deleting */
            
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
            gui_syncsplash(HZ, ID2P(LANG_CANCEL));
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
static bool delete_handler(bool is_dir)
{
    const char *lines[]={
        ID2P(LANG_REALLY_DELETE),
        selected_file
    };
    const char *yes_lines[]={
        ID2P(LANG_DELETED),
        selected_file
    };

    struct text_message message={lines, 2};
    struct text_message yes_message={yes_lines, 2};

    if(gui_syncyesno_run(&message, &yes_message, NULL)!=YESNO_YES)
        return false;

    gui_syncsplash(0, str(LANG_DELETING));

    int res;
    if (is_dir)
    {
        char pathname[MAX_PATH]; /* space to go deep */
        cpu_boost(true);
        strncpy(pathname, selected_file, sizeof pathname);
        res = remove_dir(pathname, sizeof(pathname));
        cpu_boost(false);
    }
    else
        res = remove(selected_file);

    if (!res) {
        onplay_result = ONPLAY_RELOAD_DIR;
    }
    return false;
}


static bool delete_file(void)
{
    return delete_handler(false);
}

static bool delete_dir(void)
{
    return delete_handler(true);
}

#if LCD_DEPTH > 1
static bool set_backdrop(void)
{
    /* load the image */
    if(load_main_backdrop(selected_file)) {
        gui_syncsplash(HZ, str(LANG_BACKDROP_LOADED));
        set_file(selected_file, (char *)global_settings.backdrop_file,
            MAX_FILENAME);
        show_main_backdrop();
        return true;
    } else {
        gui_syncsplash(HZ, str(LANG_BACKDROP_FAILED));
        return false;
    }
}
#endif

static bool rename_file(void)
{
    char newname[MAX_PATH];
    char* ptr = strrchr(selected_file, '/') + 1;
    int pathlen = (ptr - selected_file);
    strncpy(newname, selected_file, sizeof newname);
    if (!kbd_input(newname + pathlen, (sizeof newname)-pathlen)) {
        if (!strlen(newname + pathlen) ||
            (rename(selected_file, newname) < 0)) {
            lcd_clear_display();
            lcd_puts(0,0,str(LANG_RENAME));
            lcd_puts(0,1,str(LANG_FAILED));
            lcd_update();
            cond_talk_ids_fq(LANG_RENAME, LANG_FAILED);
            sleep(HZ*2);
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

    snprintf(dirname, sizeof dirname, "%s/",
             cwd[1] ? cwd : "");

    pathlen = strlen(dirname);
    rc = kbd_input(dirname + pathlen, (sizeof dirname)-pathlen);
    if (rc < 0)
        return false;

    rc = mkdir(dirname);
    if (rc < 0) {
        cond_talk_ids_fq(LANG_CREATE_DIR, LANG_FAILED);
        gui_syncsplash(HZ, (unsigned char *)"%s %s",
                       str(LANG_CREATE_DIR), str(LANG_FAILED));
    } else {
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return true;
}

static bool properties(void)
{
    if(PLUGIN_USB_CONNECTED == filetype_load_plugin("properties",
                                                    selected_file))
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}

/* Store the current selection in the clipboard */
static bool clipboard_clip(bool copy)
{
    clipboard_selection[0] = 0;
    strncpy(clipboard_selection, selected_file, sizeof(clipboard_selection));
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

#ifdef HAVE_LCD_BITMAP
static void draw_slider(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        show_busy_slider(&screens[i], 1, LCD_HEIGHT-2*screens[i].char_height,
                         LCD_WIDTH-2, 2*screens[i].char_height-1);
        screens[i].update();
    }
}
#endif

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
            target_fd = creat(target);

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

                        if (byteswritten == -1) {
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
        gui_syncsplash(0, ID2P(LANG_COPYING));
    }
    else
    {
        gui_syncsplash(0, ID2P(LANG_MOVING));
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
            strncpy(srcpath, clipboard_selection, sizeof srcpath);
            strncpy(targetpath, target, sizeof targetpath);
    
            success = clipboard_pastedirectory(srcpath, sizeof(srcpath),
                             target, sizeof(targetpath), clipboard_is_copy);
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
        gui_syncsplash(HZ, (unsigned char *)"%s %s",
               str(LANG_PASTE), str(LANG_FAILED));
    }

    return true;
}

static int onplaymenu_callback(int action,const struct menu_item_ex *this_item);
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
        gui_syncsplash(HZ*2, ID2P(LANG_ID3_NO_INFO));
    return 0;
}
static int ratingitem_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (!selected_file || !global_settings.runtimedb)
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
    if(id3 && cuesheet_is_enabled() && id3->cuesheet_type)
    {
        browse_cuesheet(curr_cue);
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
            if (!selected_file || !cuesheet_is_enabled()
                || !id3 || !id3->cuesheet_type)
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}
MENUITEM_FUNCTION(view_cue_item, 0, ID2P(LANG_BROWSE_CUESHEET),
                  view_cue, NULL, view_cue_item_callback, Icon_NOICON);

/* CONTEXT_WPS items */
MENUITEM_FUNCTION(browse_id3_item, 0, ID2P(LANG_MENU_SHOW_ID3_INFO),
                  browse_id3, NULL, NULL, Icon_NOICON);
#ifdef HAVE_PITCHSCREEN
MENUITEM_FUNCTION(pitch_screen_item, 0, ID2P(LANG_PITCH),
                  pitch_screen, NULL, NULL, Icon_Audio);
#endif
#if CONFIG_CODEC == SWCODEC
MENUITEM_FUNCTION(eq_menu_graphical_item, 0, ID2P(LANG_EQUALIZER_GRAPHICAL),
                  eq_menu_graphical, NULL, NULL, Icon_Audio);
MENUITEM_FUNCTION(eq_browse_presets_item, 0, ID2P(LANG_EQUALIZER_BROWSE),
                  eq_browse_presets, NULL, NULL, Icon_Audio);
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
MENUITEM_FUNCTION(delete_file_item, 0, ID2P(LANG_DELETE),
                  delete_file, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(delete_dir_item, 0, ID2P(LANG_DELETE_DIR),
                  delete_dir, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(properties_item, 0, ID2P(LANG_PROPERTIES),
                  properties, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(create_dir_item, 0, ID2P(LANG_CREATE_DIR),
                  create_dir, NULL, clipboard_callback, Icon_NOICON);
MENUITEM_FUNCTION(list_viewers_item, 0, ID2P(LANG_ONPLAY_OPEN_WITH),
                  list_viewers, NULL, clipboard_callback, Icon_NOICON);
#if LCD_DEPTH > 1
MENUITEM_FUNCTION(set_backdrop_item, 0, ID2P(LANG_SET_AS_BACKDROP),
                  set_backdrop, NULL, clipboard_callback, Icon_NOICON);
#endif
#ifdef HAVE_RECORDING
static bool set_recdir(void)
{
    strncpy(global_settings.rec_directory,
            selected_file, MAX_FILENAME+1);
    settings_save();
    return false;
}
MENUITEM_FUNCTION(set_recdir_item, 0, ID2P(LANG_SET_AS_REC_DIR),
                  set_recdir, NULL, clipboard_callback, Icon_Recording);
#endif
static bool add_to_faves(void)
{
    if(PLUGIN_USB_CONNECTED == filetype_load_plugin("shortcuts_append",
                                                    selected_file))
        onplay_result = ONPLAY_RELOAD_DIR;
    return false;
}
MENUITEM_FUNCTION(add_to_faves_item, 0, ID2P(LANG_ADD_TO_FAVES),
                  add_to_faves, NULL, clipboard_callback, Icon_NOICON);


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
#endif
            if (context == CONTEXT_ID3DB)
                return ACTION_EXIT_MENUITEM;
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
            else if ((this_item == &properties_item) || 
                     (this_item == &rename_file_item) ||
                     (this_item == &clipboard_cut_item) ||
                     (this_item == &clipboard_copy_item) ||
                     (this_item == &add_to_faves_item)
                    )
            {
                /* requires an actual file */                
                return (selected_file) ?  action : ACTION_EXIT_MENUITEM;
            }
#if LCD_DEPTH > 1
            else if (this_item == &set_backdrop_item)
            {
                if (selected_file)
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
                return ACTION_EXIT_MENUITEM;
            }
#endif
            else if ((selected_file_attr & ATTR_DIRECTORY))
            {
                if ((this_item == &delete_dir_item)
                    )
                    return action;
#ifdef HAVE_RECORDING
                else if (this_item == &set_recdir_item)
                    return action;
#endif
            }
            else if (selected_file
#ifdef HAVE_MULTIVOLUME
                     /* no rename+delete for volumes */
                        && !(selected_file_attr & ATTR_VOLUME) 
#endif
                     )
            {
                if ((this_item == &delete_file_item) || 
                    (this_item == &list_viewers_item))
                {
                    return action;
                }
            }
            return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}
/* used when onplay() is called in the CONTEXT_WPS context */

            
MAKE_ONPLAYMENU( wps_onplay_menu, ID2P(LANG_ONPLAY_MENU_TITLE), 
           onplaymenu_callback, Icon_Audio,
           &sound_settings, &wps_playlist_menu, &cat_playlist_menu,
#ifdef HAVE_TAGCACHE
           &rating_item, 
#endif
           &bookmark_menu, &browse_id3_item, &delete_file_item, &view_cue_item,
#ifdef HAVE_PITCHSCREEN
           &pitch_screen_item,
#endif
#if CONFIG_CODEC == SWCODEC
           &eq_menu_graphical_item, &eq_browse_presets_item,
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
    (void)this_item;
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
int onplay(char* file, int attr, int from)
{
    const struct menu_item_ex *menu;
    onplay_result = ONPLAY_OK;
    context = from;
    selected_file = file;
    selected_file_attr = attr;
    if (context == CONTEXT_WPS)
        menu = &wps_onplay_menu;
    else
        menu = &tree_onplay_menu;
    switch (do_menu(menu, NULL, NULL, false))
    {
        case GO_TO_WPS:
            return ONPLAY_START_PLAY;
        case GO_TO_ROOT:
        case GO_TO_MAINMENU:
            return ONPLAY_MAINMENU;
        default:
            return context == CONTEXT_WPS ? ONPLAY_OK : ONPLAY_RELOAD_DIR;
    }
}
