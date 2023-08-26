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
#include "misc.h"

double display_zoom = 1;

static bool window_needs_update;

void sdl_get_window_dimensions(int *w, int *h)
{
    if (background)
    {
        *w = UI_WIDTH;
        *h = UI_HEIGHT;
    }
    else
    {
#ifdef HAVE_REMOTE_LCD
        if (showremote)
        {
            *w = SIM_LCD_WIDTH > SIM_REMOTE_WIDTH ? SIM_LCD_WIDTH : SIM_REMOTE_WIDTH;
            *h = SIM_LCD_HEIGHT + SIM_REMOTE_HEIGHT;
        }
        else
#endif
        {
            *w = SIM_LCD_WIDTH;
            *h = SIM_LCD_HEIGHT;
        }
    }
}

static inline void sdl_render(void)
{
    SDL_Texture *sdlTexture = SDL_CreateTextureFromSurface(sdlRenderer, gui_surface);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
    SDL_DestroyTexture(sdlTexture);
}


#define SNAP_MARGIN 50
int sdl_update_window(void)
{
    if (!window_needs_update)
        return false;
    window_needs_update = false;

#if defined (__APPLE__) || defined(__WIN32) /* Constrain aspect ratio */
    if (!(SDL_GetWindowFlags(window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN)))
    {
        float aspect_ratio;
        int w, h;

        sdl_get_window_dimensions(&w, &h);
        aspect_ratio = (float) h / w;

        int original_height = h;
        int original_width = w;

        SDL_GetWindowSize(window, &w, &h);
        if (w != original_width || h != original_height)
        {
            if (abs(original_width - w) < SNAP_MARGIN)
            {
                w = original_width;
                h = original_height;
            }
            else
                h = w * aspect_ratio;

            SDL_SetWindowSize(window, w, h);
        }
    }
#endif
    sdl_render();
    return true;
}

void sdl_window_needs_update(void)
{
    window_needs_update = true;

    /* For MacOS and Windows, we're on a main or
    display thread already, and can immediately
    update the window.
    On Linux, we have to defer the update, until
    it is handled by the main thread later. */
#if defined (__APPLE__) || defined(__WIN32)
    sdl_update_window();
#endif
}


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

    if (display_zoom == 1) {
        dest = src;
        SDL_BlitSurface(lcd, &src, surface, &dest);
    } else {
        /* Note: SDL_SoftStretch is currently marked as DO NOT USE
           but there are no real alternatives for efficent zooming. */
        dest.x = src.x * display_zoom;
        dest.y = src.y * display_zoom;
        dest.w = src.w * display_zoom;
        dest.h = src.h * display_zoom;
        SDL_SoftStretch(lcd, &src, surface, &dest);
    }
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

    dest.w = display_zoom;
    dest.h = display_zoom;

    for (x = x_start; x < xmax; x++) {
        dest.x = x * display_zoom;

#ifdef HAVE_LCD_SPLIT
        for (y = y_start; y < MIN(ymax, LCD_SPLIT_POS); y++) {
            dest.y = y * display_zoom;

            SDL_FillRect(surface, &dest, (Uint32)(getpixel(x, y) | 0x80));
        }
        for (y = MAX(y_start, LCD_SPLIT_POS); y < ymax; y++) {
            dest.y = (y + LCD_SPLIT_LINES) * display_zoom ;

            SDL_FillRect(surface, &dest, (Uint32)getpixel(x, y));
        }
#else
        for (y = y_start; y < ymax; y++) {
            dest.y = y * display_zoom;

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

    SDL_Rect src = {x_start * display_zoom, y_start * display_zoom,
                    width * display_zoom, height * display_zoom};
    SDL_Rect dest= {(ui_x + x_start) * display_zoom,
                    (ui_y + y_start) * display_zoom,
                    width * display_zoom, height * display_zoom};

    uint8_t alpha;
    if (SDL_GetSurfaceAlphaMod(surface,&alpha) == 0 && alpha < 255)
        SDL_FillRect(gui_surface, &dest, 0); /* alpha needs a black background */

    SDL_BlitSurface(surface, &src, gui_surface, &dest);

    if (!sdl_update_window()) /* already calls sdl_render itself */
        sdl_render();
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
