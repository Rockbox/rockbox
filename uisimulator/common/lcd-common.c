/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert E. Hak <rhak@ramapo.edu>
 *
 * Windows Copyright (C) 2002 by Felix Arends
 * X11 Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd-common.h"

/*** Define our #includes ***/
#ifdef WIN32 
 #include <windows.h>
 #include <process.h>
 #include "uisw32.h"
#else
 /* X11 */
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
#endif
/**** End #includes ****/


/*** Start global variables ***/
#ifdef WIN32

/* Simulate the target ui display */
char bitmap[LCD_HEIGHT][LCD_WIDTH];

BITMAPINFO2 bmi =
{
    { sizeof (BITMAPINFOHEADER),
	  LCD_WIDTH, -LCD_HEIGHT, 1, 8,
	  BI_RGB, 0, 0, 0, 2, 2, },
    {
        /* green background color */
        {UI_LCD_BGCOLOR, 0},
        /* black color */
        {UI_LCD_BLACK, 0} 
    }
  
};

#else
extern Display *dpy;
unsigned char lcd_framebuffer_copy[LCD_WIDTH][LCD_HEIGHT/8];
#endif

/* Both X11 and WIN32 use this */
extern unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8]; 

/*** End Global variables ***/


/*** Externed functions ***/
#ifndef WIN32
extern void screen_resized(int width, int height);

/* From uibasic.c */
extern void drawdots(int color, XPoint *points, int count);
#endif 

/*** End Externed functions ***/



/*** Begin Functions ***/
void lcd_update()
{
#ifdef WIN32
    int x, y;
    if(hGUIWnd == NULL)
        _endthread ();

    for(x = 0; x < LCD_WIDTH; x++)
        for(y = 0; y < LCD_HEIGHT; y++)
            bitmap[y][x] = ((lcd_framebuffer[x][y/8] >> (y & 7)) & 1);

    InvalidateRect (hGUIWnd, NULL, FALSE);

    Sleep (50);
#else
    int x, y, bit;
    int p=0, cp=0;

    XPoint points[LCD_WIDTH * LCD_HEIGHT];
    XPoint clearpoints[LCD_WIDTH * LCD_HEIGHT];

    for(y=0; y<LCD_HEIGHT; y+=8) {
        for(x=0; x<LCD_WIDTH; x++) {
            if(lcd_framebuffer[x][y/8] || lcd_framebuffer_copy[x][y/8]) {
                /* one or more bits/pixels are changed */
                unsigned char diff =
                    lcd_framebuffer[x][y/8] ^ lcd_framebuffer_copy[x][y/8];
                
                for(bit=0; bit<8; bit++) {
                    if(lcd_framebuffer[x][y/8]&(1<<bit)) {
                        /* set a dot */
                        points[p].x = x + MARGIN_X;
                        points[p].y = y+bit + MARGIN_Y;
                        p++;
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
    memcpy(lcd_framebuffer_copy, lcd_framebuffer, sizeof(lcd_framebuffer));

    drawdots(0, &clearpoints[0], cp);
    drawdots(1, &points[0], p);
    XSync(dpy,False);

#endif /* end WIN32*/
}

#ifdef WIN32
void lcd_backlight(bool on)
{
    if(on) {
        RGBQUAD blon = {UI_LCD_BGCOLORLIGHT, 0};
        bmi.bmiColors[0] = blon;
    } else {
        RGBQUAD blon = {UI_LCD_BGCOLOR, 0};
        bmi.bmiColors[0] = blon;
    }
    InvalidateRect (hGUIWnd, NULL, FALSE);
}
#endif
