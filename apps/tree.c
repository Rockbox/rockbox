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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "applimits.h"
#include "dir.h"
#include "file.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "usb.h"
#include "tree.h"
#include "main_menu.h"
#include "sprintf.h"
#include "mpeg.h"
#include "playlist.h"
#include "menu.h"
#include "wps.h"
#include "settings.h"
#include "status.h"
#include "debug.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

#ifdef LOADABLE_FONTS
#include "ajf.h"
#endif

#define NAME_BUFFER_SIZE (AVERAGE_FILENAME_LENGTH * MAX_FILES_IN_DIR)

char name_buffer[NAME_BUFFER_SIZE];
int name_buffer_length;
struct entry {
    short attr; /* FAT attributes */
    char *name;
};

static struct entry dircache[MAX_FILES_IN_DIR];
static int filesindir;
static char lastdir[MAX_PATH] = {0};

void browse_root(void)
{
    dirbrowse("/");
}


#ifdef HAVE_LCD_BITMAP

#define TREE_MAX_ON_SCREEN   ((LCD_HEIGHT-MARGIN_Y)/LINE_HEIGTH)
#define TREE_MAX_LEN_DISPLAY 16 /* max length that fits on screen */
 
#define MARGIN_Y      (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)  /* Y pixel margin */
#define MARGIN_X      10 /* X pixel margin */
#define LINE_Y      (global_settings.statusbar ? 1 : 0) /* Y position the entry-list starts at */
#define LINE_X      0 /* X position the entry-list starts at */
#define LINE_HEIGTH 8 /* pixels for each text line */

#define CURSOR_Y    0 /* the cursor is not positioned in regard to
                         the margins, so this is the amount of lines
                         we add to the cursor Y position to position
                         it on a line */

extern unsigned char bitmap_icons_6x8[LastIcon][6];

#else /* HAVE_LCD_BITMAP */

#define TREE_MAX_ON_SCREEN   2
#define TREE_MAX_LEN_DISPLAY 11 /* max length that fits on screen */
#define LINE_Y      0 /* Y position the entry-list starts at */
#define LINE_X      1 /* X position the entry-list starts at */

#define CURSOR_Y    0 /* not really used for players */

#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_RECORDER_KEYPAD
#define TREE_NEXT  BUTTON_DOWN
#define TREE_PREV  BUTTON_UP
#define TREE_EXIT  BUTTON_LEFT
#define TREE_ENTER BUTTON_RIGHT
#define TREE_MENU  BUTTON_F1
#define RELEASE_MASK (BUTTON_OFF)
#else
#define TREE_NEXT  BUTTON_RIGHT
#define TREE_PREV  BUTTON_LEFT
#define TREE_EXIT  BUTTON_STOP
#define TREE_ENTER BUTTON_PLAY
#define TREE_MENU  BUTTON_MENU
#define RELEASE_MASK (BUTTON_STOP)
#endif /* HAVE_RECORDER_KEYPAD */

#define TREE_ATTR_M3U 0x80 /* unused by FAT attributes */
#define TREE_ATTR_MP3 0x40 /* unused by FAT attributes */

static int build_playlist(int start_index)
{
    int i;
    int start=start_index;

    playlist_clear();

    for(i = 0;i < filesindir;i++)
    {
        if(dircache[i].attr & TREE_ATTR_MP3)
        {
            DEBUGF("Adding %s\n", dircache[i].name);
            playlist_add(dircache[i].name);
        }
        else
        {
            /* Adjust the start index when se skip non-MP3 entries */
            if(i < start)
                start_index--;
        }
    }

    return start_index;
}

static int compare(const void* p1, const void* p2)
{
    struct entry* e1 = (struct entry*)p1;
    struct entry* e2 = (struct entry*)p2;
    
    if (( e1->attr & ATTR_DIRECTORY ) == ( e2->attr & ATTR_DIRECTORY ))
        if (global_settings.sort_case)
            return strncmp(e1->name, e2->name, MAX_PATH);
        else
            return strncasecmp(e1->name, e2->name, MAX_PATH);
    else 
        return ( e2->attr & ATTR_DIRECTORY ) - ( e1->attr & ATTR_DIRECTORY );
}

static int showdir(char *path, int start)
{
#ifdef HAVE_LCD_BITMAP
    int icon_type = 0;
    int line_height = LINE_HEIGTH;
#endif
    int i;
    int tree_max_on_screen;
    bool dir_buffer_full;
#ifdef LOADABLE_FONTS
    int fh;
    unsigned char *font = lcd_getcurrentldfont();
    fh  = ajf_get_fontheight(font);
    tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
    line_height = fh;
#else
    tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif


    /* new dir? cache it */
    if (strncmp(path,lastdir,sizeof(lastdir))) {
        DIR *dir = opendir(path);
        if(!dir)
            return -1; /* not a directory */

        name_buffer_length = 0;
        dir_buffer_full = false;
        
        for ( i=0; i<MAX_FILES_IN_DIR; i++ ) {
            int len;
            struct dirent *entry = readdir(dir);
            struct entry* dptr = &dircache[i];
            if (!entry)
                break;

            len = strlen(entry->d_name);

            /* skip directories . and .. */
            if ((entry->attribute & ATTR_DIRECTORY) &&
                (((len == 1) && 
                  (!strncmp(entry->d_name, ".", 1))) ||
                 ((len == 2) && 
                  (!strncmp(entry->d_name, "..", 2))))) {
                i--;
                continue;
            }

            /* Skip dotfiles if set to skip them */
            if (!global_settings.show_hidden_files &&
                ((entry->d_name[0]=='.') ||
                 (entry->attribute & ATTR_HIDDEN))) {
                i--;
                continue;
            }

            dptr->attr = entry->attribute;

            /* mark mp3 and m3u files as such */
            if ( !(dptr->attr & ATTR_DIRECTORY) && (len > 4) ) {
                if (!strcasecmp(&entry->d_name[len-4], ".mp3"))
                    dptr->attr |= TREE_ATTR_MP3;
                else
                    if (!strcasecmp(&entry->d_name[len-4], ".m3u"))
                        dptr->attr |= TREE_ATTR_M3U;
            }

            /* filter non-mp3 or m3u files */
            if ( global_settings.mp3filter &&
                 (!(dptr->attr &
                    (ATTR_DIRECTORY|TREE_ATTR_MP3|TREE_ATTR_M3U))) ) {
                i--;
                continue;
            }

            if(len > NAME_BUFFER_SIZE - name_buffer_length - 1) {
                /* Tell the world that we ran out of buffer space */
                dir_buffer_full = true;
                break;
            }
            dptr->name = &name_buffer[name_buffer_length];
            strcpy(dptr->name,entry->d_name);
            name_buffer_length += len + 1;
        }
        filesindir = i;
        closedir(dir);
        strncpy(lastdir,path,sizeof(lastdir));
        lastdir[sizeof(lastdir)-1] = 0;
        qsort(dircache,filesindir,sizeof(struct entry),compare);

        if ( dir_buffer_full || filesindir == MAX_FILES_IN_DIR ) {
#ifdef HAVE_NEW_CHARCELL_LCD
            lcd_double_height(false);
#endif
            lcd_clear_display();
            lcd_puts(0,0,"Dir buffer");
            lcd_puts(0,1,"is full!");
            lcd_update();
            sleep(HZ*2);
            lcd_clear_display();
        }
    }

    lcd_stop_scroll();
#ifdef HAVE_NEW_CHARCELL_LCD
    lcd_double_height(false);
#endif
    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and icon */
    lcd_setfont(0);
#endif

    for ( i=start; i < start+tree_max_on_screen; i++ ) {
        int len;

        if ( i >= filesindir )
            break;

        len = strlen(dircache[i].name);

#ifdef HAVE_LCD_BITMAP
        if ( dircache[i].attr & ATTR_DIRECTORY )
            icon_type = Folder;
        else {
            if ( dircache[i].attr & TREE_ATTR_M3U )
                icon_type = Playlist;
            else
                icon_type = File;
        }
        lcd_bitmap(bitmap_icons_6x8[icon_type], 
                   4, MARGIN_Y+(i-start)*line_height, 6, 8, true);
#endif


        /* if MP3 filter is on, cut off the extension */
        if (global_settings.mp3filter && 
            (dircache[i].attr & (TREE_ATTR_M3U|TREE_ATTR_MP3)))
        {
            char temp = dircache[i].name[len-4];
            dircache[i].name[len-4] = 0;
            lcd_puts(LINE_X, i-start, dircache[i].name);
            dircache[i].name[len-4] = temp;
        }
        else
            lcd_puts(LINE_X, i-start, dircache[i].name);
    }

    status_draw();
    return filesindir;
}

bool ask_resume(void)
{
#ifdef HAVE_NEW_CHARCELL_LCD
    lcd_double_height(false);
#endif

    /* always resume? */
    if ( global_settings.resume == RESUME_ON )
        return true;

    lcd_clear_display();
    lcd_puts(0,0,"Resume?");
#ifdef HAVE_LCD_CHARCELLS
    lcd_puts(0,1,"(Play/Stop)");
#else
    lcd_puts(0,1,"Play = Yes");
    lcd_puts(0,2,"Any other = No");
#endif
    lcd_update();
    if (button_get(true) == BUTTON_PLAY)
        return true;
    return false;
}

void start_resume(void)
{
    if ( global_settings.resume &&
         global_settings.resume_index != -1 ) {
        int len = strlen(global_settings.resume_file);

        DEBUGF("Resume file %s\n",global_settings.resume_file);
        DEBUGF("Resume index %X offset %X\n",
               global_settings.resume_index,
               global_settings.resume_offset);
        DEBUGF("Resume shuffle %s seed %X\n",
               global_settings.playlist_shuffle?"on":"off",
               global_settings.resume_seed);

        /* playlist? */
        if (!strcasecmp(&global_settings.resume_file[len-4], ".m3u")) {
            char* slash;

            /* check that the file exists */
            int fd = open(global_settings.resume_file, O_RDONLY);
            if(fd<0)
                return;
            close(fd);

            if (!ask_resume())
                return;
            
            slash = strrchr(global_settings.resume_file,'/');
            if (slash) {
                *slash=0;
                play_list(global_settings.resume_file,
                          slash+1,
                          global_settings.resume_index,
                          true, /* the index is AFTER shuffle */
                          global_settings.resume_offset,
                          global_settings.resume_seed );
                *slash='/';
            }
            else {
                /* check that the dir exists */
                DIR* dir = opendir(global_settings.resume_file);
                if(!dir)
                    return;
                closedir(dir);

                if (!ask_resume())
                    return;

                play_list("/",
                          global_settings.resume_file,
                          global_settings.resume_index,
                          true,
                          global_settings.resume_offset,
                          global_settings.resume_seed );
            }
        }
        else {
            if (!ask_resume())
                return;

            if (showdir(global_settings.resume_file, 0) < 0 )
                return;
            build_playlist(global_settings.resume_index);
            play_list(global_settings.resume_file,
                      NULL, 
                      global_settings.resume_index,
                      true,
                      global_settings.resume_offset,
                      global_settings.resume_seed);
        }

        status_set_playmode(STATUS_PLAY);
        status_draw();
        wps_show();
    }
}

bool dirbrowse(char *root)
{
    int numentries=0;
    int dircursor=0;
    int start=0;
    int dirpos[MAX_DIR_LEVELS];
    int cursorpos[MAX_DIR_LEVELS];
    int dirlevel=0;
    char currdir[MAX_PATH];
    char buf[MAX_PATH];
    int i;
    int lasti=-1;
    int rc;
    int button;
    int start_index;
    int tree_max_on_screen;
#ifdef LOADABLE_FONTS
    int fh;
    unsigned char *font = lcd_getcurrentldfont();
    fh  = ajf_get_fontheight(font);
    tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#else
    tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif

    start_resume();
    button_set_release(RELEASE_MASK);

    memcpy(currdir,root,sizeof(currdir));
    numentries = showdir(root, start);
    if (numentries == -1) 
        return -1;  /* root is not a directory */

    put_cursorxy(0, CURSOR_Y + dircursor, true);

    while(1) {
        bool restore = false;

        button = button_get_w_tmo(HZ/5);
        switch ( button ) {
            case TREE_EXIT:
                i=strlen(currdir);
                if (i>1) {
                    while (currdir[i-1]!='/')
                        i--;
                    strcpy(buf,&currdir[i]);
                    if (i==1)
                        currdir[i]=0;
                    else
                        currdir[i-1]=0;

                    dirlevel--;
                    if ( dirlevel < MAX_DIR_LEVELS ) {
                        start = dirpos[dirlevel];
                        dircursor = cursorpos[dirlevel];
                    }
                    else
                        start = dircursor = 0;
                    restore = true;
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                status_draw();
                restore = true;
                break;

            case BUTTON_OFF | BUTTON_REL:
#else
            case BUTTON_STOP | BUTTON_REL:
#endif
                global_settings.resume_index = -1;
                settings_save();
                break;
                

            case TREE_ENTER:
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_PLAY:
#endif
                if ( !numentries )
                    break;
                if ((currdir[0]=='/') && (currdir[1]==0)) {
                    snprintf(buf,sizeof(buf),"%s%s",currdir,
                             dircache[dircursor+start].name);
                } else {
                    snprintf(buf,sizeof(buf),"%s/%s",currdir,
                             dircache[dircursor+start].name);
                }

                if (dircache[dircursor+start].attr & ATTR_DIRECTORY) {
                    memcpy(currdir,buf,sizeof(currdir));
                    if ( dirlevel < MAX_DIR_LEVELS ) {
                        dirpos[dirlevel] = start;
                        cursorpos[dirlevel] = dircursor;
                    }
                    dirlevel++;
                    dircursor=0;
                    start=0;
                } else {
                    int seed = current_tick;
                    lcd_stop_scroll();
                    if(dircache[dircursor+start].attr & TREE_ATTR_M3U )
                    {
                        if ( global_settings.resume )
                            snprintf(global_settings.resume_file,
                                     MAX_PATH, "%s/%s",
                                     currdir,
                                     dircache[dircursor+start].name);
                        play_list(currdir,
                                  dircache[dircursor+start].name,
                                  0,
                                  false,
                                  0, seed );
                        start_index = 0;
                    }
                    else {
                        if ( global_settings.resume )
                            strncpy(global_settings.resume_file,
                                    currdir, MAX_PATH);
                        start_index = build_playlist(dircursor+start);

                        /* it is important that we get back the index in
                           the (shuffled) list and stor that */
                        start_index = play_list(currdir, NULL,
                                                start_index, false, 0, seed);
                    }

                    if ( global_settings.resume ) {
                        /* the resume_index must always be the index in the
                           shuffled list in case shuffle is enabled */
                        global_settings.resume_index = start_index;
                        global_settings.resume_offset = 0;
                        global_settings.resume_seed = seed;
                        settings_save();
                    }

                    status_set_playmode(STATUS_PLAY);
                    status_draw();
                    lcd_stop_scroll();
                    rc = wps_show();
                    if(rc == SYS_USB_CONNECTED)
                    {
                        /* Force a re-read of the root directory */
                        strcpy(currdir, "/");
                        lastdir[0] = 0;
                        dirlevel = 0;
                        dircursor = 0;
                        start = 0;
                        global_settings.resume_index = -1;
                    }
                }
                restore = true;
                break;

            case TREE_PREV:
            case TREE_PREV | BUTTON_REPEAT:
            case BUTTON_VOL_UP:
                if(filesindir) {
                    if(dircursor) {
                        put_cursorxy(0, CURSOR_Y + dircursor, false);
                        dircursor--;
                        put_cursorxy(0, CURSOR_Y + dircursor, true);
                    }
                    else {
                        if (start) {
                            start--;
                            numentries = showdir(currdir, start);
                            put_cursorxy(0, CURSOR_Y + dircursor, true);
                        }
                        else {
                            if (numentries < tree_max_on_screen) {
                                put_cursorxy(0, CURSOR_Y + dircursor,
                                             false);
                                dircursor = numentries - 1;
                                put_cursorxy(0, CURSOR_Y + dircursor,
                                             true);
                            }
                            else {
                                start = numentries - tree_max_on_screen;
                                dircursor = tree_max_on_screen - 1;
                                numentries = showdir(currdir, start);
                                put_cursorxy(0, CURSOR_Y +
                                             tree_max_on_screen - 1, true);
                            }
                        }
                    }
                    lcd_update();
                }
                break;

            case TREE_NEXT:
            case TREE_NEXT | BUTTON_REPEAT:
            case BUTTON_VOL_DOWN:
                if(filesindir)
                {
                    if (dircursor + start + 1 < numentries ) {
                        if(dircursor+1 < tree_max_on_screen) {
                            put_cursorxy(0, CURSOR_Y + dircursor,
                                         false);
                            dircursor++;
                            put_cursorxy(0, CURSOR_Y + dircursor, true);
                        } 
                        else {
                            start++;
                            numentries = showdir(currdir, start);
                            put_cursorxy(0, CURSOR_Y + dircursor, true);
                        }
                    }
                    else {
                        if(numentries < tree_max_on_screen) {
                            put_cursorxy(0, CURSOR_Y + dircursor,
                                         false);
                            start = dircursor = 0;
                            put_cursorxy(0, CURSOR_Y + dircursor, true);
                        } 
                        else {
                            start = dircursor = 0;
                            numentries = showdir(currdir, start);
                            put_cursorxy(0, CURSOR_Y + dircursor, true);
                        }
                    }
                    lcd_update();
                }
                break;

            case TREE_MENU: {
                bool lastfilter = global_settings.mp3filter;
                bool lastsortcase = global_settings.sort_case;
                bool show_hidden_files = global_settings.show_hidden_files;

                lcd_stop_scroll();
                main_menu();
                /* do we need to rescan dir? */
                if ( lastfilter != global_settings.mp3filter ||
                     lastsortcase != global_settings.sort_case ||
                     show_hidden_files != global_settings.show_hidden_files)
                    lastdir[0] = 0;
                restore = true;
                break;
            }

            case BUTTON_ON:
                if (mpeg_is_playing())
                {
                    lcd_stop_scroll();
                    rc = wps_show();
                    if(rc == SYS_USB_CONNECTED)
                    {
                        /* Force a re-read of the root directory */
                        strcpy(currdir, "/");
                        lastdir[0] = 0;
                        dirlevel = 0;
                        dircursor = 0;
                        start = 0;
                    }
                    restore = true;
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F3:
#ifdef HAVE_LCD_BITMAP
                global_settings.statusbar = !global_settings.statusbar;
                settings_save();
#ifdef LOADABLE_FONTS
                tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#else
                tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif
                restore = true;
#endif
                break;
#endif

#ifndef SIMULATOR
            case SYS_USB_CONNECTED: {
#ifdef HAVE_LCD_BITMAP
                bool laststate=statusbar(false);
#endif
                /* Tell the USB thread that we are safe */
                DEBUGF("dirbrowse got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                
                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&button_queue);
                
                /* Force a re-read of the root directory */
                restore = true;
                strcpy(currdir, "/");
                lastdir[0] = 0;
                dirlevel = 0;
                dircursor = 0;
                start = 0;
#ifdef HAVE_LCD_BITMAP
                statusbar(laststate);
#endif
            }
            break;
#endif
        }

        if ( restore ) {
            /* restore display */
            /* We need to adjust if the number of lines on screen have
               changed because of a status bar change */
            if(CURSOR_Y+LINE_Y+dircursor>tree_max_on_screen) {
                start++;
                dircursor--;
            }
            numentries = showdir(currdir, start);
            put_cursorxy(0, CURSOR_Y + dircursor, true);
        }

        if ( numentries ) {
            i = start+dircursor;

            /* if MP3 filter is on, cut off the extension */
            if(lasti!=i || restore) {
                lasti=i;
                lcd_stop_scroll();
                if (global_settings.mp3filter && 
                    (dircache[i].attr &
                     (TREE_ATTR_M3U|TREE_ATTR_MP3)))
                {
                    int len = strlen(dircache[i].name);
                    char temp = dircache[i].name[len-4];
                    dircache[i].name[len-4] = 0;
                    lcd_puts_scroll(LINE_X, dircursor, 
                                    dircache[i].name);
                    dircache[i].name[len-4] = temp;
                }
                else
                    lcd_puts_scroll(LINE_X, dircursor,
                                    dircache[i].name);
            }
        }
        status_draw();
        lcd_update();

    }

    return false;
}
