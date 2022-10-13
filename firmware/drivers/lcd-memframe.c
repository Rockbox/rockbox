/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2007, 2011 Michael Sevakis
 *
 * Shared C code for memory framebuffer LCDs
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
#include "config.h"
#include "system.h"
#include "lcd.h"
#include "lcd-target.h"

/*** Misc. functions ***/
bool lcd_on SHAREDBSS_ATTR = false;

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return lcd_on;
}

/* For use by target driver only! */
void lcd_set_active(bool active)
{
    lcd_on = active;
}

#else
#define lcd_on true
#endif

#ifndef lcd_write_enabled
#define lcd_write_enabled() lcd_on
#endif


/*** Blitting functions ***/

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

#ifndef LCD_OPTIMIZED_UPDATE
/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_write_enabled())
        return;

    /* Copy the Rockbox framebuffer to the second framebuffer */
    lcd_copy_buffer_rect(LCD_FRAMEBUF_ADDR(0, 0), FBADDR(0,0),
                         LCD_WIDTH*LCD_HEIGHT, 1);
}
#endif /* LCD_OPTIMIZED_UPDATE */

#ifndef LCD_OPTIMIZED_UPDATE_RECT
/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (!lcd_write_enabled())
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    dst = LCD_FRAMEBUF_ADDR(x, y);
    src = FBADDR(x,y);

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
}
#endif /* LCD_OPTIMIZED_UPDATE_RECT */


/*** YUV functions ***/
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;


/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

#ifndef LCD_OPTIMIZED_BLIT_YUV
/* Performance function to blit a YUV bitmap directly to the LCD
 * src_x, src_y, width and height should be even and within the LCD's
 * boundaries.
 *
 * For portrait LCDs, show it rotated counterclockwise by 90 degrees
 */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Macrofy the bits that change between orientations */
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    #define LCD_FRAMEBUF_ADDR_ORIENTED(col, row) \
        LCD_FRAMEBUF_ADDR(row, col)
    #define lcd_write_yuv420_lines_odither_oriented(dst, src, w, s, col, row) \
        lcd_write_yuv420_lines_odither(dst, src, w, s, row, col)
    #define YUV_NEXTLINE() dst -= 2
    #define YUV_DITHER_NEXTLINE() dst -= 2, y -= 2
#else
    #define LCD_FRAMEBUF_ADDR_ORIENTED(col, row) \
        LCD_FRAMEBUF_ADDR(col, row)
    #define lcd_write_yuv420_lines_odither_oriented(dst, src, w, s, col, row) \
        lcd_write_yuv420_lines_odither(dst, src, w, s, col, row)
    #define YUV_NEXTLINE() dst += 2*LCD_FBWIDTH
    #define YUV_DITHER_NEXTLINE() dst += 2*LCD_FBWIDTH, y += 2
#endif

    if (!lcd_write_enabled())
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    /* Adjust portrait coordinates to make (0, 0) the upper right corner */
    y = LCD_WIDTH - 1 - y;
#endif

    fb_data *dst = LCD_FRAMEBUF_ADDR_ORIENTED(x, y);
    int z = stride*src_y;

    unsigned char const * yuv_src[3];
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither_oriented(dst, yuv_src, width,
                                                    stride, x, y);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            YUV_DITHER_NEXTLINE();
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_write_yuv420_lines(dst, yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            YUV_NEXTLINE();
        }
        while (--height > 0);
    }
}
#endif /* LCD_OPTIMIZED_BLIT_YUV */
