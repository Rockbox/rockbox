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
#include "debug.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

#define MAX_FILES_IN_DIR 200
#define TREE_MAX_FILENAMELEN MAX_PATH
#define MAX_DIR_LEVELS 10

struct entry {
    char attr; /* FAT attributes */
    char name[TREE_MAX_FILENAMELEN];
};

static struct entry dircache[MAX_FILES_IN_DIR];
static struct entry* dircacheptr[MAX_FILES_IN_DIR];
static int filesindir;
static char lastdir[MAX_PATH] = {0};

void browse_root(void)
{
  dirbrowse("/");
}


#ifdef HAVE_LCD_BITMAP

#define TREE_MAX_ON_SCREEN   ((LCD_HEIGHT-MARGIN_Y)/LINE_HEIGTH)
#define TREE_MAX_LEN_DISPLAY 16 /* max length that fits on screen */
 
#define MARGIN_Y      0  /* Y pixel margin */
#define MARGIN_X      12 /* X pixel margin */
#define LINE_Y      0 /* Y position the entry-list starts at */
#define LINE_X      2 /* X position the entry-list starts at */
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
#else
#define TREE_NEXT  BUTTON_RIGHT
#define TREE_PREV  BUTTON_LEFT
#define TREE_EXIT  BUTTON_STOP
#define TREE_ENTER BUTTON_PLAY
#define TREE_MENU  BUTTON_MENU
#endif /* HAVE_RECORDER_KEYPAD */

#define TREE_ATTR_M3U 0x80 /* unused by FAT attributes */
#define TREE_ATTR_MP3 0x40 /* unused by FAT attributes */

static int compare(const void* p1, const void* p2)
{
    struct entry* e1 = *(struct entry**)p1;
    struct entry* e2 = *(struct entry**)p2;
    
    if (( e1->attr & ATTR_DIRECTORY ) == ( e2->attr & ATTR_DIRECTORY ))
        if (global_settings.sort_case)
            return strncmp(e1->name, e2->name, TREE_MAX_FILENAMELEN);
        else
            return strncasecmp(e1->name, e2->name, TREE_MAX_FILENAMELEN);
    else 
        return ( e2->attr & ATTR_DIRECTORY ) - ( e1->attr & ATTR_DIRECTORY );
}

static int showdir(char *path, int start)
{
#ifdef HAVE_LCD_BITMAP
    int icon_type = 0;
#endif
    int i;

    /* new dir? cache it */
    if (strncmp(path,lastdir,sizeof(lastdir))) {
        DIR *dir = opendir(path);
        if(!dir)
            return -1; /* not a directory */

        memset(dircacheptr,0,sizeof(dircacheptr));
        for ( i=0; i<MAX_FILES_IN_DIR; i++ ) {
            int len;
            struct dirent *entry = readdir(dir);
            struct entry* dptr = &dircache[i];
            if (!entry)
                break;

            /* skip directories . and .. */
            if ((entry->attribute & ATTR_DIRECTORY) &&
                (!strncmp(entry->d_name, ".", 1) ||
                 !strncmp(entry->d_name, "..", 2))) {
                i--;
                continue;
            }
            dptr->attr = entry->attribute;
            len = strlen(entry->d_name);

            /* mark mp3 and m3u files as such */
            if ( !(dptr->attr & ATTR_DIRECTORY) && (len > 4) ) {
                if (!strcasecmp(&entry->d_name[len-4], ".mp3"))
                    dptr->attr |= TREE_ATTR_MP3;
                else
                    if (!strcasecmp(&entry->d_name[len-4], ".m3u"))
                        dptr->attr |= TREE_ATTR_M3U;
            }

            /* filter hidden files and directories and non-mp3 or m3u files */
            if ( global_settings.mp3filter &&
                 ((dptr->attr & ATTR_HIDDEN) ||
                  !(dptr->attr & (ATTR_DIRECTORY|TREE_ATTR_MP3|TREE_ATTR_M3U))) ) {
                i--;
                continue;
            }

            strncpy(dptr->name,entry->d_name,TREE_MAX_FILENAMELEN);
            dptr->name[TREE_MAX_FILENAMELEN-1]=0;
            dircacheptr[i] = dptr;
        }
        filesindir = i;
        closedir(dir);
        strncpy(lastdir,path,sizeof(lastdir));
        lastdir[sizeof(lastdir)-1] = 0;
        qsort(dircacheptr,filesindir,sizeof(struct entry*),compare);
    }

    lcd_stop_scroll();
#ifdef HAVE_NEW_CHARCELL_LCD
    lcd_double_height(false);
#endif
    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,MARGIN_Y);
    lcd_setfont(0);
#endif

    for ( i=start; i < start+TREE_MAX_ON_SCREEN; i++ ) {
        int len;

        if ( i >= filesindir )
            break;

        len = strlen(dircacheptr[i]->name);

#ifdef HAVE_LCD_BITMAP
        if ( dircacheptr[i]->attr & ATTR_DIRECTORY )
            icon_type = Folder;
        else {
            if ( dircacheptr[i]->attr & TREE_ATTR_M3U )
                icon_type = Playlist;
            else
                icon_type = File;
        }
        lcd_bitmap(bitmap_icons_6x8[icon_type], 
                   6, MARGIN_Y+(i-start)*LINE_HEIGTH, 6, 8, true);
#endif

        /* if MP3 filter is on, cut off the extension */
        if (global_settings.mp3filter && 
            (dircacheptr[i]->attr & (TREE_ATTR_M3U|TREE_ATTR_MP3)))
        {
            char temp = dircacheptr[i]->name[len-4];
            dircacheptr[i]->name[len-4] = 0;
            lcd_puts(LINE_X, LINE_Y+i-start, dircacheptr[i]->name);
            dircacheptr[i]->name[len-4] = temp;
        }
        else
            lcd_puts(LINE_X, LINE_Y+i-start, dircacheptr[i]->name);
    }

    return filesindir;
}

static int numentries=0;
static int dircursor=0;
static int start=0;
static int dirpos[MAX_DIR_LEVELS];
static int cursorpos[MAX_DIR_LEVELS];
static int dirlevel=0;
static int play_mode = 0;
static char currdir[MAX_PATH];

/* QUICK HACK! this should be handled by the playlist code later */
char* peek_next_track(int steps)
{
    static char buf[MAX_PATH];

    switch(play_mode) {
        case 1:
            /* play-full-dir mode */
      
            /* get next track in dir */
            if ( steps == 1 ) {
                while (dircursor + start + 1 < numentries ) {
                    if(dircursor+1 < TREE_MAX_ON_SCREEN)
                        dircursor++;
                    else
                        start++;
                    if ( dircacheptr[dircursor+start]->attr & TREE_ATTR_MP3 ) {
                        snprintf(buf,sizeof buf,"%s/%s",
                                 currdir, dircacheptr[dircursor+start]->name );
                        return buf;
                    }
                }
            }
            else {
                while (dircursor + start > 0) {
                    if (dircursor > 0)
                        dircursor--;
                    else
                        start--;
                    if ( dircacheptr[dircursor+start]->attr & TREE_ATTR_MP3 ) {
                        snprintf(buf, sizeof(buf), "%s/%s",
                                 currdir, dircacheptr[dircursor+start]->name);
                        return buf;
                    }
                }
            }
            break;
      
        case 2:
            /* playlist mode */
            return playlist_next(steps, currdir);
    }

    return NULL;
}

bool dirbrowse(char *root)
{
    char buf[MAX_PATH];
    int i;
    int rc;

    memcpy(currdir,root,sizeof(currdir));
    numentries = showdir(root, start);
    if (numentries == -1) 
        return -1;  /* root is not a directory */

    put_cursorxy(0, CURSOR_Y + dircursor, true);

    while(1) {
        bool restore = false;

        if ( numentries ) {
            i = start+dircursor;
            
            /* if MP3 filter is on, cut off the extension */
            if (global_settings.mp3filter && 
                (dircacheptr[i]->attr &
                 (TREE_ATTR_M3U|TREE_ATTR_MP3)))
            {
                int len = strlen(dircacheptr[i]->name);
                char temp = dircacheptr[i]->name[len-4];
                dircacheptr[i]->name[len-4] = 0;
                lcd_puts_scroll(LINE_X, LINE_Y+dircursor, 
                                dircacheptr[i]->name);
                dircacheptr[i]->name[len-4] = temp;
            }
            else
                lcd_puts_scroll(LINE_X, LINE_Y+dircursor,
                                dircacheptr[i]->name);
        }
        lcd_update();

        switch ( button_get(true) ) {
            case TREE_EXIT:
                if ( play_mode == 1 )
                    play_mode = 0;
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
                else {
                    mpeg_stop();
                    play_mode = 0;
                }
                break;

            case TREE_ENTER:
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_PLAY:
#endif
                if ( !numentries )
                    break;
                if ((currdir[0]=='/') && (currdir[1]==0)) {
                    snprintf(buf,sizeof(buf),"%s%s",currdir,
                             dircacheptr[dircursor+start]->name);
                } else {
                    snprintf(buf,sizeof(buf),"%s/%s",currdir,
                             dircacheptr[dircursor+start]->name);
                }

                if (dircacheptr[dircursor+start]->attr & ATTR_DIRECTORY) {
                    if ( play_mode == 1 )
                        play_mode = 0;
                    memcpy(currdir,buf,sizeof(currdir));
                    if ( dirlevel < MAX_DIR_LEVELS ) {
                      dirpos[dirlevel] = start;
                      cursorpos[dirlevel] = dircursor;
                    }
                    dirlevel++;
                    dircursor=0;
                    start=0;
                } else {
                    lcd_stop_scroll();
                    if(dircacheptr[dircursor+start]->attr & TREE_ATTR_M3U )
                    {
                        play_mode = 2;
                        play_list(currdir, dircacheptr[dircursor+start]->name);
                    }
                    else {
                        play_mode = 1;
                        mpeg_play(buf);
                    }
                    lcd_stop_scroll();
                    rc = wps_show();
                    if(rc == SYS_USB_CONNECTED)
                    {
                        /* Force a re-read of the root directory */
                        strcpy(currdir, "/");
                        lastdir[0] = 0;
                    }
                }
                restore = true;
                break;
                
            case TREE_PREV:
		if(filesindir)
		{
		    if(dircursor) {
			put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, false);
			dircursor--;
			put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
		    }
		    else {
			if (start) {
			    start--;
			    numentries = showdir(currdir, start);
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
			}
			else {
			    if (numentries < TREE_MAX_ON_SCREEN) {
				put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor,
					     false);
				dircursor = numentries - 1;
				put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor,
					     true);
			    }
			    else {
				start = numentries - TREE_MAX_ON_SCREEN;
				dircursor = TREE_MAX_ON_SCREEN - 1;
				numentries = showdir(currdir, start);
				put_cursorxy(0, CURSOR_Y + LINE_Y +
					     TREE_MAX_ON_SCREEN - 1, true);
			    }
			}
		    }
		    lcd_update();
		}
                break;

            case TREE_NEXT:
		if(filesindir)
		{
		    if (dircursor + start + 1 < numentries ) {
			if(dircursor+1 < TREE_MAX_ON_SCREEN) {
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor,
					 false);
			    dircursor++;
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
			} 
			else {
			    start++;
			    numentries = showdir(currdir, start);
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
			}
		    }
		    else {
			if(numentries < TREE_MAX_ON_SCREEN) {
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor,
					 false);
			    start = dircursor = 0;
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
			} 
			else {
			    start = dircursor = 0;
			    numentries = showdir(currdir, start);
			    put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
			}
		    }
		    lcd_update();
		}
                break;

            case TREE_MENU: {
                bool lastfilter = global_settings.mp3filter;
                bool lastsortcase = global_settings.sort_case;
                lcd_stop_scroll();
                main_menu();
                /* do we need to rescan dir? */
                if ( lastfilter != global_settings.mp3filter ||
                     lastsortcase != global_settings.sort_case)
                    lastdir[0] = 0;
                restore = true;
                break;
            }

            case BUTTON_ON:
                if ( play_mode ) {
                    lcd_stop_scroll();
                    rc = wps_show();
                    if(rc == SYS_USB_CONNECTED)
                    {
                        /* Force a re-read of the root directory */
                        strcpy(currdir, "/");
                        lastdir[0] = 0;
                    }
                    restore = true;
                }
                break;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("dirbrowse got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                
                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&button_queue);
                
                /* Force a re-read of the root directory */
                restore = true;
                strcpy(currdir, "/");
                lastdir[0] = 0;
                break;
#endif
        }

        if ( restore ) {
            /* restore display */
            numentries = showdir(currdir, start);
            put_cursorxy(0, CURSOR_Y + LINE_Y+dircursor, true);
        }

        lcd_stop_scroll();
    }

    return false;
}
