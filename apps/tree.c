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
#include "tree.h"
#include "play.h"
#include "main_menu.h"
#include "sprintf.h"
#include "mpeg.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

#define MAX_FILES_IN_DIR 200
#define TREE_MAX_FILENAMELEN 128
#define MAX_DIR_LEVELS 10

struct entry {
  bool file; /* true if file, false if dir */
  char name[TREE_MAX_FILENAMELEN];
};

static struct entry dircache[MAX_FILES_IN_DIR];
static struct entry* dircacheptr[MAX_FILES_IN_DIR];
static int filesindir;

void browse_root(void)
{
  dirbrowse("/");
}


#ifdef HAVE_LCD_BITMAP

#define TREE_MAX_ON_SCREEN   7
#define TREE_MAX_LEN_DISPLAY 16 /* max length that fits on screen */
 
#define MARGIN_Y      8  /* Y pixel margin */
#define MARGIN_X      12 /* X pixel margin */
#define LINE_Y      0 /* Y position the entry-list starts at */
#define LINE_X      2 /* X position the entry-list starts at */
#define LINE_HEIGTH 8 /* pixels for each text line */

extern unsigned char bitmap_icons_6x8[LastIcon][6];

#else /* HAVE_LCD_BITMAP */

#define TREE_MAX_ON_SCREEN   2
#define TREE_MAX_LEN_DISPLAY 11 /* max length that fits on screen */
#define LINE_Y      0 /* Y position the entry-list starts at */
#define LINE_X      1 /* X position the entry-list starts at */

#endif /* HAVE_LCD_BITMAP */

static int compare(const void* e1, const void* e2)
{
    return strncmp((*(struct entry**)e1)->name, (*(struct entry**)e2)->name,
                   TREE_MAX_FILENAMELEN);
}

static int showdir(char *path, int start)
{
    static char lastdir[256] = {0};

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
            struct dirent *entry = readdir(dir);
            if (!entry)
                break;
            if(entry->d_name[0] == '.') {
                /* skip names starting with a dot */
                i--;
                continue;
            }
            dircache[i].file = !(entry->attribute & ATTR_DIRECTORY);
            strncpy(dircache[i].name,entry->d_name,TREE_MAX_FILENAMELEN);
            dircache[i].name[TREE_MAX_FILENAMELEN-1]=0;
            dircacheptr[i] = &dircache[i];
        }
        filesindir = i;
        closedir(dir);
        strncpy(lastdir,path,sizeof(lastdir));
        lastdir[sizeof(lastdir)-1] = 0;
        qsort(dircacheptr,filesindir,sizeof(struct entry*),compare);
    }

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_putsxy(0,0, "[Browse]",0);
    lcd_update();
#endif

    for ( i=start; i < start+TREE_MAX_ON_SCREEN; i++ ) {
        int len;

        if ( i >= filesindir )
            break;

        len = strlen(dircacheptr[i]->name);

#ifdef HAVE_LCD_BITMAP
        if ( dircacheptr[i]->file )
            icon_type=File;
        else
            icon_type=Folder;
        lcd_bitmap(bitmap_icons_6x8[icon_type], 
                   6, MARGIN_Y+(i-start)*LINE_HEIGTH, 6, 8, true);
#endif

        if(len < TREE_MAX_LEN_DISPLAY)
            lcd_puts(LINE_X, LINE_Y+i-start, dircacheptr[i]->name);
        else {
            char storage = dircacheptr[i]->name[TREE_MAX_LEN_DISPLAY];
            dircacheptr[i]->name[TREE_MAX_LEN_DISPLAY]=0;
            lcd_puts(LINE_X, LINE_Y+i-start, dircacheptr[i]->name);
            dircacheptr[i]->name[TREE_MAX_LEN_DISPLAY]=storage;
        }
    }

    return filesindir;
}

bool dirbrowse(char *root)
{
    int numentries;
    char buf[255];
    char currdir[255];
    int dircursor=0;
    int i;
    int start=0;
    int dirpos[MAX_DIR_LEVELS];
    int dirlevel=0;

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_putsxy(0,0, "[Browse]",0);
    lcd_setmargins(0,MARGIN_Y);
    lcd_setfont(0);
#endif
    memcpy(currdir,root,sizeof(currdir));

    numentries = showdir(root, start);

    if (numentries == -1) 
        return -1;  /* root is not a directory */

    lcd_puts(0, dircursor, "-");
#ifdef HAVE_LCD_BITMAP
    lcd_update();
#endif

    while(1) {
        switch(button_get(true)) {
#if defined(SIMULATOR) && defined(HAVE_RECODER_KEYPAD)
            case BUTTON_OFF:
                return false;
#endif

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
#endif
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
                    if ( dirlevel < MAX_DIR_LEVELS )
                        start = dirpos[dirlevel];
                    else
                        start = 0;
                    numentries = showdir(currdir, start);
                    dircursor=0;
                    while ( (dircursor < TREE_MAX_ON_SCREEN) &&
                            (strcmp(dircacheptr[dircursor+start]->name,buf)))
                        dircursor++;
                    if (dircursor==TREE_MAX_ON_SCREEN)
                        dircursor=0;
                    lcd_puts(0, LINE_Y+dircursor, "-");
                }
                else
                    mpeg_stop();

                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
#endif
            case BUTTON_PLAY:
                if ((currdir[0]=='/') && (currdir[1]==0)) {
                    snprintf(buf,sizeof(buf),"%s%s",currdir,
                             dircacheptr[dircursor+start]->name);
                } else {
                    snprintf(buf,sizeof(buf),"%s/%s",currdir,
                             dircacheptr[dircursor+start]->name);
                }

                if (!dircacheptr[dircursor+start]->file) {
                    memcpy(currdir,buf,sizeof(currdir));
                    if ( dirlevel < MAX_DIR_LEVELS )
                        dirpos[dirlevel] = start+dircursor;
                    dirlevel++;
                    dircursor=0;
                    start=0;
                } else {
                    playtune(currdir, dircacheptr[dircursor+start]->name);
#ifdef HAVE_LCD_BITMAP
                    lcd_setmargins(0, MARGIN_Y);
                    lcd_setfont(0);
#endif
                }

                numentries = showdir(currdir, start);  
                lcd_puts(0, LINE_Y+dircursor, "-");
                break;
                
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_LEFT:
#endif
                if(dircursor) {
                    lcd_puts(0, LINE_Y+dircursor, " ");
                    dircursor--;
                    lcd_puts(0, LINE_Y+dircursor, "-");
                    lcd_update();
                }
                else {
                    if (start) {
                        start--;
                        numentries = showdir(currdir, start);
                        lcd_puts(0, LINE_Y+dircursor, "-");
                    }
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_RIGHT:
#endif
                if (dircursor + start + 1 < numentries ) {
                    if(dircursor+1 < TREE_MAX_ON_SCREEN) {
                        lcd_puts(0, LINE_Y+dircursor, " ");
                        dircursor++;
                        lcd_puts(0, LINE_Y+dircursor, "-");
                    } 
                    else {
                        start++;
                        numentries = showdir(currdir, start);
                        lcd_puts(0, LINE_Y+dircursor, "-");
                    }
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1:
            case BUTTON_F2:
            case BUTTON_F3:
#else
            case BUTTON_MENU:
#endif
                main_menu();

                /* restore display */
                /* TODO: this is just a copy from BUTTON_STOP, fix it */
                lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
                lcd_putsxy(0,0, "[Browse]",0);
                lcd_setmargins(0,MARGIN_Y);
                lcd_setfont(0);
#endif
                numentries = showdir(currdir, start);
                dircursor=0;
                while ( (dircursor < TREE_MAX_ON_SCREEN) && 
                        (strcmp(dircacheptr[dircursor+start]->name,buf))) 
                    dircursor++;
                if (dircursor==TREE_MAX_ON_SCREEN)
                    dircursor=0;
                lcd_puts(0, LINE_Y+dircursor, "-");

                break;
        }
        lcd_update();
    }

    return false;
}
