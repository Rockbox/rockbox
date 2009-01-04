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
#include "resize.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#define ARRAY_SIZE(array)    (int)(sizeof(array)/(sizeof(array[0])))

#define IMG_NORESIZE           0
#define IMG_RESIZE             1
#define BM_MAX_WIDTH (((LCD_WIDTH) + 7) & ~7)

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

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) || \
    defined(PLUGIN)
extern const unsigned char dither_table[16];
#define DITHERY(y) (dither_table[(y) & 15] & 0xAA)
#define DITHERX(x) (dither_table[(x) & 15])
#define DITHERXDY(x,dy) (DITHERX(x) ^ dy)
#define DITHERDXY(dx,y) (dx ^ DITHERY(y))
#define DITHERXY(x,y) (DITHERX(x) ^ DITHERY(y))
#endif

/* The /256 version has a mean squared variance from YUV luma of <1 grey level.
   The /8 version is a good deal less accurate, but sufficient on mono as we
   don't support HQ output or dithering there, yet.
*/
static inline unsigned brightness(struct uint8_rgb color)
{
#if LCD_DEPTH > 1 || defined(PLUGIN)
    return (77 * (unsigned)color.red + 150 * (unsigned)color.green
              + 29 * (unsigned)color.blue) / 256;
#else
    return (2 * (unsigned)color.red + 5 * (unsigned)color.green
              + (unsigned)color.blue) / 8;
#endif
}

#if ((LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)) \
 || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH == 2) \
     && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED))
extern const unsigned short vi_pattern[4];
#endif

/* Number of rows of data in a mono bitmap height pixels tall */
#define MONO_BM_HEIGHT(height) (((height) + 7) >> 3)

/* Number of rows of datain a LCD native bitmap height pixels tall */
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
#if LCD_DEPTH == 1 || \
    (LCD_DEPTH == 2 && LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
#define LCD_BM_HEIGHT(height) (((height) + 7) >> 3)
#elif LCD_DEPTH == 2 && LCD_PIXELFORMAT == VERTICAL_PACKING
#define LCD_BM_HEIGHT(height) (((height) + 3) >> 2)
#else
#define LCD_BM_HEIGHT(height) (height)
#endif

/* Number of rows of data in a remote native bitmap height pixels tall. */
#ifdef HAVE_REMOTE_LCD
#if LCD_REMOTE_DEPTH == 1 || \
    (LCD_REMOTE_DEPTH == 2 && LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
#define LCD_REMOTE_BM_HEIGHT(height) (((height) + 7) >> 3)
#elif LCD_REMOTE_DEPTH == 2 && LCD_REMOTE_PIXELFORMAT == VERTICAL_PACKING
#define LCD_REMOTE_BM_HEIGHT(height) (((height) + 3) >> 2)
#else
#define LCD_REMOTE_BM_HEIGHT(height) (height)
#endif
#define NATIVE_BM_HEIGHT(height,remote) ((remote) ? \
        LCD_REMOTE_BM_HEIGHT(height) : LCD_BM_HEIGHT(height))
#else
#define NATIVE_BM_HEIGHT(height,remote) LCD_BM_HEIGHT(height)
#endif

/* Convenience macro to calculate rows based on height, remote vs main LCD,
   and format
*/
#define BM_HEIGHT(height,format,remote) ((format) == FORMAT_MONO ? \
    MONO_BM_HEIGHT(height) : NATIVE_BM_HEIGHT(height,remote))
#else
#define BM_HEIGHT(height,format,remote) MONO_BM_HEIGHT(height)
#endif

/* Number of data elements in a mono bitmap width pixels wide */
#define MONO_BM_WIDTH(width) (width)

/* Number of data elements in a LCD native bitmap width pixels wide */
#if LCD_DEPTH > 1
#if LCD_DEPTH == 2 && LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define LCD_BM_WIDTH(width) (((width) + 3) >> 2)
#else
#define LCD_BM_WIDTH(width) (width)
#endif

/* Number of data elements in a remote native bitmap width pixels wide */
#ifdef HAVE_LCD_REMOTE
#if LCD_REMOTE_DEPTH == 2 && LCD_REMOTE_PIXELFORMAT == HORIZONTAL_PACKING
#define LCD_REMOTE_BM_WIDTH(width) (((width) + 3) >> 2)
#else
#define LCD_REMOTE_BM_WIDTH(width) (width)
#endif
#define NATIVE_BM_WIDTH(width,remote) ((remote) ? \
    LCD_REMOTE_BM_WIDTH(width) : LCD_BM_WIDTH(width))
#else
#define NATIVE_BM_WIDTH(width,remote) LCD_BM_WIDTH(width)
#endif

/* Convenience macro to calculate elements based on height, remote vs native
   main LCD, and format
*/
#define BM_WIDTH(width,format,remote) ((format) == FORMAT_MONO ? \
    MONO_BM_WIDTH(width) : NATIVE_BM_WIDTH(width,remote))
#else
#define BM_WIDTH(width,format,remote) MONO_BM_WIDTH(width)
#endif

/* Size in bytes of a mono bitmap of dimensions width*height */
#define MONO_BM_SIZE(width,height) (MONO_BM_WIDTH(width) * \
    MONO_BM_HEIGHT(height) * FB_DATA_SZ)

/* Size in bytes of a native bitmap of dimensions width*height */
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
#if defined(HAVE_REMOTE_LCD) && FB_DATA_SZ != FB_RDATA_SZ
#define NATIVE_BM_SIZE(width,height,format,remote) \
    (((remote) ? FB_RDATA_SZ : FB_DATA_SZ) * BM_WIDTH(width,format,remote) \
    * BM_HEIGHT(height,format,remote))
#else
#define NATIVE_BM_SIZE(width,height,format,remote) \
    (FB_DATA_SZ * BM_WIDTH(width,format,remote) * \
    BM_HEIGHT(height,format,remote))
#endif

/* Convenience macro to calculate size in bytes based on height, remote vs
   main LCD, and format
*/
#define BM_SIZE(width,height,format,remote) (((format) == FORMAT_MONO) ? \
    MONO_BM_SIZE(width,height) : NATIVE_BM_SIZE(width,height,format,remote))
#else
#define BM_SIZE(width,height,format,remote) MONO_BM_SIZE(width,height)
#endif

/* Size in bytes needed to load and scale a bitmap with target size up to
   width*height, including overhead to allow for buffer alignment.
*/
#ifdef HAVE_LCD_COLOR
#define BM_SCALED_SIZE(width,height,format,remote) \
    (BM_SIZE(width,height,format,remote) + \
    (remote ? 0 : BM_WIDTH(width,format,remote) * sizeof(uint32_t) * 9 + 3))
#else
#define BM_SCALED_SIZE(width,height,format,remote) \
    (BM_SIZE(width,height,format,remote) + \
    (width * sizeof(uint32_t) * 3 + 3))
#endif

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
                  int format,
                  const struct custom_format *cformat);

int read_bmp_fd(int fd,
                struct bitmap *bm,
                int maxsize,
                int format,
                const struct custom_format *cformat);
#ifdef PLUGIN
struct plugin_api;
void bmp_init(const struct plugin_api * api);
#endif

#endif
