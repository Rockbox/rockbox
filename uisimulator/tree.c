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

#include <dir.h>
#include <types.h>
#include <lcd.h>
#include <button.h>
#include "kernel.h"
#ifdef WIN32
#include <windows.h>
#endif // WIN32

#define TREE_MAX_LEN 15
#define TREE_MAX_ON_SCREEN 7

int dircursor=0;

bool dirbrowse(char *root)
{
  DIR *dir = opendir(root);
  int i;
  struct dirent *entry;
  char buffer[TREE_MAX_ON_SCREEN][20];

  if(!dir)
    return TRUE; /* failure */

  lcd_clearrect(0, 0, LCD_WIDTH, LCD_HEIGHT);

  lcd_puts(0,0, "[Browse]", 0);

  i=0;
  while((entry = readdir(dir))) {
    strncpy(buffer[i], entry->d_name, TREE_MAX_LEN);
    buffer[i][TREE_MAX_LEN]=0;
    lcd_puts(6, 8+i*8, buffer[i], 0);

    if(++i >= TREE_MAX_ON_SCREEN)
      break;
  }

  closedir(dir);

  lcd_puts(0, 8+dircursor, "-", 0);

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
      
    case BUTTON_UP:
      if(dircursor) {
        lcd_puts(0, 8+dircursor, " ", 0);
        dircursor -= 8;
        lcd_puts(0, 8+dircursor, "-", 0);
        lcd_update();
      }
      break;
    case BUTTON_DOWN:
      if(dircursor < (8 * TREE_MAX_ON_SCREEN)) {
        lcd_puts(0, 8+dircursor, " ", 0);
        dircursor += 8;
        lcd_puts(0, 8+dircursor, "-", 0);
        lcd_update();
      }
      break;
    }
  }

  return FALSE;
}
