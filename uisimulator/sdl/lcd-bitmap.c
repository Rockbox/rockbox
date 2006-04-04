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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "debug.h"
#include "uisdl.h"
#include "lcd-sdl.h"

SDL_Surface* lcd_surface;
int lcd_backlight_val;

#if LCD_DEPTH <= 8
SDL_Color lcd_color_zero = {UI_LCD_BGCOLOR, 0};
SDL_Color lcd_backlight_color_zero = {UI_LCD_BGCOLORLIGHT, 0};
SDL_Color lcd_color_max  = {0, 0, 0, 0};
#endif

#if LCD_DEPTH < 8
int lcd_ex_shades = 0;
unsigned long (*lcd_ex_getpixel)(int, int) = NULL;
#endif

static unsigned long get_lcd_pixel(int x, int y)
{
#if LCD_DEPTH == 1
    return ((lcd_framebuffer[y/8][x] >> (y & 7)) & 1);
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    return ((lcd_framebuffer[y][x/4] >> (2 * (~x & 3))) & 3);
#else
    return ((lcd_framebuffer[y/4][x] >> (2 * (y & 3))) & 3);
#endif
#elif LCD_DEPTH == 16
#if LCD_PIXELFORMAT == RGB565SWAPPED
    unsigned bits = lcd_framebuffer[y][x];
    return (bits >> 8) | (bits << 8);
#else
    return lcd_framebuffer[y][x];
#endif
#endif
}

void lcd_update(void)
{
    /* update a full screen rect */
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x_start, int y_start, int width, int height)
{
    sdl_update_rect(lcd_surface, x_start, y_start, width, height, LCD_WIDTH,
                    LCD_HEIGHT, get_lcd_pixel);
    sdl_gui_update(lcd_surface, x_start, y_start, width, height, LCD_WIDTH,
                   LCD_HEIGHT, background ? UI_LCD_POSX : 0, background? UI_LCD_POSY : 0);
}

#ifdef CONFIG_BACKLIGHT
void sim_backlight(int value)
{
    lcd_backlight_val = value;

#if LCD_DEPTH <= 8
    if (value > 0) {
        sdl_set_gradient(lcd_surface, &lcd_backlight_color_zero,
                         &lcd_color_max, 0, (1<<LCD_DEPTH));
    } else {
        sdl_set_gradient(lcd_surface, &lcd_color_zero, &lcd_color_max,
                         0, (1<<LCD_DEPTH));
    }
#if LCD_DEPTH < 8
    if (lcd_ex_shades) {
        if (value > 0) {
            sdl_set_gradient(lcd_surface, &lcd_color_max,
                             &lcd_backlight_color_zero, (1<<LCD_DEPTH),
                             lcd_ex_shades);
        } else {
            sdl_set_gradient(lcd_surface, &lcd_color_max, &lcd_color_zero,
                             (1<<LCD_DEPTH), lcd_ex_shades);
        }
    }
#endif

    sdl_gui_update(lcd_surface, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH,
                   LCD_HEIGHT, background ? UI_LCD_POSX : 0, background? UI_LCD_POSY : 0);

#else
    DEBUGF("backlight: %s\n", (value > 0) ? "on" : "off");
#endif
}
#endif

/* initialise simulator lcd driver */
void sim_lcd_init(void)
{
#if LCD_DEPTH == 16
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_WIDTH * display_zoom,
        LCD_HEIGHT * display_zoom, LCD_DEPTH, 0, 0, 0, 0);
#else
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_WIDTH * display_zoom,
        LCD_HEIGHT * display_zoom, 8, 0, 0, 0, 0);
#endif

#if LCD_DEPTH <= 8
    sdl_set_gradient(lcd_surface, &lcd_backlight_color_zero, &lcd_color_max,
                     0, (1<<LCD_DEPTH));
#endif
}

#if LCD_DEPTH < 8
void sim_lcd_ex_init(int shades, unsigned long (*getpixel)(int, int))
{
    lcd_ex_shades = shades;
    lcd_ex_getpixel = getpixel;
    if (shades) {
#ifdef CONFIG_BACKLIGHT
        if (lcd_backlight_val > 0) {
            sdl_set_gradient(lcd_surface, &lcd_color_max,
                             &lcd_backlight_color_zero, (1<<LCD_DEPTH),
                             shades);
        }
        else 
#endif
        {
            sdl_set_gradient(lcd_surface, &lcd_color_max, &lcd_color_zero,
                             (1<<LCD_DEPTH), shades);
        }
    }
}

void sim_lcd_ex_update_rect(int x_start, int y_start, int width, int height)
{
    if (lcd_ex_getpixel) {
        sdl_update_rect(lcd_surface, x_start, y_start, width, height,
                        LCD_WIDTH, LCD_HEIGHT, lcd_ex_getpixel);
        sdl_gui_update(lcd_surface, x_start, y_start, width, height, LCD_WIDTH,
                       LCD_HEIGHT, background ? UI_LCD_POSX : 0,
                       background? UI_LCD_POSY : 0);
    }
}
#endif

