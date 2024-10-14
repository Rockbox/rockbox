/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#include "../imageviewer.h"
#include "jpeg_decoder.h"

#ifdef HAVE_LCD_COLOR
#include "yuv2rgb.h"
#endif

/**************** begin Application ********************/

/************************* Types ***************************/

struct t_disp
{
#ifdef HAVE_LCD_COLOR
    unsigned char* bitmap[3]; /* Y, Cr, Cb */
    int csub_x, csub_y;
#else
    unsigned char* bitmap[1]; /* Y only */
#endif
    int stride;
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

static struct jpeg jpg; /* too large for stack */

/************************* Implementation ***************************/

static void draw_image_rect(struct image_info *info,
                            int x, int y, int width, int height)
{
    struct t_disp* pdisp = (struct t_disp*)info->data;
#ifdef HAVE_LCD_COLOR
    yuv_bitmap_part(
        pdisp->bitmap, pdisp->csub_x, pdisp->csub_y,
        info->x + x, info->y + y, pdisp->stride,
        x + MAX(0, (LCD_WIDTH - info->width) / 2),
        y + MAX(0, (LCD_HEIGHT - info->height) / 2),
        width, height,
        iv->settings->jpeg_colour_mode, iv->settings->jpeg_dither_mode);
#else
    mylcd_ub_gray_bitmap_part(
        pdisp->bitmap[0], info->x + x, info->y + y, pdisp->stride,
        x + MAX(0, (LCD_WIDTH-info->width)/2),
        y + MAX(0, (LCD_HEIGHT-info->height)/2),
        width, height);
#endif
}

static int img_mem(int ds)
{
    int size;
    struct jpeg *p_jpg = &jpg;

    size = (p_jpg->x_phys/ds/p_jpg->subsample_x[0])
         * (p_jpg->y_phys/ds/p_jpg->subsample_y[0]);
#ifdef HAVE_LCD_COLOR
    if (p_jpg->blocks > 1) /* colour, add requirements for chroma */
    {
        size += (p_jpg->x_phys/ds/p_jpg->subsample_x[1])
              * (p_jpg->y_phys/ds/p_jpg->subsample_y[1]);
        size += (p_jpg->x_phys/ds/p_jpg->subsample_x[2])
              * (p_jpg->y_phys/ds/p_jpg->subsample_y[2]);
    }
#endif
    return size;
}

static int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size,
                      int offset, int filesize)
{
    int fd;
    unsigned char* buf_jpeg; /* compressed JPEG image */
    int status;
    struct jpeg *p_jpg = &jpg;

    rb->memset(&disp, 0, sizeof(disp));
    rb->memset(&jpg, 0, sizeof(jpg));

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

    /* allocate JPEG buffer */
    buf_jpeg = buf;

    /* we can start the decompressed images behind it */
    buf_images = buf_root = buf + filesize;
    buf_images_size = root_size = *buf_size - filesize;

    if (buf_images_size <= 0)
    {
        rb->close(fd);
        return PLUGIN_OUTOFMEM;
    }

    if(!iv->running_slideshow)
    {
        rb->lcd_puts(0, 0, rb->strrchr(filename,'/')+1);
        rb->lcd_putsf(0, 1, "loading %d bytes", filesize);
        rb->lcd_update();
    }

    rb->read(fd, buf_jpeg, filesize);
    rb->close(fd);

    if(!iv->running_slideshow)
    {
        rb->lcd_puts(0, 2, "decoding markers");
        rb->lcd_update();
    }
#ifdef DISK_SPINDOWN
    else if(iv->immediate_ata_off)
    {
        /* running slideshow and time is long enough: power down disk */
        rb->storage_sleep();
    }
#endif

    /* process markers, unstuffing */
    status = process_markers(buf_jpeg, filesize, p_jpg);

    if (status < 0 || (status & (DQT | SOF0)) != (DQT | SOF0))
    {   /* bad format or minimum components not contained */
#ifndef HAVE_LCD_COLOR
        rb->splashf(HZ, "unsupported %d", status);
        return PLUGIN_ERROR;
#else
        return PLUGIN_JPEG_PROGRESSIVE;
#endif
    }

    if (!(status & DHT)) /* if no Huffman table present: */
        default_huff_tbl(p_jpg); /* use default */
    build_lut(p_jpg); /* derive Huffman and other lookup-tables */

    if(!iv->running_slideshow)
    {
        rb->lcd_putsf(0, 2, "image %dx%d", p_jpg->x_size, p_jpg->y_size);
        rb->lcd_update();
    }

    info->x_size = p_jpg->x_size;
    info->y_size = p_jpg->y_size;
    *buf_size = buf_images_size;
    return PLUGIN_OK;
}

static int get_image(struct image_info *info, int frame, int ds)
{
    (void)frame;
    int w, h; /* used to center output */
    int size; /* decompressed image size */
    long time; /* measured ticks */
    int status;
    struct jpeg* p_jpg = &jpg;
    struct t_disp* p_disp = &disp[ds]; /* short cut */

    info->width = p_jpg->x_size / ds;
    info->height = p_jpg->y_size / ds;
    info->data = p_disp;

    if (p_disp->bitmap[0] != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    /* assign image buffer */

    /* physical size needed for decoding */
    size = img_mem(ds);
    if (buf_images_size <= size)
    {   /* have to discard the current */
        int i;
        for (i=1; i<=8; i++)
            disp[i].bitmap[0] = NULL; /* invalidate all bitmaps */
        buf_images = buf_root; /* start again from the beginning of the buffer */
        buf_images_size = root_size;
    }

#ifdef HAVE_LCD_COLOR
    if (p_jpg->blocks > 1) /* colour jpeg */
    {
        int i;

        for (i = 1; i < 3; i++)
        {
            size = (p_jpg->x_phys / ds / p_jpg->subsample_x[i])
                 * (p_jpg->y_phys / ds / p_jpg->subsample_y[i]);
            p_disp->bitmap[i] = buf_images;
            buf_images += size;
            buf_images_size -= size;
        }
        p_disp->csub_x = p_jpg->subsample_x[1];
        p_disp->csub_y = p_jpg->subsample_y[1];
    }
    else
    {
        p_disp->csub_x = p_disp->csub_y = 0;
        p_disp->bitmap[1] = p_disp->bitmap[2] = buf_images;
    }
#endif
    /* size may be less when decoded (if height is not block aligned) */
    size = (p_jpg->x_phys/ds) * (p_jpg->y_size/ds);
    p_disp->bitmap[0] = buf_images;
    buf_images += size;
    buf_images_size -= size;

    if(!iv->running_slideshow)
    {
        rb->lcd_putsf(0, 3, "decoding %d*%d", info->width, info->height);
        rb->lcd_update();
    }

    /* update image properties */
    p_disp->stride = p_jpg->x_phys / ds; /* use physical size for stride */

    /* the actual decoding */
    time = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, iv->cb_progress);
    rb->cpu_boost(false);
#else
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, iv->cb_progress);
#endif
    if (status)
    {
        rb->splashf(HZ, "decode error %d", status);
        return PLUGIN_ERROR;
    }
    time = *rb->current_tick - time;

    if(!iv->running_slideshow)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    return PLUGIN_OK;
}

const struct image_decoder image_decoder = {
    false,
    img_mem,
    load_image,
    get_image,
    draw_image_rect,
};

IMGDEC_HEADER
