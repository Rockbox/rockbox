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

#include "file.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"
#include "sprintf.h"

#include "id3.h"
#include "mpeg.h"
#include "settings.h"

#ifdef MPEG_PLAY
#include "mpegplay.h"
#endif

#define LINE_Y      1 /* initial line */

void playtune(char *filename)
{
    static char mfile[256];
    struct mp3entry mp3;
    bool good=1;
#ifdef HAVE_LCD_BITMAP
    char buffer[256];
#endif

    if(mp3info(&mp3, filename)) {
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
    
        snprintf(buffer,sizeof(buffer), "%d ms", mp3.length);
        lcd_puts(0, LINE_Y+3, buffer);
    
        snprintf(buffer,sizeof(buffer), "%d kbits", mp3.bitrate);
        lcd_puts(0, LINE_Y+4, buffer);
    
        snprintf(buffer,sizeof(buffer), "%d Hz", mp3.frequency);
        lcd_puts(0, LINE_Y+5, buffer);
#else
        lcd_puts(0, 0, mp3.artist?mp3.artist:"<no artist>");
        lcd_puts(0, 1, mp3.title?mp3.title:"<no title>");
#endif
    }

#ifdef HAVE_LCD_BITMAP
    lcd_update();
#endif

    strncpy(mfile, filename, 256);
    mpeg_play(mfile);

    while(1) {
        switch ( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
                global_settings.volume += 2;
                if(global_settings.volume > 100)
                    global_settings.volume = 100;
                mpeg_volume(global_settings.volume);
                break;
                
            case BUTTON_DOWN:
                global_settings.volume -= 2;
                if(global_settings.volume < 0)
                    global_settings.volume = 0;
                mpeg_volume(global_settings.volume);
                break;
                
            case BUTTON_OFF:
            case BUTTON_LEFT:
                return;
                break;
#else
            case BUTTON_RIGHT:
                global_settings.volume += 2;
                if(global_settings.volume > 100)
                    global_settings.volume = 100;
                mpeg_volume(global_settings.volume);
                break;
                
            case BUTTON_LEFT:
                global_settings.volume -= 2;
                if(global_settings.volume < 0)
                    global_settings.volume = 0;
                mpeg_volume(global_settings.volume);
                break;
                
            case BUTTON_STOP:
#endif
                return;
                break;
        }
    }
}
