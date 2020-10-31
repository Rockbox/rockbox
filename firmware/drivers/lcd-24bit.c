/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
 * Copyright (C) 2009 by Karl Kurbjun
 *
 * Rockbox driver for 16-bit colour LCDs
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

#include <stdio.h>
#include "config.h"

#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <stdlib.h>
#include "string-extra.h" /* mem*() */
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "scroll_engine.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#define ROW_INC lcd_current_viewport->buffer->stride
#define COL_INC 1

extern lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_backdrop[];
extern lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_bgcolor[];

static void ICODE_ATTR lcd_alpha_bitmap_part_mix(const fb_data* image,
                                      const unsigned char *src, int src_x,
                                      int src_y, int x, int y,
                                      int width, int height,
                                      int stride_image, int stride_src);

#include "lcd-color-common.c"
#include "lcd-bitmap-common.c"


/* Clear the current viewport */
void lcd_clear_viewport(void)
{
    fb_data *dst, *dst_end;
    int x, y, width, height;
    int len, step;

    x = lcd_current_viewport->x;
    y = lcd_current_viewport->y;
    width = lcd_current_viewport->width;
    height = lcd_current_viewport->height;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    len  = STRIDE_MAIN(width, height);
    step = STRIDE_MAIN(ROW_INC, COL_INC);

    dst = FBADDR(x, y);
    dst_end = FBADDR(x + width - 1 , y + height - 1);

    if (lcd_current_viewport->drawmode & DRMODE_INVERSEVID)
    {
        fb_data px = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
        do
        {
            fb_data *end = dst + len;
            do {
                *dst++ = px;
            } while (dst < end);
            dst += step - len;
        }
        while (dst <= dst_end);
    }
    else
    {
        if (!lcd_backdrop)
        {
            fb_data px = FB_SCALARPACK(lcd_current_viewport->bg_pattern);
            do
            {
                fb_data *end = dst + len;
                do {
                    *dst++ = px;
                } while (dst < end);
                dst += step - len;
            }
            while (dst <= dst_end);
        }
        else
        {
            do
            {
                memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
                       len * sizeof(fb_data));
                dst += step;
            }
            while (dst <= dst_end);
        }
    }

    if (lcd_current_viewport == &default_vp)
        lcd_scroll_stop();
    else
        lcd_scroll_stop_viewport(lcd_current_viewport);

    lcd_current_viewport->flags &= ~(VP_FLAG_VP_SET_CLEAN);
}

/*** low-level drawing functions ***/

static void ICODE_ATTR setpixel(fb_data *address)
{
    *address = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
}

static void ICODE_ATTR clearpixel(fb_data *address)
{
    *address = FB_SCALARPACK(lcd_current_viewport->bg_pattern);
}

static void ICODE_ATTR clearimgpixel(fb_data *address)
{
    *address = *(fb_data *)((long)address + lcd_backdrop_offset);
}

static void ICODE_ATTR flippixel(fb_data *address)
{
    unsigned px = FB_UNPACK_SCALAR_LCD(*address);
    *address = FB_SCALARPACK(~px);
}

static void ICODE_ATTR nopixel(fb_data *address)
{
    (void)address;
}

lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_bgcolor[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_backdrop[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearimgpixel, nopixel, clearimgpixel
};

lcd_fastpixelfunc_type* const * lcd_fastpixelfuncs = lcd_fastpixelfuncs_bgcolor;

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    enum fill_opt fillopt = OPT_NONE;
    fb_data *dst, *dst_end;
    int len, step;
    fb_data bits;
    memset(&bits, 0, sizeof(fb_data));

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= lcd_current_viewport->width) ||
        (y >= lcd_current_viewport->height) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > lcd_current_viewport->width)
        width = lcd_current_viewport->width - x;
    if (y + height > lcd_current_viewport->height)
        height = lcd_current_viewport->height - y;

    /* adjust for viewport */
    x += lcd_current_viewport->x;
    y += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    /* drawmode and optimisation */
    if (lcd_current_viewport->drawmode & DRMODE_INVERSEVID)
    {
        if (lcd_current_viewport->drawmode & DRMODE_BG)
        {
            if (!lcd_backdrop)
            {
                fillopt = OPT_SET;
                bits = FB_SCALARPACK(lcd_current_viewport->bg_pattern);
            }
            else
                fillopt = OPT_COPY;
        }
    }
    else
    {
        if (lcd_current_viewport->drawmode & DRMODE_FG)
        {
            fillopt = OPT_SET;
            bits = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
        }
    }
    if (fillopt == OPT_NONE && lcd_current_viewport->drawmode != DRMODE_COMPLEMENT)
        return;

    dst = FBADDR(x, y);
    dst_end = FBADDR(x + width - 1, y + height - 1);

    len  = STRIDE_MAIN(width, height);
    step = STRIDE_MAIN(ROW_INC, COL_INC);

    do
    {
        switch (fillopt)
        {
          case OPT_SET:
          {
            fb_data *start = dst;
            fb_data *end = start + len;
            do {
                *start = bits;
            } while (++start < end);
            break;
          }

          case OPT_COPY:
            memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
                   len * sizeof(fb_data));
            break;

          case OPT_NONE:  /* DRMODE_COMPLEMENT */
          {
            fb_data *start = dst;
            fb_data *end = start + len;
            do {
                flippixel(start);
            } while (++start < end);
            break;
          }
        }
        dst += step;
    }
    while (dst <= dst_end);
}

/* About Rockbox' internal monochrome bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the mono bitmap format used on all other targets so far; the
 * pixel packing doesn't really matter on a 8bit+ target. */

/* Draw a partial monochrome bitmap */

void ICODE_ATTR lcd_mono_bitmap_part(const unsigned char *src, int src_x,
                                     int src_y, int stride, int x, int y,
                                     int width, int height)
{
    const unsigned char *src_end;
    fb_data *dst, *dst_col;
    unsigned dmask = 0x100; /* bit 8 == sentinel */
    int drmode = lcd_current_viewport->drawmode;
    int row;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= lcd_current_viewport->width) ||
        (y >= lcd_current_viewport->height) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > lcd_current_viewport->width)
        width = lcd_current_viewport->width - x;
    if (y + height > lcd_current_viewport->height)
        height = lcd_current_viewport->height - y;

    /* adjust for viewport */
    x += lcd_current_viewport->x;
    y += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;
    dst_col = FBADDR(x, y);


    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0x1ff;          /* bit 8 == sentinel */
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }

    /* Use extra bit to avoid if () in the switch-cases below */
    if ((drmode & DRMODE_BG) && lcd_backdrop)
        drmode |= DRMODE_INT_BD;

    /* go through each column and update each pixel  */
    do
    {
        const unsigned char *src_col = src++;
        unsigned data = (*src_col ^ dmask) >> src_y;
        fb_data fg, bg;
        uintptr_t bo;

        dst = dst_col;
        dst_col += COL_INC;
        row = height;

#define UPDATE_SRC  do {                  \
            data >>= 1;                   \
            if (data == 0x001) {          \
                src_col += stride;        \
                data = *src_col ^ dmask;  \
            }                             \
        } while (0)

        switch (drmode)
        {
          case DRMODE_COMPLEMENT:
            do
            {
                if (data & 0x01)
                    flippixel(dst);

                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

          case DRMODE_BG|DRMODE_INT_BD:
            bo = lcd_backdrop_offset;
            do
            {
                if (!(data & 0x01))
                    *dst = *(fb_data *)((long)dst + bo);

                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

        case DRMODE_BG:
            bg = FB_SCALARPACK(lcd_current_viewport->bg_pattern);
            do
            {
                if (!(data & 0x01))
                    *dst = bg;

                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

          case DRMODE_FG:
            fg = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
            do
            {
                if (data & 0x01)
                    *dst = fg;

                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

          case DRMODE_SOLID|DRMODE_INT_BD:
            fg = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
            bo = lcd_backdrop_offset;
            do
            {
                *dst = (data & 0x01) ? fg
                           : *(fb_data *)((long)dst + bo);
                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;

          case DRMODE_SOLID:
            fg = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
            bg = FB_SCALARPACK(lcd_current_viewport->bg_pattern);
            do
            {
                *dst = (data & 0x01) ? fg : bg;
                dst += ROW_INC;
                UPDATE_SRC;
            }
            while (--row);
            break;
        }
    }
    while (src < src_end);
}
/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}


/* About Rockbox' internal alpha channel format (for ALPHA_COLOR_FONT_DEPTH == 2)
 *
 * For each pixel, 4bit of alpha information is stored in a byte-stream,
 * so two pixels are packed into one byte.
 * The lower nibble is the first pixel, the upper one the second. The stride is
 * horizontal. E.g row0: pixel0: byte0[0:3], pixel1: byte0[4:7], pixel2: byte1[0:3],...
 * The format is independant of the internal display orientation and color
 * representation, as to support the same font files on all displays.
 * The values go linear from 0 (fully opaque) to 15 (fully transparent)
 * (note how this is the opposite of the alpha channel in the ARGB format).
 *
 * This might suggest that rows need to have an even number of pixels.
 * However this is generally not the case. lcd_alpha_bitmap_part_mix() can deal
 * with uneven colums (i.e. two rows can share one byte). And font files do
 * exploit this.
 * However, this is difficult to do for image files, especially bottom-up bitmaps,
 * so lcd_bmp() do expect even rows.
 */

#define ALPHA_COLOR_FONT_DEPTH 2
#define ALPHA_COLOR_LOOKUP_SHIFT (1 << ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_LOOKUP_SIZE ((1 << ALPHA_COLOR_LOOKUP_SHIFT) - 1)
#define ALPHA_COLOR_PIXEL_PER_BYTE (8 >> ALPHA_COLOR_FONT_DEPTH)
#define ALPHA_COLOR_PIXEL_PER_WORD (32 >> ALPHA_COLOR_FONT_DEPTH)

/* This is based on SDL (src/video/SDL_RLEaccel.c) ALPHA_BLIT32_888() macro */
static inline fb_data blend_two_colors(unsigned c1, unsigned c2, unsigned a)
{
    unsigned s = c1;
    unsigned d = c2;
    unsigned s1 = s & 0xff00ff;
    unsigned d1 = d & 0xff00ff;
    a += a >> (ALPHA_COLOR_LOOKUP_SHIFT - 1);
    d1 = (d1 + ((s1 - d1) * a >> ALPHA_COLOR_LOOKUP_SHIFT)) & 0xff00ff;
    s &= 0xff00;
    d &= 0xff00;
    d = (d + ((s - d) * a >> ALPHA_COLOR_LOOKUP_SHIFT)) & 0xff00;

    return FB_SCALARPACK(d1 | d);
}

/* Blend an image with an alpha channel
 * if image is NULL, drawing will happen according to the drawmode
 * src is the alpha channel (4bit per pixel) */
static void ICODE_ATTR lcd_alpha_bitmap_part_mix(const fb_data* image,
                                      const unsigned char *src, int src_x,
                                      int src_y, int x, int y,
                                      int width, int height,
                                      int stride_image, int stride_src)
{
    fb_data *dst, *dst_row;
    unsigned dmask = 0x00000000;
    int drmode = lcd_current_viewport->drawmode;
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= lcd_current_viewport->width) ||
         (y >= lcd_current_viewport->height) || (x + width <= 0) || (y + height <= 0))
        return;

    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > lcd_current_viewport->width)
        width = lcd_current_viewport->width - x;
    if (y + height > lcd_current_viewport->height)
        height = lcd_current_viewport->height - y;

    /* adjust for viewport */
    x += lcd_current_viewport->x;
    y += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
            || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    /* the following drawmode combinations are possible:
     * 1) COMPLEMENT: just negates the framebuffer contents
     * 2) BG and BG+backdrop: draws _only_ background pixels with either
     *    the background color or the backdrop (if any). The backdrop
     *    is an image in native lcd format
     * 3) FG and FG+image: draws _only_ foreground pixels with either
     *    the foreground color or an image buffer. The image is in
     *    native lcd format
     * 4) SOLID, SOLID+backdrop, SOLID+image, SOLID+backdrop+image, i.e. all
     *    possible combinations of 2) and 3). Draws both, fore- and background,
     *    pixels. The rules of 2) and 3) apply.
     *
     * INVERSEVID swaps fore- and background pixels, i.e. background pixels
     * become foreground ones and vice versa.
     */
    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0xffffffff;
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }

    /* Use extra bits to avoid if () in the switch-cases below */
    if (image != NULL)
        drmode |= DRMODE_INT_IMG;

    if ((drmode & DRMODE_BG) && lcd_backdrop)
        drmode |= DRMODE_INT_BD;

    dst_row = FBADDR(x, y);

    int col, row = height;
    unsigned data, pixels;
    unsigned skip_end = (stride_src - width);
    unsigned skip_start = src_y * stride_src + src_x;
    unsigned skip_start_image = STRIDE_MAIN(src_y * stride_image + src_x,
                                            src_x * stride_image + src_y);

#ifdef ALPHA_BITMAP_READ_WORDS
    uint32_t *src_w = (uint32_t *)((uintptr_t)src & ~3);
    skip_start += ALPHA_COLOR_PIXEL_PER_BYTE * ((uintptr_t)src & 3);
    src_w += skip_start / ALPHA_COLOR_PIXEL_PER_WORD;
    data = letoh32(*src_w++) ^ dmask;
    pixels = skip_start % ALPHA_COLOR_PIXEL_PER_WORD;
#else
    src += skip_start / ALPHA_COLOR_PIXEL_PER_BYTE;
    data = *src ^ dmask;
    pixels = skip_start % ALPHA_COLOR_PIXEL_PER_BYTE;
#endif
    data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
#ifdef ALPHA_BITMAP_READ_WORDS
    pixels = 8 - pixels;
#endif

    /* image is only accessed in DRMODE_INT_IMG cases, i.e. when non-NULL.
     * Therefore NULL accesses are impossible and we can increment
     * unconditionally (applies for stride at the end of the loop as well) */
    image += skip_start_image;
    /* go through the rows and update each pixel */
    do
    {
        /* saving lcd_current_viewport->fg/bg_pattern and lcd_backdrop_offset into these
         * temp vars just before the loop helps gcc to opimize the loop better
         * (testing showed ~15% speedup) */
        unsigned fg, bg;
        ptrdiff_t bo, img_offset;
        col = width;
        dst = dst_row;
        dst_row += ROW_INC;
#ifdef ALPHA_BITMAP_READ_WORDS
#define UPDATE_SRC_ALPHA    do { \
            if (--pixels) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
            { \
                data = letoh32(*src_w++) ^ dmask; \
                pixels = ALPHA_COLOR_PIXEL_PER_WORD; \
            } \
        } while (0)
#elif ALPHA_COLOR_PIXEL_PER_BYTE == 2
#define UPDATE_SRC_ALPHA    do { \
            if (pixels ^= 1) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
                data = *(++src) ^ dmask; \
        } while (0)
#else
#define UPDATE_SRC_ALPHA    do { \
            if (pixels = (++pixels % ALPHA_COLOR_PIXEL_PER_BYTE)) \
                data >>= ALPHA_COLOR_LOOKUP_SHIFT; \
            else \
                data = *(++src) ^ dmask; \
        } while (0)
#endif

        switch (drmode)
        {
            case DRMODE_COMPLEMENT:
                do
                {
                    unsigned px = FB_UNPACK_SCALAR_LCD(*dst);
                    *dst = blend_two_colors(px, ~px,
                                data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_BG|DRMODE_INT_BD:
                bo = lcd_backdrop_offset;
                do
                {
                    unsigned px = FB_UNPACK_SCALAR_LCD(*dst);
                    unsigned c = FB_UNPACK_SCALAR_LCD(*(fb_data *)((uintptr_t)dst +  bo));
                    *dst = blend_two_colors(c, px, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    image += STRIDE_MAIN(1, stride_image);
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_BG:
                bg = lcd_current_viewport->bg_pattern;
                do
                {
                    unsigned px = FB_UNPACK_SCALAR_LCD(*dst);
                    *dst = blend_two_colors(bg, px, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_FG|DRMODE_INT_IMG:
                img_offset = image - dst;
                do
                {
                    unsigned px1 = FB_UNPACK_SCALAR_LCD(*dst);
                    unsigned px2 = FB_UNPACK_SCALAR_LCD(*(dst + img_offset));
                    *dst = blend_two_colors(px1, px2, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_FG:
                fg = lcd_current_viewport->fg_pattern;
                do
                {
                    unsigned px = FB_UNPACK_SCALAR_LCD(*dst);
                    *dst = blend_two_colors(px, fg, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_SOLID|DRMODE_INT_BD:
                bo = lcd_backdrop_offset;
                fg = lcd_current_viewport->fg_pattern;
                do
                {
                    unsigned c = FB_UNPACK_SCALAR_LCD(*(fb_data *)((uintptr_t)dst +  bo));
                    *dst = blend_two_colors(c, fg, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_SOLID|DRMODE_INT_IMG:
                bg = lcd_current_viewport->bg_pattern;
                img_offset = image - dst;
                do
                {
                    unsigned c = FB_UNPACK_SCALAR_LCD(*(dst + img_offset));
                    *dst = blend_two_colors(bg, c, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_SOLID|DRMODE_INT_BD|DRMODE_INT_IMG:
                bo = lcd_backdrop_offset;
                img_offset = image - dst;
                do
                {
                    unsigned px = FB_UNPACK_SCALAR_LCD(*(fb_data *)((uintptr_t)dst +  bo));
                    unsigned c = FB_UNPACK_SCALAR_LCD(*(dst + img_offset));
                    *dst = blend_two_colors(px, c, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
            case DRMODE_SOLID:
                bg = lcd_current_viewport->bg_pattern;
                fg = lcd_current_viewport->fg_pattern;
                do
                {
                    *dst = blend_two_colors(bg, fg, data & ALPHA_COLOR_LOOKUP_SIZE );
                    dst += COL_INC;
                    UPDATE_SRC_ALPHA;
                }
                while (--col);
                break;
        }
#ifdef ALPHA_BITMAP_READ_WORDS
        if (skip_end < pixels)
        {
            pixels -= skip_end;
            data >>= skip_end * ALPHA_COLOR_LOOKUP_SHIFT;
        } else {
            pixels = skip_end - pixels;
            src_w += pixels / ALPHA_COLOR_PIXEL_PER_WORD;
            pixels %= ALPHA_COLOR_PIXEL_PER_WORD;
            data = letoh32(*src_w++) ^ dmask;
            data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
            pixels = 8 - pixels;
        }
#else
        if (skip_end)
        {
            pixels += skip_end;
            if (pixels >= ALPHA_COLOR_PIXEL_PER_BYTE)
            {
                src += pixels / ALPHA_COLOR_PIXEL_PER_BYTE;
                pixels %= ALPHA_COLOR_PIXEL_PER_BYTE;
                data = *src ^ dmask;
                data >>= pixels * ALPHA_COLOR_LOOKUP_SHIFT;
            } else
                data >>= skip_end * ALPHA_COLOR_LOOKUP_SHIFT;
        }
#endif

        image += STRIDE_MAIN(stride_image,1);
    } while (--row);
}

/*** drawing functions ***/

/* Draw a horizontal line (optimised) */
void lcd_hline(int x1, int x2, int y)
{
    int x, width;
    fb_data *dst, *dst_end;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[lcd_current_viewport->drawmode];

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)lcd_current_viewport->height) ||
        (x1 >= lcd_current_viewport->width) ||
        (x2 < 0))
        return;

    if (x1 < 0)
        x1 = 0;
    if (x2 >= lcd_current_viewport->width)
        x2 = lcd_current_viewport->width-1;

    /* Adjust x1 and y to viewport */
    x1 += lcd_current_viewport->x;
    x2 += lcd_current_viewport->x;
    y += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if (((unsigned)y >= (unsigned) LCD_HEIGHT) || (x1 >= LCD_WIDTH)
        || (x2 < 0))
        return;

    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= LCD_WIDTH)
        x2 = LCD_WIDTH-1;
#endif

    width = x2 - x1 + 1;

    dst = FBADDR(x1 , y);
    dst_end = dst + width;
    do
    {
        pfunc(dst);
    }
    while (++dst < dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    int y;
    fb_data *dst, *dst_end;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[lcd_current_viewport->drawmode];

    /* direction flip */
    if (y2 < y1)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)lcd_current_viewport->width) ||
        (y1 >= lcd_current_viewport->height) ||
        (y2 < 0))
        return;

    if (y1 < 0)
        y1 = 0;
    if (y2 >= lcd_current_viewport->height)
        y2 = lcd_current_viewport->height-1;

    /* adjust for viewport */
    x += lcd_current_viewport->x;
    y1 += lcd_current_viewport->y;
    y2 += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if (( (unsigned) x >= (unsigned)LCD_WIDTH) || (y1 >= LCD_HEIGHT)
        || (y2 < 0))
        return;

    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= LCD_HEIGHT)
        y2 = LCD_HEIGHT-1;
#endif

    dst = FBADDR(x , y1);
    dst_end = dst + (y2 - y1) * LCD_WIDTH;

    do
    {
        pfunc(dst);
        dst += LCD_WIDTH;
    }
    while (dst <= dst_end);
}

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const fb_data *src, int src_x, int src_y,
                                int stride, int x, int y, int width,
                                int height)
{
    fb_data *dst;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= lcd_current_viewport->width) ||
        (y >= lcd_current_viewport->height) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }

    if (x + width > lcd_current_viewport->width)
        width = lcd_current_viewport->width - x;
    if (y + height > lcd_current_viewport->height)
        height = lcd_current_viewport->height - y;

    /* adjust for viewport */
    x += lcd_current_viewport->x;
    y += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    src += stride * src_y + src_x; /* move starting point */
    dst = FBADDR(x, y);

    do
    {
        memcpy(dst, src, width * sizeof(fb_data));
        src += stride;
        dst += LCD_WIDTH;
    }
    while (--height > 0);
}

/* Draw a partial native bitmap with transparency and foreground colors */
void ICODE_ATTR lcd_bitmap_transparent_part(const fb_data *src, int src_x,
                                            int src_y, int stride, int x,
                                            int y, int width, int height)
{
    fb_data *dst;
    fb_data fg, transparent, replacewithfg;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= lcd_current_viewport->width) ||
        (y >= lcd_current_viewport->height) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }

    if (x + width > lcd_current_viewport->width)
        width = lcd_current_viewport->width - x;
    if (y + height > lcd_current_viewport->height)
        height = lcd_current_viewport->height - y;

    /* adjust for viewport */
    x += lcd_current_viewport->x;
    y += lcd_current_viewport->y;

#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)
        || (x + width <= 0) || (y + height <= 0))
        return;

    /* clip image in viewport in screen */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
#endif

    src += stride * src_y + src_x; /* move starting point */
    dst = FBADDR(x, y);

    transparent = FB_SCALARPACK(TRANSPARENT_COLOR);
    replacewithfg = FB_SCALARPACK(REPLACEWITHFG_COLOR);
    fg = FB_SCALARPACK(lcd_current_viewport->fg_pattern);
#define CMP(c1, c2) (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b)

    do
    {
        const fb_data *src_row = src;
        fb_data *dst_row = dst;
        fb_data *row_end = dst_row + width;
        do
        {
            fb_data data = *src_row++;
            if (!CMP(data, transparent))
            {
                if (CMP(data, replacewithfg))
                    data = fg;
                *dst_row = data;
            }
        }
        while (++dst_row < row_end);
        src += stride;
        dst += LCD_WIDTH;
    }
    while (--height > 0);
}
