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
#include "config.h"

#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "scroll_engine.h"

enum fill_opt {
    OPT_NONE = 0,
    OPT_SET,
    OPT_COPY
};

/*** globals ***/
fb_data lcd_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH]
    IRAM_LCDFRAMEBUFFER CACHEALIGN_AT_LEAST_ATTR(16);


static fb_data* lcd_backdrop = NULL;
static long lcd_backdrop_offset IDATA_ATTR = 0;

#ifdef HAVE_LCD_ENABLE
static void (*lcd_enable_hook)(void) = NULL;
#endif

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCD_WIDTH,
    .height   = LCD_HEIGHT,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = LCD_DEFAULT_BG,
    .lss_pattern = LCD_DEFAULT_BG,
    .lse_pattern = LCD_DEFAULT_BG,
    .lst_pattern = LCD_DEFAULT_BG,
};

/* The Gigabeat target build requires access to the current fg_pattern
   in lcd-meg-fx.c */
#if (!defined(TOSHIBA_GIGABEAT_F)&& !defined(TOSHIBA_GIGABEAT_S)) || defined(SIMULATOR)
static struct viewport* current_vp IDATA_ATTR = &default_vp;
#else
struct viewport* current_vp IDATA_ATTR = &default_vp;
#endif

/* LCD init */
void lcd_init(void)
{
    lcd_clear_display();

    /* Call device specific init */
    lcd_init_device();
    scroll_init();
}

/*** Helpers - consolidate optional code ***/
#ifdef HAVE_LCD_ENABLE
void lcd_set_enable_hook(void (*enable_hook)(void))
{
    lcd_enable_hook = enable_hook;
}

/* To be called by target driver after enabling display and refreshing it */
void lcd_call_enable_hook(void)
{
   void (*enable_hook)(void) = lcd_enable_hook;

    if (enable_hook != NULL)
        enable_hook();
}
#endif

/*** Viewports ***/

void lcd_set_viewport(struct viewport* vp)
{
    if (vp == NULL)
        current_vp = &default_vp;
    else
        current_vp = vp;
}

void lcd_update_viewport(void)
{
    lcd_update_rect(current_vp->x, current_vp->y,
                    current_vp->width, current_vp->height);
}

void lcd_update_viewport_rect(int x, int y, int width, int height)
{
    lcd_update_rect(current_vp->x + x, current_vp->y + y, width, height);
}

/*** parameter handling ***/

void lcd_set_drawmode(int mode)
{
    current_vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_get_drawmode(void)
{
    return current_vp->drawmode;
}

void lcd_set_foreground(unsigned color)
{
    current_vp->fg_pattern = color;
}

unsigned lcd_get_foreground(void)
{
    return current_vp->fg_pattern;
}

void lcd_set_background(unsigned color)
{
    current_vp->bg_pattern = color;
}

unsigned lcd_get_background(void)
{
    return current_vp->bg_pattern;
}

void lcd_set_selector_start(unsigned color)
{
    current_vp->lss_pattern = color;
}

void lcd_set_selector_end(unsigned color)
{
    current_vp->lse_pattern = color;
}

void lcd_set_selector_text(unsigned color)
{
    current_vp->lst_pattern = color;
}

void lcd_set_drawinfo(int mode, unsigned fg_color, unsigned bg_color)
{
    lcd_set_drawmode(mode);
    current_vp->fg_pattern = fg_color;
    current_vp->bg_pattern = bg_color;
}

int lcd_getwidth(void)
{
    return current_vp->width;
}

int lcd_getheight(void)
{
    return current_vp->height;
}

void lcd_setfont(int newfont)
{
    current_vp->font = newfont;
}

int lcd_getfont(void)
{
    return current_vp->font;
}

int lcd_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, current_vp->font);
}

/*** low-level drawing functions ***/

#define LCDADDR(x, y) (&lcd_framebuffer[(y)][(x)])

static void ICODE_ATTR setpixel(fb_data *address)
{
    *address = current_vp->fg_pattern;
}

static void ICODE_ATTR clearpixel(fb_data *address)
{
    *address = current_vp->bg_pattern;
}

static void ICODE_ATTR clearimgpixel(fb_data *address)
{
    *address = *(fb_data *)((long)address + lcd_backdrop_offset);
}

static void ICODE_ATTR flippixel(fb_data *address)
{
    *address = ~(*address);
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

void lcd_set_backdrop(fb_data* backdrop)
{
    lcd_backdrop = backdrop;
    if (backdrop)
    {
        lcd_backdrop_offset = (long)backdrop - (long)&lcd_framebuffer[0][0];
        lcd_fastpixelfuncs = lcd_fastpixelfuncs_backdrop;
    }
    else
    {
        lcd_backdrop_offset = 0;
        lcd_fastpixelfuncs = lcd_fastpixelfuncs_bgcolor;
    }
}

fb_data* lcd_get_backdrop(void)
{
    return lcd_backdrop;
}

/*** drawing functions ***/

/* Clear the current viewport */
void lcd_clear_viewport(void)
{
    fb_data *dst, *dst_end;

    dst = LCDADDR(current_vp->x, current_vp->y);
    dst_end = dst + current_vp->height * LCD_WIDTH;

    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        do
        {
            memset16(dst, current_vp->fg_pattern, current_vp->width);
            dst += LCD_WIDTH;
        }
        while (dst < dst_end);
    }
    else
    {
        if (!lcd_backdrop)
        {
            do
            {
                memset16(dst, current_vp->bg_pattern, current_vp->width);
                dst += LCD_WIDTH;
            }
            while (dst < dst_end);
        }
        else
        {
            do
            {
                memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
                       current_vp->width * sizeof(fb_data));
                dst += LCD_WIDTH;
            }
            while (dst < dst_end);
        }
    }

    if (current_vp == &default_vp)
    {
        lcd_scroll_info.lines = 0;
    }
    else
    {
        lcd_scroll_stop(current_vp);
    }
}

/* Clear the whole display */
void lcd_clear_display(void)
{
    struct viewport* old_vp = current_vp;

    current_vp = &default_vp;

    lcd_clear_viewport();

    current_vp = old_vp;
}

/* Set a single pixel */
void lcd_drawpixel(int x, int y)
{
    if (((unsigned)x < (unsigned)current_vp->width) && 
        ((unsigned)y < (unsigned)current_vp->height))
        lcd_fastpixelfuncs[current_vp->drawmode](LCDADDR(current_vp->x+x, current_vp->y+y));
}

/* Draw a line */
void lcd_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[current_vp->drawmode];

    deltay = abs(y2 - y1);
    if (deltay == 0)
    {
        DEBUGF("lcd_drawline() called for horizontal line - optimisation.\n");
        lcd_hline(x1, x2, y1);
        return;
    }
    deltax = abs(x2 - x1);
    if (deltax == 0)
    {
        DEBUGF("lcd_drawline() called for vertical line - optimisation.\n");
        lcd_vline(x1, y1, y2);
        return;
    }
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i = 0; i < numpixels; i++)
    {
        if (((unsigned)x < (unsigned)current_vp->width) && ((unsigned)y < (unsigned)current_vp->height))
            pfunc(LCDADDR(x + current_vp->x, y + current_vp->y));

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Draw a horizontal line (optimised) */
void lcd_hline(int x1, int x2, int y)
{
    int x, width;
    unsigned bits = 0;
    enum fill_opt fillopt = OPT_NONE;
    fb_data *dst, *dst_end;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[current_vp->drawmode];

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)current_vp->height) || 
        (x1 >= current_vp->width) ||
        (x2 < 0))
        return;

    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= current_vp->width)
        x2 = current_vp->width-1;

    width = x2 - x1 + 1;

    /* Adjust x1 and y to viewport */
    x1 += current_vp->x;
    y += current_vp->y;

    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        if (current_vp->drawmode & DRMODE_BG)
        {
            if (!lcd_backdrop)
            {
                fillopt = OPT_SET;
                bits = current_vp->bg_pattern;
            }
            else
                fillopt = OPT_COPY;
        }
    }
    else
    {
        if (current_vp->drawmode & DRMODE_FG)
        {
            fillopt = OPT_SET;
            bits = current_vp->fg_pattern;
        }
    }
    dst = LCDADDR(x1, y);

    switch (fillopt)
    {
      case OPT_SET:
        memset16(dst, bits, width);
        break;

      case OPT_COPY:
        memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
               width * sizeof(fb_data));
        break;

      case OPT_NONE:
        dst_end = dst + width;
        do
            pfunc(dst++);
        while (dst < dst_end);
        break;
    }
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    int y;
    fb_data *dst, *dst_end;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[current_vp->drawmode];

    /* direction flip */
    if (y2 < y1)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }

    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)current_vp->width) ||
        (y1 >= current_vp->height) ||
        (y2 < 0))
        return;

    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= current_vp->height)
        y2 = current_vp->height-1;

    dst = LCDADDR(x + current_vp->x, y1 + current_vp->y);
    dst_end = dst + (y2 - y1) * LCD_WIDTH;

    do
    {
        pfunc(dst);
        dst += LCD_WIDTH;
    }
    while (dst <= dst_end);
}

/* Draw a rectangular box */
void lcd_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    lcd_vline(x, y, y2);
    lcd_vline(x2, y, y2);
    lcd_hline(x, x2, y);
    lcd_hline(x, x2, y2);
}

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    unsigned bits = 0;
    enum fill_opt fillopt = OPT_NONE;
    fb_data *dst, *dst_end;
    lcd_fastpixelfunc_type *pfunc = lcd_fastpixelfuncs[current_vp->drawmode];

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
        return;

    /* clipping */
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        if (current_vp->drawmode & DRMODE_BG)
        {
            if (!lcd_backdrop)
            {
                fillopt = OPT_SET;
                bits = current_vp->bg_pattern;
            }
            else
                fillopt = OPT_COPY;
        }
    }
    else
    {
        if (current_vp->drawmode & DRMODE_FG)
        {
            fillopt = OPT_SET;
            bits = current_vp->fg_pattern;
        }
    }
    dst = LCDADDR(current_vp->x + x, current_vp->y + y);
    dst_end = dst + height * LCD_WIDTH;

    do
    {
        fb_data *dst_row, *row_end;

        switch (fillopt)
        {
          case OPT_SET:
            memset16(dst, bits, width);
            break;

          case OPT_COPY:
            memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
                   width * sizeof(fb_data));
            break;

          case OPT_NONE:
            dst_row = dst;
            row_end = dst_row + width;
            do
                pfunc(dst_row++);
            while (dst_row < row_end);
            break;
        }
        dst += LCD_WIDTH;
    }
    while (dst < dst_end);
}

/* Fill a rectangle with a gradient */
static void lcd_gradient_rect(int x1, int x2, int y, int h)
{
    int old_pattern = current_vp->fg_pattern;

    if (h == 0) return;

    int h_r = RGB_UNPACK_RED(current_vp->lss_pattern) << 16;
    int h_b = RGB_UNPACK_BLUE(current_vp->lss_pattern) << 16;
    int h_g = RGB_UNPACK_GREEN(current_vp->lss_pattern) << 16;
    int rstep = (h_r - ((signed)RGB_UNPACK_RED(current_vp->lse_pattern) << 16)) / h;
    int gstep = (h_g - ((signed)RGB_UNPACK_GREEN(current_vp->lse_pattern) << 16)) / h;
    int bstep = (h_b - ((signed)RGB_UNPACK_BLUE(current_vp->lse_pattern) << 16)) / h;
    int count;

    current_vp->fg_pattern = current_vp->lss_pattern;
    for(count = 0; count < h; count++) {
        lcd_hline(x1, x2, y + count);
        h_r -= rstep;
        h_g -= gstep;
        h_b -= bstep;
        current_vp->fg_pattern = LCD_RGBPACK(h_r >> 16, h_g >> 16, h_b >> 16);
    }

    current_vp->fg_pattern = old_pattern;
}

#define H_COLOR(lss, lse, cur_line, max_line) \
        (((lse) - (lss)) * (cur_line) / (max_line) + (lss))

/* Fill a rectangle with a gradient for scrolling line. To draw a gradient that
   covers several lines, we need to know how many lines will be covered
   (the num_lines arg), and which one is the current line within the selection
   (the cur_line arg). */
static void lcd_gradient_rect_scroll(int x1, int x2, int y, int h,
                                     unsigned char num_lines, unsigned char cur_line)
{
    if (h == 0 || num_lines == 0) return;

    unsigned tmp_lss = current_vp->lss_pattern;
    unsigned tmp_lse = current_vp->lse_pattern;
    int lss_r = (signed)RGB_UNPACK_RED(current_vp->lss_pattern);
    int lss_b = (signed)RGB_UNPACK_BLUE(current_vp->lss_pattern);
    int lss_g = (signed)RGB_UNPACK_GREEN(current_vp->lss_pattern);
    int lse_r = (signed)RGB_UNPACK_RED(current_vp->lse_pattern);
    int lse_b = (signed)RGB_UNPACK_BLUE(current_vp->lse_pattern);
    int lse_g = (signed)RGB_UNPACK_GREEN(current_vp->lse_pattern);

    int h_r = H_COLOR(lss_r, lse_r, cur_line, num_lines);
    int h_g = H_COLOR(lss_g, lse_g, cur_line, num_lines);
    int h_b = H_COLOR(lss_b, lse_b, cur_line, num_lines);
    lcd_set_selector_start(LCD_RGBPACK(h_r, h_g, h_b));

    int l_r = H_COLOR(lss_r, lse_r, cur_line+1, num_lines);
    int l_g = H_COLOR(lss_g, lse_g, cur_line+1, num_lines);
    int l_b = H_COLOR(lss_b, lse_b, cur_line+1, num_lines);
    lcd_set_selector_end(LCD_RGBPACK(l_r, l_g, l_b));

    lcd_gradient_rect(x1, x2, y, h);

    current_vp->lss_pattern = tmp_lss;
    current_vp->lse_pattern = tmp_lse;
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
    fb_data *dst, *dst_end;
    lcd_fastpixelfunc_type *fgfunc, *bgfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    dst = LCDADDR(current_vp->x + x, current_vp->y + y);
    fgfunc = lcd_fastpixelfuncs[current_vp->drawmode];
    bgfunc = lcd_fastpixelfuncs[current_vp->drawmode ^ DRMODE_INVERSEVID];
    do
    {
        const unsigned char *src_col = src++;
        unsigned data = *src_col >> src_y;
        fb_data *dst_col = dst++;
        int numbits = 8 - src_y;
        dst_end = dst_col + height * LCD_WIDTH;
        do
        {
            if (data & 0x01)
                fgfunc(dst_col);
            else
                bgfunc(dst_col);

            dst_col += LCD_WIDTH;

            data >>= 1;
            if (--numbits == 0) 
            {
                src_col += stride;
                data = *src_col;
                numbits = 8;
            }
        }
        while (dst_col < dst_end);
    }
    while (src < src_end);
}
/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const fb_data *src, int src_x, int src_y,
                                int stride, int x, int y, int width,
                                int height)
{
    fb_data *dst, *dst_end;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    src += stride * src_y + src_x; /* move starting point */
    dst = LCDADDR(current_vp->x + x, current_vp->y + y);
    dst_end = dst + height * LCD_WIDTH;

    do
    {
        memcpy(dst, src, width * sizeof(fb_data));
        src += stride;
        dst += LCD_WIDTH;
    }
    while (dst < dst_end);
}

/* Draw a full native bitmap */
void lcd_bitmap(const fb_data *src, int x, int y, int width, int height)
{
    lcd_bitmap_part(src, 0, 0, width, x, y, width, height);
}

#if !defined(TOSHIBA_GIGABEAT_F) && !defined(TOSHIBA_GIGABEAT_S) \
    || defined(SIMULATOR)
/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_transparent_part(const fb_data *src, int src_x,
                                            int src_y, int stride, int x,
                                            int y, int width, int height)
{
    fb_data *dst, *dst_end;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;

    src += stride * src_y + src_x; /* move starting point */
    dst = LCDADDR(current_vp->x + x, current_vp->y + y);
    dst_end = dst + height * LCD_WIDTH;

    do
    {
        int i;
        for(i = 0;i < width;i++)
        {
            if (src[i] == REPLACEWITHFG_COLOR)
                dst[i] = current_vp->fg_pattern;
            else if(src[i] != TRANSPARENT_COLOR)
                dst[i] = src[i];
        }
        src += stride;
        dst += LCD_WIDTH;
    }
    while (dst < dst_end);
}
#endif /* !defined(TOSHIBA_GIGABEAT_F) || defined(SIMULATOR) */

/* Draw a full native bitmap with a transparent color */
void lcd_bitmap_transparent(const fb_data *src, int x, int y,
                            int width, int height)
{
    lcd_bitmap_transparent_part(src, 0, 0, width, x, y, width, height);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short ch;
    unsigned short *ucs;
    struct font* pf = font_get(current_vp->font);

    ucs = bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < current_vp->width)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = font_get_width(pf,ch);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = font_get_bits(pf, ch);

        lcd_mono_bitmap_part(bits, ofs, 0, width, x, y, width - ofs, pf->height);

        x += width - ofs;
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_putsxy(int x, int y, const unsigned char *str)
{
    lcd_putsxyofs(x, y, 0, str);
}

/*** line oriented text output ***/

/* put a string at a given char position */
void lcd_puts(int x, int y, const unsigned char *str)
{
    lcd_puts_style_offset(x, y, str, STYLE_DEFAULT, 0);
}

void lcd_puts_style(int x, int y, const unsigned char *str, int style)
{
    lcd_puts_style_offset(x, y, str, style, 0);
}

void lcd_puts_offset(int x, int y, const unsigned char *str, int offset)
{
    lcd_puts_style_offset(x, y, str, STYLE_DEFAULT, offset);
}

/* put a string at a given char position, style, and pixel position,
 * skipping first offset pixel columns */
void lcd_puts_style_offset(int x, int y, const unsigned char *str, int style,
                           int offset)
{
    int xpos,ypos,w,h,xrect;
    int lastmode = current_vp->drawmode;
    int oldfgcolor = current_vp->fg_pattern;
    int oldbgcolor = current_vp->bg_pattern;

    /* make sure scrolling is turned off on the line we are updating */
    lcd_scroll_stop_line(current_vp, y);

    if(!str || !str[0])
        return;

    lcd_getstringsize(str, &w, &h);
    xpos = x*w / utf8length(str);
    ypos = y*h;
    current_vp->drawmode = (style & STYLE_INVERT) ?
               (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    if (style & STYLE_COLORED) {
        if (current_vp->drawmode == DRMODE_SOLID)
            current_vp->fg_pattern = style & STYLE_COLOR_MASK;
        else
            current_vp->bg_pattern = style & STYLE_COLOR_MASK;
    }
    current_vp->drawmode ^= DRMODE_INVERSEVID;
    xrect = xpos + MAX(w - offset, 0);

    if (style & STYLE_GRADIENT) {
        current_vp->drawmode = DRMODE_FG;
        if (CURLN_UNPACK(style) == 0)
            lcd_gradient_rect(xpos, current_vp->width, ypos, h*NUMLN_UNPACK(style));
        current_vp->fg_pattern = current_vp->lst_pattern;
    }
    else if (style & STYLE_COLORBAR) {
        current_vp->drawmode = DRMODE_FG;
        current_vp->fg_pattern = current_vp->lss_pattern;
        lcd_fillrect(xpos, ypos, current_vp->width - xpos, h);
        current_vp->fg_pattern = current_vp->lst_pattern;
    }
    else {
        lcd_fillrect(xrect, ypos, current_vp->width - xrect, h);
        current_vp->drawmode = (style & STYLE_INVERT) ?
        (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    }
    lcd_putsxyofs(xpos, ypos, offset, str);
    current_vp->drawmode = lastmode;
    current_vp->fg_pattern = oldfgcolor;
    current_vp->bg_pattern = oldbgcolor;
}

/*** scrolling ***/
void lcd_puts_scroll(int x, int y, const unsigned char *string)
{
    lcd_puts_scroll_style(x, y, string, STYLE_DEFAULT);
}

void lcd_puts_scroll_style(int x, int y, const unsigned char *string, int style)
{
     lcd_puts_scroll_style_offset(x, y, string, style, 0);
}

void lcd_puts_scroll_offset(int x, int y, const unsigned char *string, int offset)
{
     lcd_puts_scroll_style_offset(x, y, string, STYLE_DEFAULT, offset);
}

/* Initialise a scrolling line at (x,y) in current viewport */

void lcd_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                         int style, int offset)
{
    struct scrollinfo* s;
    int w, h;

    if ((unsigned)y >= (unsigned)current_vp->height)
        return;

    /* remove any previously scrolling line at the same location */
    lcd_scroll_stop_line(current_vp, y);

    if (lcd_scroll_info.lines >= LCD_SCROLLABLE_LINES) return;

    s = &lcd_scroll_info.scroll[lcd_scroll_info.lines];

    s->start_tick = current_tick + lcd_scroll_info.delay;
    s->style = style;
    lcd_puts_style_offset(x,y,string,style,offset);

    lcd_getstringsize(string, &w, &h);

    if (current_vp->width - x * 8 < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, string);

        /* get width */
        s->width = lcd_getstringsize(s->line, &w, &h);

        /* scroll bidirectional or forward only depending on the string
           width */
        if ( lcd_scroll_info.bidir_limit ) {
            s->bidir = s->width < (current_vp->width) *
                (100 + lcd_scroll_info.bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir) { /* add spaces if scrolling in the round */
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->width = lcd_getstringsize(s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, string, current_vp->width/2);

        s->vp = current_vp;
        s->y = y;
        s->len = utf8length(string);
        s->offset = offset;
        s->startx = x * s->width / s->len;
        s->backward = false;
        lcd_scroll_info.lines++;
    }
}

void lcd_scroll_fn(void)
{
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;
    int lastmode;
    unsigned old_fgcolor;
    unsigned old_bgcolor;
    struct viewport* old_vp = current_vp;

    for ( index = 0; index < lcd_scroll_info.lines; index++ ) {
        s = &lcd_scroll_info.scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick))
            continue;

        lcd_set_viewport(s->vp);
        old_fgcolor = current_vp->fg_pattern;
        old_bgcolor = current_vp->bg_pattern;

        if (s->style&STYLE_COLORED) {
            if (s->style&STYLE_MODE_MASK) {
                current_vp->fg_pattern = old_fgcolor;
                current_vp->bg_pattern = s->style&STYLE_COLOR_MASK;
            }
            else {
                current_vp->fg_pattern = s->style&STYLE_COLOR_MASK;
                current_vp->bg_pattern = old_bgcolor;
            }
        }

        if (s->backward)
            s->offset -= lcd_scroll_info.step;
        else
            s->offset += lcd_scroll_info.step;

        pf = font_get(current_vp->font);
        xpos = s->startx;
        ypos = s->y * pf->height;

        if (s->bidir) { /* scroll bidirectional */
            if (s->offset <= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->backward = false;
                s->start_tick = current_tick + lcd_scroll_info.delay * 2;
            }
            if (s->offset >= s->width - (current_vp->width - xpos)) {
                /* at end of line */
                s->offset = s->width - (current_vp->width - xpos);
                s->backward = true;
                s->start_tick = current_tick + lcd_scroll_info.delay * 2;
            }
        }
        else {
            /* scroll forward the whole time */
            if (s->offset >= s->width)
                s->offset %= s->width;
        }

        lastmode = current_vp->drawmode;
        switch (s->style&STYLE_MODE_MASK) {
            case STYLE_INVERT:
                current_vp->drawmode = DRMODE_SOLID|DRMODE_INVERSEVID;
                break;
            case STYLE_COLORBAR:
                /* Solid colour line selector */
                current_vp->drawmode = DRMODE_FG;
                current_vp->fg_pattern = current_vp->lss_pattern;
                lcd_fillrect(xpos, ypos, current_vp->width - xpos, pf->height);
                current_vp->fg_pattern = current_vp->lst_pattern;
                break;
            case STYLE_GRADIENT:
                /* Gradient line selector */
                current_vp->drawmode = DRMODE_FG;
                lcd_gradient_rect_scroll(xpos, current_vp->width, ypos, (signed)pf->height,
                                         NUMLN_UNPACK(s->style),
                                         CURLN_UNPACK(s->style));
                current_vp->fg_pattern = current_vp->lst_pattern;
                break;
            default:
                current_vp->drawmode = DRMODE_SOLID;
                break;
        }
        lcd_putsxyofs(xpos, ypos, s->offset, s->line);
        current_vp->drawmode = lastmode;
        current_vp->fg_pattern = old_fgcolor;
        current_vp->bg_pattern = old_bgcolor;
        lcd_update_viewport_rect(xpos, ypos, current_vp->width - xpos, pf->height);
    }

    lcd_set_viewport(old_vp);
}
