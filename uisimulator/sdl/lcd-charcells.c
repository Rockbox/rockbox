/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "debug.h"
#include "lcd.h"
#include "lcd-charcell.h"
#include "misc.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "lcd-playersim.h"
#include "uisdl.h"
#include "lcd-sdl.h"

/* extern functions, needed for screendump() */
extern int sim_creat(const char *name);

SDL_Surface* lcd_surface;

SDL_Color lcd_bl_color_dark    = {RED_CMP(LCD_BL_DARKCOLOR),
                                  GREEN_CMP(LCD_BL_DARKCOLOR),
                                  BLUE_CMP(LCD_BL_DARKCOLOR), 0};
SDL_Color lcd_bl_color_bright  = {RED_CMP(LCD_BL_BRIGHTCOLOR),
                                  GREEN_CMP(LCD_BL_BRIGHTCOLOR),
                                  BLUE_CMP(LCD_BL_BRIGHTCOLOR), 0};
SDL_Color lcd_color_dark    = {RED_CMP(LCD_DARKCOLOR),
                               GREEN_CMP(LCD_DARKCOLOR),
                               BLUE_CMP(LCD_DARKCOLOR), 0};
SDL_Color lcd_color_bright  = {RED_CMP(LCD_BRIGHTCOLOR),
                               GREEN_CMP(LCD_BRIGHTCOLOR),
                               BLUE_CMP(LCD_BRIGHTCOLOR), 0};


static unsigned long get_lcd_pixel(int x, int y)
{
    return sim_lcd_framebuffer[y][x];
}

void sim_lcd_update_rect(int x_start, int y_start, int width, int height)
{
    sdl_update_rect(lcd_surface, x_start, y_start, width, height,
                    SIM_LCD_WIDTH, SIM_LCD_HEIGHT, get_lcd_pixel);
    sdl_gui_update(lcd_surface, x_start, y_start, width, height,
                   SIM_LCD_WIDTH, SIM_LCD_HEIGHT,
                   background ? UI_LCD_POSX : 0, background ? UI_LCD_POSY : 0);
}

void lcd_update(void)
{
    int x, y;
    
    for (y = 0; y < lcd_pattern_count; y++)
        if (lcd_patterns[y].count > 0)
            sim_lcd_define_pattern(y, lcd_patterns[y].pattern);
        
    for (y = 0; y < LCD_HEIGHT; y++)
        for (x = 0; x < LCD_WIDTH; x++)
            lcd_print_char(x, y, lcd_charbuffer[y][x]);

    if (lcd_cursor.visible)
        lcd_print_char(lcd_cursor.x, lcd_cursor.y, lcd_cursor.hw_char);

    sim_lcd_update_rect(0, ICON_HEIGHT, SIM_LCD_WIDTH,
                        LCD_HEIGHT*CHAR_HEIGHT*CHAR_PIXEL);
}

#ifdef HAVE_BACKLIGHT
void sim_backlight(int value)
{
    if (value > 0) {
        sdl_set_gradient(lcd_surface, &lcd_bl_color_bright,
                         &lcd_bl_color_dark, 0, (1<<LCD_DEPTH));
    } else {
        sdl_set_gradient(lcd_surface, &lcd_color_bright,
                         &lcd_color_dark, 0, (1<<LCD_DEPTH));
    }

    sim_lcd_update_rect(0, 0, SIM_LCD_WIDTH, SIM_LCD_HEIGHT);
}
#endif

/* initialise simulator lcd driver */
void sim_lcd_init(void)
{
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
                                       SIM_LCD_WIDTH * display_zoom,
                                       SIM_LCD_HEIGHT * display_zoom,
                                       8, 0, 0, 0, 0);

    sdl_set_gradient(lcd_surface, &lcd_bl_color_bright,
                     &lcd_bl_color_dark, 0, (1<<LCD_DEPTH));
}

#define BMP_COMPRESSION 0 /* BI_RGB */
#define BMP_NUMCOLORS (1 << LCD_DEPTH)
#define BMP_BPP 1
#define BMP_LINESIZE (((SIM_LCD_WIDTH + 31) / 32) * 4)

#define BMP_HEADERSIZE (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE   (BMP_LINESIZE * SIM_LCD_HEIGHT)
#define BMP_TOTALSIZE  (BMP_HEADERSIZE + BMP_DATASIZE)

#define LE16_CONST(x) (x)&0xff, ((x)>>8)&0xff
#define LE32_CONST(x) (x)&0xff, ((x)>>8)&0xff, ((x)>>16)&0xff, ((x)>>24)&0xff

static const unsigned char bmpheader[] =
{
    0x42, 0x4d,                 /* 'BM' */
    LE32_CONST(BMP_TOTALSIZE),  /* Total file size */
    0x00, 0x00, 0x00, 0x00,     /* Reserved */
    LE32_CONST(BMP_HEADERSIZE), /* Offset to start of pixel data */

    0x28, 0x00, 0x00, 0x00,     /* Size of (2nd) header */
    LE32_CONST(SIM_LCD_WIDTH),  /* Width in pixels */
    LE32_CONST(SIM_LCD_HEIGHT), /* Height in pixels */
    0x01, 0x00,                 /* Number of planes (always 1) */
    LE16_CONST(BMP_BPP),        /* Bits per pixel 1/4/8/16/24 */
    LE32_CONST(BMP_COMPRESSION),/* Compression mode */
    LE32_CONST(BMP_DATASIZE),   /* Size of bitmap data */
    0xc4, 0x0e, 0x00, 0x00,     /* Horizontal resolution (pixels/meter) */
    0xc4, 0x0e, 0x00, 0x00,     /* Vertical resolution (pixels/meter) */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of used colours */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of important colours */

    BMP_COLOR(LCD_BL_BRIGHTCOLOR),
    BMP_COLOR(LCD_BL_DARKCOLOR)
};

void screen_dump(void)
{
    int fd;
    char filename[MAX_PATH];
    int x, y;
    static unsigned char line[BMP_LINESIZE];

    create_numbered_filename(filename, "", "dump_", ".bmp", 4
        IF_CNFN_NUM_(, NULL));
    DEBUGF("screen_dump\n");

    fd = sim_creat(filename);
    if (fd < 0)
        return;

    write(fd, bmpheader, sizeof(bmpheader));
    SDL_LockSurface(lcd_surface);

    /* BMP image goes bottom up */
    for (y = SIM_LCD_HEIGHT - 1; y >= 0; y--)
    {
        Uint8 *src = (Uint8 *)lcd_surface->pixels 
                   + y * SIM_LCD_WIDTH * display_zoom * display_zoom;
        unsigned char *dst = line;
        unsigned dst_mask = 0x80;

        memset(line, 0, sizeof(line));
        for (x = SIM_LCD_WIDTH; x > 0; x--)
        {
            if (*src)
                *dst |= dst_mask;
            src += display_zoom;
            dst_mask >>= 1;
            if (dst_mask == 0)
            {
                dst++;
                dst_mask = 0x80;
            }
        }
        write(fd, line, sizeof(line));
    }
    SDL_UnlockSurface(lcd_surface);
    close(fd);
}
