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

extern unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];
extern void screen_resized(int width, int height);
extern Display *dpy;

#ifdef HAVE_LCD_BITMAP
unsigned char lcd_framebuffer_copy[LCD_HEIGHT/8][LCD_WIDTH];

void lcd_set_invert_display(bool invert)
{
    (void)invert;
}

void lcd_update (void)
{
    int x, y;
    int p=0;
    int bit;
    struct coordinate points[LCD_WIDTH * LCD_HEIGHT];
    int cp=0;
    struct coordinate clearpoints[LCD_WIDTH * LCD_HEIGHT];

    for(y=0; y<LCD_HEIGHT; y+=8) {
        for(x=0; x<LCD_WIDTH; x++) {
            if(lcd_framebuffer[y/8][x] || lcd_framebuffer_copy[y/8][x]) {
                /* one or more bits/pixels are changed */
                unsigned char diff =
                    lcd_framebuffer[y/8][x] ^ lcd_framebuffer_copy[y/8][x];

                for(bit=0; bit<8; bit++) {
                    if(lcd_framebuffer[y/8][x]&(1<<bit)) {
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
    memcpy(lcd_framebuffer_copy, lcd_framebuffer, sizeof(lcd_framebuffer));

    drawdots(0, &clearpoints[0], cp);
    drawdots(1, &points[0], p);
/* printf("lcd_update: Draws %d pixels, clears %d pixels (max %d/%d)\n", 
       p, cp, p+cp, LCD_HEIGHT*LCD_WIDTH); */
    XSync(dpy,False);
}

void lcd_update_rect(int x_start, int y_start,
                     int width, int height)
{
    int x;
    int yline=y_start;
    int y;
    int p=0;
    int bit;
    int cp=0;
    int xmax;
    int ymax;
    struct coordinate points[LCD_WIDTH * LCD_HEIGHT];
    struct coordinate clearpoints[LCD_WIDTH * LCD_HEIGHT];

#if 0
    fprintf(stderr, "%04d: lcd_update_rect(%d, %d, %d, %d)\n",
            counter++, x_start, y_start, width, height);
#endif
    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (yline + height)/8;
    yline /= 8;
 
    xmax = x_start + width;

    if(xmax > LCD_WIDTH)
        xmax = LCD_WIDTH;
    if(ymax >= LCD_HEIGHT/8)
        ymax = LCD_HEIGHT/8-1;

    for(; yline<=ymax; yline++) {
        y = yline * 8;
        for(x=x_start; x<xmax; x++) {
            if(lcd_framebuffer[yline][x] || lcd_framebuffer_copy[yline][x]) {
                /* one or more bits/pixels are changed */
                unsigned char diff =
                  lcd_framebuffer[yline][x] ^ lcd_framebuffer_copy[yline][x];

                for(bit=0; bit<8; bit++) {
                    if(lcd_framebuffer[yline][x]&(1<<bit)) {
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

                /* update the copy */
                lcd_framebuffer_copy[yline][x] = lcd_framebuffer[yline][x];
            }
        }
    }

    drawdots(0, &clearpoints[0], cp);
    drawdots(1, &points[0], p);
   /* printf("lcd_update_rect: Draws %d pixels, clears %d pixels\n", p, cp);*/
    XSync(dpy,False);
}
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
    XSync(dpy,False);
  lcd_display_redraw=false;
}

#endif
