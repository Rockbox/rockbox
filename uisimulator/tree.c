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

#include <string.h>
#include <stdlib.h>

#include <dir.h>
#include <file.h>
#include <types.h>
#include <lcd.h>
#include <button.h>
#include "kernel.h"
#include "tree.h"

#include "icons.h"

#include "play.h"

#define TREE_MAX_FILENAMELEN 64
#define TREE_MAX_ON_SCREEN   7
#define TREE_MAX_LEN_DISPLAY 16 /* max length that fits on screen */
 
void browse_root(void) {
	dirbrowse("/");
}

struct entry {
  int file; /* TRUE if file, FALSE if dir */
  char name[TREE_MAX_FILENAMELEN];
  int namelen;
};

#define LINE_Y      8 /* Y position the entry-list starts at */
#define LINE_X      12 /* X position the entry-list starts at */
#define LINE_HEIGTH 8 /* pixels for each text line */

#ifdef HAVE_LCD_BITMAP

extern unsigned char bitmap_icons_6x8[LastIcon][6];
extern icons_6x8;

int static showdir(char *path, struct entry *buffer, int start, 
                   int scrollpos, int* at_end)
{
  int i;
  int j=0;
  int icon_type = 0;
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

    buffer[i].file?(icon_type=FileIcon):(icon_type=DirIcon);
    lcd_bitmap(bitmap_icons_6x8[icon_type], 6, LINE_Y+i*LINE_HEIGTH, 6, 
               8, TRUE);

    if(len < TREE_MAX_LEN_DISPLAY)
      lcd_puts(LINE_X, LINE_Y+i*LINE_HEIGTH, buffer[i].name, 0);
    else {
      char storage = buffer[i].name[TREE_MAX_LEN_DISPLAY];
      buffer[i].name[TREE_MAX_LEN_DISPLAY]=0;
      lcd_puts(LINE_X, LINE_Y+i*LINE_HEIGTH, buffer[i].name, 0);
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
      lcd_puts(LINE_X, LINE_Y+j*LINE_HEIGTH,"                 ", 0);  
  }
  closedir(dir);

  return i;
}

#endif

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

#ifdef HAVE_LCD_BITMAP
  lcd_clear_display();

  lcd_puts(0,0, "[Browse]", 0);
  memcpy(currdir,root,sizeof(currdir));

  numentries = showdir(root, buffer, 0, start, &at_end);

  if (numentries == -1) return -1;  /* root is not a directory */

  lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);

  lcd_update();

  while(1) {
    int key = button_get();

    if(!key) {
      sleep(1);
      continue;
    }
    switch(key) {
    case BUTTON_OFF:
      return FALSE;
      break;

    case BUTTON_LEFT:
      i=strlen(currdir);
      if (i==1) {
        return FALSE;
      } else {
        while (currdir[i-1]!='/') i--;
        strcpy(buf,&currdir[i]);
        if (i==1) currdir[i]=0;
        else currdir[i-1]=0;

        lcd_clear_display();
        lcd_puts(0,0, "[Browse]", 0);
        numentries = showdir(currdir, buffer, 0, 0, &at_end);  
        dircursor=0;
        start=0;
        while ( (dircursor < TREE_MAX_ON_SCREEN) && 
                (strcmp(buffer[dircursor].name,buf)!=0)) dircursor++;
        if (dircursor==TREE_MAX_ON_SCREEN) dircursor=0;
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
        lcd_update();
      }

      break;

    case BUTTON_RIGHT:
    case BUTTON_PLAY:
      if ((currdir[0]=='/') && (currdir[1]==0)) {
        sprintf(buf,"%s%s",currdir,buffer[dircursor].name);
      } else {
        sprintf(buf,"%s/%s",currdir,buffer[dircursor].name);
      }

      if (!buffer[dircursor].file) {
        memcpy(currdir,buf,sizeof(currdir));
        dircursor=0;
        start=0;
      } else {
        playtune(currdir, buffer[dircursor].name);
      }

      lcd_clear_display();
      lcd_puts(0,0, "[Browse]", 0);
      numentries = showdir(currdir, buffer, 0, start, &at_end);  
      lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
      lcd_update();
      break;
      
    case BUTTON_UP:
      if(dircursor) {
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, " ", 0);
        dircursor--;
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
        lcd_update();
      } else
      {
        if (start) {   
            lcd_clear_display();
            lcd_puts(0,0, "[Browse]", 0);
            numentries = showdir(currdir, buffer, 0, --start, &at_end);
            lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
            lcd_update();
        }
      }
      break;
    case BUTTON_DOWN:
      if(dircursor+1 < numentries) {
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, " ", 0);
        dircursor++;
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
        lcd_update();
      } else
      {
        if (!at_end) {
          lcd_clear_display();
          lcd_puts(0,0, "[Browse]", 0);
          numentries = showdir(currdir, buffer, 0, ++start, &at_end);
       
          lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
          lcd_update();
        }
      }
      break;
    }
  }
#endif

  return FALSE;
}
