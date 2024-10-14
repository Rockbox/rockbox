/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * user intereface of image viewer.
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

#ifndef _IMAGE_VIEWER_H
#define _IMAGE_VIEWER_H

#include "plugin.h"

/* different graphics libraries */
#if LCD_DEPTH < 8
#define USEGSLIB
#include <lib/grey.h>
#else
#include <lib/xlcd.h>
#endif

#include <lib/mylcd.h>

#if defined(USEGSLIB) && defined(IMGDEC)
#undef mylcd_ub_
#undef myxlcd_ub_
#define mylcd_ub_(fn)       iv->fn
#define myxlcd_ub_(fn)      iv->fn
#endif

/* Min memory allowing us to use the plugin buffer
 * and thus not stopping the music
 * *Very* rough estimation:
 * Max 10 000 dir entries * 4bytes/entry (char **) = 40000 bytes
 * + 30k code size = 70 000
 * + 50k min for image = 120 000
 */
#define MIN_MEM 120000

/* State code for output with return. */
enum {
    PLUGIN_OTHER = 0x200,
    PLUGIN_ABORT,
    PLUGIN_OUTOFMEM,
    PLUGIN_JPEG_PROGRESSIVE,

    ZOOM_IN,
    ZOOM_OUT,
    NEXT_FRAME,
};

#if (CONFIG_PLATFORM & PLATFORM_NATIVE) && defined(HAVE_DISK_STORAGE)
#define DISK_SPINDOWN
#endif
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
#define USE_PLUG_BUF
#endif

/* Settings. jpeg needs these */
struct imgview_settings
{
#ifdef HAVE_LCD_COLOR
    int jpeg_colour_mode;
    int jpeg_dither_mode;
#endif
    int ss_timeout;
};

/* structure passed to image decoder. */
struct image_info {
    int x_size, y_size; /* set size of loaded image in load_image(). */
    int width, height;  /* set size of resized image in get_image(). */
    int x, y;           /* display position */
    int frames_count;   /* number of subframes */
    int delay;          /* delay expressed in ticks between frames */
    void *data;         /* use freely in decoder. not touched in ui. */
};

struct imgdec_api {
    const struct imgview_settings *settings;
    bool slideshow_enabled;   /* run slideshow */
    bool running_slideshow;   /* loading image because of slideshw */
#ifdef DISK_SPINDOWN
    bool immediate_ata_off;   /* power down disk after loading */
#endif
#ifdef USE_PLUG_BUF
    bool plug_buf;  /* are we using the plugin buffer or the audio buffer? */
#endif

    /* callback updating a progress meter while image decoding */
    void (*cb_progress)(int current, int total);

#ifdef USEGSLIB
    void (*gray_bitmap_part)(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height);
#endif
};

/* functions need to be implemented in each image decoders. */
struct image_decoder {
    /* set true if unscaled image can be always displayed even when there isn't
     * enough memory for resized image. e.g. when using native format to store
     * image. */
    const bool unscaled_avail;

    /* return needed size of buffer to store downscaled image by ds.
     * this is used to calculate min downscale. */
    int (*img_mem)(int ds);

    /* load image from filename. use the passed buffer to store loaded, decoded
     * or resized image later, so save it to local variables if needed.
     * set width and height of info properly. also, set buf_size to remaining
     * size of buf after load image. it is used to calculate min downscale.
     * return PLUGIN_ERROR for error. ui will skip to next image. */
    int (*load_image)(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size, int offset, int filesize);
    /* downscale loaded image by ds. use the buffer passed to load_image to
     * reszie image and/or store resized image.
     * return PLUGIN_ERROR for error. ui will skip to next image. */
    int (*get_image)(struct image_info *info, int frame, int ds);

    /* draw part of image */
    void (*draw_image_rect)(struct image_info *info,
                            int x, int y, int width, int height);
};

#define IMGDEC_API_VERSION 1

/* image decoder header */
struct imgdec_header {
    struct lc_header lc_hdr; /* must be the first */
    const struct image_decoder *decoder;
    const struct plugin_api **api;
    unsigned short plugin_api_version;
    size_t plugin_api_size;
    const struct imgdec_api **img_api;
    size_t img_api_size;
};

#ifdef IMGDEC
extern const struct imgdec_api *iv;
extern const struct image_decoder image_decoder;

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#define IMGDEC_HEADER \
        const struct plugin_api *rb DATA_ATTR; \
        const struct imgdec_api *iv DATA_ATTR; \
        const struct imgdec_header __header \
        __attribute__ ((section (".header")))= { \
        { PLUGIN_MAGIC, TARGET_ID, IMGDEC_API_VERSION, \
          plugin_start_addr, plugin_end_addr, }, &image_decoder, \
          &rb, PLUGIN_API_VERSION, sizeof(struct plugin_api), \
          &iv, sizeof(struct imgdec_api) };
#else /* PLATFORM_HOSTED */
#define IMGDEC_HEADER \
        const struct plugin_api *rb DATA_ATTR; \
        const struct imgdec_api *iv DATA_ATTR; \
        const struct imgdec_header __header \
        __attribute__((visibility("default"))) = { \
        { PLUGIN_MAGIC, TARGET_ID, IMGDEC_API_VERSION, NULL, NULL }, \
          &image_decoder, &rb, PLUGIN_API_VERSION, sizeof(struct plugin_api), \
          &iv, sizeof(struct imgdec_api), };
#endif /* CONFIG_PLATFORM */
#endif

#endif /* _IMAGE_VIEWER_H */
