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

#include "file.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"

#include "id3.h"

#ifdef MPEG_PLAY
#include "common/mpegplay.h"
#endif

#define LINE_Y      1 /* initial line */

void playtune(char *dir, char *file)
{
    char buffer[256];
    int fd;
    mp3entry mp3;
    bool good=1;

    sprintf(buffer, "%s/%s", dir, file);

    if(mp3info(&mp3, buffer)) {
        DEBUGF("id3 failure!");
        good=0;
    }
    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0,0);
    lcd_setfont(0);
#endif

    if(!good) {
        lcd_puts(0, 0, "[no id3 info]");
    }
    else {
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, 0, "[id3 info]");
        lcd_puts(0, LINE_Y, mp3.title?mp3.title:"");
        lcd_puts(0, LINE_Y+1, mp3.album?mp3.album:"");
        lcd_puts(0, LINE_Y+2, mp3.artist?mp3.artist:"");
    
        sprintf(buffer, "%d ms", mp3.length);
        lcd_puts(0, LINE_Y+3, buffer);
    
        sprintf(buffer, "%d kbits", mp3.bitrate);
        lcd_puts(0, LINE_Y+4, buffer);
    
        sprintf(buffer, "%d Hz", mp3.frequency);
        lcd_puts(0, LINE_Y+5, buffer);
#else
        lcd_puts(0, 0, mp3.artist?mp3.artist:"<no artist>");
        lcd_puts(0, 1, mp3.title?mp3.title:"<no title>");
#endif
    }
    lcd_update();

#ifdef MPEG_PLAY
    sprintf(buffer, "%s/%s", dir, file);
    mpeg_play(buffer);
    return;
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
                return;
                break;
        }
    }
}
