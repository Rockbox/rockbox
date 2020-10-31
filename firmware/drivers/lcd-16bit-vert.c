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
 * Rockbox driver for 16-bit colour LCDs with vertical strides
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

#define ROW_INC 1
#define COL_INC lcd_current_viewport->buffer->stride

extern lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_backdrop[];
extern lcd_fastpixelfunc_type* const lcd_fastpixelfuncs_bgcolor[];

static void ICODE_ATTR lcd_alpha_bitmap_part_mix(const fb_data* image,
                                      const unsigned char *src, int src_x,
                                      int src_y, int x, int y,
                                      int width, int height,
                                      int stride_image, int stride_src);

#include "lcd-color-common.c"
#include "lcd-16bit-common.c"
#include "lcd-bitmap-common.c"

/*** drawing functions ***/

/* Draw a horizontal line (optimised) */
void lcd_hline(int x1, int x2, int y)
{
    int x;
    fb_data *dst, *dst_end;
    int stride_dst;

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

    dst = FBADDR(x1 , y );
    stride_dst = lcd_current_viewport->buffer->stride;
    dst_end = dst + (x2 - x1) * stride_dst;

    do
    {
        pfunc(dst);
        dst += stride_dst;
    }
    while (dst <= dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    int y, height;
    unsigned bits = 0;
    enum fill_opt fillopt = OPT_NONE;
    fb_data *dst, *dst_end;

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

    height = y2 - y1 + 1;

    /* drawmode and optimisation */
    if (lcd_current_viewport->drawmode & DRMODE_INVERSEVID)
    {
        if (lcd_current_viewport->drawmode & DRMODE_BG)
        {
            if (!lcd_backdrop)
            {
                fillopt = OPT_SET;
                bits = lcd_current_viewport->bg_pattern;
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
            bits = lcd_current_viewport->fg_pattern;
        }
    }
    if (fillopt == OPT_NONE && lcd_current_viewport->drawmode != DRMODE_COMPLEMENT)
        return;

    dst = FBADDR(x, y1);

    switch (fillopt)
    {
      case OPT_SET:
        memset16(dst, bits, height);
        break;

      case OPT_COPY:
        memcpy(dst, (void *)((long)dst + lcd_backdrop_offset),
               height * sizeof(fb_data));
        break;

      case OPT_NONE:  /* DRMODE_COMPLEMENT */
        dst_end = dst + height;
        do
            *dst = ~(*dst);
        while (++dst < dst_end);
        break;
    }
}

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const fb_data *src, int src_x, int src_y,
                                int stride, int x, int y, int width,
                                int height)
{
    fb_data *dst;
    int stride_dst;
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

    src += stride * src_x + src_y; /* move starting point */
    dst = FBADDR(x, y);
    stride_dst = lcd_current_viewport->buffer->stride;
    fb_data *dst_end = dst + width * stride_dst;

    do
    {
        memcpy(dst, src, height * sizeof(fb_data));
        src += stride;
        dst += stride_dst;
    }
    while (dst < dst_end);
}

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_transparent_part(const fb_data *src, int src_x,
                                            int src_y, int stride, int x,
                                            int y, int width, int height)
{
    fb_data *dst, *dst_end;
    int stride_dst;

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

    src += stride * src_x + src_y; /* move starting point */
    dst = FBADDR(x, y);
    stride_dst = lcd_current_viewport->buffer->stride;
    dst_end = dst + width * stride_dst;

    do
    {
        int i;
        for(i = 0;i < height;i++)
        {
            if (src[i] == REPLACEWITHFG_COLOR)
                dst[i] = lcd_current_viewport->fg_pattern;
            else if(src[i] != TRANSPARENT_COLOR)
                dst[i] = src[i];
        }
        src += stride;
        dst += stride_dst;
    }
    while (dst < dst_end);
}
