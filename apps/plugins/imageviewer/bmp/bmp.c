/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * BMP image viewer
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
#include "bmp.h"
#include "resize.h"
#include <lib/feature_wrappers.h>
#include <lib/pluginlib_bmp.h>

#include "../imageviewer.h"

#ifdef HAVE_LCD_COLOR
#define resize_bitmap   smooth_resize_bitmap
#elif defined(USEGSLIB)
#define resize_bitmap   grey_resize_bitmap
#else
#define resize_bitmap   simple_resize_bitmap
#endif

/**************** begin Application ********************/


/************************* Types ***************************/

struct t_disp
{
    unsigned char* bitmap;
};

/************************* Globals ***************************/

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
static struct t_disp disp[9];

/* my memory pool (from the mp3 buffer) */
static char print[32]; /* use a common snprintf() buffer */

/* the root of the images, hereafter are decompresed ones */
static unsigned char* buf_root;
static int root_size;

/* up to here currently used by image(s) */
static unsigned char* buf_images;
static ssize_t buf_images_size;

struct bitmap bmp;

/************************* Implementation ***************************/

#if defined(USEGSLIB) && (CONFIG_PLATFORM & PLATFORM_HOSTED)
/* hack: fix error "undefined reference to `_grey_info'". */
GREY_INFO_STRUCT
#endif /* USEGSLIB */

static void draw_image_rect(struct image_info *info,
                            int x, int y, int width, int height)
{
    struct t_disp* pdisp = (struct t_disp*)info->data;
#ifdef HAVE_LCD_COLOR
    rb->lcd_bitmap_part(
        (fb_data*)pdisp->bitmap, info->x + x, info->y + y,
        STRIDE(SCREEN_MAIN, info->width, info->height),
        x + MAX(0, (LCD_WIDTH-info->width)/2),
        y + MAX(0, (LCD_HEIGHT-info->height)/2),
        width, height);
#else
    mylcd_ub_gray_bitmap_part(
        pdisp->bitmap, info->x + x, info->y + y, info->width,
        x + MAX(0, (LCD_WIDTH-info->width)/2),
        y + MAX(0, (LCD_HEIGHT-info->height)/2),
        width, height);
#endif
}

static int img_mem(int ds)
{
#ifndef USEGSLIB
    return (bmp.width/ds) * (bmp.height/ds) * sizeof (fb_data);
#else
    return (bmp.width/ds) * (bmp.height/ds);
#endif
}

static int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size,
                      int offset, int filesize)
{
    (void)filesize;
    int w, h; /* used to center output */
    long time; /* measured ticks */
    int fd;
    int size;
    int format = FORMAT_NATIVE;
#ifdef USEGSLIB
    const struct custom_format *cformat = &format_grey;
#else
    const struct custom_format *cformat = &format_native;
#endif

    rb->memset(&disp, 0, sizeof(disp));
    rb->memset(&bmp, 0, sizeof(bmp)); /* clear info struct */

    if ((intptr_t)buf & 3)
    {
        *buf_size -= 4-((intptr_t)buf&3);
        buf += 4-((intptr_t)buf&3);
    }
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

    int ds = 1;
    /* check size of image needed to load image. */
    size = scaled_read_bmp_fd(fd, &bmp, 0, format | FORMAT_RETURN_SIZE, cformat);
#if (LCD_PIXEL_ASPECT_HEIGHT != 1 || LCD_PIXEL_ASPECT_WIDTH != 1)
    if (size <= *buf_size)
    {
        /* correct aspect */
        format |= FORMAT_RESIZE|FORMAT_KEEP_ASPECT;
        bmp.width *= LCD_PIXEL_ASPECT_HEIGHT;
        bmp.height *= LCD_PIXEL_ASPECT_WIDTH;
        bmp.width /= MAX(LCD_PIXEL_ASPECT_HEIGHT, LCD_PIXEL_ASPECT_WIDTH);
        bmp.height /= MAX(LCD_PIXEL_ASPECT_HEIGHT, LCD_PIXEL_ASPECT_WIDTH);
    }
#endif
#ifdef USE_PLUG_BUF
    if (!iv->plug_buf)
#endif
    {
        while (size > *buf_size && bmp.width >= 2 && bmp.height >= 2 && ds < 8)
        {
            /* too large for the buffer. resize on load. */
            format |= FORMAT_RESIZE|FORMAT_KEEP_ASPECT;
            ds *= 2;
            bmp.width /= 2;
            bmp.height /= 2;
            rb->lseek(fd, 0, SEEK_SET);
            size = scaled_read_bmp_fd(fd, &bmp, 0, format | FORMAT_RETURN_SIZE, cformat);
        }
    }
    if (size <= 0)
    {
        rb->close(fd);
        rb->splashf(HZ, "read error %d", size);
        return PLUGIN_ERROR;
    }

    if (size > *buf_size)
    {
        rb->close(fd);
        return PLUGIN_OUTOFMEM;
    }

    if (!iv->running_slideshow)
    {
        rb->lcd_puts(0, 0, rb->strrchr(filename,'/')+1);
        rb->lcd_putsf(0, 1, "loading %dx%d%s",
                        bmp.width, bmp.height, ds == 1?"":"(resize on load)");
        rb->lcd_update();
    }

    /* allocate bitmap buffer */
    bmp.data = buf;

    /* actual loading */
    time = *rb->current_tick;
    rb->lseek(fd, 0, SEEK_SET);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    size = scaled_read_bmp_fd(fd, &bmp, *buf_size, format, cformat);
    rb->cpu_boost(false);
#else
    size = scaled_read_bmp_fd(fd, &bmp, *buf_size, format, cformat);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
    rb->close(fd);
    time = *rb->current_tick - time;

    if (size <= 0)
    {
        rb->splashf(HZ, "load error %d", size);
        return PLUGIN_ERROR;
    }

    if (!iv->running_slideshow)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }
#ifdef DISK_SPINDOWN
    else if(iv->immediate_ata_off)
    {
        /* running slideshow and time is long enough: power down disk */
        rb->storage_sleep();
    }
#endif

    /* we can start the resized images behind it */
    buf_images = buf_root = buf + size;
    buf_images_size = root_size = *buf_size - size;

    if (!iv->running_slideshow)
    {
        rb->lcd_putsf(0, 2, "image %dx%d", bmp.width, bmp.height);
        rb->lcd_update();
    }

    info->x_size = bmp.width;
    info->y_size = bmp.height;
    *buf_size = buf_images_size;
    return PLUGIN_OK;
}

static int get_image(struct image_info *info, int frame, int ds)
{
    (void)frame;
    struct t_disp* p_disp = &disp[ds]; /* short cut */

    info->width = bmp.width/ds;
    info->height = bmp.height/ds;
    info->data = p_disp;

    if (p_disp->bitmap != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    /* assign image buffer */
    if (ds > 1)
    {
        int size; /* resized image size */
        struct bitmap bmp_dst;

        size = img_mem(ds);
        if (buf_images_size <= size)
        {
            /* have to discard the current */
            int i;
            for (i=1; i<=8; i++)
                disp[i].bitmap = NULL; /* invalidate all bitmaps */
            buf_images = buf_root; /* start again from the beginning of the buffer */
            buf_images_size = root_size;
        }

        p_disp->bitmap = buf_images;
        buf_images += size;
        buf_images_size -= size;

        if (!iv->running_slideshow)
        {
            rb->lcd_putsf(0, 3, "resizing %d*%d", info->width, info->height);
            rb->lcd_update();
        }

        bmp_dst.width = info->width;
        bmp_dst.height = info->height;
        bmp_dst.data = p_disp->bitmap;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
        resize_bitmap(&bmp, &bmp_dst);
        rb->cpu_boost(false);
#else
        resize_bitmap(&bmp, &bmp_dst);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
    }
    else
    {
        p_disp->bitmap = bmp.data;
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
