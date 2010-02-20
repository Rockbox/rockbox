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

static struct viewport* current_vp IDATA_ATTR = &default_vp;

/* LCD init */
void lcd_init(void)
{
    lcd_clear_display();

    /* Call device specific init */
    lcd_init_device();
    scroll_init();
}
/*** Viewports ***/

void lcd_set_viewport(struct viewport* vp)
{
    if (vp == NULL)
        current_vp = &default_vp;
    else
        current_vp = vp;
        
#if defined(SIMULATOR)
    /* Force the viewport to be within bounds.  If this happens it should
     *  be considered an error - the viewport will not draw as it might be
     *  expected.
     */
    if((unsigned) current_vp->x > (unsigned) LCD_WIDTH 
        || (unsigned) current_vp->y > (unsigned) LCD_HEIGHT 
        || current_vp->x + current_vp->width > LCD_WIDTH
        || current_vp->y + current_vp->height > LCD_HEIGHT)
    {
#if !defined(HAVE_VIEWPORT_CLIP)
        DEBUGF("ERROR: "
#else
        DEBUGF("NOTE: "
#endif
            "set_viewport out of bounds: x: %d y: %d width: %d height:%d\n", 
            current_vp->x, current_vp->y, 
            current_vp->width, current_vp->height);
    }
    
#endif
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
    if (   ((unsigned)x < (unsigned)current_vp->width) 
        && ((unsigned)y < (unsigned)current_vp->height)
#if defined(HAVE_VIEWPORT_CLIP)
        && ((unsigned)x < (unsigned)LCD_WIDTH)
        && ((unsigned)y < (unsigned)LCD_HEIGHT)
#endif
        )
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
        /* DEBUGF("lcd_drawline() called for horizontal line - optimisation.\n"); */
        lcd_hline(x1, x2, y1);
        return;
    }
    deltax = abs(x2 - x1);
    if (deltax == 0)
    {
        /* DEBUGF("lcd_drawline() called for vertical line - optimisation.\n"); */
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
        if (   ((unsigned)x < (unsigned)current_vp->width) 
            && ((unsigned)y < (unsigned)current_vp->height)
#if defined(HAVE_VIEWPORT_CLIP)
            && ((unsigned)x < (unsigned)LCD_WIDTH)
            && ((unsigned)y < (unsigned)LCD_HEIGHT)
#endif
            )
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

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)current_vp->height) || 
        (x1 >= current_vp->width) ||
        (x2 < 0))
        return;
        
    if (x1 < 0)
        x1 = 0;
    if (x2 >= current_vp->width)
        x2 = current_vp->width-1;

    /* Adjust x1 and y to viewport */
    x1 += current_vp->x;
    x2 += current_vp->x;
    y += current_vp->y;
    
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

    /* drawmode and optimisation */
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
    if (fillopt == OPT_NONE && current_vp->drawmode != DRMODE_COMPLEMENT)
        return;

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

      case OPT_NONE:  /* DRMODE_COMPLEMENT */
        dst_end = dst + width;
        do
            *dst = ~(*dst);
        while (++dst < dst_end);
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

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)current_vp->width) ||
        (y1 >= current_vp->height) ||
        (y2 < 0))
        return;

    if (y1 < 0)
        y1 = 0;
    if (y2 >= current_vp->height)
        y2 = current_vp->height-1;
        
    /* adjust for viewport */
    x += current_vp->x;
    y1 += current_vp->y;
    y2 += current_vp->y;
    
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

    dst = LCDADDR(x , y1);
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

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;
        
    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;

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
    if (fillopt == OPT_NONE && current_vp->drawmode != DRMODE_COMPLEMENT)
        return;

    dst = LCDADDR(x, y);
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

          case OPT_NONE:  /* DRMODE_COMPLEMENT */
            dst_row = dst;
            row_end = dst_row + width;
            do
                *dst_row = ~(*dst_row);
            while (++dst_row < row_end);
            break;
        }
        dst += LCD_WIDTH;
    }
    while (dst < dst_end);
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
    unsigned dmask = 0x100; /* bit 8 == sentinel */
    int drmode = current_vp->drawmode;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) || 
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;
        
    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;
        
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
    dst = LCDADDR(x, y);
    dst_end = dst + height * LCD_WIDTH;

    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0x1ff;          /* bit 8 == sentinel */
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }

    do
    {
        const unsigned char *src_col = src++;
        unsigned data = (*src_col ^ dmask) >> src_y;
        fb_data *dst_col = dst++;
        int fg, bg;
        long bo;

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
                    *dst_col = ~(*dst_col);

                dst_col += LCD_WIDTH;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;

          case DRMODE_BG:
            if (lcd_backdrop)
            {
                bo = lcd_backdrop_offset;
                do
                {
                    if (!(data & 0x01))
                        *dst_col = *(fb_data *)((long)dst_col + bo);

                    dst_col += LCD_WIDTH;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            else
            {
                bg = current_vp->bg_pattern;
                do
                {
                    if (!(data & 0x01))
                        *dst_col = bg;

                    dst_col += LCD_WIDTH;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            break;

          case DRMODE_FG:
            fg = current_vp->fg_pattern;
            do
            {
                if (data & 0x01)
                    *dst_col = fg;

                dst_col += LCD_WIDTH;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;

          case DRMODE_SOLID:
            fg = current_vp->fg_pattern;
            if (lcd_backdrop)
            {
                bo = lcd_backdrop_offset;
                do
                {
                    *dst_col = (data & 0x01) ? fg 
                               : *(fb_data *)((long)dst_col + bo);
                    dst_col += LCD_WIDTH;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            else
            {
                bg = current_vp->bg_pattern;
                do
                {
                    *dst_col = (data & 0x01) ? fg : bg;
                    dst_col += LCD_WIDTH;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
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

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const fb_data *src, int src_x, int src_y,
                                int stride, int x, int y, int width,
                                int height)
{
    fb_data *dst;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) ||
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;    
    
    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;
    
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
    dst = LCDADDR(x, y);

    do
    {
        memcpy(dst, src, width * sizeof(fb_data));
        src += stride;
        dst += LCD_WIDTH;
    }
    while (--height > 0);
}

/* Draw a full native bitmap */
void lcd_bitmap(const fb_data *src, int x, int y, int width, int height)
{
    lcd_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* Draw a partial native bitmap with transparency and foreground colors */
void ICODE_ATTR lcd_bitmap_transparent_part(const fb_data *src, int src_x,
                                            int src_y, int stride, int x,
                                            int y, int width, int height)
{
    fb_data *dst;
    unsigned fg = current_vp->fg_pattern;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width) ||
        (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    
    if (x + width > current_vp->width)
        width = current_vp->width - x;
    if (y + height > current_vp->height)
        height = current_vp->height - y;    
    
    /* adjust for viewport */
    x += current_vp->x;
    y += current_vp->y;
    
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
    dst = LCDADDR(x, y);

#ifdef CPU_ARM
    {
        int w, px;
        asm volatile (
        ".rowstart:                              \n"
            "mov     %[w], %[width]              \n" /* Load width for inner loop */
        ".nextpixel:                             \n"
            "ldrh    %[px], [%[s]], #2           \n" /* Load src pixel */
            "add     %[d], %[d], #2              \n" /* Uncoditionally increment dst */
                                                 /* done here for better pipelining */
            "cmp     %[px], %[fgcolor]           \n" /* Compare to foreground color */
            "streqh  %[fgpat], [%[d], #-2]       \n" /* Store foregroud if match */
            "cmpne   %[px], %[transcolor]        \n" /* Compare to transparent color */
            "strneh  %[px], [%[d], #-2]          \n" /* Store dst if not transparent */
            "subs    %[w], %[w], #1              \n" /* Width counter has run down? */
            "bgt     .nextpixel                  \n" /* More in this row? */
            "add     %[s], %[s], %[sstp], lsl #1 \n" /* Skip over to start of next line */
            "add     %[d], %[d], %[dstp], lsl #1 \n"
            "subs    %[h], %[h], #1              \n" /* Height counter has run down? */
            "bgt     .rowstart                   \n" /* More rows? */
            : [w]"=&r"(w), [h]"+&r"(height), [px]"=&r"(px),
              [s]"+&r"(src), [d]"+&r"(dst)
            : [width]"r"(width),
              [sstp]"r"(stride - width),
              [dstp]"r"(LCD_WIDTH - width),
              [transcolor]"r"(TRANSPARENT_COLOR),
              [fgcolor]"r"(REPLACEWITHFG_COLOR),
              [fgpat]"r"(fg)
        );
    }
#else  /* optimized C version */
    do
    {
        const fb_data *src_row = src;
        fb_data *dst_row = dst;
        fb_data *row_end = dst_row + width;
        do
        {
            unsigned data = *src_row++;
            if (data != TRANSPARENT_COLOR)
            {
                if (data == REPLACEWITHFG_COLOR)
                    data = fg;
                *dst_row = data;
            }
        }
        while (++dst_row < row_end);
        src += stride;
        dst += LCD_WIDTH;
    }
    while (--height > 0);
#endif
}

/* Draw a full native bitmap with transparent and foreground colors */
void lcd_bitmap_transparent(const fb_data *src, int x, int y,
                            int width, int height)
{
    lcd_bitmap_transparent_part(src, 0, 0, width, x, y, width, height);
}

#include "lcd-bitmap-common.c"
