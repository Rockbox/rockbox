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

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

#define TREE_MAX_FILENAMELEN 128
#define MAX_DIR_LEVELS 10

struct entry {
  bool file; /* true if file, false if dir */
  char name[TREE_MAX_FILENAMELEN];
  int namelen;
};

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
extern icons_6x8;

#else /* HAVE_LCD_BITMAP */

#define TREE_MAX_ON_SCREEN   2
#define TREE_MAX_LEN_DISPLAY 11 /* max length that fits on screen */
#define LINE_Y      0 /* Y position the entry-list starts at */
#define LINE_X      1 /* X position the entry-list starts at */

#endif /* HAVE_LCD_BITMAP */

static int showdir(char *path, struct entry *buffer, int start, 
                   int scrollpos, int* at_end)
{
#ifdef HAVE_LCD_BITMAP
    int icon_type = 0;
#endif
    int i;
    int j=0;
    DIR *dir = opendir(path);
    struct dirent *entry;

    if(!dir)
        return -1; /* not a directory */

    i=start;
    *at_end=0; /* Have we displayed the last directory entry? */
    while((entry = readdir(dir))) {
        int len;

        if(entry->d_name[0] == '.')
            /* skip names starting with a dot */
            continue;

        if(j++ < scrollpos) 
            continue ;

        len = strlen(entry->d_name);
        if(len < TREE_MAX_FILENAMELEN)
            /* strncpy() is evil, we memcpy() instead, +1 includes the
               trailing zero */
            memcpy(buffer[i].name, entry->d_name, len+1);
        else
            memcpy(buffer[i].name, "too long", 9);

        buffer[i].file = !(entry->attribute&ATTR_DIRECTORY);

#ifdef HAVE_LCD_BITMAP
        if ( buffer[i].file )
            icon_type=File;
        else
            icon_type=Folder;
        lcd_bitmap(bitmap_icons_6x8[icon_type], 6, MARGIN_Y+i*LINE_HEIGTH, 6, 
                   8, true);
#endif

        if(len < TREE_MAX_LEN_DISPLAY)
            lcd_puts(LINE_X, LINE_Y+i, buffer[i].name);
        else {
            char storage = buffer[i].name[TREE_MAX_LEN_DISPLAY];
            buffer[i].name[TREE_MAX_LEN_DISPLAY]=0;
            lcd_puts(LINE_X, LINE_Y+i, buffer[i].name);
            buffer[i].name[TREE_MAX_LEN_DISPLAY]=storage;
        }

        if(++i >= TREE_MAX_ON_SCREEN)
            break;
    }

    if (entry==0) {
        *at_end=1;
    } else {
        *at_end=(readdir(dir)==0); 
    }
    j = i ;
    while (j++ < TREE_MAX_ON_SCREEN) {
        lcd_puts(LINE_X, LINE_Y+j,"                 ");
    }
    closedir(dir);

    return i;
}

bool dirbrowse(char *root)
{
    struct entry buffer[TREE_MAX_ON_SCREEN];
    int numentries;
    char buf[255];
    char currdir[255];
    int dircursor=0;
    int i;
    int start=0;
    int at_end=0;
    int dirpos[MAX_DIR_LEVELS];
    int dirlevel=0;

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_putsxy(0,0, "[Browse]",0);
    lcd_setmargins(0,MARGIN_Y);
    lcd_setfont(0);
#endif
    memcpy(currdir,root,sizeof(currdir));

    numentries = showdir(root, buffer, 0, start, &at_end);

    if (numentries == -1) 
        return -1;  /* root is not a directory */

    lcd_puts(0, dircursor, "-");
#ifdef HAVE_LCD_BITMAP
    lcd_update();
#endif

    while(1) {
        int key = button_get();

        if(!key) {
            sleep(1);
            continue;
        }
        switch(key) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
                return false;
                break;
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
#endif
                i=strlen(currdir);
                if (i==1) {
                    return false;
                }
                else {
                    while (currdir[i-1]!='/')
                        i--;
                    strcpy(buf,&currdir[i]);
                    if (i==1)
                        currdir[i]=0;
                    else
                        currdir[i-1]=0;

                    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
                    lcd_putsxy(0,0, "[Browse]",0);
#endif
                    dirlevel--;
                    if ( dirlevel < MAX_DIR_LEVELS )
                        start = dirpos[dirlevel];
                    else
                        start = 0;
                    numentries = showdir(currdir, buffer, 0, start, &at_end);
                    dircursor=0;
                    while ( (dircursor < TREE_MAX_ON_SCREEN) && 
                            (strcmp(buffer[dircursor].name,buf)!=0)) 
                        dircursor++;
                    if (dircursor==TREE_MAX_ON_SCREEN)
                        dircursor=0;
                    lcd_puts(0, LINE_Y+dircursor, "-");
#ifdef HAVE_LCD_BITMAP
                    lcd_update();
#endif
                }

                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
#endif
            case BUTTON_PLAY:
                if ((currdir[0]=='/') && (currdir[1]==0)) {
                    snprintf(buf,sizeof(buf),"%s%s",currdir,buffer[dircursor].name);
                } else {
                    snprintf(buf,sizeof(buf),"%s/%s",currdir,buffer[dircursor].name);
                }

                if (!buffer[dircursor].file) {
                    memcpy(currdir,buf,sizeof(currdir));
                    if ( dirlevel < MAX_DIR_LEVELS )
                        dirpos[dirlevel] = start+dircursor;
                    dirlevel++;
                    dircursor=0;
                    start=0;
                } else {
                    playtune(currdir, buffer[dircursor].name);
#ifdef HAVE_LCD_BITMAP
                    lcd_setmargins(0, MARGIN_Y);
                    lcd_setfont(0);
#endif
                }

                lcd_clear_display();
                numentries = showdir(currdir, buffer, 0, start, &at_end);  
                lcd_puts(0, LINE_Y+dircursor, "-");
#ifdef HAVE_LCD_BITMAP
                lcd_putsxy(0,0, "[Browse]",0);
                lcd_update();
#endif
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
#ifdef HAVE_LCD_BITMAP
                    lcd_update();
#endif
                }
                else {
                    if (start) {   
                        lcd_clear_display();
                        start--;
                        numentries = showdir(currdir, buffer, 0, 
                                             start, &at_end);
                        lcd_puts(0, LINE_Y+dircursor, "-");
#ifdef HAVE_LCD_BITMAP
                        lcd_putsxy(0,0, "[Browse]",0);
                        lcd_update();
#endif
                    }
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_RIGHT:
#endif
                if(dircursor+1 < numentries) {
                    lcd_puts(0, LINE_Y+dircursor, " ");
                    dircursor++;
                    lcd_puts(0, LINE_Y+dircursor, "-");
#ifdef HAVE_LCD_BITMAP
                    lcd_update();
#endif
                } else
                {
                    if (!at_end) {
                        lcd_clear_display();
                        start++;
                        numentries = showdir(currdir, buffer, 0, 
                                             start, &at_end);
                        lcd_puts(0, LINE_Y+dircursor, "-");
#ifdef HAVE_LCD_BITMAP
                        lcd_putsxy(0,0, "[Browse]",0);
                        lcd_update();
#endif
                    }
                }
                break;
        }
    }

    return false;
}
