/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2012 Marcin Bukat
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
#include "bmp.h"
#include "gif_decoder.h"
#include "gif_lib.h"

/* decoder context struct */
static struct gif_decoder decoder;

static char print[32]; /* use a common snprintf() buffer */

/* pointers to decompressed frame in the possible sizes ds = (1,2,4,8)
 * basicaly equivalent to *disp[n][4] where n is the number of frames
 * in gif file. The matrix is allocated after decoded frames.
 */
static unsigned char **disp;
static unsigned char *disp_buf;

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
    struct gif_decoder *p_decoder = &decoder;
    return (p_decoder->native_img_size/ds + 3) & ~3;
}

static int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size,
                      int offset, int filesize)
{
    (void)offset;(void)filesize;

    int w, h;
    long time = 0; /* measured ticks */
    struct gif_decoder *p_decoder = &decoder;

    unsigned char *memory, *memory_max;
    size_t memory_size, img_size, disp_size;

    /* align buffer */
    memory = (unsigned char *)((intptr_t)(buf + 3) & ~3);
    memory_max = (unsigned char *)((intptr_t)(memory + *buf_size) & ~3);
    memory_size = memory_max - memory;

#ifdef DISK_SPINDOWN
        if (iv->running_slideshow && iv->immediate_ata_off) {
            /* running slideshow and time is long enough: power down disk */
            rb->storage_sleep();
        }
#endif

        /* initialize decoder context struct, set buffer decoder is free 
         * to use.
         */
        gif_decoder_init(p_decoder, memory, memory_size);

        /* populate internal data from gif file control structs */
        gif_open(filename, p_decoder);

        if (!p_decoder->error)
        {

            if (!iv->running_slideshow)
            {
                rb->lcd_putsf(0, 2, "file: %s",
                              filename);
                rb->lcd_putsf(0, 3, "size: %dx%d",
                              p_decoder->width,
                              p_decoder->height);
                rb->lcd_update();
            }

            /* the actual decoding */
            time = *rb->current_tick;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            gif_decode(p_decoder, iv->cb_progress);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            time = *rb->current_tick - time;
        }

    gif_decoder_destroy_memory_pool(p_decoder);

    if (!iv->running_slideshow && !p_decoder->error)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    if (p_decoder->error)
    {
        if (p_decoder->error == D_GIF_ERR_NOT_ENOUGH_MEM)
            return PLUGIN_OUTOFMEM;

        rb->splashf(HZ, "%s", GifErrorString(p_decoder->error));
        return PLUGIN_ERROR;
    }

    info->x_size = p_decoder->width;
    info->y_size = p_decoder->height;
    info->frames_count = p_decoder->frames_count;
    info->delay = p_decoder->delay;

    /* check mem constraints
     * each frame can have 4 scaled versions with ds = (1,2,4,8)
     */
    img_size = (p_decoder->native_img_size*p_decoder->frames_count + 3) & ~3;
    disp_size = (sizeof(unsigned char *)*p_decoder->frames_count*4 + 3) & ~3;

    /* No memory to allocate disp matrix */
    if (memory_size < img_size + disp_size)
        return PLUGIN_OUTOFMEM;

    disp = (unsigned char **)(p_decoder->mem + img_size);
    disp_buf = (unsigned char *)disp + disp_size;

    *buf_size = memory_max - disp_buf;

    /* set all pointers to NULL initially */
    memset(disp, 0, sizeof(unsigned char *)*p_decoder->frames_count*4);

    return PLUGIN_OK;
}

/* small helper to convert scalling factor ds
 * into disp[frame][] array index
 */
static int ds2index(int ds)
{
    int index = 0;

    ds >>= 1;
    while (ds)
    {
        index++;
        ds >>=1;
    }

    return index;
}

static int get_image(struct image_info *info, int frame, int ds)
{
    unsigned char **p_disp = disp + frame*4 + ds2index(ds);
    struct gif_decoder *p_decoder = &decoder;

    info->width = p_decoder->width / ds;
    info->height = p_decoder->height / ds;
    info->data = p_disp;

    if (*p_disp != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    /* assign image buffer */
    if (ds > 1)
    {
        if (!iv->running_slideshow && (info->frames_count == 1))
        {
            rb->lcd_putsf(0, 3, "resizing %d*%d", info->width, info->height);
            rb->lcd_update();
        }
        struct bitmap bmp_src, bmp_dst;

        /* size of the scalled image */
        int size = img_mem(ds);

        if (disp_buf + size >= p_decoder->mem + p_decoder->mem_size)
        {
            /* have to discard scaled versions */
            for (int i=0; i<p_decoder->frames_count; i++)
            {
                /* leave unscaled pointer allone,
                 * set rest to NULL
                 */
                p_disp = disp + i*4 + 1;
                memset(p_disp, 0, 3*sizeof(unsigned char *));
            }

            /* start again from the beginning of the buffer */
            disp_buf = p_decoder->mem +
                       p_decoder->native_img_size*p_decoder->frames_count +
                       sizeof(unsigned char *)*p_decoder->frames_count*4;
        }

        *p_disp = disp_buf;
        disp_buf += size;

        bmp_src.width = p_decoder->width;
        bmp_src.height = p_decoder->height;
        bmp_src.data = p_decoder->mem + p_decoder->native_img_size*frame;

        bmp_dst.width = info->width;
        bmp_dst.height = info->height;
        bmp_dst.data = *p_disp;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
#endif
        resize_bitmap(&bmp_src, &bmp_dst);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
    }
    else
    {
        *p_disp = p_decoder->mem + p_decoder->native_img_size*frame;
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
