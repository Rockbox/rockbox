/*****************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Andrew Mahone
 *
 * scaler benchmark
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
#include "lib/jpeg_mem.h"


static unsigned char output;
static int output_y = 0;
static int font_h;
static unsigned char *plugin_buf;
struct img_part part;

/* a null output plugin to save memory and better isolate scale cost */
static unsigned int get_size_null(struct bitmap *bm)
{
    (void) bm;
    return 1;
}

static void output_row_null(uint32_t row, void * row_in,
                            struct scaler_context *ctx)
{
    (void) row;
    uint32_t *in = (uint32_t *)row_in;
#ifdef HAVE_LCD_COLOR
    uint32_t *lim = in + ctx->bm->width * 3;
#else
    uint32_t *lim = in + ctx->bm->width;
#endif
    while (in < lim)
        output = SC_OUT(*in++, ctx);
    return;
}

struct img_part *store_part_null(void *args)
{
    (void) args;
    part.len = 256;
    part.buf = (typeof(part.buf))plugin_buf;
    return &part;
}

const struct custom_format format_null = {
    .output_row_8 = NULL,
#ifdef HAVE_LCD_COLOR
    .output_row_32 = {
        output_row_null,
        output_row_null
    },
#else
    .output_row_32 = output_row_null,
#endif
    .get_size = get_size_null
};

#define lcd_printf(...) \
do { \
    rb->lcd_putsxyf(0, output_y, __VA_ARGS__); \
    rb->lcd_update_rect(0, output_y, LCD_WIDTH, font_h); \
    output_y += font_h; \
} while (0)

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    size_t plugin_buf_len;
    plugin_buf = (unsigned char *)rb->plugin_get_buffer(&plugin_buf_len);
    struct bitmap bm;
    struct dim in_dim;
    struct rowset rset = {
        .rowstep = 1,
        .rowstart = 0,
    };
    (void)parameter;

    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_getstringsize("A", NULL, &font_h);
    bm.data = plugin_buf;
    int in, out;
    for (in = 64; in < 1025; in <<= 2)
    {
        for (out = 64; out < 257; out <<= 1)
        {
            if (in == out)
                continue;
            lcd_printf("timing %dx%d->%dx>%d scale", in, in, out, out);
            long t1, t2, t_end;
            int count = 0;
            t2 = *(rb->current_tick);
            in_dim.width = in_dim.height = in;
            bm.width = bm.height = rset.rowstop = out;
            while (t2 != (t1 = *(rb->current_tick)));
            t_end = t1 + 10 * HZ;
            do {
                resize_on_load(&bm, false, &in_dim, &rset, (unsigned char *)plugin_buf, plugin_buf_len, &format_null, IF_PIX_FMT(0,) store_part_null, NULL);
                count++;
                t2 = *(rb->current_tick);
            } while (TIME_BEFORE(t2, t_end) || count < 10);
            t2 -= t1;
            t2 *= 10;
            t2 += count >> 1;
            t2 /= count;
            t1 = t2 / 1000;
            t2 -= t1 * 1000;
            lcd_printf("%01d.%03d secs/scale", (int)t1, (int)t2);
            if (!(bm.width && bm.height))
                break;
        }
    }

    while (rb->get_action(CONTEXT_STD,1) != ACTION_STD_OK) rb->yield();
    return PLUGIN_OK;
}
