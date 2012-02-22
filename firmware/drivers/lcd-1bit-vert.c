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

/*** globals ***/
FBFN(data) LCDFN(static_framebuffer)[LCDM(FBHEIGHT)][LCDM(FBWIDTH)] IRAM_LCDFRAMEBUFFER;
FBFN(data) *LCDFN(framebuffer) = &LCDFN(static_framebuffer)[0][0];

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCDM(WIDTH),
    .height   = LCDM(HEIGHT),
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
};

static struct viewport* current_vp = &default_vp;

/*** Viewports ***/

void LCDFN(set_viewport)(struct viewport* vp)
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
    if((unsigned) current_vp->x > (unsigned) LCDM(WIDTH) 
        || (unsigned) current_vp->y > (unsigned) LCDM(HEIGHT) 
        || current_vp->x + current_vp->width > LCDM(WIDTH)
        || current_vp->y + current_vp->height > LCDM(HEIGHT))
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

void LCDFN(update_viewport)(void)
{
    LCDFN(update_rect)(current_vp->x, current_vp->y,
                       current_vp->width, current_vp->height);
}

void LCDFN(update_viewport_rect)(int x, int y, int width, int height)
{
    LCDFN(update_rect)(current_vp->x + x, current_vp->y + y, width, height);
}

/* LCD init */
void LCDFN(init)(void)
{
    LCDFN(clear_display)();
    LCDFN(init_device)();
#ifdef MAIN_LCD
    scroll_init();
#endif
}

/*** parameter handling ***/

void LCDFN(set_drawmode)(int mode)
{
    current_vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int LCDFN(get_drawmode)(void)
{
    return current_vp->drawmode;
}

int LCDFN(getwidth)(void)
{
    return current_vp->width;
}

int LCDFN(getheight)(void)
{
    return current_vp->height;
}

void LCDFN(setfont)(int newfont)
{
    current_vp->font = newfont;
}

int LCDFN(getfont)(void)
{
    return current_vp->font;
}

int LCDFN(getstringsize)(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, current_vp->font);
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
    unsigned bits = (current_vp->drawmode & DRMODE_INVERSEVID) ? 0xFFu : 0;

    memset(LCDFN(framebuffer), bits, FBSIZE);
    LCDFN(scroll_info).lines = 0;
}

/* Clear the current viewport */
void LCDFN(clear_viewport)(void)
{
    int oldmode;

    if (current_vp == &default_vp)
    {
        LCDFN(clear_display)();
    }
    else
    {
        oldmode = current_vp->drawmode;

        /* Invert the INVERSEVID bit and set basic mode to SOLID */
        current_vp->drawmode = (~current_vp->drawmode & DRMODE_INVERSEVID) | 
                               DRMODE_SOLID;

        LCDFN(fillrect)(0, 0, current_vp->width, current_vp->height);

        current_vp->drawmode = oldmode;

        LCDFN(scroll_stop)(current_vp);
    }
}

/* Set a single pixel */
void LCDFN(drawpixel)(int x, int y)
{
    if (   ((unsigned)x < (unsigned)current_vp->width) 
        && ((unsigned)y < (unsigned)current_vp->height)
#if defined(HAVE_VIEWPORT_CLIP)
        && ((unsigned)x < (unsigned)LCDM(WIDTH))
        && ((unsigned)y < (unsigned)LCDM(HEIGHT))
#endif
        )
        LCDFN(pixelfuncs)[current_vp->drawmode](current_vp->x + x, current_vp->y + y);
}

/* Draw a line */
void LCDFN(drawline)(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    LCDFN(pixelfunc_type) *pfunc = LCDFN(pixelfuncs)[current_vp->drawmode];

    deltax = abs(x2 - x1);
    if (deltax == 0)
    {
        /* DEBUGF(LCDNAME "drawline() called for vertical line - optimisation.\n"); */
        LCDFN(vline)(x1, y1, y2);
        return;
    }
    deltay = abs(y2 - y1);
    if (deltay == 0)
    {
        /* DEBUGF(LCDNAME "drawline() called for horizontal line - optimisation.\n"); */
        LCDFN(hline)(x1, x2, y1);
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
            && ((unsigned)x < (unsigned)LCDM(WIDTH))
            && ((unsigned)y < (unsigned)LCDM(HEIGHT))
#endif
            )
            pfunc(current_vp->x + x, current_vp->y + y);

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
void LCDFN(hline)(int x1, int x2, int y)
{
    int x, width;
    unsigned char *dst, *dst_end;
    unsigned mask;
    LCDFN(blockfunc_type) *bfunc;

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }
    
    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if (((unsigned)y >= (unsigned)current_vp->height) || (x1 >= current_vp->width)
        || (x2 < 0))
        return;  
    
    if (x1 < 0)
        x1 = 0;
    if (x2 >= current_vp->width)
        x2 = current_vp->width-1;
        
    /* adjust to viewport */
    x1 += current_vp->x;
    x2 += current_vp->x;
    y += current_vp->y;
    
#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if (((unsigned)y >= (unsigned) LCDM(HEIGHT)) || (x1 >= LCDM(WIDTH))
        || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= LCDM(WIDTH))
        x2 = LCDM(WIDTH)-1;
#endif

    width = x2 - x1 + 1;

    bfunc = LCDFN(blockfuncs)[current_vp->drawmode];
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
    int ny;
    FBFN(data) *dst;
    unsigned mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

    /* direction flip */
    if (y2 < y1)
    {
        ny = y1;
        y1 = y2;
        y2 = ny;
    }

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if (((unsigned)x >= (unsigned)current_vp->width) || (y1 >= current_vp->height)
        || (y2 < 0))
        return;  
    
    if (y1 < 0)
        y1 = 0;
    if (y2 >= current_vp->height)
        y2 = current_vp->height-1;
        
    /* adjust for viewport */
    y1 += current_vp->y;
    y2 += current_vp->y;
    x += current_vp->x;
    
#if defined(HAVE_VIEWPORT_CLIP)
    /********************* Viewport on screen clipping ********************/
    /* nothing to draw? */
    if (( (unsigned) x >= (unsigned)LCDM(WIDTH)) || (y1 >= LCDM(HEIGHT)) 
        || (y2 < 0))
        return;
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= LCDM(HEIGHT))
        y2 = LCDM(HEIGHT)-1;
#endif

    bfunc = LCDFN(blockfuncs)[current_vp->drawmode];
    dst   = LCDFB(x,y1>>3);
    ny    = y2 - (y1 & ~7);
    mask  = 0xFFu << (y1 & 7);
    mask_bottom = 0xFFu >> (~ny & 7);

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFu);
        dst += LCDM(WIDTH);
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFu);
}

/* Draw a rectangular box */
void LCDFN(drawrect)(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    LCDFN(vline)(x, y, y2);
    LCDFN(vline)(x2, y, y2);
    LCDFN(hline)(x, x2, y);
    LCDFN(hline)(x, x2, y2);
}

/* Fill a rectangular area */
void LCDFN(fillrect)(int x, int y, int width, int height)
{
    int ny;
    FBFN(data) *dst, *dst_end;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    LCDFN(blockfunc_type) *bfunc;
    bool fillopt = false;

    /******************** In viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width)
        || (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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
    if ((x >= LCDM(WIDTH)) || (y >= LCDM(HEIGHT)) 
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
    if (x + width > LCDM(WIDTH))
        width = LCDM(WIDTH) - x;
    if (y + height > LCDM(HEIGHT))
        height = LCDM(HEIGHT) - y;
#endif

    if (current_vp->drawmode & DRMODE_INVERSEVID)
    {
        if (current_vp->drawmode & DRMODE_BG)
        {
            fillopt = true;
        }
    }
    else
    {
        if (current_vp->drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = 0xFFu;
        }
    }
    bfunc = LCDFN(blockfuncs)[current_vp->drawmode];
    dst   = LCDFB(x,y>>3);
    ny    = height - 1 + (y & 7);
    mask  = 0xFFu << (y & 7);
    mask_bottom = 0xFFu >> (~ny & 7);

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

        dst += LCDM(WIDTH);
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
    int shift, ny;
    FBFN(data) *dst, *dst_end;
    unsigned mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width)
        || (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
        return;
        
    /* clip image in viewport */
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
    if ((x >= LCDM(WIDTH)) || (y >= LCDM(HEIGHT)) 
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
    if (x + width > LCDM(WIDTH))
        width = LCDM(WIDTH) - x;
    if (y + height > LCDM(HEIGHT))
        height = LCDM(HEIGHT) - y;
#endif

    src    += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y  &= 7;
    y      -= src_y;
    dst    = LCDFB(x,y>>3);
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = LCDFN(blockfuncs)[current_vp->drawmode];
    mask   = 0xFFu << (shift + src_y);
    mask_bottom = 0xFFu >> (~ny & 7);
    
    if (shift == 0)
    {
        bool copyopt = (current_vp->drawmode == DRMODE_SOLID);

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
            dst += LCDM(WIDTH);
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
                dst_col += LCDM(WIDTH);
                data >>= 8;
            }
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

#include "lcd-bitmap-common.c"
