/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "fractal_sets.h"
#include "mandelbrot_set.h"

#define BUTTON_YIELD_TIMEOUT (HZ / 4)

#ifdef USEGSLIB
static unsigned char imgbuffer[LCD_HEIGHT];
#else
static fb_data imgbuffer[LCD_HEIGHT];
#endif

#define NUM_COLORS ((unsigned)(1 << LCD_DEPTH))

/*
 * Spread iter's colors over color range.
 * 345 (=15*26-45) is max_iter maximal value
 * This implementation ignores pixel format, thus it is not uniformly spread
 */
#define LCOLOR(iter) ((iter * NUM_COLORS) / 345)

#ifdef HAVE_LCD_COLOR
#define COLOR(iter) (fb_data)LCOLOR(iter)
#define CONVERGENCE_COLOR LCD_RGBPACK(0, 0, 0)
#else /* greyscale */
#define COLOR(iter) (unsigned char)LCOLOR(iter)
#define CONVERGENCE_COLOR 0
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

#ifndef USEGSLIB
#define UPDATE_FREQ (HZ/50)
#endif

/* Fixed point format s5.26: sign, 5 bits integer part, 26 bits fractional part */
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

static void mandelbrot_init(void);

static int mandelbrot_calc_low_prec(struct fractal_rect *rect,
        int (*button_yield_cb)(void *), void *button_yield_ctx);

static int mandelbrot_calc_high_prec(struct fractal_rect *rect,
        int (*button_yield_cb)(void *), void *button_yield_ctx);

static void mandelbrot_move(int dx, int dy);

static int mandelbrot_zoom(int factor);

static int mandelbrot_precision(int d);

struct fractal_ops mandelbrot_ops =
{
    .init = mandelbrot_init,
    .calc = NULL,
    .move = mandelbrot_move,
    .zoom = mandelbrot_zoom,
    .precision = mandelbrot_precision,
};

#define LOG2_OUT_OF_BOUNDS -32767

static int ilog2_fp(long value) /* calculate integer log2(value_fp_6.26) */
{
    int i = 0;

    if (value <= 0)
    {
        return LOG2_OUT_OF_BOUNDS;
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

static int recalc_parameters(void)
{
    x_step = (x_max - x_min) / LCD_WIDTH;
    y_step = (y_max - y_min) / LCD_HEIGHT;
    step_log2 = ilog2_fp(MIN(x_step, y_step));

    if (step_log2 == LOG2_OUT_OF_BOUNDS)
        return 1; /* out of bounds */

    x_delta = X_DELTA(x_step);
    y_delta = Y_DELTA(y_step);
    y_delta = (y_step * LCD_HEIGHT) / 8;
    max_iter = MAX(15, -15 * step_log2 - 45);

    ops->calc = (step_log2 <= -10) ?
        mandelbrot_calc_high_prec : mandelbrot_calc_low_prec;

    return 0;
}

static void mandelbrot_init(void)
{
    ops = &mandelbrot_ops;

    x_min = MB_XOFS - MB_XFAC;
    x_max = MB_XOFS + MB_XFAC;
    y_min = -MB_YFAC;
    y_max = MB_YFAC;

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

    a32 = x_min + x_step * rect->px_min;

    for (p_x = rect->px_min; p_x < rect->px_max; p_x++)
    {
        a = a32 >> 16;

        b32 = y_min + y_step * (LCD_HEIGHT - rect->py_max);

        for (p_y = rect->py_max - 1; p_y >= rect->py_min; p_y--)
        {
            b = b32 >> 16;
            x = a;
            y = b;
            n_iter = 0;

            while (++n_iter <= max_iter)
            {
                x2 = MULS16_ASR10(x, x);
                y2 = MULS16_ASR10(y, y);

                if (x2 + y2 > (4<<10)) break;

                y = 2 * MULS16_ASR10(x, y) + b;
                x = x2 - y2 + a;
            }

            if  (n_iter > max_iter)
                imgbuffer[p_y] = CONVERGENCE_COLOR;
            else
                imgbuffer[p_y] = COLOR(n_iter);

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

            b32 += y_step;
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

        a32 += x_step;
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

    a = x_min + x_step * rect->px_min;

    for (p_x = rect->px_min; p_x < rect->px_max; p_x++)
    {
        b = y_min + y_step * (LCD_HEIGHT - rect->py_max);

        for (p_y = rect->py_max - 1; p_y >= rect->py_min; p_y--)
        {
            x = a;
            y = b;
            n_iter = 0;

            while (++n_iter <= max_iter)
            {
                x2 = MULS32_ASR26(x, x);
                y2 = MULS32_ASR26(y, y);

                if (x2 + y2 > (4L<<26)) break;

                y = 2 * MULS32_ASR26(x, y) + b;
                x = x2 - y2 + a;
            }

            if  (n_iter > max_iter)
                imgbuffer[p_y] = CONVERGENCE_COLOR;
            else
                imgbuffer[p_y] = COLOR(n_iter);

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

            b += y_step;
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
        a += x_step;
    }

    rect->valid = 0;

    return 0;
}

static void mandelbrot_move(int dx, int dy)
{
    long d_x = (long)dx * x_delta;
    long d_y = (long)dy * y_delta;

    x_min += d_x;
    x_max += d_x;
    y_min += d_y;
    y_max += d_y;
}

static int mandelbrot_zoom(int factor)
{
    int res;
    long factor_x = (long)factor * x_delta;
    long factor_y = (long)factor * y_delta;

    x_min += factor_x;
    x_max -= factor_x;
    y_min += factor_y;
    y_max -= factor_y;

    res = recalc_parameters();
    if (res) /* zoom not possible, revert */
    {
        mandelbrot_zoom(-factor);
    }

    return res;
}

static int mandelbrot_precision(int d)
{
    int changed = 0;

    /* Precision increase */
    for (; d > 0; d--)
    {
        max_iter += max_iter / 2;
        changed = 1;
    }

    /* Precision decrease */
    for (; d < 0 && max_iter >= 15; d++)
    {
        max_iter -= max_iter / 3;
        changed = 1;
    }

    return changed;
}

