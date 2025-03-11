/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#include <string.h>
#include <stdlib.h>
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
#define FBSIZE FRAMEBUFFER_SIZE
#define LCDM(ma) LCD_ ## ma
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
};

struct viewport* CURRENT_VP;

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

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    *LCDFB(x,y>>3) |= BIT_N(y & 7);
}

static void clearpixel(int x, int y)
{
    *LCDFB(x,y>>3) &= ~BIT_N(y & 7);
}

static void flippixel(int x, int y)
{
    *LCDFB(x,y>>3) ^= BIT_N(y & 7);
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

LCDFN(pixelfunc_type)* const LCDFN(pixelfuncs)[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

static void ICODE_ATTR flipblock(FBFN(data) *address, unsigned mask,
                                 unsigned bits)
{
    *address ^= bits & mask;
}

static void ICODE_ATTR bgblock(FBFN(data) *address, unsigned mask,
                               unsigned bits)
{
    *address &= bits | ~mask;
}

static void ICODE_ATTR fgblock(FBFN(data) *address, unsigned mask,
                               unsigned bits)
{
    *address |= bits & mask;
}

static void ICODE_ATTR solidblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    unsigned data = *(char*)address;

    bits    ^= data;
    *address = data ^ (bits & mask);
}

static void ICODE_ATTR flipinvblock(FBFN(data) *address, unsigned mask,
                                    unsigned bits)
{
    *address ^= ~bits & mask;
}

static void ICODE_ATTR bginvblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    *address &= ~(bits & mask);
}

static void ICODE_ATTR fginvblock(FBFN(data) *address, unsigned mask,
                                  unsigned bits)
{
    *address |= ~bits & mask;
}

static void ICODE_ATTR solidinvblock(FBFN(data) *address, unsigned mask,
                                     unsigned bits)
{
    unsigned data = *(char *)address;

    bits     = ~bits ^ data;
    *address = data ^ (bits & mask);
}

LCDFN(blockfunc_type)* const LCDFN(blockfuncs)[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

/*** drawing functions ***/

/* Clear the whole display */
void LCDFN(clear_display)(void)
{
    unsigned bits = (CURRENT_VP->drawmode & DRMODE_INVERSEVID) ? 0xFFu : 0;

    memset(LCDFB(0, 0), bits, FBSIZE);
    LCDFN(scroll_stop)();
}

/* Draw a horizontal line (optimised) */
void LCDFN(hline)(int x1, int x2, int y)
{
    struct viewport *vp = CURRENT_VP;
    int width;
    unsigned char *dst, *dst_end;
    unsigned mask;
    LCDFN(blockfunc_type) *bfunc;

    if (!clip_viewport_hline(vp, &x1, &x2, &y))
        return;

    width = x2 - x1 + 1;

    bfunc = LCDFN(blockfuncs)[vp->drawmode];
    dst   = LCDFB(x1,y>>3);
    mask  = BIT_N(y & 7);

    dst_end = dst + width;
    do
        bfunc(dst++, mask, 0xFFu);
    while (dst < dst_end);
}

/* Draw a vertical line (optimised) */
void LCDFN(vline)(int x, int y1, int y2)
{
    struct viewport *vp = CURRENT_VP;
    int ny;
    FBFN(data) *dst;
    int stride_dst;
    unsigned mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

    if (!clip_viewport_vline(vp, &x, &y1, &y2))
        return;

    bfunc = LCDFN(blockfuncs)[vp->drawmode];
    dst   = LCDFB(x,y1>>3);
    ny    = y2 - (y1 & ~7);
    mask  = 0xFFu << (y1 & 7);
    mask_bottom = 0xFFu >> (~ny & 7);
    stride_dst = vp->buffer->stride;

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFu);
        dst += stride_dst;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFu);
}

/* Fill a rectangular area */
void LCDFN(fillrect)(int x, int y, int width, int height)
{
    struct viewport *vp = CURRENT_VP;
    int ny;
    FBFN(data) *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    LCDFN(blockfunc_type) *bfunc;
    bool fillopt = false;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, NULL, NULL))
        return;

    if (vp->drawmode & DRMODE_INVERSEVID)
    {
        if (vp->drawmode & DRMODE_BG)
        {
            fillopt = true;
        }
    }
    else
    {
        if (vp->drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = 0xFFu;
        }
    }
    bfunc = LCDFN(blockfuncs)[vp->drawmode];
    dst   = LCDFB(x,y>>3);
    ny    = height - 1 + (y & 7);
    mask  = 0xFFu << (y & 7);
    mask_bottom = 0xFFu >> (~ny & 7);
    stride_dst = vp->buffer->stride;

    for (; ny >= 8; ny -= 8)
    {
        if (fillopt && (mask == 0xFFu))
            memset(dst, bits, width);
        else
        {
            FBFN(data) *dst_row = dst;

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

/* About Rockbox' internal bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial bitmap */
void ICODE_ATTR LCDFN(bitmap_part)(const unsigned char *src, int src_x,
                                   int src_y, int stride, int x, int y,
                                   int width, int height)
{
    struct viewport *vp = CURRENT_VP;
    int shift, ny;
    FBFN(data) *dst, *dst_end;
    int stride_dst;
    unsigned mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

    if (!clip_viewport_rect(vp, &x, &y, &width, &height, &src_x, &src_y))
        return;

    src    += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    y      -= src_y;
    dst    = LCDFB(x,y>>3);
    stride_dst = vp->buffer->stride;
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = LCDFN(blockfuncs)[vp->drawmode];
    mask   = 0xFFu << (shift + src_y);
    mask_bottom = 0xFFu >> (~ny & 7);

    if (shift == 0)
    {
        bool copyopt = (vp->drawmode == DRMODE_SOLID);

        for (; ny >= 8; ny -= 8)
        {
            if (copyopt && (mask == 0xFFu))
                memcpy(dst, src, width);
            else
            {
                const unsigned char *src_row = src;
                FBFN(data) *dst_row = dst;

                dst_end = dst_row + width;
                do
                    bfunc(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }

            src += stride;
            dst += stride_dst;
            mask = 0xFFu;
        }
        mask &= mask_bottom;

        if (copyopt && (mask == 0xFFu))
            memcpy(dst, src, width);
        else
        {
            dst_end = dst + width;
            do
                bfunc(dst++, mask, *src++);
            while (dst < dst_end);
        }
    }
    else
    {
        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            FBFN(data) *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;

            for (y = ny; y >= 8; y -= 8)
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFu)
                {
                    bfunc(dst_col, mask_col, data);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += stride_dst;
                data >>= 8;
            }
#if !(CONFIG_PLATFORM & PLATFORM_NATIVE)
            if (y>=shift) /* ASAN fix keep from reading beyond buffer */
#endif
                data |= *src_col << shift;
            bfunc(dst_col, mask_col & mask_bottom, data);
        }
        while (dst < dst_end);
    }
}

/* Draw a full bitmap */
void LCDFN(bitmap)(const unsigned char *src, int x, int y, int width,
                   int height)
{
    LCDFN(bitmap_part)(src, 0, 0, width, x, y, width, height);
}
