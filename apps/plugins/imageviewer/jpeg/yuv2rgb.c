/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#include "plugin.h"
#include "yuv2rgb.h"

/*
 * Conversion of full 0-255 range YCrCb to RGB:
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 * Scaled (yields s15-bit output):
 *   |R|   |128    0  179| |Y       |
 *   |G| = |128  -43  -91| |Cb - 128|
 *   |B|   |128  227    0| |Cr - 128|
 */
#define YFAC            128
#define RVFAC           179
#define GUFAC           (-43)
#define GVFAC           (-91)
#define BUFAC           227
#define YUV_WHITE       (255*YFAC)
#define NODITHER_DELTA  (127*YFAC)
#define COMPONENT_SHIFT  15
#define MATRIX_SHIFT      7

static inline int clamp_component_bits(int x, int bits)
{
    if ((unsigned)x > (1u << bits) - 1)
        x = x < 0 ? 0 : (1 << bits) - 1;
    return x;
}

static inline int component_to_lcd(int x, int bits, int delta)
{
    /* Formula used in core bitmap loader. */
    return (((1 << bits) - 1)*x + (x >> (8 - bits)) + delta) >> COMPONENT_SHIFT;
}

static inline int lcd_to_component(int x, int bits, int delta)
{
    /* Reasonable, approximate reversal to get a full range back from the
       quantized value. */
    return YUV_WHITE*x / ((1 << bits) - 1);
    (void)delta;
}

#define RED 0
#define GRN 1
#define BLU 2

struct rgb_err
{
    int16_t errbuf[LCD_WIDTH+2]; /* Error record for line below            */
} rgb_err_buffers[3];

struct rgb_pixel
{
    int r, g, b;                 /* Current pixel components in s16.0      */
    int inc;                     /* Current line increment (-1 or 1)       */
    int row;                     /* Current row in source image            */
    int col;                     /* Current column in source image         */
    int ce[3];                   /* Errors to apply to current pixel       */
    struct rgb_err *e;           /* RED, GRN, BLU                          */
    int epos;                    /* Current position in error record       */
};

struct rgb_pixel *pixel;

/** round and truncate to lcd depth **/
static fb_data pixel_to_lcd_colour(void)
{
    struct rgb_pixel *p = pixel;
    int r, g, b;

    r = component_to_lcd(p->r, LCD_RED_BITS, NODITHER_DELTA);
    r = clamp_component_bits(r, LCD_RED_BITS);

    g = component_to_lcd(p->g, LCD_GREEN_BITS, NODITHER_DELTA);
    g = clamp_component_bits(g, LCD_GREEN_BITS);

    b = component_to_lcd(p->b, LCD_BLUE_BITS, NODITHER_DELTA);
    b = clamp_component_bits(b, LCD_BLUE_BITS);

    return FB_RGBPACK_LCD(r, g, b);
}

/** write a monochrome pixel to the colour LCD **/
static fb_data pixel_to_lcd_gray(void)
{
    int r, g, b;

    g = pixel->g;

    r = component_to_lcd(g, LCD_RED_BITS, NODITHER_DELTA);
    r = clamp_component_bits(r, LCD_RED_BITS);

    b = component_to_lcd(g, LCD_BLUE_BITS, NODITHER_DELTA);
    b  = clamp_component_bits(b, LCD_BLUE_BITS);

    g = component_to_lcd(g, LCD_GREEN_BITS, NODITHER_DELTA);
    g = clamp_component_bits(g, LCD_GREEN_BITS);

    return FB_RGBPACK_LCD(r, g, b);
}

/**
 * Bayer ordered dithering - swiped from the core bitmap loader.
 */
static fb_data pixel_odither_to_lcd(void)
{
    /* canonical ordered dither matrix */
    static const unsigned char dither_matrix[16][16] = {
        {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
        { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
        {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
        { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
        {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
        { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
        {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
        { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
        {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
        { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
        {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
        { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
        {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
        { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
        {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
        { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
    };

    struct rgb_pixel *p = pixel;
    int r, g, b, delta;

    delta = dither_matrix[p->col & 15][p->row & 15] << MATRIX_SHIFT;

    r = component_to_lcd(p->r, LCD_RED_BITS, delta);
    r = clamp_component_bits(r, LCD_RED_BITS);

    g = component_to_lcd(p->g, LCD_GREEN_BITS, delta);
    g = clamp_component_bits(g, LCD_GREEN_BITS);

    b = component_to_lcd(p->b, LCD_BLUE_BITS, delta);
    b = clamp_component_bits(b, LCD_BLUE_BITS);

    p->col += p->inc;

    return FB_RGBPACK_LCD(r, g, b);
} 

/**
 * Floyd/Steinberg dither to lcd depth.
 *
 * Apply filter to each component in serpentine pattern. Kernel shown for
 * L->R scan. Kernel is reversed for R->L.
 *        *   7
 *    3   5   1     (1/16)
 */
static inline void distribute_error(int *ce, struct rgb_err *e,
                                    int err, int epos, int inc)
{
    *ce                  = (7*err >> 4) + e->errbuf[epos+inc];
    e->errbuf[epos+inc]  =   err >> 4;
    e->errbuf[epos]     += 5*err >> 4;
    e->errbuf[epos-inc] += 3*err >> 4;
}

static fb_data pixel_fsdither_to_lcd(void)
{
    struct rgb_pixel *p = pixel;
    int rc, gc, bc, r, g, b;
    int inc, epos;

    /* Full components with error terms */
    rc = p->r + p->ce[RED];
    r  = component_to_lcd(rc, LCD_RED_BITS, 0);
    r  = clamp_component_bits(r, LCD_RED_BITS);

    gc = p->g + p->ce[GRN];
    g  = component_to_lcd(gc, LCD_GREEN_BITS, 0);
    g  = clamp_component_bits(g, LCD_GREEN_BITS);

    bc = p->b + p->ce[BLU];
    b  = component_to_lcd(bc, LCD_BLUE_BITS, 0);
    b  = clamp_component_bits(b, LCD_BLUE_BITS);

    /* Get pixel errors */
    rc -= lcd_to_component(r, LCD_RED_BITS, 0);
    gc -= lcd_to_component(g, LCD_GREEN_BITS, 0);
    bc -= lcd_to_component(b, LCD_BLUE_BITS, 0);

    /* Spead error to surrounding pixels. */
    inc      = p->inc;
    epos     = p->epos;
    p->epos += inc;

    distribute_error(&p->ce[RED], &p->e[RED], rc, epos, inc);
    distribute_error(&p->ce[GRN], &p->e[GRN], gc, epos, inc);
    distribute_error(&p->ce[BLU], &p->e[BLU], bc, epos, inc);

    /* Pack and return pixel */
    return FB_RGBPACK_LCD(r, g, b);
}

/* Functions for each output mode, colour then grayscale. */
static fb_data (* const pixel_funcs[COLOUR_NUM_MODES][DITHER_NUM_MODES])(void) =
{
    [COLOURMODE_COLOUR] =
    {
        [DITHER_NONE]      = pixel_to_lcd_colour,
        [DITHER_ORDERED]   = pixel_odither_to_lcd,
        [DITHER_DIFFUSION] = pixel_fsdither_to_lcd,
    },
    [COLOURMODE_GRAY] =
    {
        [DITHER_NONE]      = pixel_to_lcd_gray,
        [DITHER_ORDERED]   = pixel_odither_to_lcd,
        [DITHER_DIFFUSION] = pixel_fsdither_to_lcd,
    },
};

/* These defines are used fornormal horizontal strides and vertical strides. */
#if LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define LCDADDR(x, y)   (lcd_fb + LCD_HEIGHT*(x) + (y))
#define ROWENDOFFSET    (width*LCD_HEIGHT)
#define ROWOFFSET       (1)
#define COLOFFSET       (LCD_HEIGHT)
#else
#define LCDADDR(x, y) (lcd_fb + LCD_WIDTH*(y) + (x))
#define ROWENDOFFSET    (width)
#define ROWOFFSET       (LCD_WIDTH)
#define COLOFFSET       (1)
#endif

/**
 * Draw a partial YUV colour bitmap
 *
 * Runs serpentine pattern when dithering is DITHER_DIFFUSION, else scan is
 * always L->R.
 */
void yuv_bitmap_part(unsigned char *src[3], int csub_x, int csub_y,
                     int src_x, int src_y, int stride,
                     int x, int y, int width, int height,
                     int colour_mode, int dither_mode)
{
    struct viewport *vp_main = *(rb->screens[SCREEN_MAIN]->current_viewport);
    fb_data *lcd_fb = vp_main->buffer->fb_ptr;
    fb_data *dst, *dst_end;
    fb_data (*pixel_func)(void);
    struct rgb_pixel px;
    int dst_inc;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    pixel = &px;

    dst = LCDADDR(x, y);
    dst_end = LCDADDR(x, y+height);

    if (colour_mode == COLOURMODE_GRAY)
        csub_y = 0; /* Ignore Cb, Cr */

    pixel_func = pixel_funcs[colour_mode]
                            [dither_mode];

    if (dither_mode == DITHER_DIFFUSION)
    {
        /* Reset error terms. */
        px.e = rgb_err_buffers;
        px.ce[RED] = px.ce[GRN] = px.ce[BLU] = 0;
        rb->memset(px.e, 0, 3*sizeof (struct rgb_err));
    }

    do
    {
        fb_data *dst_row, *row_end;
        const unsigned char *ysrc;
        px.inc = 1;

        if (dither_mode == DITHER_DIFFUSION)
        {
            /* Use R->L scan on odd lines */
            px.inc -= (src_y & 1) << 1;
            px.epos = x + 1;

            if (px.inc < 0)
                px.epos += width - 1;
        }

        if (px.inc == 1)
        {
            /* Scan is L->R */
            dst_inc = COLOFFSET;
            dst_row = dst;
            row_end = dst_row + ROWENDOFFSET;
            px.col  = src_x;
        }
        else
        {
            /* Scan is R->L */
            dst_inc = -COLOFFSET;
            row_end = dst + dst_inc;
            dst_row = row_end + ROWENDOFFSET;
            px.col  = src_x + width - 1;
        }

        ysrc = src[0] + stride * src_y + px.col;
        px.row = src_y;

        /* Do one row of pixels */
        if (csub_y) /* colour */
        {
            /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
            const unsigned char *usrc, *vsrc;

            usrc = src[1] + (stride/csub_x) * (src_y/csub_y)
                                            + (px.col/csub_x);
            vsrc = src[2] + (stride/csub_x) * (src_y/csub_y)
                                            + (px.col/csub_x);
            int xphase = px.col % csub_x;
            int xphase_reset = px.inc * csub_x;
            int y, v, u, rv, guv, bu;

            v     = *vsrc - 128;
            vsrc += px.inc;
            u     = *usrc - 128;
            usrc += px.inc;
            rv    =           RVFAC*v;
            guv   = GUFAC*u + GVFAC*v;
            bu    = BUFAC*u;

            while (1)
            {
                y     = YFAC*(*ysrc);
                ysrc += px.inc;
                px.r  = y + rv;
                px.g  = y + guv;
                px.b  = y + bu;

                *dst_row = pixel_func();
                dst_row += dst_inc;

                if (dst_row == row_end)
                    break;

                xphase += px.inc;
                if ((unsigned)xphase < (unsigned)csub_x)
                    continue;

                /* fetch new chromas */
                v     = *vsrc - 128;
                vsrc += px.inc;
                u     = *usrc - 128;
                usrc += px.inc;
                rv    =           RVFAC*v;
                guv   = GUFAC*u + GVFAC*v;
                bu    = BUFAC*u;

                xphase -= xphase_reset;
            }
        }
        else /* monochrome */
        {
            do
            {
                /* Set all components the same for dithering purposes */
                px.g  = px.r = px.b = YFAC*(*ysrc);
                *dst_row = pixel_func();
                ysrc    += px.inc;
                dst_row += dst_inc;
            }
            while (dst_row != row_end);
        }

        src_y++;
        dst += ROWOFFSET;
    }
    while (dst < dst_end);
}
