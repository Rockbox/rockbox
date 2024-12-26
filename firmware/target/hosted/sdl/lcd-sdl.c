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

#include <SDL.h>
#include "lcd-sdl.h"
#include "sim-ui-defines.h"
#include "system.h" /* for MIN() and MAX() */
#include "window-sdl.h"

void sdl_update_rect(SDL_Surface *surface, int x_start, int y_start, int width,
                     int height, int max_x, int max_y,
                     unsigned long (*getpixel)(int, int))
{
    SDL_Rect dest;
#if LCD_DEPTH >= 8 && (LCD_PIXELFORMAT == RGB565) && \
    (LCD_STRIDEFORMAT == HORIZONTAL_STRIDE) && \
    !defined(HAVE_LCD_SPLIT) && !defined(HAVE_REMOTE_LCD)
    SDL_Rect src;
    (void)max_x;
    (void)max_y;
    (void)getpixel;
    /* Update complete screen via one blit operation (fast) */
    SDL_Surface *lcd = SDL_CreateRGBSurfaceFrom(FBADDR(0, 0), LCD_FBWIDTH,
                                                LCD_FBHEIGHT, LCD_DEPTH,
                                                LCD_FBWIDTH * LCD_DEPTH/8,
                                                0, 0, 0, 0);
    src.x = x_start;
    src.y = y_start;
    src.w = width;
    src.h = height;

    dest = src;
    SDL_BlitSurface(lcd, &src, surface, &dest);
    SDL_FreeSurface(lcd);
#else
    int x, y;
    int xmax, ymax;
    /* Very slow pixel-by-pixel drawing */
    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > max_x)
        xmax = max_x;
    if(ymax >= max_y)
        ymax = max_y;

    dest.w = 1;
    dest.h = 1;

    for (x = x_start; x < xmax; x++) {
        dest.x = x;

#ifdef HAVE_LCD_SPLIT
        for (y = y_start; y < MIN(ymax, LCD_SPLIT_POS); y++) {
            dest.y = y;

            SDL_FillRect(surface, &dest, (Uint32)(getpixel(x, y) | 0x80));
        }
        for (y = MAX(y_start, LCD_SPLIT_POS); y < ymax; y++) {
            dest.y = (y + LCD_SPLIT_LINES) ;

            SDL_FillRect(surface, &dest, (Uint32)getpixel(x, y));
        }
#else
        for (y = y_start; y < ymax; y++) {
            dest.y = y;

            SDL_FillRect(surface, &dest, (Uint32)getpixel(x, y));
        }
#endif
    }
#endif
}

void sdl_gui_update(SDL_Surface *surface, int x_start, int y_start, int width,
                    int height, int max_x, int max_y, int ui_x, int ui_y)
{
    if (x_start + width > max_x)
        width = max_x - x_start;
    if (y_start + height > max_y)
        height = max_y - y_start;

    SDL_Rect src = {x_start, y_start, width, height};
    SDL_Rect dest= {ui_x + x_start, ui_y + y_start, width, height};

    uint8_t alpha;

    SDL_LockMutex(window_mutex);

    if (SDL_GetSurfaceAlphaMod(surface,&alpha) == 0 && alpha < 255)
        SDL_FillRect(sim_lcd_surface, NULL, 0); /* alpha needs a black background */

    SDL_BlitSurface(surface, &src, sim_lcd_surface, NULL);
    SDL_UpdateTexture(gui_texture, &dest,
                      sim_lcd_surface->pixels, sim_lcd_surface->pitch);

    if (!sdl_window_adjust()) /* already calls sdl_window_render itself */
        sdl_window_render();
    SDL_UnlockMutex(window_mutex);
}

/* set a range of bitmap indices to a gradient from startcolour to endcolour */
void sdl_set_gradient(SDL_Surface *surface, SDL_Color *start, SDL_Color *end,
                      int first, int steps)
{
    int i;
    SDL_Color palette[steps];

    for (i = 0; i < steps; i++) {
        palette[i].r = start->r + (end->r - start->r) * i / (steps - 1);
        palette[i].g = start->g + (end->g - start->g) * i / (steps - 1);
        palette[i].b = start->b + (end->b - start->b) * i / (steps - 1);
    }

    SDL_SetPaletteColors(surface->format->palette, palette, first , steps);
}

int lcd_get_dpi(void)
{
#if (CONFIG_PLATFORM & PLATFORM_MAEMO5)
    return 267;
#elif (CONFIG_PLATFORM & PLATFORM_MAEMO4)
    return 225;
#elif (CONFIG_PLATFORM & PLATFORM_PANDORA)
    return 217;
#else
    /* TODO: find a way to query it from the OS, SDL doesn't support it
     * directly; for now assume the more or less standard 96 */
    return 96;
#endif
}
