/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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
#include "cpu.h"
#include "kernel.h"
#include "lcd.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "scroll_engine.h"

/*** globals ***/

static fb_data lcd_static_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH] IRAM_LCDFRAMEBUFFER;
static void *lcd_frameaddress_default(int x, int y);

const unsigned char lcd_dibits[16] ICONST_ATTR = {
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
};

static const unsigned char pixmask[4] ICONST_ATTR = {
    0x03, 0x0C, 0x30, 0xC0
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
    return fb->fb_ptr + element; /*(element % fb->elems);*/
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
    unsigned mask = pixmask[y & 3];
    fb_data *address = FBADDR(x,y>>2);
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask);
}

static void clearpixel(int x, int y)
{
    unsigned mask = pixmask[y & 3];
    fb_data *address = FBADDR(x,y>>2);
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask);
}

static void clearimgpixel(int x, int y)
{
    unsigned mask = pixmask[y & 3];
    fb_data *address = FBADDR(x,y>>2);
    unsigned data = *address;

    *address = data ^ ((data ^ *(address + lcd_backdrop_offset)) & mask);
}

static void flippixel(int x, int y)
{
    unsigned mask = pixmask[y & 3];
    fb_data *address = FBADDR(x,y>>2);

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

    lcd_scroll_info.lines = 0;
}

/* Clear the current viewport */
void lcd_clear_viewport(void)
{
    int lastmode;

    if (lcd_current_viewport == &default_vp &&
           default_vp.buffer == &lcd_framebuffer_default)
    {
        lcd_clear_display();
    }
    else
    {
        lastmode = lcd_current_viewport->drawmode;

        /* Invert the INVERSEVID bit and set basic mode to SOLID */
        lcd_current_viewport->drawmode = (~lastmode & DRMODE_INVERSEVID) |
                               DRMODE_SOLID;

        lcd_fillrect(0, 0, lcd_current_viewport->width, lcd_current_viewport->height);

        lcd_current_viewport->drawmode = lastmode;

        lcd_scroll_stop_viewport(lcd_current_viewport);
    }
    lcd_current_viewport->flags &= ~(VP_FLAG_VP_SET_CLEAN);
}

/* Draw a horizontal line (optimised) */
void lcd_hline(int x1, int x2, int y)
{
    struct viewport *vp = lcd_current_viewport;
    int width;
    fb_data *dst, *dst_end;
    unsigned mask;
    lcd_blockfunc_type *bfunc;

    if (!clip_viewport_hline(vp, &x1, &x2, &y))
        return;

    width = x2 - x1 + 1;

    bfunc = lcd_blockfuncs[vp->drawmode];
    dst   = FBADDR(x1,y>>2);
    mask  = pixmask[y & 3];

    dst_end = dst + width;
    do
        bfunc(dst++, mask, 0xFFu);
    while (dst < dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_vline(int x, int y1, int y2)
{
    struct viewport *vp = lcd_current_viewport;
    int ny;
    fb_data *dst;
    int stride_dst;
    unsigned mask, mask_bottom;
    lcd_blockfunc_type *bfunc;

    if (!clip_viewport_vline(vp, &x, &y1, &y2))
        return;

    bfunc = lcd_blockfuncs[vp->drawmode];
    dst   = FBADDR(x,y1>>2);
    stride_dst = vp->buffer->stride;
    ny    = y2 - (y1 & ~3);
    mask  = 0xFFu << (2 * (y1 & 3));
    mask_bottom = 0xFFu >> (2 * (~ny & 3));

    for (; ny >= 4; ny -= 4)
    {
        bfunc(dst, mask, 0xFFu);
        dst += stride_dst;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFu);
}

/* Fill a rectangular area */
void lcd_fillrect(int x, int y, int width, int height)
{
    struct viewport *vp = lcd_current_viewport;
    int ny;
    fb_data *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    lcd_blockfunc_type *bfunc;
    bool fillopt = false;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, NULL, NULL))
        return;

    if (vp->drawmode & DRMODE_INVERSEVID)
    {
        if ((vp->drawmode & DRMODE_BG) && !lcd_backdrop)
        {
            fillopt = true;
            bits = bg_pattern;
        }
    }
    else
    {
        if (vp->drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = fg_pattern;
        }
    }
    bfunc = lcd_blockfuncs[vp->drawmode];
    dst   = FBADDR(x,y>>2);
    stride_dst = vp->buffer->stride;
    ny    = height - 1 + (y & 3);
    mask  = 0xFFu << (2 * (y & 3));
    mask_bottom = 0xFFu >> (2 * (~ny & 3));

    for (; ny >= 4; ny -= 4)
    {
        if (fillopt && (mask == 0xFFu))
            memset(dst, bits, width);
        else
        {
            fb_data *dst_row = dst;

            dst_end = dst_row + width;
            do
                bfunc(dst_row++, mask, 0xFFu);
            while (dst_row < dst_end);
        }

        dst += stride_dst;
        mask = 0xFFu;
    }
    mask &= mask_bottom;

    if (fillopt && (mask == 0xFFu))
        memset(dst, bits, width);
    else
    {
        dst_end = dst + width;
        do
            bfunc(dst++, mask, 0xFFu);
        while (dst < dst_end);
    }
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
 * This is similar to the internal lcd hw format. */

/* Draw a partial monochrome bitmap */
void ICODE_ATTR lcd_mono_bitmap_part(const unsigned char *src, int src_x,
                                     int src_y, int stride, int x, int y,
                                     int width, int height)
{
    struct viewport *vp = lcd_current_viewport;
    int shift, ny;
    fb_data *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;
    lcd_blockfunc_type *bfunc;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, &src_x, &src_y))
        return;

    src    += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    y      -= src_y;
    dst    = FBADDR(x,y>>2);
    stride_dst = vp->buffer->stride;
    shift  = y & 3;
    ny     = height - 1 + shift + src_y;
    mask   = 0xFFFFu << (2 * (shift + src_y));
             /* Overflowing bits aren't important. */
    mask_bottom  = 0xFFFFu >> (2 * (~ny & 7));

    bfunc  = lcd_blockfuncs[vp->drawmode];

    if (shift == 0)
    {
        unsigned dmask1, dmask2, data;

        dmask1 = mask & 0xFFu;
        dmask2 = mask >> 8;

        for (; ny >= 8; ny -= 8)
        {
            const unsigned char *src_row = src;
            fb_data *dst_row = dst + stride_dst;

            dst_end = dst_row + width;

            if (dmask1 != 0)
            {
                do
                {
                    data = *src_row++;
                    bfunc(dst_row - stride_dst, dmask1, lcd_dibits[data&0x0F]);
                    bfunc(dst_row++, dmask2, lcd_dibits[(data>>4)&0x0F]);
                }
                while (dst_row < dst_end);
            }
            else
            {
                do
                    bfunc(dst_row++, dmask2, lcd_dibits[((*src_row++)>>4)&0x0F]);
                while (dst_row < dst_end);
            }
            src += stride;
            dst += 2*stride_dst;
            dmask1 = dmask2 = 0xFFu;
        }
        dmask1 &= mask_bottom;
                  /* & 0xFFu is unnecessary here - dmask1 can't exceed that*/
        dmask2 &= (mask_bottom >> 8);
        dst_end = dst + width;

        if (dmask1 != 0)
        {
            if (dmask2 != 0)
            {
                do
                {
                    data = *src++;
                    bfunc(dst, dmask1, lcd_dibits[data&0x0F]);
                    bfunc((dst++) + stride_dst, dmask2, lcd_dibits[(data>>4)&0x0F]);
                }
                while (dst < dst_end);
            }
            else
            {
                do
                    bfunc(dst++, dmask1, lcd_dibits[(*src++)&0x0F]);
                while (dst < dst_end);
            }
        }
        else
        {
            do
                bfunc((dst++) + stride_dst, dmask2, lcd_dibits[((*src++)>>4)&0x0F]);
            while (dst < dst_end);
        }
    }
    else
    {
        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            fb_data *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;

            for (y = ny; y >= 8; y -= 8)
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFFFu)
                {
                    if (mask_col & 0xFFu)
                        bfunc(dst_col, mask_col, lcd_dibits[data&0x0F]);
                    bfunc(dst_col + stride_dst, mask_col >> 8,
                          lcd_dibits[(data>>4)&0x0F]);
                    mask_col = 0xFFFFu;
                }
                else
                    mask_col >>= 16;

                src_col += stride;
                dst_col += 2*stride_dst;
                data >>= 8;
            }
            data |= *src_col << shift;
            mask_col &= mask_bottom ;
            if (mask_col & 0xFFu)
                bfunc(dst_col, mask_col, lcd_dibits[data&0x0F]);
            if (mask_col & 0xFF00u)
                bfunc(dst_col + stride_dst, mask_col >> 8,
                      lcd_dibits[(data>>4)&0x0F]);
        }
        while (dst < dst_end);
    }
}

/* Draw a full monochrome bitmap */
void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width, int height)
{
    lcd_mono_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* About Rockbox' internal native bitmap format:
 *
 * A bitmap contains two bits for every pixel. 00 = white, 01 = light grey,
 * 10 = dark grey, 11 = black. Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..3, the second row defines pixel row 4..7 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void ICODE_ATTR lcd_bitmap_part(const fb_data *src, int src_x, int src_y,
                                int stride, int x, int y, int width,
                                int height)
{
    struct viewport *vp = lcd_current_viewport;
    int shift, ny;
    fb_data *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, &src_x, &src_y))
        return;

    src   += stride * (src_y >> 2) + src_x; /* move starting point */
    src_y &= 3;
    y     -= src_y;
    dst    = FBADDR(x,y>>2);
    stride_dst = vp->buffer->stride;
    shift  = y & 3;
    ny     = height - 1 + shift + src_y;

    mask   = 0xFFu << (2 * (shift + src_y));
    mask_bottom = 0xFFu >> (2 * (~ny & 3));

    if (shift == 0)
    {
        for (; ny >= 4; ny -= 4)
        {
            if (mask == 0xFFu)
                memcpy(dst, src, width);
            else
            {
                const fb_data *src_row = src;
                fb_data *dst_row = dst;

                dst_end = dst_row + width;
                do
                    setblock(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }
            src += stride;
            dst += stride_dst;
            mask = 0xFFu;
        }
        mask &= mask_bottom;

        if (mask == 0xFFu)
            memcpy(dst, src, width);
        else
        {
            dst_end = dst + width;
            do
                setblock(dst++, mask, *src++);
            while (dst < dst_end);
        }
    }
    else
    {
        shift *= 2;
        dst_end = dst + width;
        do
        {
            const fb_data *src_col = src++;
            fb_data *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;

            for (y = ny; y >= 4; y -= 4)
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFu)
                {
                    setblock(dst_col, mask_col, data);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += stride_dst;
                data >>= 8;
            }
            data |= *src_col << shift;
            setblock(dst_col, mask_col & mask_bottom, data);
        }
        while (dst < dst_end);
    }
}

/* Draw a full native bitmap */
void lcd_bitmap(const fb_data *src, int x, int y, int width, int height)
{
    lcd_bitmap_part(src, 0, 0, width, x, y, width, height);
}
