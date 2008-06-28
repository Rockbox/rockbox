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

#include "uisdl.h"
#include "lcd-sdl.h"
#include "lcd-remote-bitmap.h"

SDL_Surface *remote_surface;

SDL_Color remote_color_zero = {UI_REMOTE_BGCOLOR, 0};
SDL_Color remote_backlight_color_zero = {UI_REMOTE_BGCOLORLIGHT, 0};
SDL_Color remote_color_max  = {0, 0, 0, 0};

static unsigned long get_lcd_remote_pixel(int x, int y) {
#if LCD_REMOTE_DEPTH == 1
    return (lcd_remote_framebuffer[y/8][x] >> (y & 7)) & 1;
#elif LCD_REMOTE_DEPTH == 2
#if LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED
    unsigned bits = (lcd_remote_framebuffer[y/8][x] >> (y & 7)) & 0x0101;
    return (bits | (bits >> 7)) & 3;
#endif
#endif
}

void lcd_remote_update (void)
{
    lcd_remote_update_rect(0, 0, LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT);
}

void lcd_remote_update_rect(int x_start, int y_start, int width, int height)
{
    sdl_update_rect(remote_surface, x_start, y_start, width, height,
        LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT, get_lcd_remote_pixel);
    sdl_gui_update(remote_surface, x_start, y_start, width, height,
        LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT, background ? UI_REMOTE_POSX : 0,
        background ? UI_REMOTE_POSY : LCD_HEIGHT);
}

void sim_remote_backlight(int value)
{
    if (value > 0) {
        sdl_set_gradient(remote_surface, &remote_backlight_color_zero,
                         &remote_color_max, 0, (1<<LCD_REMOTE_DEPTH));
    } else {
        sdl_set_gradient(remote_surface, &remote_color_zero, &remote_color_max,
                         0, (1<<LCD_REMOTE_DEPTH));
    }

    sdl_gui_update(remote_surface, 0, 0, LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT,
                   LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT,
                   background ? UI_REMOTE_POSX : 0,
                   background? UI_REMOTE_POSY : LCD_HEIGHT);
}

/* initialise simulator lcd remote driver */
void sim_lcd_remote_init(void)
{
    remote_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
        LCD_REMOTE_WIDTH * display_zoom, LCD_REMOTE_HEIGHT * display_zoom,
        8, 0, 0, 0, 0);

    sdl_set_gradient(remote_surface, &remote_backlight_color_zero,
                     &remote_color_max, 0, (1<<LCD_REMOTE_DEPTH));
}

