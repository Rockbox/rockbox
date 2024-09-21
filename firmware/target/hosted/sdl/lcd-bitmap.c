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
#include "sim-ui-defines.h"
#include "system.h"
#include "button-sdl.h"
#include "lcd-sdl.h"
#include "screendump.h"
#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
#include "maemo-thread.h"
#endif

SDL_Surface* lcd_surface;

#if LCD_DEPTH <= 8
#ifdef HAVE_BACKLIGHT
SDL_Color lcd_bl_color_dark    = {RED_CMP(LCD_BL_DARKCOLOR),
                                  GREEN_CMP(LCD_BL_DARKCOLOR),
                                  BLUE_CMP(LCD_BL_DARKCOLOR), 0};
SDL_Color lcd_bl_color_bright  = {RED_CMP(LCD_BL_BRIGHTCOLOR),
                                  GREEN_CMP(LCD_BL_BRIGHTCOLOR),
                                  BLUE_CMP(LCD_BL_BRIGHTCOLOR), 0};
#ifdef HAVE_LCD_SPLIT
SDL_Color lcd_bl_color2_dark   = {RED_CMP(LCD_BL_DARKCOLOR_2),
                                  GREEN_CMP(LCD_BL_DARKCOLOR_2),
                                  BLUE_CMP(LCD_BL_DARKCOLOR_2), 0};
SDL_Color lcd_bl_color2_bright = {RED_CMP(LCD_BL_BRIGHTCOLOR_2),
                                  GREEN_CMP(LCD_BL_BRIGHTCOLOR_2),
                                  BLUE_CMP(LCD_BL_BRIGHTCOLOR_2), 0};
#endif
#endif /* HAVE_BACKLIGHT */
SDL_Color lcd_color_dark    = {RED_CMP(LCD_DARKCOLOR),
                               GREEN_CMP(LCD_DARKCOLOR),
                               BLUE_CMP(LCD_DARKCOLOR), 0};
SDL_Color lcd_color_bright  = {RED_CMP(LCD_BRIGHTCOLOR),
                               GREEN_CMP(LCD_BRIGHTCOLOR),
                               BLUE_CMP(LCD_BRIGHTCOLOR), 0};
#ifdef HAVE_LCD_SPLIT
SDL_Color lcd_color2_dark   = {RED_CMP(LCD_DARKCOLOR_2),
                               GREEN_CMP(LCD_DARKCOLOR_2),
                               BLUE_CMP(LCD_DARKCOLOR_2), 0};
SDL_Color lcd_color2_bright = {RED_CMP(LCD_BRIGHTCOLOR_2),
                               GREEN_CMP(LCD_BRIGHTCOLOR_2),
                               BLUE_CMP(LCD_BRIGHTCOLOR_2), 0};
#endif

#ifdef HAVE_LCD_SPLIT
#define NUM_SHADES    128
#else
#define NUM_SHADES    129
#endif

#else /* LCD_DEPTH > 8 */

#ifdef HAVE_TRANSFLECTIVE_LCD
#define BACKLIGHT_OFF_ALPHA 85 /* 1/3 brightness */
#else
#define BACKLIGHT_OFF_ALPHA 0  /* pitch black */
#endif

#endif /* LCD_DEPTH */

#if LCD_DEPTH < 8
unsigned long (*lcd_ex_getpixel)(int, int) = NULL;
#endif /* LCD_DEPTH < 8 */

#if LCD_DEPTH == 2
/* Only defined for positive, non-split LCD for now */
static const unsigned char colorindex[4] = {128, 85, 43, 0};
#endif

static unsigned long get_lcd_pixel(int x, int y)
{
#if LCD_DEPTH == 1
#ifdef HAVE_NEGATIVE_LCD
    return (*FBADDR(x, y/8) & (1 << (y & 7))) ? (NUM_SHADES-1) : 0;
#else
    return (*FBADDR(x, y/8) & (1 << (y & 7))) ? 0 : (NUM_SHADES-1);
#endif
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    return colorindex[(*FBADDR(x/4, y) >> (2 * (~x & 3))) & 3];
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
    return colorindex[(*FBADDR(x, y/4) >> (2 * (y & 3))) & 3];
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
    unsigned bits = (*FBADDR(x, y/8) >> (y & 7)) & 0x0101;
    return colorindex[(bits | (bits >> 7)) & 3];
#endif
#elif LCD_DEPTH == 16
#if LCD_PIXELFORMAT == RGB565SWAPPED
    unsigned bits = *FBADDR(x, y);
    return (bits >> 8) | (bits << 8);
#else
    return *FBADDR(x, y);
#endif
#elif LCD_DEPTH >= 24
    return FB_UNPACK_SCALAR_LCD(*FBADDR(x, y));
#endif
}

void lcd_update(void)
{
    /* update a full screen rect */
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x_start, int y_start, int width, int height)
{
#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
    /* Don't update display if not shown */
    if (!maemo_display_on)
        return;

    /* Don't update if we don't have the input focus */
    if (!sdl_app_has_input_focus)
        return;
#endif

    sdl_update_rect(lcd_surface, x_start, y_start, width, height,
                    LCD_WIDTH, LCD_HEIGHT, get_lcd_pixel);
    sdl_gui_update(lcd_surface, x_start, y_start, width,
                   height + LCD_SPLIT_LINES, SIM_LCD_WIDTH, SIM_LCD_HEIGHT,
                   background ? UI_LCD_POSX : 0, background? UI_LCD_POSY : 0);
}

#ifdef HAVE_BACKLIGHT
void sim_backlight(int value)
{
#if LCD_DEPTH <= 8
    if (value > 0) {
        sdl_set_gradient(lcd_surface, &lcd_bl_color_dark,
                         &lcd_bl_color_bright, 0, NUM_SHADES);
#ifdef HAVE_LCD_SPLIT
        sdl_set_gradient(lcd_surface, &lcd_bl_color2_dark,
                         &lcd_bl_color2_bright, NUM_SHADES, NUM_SHADES);
#endif
    } else {
        sdl_set_gradient(lcd_surface, &lcd_color_dark,
                         &lcd_color_bright, 0, NUM_SHADES);
#ifdef HAVE_LCD_SPLIT
        sdl_set_gradient(lcd_surface, &lcd_color2_dark,
                         &lcd_color2_bright, NUM_SHADES, NUM_SHADES);
#endif
    }
#else /* LCD_DEPTH > 8 */
#if defined(HAVE_TRANSFLECTIVE_LCD) && defined(HAVE_LCD_SLEEP)
    if (!lcd_active())
        SDL_SetSurfaceAlphaMod(lcd_surface, 0);
    else
        SDL_SetSurfaceAlphaMod(lcd_surface,
                        MAX(BACKLIGHT_OFF_ALPHA, (value * 255) / 100));
#else
    SDL_SetSurfaceAlphaMod(lcd_surface, (value * 255) / 100);
#endif
#endif /* LCD_DEPTH */

    sdl_gui_update(lcd_surface, 0, 0, SIM_LCD_WIDTH, SIM_LCD_HEIGHT,
                   SIM_LCD_WIDTH, SIM_LCD_HEIGHT,
                   background ? UI_LCD_POSX : 0, background? UI_LCD_POSY : 0);
}
#endif /* HAVE_BACKLIGHT */

/* initialise simulator lcd driver */
void lcd_init_device(void)
{
#if LCD_DEPTH >= 16
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                       SIM_LCD_WIDTH * display_zoom,
                                       SIM_LCD_HEIGHT * display_zoom,
                                       LCD_DEPTH, 0, 0, 0, 0);
    SDL_SetSurfaceBlendMode(lcd_surface, SDL_BLENDMODE_BLEND);
#elif LCD_DEPTH <= 8
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                       SIM_LCD_WIDTH * display_zoom,
                                       SIM_LCD_HEIGHT * display_zoom,
                                       8, 0, 0, 0, 0);

#ifdef HAVE_BACKLIGHT
    sdl_set_gradient(lcd_surface, &lcd_bl_color_dark,
                     &lcd_bl_color_bright, 0, NUM_SHADES);
#ifdef HAVE_LCD_SPLIT
    sdl_set_gradient(lcd_surface, &lcd_bl_color2_dark,
                     &lcd_bl_color2_bright, NUM_SHADES, NUM_SHADES);
#endif
#else /* !HAVE_BACKLIGHT */
    sdl_set_gradient(lcd_surface, &lcd_color_dark,
                     &lcd_color_bright, 0, NUM_SHADES);
#ifdef HAVE_LCD_SPLIT
    sdl_set_gradient(lcd_surface, &lcd_color2_dark,
                     &lcd_color2_bright, NUM_SHADES, NUM_SHADES);
#endif
#endif /* !HAVE_BACKLIGHT */
#endif /* LCD_DEPTH */
}

#if LCD_DEPTH < 8
void sim_lcd_ex_init(unsigned long (*getpixel)(int, int))
{
    lcd_ex_getpixel = getpixel;
}

void sim_lcd_ex_update_rect(int x_start, int y_start, int width, int height)
{
    if (lcd_ex_getpixel) {
        sdl_update_rect(lcd_surface, x_start, y_start, width, height,
                        LCD_WIDTH, LCD_HEIGHT, lcd_ex_getpixel);
        sdl_gui_update(lcd_surface, x_start, y_start, width,
                       height + LCD_SPLIT_LINES, SIM_LCD_WIDTH, SIM_LCD_HEIGHT,
                       background ? UI_LCD_POSX : 0,
                       background ? UI_LCD_POSY : 0);
    }
}
#endif
