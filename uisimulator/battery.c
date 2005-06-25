/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 1999 Robert Hak (rhak@ramapo.edu)
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
#include "debug.h"
#define CONFIG_KEYPAD RECORDER_PAD
#include "button.h"

#ifdef SIMULATOR
#include <stdio.h>
#include <unistd.h>
#endif

/* I hacked this function to be placed inside because I figure we will need
   something like it eventually.

   Args are fairly straight forward. 
   int xbase: location of "bottom" of battery on screen 
   int ybase: location of "left" edge of battery on screen 
   int len: how long is the battery to be (in pixels) 
   int wid: how tall is the battery to be (in pixels) 
   int percent: what percentage of the charge has been used 

   Note: I am making use of the Logf() function until logging is 
         straightened out. 
*/ 

void draw_battery(int xbase, int ybase, int len, int wid, int percent) 
{ 
    float capacity = 0; 
    int bar_xoffset = 2; 
    int bar_yoffset = 2; 
    int bar_len = 0; 
    int bar_wid = wid - (bar_xoffset*2); 
    int i = 0; 

    /* We only worry about length and width because if you place 
       the battery off the screen, its your own damn fault. We log 
       and then just draw an empty battery */ 
    if((percent > 100) || (percent < 0) || (len < 0) || (wid < 0)) { 
/*       debug("Error: Battery data invalid");  */
        percent = 0; 
    }

    /* top batt. edge */ 
    lcd_hline(xbase, xbase+len-2, ybase);

    /* bot batt. edge */ 
    lcd_hine(xbase, xbase+len-2, ybase+wid);

    /* left batt. edge */ 
    lcd_vline(xbase, ybase, ybase+wid);

    /* right batt. edge */ 
    lcd_vline(xbase+len, ybase+1, ybase+wid-1); 

    /* 2 dots that account for the nub on the right side of the battery */ 
    lcd_drawpixel(xbase+len-1, ybase+1); 
    lcd_drawpixel(xbase+len-1, ybase+wid-1); 

    if(percent > 0) { 
        /* % battery is full, 100% is length-bar_xoffset-1 pixels */ 
        capacity = ((float)percent / 100.0) * (len-(bar_xoffset*2)-1); 
        bar_len = capacity; 

        for(i = 0; i < bar_wid+1; i++) { 
            lcd_hline(xbase+bar_xoffset,
                      xbase+bar_xoffset+bar_len, ybase+bar_yoffset+i); 
        } 
    } 
	lcd_update();
} 









