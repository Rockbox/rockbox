/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 1999 Mattis Wadman (nappe@sudac.org)
 *
 * Heavily modified for embedded use by Björn Stenberg (bjorn@haxx.se)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "types.h"
#include "lcd.h"
#define HAVE_RECORDER_KEYPAD
#include "button.h"

#ifdef SIMULATOR
#include <stdio.h>
#include <unistd.h>
#endif

#define LINE_HEIGHT 8

#define MAX_LINE 3 /* the last index with info, starting on 0 */

void app_main(void)
{
  lcd_puts(0, 0,  "-Rockabox", 0);
  lcd_puts(6, 8,  "Boxrock", 0);
  lcd_puts(6, 16, "Robkoxx", 0);
  lcd_puts(6, 24, "Tetris", 0);
  lcd_puts(8, 38, "Rockbox!", 2);
  int cursor = 0;
  int key;

  while(1) {
    key = button_get();

    switch(key) {
    case BUTTON_UP:
      if(cursor) {
        lcd_puts(0, cursor, " ", 0);
        cursor-= LINE_HEIGHT;
        lcd_puts(0, cursor, "-", 0);
      }
      break;
    case BUTTON_DOWN:
      if(cursor<(MAX_LINE*LINE_HEIGHT)) {
        lcd_puts(0, cursor, " ", 0);
        cursor+=LINE_HEIGHT;
        lcd_puts(0, cursor, "-", 0);
      }
      break;
    case BUTTON_RIGHT:      
      if(cursor == (MAX_LINE * LINE_HEIGHT)) {
        lcd_clearrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
        tetris();
      }
      break;
    }
    lcd_update();
    sleep(1);
  }
}
