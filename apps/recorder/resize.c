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
#include <general.h>
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

/* calculate the maximum dimensions which will preserve the aspect ration of
   src while fitting in the constraints passed in dst, and store result in dst,
   returning 0 if rounding and 1 if not rounding.
*/
int recalc_dimension(struct dim *dst, struct dim *src)
{
    int tmp;
    if (dst->width <= 0)
        dst->width = LCD_WIDTH;
    if (dst->height <= 0)
        dst->height = LCD_HEIGHT;
#ifndef HAVE_UPSCALER
    if (dst->width > src->width || dst->height > src->height)
    {
        dst->width = src->width;
        dst->height = src->height;
    }
    if (src->width == dst->width && src->height == dst->height)
        return 1;
#endif
    tmp = (src->width * dst->height + (src->height >> 1)) / src->height;
    if (tmp > dst->width)
        dst->height = (src->height * dst->width + (src->width >> 1))
                      / src->width;
    else
        dst->width = tmp;
    return src->width == dst->width && src->height == dst->height;
}

/* All of these scalers use variations of Bresenham's algorithm to convert from
   their input to output coordinates.  The error value is shifted from the
   "classic" version such that it is a useful input to the scaling calculation.
*/

#ifdef HAVE_LCD_COLOR
/* dither + pack on channel of RGB565, R an B share a packing macro */
#define PACKRB(v, delta)  ((31 * v + (v >> 3) + delta) >> 8)
#define PACKG(g, delta) ((63 * g + (g >> 2) + delta) >> 8)
#endif

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

/* struct which containers various parameters shared between vertical scaler,
   horizontal scaler, and row output
*/
struct scaler_context {
    uint32_t divisor;
    uint32_t round;
    struct bitmap *bm;
    struct dim *src;
    unsigned char *buf;
    bool dither;
    int len;
    void *args;
    struct img_part* (*store_part)(void *);
    void (*output_row)(uint32_t,void*,struct scaler_context*);
    bool (*h_scaler)(void*,struct scaler_context*, bool);
};

/* Set up rounding and scale factors for horizontal area scaler */
static inline void scale_h_area_setup(struct scaler_context *ctx)
{
/* sum is output value * src->width */
    SDEBUGF("scale_h_area_setup\n");
    ctx->divisor = ctx->src->width;
}

/* horizontal area average scaler */
static bool scale_h_area(void *out_line_ptr,
                         struct scaler_context *ctx, bool accum)
{
    SDEBUGF("scale_h_area\n");
    unsigned int ix, ox, oxe, mul;
#ifdef HAVE_LCD_COLOR
    struct uint32_rgb rgbvalacc = { 0, 0, 0 },
                      rgbvaltmp = { 0, 0, 0 },
                      *out_line = (struct uint32_rgb *)out_line_ptr;
#else
    uint32_t acc = 0, tmp = 0, *out_line = (uint32_t*)out_line_ptr;
#endif
    struct img_part *part;
    FILL_BUF_INIT(part,ctx->store_part,ctx->args);
    ox = 0;
    oxe = 0;
    mul = 0;
    /* give other tasks a chance to run */
    yield();
    for (ix = 0; ix < (unsigned int)ctx->src->width; ix++)
    {
        oxe += ctx->bm->width;
        /* end of current area has been reached */
        /* fill buffer if needed */
        FILL_BUF(part,ctx->store_part,ctx->args);
#ifdef HAVE_LCD_COLOR
        if (oxe >= (unsigned int)ctx->src->width)
        {
            /* "reset" error, which now represents partial coverage of next
               pixel by the next area
            */
            oxe -= ctx->src->width;

            /* add saved partial pixel from start of area */
            rgbvalacc.r = rgbvalacc.r * ctx->bm->width + rgbvaltmp.r * mul;
            rgbvalacc.g = rgbvalacc.g * ctx->bm->width + rgbvaltmp.g * mul;
            rgbvalacc.b = rgbvalacc.b * ctx->bm->width + rgbvaltmp.b * mul;

            /* get new pixel , then add its partial coverage to this area */
            rgbvaltmp.r = part->buf->red;
            rgbvaltmp.g = part->buf->green;
            rgbvaltmp.b = part->buf->blue;
            mul = ctx->bm->width - oxe;
            rgbvalacc.r += rgbvaltmp.r * mul;
            rgbvalacc.g += rgbvaltmp.g * mul;
            rgbvalacc.b += rgbvaltmp.b * mul;
            /* store or accumulate to output row */
            if (accum)
            {
                rgbvalacc.r += out_line[ox].r;
                rgbvalacc.g += out_line[ox].g;
                rgbvalacc.b += out_line[ox].b;
            }
            out_line[ox].r = rgbvalacc.r;
            out_line[ox].g = rgbvalacc.g;
            out_line[ox].b = rgbvalacc.b;
            /* reset accumulator */
            rgbvalacc.r = 0;
            rgbvalacc.g = 0;
            rgbvalacc.b = 0;
            mul = ctx->bm->width - mul;
            ox += 1;
        /* inside an area */
        } else {
            /* add pixel value to accumulator */
            rgbvalacc.r += part->buf->red;
            rgbvalacc.g += part->buf->green;
            rgbvalacc.b += part->buf->blue;
        }
#else
        if (oxe >= (unsigned int)ctx->src->width)
        {
            /* "reset" error, which now represents partial coverage of next
               pixel by the next area
            */
            oxe -= ctx->src->width;

            /* add saved partial pixel from start of area */
            acc = acc * ctx->bm->width + tmp * mul;

            /* get new pixel , then add its partial coverage to this area */
            tmp = *(part->buf);
            mul = ctx->bm->width - oxe;
            acc += tmp * mul;
            /* round, divide, and either store or accumulate to output row */
            if (accum)
            {
                acc += out_line[ox];
            }
            out_line[ox] = acc;
            /* reset accumulator */
            acc = 0;
            mul = ctx->bm->width - mul;
            ox += 1;
        /* inside an area */
        } else {
            /* add pixel value to accumulator */
            acc += *(part->buf);
        }
#endif
        part->buf++;
        part->len--;
    }
    return true;
}

/* vertical area average scaler */
static inline bool scale_v_area(struct rowset *rset, struct scaler_context *ctx)
{
    uint32_t mul, x, oy, iy, oye;

    /* Set up rounding and scale factors */
    ctx->divisor *= ctx->src->height;
    ctx->round = ctx->divisor >> 1;
    ctx->divisor = ((ctx->divisor - 1 + 0x80000000U) / ctx->divisor) << 1;
    mul = 0;
    oy = rset->rowstart;
    oye = 0;
#ifdef HAVE_LCD_COLOR
    uint32_t *rowacc = (uint32_t *) ctx->buf,
             *rowtmp = rowacc + 3 * ctx->bm->width;
    memset((void *)ctx->buf, 0, ctx->bm->width * 2 * sizeof(struct uint32_rgb));
#else
    uint32_t *rowacc = (uint32_t *) ctx->buf,
             *rowtmp = rowacc + ctx->bm->width;
    memset((void *)ctx->buf, 0, ctx->bm->width * 2 * sizeof(uint32_t));
#endif
    SDEBUGF("scale_v_area\n");
    /* zero the accumulator and temp rows */
    for (iy = 0; iy < (unsigned int)ctx->src->height; iy++)
    {
        oye += ctx->bm->height;
        /* end of current area has been reached */
        if (oye >= (unsigned int)ctx->src->height)
        {
            /* "reset" error, which now represents partial coverage of the next
               row by the next area
            */
            oye -= ctx->src->height;
            /* add stored partial row to accumulator */
#ifdef HAVE_LCD_COLOR
            for (x = 0; x < 3 * (unsigned int)ctx->bm->width; x++)
#else
            for (x = 0; x < (unsigned int)ctx->bm->width; x++)
#endif
                rowacc[x] = rowacc[x] * ctx->bm->height + mul * rowtmp[x];
            /* store new scaled row in temp row */
            if(!ctx->h_scaler(rowtmp, ctx, false))
                return false;
            /* add partial coverage by new row to this area, then round and
               scale to final value
            */
            mul = ctx->bm->height - oye;
#ifdef HAVE_LCD_COLOR
            for (x = 0; x < 3 * (unsigned int)ctx->bm->width; x++)
#else
            for (x = 0; x < (unsigned int)ctx->bm->width; x++)
#endif
                rowacc[x] += mul * rowtmp[x];
            ctx->output_row(oy, (void*)rowacc, ctx);
            /* clear accumulator row, store partial coverage for next row */
#ifdef HAVE_LCD_COLOR
            memset((void *)rowacc, 0, ctx->bm->width * sizeof(uint32_t) * 3);
#else
            memset((void *)rowacc, 0, ctx->bm->width * sizeof(uint32_t));
#endif
            mul = oye;
            oy += rset->rowstep;
        /* inside an area */
        } else {
            /* accumulate new scaled row to rowacc */
            if (!ctx->h_scaler(rowacc, ctx, true))
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
static inline void scale_h_linear_setup(struct scaler_context *ctx)
{
    ctx->divisor = ctx->bm->width - 1;
}

/* horizontal linear scaler */
static bool scale_h_linear(void *out_line_ptr, struct scaler_context *ctx,
                           bool accum)
{
    unsigned int ix, ox, ixe;
    /* type x = x is an ugly hack for hiding an unitialized data warning. The
       values are conditionally initialized before use, but other values are
       set such that this will occur before these are used.
    */
#ifdef HAVE_LCD_COLOR
    struct uint32_rgb rgbval=rgbval, rgbinc=rgbinc,
                      *out_line = (struct uint32_rgb*)out_line_ptr;
#else
    uint32_t val=val, inc=inc, *out_line = (uint32_t*)out_line_ptr;
#endif
    struct img_part *part;
    SDEBUGF("scale_h_linear\n");
    FILL_BUF_INIT(part,ctx->store_part,ctx->args);
    ix = 0;
    /* The error is set so that values are initialized on the first pass. */
    ixe = ctx->bm->width - 1;
    /* give other tasks a chance to run */
    yield();
    for (ox = 0; ox < (uint32_t)ctx->bm->width; ox++)
    {
#ifdef HAVE_LCD_COLOR
        if (ixe >= ((uint32_t)ctx->bm->width - 1))
        {
            /* Store the new "current" pixel value in rgbval, and the color
               step value in rgbinc.
            */
            ixe -= (ctx->bm->width - 1);
            rgbinc.r = -(part->buf->red);
            rgbinc.g = -(part->buf->green);
            rgbinc.b = -(part->buf->blue);
            rgbval.r = (part->buf->red) * (ctx->bm->width - 1);
            rgbval.g = (part->buf->green) * (ctx->bm->width - 1);
            rgbval.b = (part->buf->blue) * (ctx->bm->width - 1);
            ix += 1;
            /* If this wasn't the last pixel, add the next one to rgbinc. */
            if (ix < (uint32_t)ctx->src->width) {
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
            rgbinc.r *= ctx->src->width - 1;
            rgbinc.g *= ctx->src->width - 1;
            rgbinc.b *= ctx->src->width - 1;
        } else {
            rgbval.r += rgbinc.r;
            rgbval.g += rgbinc.g;
            rgbval.b += rgbinc.b;
        }
        /* round and scale values, and accumulate or store to output */
        if (accum)
        {
            out_line[ox].r += rgbval.r;
            out_line[ox].g += rgbval.g;
            out_line[ox].b += rgbval.b;
        } else {
            out_line[ox].r = rgbval.r;
            out_line[ox].g = rgbval.g;
            out_line[ox].b = rgbval.b;
        }
#else
        if (ixe >= ((uint32_t)ctx->bm->width - 1))
        {
            /* Store the new "current" pixel value in rgbval, and the color
               step value in rgbinc.
            */
            ixe -= (ctx->bm->width - 1);
            val = *(part->buf);
            inc = -val;
            val *= (ctx->bm->width - 1);
            ix += 1;
            /* If this wasn't the last pixel, add the next one to rgbinc. */
            if (ix < (uint32_t)ctx->src->width) {
                part->buf++;
                part->len--;
                /* Fetch new pixels if needed */
                FILL_BUF(part,ctx->store_part,ctx->args);
                inc += *(part->buf);
                /* Add a partial step to rgbval, in this pixel isn't precisely
                   aligned with the new source pixel
                */
                val += inc * ixe;
            }
            /* Now multiply the color increment to its proper value */
            inc *= ctx->src->width - 1;
        } else
            val += inc;
        /* round and scale values, and accumulate or store to output */
        if (accum)
        {
            out_line[ox] += val;
        } else {
            out_line[ox] = val;
        }
#endif
        ixe += ctx->src->width - 1;
    }
    return true;
}

/* vertical linear scaler */
static inline bool scale_v_linear(struct rowset *rset,
                                  struct scaler_context *ctx)
{
    uint32_t mul, x, iy, iye;
    int32_t oy;
    /* Set up scale and rounding factors, the divisor is bm->height - 1 */
    ctx->divisor *= (ctx->bm->height - 1);
    ctx->round = ctx->divisor >> 1;
    ctx->divisor = ((ctx->divisor - 1 + 0x80000000U) / ctx->divisor) << 1;
    /* Set up our two temp buffers. The names are generic because they'll be
       swapped each time a new input row is read
    */
#ifdef HAVE_LCD_COLOR
    uint32_t *rowinc = (uint32_t *)(ctx->buf),
             *rowval = rowinc + 3 * ctx->bm->width,
             *rowtmp = rowval + 3 * ctx->bm->width;
#else
    uint32_t *rowinc = (uint32_t *)(ctx->buf),
             *rowval = rowinc + ctx->bm->width,
             *rowtmp = rowval + ctx->bm->width;
#endif

    SDEBUGF("scale_v_linear\n");
    mul = 0;
    iy = 0;
    iye = ctx->bm->height - 1;
    /* get first scaled row in rowtmp */
    if(!ctx->h_scaler((void*)rowtmp, ctx, false))
        return false;
    for (oy = rset->rowstart; oy != rset->rowstop; oy += rset->rowstep)
    {
        if (iye >= (uint32_t)ctx->bm->height - 1)
        {
            iye -= ctx->bm->height - 1;
            iy += 1;
#ifdef HAVE_LCD_COLOR
            for (x = 0; x < 3 * (uint32_t)ctx->bm->width; x++)
#else
            for (x = 0; x < (uint32_t)ctx->bm->width; x++)
#endif
            {
                rowinc[x] = -rowtmp[x];
                rowval[x] = rowtmp[x] * (ctx->bm->height - 1);
            }
            if (iy < (uint32_t)ctx->src->height)
            {
                if (!ctx->h_scaler((void*)rowtmp, ctx, false))
                    return false;
#ifdef HAVE_LCD_COLOR
                for (x = 0; x < 3 * (uint32_t)ctx->bm->width; x++)
#else
                for (x = 0; x < (uint32_t)ctx->bm->width; x++)
#endif
                {
                    rowinc[x] += rowtmp[x];
                    rowval[x] += rowinc[x] * iye;
                    rowinc[x] *= ctx->src->height - 1;
                }
            }
        } else
#ifdef HAVE_LCD_COLOR
            for (x = 0; x < 3 * (uint32_t)ctx->bm->width; x++)
#else
            for (x = 0; x < (uint32_t)ctx->bm->width; x++)
#endif
                    rowval[x] += rowinc[x];
        ctx->output_row(oy, (void*)rowval, ctx);
        iye += ctx->src->height - 1;
    }
    return true;
}
#endif /* HAVE_UPSCALER */

void output_row_native(uint32_t row, void * row_in, struct scaler_context *ctx)
{
    int col;
    int fb_width = BM_WIDTH(ctx->bm->width,FORMAT_NATIVE,0);
    uint8_t dy = DITHERY(row);
#ifdef HAVE_LCD_COLOR
    struct uint32_rgb *qp = (struct uint32_rgb*)row_in;
#else
    uint32_t *qp = (uint32_t*)row_in;
#endif
    SDEBUGF("output_row: y: %d in: %p\n",row, row_in);
#if LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
                /* greyscale iPods */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width * row;
                int shift = 6;
                int delta = 127;
                unsigned bright;
                unsigned data = 0;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    bright = ((*qp++) + ctx->round) *
                             (uint64_t)ctx->divisor >> 32;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    data |= (~bright & 3) << shift;
                    shift -= 2;
                    if (shift < 0) {
                        *dest++ = data;
                        data = 0;
                        shift = 6;
                    }
                }
                if (shift < 6)
                    *dest++ = data;
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
                /* iriver H1x0 */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width *
                                (row >> 2);
                int shift = 2 * (row & 3);
                int delta = 127;
                unsigned bright;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    bright = ((*qp++) + ctx->round) *
                             (uint64_t)ctx->divisor >> 32;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= (~bright & 3) << shift;
                }
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
                /* iAudio M3 */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width *
                                (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    bright = ((*qp++) + ctx->round) *
                             (uint64_t)ctx->divisor >> 32;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= vi_pattern[bright] << shift;
                }
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
                /* iriver h300, colour iPods, X5 */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width * row;
                int delta = 127;
                unsigned r, g, b;
                struct uint32_rgb q0;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    q0 = *qp++;
                    r = (q0.r + ctx->round) * (uint64_t)ctx->divisor >> 32;
                    g = (q0.g + ctx->round) * (uint64_t)ctx->divisor >> 32;
                    b = (q0.b + ctx->round) * (uint64_t)ctx->divisor >> 32;
                    r = (31 * r + (r >> 3) + delta) >> 8;
                    g = (63 * g + (g >> 2) + delta) >> 8;
                    b = (31 * b + (b >> 3) + delta) >> 8;
                    *dest++ = LCD_RGBPACK_LCD(r, g, b);
                }
#endif /* LCD_DEPTH */
}

int resize_on_load(struct bitmap *bm, bool dither, struct dim *src,
                   struct rowset *rset, unsigned char *buf, unsigned int len,
                   struct img_part* (*store_part)(void *args),
                   void *args)
{

#ifdef HAVE_UPSCALER
    const int sw = src->width;
    const int sh = src->height;
    const int dw = bm->width;
    const int dh = bm->height;
#endif
    int ret;
#ifdef HAVE_LCD_COLOR
    unsigned int needed = sizeof(struct uint32_rgb) * 3 * bm->width;
#else
    unsigned int needed = sizeof(uint32_t) * 3 * bm->width;
#endif
#if MAX_SC_STACK_ALLOC
    uint8_t sc_buf[(needed <= len  || needed > MAX_SC_STACK_ALLOC) ?
                   0 : needed];
#endif
    len = (unsigned int)align_buffer(PUN_PTR(void**, &buf), len,
                                         sizeof(uint32_t));
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

    struct scaler_context ctx;
    cpu_boost(true);
    ctx.store_part = store_part;
    ctx.args = args;
#if MAX_SC_STACK_ALLOC
    ctx.buf = needed > len ? sc_buf : buf;
#else
    ctx.buf = buf;
#endif
    ctx.len = len;
    ctx.bm = bm;
    ctx.src = src;
    ctx.dither = dither;
    ctx.output_row = output_row_native;
#ifdef HAVE_UPSCALER
    if (sw > dw)
    {
#endif
        ctx.h_scaler = scale_h_area;
        scale_h_area_setup(&ctx);
#ifdef HAVE_UPSCALER
    } else {
        ctx.h_scaler = scale_h_linear;
        scale_h_linear_setup(&ctx);
    }
#endif
#ifdef HAVE_UPSCALER
    if (sh > dh)
#endif
        ret = scale_v_area(rset, &ctx);
#ifdef HAVE_UPSCALER
    else
        ret = scale_v_linear(rset, &ctx);
#endif
    cpu_boost(false);
    if (!ret)
        return 0;
    return BM_SIZE(bm->width,bm->height,bm->format,0);
}
