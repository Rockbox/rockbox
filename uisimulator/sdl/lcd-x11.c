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

#include <SDL.h>

extern SDL_Surface *surface;

extern void screen_resized(int width, int height);
extern bool lcd_display_redraw;

#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH==16
fb_data lcd_framebuffer_copy[LCD_HEIGHT][LCD_WIDTH*2];
#else
fb_data lcd_framebuffer_copy[LCD_HEIGHT][LCD_WIDTH];
#endif

void lcd_update (void)
{
    /* update a full screen rect */
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x_start, int y_start,
                     int width, int height)
{
    int x;
    int y;
    int p=0;
    int xmax;
    int ymax;
    int colors[LCD_WIDTH * LCD_HEIGHT];
    struct coordinate points[LCD_WIDTH * LCD_HEIGHT];

#if 0
    fprintf(stderr, "%04d: lcd_update_rect(%d, %d, %d, %d)\n",
            counter++, x_start, y_start, width, height);
#endif
    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > LCD_WIDTH)
        xmax = LCD_WIDTH;
    if(ymax >= LCD_HEIGHT)
        ymax = LCD_HEIGHT;

    for (x = x_start; x < xmax; x++)
        for (y = y_start; y < ymax; y++)
        {
#if LCD_DEPTH == 1
            Uint32 sdl_white = SDL_MapRGB(surface->format, 255, 255, 255);
            Uint32 sdl_black = SDL_MapRGB(surface->format, 0, 0, 0);
            points[p].x = x + MARGIN_X;
            points[p].y = y + MARGIN_Y;
            colors[p] = ((lcd_framebuffer[y/8][x] >> (y & 7)) & 1) ? sdl_black : sdl_white;
#elif LCD_DEPTH == 2
            unsigned gray = ((lcd_framebuffer[y/4][x] >> (2 * (y & 3))) & 3) << 6;
            points[p].x = x + MARGIN_X;
            points[p].y = y + MARGIN_Y;
            colors[p] = SDL_MapRGB(surface->format, 255-gray, 255-gray, 255-gray);
#elif LCD_DEPTH == 16
#if LCD_PIXELFORMAT == RGB565SWAPPED
            unsigned b = ((lcd_framebuffer[y][x] >> 11) & 0x1f) << 3;
            unsigned g = ((lcd_framebuffer[y][x] >> 5) & 0x3f) << 2;
            unsigned r = ((lcd_framebuffer[y][x]) & 0x1f) << 3;
            points[p].x = x + MARGIN_X;
            points[p].y = y + MARGIN_Y;
            colors[p] = SDL_MapRGB(surface->format, r, g, b);
#else
            unsigned r = ((lcd_framebuffer[y][x] >> 11) & 0x1f) << 3;
            unsigned g = ((lcd_framebuffer[y][x] >> 5) & 0x3f) << 2;
            unsigned b = ((lcd_framebuffer[y][x]) & 0x1f) << 3;
            points[p].x = x + MARGIN_X;
            points[p].y = y + MARGIN_Y;
            colors[p] = SDL_MapRGB(surface->format, r, g, b);
#endif
#endif
            p++;
        }

    dots(colors, &points[0], p);
    /* printf("lcd_update_rect: Draws %d pixels, clears %d pixels\n", p, cp);*/
    SDL_UpdateRect(surface, 0, 0, 0, 0);
    lcd_display_redraw=false;
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
    int y;
    int p=0;
    int xmax;
    int ymax;
    struct coordinate points[LCD_REMOTE_WIDTH * LCD_REMOTE_HEIGHT];
    int colors[LCD_REMOTE_WIDTH * LCD_REMOTE_HEIGHT];
    Uint32 sdl_white = SDL_MapRGB(surface->format, 255, 255, 255);
    Uint32 sdl_black = SDL_MapRGB(surface->format, 0, 0, 0);

#if 0
    fprintf(stderr, "%04d: lcd_update_rect(%d, %d, %d, %d)\n",
            counter++, x_start, y_start, width, height);
#endif
    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > LCD_REMOTE_WIDTH)
        xmax = LCD_REMOTE_WIDTH;
    if(ymax >= LCD_REMOTE_HEIGHT)
        ymax = LCD_REMOTE_HEIGHT;

    for (x = x_start; x < xmax; x++)
        for (y = y_start; y < ymax; y++) {
            colors[p] = ((lcd_remote_framebuffer[y/8][x] >> (y & 7)) & 1) ? sdl_black : sdl_white;
            points[p].x = x + MARGIN_X;
            points[p].y = y + MARGIN_Y + REMOTE_START_Y;
            p++;
        }

    dots(colors, &points[0], p);
    /* printf("lcd_update_rect: Draws %d pixels, clears %d pixels\n", p, cp);*/
    SDL_UpdateRect(surface, 0, 0, 0, 0);
    lcd_display_redraw=false;
}


#endif

#endif
#ifdef HAVE_LCD_CHARCELLS

/* Defined in lcd-playersim.c */
extern void lcd_print_char(int x, int y);
extern unsigned char lcd_buffer[2][11];

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
        SDL_UpdateRect(surface, 0, 0, 0, 0);
    }
    lcd_display_redraw=false;
}

#endif
