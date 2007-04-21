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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "applimits.h"
#include "lcd.h"
#include "action.h"
#include "usb.h"
#include "audio.h"
#include "playlist.h"
#include "settings.h"
#include "tree.h"
#include "bookmark.h"
#include "dir.h"
#include "status.h"
#include "system.h"
#include "errno.h"
#include "icons.h"
#include "atoi.h"
#include "string.h"
#include "menu.h"
#include "lang.h"
#include "screens.h"
#include "status.h"
#include "debug.h"
#include "kernel.h"
#include "sprintf.h"
#include "talk.h"
#include "misc.h"
#include "abrepeat.h"
#include "splash.h"
#include "yesno.h"

#if LCD_DEPTH > 1
#include "backdrop.h"
#endif

#define MAX_BOOKMARKS 10
#define MAX_BOOKMARK_SIZE  350
#define RECENT_BOOKMARK_FILE ROCKBOX_DIR "/most-recent.bmark"

static bool  add_bookmark(const char* bookmark_file_name, const char* bookmark, 
                          bool most_recent);
static bool  check_bookmark(const char* bookmark);
static char* create_bookmark(void);
static bool  delete_bookmark(const char* bookmark_file_name, int bookmark_id);
static void  display_bookmark(const char* bookmark,
                              int bookmark_id, 
                              int bookmark_count);
static void  say_bookmark(const char* bookmark,
                          int bookmark_id);
static bool play_bookmark(const char* bookmark);
static bool  generate_bookmark_file_name(const char *in);
static char* get_bookmark(const char* bookmark_file, int bookmark_count);
static const char* skip_token(const char* s);
static const char* int_token(const char* s, int* dest);
static const char* long_token(const char* s, long* dest);
static const char* bool_token(const char* s, bool* dest);
static bool  parse_bookmark(const char *bookmark,
                            int *resume_index,
                            int *resume_offset,
                            int *resume_seed,
                            int *resume_first_index,
                            char* resume_file,
                            unsigned int resume_file_size,
                            long* ms,
                            int * repeat_mode,
                            bool *shuffle,
                            char* file_name);
static char* select_bookmark(const char* bookmark_file_name);
static bool  system_check(void);
static bool  write_bookmark(bool create_bookmark_file);
static int   get_bookmark_count(const char* bookmark_file_name);

static char global_temp_buffer[MAX_PATH+1];
static char global_bookmark_file_name[MAX_PATH];
static char global_read_buffer[MAX_BOOKMARK_SIZE];
static char global_bookmark[MAX_BOOKMARK_SIZE];
static char global_filename[MAX_PATH];

/* ----------------------------------------------------------------------- */
/* This is the interface function from the main menu.                      */
/* ----------------------------------------------------------------------- */
bool bookmark_create_menu(void)
{
    write_bookmark(true);
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function acts as the load interface from the main menu             */
/* This function determines the bookmark file name and then loads that file*/
/* for the user.  The user can then select a bookmark to load.             */
/* If no file/directory is currently playing, the menu item does not work. */
/* ----------------------------------------------------------------------- */
bool bookmark_load_menu(void)
{
    if (system_check())
    {
        char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
        if (generate_bookmark_file_name(name))
        {
            char* bookmark = select_bookmark(global_bookmark_file_name);
            
            if (bookmark != NULL)
            {
                return play_bookmark(bookmark);
            }
        }
    }

    return false;
}

/* ----------------------------------------------------------------------- */
/* Gives the user a list of the Most Recent Bookmarks.  This is an         */
/* interface function                                                      */
/* ----------------------------------------------------------------------- */
bool bookmark_mrb_load()
{
    char* bookmark = select_bookmark(RECENT_BOOKMARK_FILE);

    if (bookmark != NULL)
    {
        return play_bookmark(bookmark);
    }

    return false;
}

/* ----------------------------------------------------------------------- */
/* This function handles an autobookmark creation.  This is an interface   */
/* function.                                                               */
/* ----------------------------------------------------------------------- */
bool bookmark_autobookmark(void)
{
    if (!system_check())
        return false;

    audio_pause();    /* first pause playback */
    switch (global_settings.autocreatebookmark)
    {
        case BOOKMARK_YES:
            return write_bookmark(true);

        case BOOKMARK_NO:
            return false;

        case BOOKMARK_RECENT_ONLY_YES:
            return write_bookmark(false);
    }
#ifdef HAVE_LCD_BITMAP
    unsigned char *lines[]={str(LANG_AUTO_BOOKMARK_QUERY)};
    struct text_message message={(char **)lines, 1};
#else
    unsigned char *lines[]={str(LANG_AUTO_BOOKMARK_QUERY),
                            str(LANG_RESUME_CONFIRM_PLAYER)};
    struct text_message message={(char **)lines, 2};
#endif
#if LCD_DEPTH > 1
    show_main_backdrop(); /* switch to main backdrop as we may come from wps */
#endif
    gui_syncstatusbar_draw(&statusbars, false);
    if(gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES)
    {
        if (global_settings.autocreatebookmark == BOOKMARK_RECENT_ONLY_ASK)
            return write_bookmark(false);
        else
            return write_bookmark(true);
    }
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function takes the current current resume information and writes   */
/* that to the beginning of the bookmark file.                             */
/* This file will contain N number of bookmarks in the following format:   */
/* resume_index*resume_offset*resume_seed*resume_first_index*              */
/* resume_file*milliseconds*MP3 Title*                                     */
/* ------------------------------------------------------------------------*/
static bool write_bookmark(bool create_bookmark_file)
{
    bool   success=false;
    char*  bookmark;

    if (!system_check())
       return false; /* something didn't happen correctly, do nothing */

    bookmark = create_bookmark();
    if (!bookmark)
       return false; /* something didn't happen correctly, do nothing */

    if (global_settings.usemrb)
        success = add_bookmark(RECENT_BOOKMARK_FILE, bookmark, true);


    /* writing the bookmark */
    if (create_bookmark_file)
    {
        char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
        if (generate_bookmark_file_name(name))
        {
            success = add_bookmark(global_bookmark_file_name, bookmark, false);
        }
    }

    gui_syncsplash(HZ, str(success ? LANG_BOOKMARK_CREATE_SUCCESS
        : LANG_BOOKMARK_CREATE_FAILURE));

    return true;
}

/* ----------------------------------------------------------------------- */
/* This function adds a bookmark to a file.                                */
/* ------------------------------------------------------------------------*/
static bool add_bookmark(const char* bookmark_file_name, const char* bookmark, 
                         bool most_recent)
{
    int    temp_bookmark_file = 0;
    int    bookmark_file = 0;
    int    bookmark_count = 0;
    char*  playlist = NULL;
    char*  cp;
    char*  tmp;
    int    len = 0;
    bool   unique = false;

    /* Opening up a temp bookmark file */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer),
             "%s.tmp", bookmark_file_name);
    temp_bookmark_file = open(global_temp_buffer,
                              O_WRONLY | O_CREAT | O_TRUNC);
    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    if (most_recent && (global_settings.usemrb == BOOKMARK_UNIQUE_ONLY))
    {
        playlist = strchr(bookmark,'/');
        cp = strrchr(bookmark,';');
        len = cp - playlist;
        unique = true;
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
                        
            cp  = strchr(global_read_buffer,'/');
            tmp = strrchr(global_read_buffer,';');
            if (check_bookmark(global_read_buffer) &&
               (!unique || len != tmp -cp || strncmp(playlist,cp,len)))
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
/* ----------------------------------------------------------------------- */
static char* create_bookmark()
{
    int resume_index = 0;
    char *file;

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
             "%d;%ld;%d;%d;%ld;%d;%d;%s;%s",
             resume_index,
             id3->offset,
             playlist_get_seed(NULL),
             0,
             id3->elapsed,
             global_settings.repeat_mode,
             global_settings.playlist_shuffle,
             playlist_get_name(NULL, global_temp_buffer,
                sizeof(global_temp_buffer)),
             file+1);

    /* checking to see if the bookmark is valid */
    if (check_bookmark(global_bookmark))
        return global_bookmark;
    else
        return NULL;
}

static bool check_bookmark(const char* bookmark)
{
    return parse_bookmark(bookmark,
                          NULL,NULL,NULL, NULL,
                          NULL,0,NULL,NULL,
                          NULL, NULL);
}

/* ----------------------------------------------------------------------- */
/* This function will determine if an autoload is necessary.  This is an   */
/* interface function.                                                     */
/* ------------------------------------------------------------------------*/
bool bookmark_autoload(const char* file)
{
    int  key;
    int  fd;
    int i;

    if(global_settings.autoloadbookmark == BOOKMARK_NO)
        return false;

    /*Checking to see if a bookmark file exists.*/
    if(!generate_bookmark_file_name(file))
    {
        return false;
    }
    fd = open(global_bookmark_file_name, O_RDONLY);
    if(fd<0)
        return false;
    close(fd);
    if(global_settings.autoloadbookmark == BOOKMARK_YES)
    {
        return bookmark_load(global_bookmark_file_name, true);
    }
    else
    {
        /* Prompting user to confirm bookmark load */
        FOR_NB_SCREENS(i)
            screens[i].clear_display();
        
        gui_syncstatusbar_draw(&statusbars, true);
        
        FOR_NB_SCREENS(i)
        {
#ifdef HAVE_LCD_BITMAP
            screens[i].setmargins(0, global_settings.statusbar
                ? STATUSBAR_HEIGHT : 0);
            screens[i].puts_scroll(0,0, str(LANG_BOOKMARK_AUTOLOAD_QUERY));
            screens[i].puts(0,1, str(LANG_CONFIRM_WITH_PLAY_RECORDER));
            screens[i].puts(0,2, str(LANG_BOOKMARK_SELECT_LIST_BOOKMARKS));
            screens[i].puts(0,3, str(LANG_CANCEL_WITH_ANY_RECORDER));
#else
            screens[i].puts_scroll(0,0, str(LANG_BOOKMARK_AUTOLOAD_QUERY));
            screens[i].puts(0,1,str(LANG_RESUME_CONFIRM_PLAYER));
#endif
            screens[i].update();
        }

        /* Wait for a key to be pushed */
        key = get_action(CONTEXT_BOOKMARKSCREEN,TIMEOUT_BLOCK);
        switch(key)
        {
#ifdef HAVE_LCD_BITMAP
            case ACTION_STD_NEXT:
                action_signalscreenchange();
                return bookmark_load(global_bookmark_file_name, false);
#endif
            case ACTION_BMS_SELECT:
                action_signalscreenchange();
                return bookmark_load(global_bookmark_file_name, true);

            default:
                break;
        }

        action_signalscreenchange();
        return false;
    }
}

/* ----------------------------------------------------------------------- */
/* This function loads the bookmark information into the resume memory.    */
/* This is an interface function.                                          */
/* ------------------------------------------------------------------------*/
bool bookmark_load(const char* file, bool autoload)
{
    int  fd;
    char* bookmark = NULL;;

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
        bookmark = select_bookmark(file);
    }

    if (bookmark != NULL)
    {
        return play_bookmark(bookmark);
    }

    return true;
}


static int get_bookmark_count(const char* bookmark_file_name)
{
    int read_count = 0;
    int file = open(bookmark_file_name, O_RDONLY);

    if(file < 0)
        return -1;

    /* Get the requested bookmark */
    while(read_line(file, global_read_buffer, sizeof(global_read_buffer)) > 0)
    {
        read_count++;
    }
    
    close(file);
    return read_count;
 
    
}

/* ----------------------------------------------------------------------- */
/* This displays a the bookmarks in a file and allows the user to          */
/* select one to play.                                                     */
/* ------------------------------------------------------------------------*/
static char* select_bookmark(const char* bookmark_file_name)
{
    int bookmark_id = 0;
    int bookmark_id_prev = -1;
    int key;
    char* bookmark = NULL;
    int bookmark_count = 0;

#ifdef HAVE_LCD_BITMAP
    int i;

    FOR_NB_SCREENS(i)
        screens[i].setmargins(0, global_settings.statusbar
            ? STATUSBAR_HEIGHT : 0);
#endif

    bookmark_count = get_bookmark_count(bookmark_file_name);
    if (bookmark_count < 1) /* error opening file, or empty file */
    {
        gui_syncsplash(HZ, str(LANG_BOOKMARK_LOAD_EMPTY));
        return NULL;
    }
    action_signalscreenchange();
    while(true)
    {
        if(bookmark_id < 0)
            bookmark_id = bookmark_count -1;
        if(bookmark_id >= bookmark_count)
            bookmark_id = 0;

        if (bookmark_id != bookmark_id_prev)
        {
            bookmark = get_bookmark(bookmark_file_name, bookmark_id);
            bookmark_id_prev = bookmark_id;
        }
        
        if (!bookmark)
        {
            /* if there were no bookmarks in the file, delete the file and exit. */
            if(bookmark_id <= 0)
            {
                gui_syncsplash(HZ, str(LANG_BOOKMARK_LOAD_EMPTY));
                remove(bookmark_file_name);
                action_signalscreenchange();
                return NULL;
            }
            else
            {
               bookmark_id_prev = bookmark_id;
               bookmark_id--;
               continue;
            }
        }
        else
        {
            display_bookmark(bookmark, bookmark_id, bookmark_count);
            if (global_settings.talk_menu) /* for voice UI */
                say_bookmark(bookmark, bookmark_id);
        }

        /* waiting for the user to click a button */
        key = get_action(CONTEXT_BOOKMARKSCREEN,TIMEOUT_BLOCK);
        switch(key)
        {
            case ACTION_BMS_SELECT:
                /* User wants to use this bookmark */
                action_signalscreenchange();
                return bookmark;

            case ACTION_BMS_DELETE:
                /* User wants to delete this bookmark */
                delete_bookmark(bookmark_file_name, bookmark_id);
                bookmark_id_prev=-2;
                bookmark_count--;
                if(bookmark_id >= bookmark_count)
                    bookmark_id = bookmark_count -1;
                break;

            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                bookmark_id--;
                break;

            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                bookmark_id++;
                break;

            case ACTION_BMS_EXIT:
                action_signalscreenchange();
                return NULL;

            default:
                if(default_event_handler(key) == SYS_USB_CONNECTED)
                {
                    action_signalscreenchange();
                    return NULL;
                }
                break;
        }
    }
    action_signalscreenchange();
    return NULL;
}


/* ----------------------------------------------------------------------- */
/* This function takes a location in a bookmark file and deletes that      */
/* bookmark.                                                               */
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
                              O_WRONLY | O_CREAT | O_TRUNC);

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
/* This function parses a bookmark and displays it for the user.           */
/* ------------------------------------------------------------------------*/
static void display_bookmark(const char* bookmark,
                             int bookmark_id,
                             int bookmark_count)
{
    int  resume_index = 0;
    long  ms = 0;
    int  repeat_mode = 0;
    bool playlist_shuffle = false;
    char *dot;
    char time_buf[32];
    int i;

    /* getting the index and the time into the file */
    parse_bookmark(bookmark,
                   &resume_index, NULL, NULL, NULL, NULL, 0,
                   &ms, &repeat_mode, &playlist_shuffle,
                   global_filename);

    FOR_NB_SCREENS(i)
        screens[i].clear_display();

#ifdef HAVE_LCD_BITMAP
    /* bookmark shuffle and repeat states*/
    switch (repeat_mode)
    {
#ifdef AB_REPEAT_ENABLE
        case REPEAT_AB:
            statusbar_icon_play_mode(Icon_RepeatAB);
            break;
#endif

        case REPEAT_ONE:
            statusbar_icon_play_mode(Icon_RepeatOne);
            break;

        case REPEAT_ALL:
            statusbar_icon_play_mode(Icon_Repeat);
            break;
    }
    if(playlist_shuffle)
        statusbar_icon_shuffle();

    /* File Name */
    dot = strrchr(global_filename, '.');

    if (dot)
        *dot='\0';

    FOR_NB_SCREENS(i)
        screens[i].puts_scroll(0, 0, (unsigned char *)global_filename);

    if (dot)
        *dot='.';

    /* bookmark number */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer), "%s: %d/%d",
             str(LANG_BOOKMARK_SELECT_BOOKMARK_TEXT),
             bookmark_id + 1, bookmark_count);
    FOR_NB_SCREENS(i)
        screens[i].puts_scroll(0, 1, (unsigned char *)global_temp_buffer);

    /* bookmark resume index */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer), "%s: %d",
             str(LANG_BOOKMARK_SELECT_INDEX_TEXT), resume_index+1);
    FOR_NB_SCREENS(i)
        screens[i].puts_scroll(0, 2, (unsigned char *)global_temp_buffer);

    /* elapsed time*/
    format_time(time_buf, sizeof(time_buf), ms);
    snprintf(global_temp_buffer, sizeof(global_temp_buffer), "%s: %s",
             str(LANG_BOOKMARK_SELECT_TIME_TEXT), time_buf);
    FOR_NB_SCREENS(i)
        screens[i].puts_scroll(0, 3, (unsigned char *)global_temp_buffer);

    /* commands */
    FOR_NB_SCREENS(i)
    {
        screens[i].puts_scroll(0, 4, str(LANG_BOOKMARK_SELECT_PLAY));
        screens[i].puts_scroll(0, 5, str(LANG_BOOKMARK_SELECT_EXIT));
        screens[i].puts_scroll(0, 6, str(LANG_BOOKMARK_SELECT_DELETE));
        screens[i].update();
    }
#else
    dot = strrchr(global_filename, '.');

    if (dot)
        *dot='\0';

    format_time(time_buf, sizeof(time_buf), ms);
    snprintf(global_temp_buffer, sizeof(global_temp_buffer), 
             "%d/%d, %s, %s", (bookmark_id + 1), bookmark_count, 
             time_buf, global_filename);

    if (dot)
        *dot='.';

    gui_syncstatusbar_draw(&statusbars, false);
    
    FOR_NB_SCREENS(i)
    {
        screens[i].puts_scroll(0,0,global_temp_buffer);
        screens[i].puts(0,1,str(LANG_RESUME_CONFIRM_PLAYER));
        screens[i].update();
    }
#endif
}


/* ----------------------------------------------------------------------- */
/* This function parses a bookmark, says the voice UI part of it.          */
/* ------------------------------------------------------------------------*/
static void say_bookmark(const char* bookmark,
                         int bookmark_id)
{
    int resume_index;
    long ms;
    char dir[MAX_PATH];
    bool enqueue = false; /* only the first voice is not queued */

    parse_bookmark(bookmark,
                   &resume_index, 
                   NULL, NULL, NULL, 
                   dir, sizeof(dir),
                   &ms, NULL, NULL,
                   NULL);
/* disabled, because transition between talkbox and voice UI clip is not nice */
#if 0 
    if (global_settings.talk_dir >= 3)
    {   /* "talkbox" enabled */
        char* last = strrchr(dir, '/');
        if (last)
        {   /* compose filename for talkbox */
            strncpy(last + 1, dir_thumbnail_name, sizeof(dir)-(last-dir)-1);
            talk_file(dir, enqueue);
            enqueue = true;
        }
    }
#endif
    talk_id(VOICE_EXT_BMARK, enqueue);
    talk_number(bookmark_id + 1, true);
    talk_id(LANG_BOOKMARK_SELECT_INDEX_TEXT, true);
    talk_number(resume_index + 1, true);
    talk_id(LANG_BOOKMARK_SELECT_TIME_TEXT, true);
    if (ms / 60000)
        talk_value(ms / 60000, UNIT_MIN, true);
    talk_value((ms % 60000) / 1000, UNIT_SEC, true);
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark and then plays it.                      */
/* ------------------------------------------------------------------------*/
static bool play_bookmark(const char* bookmark)
{
    int index;
    int offset;
    int seed;

    if (parse_bookmark(bookmark,
                       &index,
                       &offset,
                       &seed,
                       NULL,
                       global_temp_buffer,
                       sizeof(global_temp_buffer),
                       NULL,
                       &global_settings.repeat_mode,
                       &global_settings.playlist_shuffle,
                       global_filename))
    {
        bookmark_play(global_temp_buffer, index, offset, seed,
            global_filename);
        return true;
    }
    
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function retrieves a given bookmark from a file.                   */
/* If the bookmark requested is beyond the number of bookmarks available   */
/* in the file, it will return the last one.                               */
/* It also returns the index number of the bookmark in the file            */
/* ------------------------------------------------------------------------*/
static char* get_bookmark(const char* bookmark_file, int bookmark_count)
{
    int read_count = -1;
    int result = 0;
    int file = open(bookmark_file, O_RDONLY);

    if (file < 0)
        return NULL;

    /* Get the requested bookmark */
    while (read_count < bookmark_count)
    {
        /*Reading in a single bookmark */
        result = read_line(file,
                           global_read_buffer,
                           sizeof(global_read_buffer));

        /* Reading past the last bookmark in the file
           causes the loop to stop */
        if (result <= 0)
            break;

        read_count++;
    }

    close(file);
    if ((read_count >= 0) && (read_count == bookmark_count))
        return global_read_buffer;
    else
        return NULL;
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
    if (dest != NULL)
    {
        *dest = atoi(s);
    }
    
    return skip_token(s);
}

static const char* long_token(const char* s, long* dest)
{
    if (dest != NULL)
    {
        *dest = atoi(s);    /* Should be atol, but we don't have it. */
    }

    return skip_token(s);
}

static const char* bool_token(const char* s, bool* dest)
{
    if (dest != NULL)
    {
        *dest = atoi(s) != 0;
    }

    return skip_token(s);
}

/* ----------------------------------------------------------------------- */
/* This function takes a bookmark and parses it.  This function also       */
/* validates the bookmark.  Passing in NULL for an output variable         */
/* indicates that value is not requested.                                  */
/* ----------------------------------------------------------------------- */
static bool parse_bookmark(const char *bookmark,
                           int *resume_index,
                           int *resume_offset,
                           int *resume_seed,
                           int *resume_first_index,
                           char* resume_file,
                           unsigned int resume_file_size,
                           long* ms,
                           int * repeat_mode, bool *shuffle,
                           char* file_name)
{
    const char* s = bookmark;
    const char* end;
    
    s = int_token(s,  resume_index);
    s = int_token(s,  resume_offset);
    s = int_token(s,  resume_seed);
    s = int_token(s,  resume_first_index);
    s = long_token(s, ms);
    s = int_token(s,  repeat_mode);
    s = bool_token(s, shuffle);

    if (*s == 0)
    {
        return false;
    }
    
    end = strchr(s, ';');

    if (resume_file != NULL)
    {
        size_t len = (end == NULL) ? strlen(s) : (size_t) (end - s);

        len = MIN(resume_file_size - 1, len);
        strncpy(resume_file, s, len);
        resume_file[len] = 0;
    }

    if (end != NULL && file_name != NULL)
    {
        end++;
        strncpy(file_name, end, MAX_PATH - 1);
        file_name[MAX_PATH - 1] = 0;
    }
    
    return true;
}

/* ----------------------------------------------------------------------- */
/* This function is used by multiple functions and is used to generate a   */
/* bookmark named based off of the input.                                  */
/* Changing this function could result in how the bookmarks are stored.    */
/* it would be here that the centralized/decentralized bookmark code       */
/* could be placed.                                                        */
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
        strcpy(global_bookmark_file_name, in);
        if(global_bookmark_file_name[len-1] == '/')
            len--;
        strcpy(&global_bookmark_file_name[len], ".bmark");
    }

    return true;
}

/* ----------------------------------------------------------------------- */
/* Returns true if a bookmark file exists for the current playlist         */
/* ----------------------------------------------------------------------- */
bool bookmark_exist(void)
{
    bool exist=false;

    if(system_check())
    {
        char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
        if (generate_bookmark_file_name(name))
        {
            int fd=open(global_bookmark_file_name, O_RDONLY);
            if (fd >=0)
            {
                close(fd);
                exist=true;
            }
        }
    }

    return exist;
}

/* ----------------------------------------------------------------------- */
/* Checks the current state of the system and returns if it is in a        */
/* bookmarkable state.                                                     */
/* ----------------------------------------------------------------------- */
/* Inputs:                                                                 */
/* ----------------------------------------------------------------------- */
/* Outputs:                                                                */
/* return bool:  Indicates if the system was in a bookmarkable state       */
/* ----------------------------------------------------------------------- */
static bool system_check(void)
{
    int resume_index = 0;
    struct mp3entry *id3 = audio_current_track();

    if (!id3)
    {
        /* no track playing */
        return false; 
    }

    /* Checking to see if playing a queued track */
    if (playlist_get_resume_info(&resume_index) == -1)
    {
        /* something bad happened while getting the queue information */
        return false;
    }
    else if (playlist_modified(NULL))
    {
        /* can't bookmark while in the queue */
        return false;
    }

    return true;
}

