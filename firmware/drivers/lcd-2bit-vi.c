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

#ifndef LCDFN /* Not compiling for remote - define macros for main LCD. */
#define LCDFN(fn) lcd_ ## fn
#define FBFN(fn)  fb_ ## fn
#define LCDM(ma)  LCD_ ## ma
#define LCDNAME "lcd_"
#define MAIN_LCD
#endif

/*** globals ***/

FBFN(data) LCDFN(framebuffer)[LCDM(FBHEIGHT)][LCDM(FBWIDTH)] IRAM_LCDFRAMEBUFFER;

static const FBFN(data) patterns[4] = {0xFFFF, 0xFF00, 0x00FF, 0x0000};

static FBFN(data) *backdrop = NULL;
static long backdrop_offset IDATA_ATTR = 0;

static struct viewport default_vp =
{
    .x        = 0,
    .y        = 0,
    .width    = LCDM(WIDTH),
    .height   = LCDM(HEIGHT),
    .font     = FONT_SYSFIXED,
    .drawmode = DRMODE_SOLID,
    .fg_pattern = LCDM(DEFAULT_FG),
    .bg_pattern = LCDM(DEFAULT_BG)
};

static struct viewport * current_vp IBSS_ATTR;

static unsigned fg_pattern IBSS_ATTR;
static unsigned bg_pattern IBSS_ATTR;

/*** Viewports ***/

void LCDFN(set_viewport)(struct viewport* vp)
{
    if (vp == NULL)
        current_vp = &default_vp;
    else
        current_vp = vp;

    fg_pattern = patterns[current_vp->fg_pattern & 3];
    bg_pattern = patterns[current_vp->bg_pattern & 3];
    
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
    LCDFN(set_viewport)(NULL);
    LCDFN(clear_display)();
#ifndef SIMULATOR
    LCDFN(init_device)();
#endif
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

void LCDFN(set_drawmode)(int mode)
{
    current_vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int LCDFN(get_drawmode)(void)
{
    return current_vp->drawmode;
}

void LCDFN(set_foreground)(unsigned brightness)
{
    current_vp->fg_pattern = brightness;
    fg_pattern = patterns[brightness & 3];
}

unsigned LCDFN(get_foreground)(void)
{
    return current_vp->fg_pattern;
}

void LCDFN(set_background)(unsigned brightness)
{
    current_vp->bg_pattern = brightness;
    bg_pattern = patterns[brightness & 3];
}

unsigned LCDFN(get_background)(void)
{
    return current_vp->bg_pattern;
}

void LCDFN(set_drawinfo)(int mode, unsigned fg_brightness,
                         unsigned bg_brightness)
{
    LCDFN(set_drawmode)(mode);
    LCDFN(set_foreground)(fg_brightness);
    LCDFN(set_background)(bg_brightness);
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
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = &LCDFN(framebuffer)[y>>3][x];
    unsigned data = *address;

    *address = data ^ ((data ^ fg_pattern) & mask);
}

static void clearpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = &LCDFN(framebuffer)[y>>3][x];
    unsigned data = *address;

    *address = data ^ ((data ^ bg_pattern) & mask);
}

static void clearimgpixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = &LCDFN(framebuffer)[y>>3][x];
    unsigned data = *address;

    *address = data ^ ((data ^ *(FBFN(data) *)((long)address
               + backdrop_offset)) & mask);
}

static void flippixel(int x, int y)
{
    unsigned mask = 0x0101 << (y & 7);
    FBFN(data) *address = &LCDFN(framebuffer)[y>>3][x];

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
        backdrop_offset = (long)bd - (long)LCDFN(framebuffer);
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
        memset(LCDFN(framebuffer), patterns[default_vp.fg_pattern & 3],
               sizeof LCDFN(framebuffer));
    }
    else
    {
        if (backdrop)
            memcpy(LCDFN(framebuffer), backdrop, sizeof LCDFN(framebuffer));
        else
            memset(LCDFN(framebuffer), patterns[default_vp.bg_pattern & 3],
                   sizeof LCDFN(framebuffer));
    }

    LCDFN(scroll_info).lines = 0;
}

/* Clear the current viewport */
void LCDFN(clear_viewport)(void)
{
    int lastmode;

    if (current_vp == &default_vp)
    {
        LCDFN(clear_display)();
    }
    else
    {
        lastmode = current_vp->drawmode;

        /* Invert the INVERSEVID bit and set basic mode to SOLID */
        current_vp->drawmode = (~lastmode & DRMODE_INVERSEVID) | 
                               DRMODE_SOLID;

        LCDFN(fillrect)(0, 0, current_vp->width, current_vp->height);

        current_vp->drawmode = lastmode;

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
        LCDFN(pixelfuncs)[current_vp->drawmode](current_vp->x+x, current_vp->y+y);
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
        DEBUGF(LCDNAME "drawline() called for vertical line - optimisation.\n");
        LCDFN(vline)(x1, y1, y2);
        return;
    }
    deltay = abs(y2 - y1);
    if (deltay == 0)
    {
        DEBUGF(LCDNAME "drawline() called for horizontal line - optimisation.\n");
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
    int x;
    int width;
    FBFN(data) *dst, *dst_end;
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

    /* adjust x1 and y to viewport */
    x1 += current_vp->x;
    y += current_vp->y;

#if defined(HAVE_VIEWPORT_CLIP)
    x2 += current_vp->x;

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
    dst   = &LCDFN(framebuffer)[y>>3][x1];
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
    dst   = &LCDFN(framebuffer)[y1>>3][x];
    ny    = y2 - (y1 & ~7);
    mask  = (0xFFu << (y1 & 7)) & 0xFFu;
    mask |= mask << 8;
    mask_bottom  = 0xFFu >> (~ny & 7);
    mask_bottom |= mask_bottom << 8;

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFFFu);
        dst += LCDM(WIDTH);
        mask = 0xFFFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFFFu);
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
        if ((current_vp->drawmode & DRMODE_BG) && !backdrop)
        {
            fillopt = true;
            bits = bg_pattern;
        }
    }
    else
    {
        if (current_vp->drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = fg_pattern;
        }
    }
    bfunc = LCDFN(blockfuncs)[current_vp->drawmode];
    dst   = &LCDFN(framebuffer)[y>>3][x];
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

        dst += LCDM(WIDTH);
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
    unsigned data, mask, mask_bottom;
    LCDFN(blockfunc_type) *bfunc;

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
    dst    = &LCDFN(framebuffer)[y>>3][x];
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = LCDFN(blockfuncs)[current_vp->drawmode];
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
            dst += LCDM(WIDTH);
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
                dst_col += LCDM(WIDTH);
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
    unsigned mask, mask_bottom;

    /******************** Image in viewport clipping **********************/
    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= current_vp->width)
        || (y >= current_vp->height) || (x + width <= 0) || (y + height <= 0))
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

    src   += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y &= 7;
    y     -= src_y;
    dst    = &LCDFN(framebuffer)[y>>3][x];
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
            dst += LCDM(WIDTH);
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
                dst_col += LCDM(WIDTH);
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

#include "lcd-bitmap-common.c"
