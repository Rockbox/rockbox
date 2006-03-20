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
#ifdef HAVE_LCD_COLOR
#include "backdrop.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif
#include "main_menu.h"
#include "sound_menu.h"
#include "database.h"
#if CONFIG_CODEC == SWCODEC
#include "eq_menu.h"
#endif
#include "playlist_menu.h"

static int context;
static char* selected_file = NULL;
static int selected_file_attr = 0;
static int onplay_result = ONPLAY_OK;
static char clipboard_selection[MAX_PATH];
static int clipboard_selection_attr = 0;
static bool clipboard_is_copy = false;

/* For playlist options */
struct playlist_args {
    int position;
    bool queue;
};

/* ----------------------------------------------------------------------- */
/* Displays the bookmark menu options for the user to decide.  This is an  */
/* interface function.                                                     */
/* ----------------------------------------------------------------------- */
bool bookmark_menu(void)
{
    int i,m;
    bool result;
    struct menu_item items[3];

    i=0;

    if ((audio_status() & AUDIO_STATUS_PLAY))
    {
        items[i].desc = ID2P(LANG_BOOKMARK_MENU_CREATE);
        items[i].function = bookmark_create_menu;
        i++;

        if (bookmark_exist())
        {
            items[i].desc = ID2P(LANG_BOOKMARK_MENU_LIST);
            items[i].function = bookmark_load_menu;
            i++;
        }
    }

    m=menu_init( items, i, NULL, NULL, NULL, NULL );

#ifdef HAVE_LCD_CHARCELLS
    status_set_param(true);
#endif
    result = menu_run(m);
#ifdef HAVE_LCD_CHARCELLS
    status_set_param(false);
#endif
    menu_exit(m);

    settings_save();

    return result;
}

static bool list_viewers(void)
{
    struct menu_item menu[16];
    int m, i, result;
    int ret = 0;

    i=filetype_load_menu(menu,sizeof(menu)/sizeof(*menu));
    if (i)
    {
        m = menu_init( menu, i, NULL, NULL, NULL, NULL );
        result = menu_show(m);
        menu_exit(m);
        if (result >= 0)
            ret = filetype_load_plugin((char *)menu[result].desc,selected_file);
    }
    else
    {
        /* FIX: translation! */
        gui_syncsplash(HZ*2, true, (unsigned char *)"No viewers found");
    }

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

    if (new_playlist)
        playlist_create(NULL, NULL);

    if ((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
        playlist_insert_track(NULL, selected_file, position, queue);
    else if (selected_file_attr & ATTR_DIRECTORY)
    {
        bool recurse = false;

        if (global_settings.recursive_dir_insert != RECURSE_ASK)
            recurse = (bool)global_settings.recursive_dir_insert;
        else
        {
            /* Ask if user wants to recurse directory */
            bool exit = false;
            
            lcd_clear_display();
            lcd_puts_scroll(0, 0, str(LANG_RECURSE_DIRECTORY_QUESTION));
            lcd_puts_scroll(0, 1, (unsigned char *)selected_file);
            
#ifdef HAVE_LCD_BITMAP
            lcd_puts(0, 3, str(LANG_CONFIRM_WITH_PLAY_RECORDER));
            lcd_puts(0, 4, str(LANG_CANCEL_WITH_ANY_RECORDER)); 
#endif
            
            lcd_update();
            
            while (!exit) {
                int btn = button_get(true);
                switch (btn) {
                    case SETTINGS_OK:
                        recurse = true;
                        exit = true;
                        break;

                    default:
                        /* ignore button releases */
                        if (!(btn & BUTTON_REL))
                            exit = true;
                        break;
                }
            }
        }

        playlist_insert_directory(NULL, selected_file, position, queue,
                                  recurse);
    }
    else if ((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_M3U)
        playlist_insert_playlist(NULL, selected_file, position, queue);

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

/* Sub-menu for playlist options */
static bool playlist_options(void)
{
    struct menu_item items[13]; 
    struct playlist_args args[13]; /* increase these 2 if you add entries! */
    int m, i=0, pstart=0, result;
    bool ret = false;

    if ((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_M3U &&
        context == CONTEXT_TREE)
    {
        items[i].desc = ID2P(LANG_VIEW);
        items[i].function = view_playlist;
        i++;
        pstart++;
    }

    if (audio_status() & AUDIO_STATUS_PLAY &&
        context == CONTEXT_WPS)
    {
        items[i].desc = ID2P(LANG_VIEW_DYNAMIC_PLAYLIST);
        items[i].function = playlist_viewer;
        i++;
        pstart++;
        
        items[i].desc = ID2P(LANG_SEARCH_IN_PLAYLIST);
        items[i].function = search_playlist;
        i++;
        pstart++;
        
        items[i].desc = ID2P(LANG_SAVE_DYNAMIC_PLAYLIST);
        items[i].function = save_playlist;
        i++;
        pstart++;
        
        items[i].desc = ID2P(LANG_SHUFFLE_PLAYLIST);
        items[i].function = shuffle_playlist;
        i++;
        pstart++;      
    }
       
    if (context == CONTEXT_TREE || context == CONTEXT_ID3DB)
    {
        if (audio_status() & AUDIO_STATUS_PLAY)
        {
            items[i].desc = ID2P(LANG_INSERT);
            args[i].position = PLAYLIST_INSERT;
            args[i].queue = false;
            i++;

            items[i].desc = ID2P(LANG_INSERT_FIRST);
            args[i].position = PLAYLIST_INSERT_FIRST;
            args[i].queue = false;
            i++;

            items[i].desc = ID2P(LANG_INSERT_LAST);
            args[i].position = PLAYLIST_INSERT_LAST;
            args[i].queue = false;
            i++;

            items[i].desc = ID2P(LANG_INSERT_SHUFFLED);
            args[i].position = PLAYLIST_INSERT_SHUFFLED;
            args[i].queue = false;
            i++;
         
            items[i].desc = ID2P(LANG_QUEUE);
            args[i].position = PLAYLIST_INSERT;
            args[i].queue = true;
            i++;

            items[i].desc = ID2P(LANG_QUEUE_FIRST);
            args[i].position = PLAYLIST_INSERT_FIRST;
            args[i].queue = true;
            i++;

            items[i].desc = ID2P(LANG_QUEUE_LAST);
            args[i].position = PLAYLIST_INSERT_LAST;
            args[i].queue = true;
            i++;

            items[i].desc = ID2P(LANG_QUEUE_SHUFFLED);
            args[i].position = PLAYLIST_INSERT_SHUFFLED;
            args[i].queue = true;
            i++;
        }
        else if (((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_MPA) ||
                 (selected_file_attr & ATTR_DIRECTORY))
        {
            items[i].desc = ID2P(LANG_INSERT);
            args[i].position = PLAYLIST_INSERT;
            args[i].queue = false;
            i++;
        }
    }

    m = menu_init( items, i, NULL, NULL, NULL, NULL );
    result = menu_show(m);
    if (result >= 0 && result < pstart)
        ret = items[result].function();
    else if (result >= pstart)
        ret = add_to_playlist(args[result].position, args[result].queue);
    menu_exit(m);

    return ret;
}


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
            result = remove(dirname);
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
    char *lines[]={
        (char *)str(LANG_REALLY_DELETE),
        selected_file
    };
    char *yes_lines[]={
        (char *)str(LANG_DELETED),
        selected_file
    };

    struct text_message message={lines, 2};
    struct text_message yes_message={yes_lines, 2};
    if(gui_syncyesno_run(&message, &yes_message, NULL)!=YESNO_YES)
        return false;
    int res;
    if (is_dir)
    {
        char pathname[MAX_PATH]; /* space to go deep */
        strncpy(pathname, selected_file, sizeof pathname);
        res = remove_dir(pathname, sizeof(pathname));
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

#ifdef HAVE_LCD_COLOR
static bool set_backdrop(void)
{
    struct bitmap bm;
    int ret;

    /* load the image */
    bm.data=(char*)&main_backdrop[0][0];
    ret = read_bmp_file(selected_file, &bm,
                        sizeof(main_backdrop), FORMAT_NATIVE);

    if ((ret > 0) && (bm.width == LCD_WIDTH) 
                  && (bm.height == LCD_HEIGHT)) {
        lcd_set_backdrop(&main_backdrop[0][0]);
        gui_syncsplash(HZ, true, str(LANG_BACKDROP_LOADED));
        set_file(selected_file, (char *)global_settings.backdrop_file, MAX_FILENAME);
        return true;
    } else {
        lcd_set_backdrop(NULL);
        gui_syncsplash(HZ, true, str(LANG_BACKDROP_FAILED));
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
            sleep(HZ*2);
        }
        else
            onplay_result = ONPLAY_RELOAD_DIR;
    }

    return false;
}

bool create_dir(void)
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

    rc = mkdir(dirname, 0);
    if (rc < 0) {
        gui_syncsplash(HZ, true, (unsigned char *)"%s %s",
                       str(LANG_CREATE_DIR), str(LANG_FAILED));
    } else {
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return true;
}

/* Store the current selection in the clipboard */
static bool clipboard_clip(bool copy)
{
    clipboard_selection[0] = 0;
    strncpy(clipboard_selection, selected_file, MAX_PATH);
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
    int src_fd, target_fd, buffersize, size, bytesread, byteswritten;
    char *buffer;
    bool result = false;

    if (copy) {
        /* See if we can get the plugin buffer for the file copy buffer */
        buffer = (char *) plugin_get_buffer(&buffersize);
        if (buffer == NULL || buffersize < 512) {
            /* Not large enough, try for a disk sector worth of stack instead */
            buffersize = 512;
            buffer = (char *) __builtin_alloca(buffersize);
        }

        if (buffer == NULL) {
            return false;
        }

        buffersize &= ~0x1ff;  /* Round buffer size to multiple of sector size */

        src_fd = open(src, O_RDONLY);

        if (src_fd >= 0) {    
            target_fd = creat(target, O_WRONLY);

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
                /* Failed because cross volume rename doesn't work. Copy instead */
                result = clipboard_pastefile(src, target, true);

                if (result) {
                    result = remove(src);
                }
            }
        }
#endif
    }

    return result;
}

/* Paste a directory to a new location. Designed to be called by clipboard_paste */
static bool clipboard_pastedirectory(char *src, int srclen, char *target, int targetlen, bool copy)
{
    DIR *srcdir;
    int srcdirlen = strlen(src);
    int targetdirlen = strlen(target);
    int fd;
    bool result = true;

    /* Check if the target exists */
    fd = open(target, O_RDONLY);
    close(fd);
    
    if (fd < 0) {
        if (!copy) {
            /* Just move the directory */
            result = rename(src, target) == 0;

#ifdef HAVE_MULTIVOLUME
            if (!result && errno == EXDEV) {
                /* Try a copy as we're going across devices */
                result = clipboard_pastedirectory(src, srclen, target, targetlen, true);

                /* If it worked, remove the source directory */
                if (result) {
                    remove_dir(src, srclen);
                }
            }
#endif
            return result;
        } else {
            /* Make a directory to copy things to */
            result = mkdir(target, 0) == 0;
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
        snprintf(target+targetdirlen, targetlen-targetdirlen, "/%s", entry->d_name);

        DEBUGF("Copy %s to %s\n", src, target);

        if (entry->attribute & ATTR_DIRECTORY) 
        {   /* copy/move a subdirectory */
            if (!strcmp((char *)entry->d_name, ".") ||
                !strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            result = clipboard_pastedirectory(src, srclen, target, targetlen, copy); /* recursion */
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
    int target_fd;

    unsigned char *lines[]={str(LANG_REALLY_OVERWRITE)};
    struct text_message message={(char **)lines, 1};

    /* Get the name of the current directory */
    cwd = getcwd(NULL, 0);
    snprintf(target, sizeof target, "%s", cwd[1] ? cwd : "");

    /* Figure out the name of the selection */
    nameptr = strrchr(clipboard_selection, '/');

    /* Paste the name on to the current directory to give us our final target */
    strcat(target, nameptr);

    /* Check if we're going to overwrite */
    target_fd = open(target, O_RDONLY);
    close(target_fd);

    /* If the target existed but they choose not to overwite, exit */
    if (target_fd >= 0 &&
        (gui_syncyesno_run(&message, NULL, NULL) == YESNO_NO)) {
        return false;
    }

    /* Now figure out what we're doing */
    if (clipboard_selection_attr & ATTR_DIRECTORY) {
        /* Recursion. Set up external stack */
        char srcpath[MAX_PATH];
        char targetpath[MAX_PATH];

        strncpy(srcpath, clipboard_selection, sizeof srcpath);
        strncpy(targetpath, target, sizeof targetpath);

        success = clipboard_pastedirectory(srcpath, sizeof(srcpath), target, sizeof(targetpath), clipboard_is_copy);
    } else {
        success = clipboard_pastefile(clipboard_selection, target, clipboard_is_copy);
    }

    /* Did it work? */
    if (success) {
        /* Reset everything */
        clipboard_selection[0] = 0;
        clipboard_selection_attr = 0;
        clipboard_is_copy = false;

        /* Force reload of the current directory */
        onplay_result = ONPLAY_RELOAD_DIR;
    } else {
        gui_syncsplash(HZ, true, (unsigned char *)"%s %s",
               str(LANG_PASTE), str(LANG_FAILED));        
    }

    return true;
}

static bool exit_to_main;

/* catch MENU_EXIT_MENU within context menu to call the main menu afterwards */
static int onplay_callback(int key, int menu)
{
    (void)menu;

    if (key == MENU_EXIT_MENU
#ifdef MENU_RC_EXIT_MENU
        || key == MENU_RC_EXIT_MENU
#endif
       )
        exit_to_main = true;

    return key;
}

int onplay(char* file, int attr, int from)
{
#if CONFIG_CODEC == SWCODEC
    struct menu_item items[13]; /* increase this if you add entries! */
#else
    struct menu_item items[11];
#endif
    int m, i=0, result;
#ifdef HAVE_LCD_COLOR
    char *suffix;
#endif

    onplay_result = ONPLAY_OK;
    context=from;
    exit_to_main = false;
    selected_file = file;
    selected_file_attr = attr;

    if (context == CONTEXT_WPS)
    {
        items[i].desc = ID2P(LANG_SOUND_SETTINGS);
        items[i].function = sound_menu;
        i++;
    }

    if (context == CONTEXT_WPS ||
        context == CONTEXT_TREE ||
        ((context == CONTEXT_ID3DB) &&
         (attr & TREE_ATTR_MASK) == TREE_ATTR_MPA))
    {
        items[i].desc = ID2P(LANG_PLAYLIST);
        items[i].function = playlist_options;
        i++;
    }

    if (context == CONTEXT_WPS)
    {
        items[i].desc = ID2P(LANG_BOOKMARK_MENU);
        items[i].function = bookmark_menu;
        i++;
    }
        
    if (file)
    {
        if (context == CONTEXT_WPS)
        {
            items[i].desc = ID2P(LANG_MENU_SHOW_ID3_INFO);
            items[i].function = browse_id3;
            i++;
            if(rundb_initialized)
            {
                items[i].desc = ID2P(LANG_MENU_SET_RATING);
                items[i].function = set_rating;
                i++;
            }
        }
        
#ifdef HAVE_MULTIVOLUME
        if (!(attr & ATTR_VOLUME)) /* no rename+delete for volumes */
#endif
        {
            if (context == CONTEXT_TREE)
            {
                items[i].desc = ID2P(LANG_RENAME);
                items[i].function = rename_file;
                i++;

                items[i].desc = ID2P(LANG_CUT);
                items[i].function = clipboard_cut;
                i++;

                items[i].desc = ID2P(LANG_COPY);
                items[i].function = clipboard_copy;
                i++;

                if (clipboard_selection[0] != 0) /* Something in the clipboard? */
                {
                    items[i].desc = ID2P(LANG_PASTE);
                    items[i].function = clipboard_paste;
                    i++;
                }
            }

            if (!(attr & ATTR_DIRECTORY) && context == CONTEXT_TREE)
            {
                items[i].desc = ID2P(LANG_DELETE);
                items[i].function = delete_file;
                i++;

#ifdef HAVE_LCD_COLOR
                suffix = strrchr(file, '.');
                if (suffix)
                {
                    if (strcasecmp(suffix, ".bmp") == 0)
                    {
                        items[i].desc = ID2P(LANG_SET_AS_BACKDROP);
                        items[i].function = set_backdrop;
                        i++;
                    }
                }
#endif
            }
            else
            {
                if (context == CONTEXT_TREE)
                {
                    items[i].desc = ID2P(LANG_DELETE_DIR);
                    items[i].function = delete_dir;
                    i++;
                }
            }
        }

        if (!(attr & ATTR_DIRECTORY))
        {
            items[i].desc = ID2P(LANG_ONPLAY_OPEN_WITH);
            items[i].function = list_viewers;
            i++;
        }
    }
    else
    {
        if (strlen(clipboard_selection) != 0)
        {
            items[i].desc = ID2P(LANG_PASTE);
            items[i].function = clipboard_paste;
            i++;
        }
    }

    if (context == CONTEXT_TREE)
    {
        items[i].desc = ID2P(LANG_CREATE_DIR);
        items[i].function = create_dir;
        i++;
    }

#if CONFIG_CODEC == SWCODEC
    /* Equalizer menu items */
    if (context == CONTEXT_WPS)
    {
        items[i].desc = ID2P(LANG_EQUALIZER_GRAPHICAL);
        items[i].function = eq_menu_graphical;
        i++;
        items[i].desc = ID2P(LANG_EQUALIZER_BROWSE);
        items[i].function = eq_browse_presets;
        i++;
    }
#endif

    /* DIY menu handling, since we want to exit after selection */
    if (i)
    {
        m = menu_init( items, i, onplay_callback, NULL, NULL, NULL );
        result = menu_show(m);
        if (result >= 0)
            items[result].function();
        menu_exit(m);
        
        if (exit_to_main)
            result = main_menu();

#ifdef HAVE_LCD_BITMAP
        if (global_settings.statusbar)
            lcd_setmargins(0, STATUSBAR_HEIGHT);
        else
            lcd_setmargins(0, 0);
#endif
    }

    return onplay_result;
}
