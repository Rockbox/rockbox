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

#include <file.h>
#include <types.h>
#include <lcd.h>
#include <button.h>
#include "kernel.h"
#include "tree.h"

#include "id3.h"

#define LINE_Y      8 /* initial line */
#define LINE_HEIGTH 8 /* line height in pixels */

void playtune(char *dir, char *file)
{
  char buffer[256];
  int fd;
  mp3entry mp3;
  bool good=1;

  sprintf(buffer, "%s/%s", dir, file);

  if(mp3info(&mp3, buffer)) {
    Logf("Failure!");
    good=0;
  }
  else {
    printf("****** File: %s\n"
           "      Title: %s\n"
           "     Artist: %s\n"
           "      Album: %s\n"
           "     Length: %d ms\n"
           "    Bitrate: %d\n"
           "  Frequency: %d\n",
           buffer,
           mp3.title?mp3.title:"<blank>",
           mp3.artist?mp3.artist:"<blank>",
           mp3.album?mp3.album:"<blank>",
           mp3.length,
           mp3.bitrate,
           mp3.frequency);
  }
#ifdef HAVE_LCD_BITMAP
  lcd_clear_display();
  if(!good) {
    lcd_puts(0, 0, "[no id3 info]", 0);
  }
  else {
    lcd_puts(0, 0, "[id3 info]", 0);
    lcd_puts(0, LINE_Y, mp3.title?mp3.title:"", 0);
    lcd_puts(0, LINE_Y+1*LINE_HEIGTH, mp3.album?mp3.album:"", 0);
    lcd_puts(0, LINE_Y+2*LINE_HEIGTH, mp3.artist?mp3.artist:"", 0);
    
    sprintf(buffer, "%d ms", mp3.length);
    lcd_puts(0, LINE_Y+3*LINE_HEIGTH, buffer, 0);
    
    sprintf(buffer, "%d kbits", mp3.bitrate);
    lcd_puts(0, LINE_Y+4*LINE_HEIGTH, buffer, 0);
    
    sprintf(buffer, "%d Hz", mp3.frequency);
    lcd_puts(0, LINE_Y+5*LINE_HEIGTH, buffer, 0);
  }
  lcd_update();
#endif

  while(1) {
    int key = button_get();

    if(!key) {
      sleep(30);
      continue;
    }
    switch(key) {
    case BUTTON_OFF:
    case BUTTON_LEFT:
      return FALSE;
      break;
    }
  }
}
