/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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
#include <stdlib.h>
#include <stdbool.h>
#include "string-extra.h"
#include "panic.h"

#include "applimits.h"
#include "dir.h"
#include "file.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "usb.h"
#include "tree.h"
#include "audio.h"
#include "playlist.h"
#include "menu.h"
#include "skin_engine/skin_engine.h"
#include "settings.h"
#include "debug.h"
#include "storage.h"
#include "rolo.h"
#include "icons.h"
#include "lang.h"
#include "screens.h"
#include "keyboard.h"
#include "bookmark.h"
#include "onplay.h"
#include "core_alloc.h"
#include "power.h"
#include "action.h"
#include "talk.h"
#include "filetypes.h"
#include "misc.h"
#include "pathfuncs.h"
#include "filetree.h"
#include "tagtree.h"
#ifdef HAVE_RECORDING
#include "recorder/recording.h"
#endif
#include "rtc.h"
#include "dircache.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#include "yesno.h"
#include "eeprom_settings.h"
#include "playlist_catalog.h"

/* gui api */
#include "list.h"
#include "splash.h"
#include "quickscreen.h"
#include "shortcuts.h"
#include "appevents.h"

#include "root_menu.h"

static struct gui_synclist tree_lists;

/* I put it here because other files doesn't use it yet,
 * but should be elsewhere since it will be used mostly everywhere */
static struct tree_context tc;

char lastfile[MAX_PATH];
static char lastdir[MAX_PATH];
#ifdef HAVE_TAGCACHE
static int lasttable, lastextra;
#endif

static bool reload_dir = false;

static bool start_wps = false;
static int curr_context = false;/* id3db or tree*/

static int dirbrowse(void);
static int ft_play_dirname(char* name);
static int ft_play_filename(char *dir, char *file, int attr);
static void say_filetype(int attr);

struct entry* tree_get_entries(struct tree_context *t)
{
    return core_get_data(t->cache.entries_handle);
}

struct entry* tree_get_entry_at(struct tree_context *t, int index)
{
    if(index < 0 || index >= t->cache.max_entries)
        return NULL; /* no entry */
    struct entry* entries = tree_get_entries(t);
    return &entries[index];
}

static const char* tree_get_filename(int selected_item, void *data,
                                     char *buffer, size_t buffer_len)
{
    struct tree_context * local_tc=(struct tree_context *)data;
    char *name;
    int attr=0;
    bool stripit = false;
#ifdef HAVE_TAGCACHE
    bool id3db = *(local_tc->dirfilter) == SHOW_ID3DB;

    if (id3db)
    {
        return tagtree_get_entry_name(&tc, selected_item, buffer, buffer_len);
    }
    else
#endif
    {
        struct entry *entry = tree_get_entry_at(local_tc, selected_item);
        if (!entry)
            panicf("Invalid tree entry %s", __func__);
        name = entry->name;
        attr = entry->attr;
    }

    if(!(attr & ATTR_DIRECTORY))
    {
        switch(global_settings.show_filename_ext)
        {
            case 0:
                /* show file extension: off */
                stripit = true;
                break;
            case 1:
                /* show file extension: on */
                break;
            case 2:
                /* show file extension: only unknown types */
                stripit = filetype_supported(attr);
                break;
            case 3:
            default:
                /* show file extension: only when viewing all */
                stripit = (*(local_tc->dirfilter) != SHOW_ID3DB) &&
                          (*(local_tc->dirfilter) != SHOW_ALL);
                break;
        }
    }

    if(stripit)
    {
        return(strip_extension(buffer, buffer_len, name));
    }
    return(name);
}

#ifdef HAVE_LCD_COLOR
static int tree_get_filecolor(int selected_item, void * data)
{
    if (*tc.dirfilter == SHOW_ID3DB)
        return -1;
    struct tree_context * local_tc=(struct tree_context *)data;
    struct entry *entry = tree_get_entry_at(local_tc, selected_item);
    if (!entry)
        panicf("Invalid tree entry %s", __func__);

    return filetype_get_color(entry->name, entry->attr);
}
#endif

static enum themable_icons tree_get_fileicon(int selected_item, void * data)
{
    struct tree_context * local_tc=(struct tree_context *)data;
#ifdef HAVE_TAGCACHE
    bool id3db = *(local_tc->dirfilter) == SHOW_ID3DB;
    if (id3db) {
        return tagtree_get_icon(&tc);
    }
    else
#endif
    {
        struct entry *entry = tree_get_entry_at(local_tc, selected_item);
        if (!entry)
            panicf("Invalid tree entry %s", __func__);

        return filetype_get_icon(entry->attr);
    }
}

static int tree_voice_cb(int selected_item, void * data)
{
    struct tree_context * local_tc=(struct tree_context *)data;
    char *name;
    int attr=0;
    int customaction = ONPLAY_NO_CUSTOMACTION;
#ifdef HAVE_TAGCACHE
    bool id3db = *(local_tc->dirfilter) == SHOW_ID3DB;
    char buf[AVERAGE_FILENAME_LENGTH*2];

    if (id3db)
    {
        attr = tagtree_get_attr(local_tc);
        name = tagtree_get_entry_name(local_tc, selected_item, buf, sizeof(buf));
        customaction = tagtree_get_custom_action(local_tc);
    }
    else
#endif
    {
        struct entry *entry = tree_get_entry_at(local_tc, selected_item);
        if (!entry)
            panicf("Invalid tree entry %s", __func__);

        name = entry->name;
        attr = entry->attr;
    }
    bool is_dir = (attr & ATTR_DIRECTORY);
    bool did_clip = false;
    /* First the .talk clip case */
    if(is_dir)
    {
        if(global_settings.talk_dir_clip)
        {
            did_clip = true;
            if (ft_play_dirname(name) <= 0)
                /* failed, not existing */
                did_clip = false;
        }
    } else { /* it's a file */
        if (global_settings.talk_file_clip && (attr & FILE_ATTR_THUMBNAIL))
        {
            did_clip = true;
            if (ft_play_filename(local_tc->currdir, name, attr) <= 0)
                /* failed, not existing */
                did_clip = false;
        }
    }
    bool spell_name = (customaction == ONPLAY_CUSTOMACTION_FIRSTLETTER);
    if(!did_clip)
    {
        /* say the number or spell if required or as a fallback */
        switch (is_dir ? global_settings.talk_dir : global_settings.talk_file)
        {
        case 1: /* as numbers */
            talk_id(is_dir ? VOICE_DIR : VOICE_FILE, false);
            talk_number(selected_item+1        - (is_dir ? 0 : local_tc->dirsindir),
                        true);
            break;
        case 2: /* spelled */
            talk_shutup();
            if(global_settings.talk_filetype)
            {
                if(is_dir)
                    talk_id(VOICE_DIR, true);
            }
            spell_name = true;
            break;
        }
    }

    if(global_settings.talk_filetype && !is_dir
       && *local_tc->dirfilter < NUM_FILTER_MODES)
    {
        say_filetype(attr);
    }

    /* spell name AFTER voicing filetype */
    if (spell_name) {
        bool stripit = false;
        char *ext = NULL;

        /* Don't spell the extension if it's not displayed */
        if (!is_dir) {
            switch(global_settings.show_filename_ext) {
            case 0:
                /* show file extension: off */
                stripit = true;
                break;
            case 1:
                /* show file extension: on */
                stripit = false;
                break;
            case 2:
                /* show file extension: only unknown types */
                stripit = filetype_supported(attr);
                break;
            case 3:
            default:
                /* show file extension: only when viewing all */
                stripit = (*(local_tc->dirfilter) != SHOW_ID3DB) &&
                          (*(local_tc->dirfilter) != SHOW_ALL);
                break;
            }

            if (stripit) {
                ext = strrchr(name, '.');
                if (ext)
                    *ext = 0;
            }
        }
        talk_spell(name, true);

        if (stripit && ext)
            *ext = '.';
    }

    return 0;
}

bool check_rockboxdir(void)
{
    if(!dir_exists(ROCKBOX_DIR))
    {   /* No need to localise this message.
           If .rockbox is missing, it wouldn't work anyway */
        FOR_NB_SCREENS(i)
            screens[i].clear_display();
        splash(HZ*2, "No .rockbox directory");
        FOR_NB_SCREENS(i)
            screens[i].clear_display();
        splash(HZ*2, "Installation incomplete");
        return false;
    }
    return true;
}

/* do this really late in the init sequence */
void tree_init(void)
{
    check_rockboxdir();
    strcpy(tc.currdir, "/");
}


struct tree_context* tree_get_context(void)
{
    return &tc;
}

void tree_lock_cache(struct tree_context *t)
{
    core_pin(t->cache.name_buffer_handle);
    core_pin(t->cache.entries_handle);
}

void tree_unlock_cache(struct tree_context *t)
{
    core_unpin(t->cache.name_buffer_handle);
    core_unpin(t->cache.entries_handle);
}

/*
 * Returns the position of a given file in the current directory
 * returns -1 if not found
 */
static int tree_get_file_position(char * filename)
{
    int i, ret = -1;/* no file match, return undefined */

    tree_lock_cache(&tc);
    struct entry *entries = tree_get_entries(&tc);

    /* use lastfile to determine the selected item (default=0) */
    for (i=0; i < tc.filesindir; i++)
    {
        if (!strcasecmp(entries[i].name, filename))
        {
            ret = i;
            break;
        }
    }
    tree_unlock_cache(&tc);
    return(ret);
}

/*
 * Called when a new dir is loaded (for example when returning from other apps ...)
 * also completely redraws the tree
 */
static int update_dir(void)
{
    struct gui_synclist * const list = &tree_lists;
    int show_path_in_browser = global_settings.show_path_in_browser;
    bool changed = false;

    const char* title = NULL;/* Must clear the title as the list is reused */
    int icon = NOICON;

#ifdef HAVE_TAGCACHE
    bool id3db = *tc.dirfilter == SHOW_ID3DB;
#else
    const bool id3db = false;
#endif

#ifdef HAVE_TAGCACHE
    /* Checks for changes */
    if (id3db) {
        if (tc.currtable != lasttable ||
            tc.currextra != lastextra ||
            reload_dir)
        {
            if (tagtree_load(&tc) < 0)
                return -1;

            lasttable = tc.currtable;
            lastextra = tc.currextra;
            changed = true;
        }
    }
    else
#endif
    {
        tc.sort_dir = global_settings.sort_dir;
        /* if the tc.currdir has been changed, reload it ...*/
        if (reload_dir || strncmp(tc.currdir, lastdir, sizeof(lastdir)))
        {
            if (ft_load(&tc, NULL) < 0)
                return -1;
            strmemccpy(lastdir, tc.currdir, MAX_PATH);
            changed = true;
        }
    }
    /* if selected item is undefined */
    if (tc.selected_item == -1)
    {
        if (!id3db)
            /* use lastfile to determine the selected item */
            tc.selected_item = tree_get_file_position(lastfile);

        /* If the file doesn't exists, select the first one (default) */
        if(tc.selected_item < 0)
            tc.selected_item = 0;
        changed = true;
    }
    if (changed)
    {
        if( !id3db && tc.dirfull )
        {
            splash(HZ, ID2P(LANG_SHOWDIR_BUFFER_FULL));
        }
    }

    gui_synclist_init(list, &tree_get_filename, &tc, false, 1, NULL);

#ifdef HAVE_TAGCACHE
    if (id3db)
    {
        if (show_path_in_browser == SHOW_PATH_FULL
            || show_path_in_browser == SHOW_PATH_CURRENT)
        {
            title = tagtree_get_title(&tc);
            icon = filetype_get_icon(ATTR_DIRECTORY);
        }
    }
    else
#endif
    {
        if (tc.browse && tc.browse->title)
        {
            title = tc.browse->title;
            icon = tc.browse->icon;
            if (icon == NOICON)
                icon = filetype_get_icon(ATTR_DIRECTORY);
            /* display sub directories in the title of plugin browser */
            if (tc.dirlevel > 0 && *tc.dirfilter == SHOW_PLUGINS)
            {
                char *subdir = strrchr(tc.currdir, '/');
                if (subdir)
                    title = subdir + 1; /* step past the separator */
            }
        }
        else
        {
            if (show_path_in_browser == SHOW_PATH_FULL)
            {
                title = tc.currdir;
                icon = filetype_get_icon(ATTR_DIRECTORY);
            }
            else if (show_path_in_browser == SHOW_PATH_CURRENT)
            {
                title = strrchr(tc.currdir, '/');
                if (title != NULL)
                {
                    title++; /* step past the separator */
                    if (*title == '\0')
                    {
                        /* Display "Files" for the root dir */
                        title = str(LANG_DIR_BROWSER);
                    }
                    icon = filetype_get_icon(ATTR_DIRECTORY);
                }
            }
        }
    }

    /* set title and icon, if nothing is set, clear the title
     * with NULL and icon as NOICON as the list is reused */
    gui_synclist_set_title(list, title, icon);

    gui_synclist_set_nb_items(list, tc.filesindir);
    gui_synclist_set_icon_callback(list,
                            global_settings.show_icons?tree_get_fileicon:NULL);
    gui_synclist_set_voice_callback(list, &tree_voice_cb);
#ifdef HAVE_LCD_COLOR
    gui_synclist_set_color_callback(list, &tree_get_filecolor);
#endif
    if( tc.selected_item >= tc.filesindir)
        tc.selected_item=tc.filesindir-1;

    gui_synclist_select_item(list, tc.selected_item);
    gui_synclist_draw(list);
    gui_synclist_speak_item(list);
    return tc.filesindir;
}

/* load tracks from specified directory to resume play */
void resume_directory(const char *dir)
{
    int dirfilter = *tc.dirfilter;
    int ret;
#ifdef HAVE_TAGCACHE
    bool id3db = *tc.dirfilter == SHOW_ID3DB;
#else
    const bool id3db = false;
#endif
    /* make sure the dirfilter is sane. The only time it should be possible
     * thats its not is when resume playlist is called from a plugin
     */
    if (!id3db)
        *tc.dirfilter = global_settings.dirfilter;
    ret = ft_load(&tc, dir);
    *tc.dirfilter = dirfilter;
    if (ret < 0)
        return;
    lastdir[0] = 0;

    ft_build_playlist(&tc, 0);

#ifdef HAVE_TAGCACHE
    if (id3db)
        tagtree_load(&tc);
#endif
}

/* Returns the current working directory and also writes cwd to buf if
   non-NULL.  In case of error, returns NULL. */
char *getcwd(char *buf, getcwd_size_t size)
{
    if (!buf)
        return tc.currdir;
    else if (size)
    {
        if (strmemccpy(buf, tc.currdir, size) != NULL)
            return buf;
    }
    /* size == 0, or truncation in strmemccpy */
    return NULL;
}

/* Force a reload of the directory next time directory browser is called */
void reload_directory(void)
{
    reload_dir = true;
}

char* get_current_file(char* buffer, size_t buffer_len)
{
#ifdef HAVE_TAGCACHE
    /* in ID3DB mode it is a bad idea to call this function */
    /* (only happens with `follow playlist') */
    if( *tc.dirfilter == SHOW_ID3DB )
        return NULL;
#endif

    struct entry *entry = tree_get_entry_at(&tc, tc.selected_item);
    if (entry && getcwd(buffer, buffer_len))
    {
        if (!tc.dirlength)
            return buffer;

        size_t usedlen = strlen(buffer);

        if (usedlen + 2 < buffer_len) /* ensure enough room for '/' + '\0' */
        {
            if (buffer[usedlen-1] != '/')
            {
                buffer[usedlen] = '/';
                /* strmemccpy will zero terminate if we run out of space after */
                usedlen++;
            }
            buffer_len -= usedlen;
            if (strmemccpy(buffer + usedlen, entry->name, buffer_len) != NULL)
                return buffer;
        }
    }
    return NULL;
}

/* Allow apps to change our dirfilter directly (required for sub browsers)
   if they're suddenly going to become a file browser for example */
void set_dirfilter(int l_dirfilter)
{
    *tc.dirfilter = l_dirfilter;
}

/* Selects a path + file and update tree context properly */
static void set_current_file_ex(const char *path, const char *filename)
{
    int i;

#ifdef HAVE_TAGCACHE
    /* in ID3DB mode it is a bad idea to call this function */
    /* (only happens with `follow playlist') */
    if( *tc.dirfilter == SHOW_ID3DB )
        return;
#endif

    if (!filename) /* path and filename supplied combined */
    {
        /* separate directory from filename */
        /* gets the directory's name and put it into tc.currdir */
        filename = strrchr(path+1,'/');
        size_t endpos = filename - path;
        if (filename && endpos < MAX_PATH - 1)
        {
            strmemccpy(tc.currdir, path, endpos + 1);
            filename++;
        }
        else
        {
            strcpy(tc.currdir, "/");
            filename = path+1;
        }
    }
    else /* path and filename came in separate ensure an ending '/' */
    {
        char *end_p = strmemccpy(tc.currdir, path, MAX_PATH);
        size_t endpos = end_p - tc.currdir;
        if (endpos < MAX_PATH)
        {
            if (tc.currdir[endpos - 2] != '/')
            {
                tc.currdir[endpos - 1] = '/';
                tc.currdir[endpos] = '\0';
            }
        }
    }
    strmemccpy(lastfile, filename, MAX_PATH);


    /* If we changed dir we must recalculate the dirlevel
       and adjust the selected history properly */
    if (strncmp(tc.currdir,lastdir,sizeof(lastdir)))
    {
        tc.dirlevel =  0;
        tc.selected_item_history[tc.dirlevel] = -1;

        /* use '/' to calculate dirlevel */
        for (i = 1; path[i] != '\0'; i++)
        {
            if (path[i] == '/')
            {
                tc.dirlevel++;
                tc.selected_item_history[tc.dirlevel] = -1;
            }
        }
    }
    if (ft_load(&tc, NULL) >= 0)
    {
        tc.selected_item = tree_get_file_position(lastfile);
        if (!tc.is_browsing && tc.out_of_tree == 0)
        {
            /* the browser is closed */
            /* don't allow the previous items to overwrite what we just loaded */
            tc.out_of_tree = tc.selected_item + 1;
        }
    }
}

/* Selects a file and update tree context properly */
void set_current_file(const char *path)
{
    set_current_file_ex(path, NULL);
}


/* main loop, handles key events */
static int dirbrowse(void)
{
    int numentries=0;
    char buf[MAX_PATH];
    int button;
    int oldbutton;
    bool reload_root = false;
    int lastfilter = *tc.dirfilter;
    bool lastsortcase = global_settings.sort_case;
    bool exit_func = false;

    char* currdir = tc.currdir; /* just a shortcut */
#ifdef HAVE_TAGCACHE
    bool id3db = *tc.dirfilter == SHOW_ID3DB;

    if (id3db)
        curr_context=CONTEXT_ID3DB;
    else
#endif
        curr_context=CONTEXT_TREE;
    if (tc.selected_item < 0)
        tc.selected_item = 0;
#ifdef HAVE_TAGCACHE
    lasttable = -1;
    lastextra = -1;
#endif

    start_wps = false;
    numentries = update_dir();
    reload_dir = false;
    if (numentries == -1)
        return GO_TO_PREVIOUS;  /* currdir is not a directory */

    if (*tc.dirfilter > NUM_FILTER_MODES && numentries==0)
    {
        splash(HZ*2, ID2P(LANG_NO_FILES));
        return GO_TO_PREVIOUS;  /* No files found for rockbox_browse() */
    }

    while(tc.browse && tc.is_browsing) {
        bool restore = false;
        if (tc.dirlevel < 0)
            tc.dirlevel = 0; /* shouldnt be needed.. this code needs work! */

        keyclick_set_callback(gui_synclist_keyclick_callback, &tree_lists);
        button = get_action(CONTEXT_TREE|ALLOW_SOFTLOCK,
                            list_do_action_timeout(&tree_lists, HZ/2));
        oldbutton = button;
        gui_synclist_do_button(&tree_lists, &button);
        tc.selected_item = gui_synclist_get_sel_pos(&tree_lists);
        int customaction = ONPLAY_NO_CUSTOMACTION;
        bool do_restore_display = true;
        #ifdef HAVE_TAGCACHE
            if (id3db && (button == ACTION_STD_OK || button == ACTION_STD_CONTEXT))
            {
                customaction = tagtree_get_custom_action(&tc);
                if (customaction == ONPLAY_CUSTOMACTION_SHUFFLE_SONGS)
                {
                    /* The code to insert shuffled is on the context branch of the switch so we always go here */
                    button = ACTION_STD_CONTEXT;
                    do_restore_display = false;
                }
            }
        #endif
        switch ( button ) {
            case ACTION_STD_OK:
                /* nothing to do if no files to display */
                if ( numentries == 0 )
                    break;
                if (tc.browse->flags & BROWSE_SELECTONLY)
                {
                    struct entry *entry = tree_get_entry_at(&tc, tc.selected_item);
                    if (!entry)
                        panicf("Invalid tree entry %s", __func__);

                    short attr = entry->attr;
                    if(!(attr & ATTR_DIRECTORY))
                    {
                        tc.browse->flags |= BROWSE_SELECTED;
                        get_current_file(tc.browse->buf, tc.browse->bufsize);
                        return GO_TO_PREVIOUS;
                    }
                }
#ifdef HAVE_TAGCACHE
                switch (id3db ? tagtree_enter(&tc, true) : ft_enter(&tc))
#else
                switch (ft_enter(&tc))
#endif
                {
                    case GO_TO_FILEBROWSER: reload_dir = true; break;
                    case GO_TO_PLUGIN:
                        return GO_TO_PLUGIN;
                    case GO_TO_WPS:
                        return GO_TO_WPS;
#if CONFIG_TUNER
                    case GO_TO_FM:
                        return GO_TO_FM;
#endif
                    case GO_TO_ROOT: exit_func = true; break;
                    default:
                        break;
                }
                restore = do_restore_display;
                break;

            case ACTION_STD_CANCEL:
                if (*tc.dirfilter > NUM_FILTER_MODES && tc.dirlevel < 1) {
                    exit_func = true;
                    break;
                }
                if ((*tc.dirfilter == SHOW_ID3DB && tc.dirlevel == 0) ||
                    ((*tc.dirfilter != SHOW_ID3DB && !strcmp(currdir,"/"))))
                {
                    if (oldbutton == ACTION_TREE_PGLEFT)
                        break;
                    else
                        return GO_TO_ROOT;
                }

#ifdef HAVE_TAGCACHE
                if (id3db)
                    tagtree_exit(&tc, true);
                else
#endif
                    if (ft_exit(&tc) == 3)
                        exit_func = true;

                restore = do_restore_display;
                break;

            case ACTION_TREE_STOP:
                if (list_stop_handler())
                    restore = do_restore_display;
                break;

            case ACTION_STD_MENU:
                return GO_TO_ROOT;
                break;

#ifdef HAVE_RECORDING
            case ACTION_STD_REC:
                return GO_TO_RECSCREEN;
#endif

            case ACTION_TREE_WPS:
                return GO_TO_PREVIOUS_MUSIC;
                break;
#ifdef HAVE_QUICKSCREEN
            case ACTION_STD_QUICKSCREEN:
            {
                bool enter_shortcuts_menu = global_settings.shortcuts_replaces_qs;
                if (enter_shortcuts_menu && *tc.dirfilter >= NUM_FILTER_MODES)
                    break;
                else if (!enter_shortcuts_menu)
                {
                    int ret = quick_screen_quick(button);
                    if (ret == QUICKSCREEN_IN_USB)
                        reload_dir = true;
                    else if (ret == QUICKSCREEN_GOTO_SHORTCUTS_MENU)
                        enter_shortcuts_menu = true;
                }

                if (enter_shortcuts_menu && *tc.dirfilter < NUM_FILTER_MODES)
                {
                    int last_screen = global_status.last_screen;
                    global_status.last_screen = GO_TO_SHORTCUTMENU;
                    int shortcut_ret = do_shortcut_menu(NULL);
                    if (shortcut_ret == GO_TO_PREVIOUS)
                        global_status.last_screen = last_screen;
                    else
                        return shortcut_ret;
                }
                else if (enter_shortcuts_menu) /* currently disabled */
                {
                    /* QuickScreen defers skin updates, popping its activity, when
                       switching to Shortcuts Menu, so make up for that here:   */
                    FOR_NB_SCREENS(i)
                        skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_ALL);
                }

                restore = do_restore_display;
                break;
            }
#endif

#ifdef HAVE_HOTKEY
            case ACTION_TREE_HOTKEY:
                if (!global_settings.hotkey_tree)
                    break;
                /* fall through */
#endif
            case ACTION_STD_CONTEXT:
            {
                bool hotkey = button == ACTION_TREE_HOTKEY;
                int onplay_result;
                int attr = 0;

                if (tc.browse->flags & BROWSE_NO_CONTEXT_MENU)
                    break;

                if(!numentries)
                    onplay_result = onplay(NULL, 0, curr_context, hotkey, customaction);
                else {
#ifdef HAVE_TAGCACHE
                    if (id3db)
                    {
                        if (tagtree_get_attr(&tc) == FILE_ATTR_AUDIO)
                        {
                            attr = FILE_ATTR_AUDIO;
                            tagtree_get_filename(&tc, buf, sizeof(buf));
                        }
                        else
                        {
                            attr = ATTR_DIRECTORY;
                            tagtree_get_entry_name(&tc, tc.selected_item,
                                                   buf, sizeof(buf));
                            fix_path_part(buf, 0, sizeof(buf));
                        }
                    }
                    else
#endif
                    {
                        struct entry *entry = tree_get_entry_at(&tc, tc.selected_item);
                        if (!entry)
                            panicf("Invalid tree entry %s", __func__);

                        attr = entry->attr;

                        ft_assemble_path(buf, sizeof(buf), currdir, entry->name);

                    }
                    onplay_result = onplay(buf, attr, curr_context, hotkey, customaction);
                }
                switch (onplay_result)
                {
                    case ONPLAY_MAINMENU:
                        return GO_TO_ROOT;
                        break;

                    case ONPLAY_OK:
                        restore = do_restore_display;
                        break;

                    case ONPLAY_RELOAD_DIR:
                        reload_dir = true;
                        break;

                    case ONPLAY_START_PLAY:
                        return GO_TO_WPS;
                        break;

                    case ONPLAY_PLUGIN:
                        return GO_TO_PLUGIN;
                        break;
                }
                break;
            }

#ifdef HAVE_HOTSWAP
            case SYS_FS_CHANGED:
#ifdef HAVE_TAGCACHE
                if (!id3db)
#endif
                    reload_dir = true;
                /* The 'dir no longer valid' situation will be caught later
                 * by checking the showdir() result. */
                break;
#endif

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    if(*tc.dirfilter > NUM_FILTER_MODES)
                        /* leave sub-browsers after usb, doing otherwise
                           might be confusing to the user */
                        exit_func = true;
                    else
                        reload_dir = true;
                }
                break;
        }
        if (start_wps)
            return GO_TO_WPS;
        if (button && !IS_SYSEVENT(button))
        {
            storage_spin();
        }


    check_rescan:
        /* do we need to rescan dir? */
        if (reload_dir || reload_root ||
            lastfilter != *tc.dirfilter ||
            lastsortcase != global_settings.sort_case)
        {
            if (reload_root) {
                strcpy(currdir, "/");
                tc.dirlevel = 0;
#ifdef HAVE_TAGCACHE
                tc.currtable = 0;
                tc.currextra = 0;
                lasttable = -1;
                lastextra = -1;
#endif
                reload_root = false;
            }

            if (!reload_dir)
            {
                gui_synclist_select_item(&tree_lists, 0);
                gui_synclist_draw(&tree_lists);
                tc.selected_item = 0;
                lastdir[0] = 0;
            }

            lastfilter = *tc.dirfilter;
            lastsortcase = global_settings.sort_case;
            restore = do_restore_display;
        }

        if (exit_func)
            return GO_TO_PREVIOUS;

        if (restore || reload_dir) {
            /* restore display */
            numentries = update_dir();
            reload_dir = false;
            if (currdir[1] && (numentries < 0))
            {   /* not in root and reload failed */
                reload_root = true; /* try root */
                goto check_rescan;
            }
        }
    }
    return GO_TO_ROOT;
}

int create_playlist(void)
{
    bool ret;
#if 0 /* handled in catalog_add_to_a_playlist() */
    char filename[MAX_PATH + 16]; /* add enough space for extension */
    const char *playlist_dir = catalog_get_directory();
    if (tc.currdir[1] && strcmp(tc.currdir, playlist_dir) != 0)
        snprintf(filename, sizeof filename, "%s.m3u8", tc.currdir);
    else
        snprintf(filename, sizeof filename, "%s/all.m3u8", playlist_dir);

    if (kbd_input(filename, MAX_PATH, NULL))
        return 0;
    splashf(0, "%s %s", str(LANG_CREATING), filename);
#endif

    trigger_cpu_boost();
    ret = catalog_add_to_a_playlist(tc.currdir, ATTR_DIRECTORY, true, NULL, NULL);
    cancel_cpu_boost();

    return (ret) ? 1 : 0;
}

#define NUM_TC_BACKUP   3
static struct tree_context backups[NUM_TC_BACKUP];
/* do not make backup if it is not recursive call */
static int backup_count = -1;
int rockbox_browse(struct browse_context *browse)
{
    tc.is_browsing = (browse != NULL);
    int ret_val = 0;
    int dirfilter = tc.is_browsing ? browse->dirfilter : SHOW_ALL;

    if (backup_count >= NUM_TC_BACKUP)
        return GO_TO_PREVIOUS;
    if (backup_count >= 0)
        backups[backup_count] = tc;
    backup_count++;

    tc.dirfilter = &dirfilter;
    tc.sort_dir = global_settings.sort_dir;

    reload_dir = true;

    if (tc.out_of_tree > 0)
    {
        /* an item has already been loaded out_of_tree holds the selected index
         * what happens with the item is dependent on the browse context */
        tc.selected_item = tc.out_of_tree - 1;
        tc.out_of_tree = 0;
        ret_val = ft_enter(&tc);
    }
    else
    {
        if (*tc.dirfilter >= NUM_FILTER_MODES)
        {
            int last_context;
            /* don't reset if its the same browse already loaded */
            if (tc.browse != browse ||
                !(tc.currdir[1] && strstr(tc.currdir, browse->root) != NULL))
            {
                tc.browse = browse;
                tc.selected_item = 0;
                tc.dirlevel = 0;

                strmemccpy(tc.currdir, browse->root, sizeof(tc.currdir));
            }

            start_wps = false;
            last_context = curr_context;

            if (browse->selected)
            {
                set_current_file_ex(browse->root, browse->selected);
                /* set_current_file changes dirlevel, change it back */
                tc.dirlevel = 0;
            }

            ret_val = dirbrowse();
            curr_context = last_context;
        }
        else
        {
            if (dirfilter != SHOW_ID3DB && (browse->flags & BROWSE_DIRFILTER) == 0)
                tc.dirfilter = &global_settings.dirfilter;
            tc.browse = browse;
            set_current_file(browse->root);
            if (browse->flags&BROWSE_RUNFILE)
                ret_val = ft_enter(&tc);
            else
                ret_val = dirbrowse();
        }
    }

    tc.is_browsing = false;

    backup_count--;
    if (backup_count >= 0)
        tc = backups[backup_count];

    return ret_val;
}

static int move_callback(int handle, void* current, void* new)
{
    struct tree_cache* cache = &tc.cache;
    ptrdiff_t diff = new - current;
    /* FIX_PTR makes sure to not accidentally update static allocations */
#define FIX_PTR(x) \
    { if ((void*)x >= current && (void*)x < (current+cache->name_buffer_size)) x+= diff; }

    if (handle == cache->name_buffer_handle)
    {   /* update entry structs, *even if they are struct tagentry */
        struct entry *this = core_get_data(cache->entries_handle);
        struct entry *last = this + cache->max_entries;
        for(; this < last; this++)
            FIX_PTR(this->name);
    }
    /* nothing to do if entries moved */
    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

void tree_mem_init(void)
{
    /* initialize tree context struct */
    struct tree_cache* cache = &tc.cache;
    memset(&tc, 0, sizeof(tc));
    tc.dirfilter = &global_settings.dirfilter;
    tc.sort_dir = global_settings.sort_dir;

    cache->name_buffer_size = AVERAGE_FILENAME_LENGTH *
        global_settings.max_files_in_dir;
    cache->name_buffer_handle = core_alloc_ex(cache->name_buffer_size, &ops);

    cache->max_entries = global_settings.max_files_in_dir;
    cache->entries_handle =
            core_alloc_ex(cache->max_entries*(sizeof(struct entry)), &ops);
}

bool bookmark_play(char *resume_file, int index, unsigned long elapsed,
                   unsigned long offset, int seed, char *filename)
{
    int i;
    char* suffix = strrchr(resume_file, '.');
    bool started = false;

    if (suffix != NULL &&
        (!strcasecmp(suffix, ".m3u") || !strcasecmp(suffix, ".m3u8")))
    {
        /* Playlist playback */
        char* slash;
        /* check that the file exists */
        if(!file_exists(resume_file))
            return false;

        slash = strrchr(resume_file,'/');
        if (slash)
        {
            char* cp;
            *slash=0;

            cp=resume_file;
            if (!cp[0])
                cp="/";

            if (playlist_create(cp, slash+1) != -1)
            {
                if (global_settings.playlist_shuffle)
                    playlist_shuffle(seed, -1);

                playlist_start(index, elapsed, offset);
                started = true;
            }
            *slash='/';
        }
    }
    else
    {
        /* Directory playback */
        lastdir[0]='\0';
        if (playlist_create(resume_file, NULL) != -1)
        {
            char filename_buf[MAX_PATH + 1];
            const char* peek_filename;
            resume_directory(resume_file);
            if (global_settings.playlist_shuffle)
                playlist_shuffle(seed, -1);

            /* Check if the file is at the same spot in the directory,
               else search for it */
            peek_filename = playlist_peek(index, filename_buf,
                sizeof(filename_buf));

            if (peek_filename == NULL)
            {
                /* playlist has shrunk, search from the top */
                index = 0;
                peek_filename = playlist_peek(index, filename_buf,
                    sizeof(filename_buf));
                if (peek_filename == NULL)
                    return false;
            }

            if (strcmp(strrchr(peek_filename, '/') + 1, filename))
            {
                for ( i=0; i < playlist_amount(); i++ )
                {
                    peek_filename = playlist_peek(i, filename_buf,
                        sizeof(filename_buf));

                    if (peek_filename == NULL)
                        return false;

                    if (!strcmp(strrchr(peek_filename, '/') + 1, filename))
                        break;
                }
                if (i < playlist_amount())
                    index = i;
                else
                    return false;
            }

            playlist_start(index, elapsed, offset);
            started = true;
        }
    }

    if (started)
        start_wps = true;
    return started;
}

static void say_filetype(int attr)
{
    talk_id(tree_get_filetype_voiceclip(attr), true);
}

static int ft_play_dirname(char* name)
{
#ifdef HAVE_MULTIVOLUME
    int vol = path_get_volume_id(name);
    if (talk_volume_id(vol))
        return 1;
#endif

    return talk_file(tc.currdir, name, dir_thumbnail_name, NULL,
                     global_settings.talk_filetype ?
                     TALK_IDARRAY(VOICE_DIR) : NULL,
                     false);
}

static int ft_play_filename(char *dir, char *file, int attr)
{
    if (strlen(file) >= strlen(file_thumbnail_ext)
        && strcasecmp(&file[strlen(file) - strlen(file_thumbnail_ext)],
                      file_thumbnail_ext))
        /* file has no .talk extension */
        return talk_file(dir, NULL, file, file_thumbnail_ext,
                         TALK_IDARRAY(tree_get_filetype_voiceclip(attr)), false);

    /* it already is a .talk file, play this directly, but prefix it. */
    return talk_file(dir, NULL, file, NULL,
                     TALK_IDARRAY(LANG_VOICE_DIR_HOVER), false);
}

/* These two functions are called by the USB and shutdown handlers */
void tree_flush(void)
{
     tc.is_browsing = false;/* clear browse to prevent reentry to a possibly missing file */
#ifdef HAVE_TAGCACHE
    tagcache_shutdown();
#endif

#ifdef HAVE_TC_RAMCACHE
    tagcache_unload_ramcache();
#endif

#ifdef HAVE_DIRCACHE
    int old_val = global_status.dircache_size;
#ifdef HAVE_EEPROM_SETTINGS
    bool savecache = false;
#endif

    if (global_settings.dircache)
    {
        dircache_suspend();

        struct dircache_info info;
        dircache_get_info(&info);

        global_status.dircache_size = info.last_size;
    #ifdef HAVE_EEPROM_SETTINGS
        savecache = firmware_settings.initialized;
    #endif
    }
    else
    {
        global_status.dircache_size = 0;
    }

    if (old_val != global_status.dircache_size)
        status_save();

    #ifdef HAVE_EEPROM_SETTINGS
        if (savecache)
            dircache_save();
    #endif
#endif /* HAVE_DIRCACHE */
}

void tree_restore(void)
{
#ifdef HAVE_EEPROM_SETTINGS
    firmware_settings.disk_clean = false;
#endif

#ifdef HAVE_TC_RAMCACHE
    tagcache_remove_statefile();
#endif

#ifdef HAVE_DIRCACHE
    if (global_settings.dircache && dircache_resume() > 0)
    {
        /* Print "Scanning disk..." to the display. */
        splash(0, str(LANG_SCANNING_DISK));
        dircache_wait();
    }
#endif

#ifdef HAVE_TAGCACHE
    tagcache_start_scan();
#endif
}
