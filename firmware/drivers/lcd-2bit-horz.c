/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * LCD driver for horizontally-packed 2bpp greyscale display
 *
 * Based on code from the rockbox lcd's driver
 *
 * Copyright (c) 2006 Seven Le Mesle (sevlm@free.fr)
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
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "scroll_engine.h"

/*** globals ***/

static unsigned char lcd_static_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH] IRAM_LCDFRAMEBUFFER;
static void *lcd_frameaddress_default(int x, int y);

static const unsigned char pixmask[4] ICONST_ATTR = {
    0xC0, 0x30, 0x0C, 0x03
};

static fb_data* lcd_backdrop = NULL;
static long lcd_backdrop_offset IDATA_ATTR = 0;

/* shouldn't be changed unless you want system-wide framebuffer changes! */
struct frame_buffer_t lcd_framebuffer_default =
{
    .fb_ptr         = &lcd_static_framebuffer[0][0],
    .get_address_fn = &lcd_frameaddress_default,
    .stride         = STRIDE_MAIN(LCD_WIDTH, LCD_HEIGHT),
    .elems          = (LCD_FBWIDTH*LCD_FBHEIGHT),
};

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCD_WIDTH,
    .height   = LCD_HEIGHT,
    .flags    = 0,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .buffer   = NULL,
    .fg_pattern = LCD_DEFAULT_FG,
    .bg_pattern = LCD_DEFAULT_BG
};

struct viewport* lcd_current_viewport IBSS_ATTR;
static unsigned fg_pattern IBSS_ATTR;
static unsigned bg_pattern IBSS_ATTR;

static void *lcd_frameaddress_default(int x, int y)
{
    /* the default expects a buffer the same size as the screen */
    struct frame_buffer_t *fb = lcd_current_viewport->buffer;

#if LCD_STRIDEFORMAT == VERTICAL_STRIDE
    size_t element = (x * LCD_NATIVE_STRIDE(fb->stride)) + y;
#else
    size_t element = (y * LCD_NATIVE_STRIDE(fb->stride)) + x;
#endif
    return fb->fb_ptr + element;/*(element % fb->elems);*/
}

#include "lcd-bitmap-common.c"

/* LCD init */
void lcd_init(void)
{
    /* Initialize the viewport */
    lcd_set_viewport(NULL);

    lcd_clear_display();
    /* Call device specific init */
    lcd_init_device();
    scroll_init();
}

/*** parameter handling ***/

void lcd_set_foreground(unsigned brightness)
{
    lcd_current_viewport->fg_pattern = brightness;
    fg_pattern = 0x55 * (~brightness & 3);
}

unsigned lcd_get_foreground(void)
{
    return lcd_current_viewport->fg_pattern;
}

void lcd_set_background(unsigned brightness)
{
    lcd_current_viewport->bg_pattern = brightness;
    bg_pattern = 0x55 * (~brightness & 3);
}

unsigned lcd_get_background(void)
{
    return lcd_current_viewport->bg_pattern;
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    unsigned mask = pixmask[x & 3];
    fb_data *address = FBADDR(x>>2,y);
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask);
}

static void clearpixel(int x, int y)
{
    unsigned mask = pixmask[x & 3];
    fb_data *address = FBADDR(x>>2,y);
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask);
}

static void clearimgpixel(int x, int y)
{
    unsigned mask = pixmask[x & 3];
    fb_data *address = FBADDR(x>>2,y);
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask);
}

static void flippixel(int x, int y)
{
    unsigned mask =  pixmask[x & 3];
    fb_data *address = FBADDR(x>>2,y);

    *address ^= mask;
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_pixelfunc_type* const lcd_pixelfuncs_bgcolor[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

lcd_pixelfunc_type* const lcd_pixelfuncs_backdrop[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearimgpixel, nopixel, clearimgpixel
};

lcd_pixelfunc_type* const * lcd_pixelfuncs = lcd_pixelfuncs_bgcolor;


/* 'mask' and 'bits' contain 2 bits per pixel */
static void ICODE_ATTR flipblock(fb_data *address, unsigned mask,
                                 unsigned bits)
{
    *address ^= bits & mask;
}

static void ICODE_ATTR bgblock(fb_data *address, unsigned mask,
                               unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & ~bits);
}

static void ICODE_ATTR bgimgblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask & ~bits);
}

static void ICODE_ATTR fgblock(fb_data *address, unsigned mask,
                               unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & bits);
}

static void ICODE_ATTR solidblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = bg_pattern;

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR solidimgblock(fb_data *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = *(address + lcd_backdrop_offset);

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR flipinvblock(fb_data *address, unsigned mask,
                                    unsigned bits)
{
    *address ^= ~bits & mask;
}

static void ICODE_ATTR bginvblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & bits);
}

static void ICODE_ATTR bgimginvblock(fb_data *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask & bits);
}

static void ICODE_ATTR fginvblock(fb_data *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & ~bits);
}

static void ICODE_ATTR solidinvblock(fb_data *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ bg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR solidimginvblock(fb_data *address, unsigned mask,
                                        unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ *(address + lcd_backdrop_offset)) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

lcd_blockfunc_type* const lcd_blockfuncs_bgcolor[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

lcd_blockfunc_type* const lcd_blockfuncs_backdrop[8] = {
    flipblock, bgimgblock, fgblock, solidimgblock,
    flipinvblock, bgimginvblock, fginvblock, solidimginvblock
};

lcd_blockfunc_type* const * lcd_blockfuncs = lcd_blockfuncs_bgcolor;


void lcd_set_backdrop(fb_data* backdrop)
{
    lcd_backdrop = backdrop;
    if (backdrop)
    {
        lcd_backdrop_offset = (long)backdrop - (long)FBADDR(0,0);
        lcd_pixelfuncs = lcd_pixelfuncs_backdrop;
        lcd_blockfuncs = lcd_blockfuncs_backdrop;
    }
    else
    {
        lcd_backdrop_offset = 0;
        lcd_pixelfuncs = lcd_pixelfuncs_bgcolor;
        lcd_blockfuncs = lcd_blockfuncs_bgcolor;
    }
}

fb_data* lcd_get_backdrop(void)
{
    return lcd_backdrop;
}


static inline void setblock(fb_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    bits    ^= data;
    *address = data ^ (bits & mask);
}

/*** drawing functions ***/

/* Clear the whole display */
void lcd_clear_display(void)
{
    if (lcd_current_viewport->drawmode & DRMODE_INVERSEVID)
    {
        memset(FBADDR(0,0), fg_pattern, FRAMEBUFFER_SIZE);
    }
    else
    {
        if (lcd_backdrop)
            memcpy(FBADDR(0,0), lcd_backdrop, FRAMEBUFFER_SIZE);
        else
            memset(FBADDR(0,0), bg_pattern, FRAMEBUFFER_SIZE);
    }

    lcd_scroll_stop();
}

/* Draw a horizontal line (optimised) */
void lcd_hline(int x1, int x2, int y)
{
    struct viewport *vp = lcd_current_viewport;
    int nx;
    unsigned char *dst;
    unsigned mask, mask_right;
    lcd_blockfunc_type *bfunc;

    if (!clip_viewport_hline(vp, &x1, &x2, &y))
        return;

    bfunc = lcd_blockfuncs[vp->drawmode];
    dst   = FBADDR(x1>>2,y);
    nx    = x2 - (x1 & ~3);
    mask  = 0xFFu >> (2 * (x1 & 3));
    mask_right = 0xFFu << (2 * (~nx & 3));

    for (; nx >= 4; nx -= 4)
    {
        bfunc(dst++, mask, 0xFFu);
        mask = 0xFFu;
    }
    mask &= mask_right;
    bfunc(dst, mask, 0xFFu);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    struct viewport *vp = lcd_current_viewport;
    unsigned char *dst, *dst_end;
    int stride_dst;
    unsigned mask;
    lcd_blockfunc_type *bfunc;

    if (!clip_viewport_vline(vp, &x, &y1, &y2))
        return;

    bfunc = lcd_blockfuncs[vp->drawmode];
    dst   = FBADDR(x>>2,y1);
    stride_dst = LCD_FBSTRIDE(vp->buffer->stride, 0);
    mask  = pixmask[x & 3];

    dst_end = dst + (y2 - y1) * stride_dst;
    do
    {
        bfunc(dst, mask, 0xFFu);
        dst += stride_dst;
    }
    while (dst <= dst_end);
}

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    struct viewport *vp = lcd_current_viewport;
    int nx;
    unsigned char *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_right;
    lcd_blockfunc_type *bfunc;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, NULL, NULL))
        return;

    bfunc      = lcd_blockfuncs[vp->drawmode];
    dst        = FBADDR(x>>2,y);
    stride_dst = LCD_FBSTRIDE(vp->buffer->stride, 0);
    nx         = width - 1 + (x & 3);
    mask       = 0xFFu >> (2 * (x & 3));
    mask_right = 0xFFu << (2 * (~nx & 3));

    for (; nx >= 4; nx -= 4)
    {
        unsigned char *dst_col = dst;

        dst_end = dst_col + height * stride_dst;
        do
        {
            bfunc(dst_col, mask, 0xFFu);
            dst_col += stride_dst;
        }
        while (dst_col < dst_end);

        dst++;
        mask = 0xFFu;
    }
    mask &= mask_right;

    dst_end = dst + height * stride_dst;
    do
    {
        bfunc(dst, mask, 0xFFu);
        dst += stride_dst;
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
 * 0..7, the second row defines pixel row 8..15 etc. */

/* Draw a partial monochrome bitmap */
void ICODE_ATTR lcd_mono_bitmap_part(const unsigned char *src, int src_x,
                                     int src_y, int stride, int x, int y,
                                     int width, int height)
{
    struct viewport *vp = lcd_current_viewport;
    const unsigned char *src_end;
    fb_data *dst, *dst_end;
    int stride_dst;
    unsigned dmask = 0x100; /* bit 8 == sentinel */
    unsigned dst_mask;
    int drmode = vp->drawmode;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, &src_x, &src_y))
        return;

    src += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    src_end = src + width;

    dst = FBADDR(x >> 2,y);
    stride_dst = LCD_FBSTRIDE(vp->buffer->stride, 0);
    dst_end = dst + height * stride_dst;
    dst_mask = pixmask[x & 3];

    if (drmode & DRMODE_INVERSEVID)
    {
        dmask = 0x1ff;          /* bit 8 == sentinel */
        drmode &= DRMODE_SOLID; /* mask out inversevid */
    }

    do
    {
        const unsigned char *src_col = src++;
        unsigned data = (*src_col ^ dmask) >> src_y;
        fb_data *dst_col = dst;
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
                    *dst_col ^= dst_mask;

                dst_col += stride_dst;
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
                    {
                        unsigned block = *dst_col;
                        *dst_col = block
                                 ^ ((block ^ *(dst_col + bo)) & dst_mask);
                    }
                    dst_col += stride_dst;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            else
            {
                bg = bg_pattern;
                do
                {
                    if (!(data & 0x01))
                    {
                        unsigned block = *dst_col;
                        *dst_col = block ^ ((block ^ bg) & dst_mask);
                    }
                    dst_col += stride_dst;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            break;

          case DRMODE_FG:
            fg = fg_pattern;
            do
            {
                if (data & 0x01)
                {
                    unsigned block = *dst_col;
                    *dst_col = block ^ ((block ^ fg) & dst_mask);
                }
                dst_col += stride_dst;
                UPDATE_SRC;
            }
            while (dst_col < dst_end);
            break;

          case DRMODE_SOLID:
            fg = fg_pattern;
            if (lcd_backdrop)
            {
                bo = lcd_backdrop_offset;
                do
                {
                    unsigned block = *dst_col;
                    *dst_col = block ^ ((block ^ ((data & 0x01) ?
                               fg : *(dst_col + bo))) & dst_mask);

                    dst_col += stride_dst;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            else
            {
                bg = bg_pattern;
                do
                {
                    unsigned block = *dst_col;
                    *dst_col = block ^ ((block ^ ((data & 0x01) ?
                               fg : bg)) & dst_mask);

                    dst_col += stride_dst;
                    UPDATE_SRC;
                }
                while (dst_col < dst_end);
            }
            break;
        }
        dst_mask >>= 2;
        if (dst_mask == 0)
        {
            dst++;
            dst_mask = 0xC0;
        }
    }
    while (src < src_end);
}

/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* About Rockbox' internal native bitmap format:
 *
 * A bitmap contains two bits for every pixel. 00 = white, 01 = light grey,
 * 10 = dark grey, 11 = black. Bits within a byte are arranged horizontally,
 * MSB at the left.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. Each row of bytes defines one pixel row.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const unsigned char *src, int src_x,
                                int src_y, int stride, int x, int y,
                                int width, int height)
{
    struct viewport *vp = lcd_current_viewport;
    int  shift, nx;
    unsigned char *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_right;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, &src_x, &src_y))
        return;

    stride = LCD_FBSTRIDE(stride, 0); /* convert to no. of bytes */

    src   += stride * src_y + (src_x >> 2); /* move starting point */
    src_x &= 3;
    x     -= src_x;
    dst   = FBADDR(x>>2,y);
    stride_dst = LCD_FBSTRIDE(vp->buffer->stride, 0);
    shift = x & 3;
    nx    = width - 1 + shift + src_x;

    mask   = 0xFF00u >> (2 * (shift + src_x));
    mask_right = 0xFFu << (2 * (~nx & 3));

    shift *= 2;
    dst_end = dst + height * stride_dst;
    do
    {
        const unsigned char *src_row = src;
        unsigned char *dst_row = dst;
        unsigned mask_row = mask >> 8;
        unsigned data = 0;

        for (x = nx; x >= 4; x -= 4)
        {
            data = (data << 8) | *src_row++;

            if (mask_row & 0xFF)
            {
                setblock(dst_row, mask_row, data >> shift);
                mask_row = 0xFF;
            }
            else
                mask_row = mask;

            dst_row++;
        }
        data = (data << 8) | *src_row;
        setblock(dst_row, mask_row & mask_right, data >> shift);

        src += stride;
        dst += stride_dst;
    }
    while (dst < dst_end);
}

/* Draw a full native bitmap */
void lcd_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_bitmap_part(src, 0, 0, width, x, y, width, height);
}
