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

#include "play.h"

#define TREE_MAX_FILENAMELEN 64
#define TREE_MAX_ON_SCREEN   7
#define TREE_MAX_LEN_DISPLAY 17 /* max length that fits on screen */

int dircursor=0;

void browse_root(void) {
	dirbrowse("/");
}

struct entry {
  int file; /* TRUE if file, FALSE if dir */
  char name[TREE_MAX_FILENAMELEN];
  int namelen;
};

#define LINE_Y      8 /* Y position the entry-list starts at */
#define LINE_X      6 /* X position the entry-list starts at */
#define LINE_HEIGTH 8 /* pixels for each text line */

#ifdef HAVE_LCD_BITMAP

int static
showdir(char *path, struct entry *buffer, int start)
{
  int i;
  DIR *dir = opendir(path);
  struct dirent *entry;

  if(!dir)
    return 0; /* no entries */

  i=start;
  while((entry = readdir(dir))) {
    int len;

    if(entry->d_name[0] == '.')
      /* skip names starting with a dot */
      continue;

    len = strlen(entry->d_name);
    if(len < TREE_MAX_FILENAMELEN)
      /* strncpy() is evil, we memcpy() instead, +1 includes the
         trailing zero */
      memcpy(buffer[i].name, entry->d_name, len+1);
    else
      memcpy(buffer[i].name, "too long", 9);

    buffer[i].file = TRUE; /* files only for now */

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
  i--; /* number of files on screen */

  closedir(dir);

  return i;
}

#endif

bool dirbrowse(char *root)
{
  struct entry buffer[TREE_MAX_ON_SCREEN];
  int numentries;

#ifdef HAVE_LCD_BITMAP
  lcd_clear_display();

  lcd_puts(0,0, "[Browse]", 0);

  numentries = showdir(root, buffer, 0);  

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
    case BUTTON_LEFT:
      return FALSE;
      break;

    case BUTTON_RIGHT:
    case BUTTON_PLAY:
      playtune(root, buffer[dircursor].name);

      lcd_clear_display();
      lcd_puts(0,0, "[Browse]", 0);
      numentries = showdir(root, buffer, 0);  
      lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
      lcd_update();
      break;
      
    case BUTTON_UP:
      if(dircursor) {
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, " ", 0);
        dircursor--;
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
        lcd_update();
      }
      break;
    case BUTTON_DOWN:
      if(dircursor < numentries) {
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, " ", 0);
        dircursor++;
        lcd_puts(0, LINE_Y+dircursor*LINE_HEIGTH, "-", 0);
        lcd_update();
      }
      break;
    }
  }
#endif

  return FALSE;
}
