/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C)2003 by Benjamin Metzler
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
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "action.h"
#include "audio.h"
#include "playlist.h"
#include "settings.h"
#include "tree.h"
#include "bookmark.h"
#include "system.h"
#include "icons.h"
#include "menu.h"
#include "lang.h"
#include "talk.h"
#include "misc.h"
#include "splash.h"
#include "yesno.h"
#include "list.h"
#include "plugin.h"
#include "file.h"
#include "pathfuncs.h"

#define MAX_BOOKMARKS 10
#define MAX_BOOKMARK_SIZE  350
#define RECENT_BOOKMARK_FILE ROCKBOX_DIR "/most-recent.bmark"

/* Used to buffer bookmarks while displaying the bookmark list. */
struct bookmark_list
{
    const char* filename;
    size_t buffer_size;
    int start;
    int count;
    int total_count;
    bool show_dont_resume;
    bool reload;
    bool show_playlist_name;
    char* items[];
};

/* flags for optional bookmark tokens */
#define BM_PITCH    0x01
#define BM_SPEED    0x02

/* bookmark values */
static struct {
    int  resume_index;
    unsigned long resume_offset;
    int  resume_seed;
    long resume_time;
    int  repeat_mode;
    bool shuffle;
    /* optional values */
    int  pitch;
    int  speed; 
} bm;

static bool  add_bookmark(const char* bookmark_file_name, const char* bookmark, 
                          bool most_recent);
static char* create_bookmark(void);
static bool  delete_bookmark(const char* bookmark_file_name, int bookmark_id);
static void  say_bookmark(const char* bookmark,
                          int bookmark_id, bool show_playlist_name);
static bool  play_bookmark(const char* bookmark);
static bool  generate_bookmark_file_name(const char *in);
static bool  parse_bookmark(const char *bookmark, const bool get_filenames);
static int   buffer_bookmarks(struct bookmark_list* bookmarks, int first_line);
static const char* get_bookmark_info(int list_index,
                                     void* data,
                                     char *buffer,
                                     size_t buffer_len);
static int   select_bookmark(const char* bookmark_file_name, bool show_dont_resume, char** selected_bookmark);
static bool  write_bookmark(bool create_bookmark_file, const char *bookmark);
static int   get_bookmark_count(const char* bookmark_file_name);

#define TEMP_BUF_SIZE (MAX_PATH + 1)
static char global_temp_buffer[TEMP_BUF_SIZE];
/* File name created by generate_bookmark_file_name */
static char global_bookmark_file_name[MAX_PATH];
static char global_read_buffer[MAX_BOOKMARK_SIZE];
/* Bookmark created by create_bookmark*/
static char global_bookmark[MAX_BOOKMARK_SIZE];
/* Filename from parsed bookmark (can be made local where needed) */
static char global_filename[MAX_PATH];

/* ----------------------------------------------------------------------- */
/* This is an interface function from the context menu.                    */
/* Returns true on successful bookmark creation.                           */
/* ----------------------------------------------------------------------- */
bool bookmark_create_menu(void)
{
    return write_bookmark(true, create_bookmark());
}

/* ----------------------------------------------------------------------- */
/* This function acts as the load interface from the context menu.         */
/* This function determines the bookmark file name and then loads that file*/
/* for the user.  The user can then select or delete previous bookmarks.   */
/* This function returns BOOKMARK_SUCCESS on the selection of a track to   */
/* resume, BOOKMARK_FAIL if the menu is exited without a selection and     */
/* BOOKMARK_USB_CONNECTED if the menu is forced to exit due to a USB       */
/* connection.                                                             */
/* ----------------------------------------------------------------------- */
int bookmark_load_menu(void)
{
    char* bookmark;
    int ret = BOOKMARK_FAIL;

    push_current_activity(ACTIVITY_BOOKMARKSLIST);

    char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
    if (generate_bookmark_file_name(name))
    {
        ret = select_bookmark(global_bookmark_file_name, false, &bookmark);
        if (bookmark != NULL)
        {
            ret = play_bookmark(bookmark) ? BOOKMARK_SUCCESS : BOOKMARK_FAIL;
        }
    }

    pop_current_activity();
    return ret;
}

/* ----------------------------------------------------------------------- */
/* Gives the user a list of the Most Recent Bookmarks.  This is an         */
/* interface function                                                      */
/* Returns true on the successful selection of a recent bookmark.          */
/* ----------------------------------------------------------------------- */
bool bookmark_mrb_load()
{
    char* bookmark;
    bool ret = false;

    push_current_activity(ACTIVITY_BOOKMARKSLIST);
    select_bookmark(RECENT_BOOKMARK_FILE, false, &bookmark);
    if (bookmark != NULL)
    {
        ret = play_bookmark(bookmark);
    }

    pop_current_activity();
    return ret;
}

/* ----------------------------------------------------------------------- */
/* This function handles an autobookmark creation.  This is an interface   */
/* function.                                                               */
/* Returns true on successful bookmark creation.                           */
/* ----------------------------------------------------------------------- */
bool bookmark_autobookmark(bool prompt_ok)
{
    char*  bookmark;
    bool update;

    if (!bookmark_is_bookmarkable_state())
        return false;

    audio_pause();    /* first pause playback */
    update = (global_settings.autoupdatebookmark && bookmark_exists());
    bookmark = create_bookmark();
#if CONFIG_CODEC != SWCODEC
    /* Workaround for inability to speak when paused: all callers will
       just do audio_stop() when we return, so we can do it right
       away. This makes it possible to speak the "Create a Bookmark?"
       prompt and the "Bookmark Created" splash. */
    audio_stop();
#endif

    if (update)
        return write_bookmark(true, bookmark);

    switch (global_settings.autocreatebookmark)
    {
        case BOOKMARK_YES:
            return write_bookmark(true, bookmark);

        case BOOKMARK_NO:
            return false;

        case BOOKMARK_RECENT_ONLY_YES:
            return write_bookmark(false, bookmark);
    }
#ifdef HAVE_LCD_BITMAP
    const char *lines[]={ID2P(LANG_AUTO_BOOKMARK_QUERY)};
    const struct text_message message={lines, 1};
#else
    const char *lines[]={ID2P(LANG_AUTO_BOOKMARK_QUERY),
                            str(LANG_CONFIRM_WITH_BUTTON)};
    const struct text_message message={lines, 2};
#endif

    if(prompt_ok && gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES)
    {
        if (global_settings.autocreatebookmark == BOOKMARK_RECENT_ONLY_ASK)
            return write_bookmark(false, bookmark);
        else
            return write_bookmark(true, bookmark);
    }
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function takes the current current resume information and writes   */
/* that to the beginning of the bookmark file.                             */
/* This file will contain N number of bookmarks in the following format:   */
/* resume_index*resume_offset*resume_seed*resume_first_index*              */
/* resume_file*milliseconds*MP3 Title*                                     */
/* Returns true on successful bookmark write.                              */
/* Returns false if any part of the bookmarking process fails.  It is      */
/* possible that a bookmark is successfully added to the most recent        */
/* bookmark list but fails to be added to the bookmark file or vice versa. */
/* ------------------------------------------------------------------------*/
static bool write_bookmark(bool create_bookmark_file, const char *bookmark)
{
    bool ret=true;

    if (!bookmark)
    {
       ret = false; /* something didn't happen correctly, do nothing */
    }
    else
    {
        if (global_settings.usemrb)
            ret = add_bookmark(RECENT_BOOKMARK_FILE, bookmark, true);


        /* writing the bookmark */
        if (create_bookmark_file)
        {
            char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
            if (generate_bookmark_file_name(name))
            {
                ret = ret & add_bookmark(global_bookmark_file_name, bookmark, false);
            }
            else
            {
                ret = false; /* generating bookmark file failed */
            }
        }
    }

    splash(HZ, ret ? ID2P(LANG_BOOKMARK_CREATE_SUCCESS)
           : ID2P(LANG_BOOKMARK_CREATE_FAILURE));

    return ret;
}

/* Get the name of the playlist and the name of the track from a bookmark. */
/* Returns true iff both were extracted.                                   */
static bool get_playlist_and_track(const char *bookmark, char **pl_start,
                  char **pl_end, char **track)
{
    *pl_start = strchr(bookmark,'/');
    if (!(*pl_start))
        return false;
    *pl_end = strrchr(bookmark,';');
    *track = *pl_end + 1;
    return true;
}

/* ----------------------------------------------------------------------- */
/* This function adds a bookmark to a file.                                */
/* Returns true on successful bookmark add.                                */
/* ------------------------------------------------------------------------*/
static bool add_bookmark(const char* bookmark_file_name, const char* bookmark, 
                         bool most_recent)
{
    int    temp_bookmark_file = 0;
    int    bookmark_file = 0;
    int    bookmark_count = 0;
    char   *pl_start = NULL, *bm_pl_start;
    char   *pl_end = NULL, *bm_pl_end;
    int    pl_len = 0, bm_pl_len;
    char   *track = NULL, *bm_track;
    bool   comp_playlist = false;
    bool   comp_track = false;
    bool   equal;

    /* Opening up a temp bookmark file */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer),
             "%s.tmp", bookmark_file_name);
    temp_bookmark_file = open(global_temp_buffer,
                              O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    if (most_recent && ((global_settings.usemrb == BOOKMARK_ONE_PER_PLAYLIST)
                      || (global_settings.usemrb == BOOKMARK_ONE_PER_TRACK)))
    {
        if (get_playlist_and_track(bookmark, &pl_start, &pl_end, &track))
        {
            comp_playlist = true;
            pl_len = pl_end - pl_start;
            if (global_settings.usemrb == BOOKMARK_ONE_PER_TRACK)
                comp_track = true;
        }
    }

    /* Writing the new bookmark to the begining of the temp file */
    write(temp_bookmark_file, bookmark, strlen(bookmark));
    write(temp_bookmark_file, "\n", 1);
    bookmark_count++;

    /* Reading in the previous bookmarks and writing them to the temp file */
    bookmark_file = open(bookmark_file_name, O_RDONLY);
    if (bookmark_file >= 0)
    {
        while (read_line(bookmark_file, global_read_buffer,
                         sizeof(global_read_buffer)) > 0)
        {
            /* The MRB has a max of MAX_BOOKMARKS in it */
            /* This keeps it from getting too large */
            if (most_recent && (bookmark_count >= MAX_BOOKMARKS))
                break;

            if (!parse_bookmark(global_read_buffer, false))
                break;

            equal = false;
            if (comp_playlist)
            {
                if (get_playlist_and_track(global_read_buffer, &bm_pl_start,
                        &bm_pl_end, &bm_track))
                {
                    bm_pl_len = bm_pl_end - bm_pl_start;
                    equal = (pl_len == bm_pl_len) && !strncmp(pl_start, bm_pl_start, pl_len);
                    if (equal && comp_track)
                        equal = !strcmp(track, bm_track);
                }
            }
            if (!equal)
            {
                bookmark_count++;
                write(temp_bookmark_file, global_read_buffer,
                      strlen(global_read_buffer));
                write(temp_bookmark_file, "\n", 1);
            }
        }
        close(bookmark_file);
    }
    close(temp_bookmark_file);

    remove(bookmark_file_name);
    rename(global_temp_buffer, bookmark_file_name);

    return true;
}


/* ----------------------------------------------------------------------- */
/* This function takes the system resume data and formats it into a valid  */
/* bookmark.                                                               */
/* Returns not NULL on successful bookmark format.                         */
/* ----------------------------------------------------------------------- */
static char* create_bookmark()
{
    int resume_index = 0;
    char *file;

    if (!bookmark_is_bookmarkable_state())
        return NULL; /* something didn't happen correctly, do nothing */

    /* grab the currently playing track */
    struct mp3entry *id3 = audio_current_track();
    if(!id3)
        return NULL;

    /* Get some basic resume information */
    /* queue_resume and queue_resume_index are not used and can be ignored.*/
    playlist_get_resume_info(&resume_index);

    /* Get the currently playing file minus the path */
    /* This is used when displaying the available bookmarks */
    file = strrchr(id3->path,'/');
    if(NULL == file)
        return NULL;

    /* create the bookmark */
    snprintf(global_bookmark, sizeof(global_bookmark),
             /* new optional bookmark token descriptors should be inserted
                just before the "%s;%s" in this line... */
#if CONFIG_CODEC == SWCODEC && defined(HAVE_PITCHCONTROL)
             ">%d;%d;%ld;%d;%ld;%d;%d;%ld;%ld;%s;%s",
#else
             ">%d;%d;%ld;%d;%ld;%d;%d;%s;%s",
#endif
             /* ... their flags should go here ... */
#if CONFIG_CODEC == SWCODEC && defined(HAVE_PITCHCONTROL)
             BM_PITCH | BM_SPEED,
#else
             0,
#endif
             resume_index,
             id3->offset,
             playlist_get_seed(NULL),
             id3->elapsed,
             global_settings.repeat_mode,
             global_settings.playlist_shuffle,
             /* ...and their values should go here */
#if CONFIG_CODEC == SWCODEC && defined(HAVE_PITCHCONTROL)
             (long)sound_get_pitch(),
             (long)dsp_get_timestretch(),
#endif
             /* more mandatory tokens */
             playlist_get_name(NULL, global_temp_buffer,
                sizeof(global_temp_buffer)),
             file+1);

    /* checking to see if the bookmark is valid */
    if (parse_bookmark(global_bookmark, false))
        return global_bookmark;
    else
        return NULL;
}

/* ----------------------------------------------------------------------- */
/* This function will determine if an autoload is necessary.  This is an   */
/* interface function.                                                     */
/* Returns true on bookmark load or bookmark selection.                    */
/* ------------------------------------------------------------------------*/
bool bookmark_autoload(const char* file)
{
    char* bookmark;

    if(global_settings.autoloadbookmark == BOOKMARK_NO)
        return false;

    /*Checking to see if a bookmark file exists.*/
    if(!generate_bookmark_file_name(file))
    {
        return false;
    }

    if(!file_exists(global_bookmark_file_name))
        return false;

    if(global_settings.autoloadbookmark == BOOKMARK_YES)
    {
        return bookmark_load(global_bookmark_file_name, true);
    }
    else
    {
        int ret = select_bookmark(global_bookmark_file_name, true, &bookmark);
        
        if (bookmark != NULL)
        {
            if (!play_bookmark(bookmark))
            {
                /* Selected bookmark not found. */
                splash(HZ*2, ID2P(LANG_NOTHING_TO_RESUME));
            }

            /* Act as if autoload was done even if it failed, since the 
             * user did make an active selection.
             */
            return true;
        }

        return ret != BOOKMARK_SUCCESS;
    }
}

/* ----------------------------------------------------------------------- */
/* This function loads the bookmark information into the resume memory.    */
/* This is an interface function.                                          */
/* Returns true on successful bookmark load.                               */
/* ------------------------------------------------------------------------*/
bool bookmark_load(const char* file, bool autoload)
{
    int  fd;
    char* bookmark = NULL;

    if(autoload)
    {
        fd = open(file, O_RDONLY);
        if(fd >= 0)
        {
            if(read_line(fd, global_read_buffer, sizeof(global_read_buffer)) > 0)
                bookmark=global_read_buffer;
            close(fd);
        }
    }
    else
    {
        /* This is not an auto-load, so list the bookmarks */
        select_bookmark(file, false, &bookmark);
    }

    if (bookmark != NULL)
    {
        if (!play_bookmark(bookmark))
        {
            /* Selected bookmark not found. */
            if (!autoload)
            {
                splash(HZ*2, ID2P(LANG_NOTHING_TO_RESUME));
            }
            
            return false;
        }
    }

    return true;
}


static int get_bookmark_count(const char* bookmark_file_name)
{
    int read_count = 0;
    int file = open(bookmark_file_name, O_RDONLY);

    if(file < 0)
        return -1;

    while(read_line(file, global_read_buffer, sizeof(global_read_buffer)) > 0)
    {
        read_count++;
    }
    
    close(file);
    return read_count;
}

static int buffer_bookmarks(struct bookmark_list* bookmarks, int first_line)
{
    char* dest = ((char*) bookmarks) + bookmarks->buffer_size - 1;
    int read_count = 0;
    int file = open(bookmarks->filename, O_RDONLY);

    if (file < 0)
    {
        return -1;
    }

    if ((first_line != 0) && ((size_t) filesize(file) < bookmarks->buffer_size
            - sizeof(*bookmarks) - (sizeof(char*) * bookmarks->total_count)))
    {
        /* Entire file fits in buffer */
        first_line = 0;
    }
    
    bookmarks->start = first_line;
    bookmarks->count = 0;
    bookmarks->reload = false;
    
    while(read_line(file, global_read_buffer, sizeof(global_read_buffer)) > 0)
    {
        read_count++;
        
        if (read_count >= first_line)
        {
            dest -= strlen(global_read_buffer) + 1;
            
            if (dest < ((char*) bookmarks) + sizeof(*bookmarks)
                + (sizeof(char*) * (bookmarks->count + 1)))
            {
                break;
            }
            
            strcpy(dest, global_read_buffer);
            bookmarks->items[bookmarks->count] = dest;
            bookmarks->count++;
        }
    }

    close(file);
    return bookmarks->start + bookmarks->count;
}

static const char* get_bookmark_info(int list_index,
                                     void* data,
                                     char *buffer,
                                     size_t buffer_len)
{
    struct bookmark_list* bookmarks = (struct bookmark_list*) data;
    int     index = list_index / 2;

    if (bookmarks->show_dont_resume)
    {
        if (index == 0)
        {
            return list_index % 2 == 0 
                ? (char*) str(LANG_BOOKMARK_DONT_RESUME) : " ";
        }
        
        index--;
    }

    if (bookmarks->reload || (index >= bookmarks->start + bookmarks->count) 
        || (index < bookmarks->start))
    {
        int read_index = index;
        
        /* Using count as a guide on how far to move could possibly fail
         * sometimes. Use byte count if that is a problem?
         */
        
        if (read_index != 0)
        {
            /* Move count * 3 / 4 items in the direction the user is moving,
             * but don't go too close to the end.
             */
            int offset = bookmarks->count;
            int max = bookmarks->total_count - (bookmarks->count / 2);
            
            if (read_index < bookmarks->start)
            {
                offset *= 3;
            }
         
            read_index = index - offset / 4;

            if (read_index > max)
            {
                read_index = max;
            }
            
            if (read_index < 0)
            {
                read_index = 0;
            }
        }
        
        if (buffer_bookmarks(bookmarks, read_index) <= index)
        {
            return "";
        }
    }
    
    if (!parse_bookmark(bookmarks->items[index - bookmarks->start], true))
    {
        return list_index % 2 == 0 ? (char*) str(LANG_BOOKMARK_INVALID) : " ";
    }

    if (list_index % 2 == 0)
    {
        char *name;
        char *format;
        int len = strlen(global_temp_buffer);
        
        if (bookmarks->show_playlist_name && len > 0)
        {
            name = global_temp_buffer;
            len--;
            
            if (name[len] != '/')
            {
                strrsplt(name, '.');
            }
            else if (len > 1)
            {
                name[len] = '\0';
            }

            if (len > 1)
            {
                name = strrsplt(name, '/');
            }

            format = "%s : %s";
        }
        else
        {
            name = global_filename;
            format = "%s";
        }
        
        strrsplt(global_filename, '.');
        snprintf(buffer, buffer_len, format, name, global_filename);
        return buffer;
    }
    else
    {
        char time_buf[32];

        format_time(time_buf, sizeof(time_buf), bm.resume_time);
        snprintf(buffer, buffer_len, "%s, %d%s", time_buf, bm.resume_index + 1,
            bm.shuffle ? (char*) str(LANG_BOOKMARK_SHUFFLE) : "");
        return buffer;
    }
}

static int bookmark_list_voice_cb(int list_index, void* data)
{
    struct bookmark_list* bookmarks = (struct bookmark_list*) data;
    int index = list_index / 2;

    if (bookmarks->show_dont_resume)
    {
        if (index == 0)
            return talk_id(LANG_BOOKMARK_DONT_RESUME, false);
        index--;
    }
    say_bookmark(bookmarks->items[index - bookmarks->start], index,
                 bookmarks->show_playlist_name);
    return 0;
}

/* ----------------------------------------------------------------------- */
/* This displays the bookmarks in a file and allows the user to            */
/* select one to play.                                                     */
/* *selected_bookmark contains a non NULL value on successful bookmark     */
/* selection.                                                              */
/* Returns BOOKMARK_SUCCESS on successful bookmark selection, BOOKMARK_FAIL*/
/* if no selection was made and BOOKMARK_USB_CONNECTED if the selection    */
/* menu is forced to exit due to a USB connection.                         */
/* ------------------------------------------------------------------------*/
static int select_bookmark(const char* bookmark_file_name, bool show_dont_resume, char** selected_bookmark)
{
    struct bookmark_list* bookmarks;
    struct gui_synclist list;
    int item = 0;
    int action;
    size_t size;
    bool exit = false;
    bool refresh = true;
    int ret = BOOKMARK_FAIL;

    bookmarks = plugin_get_buffer(&size);
    bookmarks->buffer_size = size;
    bookmarks->show_dont_resume = show_dont_resume;
    bookmarks->filename = bookmark_file_name;
    bookmarks->start = 0;
    bookmarks->show_playlist_name
        = strcmp(bookmark_file_name, RECENT_BOOKMARK_FILE) == 0;
    gui_synclist_init(&list, &get_bookmark_info, (void*) bookmarks, false, 2, NULL);
    if(global_settings.talk_menu)
        gui_synclist_set_voice_callback(&list, bookmark_list_voice_cb);
    gui_synclist_set_title(&list, str(LANG_BOOKMARK_SELECT_BOOKMARK), 
        Icon_Bookmark);

    while (!exit)
    {
        
        if (refresh)
        {
            int count = get_bookmark_count(bookmark_file_name);
            bookmarks->total_count = count;

            if (bookmarks->total_count < 1)
            {
                /* No more bookmarks, delete file and exit */
                splash(HZ, ID2P(LANG_BOOKMARK_LOAD_EMPTY));
                remove(bookmark_file_name);
                *selected_bookmark = NULL;
                return BOOKMARK_FAIL;
            }

            if (bookmarks->show_dont_resume)
            {
                count++;
                item++;
            }

            gui_synclist_set_nb_items(&list, count * 2);

            if (item >= count)
            {
                /* Selected item has been deleted */
                item = count - 1;
                gui_synclist_select_item(&list, item * 2);
            }

            buffer_bookmarks(bookmarks, bookmarks->start);
            gui_synclist_draw(&list);
            cond_talk_ids_fq(VOICE_EXT_BMARK);
            gui_synclist_speak_item(&list);
            refresh = false;
        }

        list_do_action(CONTEXT_BOOKMARKSCREEN, HZ / 2,
                       &list, &action, LIST_WRAP_UNLESS_HELD);
        item = gui_synclist_get_sel_pos(&list) / 2;

        if (bookmarks->show_dont_resume)
        {
            item--;
        }

        if (action == ACTION_STD_CONTEXT)
        {
            MENUITEM_STRINGLIST(menu_items, ID2P(LANG_BOOKMARK_CONTEXT_MENU),
                NULL, ID2P(LANG_BOOKMARK_CONTEXT_RESUME), 
                ID2P(LANG_BOOKMARK_CONTEXT_DELETE));
            static const int menu_actions[] = 
            {
                ACTION_STD_OK, ACTION_BMS_DELETE
            };
            int selection = do_menu(&menu_items, NULL, NULL, false);
            
            refresh = true;

            if (selection >= 0 && selection <= 
                (int) (sizeof(menu_actions) / sizeof(menu_actions[0])))
            {
                action = menu_actions[selection];
            }
        }

        switch (action)
        {
        case ACTION_STD_OK:
            if (item >= 0)
            {
                talk_shutup();
                *selected_bookmark = bookmarks->items[item - bookmarks->start];
                return BOOKMARK_SUCCESS;
            }
            exit = true;
            ret = BOOKMARK_SUCCESS;
            break;

        case ACTION_TREE_WPS:
        case ACTION_STD_CANCEL:
            exit = true;
            break;

        case ACTION_BMS_DELETE:
            if (item >= 0)
            {                
                const char *lines[]={
                    ID2P(LANG_REALLY_DELETE)
                };
                const char *yes_lines[]={
                    ID2P(LANG_DELETING)
                };

                const struct text_message message={lines, 1};
                const struct text_message yes_message={yes_lines, 1};

                if(gui_syncyesno_run(&message, &yes_message, NULL)==YESNO_YES)
                {                    
                    delete_bookmark(bookmark_file_name, item);
                    bookmarks->reload = true;
                }
                refresh = true;
            }
            break;

        default:
            if (default_event_handler(action) == SYS_USB_CONNECTED)
            {
                ret = BOOKMARK_USB_CONNECTED;
                exit = true;
            }

            break;
        }
    }

    talk_shutup();
    *selected_bookmark = NULL;
    return ret;
}

/* ----------------------------------------------------------------------- */
/* This function takes a location in a bookmark file and deletes that      */
/* bookmark.                                                               */
/* Returns true on successful bookmark deletion.                           */
/* ------------------------------------------------------------------------*/
static bool delete_bookmark(const char* bookmark_file_name, int bookmark_id)
{
    int temp_bookmark_file = 0;
    int bookmark_file = 0;
    int bookmark_count = 0;

    /* Opening up a temp bookmark file */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer),
             "%s.tmp", bookmark_file_name);
    temp_bookmark_file = open(global_temp_buffer,
                              O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    /* Reading in the previous bookmarks and writing them to the temp file */
    bookmark_file = open(bookmark_file_name, O_RDONLY);
    if (bookmark_file >= 0)
    {
        while (read_line(bookmark_file, global_read_buffer,
                         sizeof(global_read_buffer)) > 0)
        {
            if (bookmark_id != bookmark_count)
            {
                write(temp_bookmark_file, global_read_buffer,
                      strlen(global_read_buffer));
                write(temp_bookmark_file, "\n", 1);
            }
            bookmark_count++;
        }
        close(bookmark_file);
    }
    close(temp_bookmark_file);

    remove(bookmark_file_name);
    rename(global_temp_buffer, bookmark_file_name);

    return true;
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark, says the voice UI part of it.          */
/* ------------------------------------------------------------------------*/
static void say_bookmark(const char* bookmark,
                         int bookmark_id, bool show_playlist_name)
{
    if (!parse_bookmark(bookmark, true))
    {
        talk_id(LANG_BOOKMARK_INVALID, false);
        return;
    }

    talk_number(bookmark_id + 1, false);

#if CONFIG_CODEC == SWCODEC
    bool is_dir = (global_temp_buffer[0]
              && global_temp_buffer[strlen(global_temp_buffer)-1] == '/');

    /* HWCODEC cannot enqueue voice file entries and .talk thumbnails
       together, because there is no guarantee that the same mp3
       parameters are used. */
    if(show_playlist_name)
    {   /* It's useful to know which playlist this is */
        if(is_dir)
            talk_dir_or_spell(global_temp_buffer,
                              TALK_IDARRAY(VOICE_DIR), true);
        else talk_file_or_spell(NULL, global_temp_buffer,
                                TALK_IDARRAY(LANG_PLAYLIST), true);
    }
#else
    (void)show_playlist_name;
#endif

    if(bm.shuffle)
        talk_id(LANG_SHUFFLE, true);

    talk_id(VOICE_BOOKMARK_SELECT_INDEX_TEXT, true);
    talk_number(bm.resume_index + 1, true);
    talk_id(LANG_TIME, true);
    talk_value(bm.resume_time / 1000, UNIT_TIME, true);

#if CONFIG_CODEC == SWCODEC
    /* Track filename */
    if(is_dir)
        talk_file_or_spell(global_temp_buffer, global_filename,
                           TALK_IDARRAY(VOICE_FILE), true);
    else
    {   /* Unfortunately if this is a playlist, we do not know in which
           directory the file is and therefore cannot find the track's
           .talk file. */
        talk_id(VOICE_FILE, true);
        talk_spell(global_filename, true);
    }
#endif
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark and then plays it.                      */
/* Returns true on successful bookmark play.                               */
/* ------------------------------------------------------------------------*/
static bool play_bookmark(const char* bookmark)
{
#if CONFIG_CODEC == SWCODEC && defined(HAVE_PITCHCONTROL)
    /* preset pitch and speed to 100% in case bookmark doesn't have info */
    bm.pitch = sound_get_pitch();
    bm.speed = dsp_get_timestretch();
#endif
    
    if (parse_bookmark(bookmark, true))
    {
        global_settings.repeat_mode = bm.repeat_mode;
        global_settings.playlist_shuffle = bm.shuffle;
#if CONFIG_CODEC == SWCODEC && defined(HAVE_PITCHCONTROL)
        sound_set_pitch(bm.pitch);
        dsp_set_timestretch(bm.speed);
#endif
        if (!warn_on_pl_erase())
            return false;
        return bookmark_play(global_temp_buffer, bm.resume_index,
            bm.resume_time, bm.resume_offset, bm.resume_seed, global_filename);
    }
    
    return false;
}

static const char* skip_token(const char* s)
{
    while (*s && *s != ';')
    {
        s++;
    }
    
    if (*s)
    {
        s++;
    }
    
    return s;
}

static const char* int_token(const char* s, int* dest)
{
    *dest = atoi(s);
    return skip_token(s);
}

static const char* long_token(const char* s, long* dest)
{
    *dest = atoi(s);    /* Should be atol, but we don't have it. */
    return skip_token(s);
}

/* ----------------------------------------------------------------------- */
/* This function takes a bookmark and parses it.  This function also       */
/* validates the bookmark.  The parse_filenames flag indicates whether     */
/* the filename tokens are to be extracted.                                */
/* Returns true on successful bookmark parse.                              */
/* ----------------------------------------------------------------------- */
static bool parse_bookmark(const char *bookmark, const bool parse_filenames)
{
    const char* s = bookmark;
    const char* end;
    
#define GET_INT_TOKEN(var)  s = int_token(s, &var)
#define GET_LONG_TOKEN(var)  s = long_token(s, &var)
#define GET_BOOL_TOKEN(var) var = (atoi(s)!=0); s = skip_token(s)
    
    /* if new format bookmark, extract the optional content flags,
       otherwise treat as an original format bookmark */
    int opt_flags = 0;
    bool new_format = (strchr(s, '>') == s);
    if (new_format)
    {
        s++;
        GET_INT_TOKEN(opt_flags);
    }
    
    /* extract all original bookmark tokens */
    GET_INT_TOKEN(bm.resume_index);
    GET_LONG_TOKEN(bm.resume_offset);
    GET_INT_TOKEN(bm.resume_seed);
    if (!new_format)    /* skip deprecated token */
        s = skip_token(s);
    GET_LONG_TOKEN(bm.resume_time);
    GET_INT_TOKEN(bm.repeat_mode);
    GET_BOOL_TOKEN(bm.shuffle);
    
    /* extract all optional bookmark tokens */
    if (opt_flags & BM_PITCH)
        GET_INT_TOKEN(bm.pitch);
    if (opt_flags & BM_SPEED)
        GET_INT_TOKEN(bm.speed);
    
    if (*s == 0)
    {
        return false;
    }
    
    end = strchr(s, ';');

    /* extract file names */
    if (parse_filenames)
    {
        size_t len = (end == NULL) ? strlen(s) : (size_t) (end - s);
        len = MIN(TEMP_BUF_SIZE - 1, len);
        strlcpy(global_temp_buffer, s, len + 1);
        
        if (end != NULL)
        {
            end++;
            strlcpy(global_filename, end, MAX_PATH);
        }
     }
    
    return true;
}

/* ----------------------------------------------------------------------- */
/* This function is used by multiple functions and is used to generate a   */
/* bookmark named based off of the input.                                  */
/* Changing this function could result in how the bookmarks are stored.    */
/* it would be here that the centralized/decentralized bookmark code       */
/* could be placed.                                                        */
/* Always returns true                                                     */
/* ----------------------------------------------------------------------- */
static bool generate_bookmark_file_name(const char *in)
{
    int len = strlen(in);

    /* if this is a root dir MP3, rename the bookmark file root_dir.bmark */
    /* otherwise, name it based on the in variable */
    if (!strcmp("/", in))
        strcpy(global_bookmark_file_name, "/root_dir.bmark");
    else
    {
#ifdef HAVE_MULTIVOLUME
        /* The "root" of an extra volume need special handling too. */
        const char *filename;
        path_strip_volume(in, &filename, true);
        bool volume_root = *filename == '\0';
#endif
        strcpy(global_bookmark_file_name, in);
        if(global_bookmark_file_name[len-1] == '/')
            len--;
#ifdef HAVE_MULTIVOLUME
        if (volume_root)
            strcpy(&global_bookmark_file_name[len], "/volume_dir.bmark");
        else
#endif
            strcpy(&global_bookmark_file_name[len], ".bmark");
    }

    return true;
}

/* ----------------------------------------------------------------------- */
/* Returns true if a bookmark file exists for the current playlist.        */
/* This is an interface function.                                          */
/* ----------------------------------------------------------------------- */
bool bookmark_exists(void)
{
    bool exist=false;

    char* name = playlist_get_name(NULL, global_temp_buffer,
                                   sizeof(global_temp_buffer));
    if (generate_bookmark_file_name(name))
    {
        exist = file_exists(global_bookmark_file_name);
    }
    return exist;
}

/* ----------------------------------------------------------------------- */
/* Checks the current state of the system and returns true if it is in a   */
/* bookmarkable state.                                                     */
/* This is an interface funtion.                                           */
/* ----------------------------------------------------------------------- */
bool bookmark_is_bookmarkable_state(void)
{
    int resume_index = 0;

    if (!(audio_status() && audio_current_track()) ||
        /* no track playing */
       (playlist_get_resume_info(&resume_index) == -1) ||
        /* invalid queue info */
       (playlist_modified(NULL)))
        /* can't bookmark while in the queue */
    {
        return false;
    }

    return true;
}

