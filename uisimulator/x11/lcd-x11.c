/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
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
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "screenhack.h"

/*
 * Specific implementations for X11, using the generic LCD API and data.
 */

#include "lcd-x11.h"

extern unsigned char display[LCD_WIDTH][LCD_HEIGHT/8];
extern void screen_resized(int width, int height);
extern Display *dpy;

unsigned char display_copy[LCD_WIDTH][LCD_HEIGHT/8];

/* this is in uibasic.c */
extern void drawdots(int color, XPoint *points, int count);

void lcd_update (void)
{
    int x, y;
    int p=0;
    int bit;
    XPoint points[LCD_WIDTH * LCD_HEIGHT];
    int cp=0;
    XPoint clearpoints[LCD_WIDTH * LCD_HEIGHT];

#if 0
    screen_resized(LCD_WIDTH, LCD_HEIGHT);

    for(y=0; y<LCD_HEIGHT; y+=8) {
        for(x=0; x<LCD_WIDTH; x++) {
            if(display[x][y/8]) {
                /* one or more bits/pixels are set */
                for(bit=0; bit<8; bit++) {
                    if(display[x][y/8]&(1<<bit)) {
                        points[p].x = x + MARGIN_X;
                        points[p].y = y+bit + MARGIN_Y;
                        p++; /* increase the point counter */
                    }
                }
            }
        }
    }
#else
    for(y=0; y<LCD_HEIGHT; y+=8) {
        for(x=0; x<LCD_WIDTH; x++) {
            if(display[x][y/8] || display_copy[x][y/8]) {
                /* one or more bits/pixels are changed */
                unsigned char diff =
                  display[x][y/8] ^ display_copy[x][y/8];

                for(bit=0; bit<8; bit++) {
                    if(display[x][y/8]&(1<<bit)) {
                        /* set a dot */
                        points[p].x = x + MARGIN_X;
                        points[p].y = y+bit + MARGIN_Y;
                        p++; /* increase the point counter */
                    }
                    else if(diff &(1<<bit)) {
                        /* clear a dot */
                        clearpoints[cp].x = x + MARGIN_X;
                        clearpoints[cp].y = y+bit + MARGIN_Y;
                        cp++; /* increase the point counter */
                    }
                }
            }
        }
    }

    /* copy a huge block */
    memcpy(display_copy, display, sizeof(display));

#endif

    drawdots(0, &clearpoints[0], cp);
    drawdots(1, &points[0], p);
/*    Logf("lcd_update: Draws %d pixels, clears %d pixels", p, cp);*/
    XSync(dpy,False);
}
