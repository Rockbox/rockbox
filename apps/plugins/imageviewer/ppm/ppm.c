/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include "lcd.h"
#include <lib/pluginlib_bmp.h>
#include "../imageviewer.h"
#include "ppm_decoder.h"
#include "bmp.h"

static char print[32]; /* use a common snprintf() buffer */

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
static unsigned char *disp[9];
static unsigned char *disp_buf;
static struct ppm_info ppm;

#if defined(HAVE_LCD_COLOR)
#define resize_bitmap   smooth_resize_bitmap
#else
#define resize_bitmap   grey_resize_bitmap
#endif

#if defined(USEGSLIB) && (CONFIG_PLATFORM & PLATFORM_HOSTED)
/* hack: fix error "undefined reference to `_grey_info'". */
GREY_INFO_STRUCT
#endif /* USEGSLIB */

static void draw_image_rect(struct image_info *info,
                            int x, int y, int width, int height)
{
    unsigned char **pdisp = (unsigned char **)info->data;

#ifdef HAVE_LCD_COLOR
    rb->lcd_bitmap_part((fb_data *)*pdisp, info->x + x, info->y + y,
                        STRIDE(SCREEN_MAIN, info->width, info->height), 
                        x + MAX(0, (LCD_WIDTH-info->width)/2),
                        y + MAX(0, (LCD_HEIGHT-info->height)/2),
                        width, height);
#else
    mylcd_ub_gray_bitmap_part(*pdisp,
                              info->x + x, info->y + y, info->width,
                              x + MAX(0, (LCD_WIDTH-info->width)/2),
                              y + MAX(0, (LCD_HEIGHT-info->height)/2),
                              width, height);
#endif
}

static int img_mem(int ds)
{

#ifdef USEGSLIB
    return (ppm.x/ds) * (ppm.y/ds);
#else
    return (ppm.x/ds) * (ppm.y/ds) * FB_DATA_SZ;
#endif
}

static int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size,
                      int offset, int filesize)
{
    int fd;
    int rc = PLUGIN_OK;
    long time = 0; /* measured ticks */
    int w, h; /* used to center output */

    unsigned char *memory, *memory_max;
    size_t memory_size;

    /* cleanup */
    memset(&disp, 0, sizeof(disp));

    /* align buffer */
    memory = (unsigned char *)((intptr_t)(buf + 3) & ~3);
    memory_max = (unsigned char *)((intptr_t)(memory + *buf_size) & ~3);
    memory_size = memory_max - memory;

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ, "err opening %s: %d", filename, fd);
        return PLUGIN_ERROR;
    }

    if (offset)
    {
        rb->lseek(fd, offset, SEEK_SET);
    }
    else
    {
        filesize = rb->filesize(fd);
    }
    DEBUGF("reading file '%s'\n", filename);

    if (!iv->running_slideshow)
    {
        rb->lcd_puts(0, 0, rb->strrchr(filename,'/')+1);
        rb->lcd_putsf(0, 1, "loading %zu bytes", filesize);
        rb->lcd_update();
    }

    /* init decoder struct */
    ppm.buf = memory;
    ppm.buf_size = memory_size;

    /* the actual decoding */
    time = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    rc = read_ppm(fd, &ppm);
    rb->cpu_boost(false);
#else
    rc = read_ppm(fd, &ppm);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
    time = *rb->current_tick - time;

    rb->close(fd);

    if (rc != PLUGIN_OK)
    {
        return rc;
    }

    if (!iv->running_slideshow)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    info->x_size = ppm.x;
    info->y_size = ppm.y;

    ppm.native_img_size = (ppm.native_img_size + 3) & ~3;
    disp_buf = buf + ppm.native_img_size;
    *buf_size = memory_max - disp_buf;

    return PLUGIN_OK;
}

static int get_image(struct image_info *info, int frame, int ds)
{
    (void)frame;
    unsigned char **p_disp = &disp[ds]; /* short cut */
    struct ppm_info *p_ppm = &ppm;

    info->width = ppm.x / ds;
    info->height = ppm.y / ds;
    info->data = p_disp;

    if (*p_disp != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    /* assign image buffer */
    if (ds > 1)
    {
        if (!iv->running_slideshow)
        {
            rb->lcd_putsf(0, 3, "resizing %d*%d", info->width, info->height);
            rb->lcd_update();
        }

        struct bitmap bmp_src, bmp_dst;
        int size = img_mem(ds);

        if (disp_buf + size >= p_ppm->buf + p_ppm->buf_size)
        {
            /* have to discard the current */
            int i;
            for (i=1; i<=8; i++)
                disp[i] = NULL; /* invalidate all bitmaps */

            /* start again from the beginning of the buffer */
            disp_buf = p_ppm->buf + p_ppm->native_img_size;
        }

        *p_disp = disp_buf;
        disp_buf += size;

        bmp_src.width = ppm.x;
        bmp_src.height = ppm.y;
        bmp_src.data = ppm.buf;

        bmp_dst.width = info->width;
        bmp_dst.height = info->height;
        bmp_dst.data = *p_disp;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
        resize_bitmap(&bmp_src, &bmp_dst);
        rb->cpu_boost(false);
#else
        resize_bitmap(&bmp_src, &bmp_dst);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
    }
    else
    {
        *p_disp = p_ppm->buf;
    }

    return PLUGIN_OK;
}

const struct image_decoder image_decoder = {
    true,
    img_mem,
    load_image,
    get_image,
    draw_image_rect,
};

IMGDEC_HEADER
