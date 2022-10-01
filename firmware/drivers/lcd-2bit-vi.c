/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
 *
 * Rockbox driver for 2bit vertically interleaved LCDs
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

#ifndef LCDFN /* Not compiling for remote - define macros for main LCD. */
#define LCDFN(fn) lcd_ ## fn
#define FBFN(fn)  fb_ ## fn
#define LCDM(ma)  LCD_ ## ma
#define FBSIZE FRAMEBUFFER_SIZE
#define LCDNAME "lcd_"
#define LCDFB(x,y) FBADDR(x, y)
#define MAIN_LCD
#endif

#ifdef MAIN_LCD
#define THIS_STRIDE STRIDE_MAIN
#else
#define THIS_STRIDE STRIDE_REMOTE
#endif

#define CURRENT_VP LCDFN(current_viewport)
/*** globals ***/

static FBFN(data) LCDFN(static_framebuffer)[LCDM(FBHEIGHT)][LCDM(FBWIDTH)] IRAM_LCDFRAMEBUFFER;
static void *LCDFN(frameaddress_default)(int x, int y);

static const FBFN(data) patterns[4] = {0xFFFF, 0xFF00, 0x00FF, 0x0000};

static FBFN(data) *backdrop = NULL;
static long backdrop_offset IDATA_ATTR = 0;

/* shouldn't be changed unless you want system-wide framebuffer changes! */
struct frame_buffer_t LCDFN(framebuffer_default) =
{
    .FBFN(ptr)      = &LCDFN(static_framebuffer)[0][0],
    .get_address_fn = &LCDFN(frameaddress_default),
    .stride         = THIS_STRIDE(LCDM(WIDTH), LCDM(HEIGHT)),
    .elems          = (LCDM(FBWIDTH)*LCDM(FBHEIGHT)),
};

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCDM(WIDTH),
    .height   = LCDM(HEIGHT),
    .flags    = 0,
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .buffer   = NULL,
    .fg_pattern = LCDM(DEFAULT_FG),
    .bg_pattern = LCDM(DEFAULT_BG)
};

struct viewport * CURRENT_VP IBSS_ATTR;

static unsigned fg_pattern IBSS_ATTR;
static unsigned bg_pattern IBSS_ATTR;

static void *LCDFN(frameaddress_default)(int x, int y)
{
    /* the default expects a buffer the same size as the screen */
    struct frame_buffer_t *fb = CURRENT_VP->buffer;
#if defined(MAIN_LCD) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
    size_t element = (x * LCDM(NATIVE_STRIDE)(fb->stride)) + y;
#else
    size_t element = (y * LCDM(NATIVE_STRIDE)(fb->stride)) + x;
#endif
    return fb->FBFN(ptr) + element;/*(element % fb->elems);*/
}

#include "lcd-bitmap-common.c"

/* LCD init */
void LCDFN(init)(void)
{
    /* Initialize the viewport */
    LCDFN(set_viewport)(NULL);

    LCDFN(clear_display)();
    LCDFN(init_device)();
#ifdef MAIN_LCD
    scroll_init();
#endif
}

/*** parameter handling ***/

#if !defined(MAIN_LCD) && defined(HAVE_LCD_COLOR)
/* When compiling for remote LCD and the main LCD is colour. */
unsigned lcd_remote_color_to_native(unsigned color)
{
    unsigned r = (color & 0xf800) >> 10;
    unsigned g = (color & 0x07e0) >> 5;
    unsigned b = (color & 0x001f) << 2;
    /*
     *                                     |R|
     * |Y'| = |0.299000 0.587000 0.114000| |G|
     *                                     |B|
     */
    return (5*r + 9*g + b) >> 8;
}
#endif

void LCDFN(set_foreground)(unsigned brightness)
{
    CURRENT_VP->fg_pattern = brightness;
    fg_pattern = patterns[brightness & 3];
}

unsigned LCDFN(get_foreground)(void)
{
    return CURRENT_VP->fg_pattern;
}

void LCDFN(set_background)(unsigned brightness)
{
    CURRENT_VP->bg_pattern = brightness;
    bg_pattern = patterns[brightness & 3];
}

unsigned LCDFN(get_background)(void)
{
    return CURRENT_VP->bg_pattern;
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = LCDFB(x,y>>3);
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask);
}

static void clearpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = LCDFB(x,y>>3);
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask);
}

static void clearimgpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = LCDFB(x,y>>3);
    unsigned data = *address;

    *address = data ^ ((data ^ *(FBFN(data) *)((long)address
               + backdrop_offset)) & mask);
}

static void flippixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = LCDFB(x,y>>3);

    *address ^= mask;
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

LCDFN(pixelfunc_type)* const LCDFN(pixelfuncs_bgcolor)[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

LCDFN(pixelfunc_type)* const LCDFN(pixelfuncs_backdrop)[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearimgpixel, nopixel, clearimgpixel
};

LCDFN(pixelfunc_type)* const *LCDFN(pixelfuncs) = LCDFN(pixelfuncs_bgcolor);

/* 'mask' and 'bits' contain 2 bits per pixel */
static void ICODE_ATTR flipblock(FBFN(data) *address, unsigned mask,
                                 unsigned bits)
{
    *address ^= bits & mask;
}

static void ICODE_ATTR bgblock(FBFN(data) *address, unsigned mask,
                               unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & ~bits);
}

static void ICODE_ATTR bgimgblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(FBFN(data) *)((long)address
               + backdrop_offset)) & mask & ~bits);
}

static void ICODE_ATTR fgblock(FBFN(data) *address, unsigned mask,
                               unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & bits);
}

static void ICODE_ATTR solidblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = bg_pattern;

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR solidimgblock(FBFN(data) *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;
    unsigned bgp  = *(FBFN(data) *)((long)address + backdrop_offset);

    bits     = bgp  ^ ((bgp ^ fg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR flipinvblock(FBFN(data) *address, unsigned mask,
                                    unsigned bits)
{
    *address ^= ~bits & mask;
}

static void ICODE_ATTR bginvblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask & bits);
}

static void ICODE_ATTR bgimginvblock(FBFN(data) *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ *(FBFN(data) *)((long)address
               + backdrop_offset)) & mask & bits);
}

static void ICODE_ATTR fginvblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask & ~bits);
}

static void ICODE_ATTR solidinvblock(FBFN(data) *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ bg_pattern) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

static void ICODE_ATTR solidimginvblock(FBFN(data) *address, unsigned mask,
                                        unsigned bits)
{
    unsigned data = *address;
    unsigned fgp  = fg_pattern;

    bits     = fgp  ^ ((fgp ^ *(FBFN(data) *)((long)address
               + backdrop_offset)) & bits);
    *address = data ^ ((data ^ bits) & mask);
}

LCDFN(blockfunc_type)* const LCDFN(blockfuncs_bgcolor)[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

LCDFN(blockfunc_type)* const LCDFN(blockfuncs_backdrop)[8] = {
    flipblock, bgimgblock, fgblock, solidimgblock,
    flipinvblock, bgimginvblock, fginvblock, solidimginvblock
};

LCDFN(blockfunc_type)* const *LCDFN(blockfuncs) = LCDFN(blockfuncs_bgcolor);


void LCDFN(set_backdrop)(FBFN(data) *bd)
{
    backdrop = bd;
    if (bd)
    {
        backdrop_offset = (long)bd - (long)LCDFB(0, 0);
        LCDFN(pixelfuncs) = LCDFN(pixelfuncs_backdrop);
        LCDFN(blockfuncs) = LCDFN(blockfuncs_backdrop);
    }
    else
    {
        backdrop_offset = 0;
        LCDFN(pixelfuncs) = LCDFN(pixelfuncs_bgcolor);
        LCDFN(blockfuncs) = LCDFN(blockfuncs_bgcolor);
    }
}

FBFN(data)* LCDFN(get_backdrop)(void)
{
    return backdrop;
}

static inline void setblock(FBFN(data) *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    bits    ^= data;
    *address = data ^ (bits & mask);
}

/*** drawing functions ***/

/* Clear the whole display */
void LCDFN(clear_display)(void)
{
    if (default_vp.drawmode & DRMODE_INVERSEVID)
    {
        memset(LCDFB(0, 0), patterns[default_vp.fg_pattern & 3],
               FBSIZE);
    }
    else
    {
        if (backdrop)
            memcpy(LCDFB(0, 0), backdrop, FBSIZE);
        else
            memset(LCDFB(0, 0), patterns[default_vp.bg_pattern & 3],
                   FBSIZE);
    }

    LCDFN(scroll_info).lines = 0;
}

/* Clear the current viewport */
void LCDFN(clear_viewport)(void)
{
    int lastmode;

    if (CURRENT_VP == &default_vp &&
           default_vp.buffer == &LCDFN(framebuffer_default))
    {
        LCDFN(clear_display)();
    }
    else
    {
        lastmode = CURRENT_VP->drawmode;

        /* Invert the INVERSEVID bit and set basic mode to SOLID */
        CURRENT_VP->drawmode = (~lastmode & DRMODE_INVERSEVID) |
                               DRMODE_SOLID;

        LCDFN(fillrect)(0, 0, CURRENT_VP->width, CURRENT_VP->height);

        CURRENT_VP->drawmode = lastmode;

        LCDFN(scroll_stop_viewport)(CURRENT_VP);
    }
    CURRENT_VP->flags &= ~(VP_FLAG_VP_SET_CLEAN);
}

/* Draw a horizontal line (optimised) */
void LCDFN(hline)(int x1, int x2, int y)
{
    int width;
    FBFN(data) *dst, *dst_end;
    unsigned mask;
    LCDFN(blockfunc_type) *bfunc;

    if (!LCDFN(clip_viewport_hline)(&x1, &x2, &y))
        return;

    width = x2 - x1 + 1;

    bfunc = LCDFN(blockfuncs)[CURRENT_VP->drawmode];
    dst   = LCDFB(x1,y>>3);
    mask  = 0x0101 << (y & 7);

    dst_end = dst + width;
    do
        bfunc(dst++, mask, 0xFFFFu);
    while (dst < dst_end);
}

/* Draw a vertical line (optimised) */
void LCDFN(vline)(int x, int y1, int y2)
{
    int ny;
    FBFN(data) *dst;
    int stride_dst;
    unsigned mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

    if (!LCDFN(clip_viewport_vline)(&x, &y1, &y2))
        return;

    bfunc = LCDFN(blockfuncs)[CURRENT_VP->drawmode];
    dst   = LCDFB(x,y1>>3);
    stride_dst = CURRENT_VP->buffer->stride;
    ny    = y2 - (y1 & ~7);
    mask  = (0xFFu << (y1 & 7)) & 0xFFu;
    mask |= mask << 8;
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFFFu);
        dst += stride_dst;
        mask = 0xFFFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFFFu);
}

/* Fill a rectangular area */
void LCDFN(fillrect)(int x, int y, int width, int height)
{
    int ny;
    FBFN(data) *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    LCDFN(blockfunc_type) *bfunc;
    bool fillopt = false;

    if (!LCDFN(clip_viewport_rect)(&x, &y, &width, &height, NULL, NULL))
        return;

    if (CURRENT_VP->drawmode & DRMODE_INVERSEVID)
    {
        if ((CURRENT_VP->drawmode & DRMODE_BG) && !backdrop)
        {
            fillopt = true;
            bits = bg_pattern;
        }
    }
    else
    {
        if (CURRENT_VP->drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = fg_pattern;
        }
    }
    bfunc = LCDFN(blockfuncs)[CURRENT_VP->drawmode];
    dst   = LCDFB(x,y>>3);
    stride_dst = CURRENT_VP->buffer->stride;
    ny    = height - 1 + (y & 7);
    mask  = (0xFFu << (y & 7)) & 0xFFu;
    mask |= mask << 8;
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    for (; ny >= 8; ny -= 8)
    {
        if (fillopt && (mask == 0xFFFFu))
            memset16(dst, bits, width);
        else
        {
            FBFN(data) *dst_row = dst;

            dst_end = dst_row + width;
            do
                bfunc(dst_row++, mask, 0xFFFFu);
            while (dst_row < dst_end);
        }

        dst += stride_dst;
        mask = 0xFFFFu;
    }
    mask &= mask_bottom;

    if (fillopt && (mask == 0xFFFFu))
        memset16(dst, bits, width);
    else
    {
        dst_end = dst + width;
        do
            bfunc(dst++, mask, 0xFFFFu);
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
void ICODE_ATTR LCDFN(mono_bitmap_part)(const unsigned char *src, int src_x,
                                        int src_y, int stride, int x, int y,
                                        int width, int height)
{
    int shift, ny;
    FBFN(data) *dst, *dst_end;
    int stride_dst;
    unsigned data, mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

    if (!LCDFN(clip_viewport_rect)(&x, &y, &width, &height, &src_x, &src_y))
        return;

    src    += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    y      -= src_y;
    dst    = LCDFB(x,y>>3);
    stride_dst = CURRENT_VP->buffer->stride;
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = LCDFN(blockfuncs)[CURRENT_VP->drawmode];
    mask   = 0xFFu << (shift + src_y);
             /* not byte-doubled here because shift+src_y can be > 7 */
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    if (shift == 0)
    {
        mask &= 0xFFu;
        mask |= mask << 8;

        for (; ny >= 8; ny -= 8)
        {
            const unsigned char *src_row = src;
            FBFN(data) *dst_row = dst;

            dst_end = dst_row + width;
            do
            {
                data = *src_row++;
                bfunc(dst_row++, mask, data | (data << 8));
            }
            while (dst_row < dst_end);

            src += stride;
            dst += stride_dst;
            mask = 0xFFFFu;
        }
        mask  &= mask_bottom;

        dst_end = dst + width;
        do
        {
            data = *src++;
            bfunc(dst++, mask, data | (data << 8));
        }
        while (dst < dst_end);
    }
    else
    {
        unsigned ddata;

        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            FBFN(data) *dst_col = dst++;
            unsigned mask_col = mask & 0xFFu;

            mask_col |= mask_col << 8;
            data = 0;

            for (y = ny; y >= 8; y -= 8)
            {
                data |= *src_col << shift;

                if (mask_col)
                {
                    ddata = data & 0xFFu;
                    bfunc(dst_col, mask_col, ddata | (ddata << 8));
                    mask_col = 0xFFFFu;
                }
                else
                {
                    mask_col  = (mask >> 8) & 0xFFu;
                    mask_col |= mask_col << 8;
                }

                src_col += stride;
                dst_col += stride_dst;
                data >>= 8;
            }
            data |= *src_col << shift;
            mask_col &= mask_bottom;
            ddata = data & 0xFFu;
            bfunc(dst_col, mask_col, ddata | (ddata << 8));
        }
        while (dst < dst_end);
    }
}

/* Draw a full monochrome bitmap */
void LCDFN(mono_bitmap)(const unsigned char *src, int x, int y, int width,
                        int height)
{
    LCDFN(mono_bitmap_part)(src, 0, 0, width, x, y, width, height);
}

/* About Rockbox' internal native bitmap format:
 *
 * A bitmap contains one bit in each byte of a pair of bytes for every pixel.
 * 00 = white, 01 = light grey, 10 = dark grey, 11 = black. Bits within a byte
 * are arranged vertically, LSB at top.
 * The pairs of bytes are stored as shorts, in row-major order, with word 0
 * being top left, word 1 2nd from left etc. The first row of words defines
 * pixel rows 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial native bitmap */
void ICODE_ATTR LCDFN(bitmap_part)(const FBFN(data) *src, int src_x,
                                   int src_y, int stride, int x, int y,
                                   int width, int height)
{
    int shift, ny;
    FBFN(data) *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;

    if (!LCDFN(clip_viewport_rect)(&x, &y, &width, &height, &src_x, &src_y))
        return;

    src   += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y &= 7;
    y     -= src_y;
    dst    = LCDFB(x,y>>3);
    stride_dst = CURRENT_VP->buffer->stride;
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    mask   = 0xFFu << (shift + src_y);
             /* not byte-doubled here because shift+src_y can be > 7 */
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    if (shift == 0)
    {
        mask &= 0xFFu;
        mask |= mask << 8;

        for (; ny >= 8; ny -= 8)
        {
            if (mask == 0xFFFFu)
                memcpy(dst, src, width * sizeof(FBFN(data)));
            else
            {
                const FBFN(data) *src_row = src;
                FBFN(data) *dst_row = dst;

                dst_end = dst_row + width;
                do
                    setblock(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }
            src += stride;
            dst += stride_dst;
            mask = 0xFFFFu;
        }
        mask &= mask_bottom;

        if (mask == 0xFFFFu)
            memcpy(dst, src, width * sizeof(FBFN(data)));
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
        unsigned datamask = (0xFFu << shift) & 0xFFu;

        datamask |= datamask << 8;

        dst_end = dst + width;
        do
        {
            const FBFN(data) *src_col = src++;
            FBFN(data) *dst_col = dst++;
            unsigned mask_col = mask & 0xFFu;
            unsigned data, olddata = 0;

            mask_col |= mask_col << 8;

            for (y = ny; y >= 8; y -= 8)
            {
                data = *src_col << shift;

                if (mask_col)
                {
                    setblock(dst_col, mask_col,
                             olddata ^((olddata ^ data) & datamask));
                    mask_col = 0xFFFFu;
                }
                else
                {
                    mask_col  = (mask >> 8) & 0xFFu;
                    mask_col |= mask_col << 8;
                }
                src_col += stride;
                dst_col += stride_dst;
                olddata = data >> 8;
            }
            data = *src_col << shift;
            setblock(dst_col, mask_col & mask_bottom,
                     olddata ^((olddata ^ data) & datamask));
        }
        while (dst < dst_end);
    }
}

/* Draw a full native bitmap */
void LCDFN(bitmap)(const FBFN(data) *src, int x, int y, int width, int height)
{
    LCDFN(bitmap_part)(src, 0, 0, width, x, y, width, height);
}
