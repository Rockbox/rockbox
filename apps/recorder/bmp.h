/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#ifndef _BMP_H_
#define _BMP_H_

#include "config.h"
#include "lcd.h"
#include "inttypes.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#define ARRAY_SIZE(array)    (int)(sizeof(array)/(sizeof(array[0])))

#define IMG_NORESIZE           0
#define IMG_RESIZE             1
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
#define MAX_WIDTH 8 
#else
#define MAX_WIDTH LCD_WIDTH
#endif

struct uint8_rgb {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

struct dim {
    short width;
    short height;
};

struct rowset {
    short rowstep;
    short rowstart;
    short rowstop;
};

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
extern const unsigned char dither_matrix[16][16];
static inline unsigned char dither_mat(unsigned int x, unsigned int y)
{
    return dither_matrix[y][x];
}
#endif

static inline unsigned brightness(struct uint8_rgb color)
{
    return (3 * (unsigned)color.red + 6 * (unsigned)color.green
              + (unsigned)color.blue) / 10;
}

#if ((LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)) \
 || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH == 2) \
     && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED))
extern const unsigned short vi_pattern[4];
static inline unsigned short vi_pat(unsigned int bright)
{
    return vi_pattern[bright];
}
#endif

static inline int get_fb_height(struct bitmap *bm, bool remote)
{
    const int height = bm->height;
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    const int format = bm->format;
#endif
    int dst_height;

#if !defined(HAVE_REMOTE_LCD) || \
    (defined(HAVE_REMOTE_LCD) &&(LCD_REMOTE_DEPTH == 1))
    (void) remote;
#endif

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    if (format == FORMAT_NATIVE) {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
        if (remote) {
#if (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
            dst_height = (height + 7) >> 3;
#endif /* LCD_REMOTE_DEPTH / LCD_REMOTE_PIXELFORMAT */
        } else
#endif /* defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1 */
        {
#if LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
            dst_height = height;
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
            dst_height = (height + 3) >> 2;
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
            dst_height = (height + 7) >> 3;
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
            dst_height = height;
#endif /* LCD_DEPTH */
        }
    } else
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */
    {
        dst_height = (height + 7) >> 3;
    }

    return dst_height;
}

static inline int get_fb_width(struct bitmap *bm, bool remote)
{
    const int width = bm->width;
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    const int format = bm->format;
#endif
    int dst_width;

#if !defined(HAVE_REMOTE_LCD) || \
    (defined(HAVE_REMOTE_LCD) &&(LCD_REMOTE_DEPTH == 1))
    (void) remote;
#endif

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    if (format == FORMAT_NATIVE) {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
        if (remote) {
#if (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
            dst_width  = width;
#endif /* LCD_REMOTE_DEPTH / LCD_REMOTE_PIXELFORMAT */
        } else
#endif /* defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1 */
        {
#if LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
            dst_width  = (width + 3) >> 2;
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
            dst_width  = width;
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
            dst_width  = width;
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
            dst_width  = width;
#endif /* LCD_DEPTH */
        }
    } else
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */
    {
        dst_width  = width;
    }

    return dst_width;
}

static inline int get_totalsize(struct bitmap *bm, bool remote)
{
    int sz;
#if defined(HAVE_REMOTE_LCD) && \
    (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
    if (remote)
        sz = sizeof(fb_remote_data);
    else
#endif /* LCD_REMOTE_DEPTH / LCD_REMOTE_PIXELFORMAT */
        sz = sizeof(fb_data);

    return get_fb_width(bm, remote) * get_fb_height(bm, remote) * sz;
}

/*********************************************************************
 * read_bmp_file()
 *
 * Reads a 8bit BMP file and puts the data in a 1-pixel-per-byte
 * array.
 * Returns < 0 for error, or number of bytes used from the bitmap buffer
 *
 **********************************************/
int read_bmp_file(const char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format);

int read_bmp_fd(int fd,
                struct bitmap *bm,
                int maxsize,
                int format);
#endif
