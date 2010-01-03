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

#include <system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <general.h>
#include "inttypes.h"
#ifndef PLUGIN
#include "debug.h"
#endif
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
#include <bmp.h>
#include "resize.h"
#else
#undef DEBUGF
#define DEBUGF(...)
#endif
#include <jpeg_load.h>

#if CONFIG_CPU == SH7034
/* 16*16->32 bit multiplication is a single instrcution on the SH1 */
#define MULUQ(a, b) ((uint32_t) (((uint16_t) (a)) * ((uint16_t) (b))))
#define MULQ(a, b) ((int32_t) (((int16_t) (a)) * ((int16_t) (b))))
#else
#define MULUQ(a, b) ((a) * (b))
#define MULQ(a, b) ((a) * (b))
#endif

/* calculate the maximum dimensions which will preserve the aspect ration of
   src while fitting in the constraints passed in dst, and store result in dst,
   returning 0 if rounding and 1 if not rounding.
*/
int recalc_dimension(struct dim *dst, struct dim *src)
{
    /* This only looks backwards. The input image size is being pre-scaled by
     * the inverse of the pixel aspect ratio, so that once the size it scaled
     * to meet the output constraints, the scaled image will have appropriate
     * proportions.
     */
    int sw = src->width * LCD_PIXEL_ASPECT_HEIGHT;
    int sh = src->height * LCD_PIXEL_ASPECT_WIDTH;
    int tmp;
    if (dst->width <= 0)
        dst->width = LCD_WIDTH;
    if (dst->height <= 0)
        dst->height = LCD_HEIGHT;
#ifndef HAVE_UPSCALER
    if (dst->width > sw || dst->height > sh)
    {
        dst->width = sw;
        dst->height = sh;
    }
    if (sw == dst->width && sh == dst->height)
        return 1;
#endif
    tmp = (sw * dst->height + (sh >> 1)) / sh;
    if (tmp > dst->width)
        dst->height = (sh * dst->width + (sw >> 1)) / sw;
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

#if defined(CPU_COLDFIRE)
#define MAC(op1, op2, num) \
    asm volatile( \
        "mac.l %0, %1, %%acc" #num \
        : \
        : "%d" (op1), "d" (op2)\
    )
#define MAC_OUT(dest, num) \
    asm volatile( \
        "movclr.l %%acc" #num ", %0" \
        : "=d" (dest) \
    )
#elif defined(CPU_SH)
/* calculate the 32-bit product of unsigned 16-bit op1 and op2 */
static inline int32_t mul_s16_s16(int16_t op1, int16_t op2)
{
    return (int32_t)(op1 * op2);
}

/* calculate the 32-bit product of signed 16-bit op1 and op2 */
static inline uint32_t mul_u16_u16(uint16_t op1, uint16_t op2)
{
    return (uint32_t)(op1 * op2);
}
#endif

/* horizontal area average scaler */
static bool scale_h_area(void *out_line_ptr,
                         struct scaler_context *ctx, bool accum)
{
    SDEBUGF("scale_h_area\n");
    unsigned int ix, ox, oxe, mul;
#if defined(CPU_SH) || defined (TEST_SH_MATH)
    const uint32_t h_i_val = ctx->src->width,
                   h_o_val = ctx->bm->width;
#else
    const uint32_t h_i_val = ctx->h_i_val,
                   h_o_val = ctx->h_o_val;
#endif
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
        oxe += h_o_val;
        /* end of current area has been reached */
        /* fill buffer if needed */
        FILL_BUF(part,ctx->store_part,ctx->args);
#ifdef HAVE_LCD_COLOR
        if (oxe >= h_i_val)
        {
            /* "reset" error, which now represents partial coverage of next
               pixel by the next area
            */
            oxe -= h_i_val;

#if defined(CPU_COLDFIRE)
/* Coldfire EMAC math */
            /* add saved partial pixel from start of area */
            MAC(rgbvalacc.r, h_o_val, 0);
            MAC(rgbvalacc.g, h_o_val, 1);
            MAC(rgbvalacc.b, h_o_val, 2);
            MAC(rgbvaltmp.r, mul, 0);
            MAC(rgbvaltmp.g, mul, 1);
            MAC(rgbvaltmp.b, mul, 2);
            /* get new pixel , then add its partial coverage to this area */
            mul = h_o_val - oxe;
            rgbvaltmp.r = part->buf->red;
            rgbvaltmp.g = part->buf->green;
            rgbvaltmp.b = part->buf->blue;
            MAC(rgbvaltmp.r, mul, 0);
            MAC(rgbvaltmp.g, mul, 1);
            MAC(rgbvaltmp.b, mul, 2);
            MAC_OUT(rgbvalacc.r, 0);
            MAC_OUT(rgbvalacc.g, 1);
            MAC_OUT(rgbvalacc.b, 2);
#else
/* generic C math */
            /* add saved partial pixel from start of area */
            rgbvalacc.r = rgbvalacc.r * h_o_val + rgbvaltmp.r * mul;
            rgbvalacc.g = rgbvalacc.g * h_o_val + rgbvaltmp.g * mul;
            rgbvalacc.b = rgbvalacc.b * h_o_val + rgbvaltmp.b * mul;

            /* get new pixel , then add its partial coverage to this area */
            rgbvaltmp.r = part->buf->red;
            rgbvaltmp.g = part->buf->green;
            rgbvaltmp.b = part->buf->blue;
            mul = h_o_val - oxe;
            rgbvalacc.r += rgbvaltmp.r * mul;
            rgbvalacc.g += rgbvaltmp.g * mul;
            rgbvalacc.b += rgbvaltmp.b * mul;
#endif /* CPU */
            rgbvalacc.r = (rgbvalacc.r + (1 << 21)) >> 22;
            rgbvalacc.g = (rgbvalacc.g + (1 << 21)) >> 22;
            rgbvalacc.b = (rgbvalacc.b + (1 << 21)) >> 22;
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
            mul = oxe;
            ox += 1;
        /* inside an area */
        } else {
            /* add pixel value to accumulator */
            rgbvalacc.r += part->buf->red;
            rgbvalacc.g += part->buf->green;
            rgbvalacc.b += part->buf->blue;
        }
#else
        if (oxe >= h_i_val)
        {
            /* "reset" error, which now represents partial coverage of next
               pixel by the next area
            */
            oxe -= h_i_val;
#if defined(CPU_COLDFIRE)
/* Coldfire EMAC math */
            /* add saved partial pixel from start of area */
            MAC(acc, h_o_val, 0);
            MAC(tmp, mul, 0);
            /* get new pixel , then add its partial coverage to this area */
            tmp = *(part->buf);
            mul = h_o_val - oxe;
            MAC(tmp, mul, 0);
            MAC_OUT(acc, 0);
#elif defined(CPU_SH)
/* SH-1 16x16->32 math */
            /* add saved partial pixel from start of area */
            acc = mul_u16_u16(acc, h_o_val) + mul_u16_u16(tmp, mul);

            /* get new pixel , then add its partial coverage to this area */
            tmp = *(part->buf);
            mul = h_o_val - oxe;
            acc += mul_u16_u16(tmp, mul);
#else
/* generic C math */
            /* add saved partial pixel from start of area */
            acc = (acc * h_o_val) + (tmp * mul);

            /* get new pixel , then add its partial coverage to this area */
            tmp = *(part->buf);
            mul = h_o_val - oxe;
            acc += tmp * mul;
#endif /* CPU */
#if !(defined(CPU_SH) || defined(TEST_SH_MATH))
            /* round, divide, and either store or accumulate to output row */
            acc = (acc + (1 << 21)) >> 22;
#endif
            if (accum)
            {
                acc += out_line[ox];
            }
            out_line[ox] = acc;
            /* reset accumulator */
            acc = 0;
            mul = oxe;
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
    uint32_t mul, oy, iy, oye;
#if defined(CPU_SH) || defined (TEST_SH_MATH)
    const uint32_t v_i_val = ctx->src->height,
                   v_o_val = ctx->bm->height;
#else
    const uint32_t v_i_val = ctx->v_i_val,
                   v_o_val = ctx->v_o_val;
#endif

    /* Set up rounding and scale factors */
    mul = 0;
    oy = rset->rowstart;
    oye = 0;
#ifdef HAVE_LCD_COLOR
    uint32_t *rowacc = (uint32_t *) ctx->buf,
             *rowtmp = rowacc + 3 * ctx->bm->width,
             *rowacc_px, *rowtmp_px;
    memset((void *)ctx->buf, 0, ctx->bm->width * 2 * sizeof(struct uint32_rgb));
#else
    uint32_t *rowacc = (uint32_t *) ctx->buf,
             *rowtmp = rowacc + ctx->bm->width,
             *rowacc_px, *rowtmp_px;
    memset((void *)ctx->buf, 0, ctx->bm->width * 2 * sizeof(uint32_t));
#endif
    SDEBUGF("scale_v_area\n");
    /* zero the accumulator and temp rows */
    for (iy = 0; iy < (unsigned int)ctx->src->height; iy++)
    {
        oye += v_o_val;
        /* end of current area has been reached */
        if (oye >= v_i_val)
        {
            /* "reset" error, which now represents partial coverage of the next
               row by the next area
            */
            oye -= v_i_val;
            /* add stored partial row to accumulator */
            for(rowacc_px = rowacc, rowtmp_px = rowtmp; rowacc_px != rowtmp;
                rowacc_px++, rowtmp_px++)
                *rowacc_px = *rowacc_px * v_o_val + *rowtmp_px * mul;
            /* store new scaled row in temp row */
            if(!ctx->h_scaler(rowtmp, ctx, false))
                return false;
            /* add partial coverage by new row to this area, then round and
               scale to final value
            */
            mul = v_o_val - oye;
            for(rowacc_px = rowacc, rowtmp_px = rowtmp; rowacc_px != rowtmp;
                rowacc_px++, rowtmp_px++)
                *rowacc_px += mul * *rowtmp_px;
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
/* horizontal linear scaler */
static bool scale_h_linear(void *out_line_ptr, struct scaler_context *ctx,
                           bool accum)
{
    unsigned int ix, ox, ixe;
#if defined(CPU_SH) || defined (TEST_SH_MATH)
    const uint32_t h_i_val = ctx->src->width - 1,
                   h_o_val = ctx->bm->width - 1;
#else
    const uint32_t h_i_val = ctx->h_i_val,
                   h_o_val = ctx->h_o_val;
#endif
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
    ixe = h_o_val;
    /* give other tasks a chance to run */
    yield();
    for (ox = 0; ox < (uint32_t)ctx->bm->width; ox++)
    {
#ifdef HAVE_LCD_COLOR
        if (ixe >= h_o_val)
        {
            /* Store the new "current" pixel value in rgbval, and the color
               step value in rgbinc.
            */
            ixe -= h_o_val;
            rgbinc.r = -(part->buf->red);
            rgbinc.g = -(part->buf->green);
            rgbinc.b = -(part->buf->blue);
#if defined(CPU_COLDFIRE)
/* Coldfire EMAC math */
            MAC(part->buf->red, h_o_val, 0);
            MAC(part->buf->green, h_o_val, 1);
            MAC(part->buf->blue, h_o_val, 2);
#else
/* generic C math */
            rgbval.r = (part->buf->red) * h_o_val;
            rgbval.g = (part->buf->green) * h_o_val;
            rgbval.b = (part->buf->blue) * h_o_val;
#endif /* CPU */
            ix += 1;
            /* If this wasn't the last pixel, add the next one to rgbinc. */
            if (LIKELY(ix < (uint32_t)ctx->src->width)) {
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
#if defined(CPU_COLDFIRE)
/* Coldfire EMAC math */
                MAC(rgbinc.r, ixe, 0);
                MAC(rgbinc.g, ixe, 1);
                MAC(rgbinc.b, ixe, 2);
#else
/* generic C math */
                rgbval.r += rgbinc.r * ixe;
                rgbval.g += rgbinc.g * ixe;
                rgbval.b += rgbinc.b * ixe;
#endif
            }
#if defined(CPU_COLDFIRE)
/* get final EMAC result out of ACC registers */
            MAC_OUT(rgbval.r, 0);
            MAC_OUT(rgbval.g, 1);
            MAC_OUT(rgbval.b, 2);
#endif
            /* Now multiply the color increment to its proper value */
            rgbinc.r *= h_i_val;
            rgbinc.g *= h_i_val;
            rgbinc.b *= h_i_val;
        } else {
            rgbval.r += rgbinc.r;
            rgbval.g += rgbinc.g;
            rgbval.b += rgbinc.b;
        }
        /* round and scale values, and accumulate or store to output */
        if (accum)
        {
            out_line[ox].r += (rgbval.r + (1 << 21)) >> 22;
            out_line[ox].g += (rgbval.g + (1 << 21)) >> 22;
            out_line[ox].b += (rgbval.b + (1 << 21)) >> 22;
        } else {
            out_line[ox].r = (rgbval.r + (1 << 21)) >> 22;
            out_line[ox].g = (rgbval.g + (1 << 21)) >> 22;
            out_line[ox].b = (rgbval.b + (1 << 21)) >> 22;
        }
#else
        if (ixe >= h_o_val)
        {
            /* Store the new "current" pixel value in rgbval, and the color
               step value in rgbinc.
            */
            ixe -= h_o_val;
            val = *(part->buf);
            inc = -val;
#if defined(CPU_COLDFIRE)
/* Coldfire EMAC math */
            MAC(val, h_o_val, 0);
#elif defined(CPU_SH)
/* SH-1 16x16->32 math */
            val = mul_u16_u16(val, h_o_val);
#else
/* generic C math */
            val = val * h_o_val;
#endif
            ix += 1;
            /* If this wasn't the last pixel, add the next one to rgbinc. */
            if (LIKELY(ix < (uint32_t)ctx->src->width)) {
                part->buf++;
                part->len--;
                /* Fetch new pixels if needed */
                FILL_BUF(part,ctx->store_part,ctx->args);
                inc += *(part->buf);
                /* Add a partial step to rgbval, in this pixel isn't precisely
                   aligned with the new source pixel
                */
#if defined(CPU_COLDFIRE)
/* Coldfire EMAC math */
                MAC(inc, ixe, 0);
#elif defined(CPU_SH)
/* SH-1 16x16->32 math */
                val += mul_s16_s16(inc, ixe);
#else
/* generic C math */
                val += inc * ixe;
#endif
            }
#if defined(CPU_COLDFIRE)
/* get final EMAC result out of ACC register */
            MAC_OUT(val, 0);
#endif
            /* Now multiply the color increment to its proper value */
#if defined(CPU_SH)
/* SH-1 16x16->32 math */
            inc = mul_s16_s16(inc, h_i_val);
#else
/* generic C math */
            inc *= h_i_val;
#endif
        } else
            val += inc;
#if !(defined(CPU_SH) || defined(TEST_SH_MATH))
        /* round and scale values, and accumulate or store to output */
        if (accum)
        {
            out_line[ox] += (val + (1 << 21)) >> 22;
        } else {
            out_line[ox] = (val + (1 << 21)) >> 22;
        }
#else
        /* round and scale values, and accumulate or store to output */
        if (accum)
        {
            out_line[ox] += val;
        } else {
            out_line[ox] = val;
        }
#endif
#endif
        ixe += h_i_val;
    }
    return true;
}

/* vertical linear scaler */
static inline bool scale_v_linear(struct rowset *rset,
                                  struct scaler_context *ctx)
{
    uint32_t mul, iy, iye;
    int32_t oy;
#if defined(CPU_SH) || defined (TEST_SH_MATH)
    const uint32_t v_i_val = ctx->src->height - 1,
                   v_o_val = ctx->bm->height - 1;
#else
    const uint32_t v_i_val = ctx->v_i_val,
                   v_o_val = ctx->v_o_val;
#endif
    /* Set up our buffers, to store the increment and current value for each
       column, and one temp buffer used to read in new rows.
    */
#ifdef HAVE_LCD_COLOR
    uint32_t *rowinc = (uint32_t *)(ctx->buf),
             *rowval = rowinc + 3 * ctx->bm->width,
             *rowtmp = rowval + 3 * ctx->bm->width,
#else
    uint32_t *rowinc = (uint32_t *)(ctx->buf),
             *rowval = rowinc + ctx->bm->width,
             *rowtmp = rowval + ctx->bm->width,
#endif
             *rowinc_px, *rowval_px, *rowtmp_px;

    SDEBUGF("scale_v_linear\n");
    mul = 0;
    iy = 0;
    iye = v_o_val;
    /* get first scaled row in rowtmp */
    if(!ctx->h_scaler((void*)rowtmp, ctx, false))
        return false;
    for (oy = rset->rowstart; oy != rset->rowstop; oy += rset->rowstep)
    {
        if (iye >= v_o_val)
        {
            iye -= v_o_val;
            iy += 1;
            for(rowinc_px = rowinc, rowtmp_px = rowtmp, rowval_px = rowval;
                rowinc_px < rowval; rowinc_px++, rowtmp_px++, rowval_px++)
            {
                *rowinc_px = -*rowtmp_px;
                *rowval_px = *rowtmp_px * v_o_val;
            }
            if (iy < (uint32_t)ctx->src->height)
            {
                if (!ctx->h_scaler((void*)rowtmp, ctx, false))
                    return false;
                for(rowinc_px = rowinc, rowtmp_px = rowtmp, rowval_px = rowval;
                    rowinc_px < rowval; rowinc_px++, rowtmp_px++, rowval_px++)
                {
                    *rowinc_px += *rowtmp_px;
                    *rowval_px += *rowinc_px * iye;
                    *rowinc_px *= v_i_val;
                }
            }
        } else
            for(rowinc_px = rowinc, rowval_px = rowval; rowinc_px < rowval;
                rowinc_px++, rowval_px++)
                *rowval_px += *rowinc_px;
        ctx->output_row(oy, (void*)rowval, ctx);
        iye += v_i_val;
    }
    return true;
}
#endif /* HAVE_UPSCALER */

#if defined(HAVE_LCD_COLOR) && (defined(HAVE_JPEG) || defined(PLUGIN))
static void output_row_32_native_fromyuv(uint32_t row, void * row_in,
                               struct scaler_context *ctx)
{
#if   defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define DEST_STEP   (ctx->bm->height)
#define Y_STEP      (1)
#else
#define DEST_STEP   (1)
#define Y_STEP      (BM_WIDTH(ctx->bm->width,FORMAT_NATIVE,0))
#endif

    int col;
    uint8_t dy = DITHERY(row);
    struct uint32_rgb *qp = (struct uint32_rgb *)row_in;
    SDEBUGF("output_row: y: %lu in: %p\n",row, row_in);
    fb_data *dest = (fb_data *)ctx->bm->data + Y_STEP * row;
    int delta = 127;
    unsigned r, g, b, y, u, v;
    
    for (col = 0; col < ctx->bm->width; col++) {
        if (ctx->dither)
            delta = DITHERXDY(col,dy);
        y = SC_OUT(qp->b, ctx);
        u = SC_OUT(qp->g, ctx);
        v = SC_OUT(qp->r, ctx);
        qp++;
        yuv_to_rgb(y, u, v, &r, &g, &b);
        r = (31 * r + (r >> 3) + delta) >> 8;
        g = (63 * g + (g >> 2) + delta) >> 8;
        b = (31 * b + (b >> 3) + delta) >> 8;
        *dest = LCD_RGBPACK_LCD(r, g, b);
        dest += DEST_STEP;
    }
}
#endif

#if !defined(PLUGIN) || LCD_DEPTH > 1
static void output_row_32_native(uint32_t row, void * row_in,
                              struct scaler_context *ctx)
{
    int col;
    int fb_width = BM_WIDTH(ctx->bm->width,FORMAT_NATIVE,0);
    uint8_t dy = DITHERY(row);
#ifdef HAVE_LCD_COLOR
    struct uint32_rgb *qp = (struct uint32_rgb*)row_in;
#else
    uint32_t *qp = (uint32_t*)row_in;
#endif
    SDEBUGF("output_row: y: %lu in: %p\n",row, row_in);
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
                    bright = SC_OUT(*qp++, ctx);
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
                    bright = SC_OUT(*qp++, ctx);
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
                    bright = SC_OUT(*qp++, ctx);
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= vi_pattern[bright] << shift;
                }
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16

#if   defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
                /* M:Robe 500 */
                (void) fb_width;
                fb_data *dest = (fb_data *)ctx->bm->data + row;
                int delta = 127;
                unsigned r, g, b;
                struct uint32_rgb q0;
                
                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    q0 = *qp++;
                    r = SC_OUT(q0.r, ctx);
                    g = SC_OUT(q0.g, ctx);
                    b = SC_OUT(q0.b, ctx);
                    r = (31 * r + (r >> 3) + delta) >> 8;
                    g = (63 * g + (g >> 2) + delta) >> 8;
                    b = (31 * b + (b >> 3) + delta) >> 8;
                    *dest = LCD_RGBPACK_LCD(r, g, b);
                    dest += ctx->bm->height;
                }
#else
                /* iriver h300, colour iPods, X5 */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width * row;
                int delta = 127;
                unsigned r, g, b;
                struct uint32_rgb q0;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    q0 = *qp++;
                    r = SC_OUT(q0.r, ctx);
                    g = SC_OUT(q0.g, ctx);
                    b = SC_OUT(q0.b, ctx);
                    r = (31 * r + (r >> 3) + delta) >> 8;
                    g = (63 * g + (g >> 2) + delta) >> 8;
                    b = (31 * b + (b >> 3) + delta) >> 8;
                    *dest++ = LCD_RGBPACK_LCD(r, g, b);
                }
#endif

#endif /* LCD_DEPTH */
}
#endif

#if defined(PLUGIN) && LCD_DEPTH > 1
unsigned int get_size_native(struct bitmap *bm)
{
    return BM_SIZE(bm->width,bm->height,FORMAT_NATIVE,0);
}

const struct custom_format format_native = {
    .output_row_8 = output_row_8_native,
#if defined(HAVE_LCD_COLOR) && (defined(HAVE_JPEG) || defined(PLUGIN))
    .output_row_32 = {
        output_row_32_native,
        output_row_32_native_fromyuv
    },
#else
    .output_row_32 = output_row_32_native,
#endif
    .get_size = get_size_native
};
#endif

int resize_on_load(struct bitmap *bm, bool dither, struct dim *src,
                   struct rowset *rset, unsigned char *buf, unsigned int len,
                   const struct custom_format *format,
                   IF_PIX_FMT(int format_index,)
                   struct img_part* (*store_part)(void *args),
                   void *args)
{
    const int sw = src->width;
    const int sh = src->height;
    const int dw = bm->width;
    const int dh = bm->height;
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
    ALIGN_BUFFER(buf, len, sizeof(uint32_t));
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
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(true);
#endif
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
#if defined(CPU_SH) || defined (TEST_SH_MATH)
    uint32_t div;
#endif
#if !defined(PLUGIN)
#if defined(HAVE_LCD_COLOR) && defined(HAVE_JPEG)
    ctx.output_row = format_index ? output_row_32_native_fromyuv
                                  : output_row_32_native;
#else
    ctx.output_row = output_row_32_native;
#endif
    if (format)
#endif
#ifdef HAVE_LCD_COLOR
        ctx.output_row = format->output_row_32[format_index];
#else
        ctx.output_row = format->output_row_32;
#endif
#ifdef HAVE_UPSCALER
    if (sw > dw)
    {
#endif
        ctx.h_scaler = scale_h_area;
#if defined(CPU_SH) || defined (TEST_SH_MATH)
        div = sw;
#else
        uint32_t h_div = (1U << 24) / sw;
        ctx.h_i_val = sw * h_div;
        ctx.h_o_val = dw * h_div;
#endif
#ifdef HAVE_UPSCALER
    } else {
        ctx.h_scaler = scale_h_linear;
#if defined(CPU_SH) || defined (TEST_SH_MATH)
        div = dw - 1;
#else
        uint32_t h_div = (1U << 24) / (dw - 1);
        ctx.h_i_val = (sw - 1) * h_div;
        ctx.h_o_val = (dw - 1) * h_div;
#endif
    }
#endif
#ifdef CPU_COLDFIRE
    unsigned old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_UNSIGNED);
#endif
#ifdef HAVE_UPSCALER
    if (sh > dh)
#endif
    {
#if defined(CPU_SH) || defined (TEST_SH_MATH)
        div *= sh;
        ctx.recip = ((uint32_t)(-div)) / div + 1;
#else
        uint32_t v_div = (1U << 22) / sh;
        ctx.v_i_val = sh * v_div;
        ctx.v_o_val = dh * v_div;
#endif
        ret = scale_v_area(rset, &ctx);
    }
#ifdef HAVE_UPSCALER
    else
    {
#if defined(CPU_SH) || defined (TEST_SH_MATH)
        div *= dh - 1;
        ctx.recip = ((uint32_t)(-div)) / div + 1;
#else
        uint32_t v_div = (1U << 22) / dh;
        ctx.v_i_val = (sh - 1) * v_div;
        ctx.v_o_val = (dh - 1) * v_div;
#endif
        ret = scale_v_linear(rset, &ctx);
    }
#endif
#ifdef CPU_COLDFIRE
    /* Restore emac status; other modules like tone control filter
     * calculation may rely on it. */
    coldfire_set_macsr(old_macsr);
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(false);
#endif
    if (!ret)
        return 0;
    return 1;
}
