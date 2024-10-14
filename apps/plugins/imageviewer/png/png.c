/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$id $
 *
 * Copyright (C) 2009 by Christophe Gouiran <bechris13250 -at- gmail -dot- com>
 *
 * Based on lodepng, a lightweight png decoder/encoder
 * (c) 2005-2008 Lode Vandevenne
 *
 * Copyright (c) 2010 Marcin Bukat
 *  - pixel format conversion & transparency handling
 *  - adaptation of tinf (tiny inflate library)
 *  - code refactoring & cleanups
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
#include "tinf.h"
#include "../imageviewer.h"
#include "png_decoder.h"
#include "bmp.h"

/* decoder context struct */
static LodePNG_Decoder decoder;

static char print[32]; /* use a common snprintf() buffer */

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
static unsigned char *disp[9];
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
    LodePNG_Decoder *p_decoder = &decoder;

#ifdef USEGSLIB
    return (p_decoder->infoPng.width/ds) * (p_decoder->infoPng.height/ds);
#else
    return (p_decoder->infoPng.width/ds) * 
           (p_decoder->infoPng.height/ds) * 
           FB_DATA_SZ;
#endif
}

static int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size,
                      int offset, int file_size)
{
    int fd;
    long time = 0; /* measured ticks */
    int w, h; /* used to center output */
    LodePNG_Decoder *p_decoder = &decoder;

    unsigned char *memory, *memory_max, *image;
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
        file_size = rb->filesize(fd);
    }

    DEBUGF("reading file '%s'\n", filename);

    if (!iv->running_slideshow) {
        rb->lcd_puts(0, 0, rb->strrchr(filename,'/')+1);
        rb->lcd_update();
    }

    if ((size_t)file_size > memory_size) {
        p_decoder->error = FILE_TOO_LARGE;
        rb->close(fd);

    } else {
        if (!iv->running_slideshow) {
            rb->lcd_putsf(0, 1, "loading %zu bytes", file_size);
            rb->lcd_update();
        }

        /* load file to the end of the buffer */
        image = memory_max - file_size;
        rb->read(fd, image, file_size);
        rb->close(fd);

        if (!iv->running_slideshow) {
            rb->lcd_puts(0, 2, "decoding image");
            rb->lcd_update();
        }
#ifdef DISK_SPINDOWN
        else if (iv->immediate_ata_off) {
            /* running slideshow and time is long enough: power down disk */
            rb->storage_sleep();
        }
#endif

        /* Initialize decoder context struct, set buffer decoder is free
         * to use.
         * Decoder assumes that raw image file is loaded at the very
         * end of the allocated buffer
         */
        LodePNG_Decoder_init(p_decoder, memory, memory_size);

        /* read file header; file is loaded at the end
         * of the allocated buffer
         */
        LodePNG_inspect(p_decoder, image, file_size);

        if (!p_decoder->error) {

            if (!iv->running_slideshow) {
                rb->lcd_putsf(0, 2, "image %dx%d",
                              p_decoder->infoPng.width,
                              p_decoder->infoPng.height);
                rb->lcd_putsf(0, 3, "decoding %d*%d",
                              p_decoder->infoPng.width,
                              p_decoder->infoPng.height);
                rb->lcd_update();
            }

            /* the actual decoding */
            time = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
            LodePNG_decode(p_decoder, image, file_size, iv->cb_progress);
            rb->cpu_boost(false);
#else
            LodePNG_decode(p_decoder, image, file_size, iv->cb_progress);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
            time = *rb->current_tick - time;
        }
    }

    if (!iv->running_slideshow && !p_decoder->error)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    if (p_decoder->error) {
        if (p_decoder->error == FILE_TOO_LARGE ||
            p_decoder->error == OUT_OF_MEMORY)
        {
            return PLUGIN_OUTOFMEM;
        }

        if (LodePNG_perror(p_decoder) != NULL)
        {
            rb->splash(HZ, LodePNG_perror(p_decoder));
        }
        else if (p_decoder->error == TINF_DATA_ERROR)
        {
            rb->splash(HZ, "Zlib decompressor error");
        }
        else
        {
            rb->splashf(HZ, "other error : %ld", p_decoder->error);
        }

        return PLUGIN_ERROR;
    }

    info->x_size = p_decoder->infoPng.width;
    info->y_size = p_decoder->infoPng.height;

    p_decoder->native_img_size = (p_decoder->native_img_size + 3) & ~3;
    disp_buf = p_decoder->buf + p_decoder->native_img_size;
    *buf_size = memory_max - disp_buf;

    return PLUGIN_OK;
}

static int get_image(struct image_info *info, int frame, int ds)
{
    (void)frame;
    unsigned char **p_disp = &disp[ds]; /* short cut */
    LodePNG_Decoder *p_decoder = &decoder;

    info->width = p_decoder->infoPng.width / ds;
    info->height = p_decoder->infoPng.height / ds;
    info->data = p_disp;

    if (*p_disp != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    /* assign image buffer */
    if (ds > 1) {
        if (!iv->running_slideshow)
        {
            rb->lcd_putsf(0, 3, "resizing %d*%d", info->width, info->height);
            rb->lcd_update();
        }
        struct bitmap bmp_src, bmp_dst;

        int size = img_mem(ds);

        if (disp_buf + size >= p_decoder->buf + p_decoder->buf_size) {
            /* have to discard the current */
            int i;
            for (i=1; i<=8; i++)
                disp[i] = NULL; /* invalidate all bitmaps */

            /* start again from the beginning of the buffer */
            disp_buf = p_decoder->buf + p_decoder->native_img_size;
        }

        *p_disp = disp_buf;
        disp_buf += size;

        bmp_src.width = p_decoder->infoPng.width;
        bmp_src.height = p_decoder->infoPng.height;
        bmp_src.data = p_decoder->buf;

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
    } else {
        *p_disp = p_decoder->buf;
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
