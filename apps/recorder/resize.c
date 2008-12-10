/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Akio Idehara, Andrew Mahone
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

/*
 * Implementation of area average and linear row and vertical scalers, and
 * nearest-neighbor grey scaler (C) 2008 Andrew Mahone
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inttypes.h"
#include "debug.h"
#include "lcd.h"
#include "file.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#ifdef ROCKBOX_DEBUG_SCALERS
#define SDEBUGF DEBUGF
#else
#define SDEBUGF(...)
#endif
#ifndef __PCTOOL__
#include "config.h"
#include "system.h"
#include "bmp.h"
#include "resize.h"
#include "resize.h"
#include "debug.h"
#else
#undef DEBUGF
#define DEBUGF(...)
#endif

/* All of these scalers use variations of Bresenham's algorithm to convert from
   their input to output coordinates. The color scalers have the error value
   shifted so that it is a useful input to the scaling algorithm.
*/

#ifdef HAVE_LCD_COLOR
/* dither + pack on channel of RGB565, R an B share a packing macro */
#define PACKRB(v, delta)  ((31 * v + (v >> 3) + delta) >> 8)
#define PACKG(g, delta) ((63 * g + (g >> 2) + delta) >> 8)

/* read new img_part unconditionally, return false on failure */
#define FILL_BUF_INIT(img_part, store_part, args) { \
    img_part = store_part(args); \
    if (img_part == NULL) \
        return false; \
}

/* read new img_part if current one is empty, return false on failure */
#define FILL_BUF(img_part, store_part, args) { \
    if (img_part->len == 0) \
        img_part = store_part(args); \
    if (img_part == NULL) \
        return false; \
}

struct uint32_rgb {
    uint32_t r;
    uint32_t g;
    uint32_t b;
};

struct scaler_context {
    uint32_t divmul;
    uint32_t round;
    struct img_part* (*store_part)(void *);
    long last_tick;
    unsigned char *buf;
    int len;
    void *args;
};

/* Set up rounding and scale factors for horizontal area scaler */
static void scale_h_area_setup(struct bitmap *bm, struct dim *src,
                               struct scaler_context *ctx)
{
    (void) bm;
/* sum is output value * src->width */
    ctx->divmul = ((src->width - 1 + 0x80000000U) / src->width) << 1;
    ctx->round = (src->width + 1) >> 1;
}

/* horizontal area average scaler */
static bool scale_h_area(struct bitmap *bm, struct dim *src,
                         struct uint32_rgb *out_line,
                         struct scaler_context *ctx, bool accum)
{
    SDEBUGF("scale_h_area\n");
    unsigned int ix, ox, oxe, mul;
    struct uint32_rgb rgbvalacc = { 0, 0, 0 }, 
                      rgbvaltmp = { 0, 0, 0 };
    struct img_part *part;
    FILL_BUF_INIT(part,ctx->store_part,ctx->args);
    ox = 0;
    oxe = 0;
    mul = 0;
    for (ix = 0; ix < (unsigned int)src->width; ix++)
    {
        oxe += bm->width;
        /* end of current area has been reached */
        if (oxe >= (unsigned int)src->width)
        {
            /* yield if we haven't since last tick */
            if (ctx->last_tick != current_tick)
            {
                yield();
                ctx->last_tick = current_tick;
            }
            /* "reset" error, which now represents partial coverage of next
               pixel by the next area
            */
            oxe -= src->width;
            /* add saved partial pixel from start of area */
            rgbvalacc.r = rgbvalacc.r * bm->width + rgbvaltmp.r * mul;
            rgbvalacc.g = rgbvalacc.g * bm->width + rgbvaltmp.g * mul;
            rgbvalacc.b = rgbvalacc.b * bm->width + rgbvaltmp.b * mul;
            /* fill buffer if needed */
            FILL_BUF(part,ctx->store_part,ctx->args);
            /* get new pixel , then add its partial coverage to this area */
            rgbvaltmp.r = part->buf->red;
            rgbvaltmp.g = part->buf->green;
            rgbvaltmp.b = part->buf->blue;
            part->buf++;
            part->len--;
            mul = bm->width - oxe;
            rgbvalacc.r += rgbvaltmp.r * mul;
            rgbvalacc.g += rgbvaltmp.g * mul;
            rgbvalacc.b += rgbvaltmp.b * mul;
            /* round, divide, and either store or accumulate to output row */
            out_line[ox].r = (accum ? out_line[ox].r : 0) +
                             ((rgbvalacc.r + ctx->round) *
                             (uint64_t)ctx->divmul >> 32);
            out_line[ox].g = (accum ? out_line[ox].g : 0) +
                             ((rgbvalacc.g + ctx->round) *
                             (uint64_t)ctx->divmul >> 32);
            out_line[ox].b = (accum ? out_line[ox].b : 0) +
                             ((rgbvalacc.b + ctx->round) *
                             (uint64_t)ctx->divmul >> 32);
            /* reset accumulator */
            rgbvalacc.r = 0;
            rgbvalacc.g = 0;
            rgbvalacc.b = 0;
            mul = bm->width - mul;
            ox += 1;
        /* inside an area */
        } else {
            /* fill buffer if needed */
            FILL_BUF(part,ctx->store_part,ctx->args);
            /* add pixel value to accumulator */
            rgbvalacc.r += part->buf->red;
            rgbvalacc.g += part->buf->green;
            rgbvalacc.b += part->buf->blue;
            part->buf++;
            part->len--;
        }
    }
    return true;
}

/* vertical area average scaler */
static bool scale_v_area(struct bitmap *bm, bool dither, struct dim *src,
                         struct rowset *rset,
                         bool (*h_scaler)(struct bitmap*, struct dim*,
                                          struct uint32_rgb*,
                                          struct scaler_context*, bool),
                         struct scaler_context *ctx)
{
    uint32_t mul, divmul, x, oy, iy, oye, round;
    int delta = 127, r, g, b;
    fb_data *row, *pix;

    /* Set up rounding and scale factors */
    divmul = ((src->height - 1 + 0x80000000U) / src->height) << 1;
    round = (src->height + 1) >> 1;
    mul = 0;
    oy = 0;
    oye = 0;
    struct uint32_rgb *rowacc = (struct uint32_rgb *)(ctx->buf),
                      *rowtmp = rowacc + bm->width;

    SDEBUGF("scale_v_area\n");
    /* zero the accumulator and temp rows */
    memset((void *)ctx->buf, 0, bm->width * 2 * sizeof(struct uint32_rgb));
    row = (fb_data *)(bm->data) + bm->width * rset->rowstart;
    for (iy = 0; iy < (unsigned int)src->height; iy++)
    {
        oye += bm->height;
        /* end of current area has been reached */
        if (oye >= (unsigned int)src->height)
        {
            /* "reset" error, which now represents partial coverage of the next
               row by the next area
            */
            oye -= src->height;
            /* add stored partial row to accumulator */
            for (x = 0; x < 3 *(unsigned int)bm->width; x++)
                ((uint32_t*)rowacc)[x] = ((uint32_t*)rowacc)[x] *
                                        bm->height + mul *
                                        ((uint32_t*)rowtmp)[x];
            /* store new scaled row in temp row */
            if(!h_scaler(bm, src, rowtmp, ctx, false))
                return false;
            /* add partial coverage by new row to this area, then round and
               scale to final value
            */
            mul = bm->height - oye;
            for (x = 0; x < 3 *(unsigned int)bm->width; x++)
            {
                ((uint32_t*)rowacc)[x] += mul * ((uint32_t*)rowtmp)[x];
                ((uint32_t*)rowacc)[x] = (round +
                                        ((uint32_t*)rowacc)[x]) *
                                        (uint64_t)divmul >> 32;
            }
            /* convert to RGB565 in output bitmap */
            pix = row;
            for (x = 0; x < (unsigned int)bm->width; x++)
            {
                if (dither)
                    delta = dither_mat(x & 0xf, oy & 0xf);
                r = PACKRB(rowacc[x].r,delta);
                g = PACKG(rowacc[x].g,delta);
                b = PACKRB(rowacc[x].b,delta);
                *pix++ = LCD_RGBPACK_LCD(r, g, b);
            }
            /* clear accumulator row, store partial coverage for next row */
            memset((void *)rowacc, 0, bm->width * sizeof(struct uint32_rgb));
            mul = oye;
            row += bm->width * rset->rowstep;
            oy += 1;
        /* inside an area */
        } else {
            /* accumulate new scaled row to rowacc */
            if (!h_scaler(bm, src, rowacc, ctx, true))
                return false;
        }
    }
    return true;
}

#ifdef HAVE_UPSCALER
/* Set up rounding and scale factors for the horizontal scaler. The divisor
   is bm->width - 1, so that the first and last pixels in the row align
   exactly between input and output
*/
static void scale_h_linear_setup(struct bitmap *bm, struct dim *src,
                                   struct scaler_context *ctx)
{
    (void) src;
    ctx->divmul = ((bm->width - 2 + 0x80000000U) / (bm->width - 1)) << 1;
    ctx->round = bm->width >> 1;
}

/* horizontal linear scaler */
static bool scale_h_linear(struct bitmap *bm, struct dim *src,
                             struct uint32_rgb *out_line,
                             struct scaler_context *ctx, bool accum) 
{
    unsigned int ix, ox, ixe;
    /* type x = x is an ugly hack for hiding an unitialized data warning. The
       values are conditionally initialized before use, but other values are
       set such that this will occur before these are used.
    */
    struct uint32_rgb rgbval=rgbval, rgbinc=rgbinc;
    struct img_part *part;
    SDEBUGF("scale_h_linear\n");
    FILL_BUF_INIT(part,ctx->store_part,ctx->args);
    ix = 0;
    /* The error is set so that values are initialized on the first pass. */
    ixe = bm->width - 1;
    for (ox = 0; ox < (uint32_t)bm->width; ox++) {
        if (ixe >= ((uint32_t)bm->width - 1))
        {
            /* yield once each tick */
            if (ctx->last_tick != current_tick)
            {
                yield();
                ctx->last_tick = current_tick;
            }
            /* Store the new "current" pixel value in rgbval, and the color 
               step value in rgbinc.
            */
            ixe -= (bm->width - 1);
            rgbinc.r = -(part->buf->red);
            rgbinc.g = -(part->buf->green);
            rgbinc.b = -(part->buf->blue);
            rgbval.r = (part->buf->red) * (bm->width - 1);
            rgbval.g = (part->buf->green) * (bm->width - 1);
            rgbval.b = (part->buf->blue) * (bm->width - 1);
            ix += 1;
            /* If this wasn't the last pixel, add the next one to rgbinc. */
            if (ix < (uint32_t)src->width) {
                part->buf++;
                part->len--;
                /* Fetch new pixels if needed */
                FILL_BUF(part,ctx->store_part,ctx->args);
                rgbinc.r += part->buf->red;
                rgbinc.g += part->buf->green;
                rgbinc.b += part->buf->blue;
                /* Add a partial step to rgbval, in this pixel isn't precisely
                   aligned with the new source pixel
                */
                rgbval.r += rgbinc.r * ixe;
                rgbval.g += rgbinc.g * ixe;
                rgbval.b += rgbinc.b * ixe;
            }
            /* Now multiple the color increment to its proper value */
            rgbinc.r *= src->width - 1;
            rgbinc.g *= src->width - 1;
            rgbinc.b *= src->width - 1;
        }
        /* round and scale values, and accumulate or store to output */
        out_line[ox].r = (accum ? out_line[ox].r : 0) +
                         ((rgbval.r + ctx->round) *
                         (uint64_t)ctx->divmul >> 32);
        out_line[ox].g = (accum ? out_line[ox].g : 0) +
                         ((rgbval.g + ctx->round) *
                         (uint64_t)ctx->divmul >> 32);
        out_line[ox].b = (accum ? out_line[ox].b : 0) +
                         ((rgbval.b + ctx->round) *
                         (uint64_t)ctx->divmul >> 32);
        rgbval.r += rgbinc.r;
        rgbval.g += rgbinc.g;
        rgbval.b += rgbinc.b;
        ixe += src->width - 1;
    }
    return true;
}

/* vertical linear scaler */
static bool scale_v_linear(struct bitmap *bm, bool dither, struct dim *src,
                           struct rowset *rset,
                           bool (*h_scaler)(struct bitmap*, struct dim*,
                                            struct uint32_rgb*,
                                            struct scaler_context*, bool),
                           struct scaler_context *ctx)
{
    uint32_t mul, divmul, x, oy, iy, iye, round;
    int delta = 127;
    struct uint32_rgb p;
    fb_data *row, *pix;
    /* Set up scale and rounding factors, the divisor is bm->height - 1 */
    divmul = ((bm->height - 2 + 0x80000000U) / (bm->height - 1)) << 1;
    round = bm->height >> 1; 
    mul = 0;
    iy = 0;
    iye = bm->height - 1;
    /* Set up our two temp buffers. The names are generic because they'll be
       swapped each time a new input row is read
    */
    struct uint32_rgb *crow1 = (struct uint32_rgb *)(ctx->buf),
                      *crow2 = crow1 + bm->width,
                      *t;

    SDEBUGF("scale_v_linear\n");
    row = (fb_data *)(bm->data) + bm->width * rset->rowstart;
    /* get first scaled row in crow2 */
    if(!h_scaler(bm, src, crow2, ctx, false))
        return false;
    for (oy = 0; oy < (uint32_t)bm->height; oy++)
    {
        if (iye >= (uint32_t)bm->height - 1)
        {
            /* swap temp rows, then read another row into crow2 */
            t = crow2;
            crow2 = crow1;
            crow1 = t;
            iye -= bm->height - 1;
            iy += 1;
            if (iy < (uint32_t)src->height)
            {
                if (!h_scaler(bm, src, crow2, ctx, false))
                    return false;
            }
        }
        pix = row;
        for (x = 0; x < (uint32_t)bm->width; x++)
        {
            /* iye and bm-height - 1 - iye represent the contribution of each
               row to the output. Calculate their weighted sum, then round and
               scale it.
            */
            p.r = (crow1[x].r * (bm->height - 1 - iye) +
                  crow2[x].r * iye + round) * (uint64_t)divmul >> 32;
            p.g = (crow1[x].g * (bm->height - 1 - iye) +
                  crow2[x].g * iye + round) * (uint64_t)divmul >> 32;
            p.b = (crow1[x].b * (bm->height - 1 - iye) +
                  crow2[x].b * iye + round) * (uint64_t)divmul >> 32;
            /* dither and pack pixels to output */
            if (dither)
                delta = dither_mat(x & 0xf, oy & 0xf);
            p.r = PACKRB(p.r,delta);
            p.g = PACKG(p.g,delta);
            p.b = PACKRB(p.b,delta);
            *pix++ = LCD_RGBPACK_LCD(p.r, p.g, p.b);
        }
        row += bm->width * rset->rowstep;
        iye += src->height - 1;
    }
    return true;
}
#endif /* HAVE_UPSCALER */
#endif /* HAVE_LCD_COLOR */

/* docs for this are still TODO, but it's Bresenham's again, used to skip or
   repeat input pixels, and with the *ls values being used for "long steps"
   that skip all the way, or nearly all the way, to the next transition of
   the associated value.
*/
#if LCD_DEPTH < 8 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH < 8)
/* nearest-neighbor up/down/non-scaler */
static inline bool scale_nearest(struct bitmap *bm,
                                struct dim *src,
                                struct rowset *rset,
                                bool remote, bool dither,
                                struct img_part* (*store_part)(void *args),
                                bool (*skip_lines)(void *args, unsigned int),
                                void *args)
{
    const int sw = src->width;
    const int sh = src->height;
    const int dw = bm->width;
    const int dh = bm->height;
    unsigned char *bitmap = bm->data;
    const int rowstep = rset->rowstep;
    const int rowstart = rset->rowstart;
    const int rowstop = rset->rowstop;
    const int fb_width = get_fb_width(bm, false);
    long last_tick = current_tick;
    int ix, ox, lx, xe, iy, oy, ly, ye, yet, oyt;
    int ixls, xels, iyls, yelsi, oyls, yelso, p;
    struct img_part *cur_part;
#ifndef HAVE_LCD_COLOR
    fb_data *dest = dest, *dest_t;
#endif
#ifdef HAVE_REMOTE_LCD
    fb_remote_data *rdest = rdest, *rdest_t;
#endif

    SDEBUGF("scale_nearest sw=%d sh=%d dw=%d dh=%d remote=%d\n", sw, sh, dw,
            dh, remote);
    ly = 0;
    iy = 0;
    ye = 0;
    ixls = (sw > (dw - 1) && dw > 1) ? sw / (dw - 1) : 1;
    xels = sw - ixls * (dw - 1) + (dw == 1 ? 1 : 0);
    iyls = (sh > (dh - 1) && dh > 1) ? sh / (dh - 1) : 1;
    oyls = dh > sh ? dh / sh : 1;
    yelsi = iyls * (dh - 1) + (dh == 1 ? 1 : 0);
    yelso = oyls * sh;
    oyls *= rowstep;
    int delta = 127;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING || \
    (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_PIXELFORMAT == HORIZONTAL_PACKING)
    uint8_t buf[4];
    int data, oxt;
#endif
#if LCD_PIXELFORMAT == VERTICAL_PACKING || \
    LCD_PIXELFORMAT == VERTICAL_INTERLEAVED || \
    (defined(HAVE_REMOTE_LCD) && \
    (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED || \
    LCD_REMOTE_PIXELFORMAT == VERTICAL_PACKING))
    int bright, shift;
#endif
    for (oy = rowstart; oy != rowstop;) {
        SDEBUGF("oy=%d iy=%d\n", oy, iy);
        if (last_tick != current_tick)
        {
            yield();
            last_tick = current_tick;
        }
        if (iy > ly && !skip_lines(args, iy - ly - 1))
            return false;
        ly = iy;

        cur_part = store_part(args);
        if (cur_part == NULL)
            return false;

        lx = 0;
        ix = 0;
        xe = 0;
#if defined(HAVE_REMOTE_LCD) && !defined(HAVE_LCD_COLOR) 
        if(!remote)
#else
        (void)remote;
#endif
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
            dest = (fb_data *)bitmap + fb_width * oy;
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
            dest = (fb_data *)bitmap + fb_width * (oy >> 2);
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
            dest = (fb_data *)bitmap + fb_width * (oy >> 3);
#endif
#ifdef HAVE_REMOTE_LCD
#ifndef HAVE_LCD_COLOR 
        else
#endif
            rdest = (fb_remote_data *)bitmap + fb_width * (oy >> 3);
#endif
        for (ox = 0; ox < dw; ox++) {
            while (cur_part->len <= ix - lx)
            {
                lx += cur_part->len;
                cur_part = store_part(args);
                if (cur_part == NULL)
                    return false;
            }
            cur_part->len -= ix - lx;
            cur_part->buf += ix - lx;
            lx = ix;
#if defined(HAVE_REMOTE_LCD) && !defined(HAVE_LCD_COLOR) 
            if(!remote)
            {
#endif
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
                /* greyscale iPods */
                buf[ox & 3] = brightness(*(cur_part->buf));
                if ((ox & 3) == 3 || ox == dw - 1)
                {
                    dest_t = dest++;
                    oyt = oy;
                    yet = ye;
                    int xo = ox & ~3;
                    while(yet < dh)
                    {
                        data = 0;
                        for (oxt = 0; oxt < (ox & 3) + 1; oxt++)
                        {
                            if (dither)
                                delta = dither_mat(oyt & 0xf, (xo + oxt) & 0xf);
                            p = (3 * buf[oxt] + (buf[oxt] >> 6) + delta) >> 8;
                            data |= (~p & 3) << ((3 - oxt) << 1);
                        }
                        *dest_t = data;
                        dest_t += rowstep * fb_width;
                        yet += sh;
                        oyt += 1;
                    }
                }
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
                /* iriver H1x0 */
                bright = brightness(*(cur_part->buf));
                dest_t = dest++;
                oyt = oy;
                yet = ye;
                while(yet < dh)
                {
                    shift = (oyt & 3) << 1;
                    if (dither)
                        delta = dither_mat(oyt & 0xf, ox & 0xf);

                    p = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest_t |= (~p & 3) << shift;
                    if ((rowstep > 0 && shift == 6) || shift == 0)
                        dest_t += rowstep * fb_width;
                    yet += sh;
                    oyt += 1;
                }
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
                bright = brightness(*(cur_part->buf));
                dest_t = dest++;
                oyt = oy;
                yet = ye;
                while(yet < dh)
                {
                    shift = oyt & 7;
                    if (dither)
                        delta = dither_mat(oyt & 0xf, ox & 0xf);

                    p = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest_t |= vi_pat(p) << shift;
                    if ((rowstep > 0 && shift == 7) || shift == 0)
                        dest_t += rowstep * fb_width;
                    yet += sh;
                    oyt += 1;
                }
#endif /* LCD_PIXELFORMAT */
#ifdef HAVE_REMOTE_LCD
#ifndef HAVE_LCD_COLOR
            } else
#endif
            {
#if LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED
                bright = brightness(*(cur_part->buf));
                rdest_t = rdest++;
                oyt = oy;
                yet = ye;
                while(yet < dh)
                {
                    shift = oyt & 7;
                    if (dither)
                        delta = dither_mat(oyt & 0xf, ox & 0xf);

                    p = (3 * bright + (bright >> 6) + delta) >> 8;
                    *rdest_t |= vi_pat(p) << shift;
                    if ((rowstep > 0 && shift == 7) || shift == 0)
                        rdest_t += rowstep * fb_width;
                    yet += sh;
                    oyt += 1;
                }
#else
                bright = brightness(*(cur_part->buf));
                rdest_t = rdest++;
                oyt = oy;
                yet = ye;
                while(yet < dh)
                {
                    shift = oyt & 7;
                    if (dither)
                        delta = dither_mat(oyt & 0xf, ox & 0xf);
                    p = (bright + delta) >> 8;
                    *rdest_t |= (~p & 1) << shift;
                    if ((rowstep > 0 && shift == 7) || shift == 0)
                        rdest_t += rowstep * fb_width;
                    yet += sh;
                    oyt += 1;
                }
#endif
            }
#endif
            xe += xels;
            ix += ixls;
            while (xe >= dw)
            {
                xe -= dw - 1;
                ix += 1;
            }
        }
        oy += oyls;
        ye += yelso;
        while (ye < dh)
        {
            ye += sh;
            oy += rowstep;
        }
        iy += iyls;
        ye -= yelsi;
        while (ye >= dh)
        {
            ye -= dh - 1;
            iy += 1;
        }
    }
    return true;
}
#endif

int resize_on_load(struct bitmap *bm, bool dither, struct dim *src,
                   struct rowset *rset, bool remote,
#ifdef HAVE_LCD_COLOR
                   unsigned char *buf, unsigned int len,
#endif
                   struct img_part* (*store_part)(void *args),
                   bool (*skip_lines)(void *args, unsigned int lines), 
                   void *args)
{

#if defined(HAVE_LCD_COLOR) && !defined(HAVE_REMOTE_LCD)
    (void)skip_lines;
#endif
#ifdef HAVE_LCD_COLOR
#ifdef HAVE_REMOTE_LCD
        if (!remote)
#endif
        {
#ifdef HAVE_UPSCALER
            const int sw = src->width;
            const int sh = src->height;
            const int dw = bm->width;
            const int dh = bm->height;
#endif
            int ret;
            unsigned int needed = sizeof(struct uint32_rgb) * 2 * bm->width;
#if MAX_SC_STACK_ALLOC
            uint8_t sc_buf[(needed <= len  || needed > MAX_SC_STACK_ALLOC) ?
                           0 : needed];
#endif
            if (needed > len)
            {
#if MAX_SC_STACK_ALLOC
                if (needed > MAX_SC_STACK_ALLOC)
                {
                    DEBUGF("unable to allocate required buffer: %d needed, "
                           "%d available, %d permitted from stack\n",
                           needed, len, MAX_SC_STACK_ALLOC);
                    return 0;
                }
                if (sizeof(sc_buf) < needed)
                {
                    DEBUGF("failed to allocate large enough buffer on stack: "
                           "%d needed, only got %d",
                           needed, MAX_SC_STACK_ALLOC);
                    return 0;
                }
#else
                DEBUGF("unable to allocate required buffer: %d needed, "
                       "%d available\n", needed, len);
                return 0;
#endif
            }

            bool (*h_scaler)(struct bitmap*, struct dim*,
                             struct uint32_rgb*,
                             struct scaler_context*, bool);
            struct scaler_context ctx;
            ctx.last_tick = current_tick;
            cpu_boost(true);
#ifdef HAVE_UPSCALER
            if (sw > dw)
            {
#endif
                h_scaler = scale_h_area;
                scale_h_area_setup(bm, src, &ctx); 
#ifdef HAVE_UPSCALER
            } else {
                h_scaler = scale_h_linear;
                scale_h_linear_setup(bm, src, &ctx);
            }
#endif
            ctx.store_part = store_part;
            ctx.args = args;
#if MAX_SC_STACK_ALLOC
            ctx.buf = needed > len ? sc_buf : buf;
#else
            ctx.buf = buf;
#endif
            ctx.len = len;
#ifdef HAVE_UPSCALER
            if (sh > dh)
#endif
                ret = scale_v_area(bm, dither, src, rset, h_scaler, &ctx);
#ifdef HAVE_UPSCALER
            else
                ret = scale_v_linear(bm, dither, src, rset, h_scaler, &ctx);
#endif
            cpu_boost(false);
            if (!ret)
                return 0;
        }
#ifdef HAVE_REMOTE_LCD
        else
#endif
#endif /* HAVE_LCD_COLOR */
#if !defined(HAVE_LCD_COLOR) || defined(HAVE_REMOTE_LCD)
        {
            if (!scale_nearest(bm, src, rset, remote, dither, store_part,
                skip_lines, args))
                return 0;
        }
#endif /* !HAVE_LCD_COLOR || HAVE_REMOTE_LCD*/
    return get_totalsize(bm, remote);
}
