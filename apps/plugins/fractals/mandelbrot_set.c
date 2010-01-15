/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: mandelbrot.c 24010 2009-12-15 20:51:41Z tomers $
 *
 * Copyright (C) 2004 Matthias Wientapper
 * Heavily extended 2005 Jens Arnold
 *
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
#include "mandelbrot_set.h"

#define BUTTON_YIELD_TIMEOUT (HZ / 4)

#ifdef USEGSLIB
unsigned char imgbuffer[LCD_HEIGHT];
#else
fb_data imgbuffer[LCD_HEIGHT];
#endif

/* 8 entries cyclical, last entry is black (convergence) */
#ifdef HAVE_LCD_COLOR
static const fb_data color[9] = {
    LCD_RGBPACK(255, 0, 159), LCD_RGBPACK(159, 0, 255), LCD_RGBPACK(0, 0, 255),
    LCD_RGBPACK(0, 159, 255), LCD_RGBPACK(0, 255, 128), LCD_RGBPACK(128, 255, 0),
    LCD_RGBPACK(255, 191, 0), LCD_RGBPACK(255, 0, 0),   LCD_RGBPACK(0, 0, 0)
};
#else /* greyscale */
static const unsigned char color[9] = {
    255, 223, 191, 159, 128, 96, 64, 32, 0
};
#endif

#if CONFIG_LCD == LCD_SSD1815
/* Recorder, Ondio: pixel_height == 1.25 * pixel_width */
#define MB_HEIGHT (LCD_HEIGHT*5/4)
#else
/* square pixels */
#define MB_HEIGHT LCD_HEIGHT
#endif

#define MB_XOFS (-0x03000000L)       /* -0.75 (s5.26) */
#if 3000*MB_HEIGHT/LCD_WIDTH >= 2400 /* width is limiting factor */
#define MB_XFAC (0x06000000LL)       /* 1.5 (s5.26) */
#define MB_YFAC (MB_XFAC*MB_HEIGHT/LCD_WIDTH)
#else                                /* height is limiting factor */
#define MB_YFAC (0x04cccccdLL)       /* 1.2 (s5.26) */
#define MB_XFAC (MB_YFAC*LCD_WIDTH/MB_HEIGHT)
#endif

#if (LCD_DEPTH < 8)
#define USEGSLIB
#else
#define UPDATE_FREQ (HZ/50)
#endif

/* Fixed point format s5.26: sign, 5 bits integer part, 26 bits fractional part */
struct mandelbrot_ctx
{
    struct fractal_ops *ops;
    long x_min;
    long x_max;
    long x_step;
    long x_delta;
    long y_min;
    long y_max;
    long y_step;
    long y_delta;
    int step_log2;
    unsigned max_iter;
} ctx;

static void mandelbrot_init(void);

static int mandelbrot_calc_low_prec(struct fractal_rect *rect,
        int (*button_yield_cb)(void *), void *button_yield_ctx);

static int mandelbrot_calc_high_prec(struct fractal_rect *rect,
        int (*button_yield_cb)(void *), void *button_yield_ctx);

static void mandelbrot_move(int dx, int dy);

static void mandelbrot_zoom(int factor);

static int mandelbrot_precision(int d);

struct fractal_ops mandelbrot_ops =
{
    .init = mandelbrot_init,
    .calc = NULL,
    .move = mandelbrot_move,
    .zoom = mandelbrot_zoom,
    .precision = mandelbrot_precision,
};

static int ilog2_fp(long value) /* calculate integer log2(value_fp_6.26) */
{
    int i = 0;

    if (value <= 0)
    {
        return -32767;
    }
    else if (value > (1L << 26))
    {
        while (value >= (2L << 26))
        {
            value >>= 1;
            i++;
        }
    }
    else
    {
        while (value < (1L<<26))
        {
            value <<= 1;
            i--;
        }
    }
    return i;
}

static void recalc_parameters(void)
{
    ctx.x_step = (ctx.x_max - ctx.x_min) / LCD_WIDTH;
    ctx.x_delta = (ctx.x_step * LCD_WIDTH) / 8;
    ctx.y_step = (ctx.y_max - ctx.y_min) / LCD_HEIGHT;
    ctx.y_delta = (ctx.y_step * LCD_HEIGHT) / 8;
    ctx.step_log2 = ilog2_fp(MIN(ctx.x_step, ctx.y_step));
    ctx.max_iter = MAX(15, -15 * ctx.step_log2 - 45);

    ctx.ops->calc = (ctx.step_log2 <= -10) ?
        mandelbrot_calc_high_prec : mandelbrot_calc_low_prec;
}

static void mandelbrot_init(void)
{
    ctx.ops = &mandelbrot_ops;

    ctx.x_min = MB_XOFS - MB_XFAC;
    ctx.x_max = MB_XOFS + MB_XFAC;
    ctx.y_min = -MB_YFAC;
    ctx.y_max = MB_YFAC;

    recalc_parameters();
}

static int mandelbrot_calc_low_prec(struct fractal_rect *rect,
        int (*button_yield_cb)(void *), void *button_yield_ctx)
{
#ifndef USEGSLIB
    long next_update = *rb->current_tick;
    int last_px = rect->px_min;
#endif
    unsigned n_iter;
    long a32, b32;
    short x, x2, y, y2, a, b;
    int p_x, p_y;
    unsigned long last_yield = *rb->current_tick;
    unsigned long last_button_yield = *rb->current_tick;

    a32 = ctx.x_min + ctx.x_step * rect->px_min;

    for (p_x = rect->px_min; p_x < rect->px_max; p_x++)
    {
        a = a32 >> 16;

        b32 = ctx.y_min + ctx.y_step * (LCD_HEIGHT - rect->py_max);

        for (p_y = rect->py_max - 1; p_y >= rect->py_min; p_y--)
        {
            b = b32 >> 16;
            x = a;
            y = b;
            n_iter = 0;

            while (++n_iter <= ctx.max_iter)
            {
                x2 = MULS16_ASR10(x, x);
                y2 = MULS16_ASR10(y, y);

                if (x2 + y2 > (4<<10)) break;

                y = 2 * MULS16_ASR10(x, y) + b;
                x = x2 - y2 + a;
            }

            if  (n_iter > ctx.max_iter)
                imgbuffer[p_y] = color[8];
            else
                imgbuffer[p_y] = color[n_iter & 7];

            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (TIME_AFTER(*rb->current_tick, last_yield))
            {
                rb->yield();
                last_yield = *rb->current_tick;
            }

            if (TIME_AFTER(*rb->current_tick, last_button_yield))
            {
                if (button_yield_cb(button_yield_ctx))
                {
#ifndef USEGSLIB
                    /* update screen part that was changed since last yield */
                    rb->lcd_update_rect(last_px, rect->py_min,
                            p_x - last_px + 1, rect->py_max - rect->py_min);
#endif
                    rect->px_min = (p_x == 0) ? 0 : p_x - 1;

                    return 1;
                }

                last_button_yield = *rb->current_tick + BUTTON_YIELD_TIMEOUT;
            }

            b32 += ctx.y_step;
        }
#ifdef USEGSLIB
        grey_ub_gray_bitmap_part(imgbuffer, 0, rect->py_min, 1,
                p_x, rect->py_min, 1, rect->py_max - rect->py_min);
#else
        rb->lcd_bitmap_part(imgbuffer, 0, rect->py_min, 1,
                p_x, rect->py_min, 1, rect->py_max - rect->py_min);

        if ((p_x == rect->px_max - 1) ||
                TIME_AFTER(*rb->current_tick, next_update))
        {
            next_update = *rb->current_tick + UPDATE_FREQ;

            /* update screen part that was changed since last yield */
            rb->lcd_update_rect(last_px, rect->py_min,
                    p_x - last_px + 1, rect->py_max - rect->py_min);
            last_px = p_x;
        }
#endif

        a32 += ctx.x_step;
    }

    rect->valid = 0;

    return 0;
}

static int mandelbrot_calc_high_prec(struct fractal_rect *rect,
        int (*button_yield_cb)(void *), void *button_yield_ctx)
{
#ifndef USEGSLIB
    long next_update = *rb->current_tick;
    int last_px = rect->px_min;
#endif
    unsigned n_iter;
    long x, x2, y, y2, a, b;
    int p_x, p_y;
    unsigned long last_yield = *rb->current_tick;
    unsigned long last_button_yield = *rb->current_tick;

    MULS32_INIT();

    a = ctx.x_min + ctx.x_step * rect->px_min;

    for (p_x = rect->px_min; p_x < rect->px_max; p_x++)
    {
        b = ctx.y_min + ctx.y_step * (LCD_HEIGHT - rect->py_max);

        for (p_y = rect->py_max - 1; p_y >= rect->py_min; p_y--)
        {
            x = a;
            y = b;
            n_iter = 0;

            while (++n_iter <= ctx.max_iter)
            {
                x2 = MULS32_ASR26(x, x);
                y2 = MULS32_ASR26(y, y);

                if (x2 + y2 > (4L<<26)) break;

                y = 2 * MULS32_ASR26(x, y) + b;
                x = x2 - y2 + a;
            }

            if  (n_iter > ctx.max_iter)
                imgbuffer[p_y] = color[8];
            else
                imgbuffer[p_y] = color[n_iter & 7];

            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (TIME_AFTER(*rb->current_tick, last_yield))
            {
                rb->yield();
                last_yield = *rb->current_tick;
            }

            if (TIME_AFTER(*rb->current_tick, last_button_yield))
            {
                if (button_yield_cb(button_yield_ctx))
                {
#ifndef USEGSLIB
                    /* update screen part that was changed since last yield */
                    rb->lcd_update_rect(last_px, rect->py_min,
                            p_x - last_px + 1, rect->py_max - rect->py_min);
#endif
                    rect->px_min = (p_x == 0) ? 0 : p_x - 1;

                    return 1;
                }

                last_button_yield = *rb->current_tick + BUTTON_YIELD_TIMEOUT;
            }

            b += ctx.y_step;
        }
#ifdef USEGSLIB
        grey_ub_gray_bitmap_part(imgbuffer, 0, rect->py_min, 1,
                p_x, rect->py_min, 1, rect->py_max - rect->py_min);
#else
        rb->lcd_bitmap_part(imgbuffer, 0, rect->py_min, 1,
                p_x, rect->py_min, 1, rect->py_max - rect->py_min);

        if ((p_x == rect->px_max - 1) ||
                TIME_AFTER(*rb->current_tick, next_update))
        {
            next_update = *rb->current_tick + UPDATE_FREQ;

            /* update screen part that was changed since last yield */
            rb->lcd_update_rect(last_px, rect->py_min,
                    p_x - last_px + 1, rect->py_max - rect->py_min);
            last_px = p_x;
        }
#endif
        a += ctx.x_step;
    }

    rect->valid = 0;

    return 0;
}

static void mandelbrot_move(int dx, int dy)
{
    long d_x = (long)dx * ctx.x_delta;
    long d_y = (long)dy * ctx.y_delta;

    ctx.x_min += d_x;
    ctx.x_max += d_x;
    ctx.y_min += d_y;
    ctx.y_max += d_y;
}

static void mandelbrot_zoom(int factor)
{
    long factor_x = (long)factor * ctx.x_delta;
    long factor_y = (long)factor * ctx.y_delta;

    ctx.x_min += factor_x;
    ctx.x_max -= factor_x;
    ctx.y_min += factor_y;
    ctx.y_max -= factor_y;

    recalc_parameters();
}

static int mandelbrot_precision(int d)
{
    int changed = 0;

    /* Precision increase */
    for (; d > 0; d--)
    {
        ctx.max_iter += ctx.max_iter / 2;
        changed = 1;
    }

    /* Precision decrease */
    for (; d < 0 && ctx.max_iter >= 15; d++)
    {
        ctx.max_iter -= ctx.max_iter / 3;
        changed = 1;
    }

    return changed;
}

