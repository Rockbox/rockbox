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
#include "config.h"

/*
 * Specific implementations for X11, using the generic LCD API and data.
 */

#include "lcd-x11.h"
#include "lcd-playersim.h"

#if LCD_DEPTH == 2
#define YBLOCK 4
#define BITOFFS 1  /* take the MSB of each pixel */
#define ANDBIT 3 /* AND with this to get the color number */
#else
#define YBLOCK 8
#define BITOFFS 0
#define ANDBIT 1
#endif

extern void screen_resized(int width, int height);

#ifdef HAVE_LCD_BITMAP
extern unsigned char lcd_framebuffer[LCD_HEIGHT/YBLOCK][LCD_WIDTH];
unsigned char lcd_framebuffer_copy[LCD_HEIGHT/YBLOCK][LCD_WIDTH];

void lcd_update (void)
{
    /* update a full screen rect */
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x_start, int y_start,
                     int width, int height)
{
    int x;
    int yline=y_start;
    int y;
    int p=0;
    int bit;
    int xmax;
    int ymax;
    int colors[LCD_WIDTH * LCD_HEIGHT];
    struct coordinate points[LCD_WIDTH * LCD_HEIGHT];

#if 0
    fprintf(stderr, "%04d: lcd_update_rect(%d, %d, %d, %d)\n",
            counter++, x_start, y_start, width, height);
#endif
    /* The Y coordinates have to work on even YBLOCK pixel rows */
    ymax = (yline + height)/YBLOCK;
    yline /= YBLOCK;

    xmax = x_start + width;

    if(xmax > LCD_WIDTH)
        xmax = LCD_WIDTH;
    if(ymax >= LCD_HEIGHT/YBLOCK)
        ymax = LCD_HEIGHT/YBLOCK-1;

    for(; yline<=ymax; yline++) {
        y = yline * YBLOCK;
        for(x=x_start; x<xmax; x++) {
            if(lcd_framebuffer[yline][x] != lcd_framebuffer_copy[yline][x]) {
                /* one or more bits/pixels are changed */

                for(bit=0; bit<YBLOCK; bit++) {
                    unsigned int col;
                    col = lcd_framebuffer[yline][x]&(ANDBIT<<(bit*LCD_DEPTH));

#if LCD_DEPTH == 2
                    /* shift down the value to the lower bits */
                    col >>= (bit * LCD_DEPTH);

                    /* set a dot */
                    colors[p] = col;
#else
                    colors[p] = col?3:0;
#endif
                    points[p].x = x + MARGIN_X;
                    points[p].y = y+bit + MARGIN_Y;
                    p++; /* increase the point counter */
                }

                /* update the copy */
                lcd_framebuffer_copy[yline][x] = lcd_framebuffer[yline][x];
            }
        }
    }

    dots(colors, &points[0], p);
    /* printf("lcd_update_rect: Draws %d pixels, clears %d pixels\n", p, cp);*/
    XtAppLock(app);
    XSync(dpy,False);
    XtAppUnlock(app);
}

#ifdef LCD_REMOTE_HEIGHT
extern unsigned char lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH];
unsigned char lcd_remote_framebuffer_copy[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH];

#define REMOTE_START_Y (LCD_HEIGHT + 2*MARGIN_Y)

void lcd_remote_update (void)
{
    lcd_remote_update_rect(0, 0, LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT);
}

void lcd_remote_update_rect(int x_start, int y_start,
                            int width, int height)
{
    int x;
    int yline=y_start;
    int y;
    int p=0;
    int bit;
    int xmax;
    int ymax;
    struct coordinate points[LCD_WIDTH * LCD_HEIGHT];
    int colors[LCD_WIDTH * LCD_HEIGHT];

#if 0
    fprintf(stderr, "%04d: lcd_update_rect(%d, %d, %d, %d)\n",
            counter++, x_start, y_start, width, height);
#endif
    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (yline + height)/8;
    yline /= 8;

    xmax = x_start + width;

    if(xmax > LCD_REMOTE_WIDTH)
        xmax = LCD_REMOTE_WIDTH;
    if(ymax >= LCD_REMOTE_HEIGHT/8)
        ymax = LCD_REMOTE_HEIGHT/8-1;

    for(; yline<=ymax; yline++) {
        y = yline * 8;
        for(x=x_start; x<xmax; x++) {
            if(lcd_remote_framebuffer[yline][x] ||
               lcd_remote_framebuffer_copy[yline][x]) {
                /* one or more bits/pixels are changed */
                unsigned char diff =
                  lcd_remote_framebuffer[yline][x] ^
                    lcd_remote_framebuffer_copy[yline][x];

                for(bit=0; bit<8; bit++) {
                    if(lcd_remote_framebuffer[yline][x]&(1<<bit)) {
                        /* set a dot */
                        colors[p]=3;
                        points[p].x = x + MARGIN_X;
                        points[p].y = y+bit + (REMOTE_START_Y + MARGIN_Y);
                        p++; /* increase the point counter */
                    }
                    else if(diff &(1<<bit)) {
                        /* clear a dot */
                        colors[p]=0;
                        points[p].x = x + MARGIN_X;
                        points[p].y = y+bit + (REMOTE_START_Y + MARGIN_Y);
                        p++; /* increase the point counter */
                    }
                }
                
                /* update the copy */
                lcd_remote_framebuffer_copy[yline][x] =
                    lcd_remote_framebuffer[yline][x];
            }
        }
    }

    dots(colors, &points[0], p);
    /* printf("lcd_update_rect: Draws %d pixels, clears %d pixels\n", p, cp);*/
    XtAppLock(app);
    XSync(dpy,False);
    XtAppUnlock(app);
}


#endif

#endif
#ifdef HAVE_LCD_CHARCELLS

/* Defined in lcd-playersim.c */
extern void lcd_print_char(int x, int y);
extern unsigned char lcd_buffer[2][11];
extern void drawrect(int color, int x1, int y1, int x2, int y2);
extern bool lcd_display_redraw;

extern unsigned char hardware_buffer_lcd[11][2];
static unsigned char lcd_buffer_copy[11][2];

void lcd_update (void)
{
    bool changed=false;
    int x, y;
    for (y=0; y<2; y++) {
        for (x=0; x<11; x++) {
            if (lcd_display_redraw ||
                lcd_buffer_copy[x][y] != hardware_buffer_lcd[x][y]) {
                lcd_buffer_copy[x][y] = hardware_buffer_lcd[x][y];
                lcd_print_char(x, y);
                changed=true;
            }
        }
    }
    if (changed)
    {
        XtAppLock(app);
        XSync(dpy,False);
        XtAppUnlock(app);
    }
    lcd_display_redraw=false;
}

#endif
