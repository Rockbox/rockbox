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
 * In-memory JPEG decode benchmark.
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


/* a null output plugin to save memory and better isolate decode cost */
static unsigned int get_size_null(struct bitmap *bm)
{
    (void) bm;
    return 1;
}

static void output_row_null(uint32_t row, void * row_in,
                            struct scaler_context *ctx)
{
    (void) row;
    (void) row_in;
    (void) ctx;
    return;
}

const struct custom_format format_null = {
    .output_row_8 = output_row_null,
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

static int output_y = 0;
static int font_h;

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
    unsigned char * plugin_buf =
        (unsigned char *)rb->plugin_get_buffer(&plugin_buf_len);
    static char filename[MAX_PATH];
    struct bitmap bm = {
        .width = LCD_WIDTH,
        .height = LCD_HEIGHT,
    };
    int ret;

    if(!parameter) return PLUGIN_ERROR;

    rb->strcpy(filename, parameter);
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_getstringsize("A", NULL, &font_h);
    int fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        lcd_printf("file open failed: %d", fd);
        goto wait;
    }
    unsigned long filesize = rb->filesize(fd);
    if (filesize > plugin_buf_len)
    {
        lcd_printf("file too large");
        goto wait;
    }
    plugin_buf_len -= filesize;
    unsigned char *jpeg_buf = plugin_buf;
    plugin_buf += filesize;
    rb->read(fd, jpeg_buf, filesize);
    rb->close(fd);
    bm.data = plugin_buf;
    struct dim jpeg_size;
    get_jpeg_dim_mem(jpeg_buf, filesize, &jpeg_size);
    lcd_printf("jpeg file size: %dx%d",jpeg_size.width, jpeg_size.height);
    bm.width = jpeg_size.width;
    bm.height = jpeg_size.height;
    char *size_str[] = { "1/1", "1/2", "1/4", "1/8" };
    int i;
    for (i = 0; i < 4; i++)
    {
        lcd_printf("timing %s decode", size_str[i]);
        ret = decode_jpeg_mem(jpeg_buf, filesize, &bm, plugin_buf_len,
                            FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT,
                            &format_null);
        if (ret == 1)
        {
            long t1, t2, t_end;
            int count = 0;
            t2 = *(rb->current_tick);
            while (t2 != (t1 = *(rb->current_tick)));
            t_end = t1 + 10 * HZ;
            do {
                decode_jpeg_mem(jpeg_buf, filesize, &bm, plugin_buf_len,
                                FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT,
                                &format_null);
                count++;
                t2 = *(rb->current_tick);
            } while (TIME_BEFORE(t2, t_end) || count < 10);
            t2 -= t1;
            t2 *= 10;
            t2 += count >> 1;
            t2 /= count;
            t1 = t2 / 1000;
            t2 -= t1 * 1000;
            lcd_printf("%01d.%03d secs/decode", (int)t1, (int)t2);
            bm.width >>= 1;
            bm.height >>= 1;
            if (!(bm.width && bm.height))
                break;
        } else
            lcd_printf("insufficient memory");
    }

wait:
    while (rb->get_action(CONTEXT_STD,1) != ACTION_STD_OK) rb->yield();
    return PLUGIN_OK;
}
