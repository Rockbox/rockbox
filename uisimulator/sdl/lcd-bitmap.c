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
#ifdef HAVE_BACKLIGHT
SDL_Color lcd_backlight_color_zero = {UI_LCD_BGCOLORLIGHT, 0};
SDL_Color lcd_backlight_color_max  = {UI_LCD_FGCOLORLIGHT, 0};
#endif
SDL_Color lcd_color_zero = {UI_LCD_BGCOLOR, 0};
SDL_Color lcd_color_max  = {UI_LCD_FGCOLOR, 0};
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
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
    return ((lcd_framebuffer[y/4][x] >> (2 * (y & 3))) & 3);
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
    unsigned bits = (lcd_framebuffer[y/8][x] >> (y & 7)) & 0x0101;
    return (bits | (bits >> 7)) & 3;
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

#ifdef HAVE_BACKLIGHT
void sim_backlight(int value)
{
    lcd_backlight_val = value;

#if LCD_DEPTH <= 8
    if (value > 0) {
        sdl_set_gradient(lcd_surface, &lcd_backlight_color_zero,
                         &lcd_backlight_color_max, 0, (1<<LCD_DEPTH));
    } else {
        sdl_set_gradient(lcd_surface, &lcd_color_zero, &lcd_color_max,
                         0, (1<<LCD_DEPTH));
    }
#if LCD_DEPTH < 8
    if (lcd_ex_shades) {
        if (value > 0) {
            sdl_set_gradient(lcd_surface, &lcd_backlight_color_max,
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
#ifdef HAVE_BACKLIGHT
    sdl_set_gradient(lcd_surface, &lcd_backlight_color_zero, &lcd_color_max,
                     0, (1<<LCD_DEPTH));
#else
    sdl_set_gradient(lcd_surface, &lcd_color_zero, &lcd_color_max, 0, 
                     (1<<LCD_DEPTH));
#endif
#endif
}

#if LCD_DEPTH < 8
void sim_lcd_ex_init(int shades, unsigned long (*getpixel)(int, int))
{
    lcd_ex_shades = shades;
    lcd_ex_getpixel = getpixel;
    if (shades) {
#ifdef HAVE_BACKLIGHT
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

#ifdef HAVE_LCD_COLOR
/**
 * |R|   |1.000000 -0.000001  1.402000| |Y'|
 * |G| = |1.000000 -0.334136 -0.714136| |Pb|
 * |B|   |1.000000  1.772000  0.000000| |Pr|
 * Scaled, normalized, rounded and tweaked to yield RGB 565:
 * |R|   |74   0 101| |Y' -  16| >> 9
 * |G| = |74 -24 -51| |Cb - 128| >> 8
 * |B|   |74 128   0| |Cr - 128| >> 9
 */
#define YFAC    (74)
#define RVFAC   (101)
#define GUFAC   (-24)
#define GVFAC   (-51)
#define BUFAC   (128)

static inline int clamp(int val, int min, int max)
{
    if (val < min)
        val = min;
    else if (val > max)
        val = max;
    return val;
}

void lcd_yuv_set_options(unsigned options)
{
    (void)options;
}

/* Draw a partial YUV colour bitmap - similiar behavior to lcd_blit_yuv
   in the core */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    const unsigned char *ysrc, *usrc, *vsrc;
    int linecounter;
    fb_data *dst, *row_end;
    long z;

    /* width and height must be >= 2 and an even number */
    width &= ~1;
    linecounter = height >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
    dst     = &lcd_framebuffer[y][x];
    row_end = dst + width;
#else
    dst     = &lcd_framebuffer[x][LCD_WIDTH - y - 1];
    row_end = dst + LCD_WIDTH * width;
#endif

    z    = stride * src_y;
    ysrc = src[0] + z + src_x;
    usrc = src[1] + (z >> 2) + (src_x >> 1);
    vsrc = src[2] + (usrc - src[1]);

    /* stride => amount to jump from end of last row to start of next */
    stride -= width;

    /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */

    do
    {
        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;            
#else
            dst += LCD_WIDTH;
#endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;            
#else
            dst += LCD_WIDTH;
#endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    -= width >> 1;
        vsrc    -= width >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
#else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
#endif

        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;            
#else
            dst += LCD_WIDTH;
#endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;            
#else
            dst += LCD_WIDTH;
#endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    += stride >> 1;
        vsrc    += stride >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
#else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
#endif
    }
    while (--linecounter > 0);

#if LCD_WIDTH >= LCD_HEIGHT
    lcd_update_rect(x, y, width, height);
#else
    lcd_update_rect(LCD_WIDTH - y - height, x, height, width);
#endif
}
#endif /* HAVE_LCD_COLOR */
