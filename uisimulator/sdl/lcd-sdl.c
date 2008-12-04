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

#include "lcd-sdl.h"
#include "uisdl.h"

int display_zoom = 1;
#ifdef UI_LCD_SPLIT
static int gradient_steps = 0;
#endif

void sdl_update_rect(SDL_Surface *surface, int x_start, int y_start, int width,
                     int height, int max_x, int max_y,
                     unsigned long (*getpixel)(int, int))
{
    int x, y;
    int xmax, ymax;
    SDL_Rect dest;

    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > max_x)
        xmax = max_x;
    if(ymax >= max_y)
        ymax = max_y;

    SDL_LockSurface(surface);

    dest.w = display_zoom;
    dest.h = display_zoom;

    for (x = x_start; x < xmax; x++) {
        dest.x = x * display_zoom;

        for (y = y_start; y < ymax; y++) {
            dest.y = y * display_zoom;

            SDL_FillRect(surface, &dest, (Uint32)getpixel(x, y));
        }
    }

    SDL_UnlockSurface(surface);
}

void sdl_gui_update(SDL_Surface *surface, IFSPLIT(SDL_Surface *real_surface,)
                    int x_start, int y_start, int width,
                    int height, int max_x, int max_y, int ui_x, int ui_y)
{
    int xmax, ymax;

    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > max_x)
        xmax = max_x;
    if(ymax >= max_y)
        ymax = max_y;

    SDL_Rect src = {x_start * display_zoom, y_start * display_zoom,
                    xmax * display_zoom, ymax * display_zoom};
    SDL_Rect dest= {(ui_x + x_start) * display_zoom, (ui_y + y_start) * display_zoom,
                    xmax * display_zoom, ymax * display_zoom};

#ifdef UI_LCD_SPLIT
    /* fix real screen coordinates */
    if(ymax >= UI_LCD_SPLIT_LINES)
        src.h += UI_LCD_SPLIT_BLACK_LINES * display_zoom;

    SDL_LockSurface(surface);
    SDL_LockSurface(real_surface);

    int pixel, npixels;

#if LCD_DEPTH != 1
#error "Split screen only works for monochrome displays !"
#endif

    npixels = display_zoom * display_zoom * UI_LCD_SPLIT_LINES * surface->pitch;
    const unsigned char * pixels_src = (const unsigned char*)surface->pixels;
    unsigned char * pixels_dst = (unsigned char*)real_surface->pixels;
    const int start_pixel = UI_LCD_SPLIT_LINES * surface->pitch * display_zoom;
    const int stop_pixel = (UI_LCD_SPLIT_LINES+UI_LCD_SPLIT_BLACK_LINES)
        * surface->pitch * display_zoom;

    /* draw top pixels, change the color */
    for (pixel = 0; pixel < npixels ; pixel++)
    {
        int pix = pixels_src[pixel] + gradient_steps;
        if(pix > 255) pix = 255;

        pixels_dst[pixel] = pix;
    }

    /* copy bottom pixels */
    memcpy(&pixels_dst[stop_pixel], &pixels_src[start_pixel],
          (UI_LCD_HEIGHT - UI_LCD_SPLIT_LINES) * surface->pitch * display_zoom);

    /* separation lines are off */
    for (pixel = start_pixel; pixel < stop_pixel ; pixel++)
        pixels_dst[pixel] = 0;

    SDL_UnlockSurface(surface);
    SDL_UnlockSurface(real_surface);

    SDL_BlitSurface(real_surface, &src, gui_surface, &dest);
#else
    SDL_BlitSurface(surface, &src, gui_surface, &dest);
#endif

    SDL_Flip(gui_surface);
}

/* set a range of bitmap indices to a gradient from startcolour to endcolour */
void sdl_set_gradient(SDL_Surface *surface, SDL_Color *start, SDL_Color *end,
                      IFSPLIT(SDL_Color *split_start,)
                      IFSPLIT(SDL_Color *split_end ,) int first, int steps)
{
    int i;

#ifdef UI_LCD_SPLIT
    int tot_steps = steps * 2;
    if (tot_steps > 256)
        tot_steps = 256;

    gradient_steps = steps;
#else
#define tot_steps steps
#endif

    SDL_Color palette[tot_steps];

    for (i = 0; i < steps; i++) {
        palette[i].r = start->r + (end->r - start->r) * i / (steps - 1);
        palette[i].g = start->g + (end->g - start->g) * i / (steps - 1);
        palette[i].b = start->b + (end->b - start->b) * i / (steps - 1);
    }

#ifdef UI_LCD_SPLIT /* extra color */
    for (i = steps ; i < tot_steps; i++) {
        palette[i].r = split_start->r + (split_end->r - split_start->r) * (i - steps) / (tot_steps - steps - 1);
        palette[i].g = split_start->g + (split_end->g - split_start->g) * (i - steps) / (tot_steps - steps - 1);
        palette[i].b = split_start->b + (split_end->b - split_start->b) * (i - steps) / (tot_steps - steps - 1);
    }
#endif

    SDL_SetPalette(surface, SDL_LOGPAL|SDL_PHYSPAL, palette, first, tot_steps);
}

