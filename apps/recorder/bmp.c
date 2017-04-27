/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

/*
2005-04-16 Tomas Salfischberger:
 - New BMP loader function, based on the old one (borrowed a lot of
   calculations and checks there.)
 - Conversion part needs some optimization, doing unneeded calulations now.
2006-11-18 Jens Arnold: complete rework
 - All canonical formats supported now (1, 4, 8, 15/16, 24 and 32 bit)
 - better protection against malformed / non-standard BMPs
 - code heavily optimised for both size and speed
 - dithering for 2 bit targets
2008-11-02 Akio Idehara: refactor for scaler frontend
2008-12-08 Andrew Mahone: partial-line reading, scaler frontend
 - read_part_line does the actual source BMP reading, return columns read
   and updates fields in a struct bmp_args with the new data and current
   reader state
 - skip_lines_bmp and store_part_bmp implement the scaler callbacks to skip
   ahead by whole lines, or read the next chunk of the current line
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inttypes.h"
#include "system.h"
#ifndef PLUGIN
#include "debug.h"
#endif
#include "lcd.h"
#include "file.h"
#include "bmp.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#ifdef ROCKBOX_DEBUG_BMP_LOADER
#define BDEBUGF DEBUGF
#else
#define BDEBUGF(...)
#endif
#ifndef __PCTOOL__
#include "config.h"
#include "resize.h"
#else
#undef DEBUGF
#define DEBUGF(...)
#endif

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#else
#define STRUCT_PACKED
#pragma pack (push, 2)
#endif

/* BMP header structure */
struct bmp_header {
    uint16_t type;          /* signature - 'BM' */
    uint32_t size;          /* file size in bytes */
    uint16_t reserved1;     /* 0 */
    uint16_t reserved2;     /* 0 */
    uint32_t off_bits;      /* offset to bitmap */
    uint32_t struct_size;   /* size of this struct (40) */
    int32_t width;          /* bmap width in pixels */
    int32_t height;         /* bmap height in pixels */
    uint16_t planes;        /* num planes - always 1 */
    uint16_t bit_count;     /* bits per pixel */
    uint32_t compression;   /* compression flag */
    uint32_t size_image;    /* image size in bytes */
    int32_t x_pels_per_meter;   /* horz resolution */
    int32_t y_pels_per_meter;   /* vert resolution */
    uint32_t clr_used;      /* 0 -> color table size */
    uint32_t clr_important; /* important color count */
} STRUCT_PACKED;

/* masks for supported BI_BITFIELDS encodings (16/32 bit) */
static const struct uint8_rgb bitfields[][4] = {
    /* 15bit */
    {
        { .blue = 0x00, .green = 0x7c, .red = 0x00, .alpha = 0x00 },
        { .blue = 0xe0, .green = 0x03, .red = 0x00, .alpha = 0x00 },
        { .blue = 0x1f, .green = 0x00, .red = 0x00, .alpha = 0x00 },
        { .blue = 0x00, .green = 0x00, .red = 0x00, .alpha = 0x00 },
    },
    /* 16bit */
    {
        { .blue = 0x00, .green = 0xf8, .red = 0x00, .alpha = 0x00  },
        { .blue = 0xe0, .green = 0x07, .red = 0x00, .alpha = 0x00  },
        { .blue = 0x1f, .green = 0x00, .red = 0x00, .alpha = 0x00  },
        { .blue = 0x00, .green = 0x00, .red = 0x00, .alpha = 0x00  },
    },
    /* 32bit BGRA */
    {
        { .blue = 0x00, .green = 0x00, .red = 0xff, .alpha = 0x00  },
        { .blue = 0x00, .green = 0xff, .red = 0x00, .alpha = 0x00  },
        { .blue = 0xff, .green = 0x00, .red = 0x00, .alpha = 0x00  },
        { .blue = 0x00, .green = 0x00, .red = 0x00, .alpha = 0xff  },
    },
    /* 32bit ABGR */
    {
        { .blue = 0x00, .green = 0x00, .red = 0x00, .alpha = 0xff },
        { .blue = 0x00, .green = 0x00, .red = 0xff, .alpha = 0x00 },
        { .blue = 0x00, .green = 0xff, .red = 0x00, .alpha = 0x00 },
        { .blue = 0xff, .green = 0x00, .red = 0x00, .alpha = 0x00 },
    },
};

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
/* the full 16x16 Bayer dither matrix may be calculated quickly with this table
*/
const unsigned char dither_table[16] =
    {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 };
#endif

#if ((LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)) \
 || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH == 2) \
     && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED))
const unsigned short vi_pattern[4] = {
    0x0101, 0x0100, 0x0001, 0x0000
};
#endif

/******************************************************************************
 * read_bmp_file()
 *
 * Reads a BMP file and puts the data in rockbox format in *bitmap.
 *
 *****************************************************************************/
int read_bmp_file(const char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format,
                  const struct custom_format *cformat)
{
    int fd, ret;
    fd = open(filename, O_RDONLY);

    /* Exit if file opening failed */
    if (fd < 0) {
        DEBUGF("read_bmp_file: can't open '%s', rc: %d\n", filename, fd);
        return fd * 10 - 1;
    }

    BDEBUGF("read_bmp_file: '%s' remote: %d resize: %d keep_aspect: %d\n",
           filename, !!(format & FORMAT_REMOTE), !!(format & FORMAT_RESIZE),
           !!(format & FORMAT_KEEP_ASPECT));
    ret = read_bmp_fd(fd, bm, maxsize, format, cformat);
    close(fd);
    return ret;
}

enum color_order {
    /* only used for different types of 32bpp images */
    BGRA, /* should be most common */
    ABGR  /* generated by some GIMP versions */
};

struct bmp_args {
    /* needs to be at least 2byte aligned for faster 16bit reads.
     * but aligning to cache should be even faster */
    unsigned char buf[BM_MAX_WIDTH * 4] CACHEALIGN_AT_LEAST_ATTR(2);
    int fd;
    short padded_width;
    short read_width;
    short width;
    short depth;
    enum color_order order;
    struct uint8_rgb *palette;
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    int cur_row;
    int cur_col;
    struct img_part part;
#endif
    /* as read_part_line() goes through the rows it'll set this to true
     * if it finds transparency. Initialize to 0 before calling */
    int alpha_detected;
    /* for checking transparency it checks the against the very first byte
     * of the bitmap. Initalize to 0x80 before calling */
    unsigned char first_alpha_byte;
};

static unsigned int read_part_line(struct bmp_args *ba)
{
    const int padded_width = ba->padded_width;
    const int read_width = ba->read_width;
    const int width = ba->width;
    int depth = ba->depth;
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    int cur_row = ba->cur_row;
    int cur_col = ba->cur_col;
#endif
    const int fd = ba->fd;
    uint8_t *ibuf;
    struct uint8_rgb *buf = (struct uint8_rgb *)(ba->buf);
    const struct uint8_rgb *palette = ba->palette;
    uint32_t component, data;
    int ret;
    int i, cols, len;

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    cols = MIN(width - cur_col,(int)BM_MAX_WIDTH);
    BDEBUGF("reading %d cols (width: %d, max: %d)\n",cols,width,BM_MAX_WIDTH);
    len = (cols * (depth == 15 ? 16 : depth) + 7) >> 3;
#else
    cols = width;
    len = read_width;
#endif
    ibuf = ((unsigned char *)buf) + (BM_MAX_WIDTH << 2) - len;
    BDEBUGF("read_part_line: cols=%d len=%d\n",cols,len);
    ret = read(fd, ibuf, len);
    if (ret != len)
    {
        DEBUGF("read_part_line: error reading image, read returned %d "
               "expected %d\n", ret, len);
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        BDEBUGF("cur_row: %d cur_col: %d cols: %d len: %d\n", cur_row, cur_col,
                cols, len);
#endif
        return 0;
    }

    /* detect if the image has useful alpha information.
     * if all alpha bits are 0xff or 0x00 discard the information.
     * if it has other bits, or is mixed with 0x00 and 0xff then interpret
     * as alpha. assume no alpha until the opposite is proven. as mixed
     * is alpha, compare to the first byte instead of 0xff and 0x00 separately
     */
    if (depth == 32 && ba->first_alpha_byte == 0x80)
        ba->first_alpha_byte = ibuf[3] ? 0xff : 0x0;

     /* select different color orders within the switch-case to avoid
      * nested if/switch */
    if (depth == 32)
        depth += ba->order;

    while (ibuf < ba->buf + (BM_MAX_WIDTH << 2))
    {
        switch (depth)
        {
          case 1:
            data = *ibuf++;
            for (i = 0; i < 8; i++)
            {
                *buf++ = palette[data & 0x80 ? 1 : 0];
                data <<= 1;
            }
            break;
          case 4:
            data = *ibuf++;
            *buf++ = palette[data >> 4];
            *buf++ = palette[data & 0xf];
            break;
          case 8:
            *buf++ = palette[*ibuf++];
            break;
          case 15:
          case 16:
            data = letoh16(*(uint16_t*)ibuf);
            component = (data << 3) & 0xf8;
            component |= component >> 5;
            buf->blue = component;
            if (depth == 15)
            {
                data >>= 2;
                component = data & 0xf8;
                component |= component >> 5;
            } else {
                data >>= 3;
                component = data & 0xfc;
                component |= component >> 6;
            }
            buf->green = component;
            data >>= 5;
            component = data & 0xf8;
            component |= component >> 5;
            buf->red = component;
            buf->alpha = 0xff;
            buf++;
            ibuf += 2;
            break;
          case 24:
            buf->blue = *ibuf++;
            buf->green = *ibuf++;
            buf->red = *ibuf++;
            buf->alpha = 0xff;
            buf++;
            break;
          case 32 + BGRA:
            buf->blue = *ibuf++;
            buf->green = *ibuf++;
            buf->red = *ibuf++;
            buf->alpha = *ibuf++;
            ba->alpha_detected |= (buf->alpha != ba->first_alpha_byte);
            buf++;
            break;
          case 32 + ABGR:
            buf->alpha = *ibuf++;
            buf->blue = *ibuf++;
            buf->green = *ibuf++;
            buf->red = *ibuf++;
            ba->alpha_detected |= (buf->alpha != ba->first_alpha_byte);
            buf++;
            break;
        }
    }
#if !defined(HAVE_LCD_COLOR) && \
    ((LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) || \
    defined(PLUGIN))
    ibuf = ba->buf;
    buf = (struct uint8_rgb*)ba->buf;
    while (ibuf < ba->buf + cols)
        *ibuf++ = brightness(*buf++);
#endif

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    cur_col += cols;
    if (cur_col == width)
    {
#endif
        int pad = padded_width - read_width;
        if (pad > 0)
        {
            BDEBUGF("seeking %d bytes to next line\n",pad);
            lseek(fd, pad, SEEK_CUR);
        }
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        cur_col = 0;
        BDEBUGF("read_part_line: completed row %d\n", cur_row);
        cur_row += 1;
    }

    ba->cur_row = cur_row;
    ba->cur_col = cur_col;
#endif
    return cols;
}

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
static struct img_part *store_part_bmp(void *args)
{
    struct bmp_args *ba = (struct bmp_args *)args;

    ba->part.len = read_part_line(ba);
#ifdef HAVE_LCD_COLOR
    ba->part.buf = (struct uint8_rgb *)ba->buf;
#else
    ba->part.buf = (uint8_t *)ba->buf;
#endif
    if (ba->part.len)
        return &(ba->part);
    else
        return NULL;
}
#endif

static inline int rgbcmp(const struct uint8_rgb *rgb1, const struct uint8_rgb *rgb2)
{
    return memcmp(rgb1, rgb2, sizeof(struct uint8_rgb));
}

#if LCD_DEPTH > 1
#if !defined(PLUGIN) && !defined(HAVE_JPEG) && !defined(HAVE_BMP_SCALING)
static inline
#endif
void output_row_8_native(uint32_t row, void * row_in,
                              struct scaler_context *ctx)
{
    int col;
    int fb_width = BM_WIDTH(ctx->bm->width,FORMAT_NATIVE,0);
    uint8_t dy = DITHERY(row);
#ifdef HAVE_LCD_COLOR
    struct uint8_rgb *qp = (struct uint8_rgb*)row_in;
#else
    uint8_t *qp = (uint8_t*)row_in;
#endif
    BDEBUGF("output_row: y: %lu in: %p\n",row, row_in);
#if LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
                /* greyscale iPods */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width * row;
                int shift = 6;
                int delta = 127;
                unsigned bright;
                unsigned data = 0;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    bright = *qp++;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    data |= (~bright & 3) << shift;
                    shift -= 2;
                    if (shift < 0) {
                        *dest++ = data;
                        data = 0;
                        shift = 6;
                    }
                }
                if (shift < 6)
                    *dest++ = data;
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
                /* iriver H1x0 */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width *
                                (row >> 2);
                int shift = 2 * (row & 3);
                int delta = 127;
                unsigned bright;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    bright = *qp++;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= (~bright & 3) << shift;
                }
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
                /* iAudio M3 */
                fb_data *dest = (fb_data *)ctx->bm->data + fb_width *
                                (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;

                for (col = 0; col < ctx->bm->width; col++) {
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    bright = *qp++;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= vi_pattern[bright] << shift;
                }
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH >= 16
                /* iriver h300, colour iPods, X5 */
                (void)fb_width;
                fb_data *dest = STRIDE_MAIN((fb_data *)ctx->bm->data + fb_width * row,
                                            (fb_data *)ctx->bm->data + row);
                int delta = 127;
                unsigned r, g, b;                
                /* setup alpha channel buffer */
                unsigned char *bm_alpha = NULL;
                if (ctx->bm->alpha_offset > 0)
                    bm_alpha = ctx->bm->data + ctx->bm->alpha_offset;
                if (bm_alpha)
                    bm_alpha += ALIGN_UP(ctx->bm->width, 2) * row/2;

                for (col = 0; col < ctx->bm->width; col++) {
                    (void) delta;
                    if (ctx->dither)
                        delta = DITHERXDY(col,dy);
                    r = qp->red;
                    g = qp->green;
                    b = qp->blue;
#if LCD_DEPTH < 24
                    r = (31 * r + (r >> 3) + delta) >> 8;
                    g = (63 * g + (g >> 2) + delta) >> 8;
                    b = (31 * b + (b >> 3) + delta) >> 8;
#endif
                    *dest = FB_RGBPACK_LCD(r, g, b);
                    dest += STRIDE_MAIN(1, ctx->bm->height);
                    if (bm_alpha) {
                        /* pack alpha channel for 2 pixels into 1 byte and negate
                         * according to the interal alpha channel format */
                        uint8_t alpha = ~qp->alpha;
                        if (col%2)
                            *bm_alpha++ |= alpha&0xf0;
                        else
                            *bm_alpha = alpha>>4;
                    }
                    qp++;
                }
#endif /* LCD_DEPTH */
}
#endif

/******************************************************************************
 * read_bmp_fd()
 *
 * Reads a BMP file in an open file descriptor and puts the data in rockbox
 * format in *bitmap.
 *
 *****************************************************************************/
int read_bmp_fd(int fd,
                struct bitmap *bm,
                int maxsize,
                int format,
                const struct custom_format *cformat)
{
    struct bmp_header bmph;
    int padded_width;
    int read_width;
    int depth, numcolors, compression, totalsize;
    int ret, hdr_size;
    bool return_size = format & FORMAT_RETURN_SIZE;
    bool read_alpha = format & FORMAT_TRANSPARENT;
    enum color_order order = BGRA;

    unsigned char *bitmap = bm->data;
    struct uint8_rgb palette[256];
    struct rowset rset;
    struct dim src_dim;
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) || \
    defined(PLUGIN)
    bool dither = false;
#endif

    bool remote = false;
#ifdef HAVE_REMOTE_LCD
    if (format & FORMAT_REMOTE) {
        remote = true;
#if LCD_REMOTE_DEPTH == 1
        format = FORMAT_MONO;
#endif
    }
#endif /* HAVE_REMOTE_LCD */

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    unsigned int resize = IMG_NORESIZE;

    if (format & FORMAT_RESIZE) {
        resize = IMG_RESIZE;
    }

#else

    (void)format;
#endif /*(LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)*/
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) || \
    defined(PLUGIN)
    if (format & FORMAT_DITHER) {
        dither = true;
    }
#endif
    /* read fileheader */
    ret = read(fd, &bmph, sizeof(struct bmp_header));
    if (ret < 0) {
        return ret * 10 - 2;
    }

    if (ret != sizeof(struct bmp_header)) {
        DEBUGF("read_bmp_fd: can't read BMP header.");
        return -3;
    }

    src_dim.width = letoh32(bmph.width);
    src_dim.height = letoh32(bmph.height);
    if (src_dim.height < 0) {     /* Top-down BMP file */
        src_dim.height = -src_dim.height;
        rset.rowstep = 1;
    } else {              /* normal BMP */
        rset.rowstep = -1;
    }

    depth = letoh16(bmph.bit_count);
    /* 4-byte boundary aligned */
    read_width = ((src_dim.width * (depth == 15 ? 16 : depth) + 7) >> 3);
    padded_width = (read_width + 3) & ~3;

    BDEBUGF("width: %d height: %d depth: %d padded_width: %d\n", src_dim.width,
            src_dim.height, depth, padded_width);

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    if ((format & 3) == FORMAT_ANY) {
        if (depth == 1)
            format = (format & ~3);
        else
            format = (format & ~3) | FORMAT_NATIVE;
    }
    bm->format = format & 1;
    if ((format & 1) == FORMAT_MONO)
    {
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        resize &= ~IMG_RESIZE;
        resize |= IMG_NORESIZE;
#endif
        remote = false;
    }
#elif !defined(PLUGIN)
    if (src_dim.width > BM_MAX_WIDTH)
            return -6;
#endif /*(LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)*/

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    if (resize & IMG_RESIZE) {
        if(format & FORMAT_KEEP_ASPECT) {
            /* keep aspect ratio.. */
            struct dim resize_dim = {
                .width = bm->width,
                .height = bm->height,
            };
            if (recalc_dimension(&resize_dim, &src_dim))
                resize = IMG_NORESIZE;
            bm->width = resize_dim.width;
            bm->height = resize_dim.height;
        }
    }

    if (!(resize & IMG_RESIZE)) {
#endif
        /* returning image size */
        bm->width = src_dim.width;
        bm->height = src_dim.height;

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    }
#endif
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    format &= 1;
#endif
    if (rset.rowstep > 0) {     /* Top-down BMP file */
        rset.rowstart = 0;
        rset.rowstop = bm->height;
    } else {              /* normal BMP */
        rset.rowstart = bm->height - 1;
        rset.rowstop = -1;
    }

    /* need even rows (see lcd-16bit-common.c for details) */
    int alphasize = ALIGN_UP(bm->width, 2) * bm->height / 2;
    if (cformat)
        totalsize = cformat->get_size(bm);
    else {
        totalsize = BM_SIZE(bm->width,bm->height,format,remote);
        if (!remote)
            if (depth == 32 && read_alpha) /* account for possible 4bit alpha per pixel */
                totalsize += alphasize;
    }

    if(return_size)
    {
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        if(resize)
            totalsize += BM_SCALED_SIZE(bm->width, 0, 0, 0);
        else if (bm->width > BM_MAX_WIDTH)
            totalsize += bm->width*4;
#endif
        return totalsize;
    }

    /* Check if this fits the buffer */
    if (totalsize > maxsize) {
        DEBUGF("read_bmp_fd: Bitmap too large for buffer: "
               "%d bytes (%d max).\n", totalsize, maxsize);
        return -6;
    }

    hdr_size = letoh32(bmph.struct_size);
    compression = letoh32(bmph.compression);
    if (depth <= 8) {
        numcolors = letoh32(bmph.clr_used);
        if (numcolors == 0)
            numcolors = BIT_N(depth);
        /* forward to the color table */
        lseek(fd, 14+hdr_size, SEEK_SET);
    } else {
        numcolors = 0;
        if (compression == 3) {
            if (hdr_size >= 56)
                numcolors = 4;
            else /* hdr_size == 52 */
                numcolors = 3;
        }
    }

    /* read color tables. for BI_BITFIELDS this actually
     * reads the color masks */
    if (numcolors > 0 && numcolors <= 256) {
        int i;
        for (i = 0; i < numcolors; i++) {
            if (read(fd, &palette[i], sizeof(struct uint8_rgb))
                    != (int)sizeof(struct uint8_rgb)) {
                DEBUGF("read_bmp_fd: Can't read color palette\n");
                return -7;
            }
        }
    }

    switch (depth) {
      case 16:
#if LCD_DEPTH >= 16
        /* don't dither 16 bit BMP to LCD with same or larger depth */
        if (!remote)
            dither = false;
#endif
        if (compression == 0) { /* BI_RGB, i.e. 15 bit */
            depth = 15;
            break;
        }   /* else fall through */

      case 32:
        if (compression == 3) { /* BI_BITFIELDS */
            bool found = false;
            int i, j;

            /* (i == 0) is 15bit, (i == 1) is 16bit, (i == {2,3}) is 32bit */
            for (i = 0; i < ARRAY_SIZE(bitfields) && !found; i++) {
                /* for 15bpp and higher numcolors has the number of color masks */
                for (j = 0; j < numcolors; j++) {
                    if (!rgbcmp(&palette[j], &bitfields[i][j])) {
                        found = true;
                    } else {
                        found = false;
                        break;
                    }
                }
            }
            if (found) {
                if (i == 1) /* 15bit */
                    depth = 15;
                else if (i == 4) /* 32bit, ABGR bitmap */
                    order = ABGR;
                break;
            }
        }   /* else fall through */

      default:
        if (compression != 0) { /* not BI_RGB */
            DEBUGF("read_bmp_fd: Unsupported compression (type %d)\n",
                   compression);
            return -8;
        }
        break;
    }

#if LCD_DEPTH >= 24
    /* Never dither 24/32 bit BMP to 24 bit LCDs */
    if (depth >= 24 && !remote)
        dither = false;
#endif

    /* Search to the beginning of the image data */
    lseek(fd, (off_t)letoh32(bmph.off_bits), SEEK_SET);

    memset(bitmap, 0, totalsize);

#ifdef HAVE_LCD_COLOR
    if (read_alpha && depth == 32)
        bm->alpha_offset = totalsize - alphasize;
    else
        bm->alpha_offset = 0;
#endif

    struct bmp_args ba = {
        .fd = fd, .padded_width = padded_width, .read_width = read_width,
        .width = src_dim.width, .depth = depth, .palette = palette,
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        .cur_row = 0, .cur_col = 0, .part = {0,0},
#endif
        .alpha_detected = false, .first_alpha_byte = 0x80,
        .order = order,
    };

#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
    if (resize)
    {
        if (resize_on_load(bm, dither, &src_dim, &rset,
                           bitmap + totalsize, maxsize - totalsize,
                           cformat, IF_PIX_FMT(0,) store_part_bmp, &ba))
            return totalsize;
        else
            return 0;
    }
#endif /* LCD_DEPTH */

#if LCD_DEPTH > 1 || defined(PLUGIN)
    struct scaler_context ctx = {
        .bm = bm,
        .dither = dither,
    };
#endif
#if defined(PLUGIN) || defined(HAVE_JPEG) || defined(HAVE_BMP_SCALING)
#if LCD_DEPTH > 1
    void (*output_row_8)(uint32_t, void*, struct scaler_context*) =
        output_row_8_native;
#elif defined(PLUGIN)
    void (*output_row_8)(uint32_t, void*, struct scaler_context*) = NULL;
#endif
#if LCD_DEPTH > 1 || defined(PLUGIN)
    if (cformat)
        output_row_8 = cformat->output_row_8;
#endif
#endif

    unsigned char *buf = ba.buf;
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) || \
    defined(PLUGIN)
    if (bm->width > BM_MAX_WIDTH)
    {
#if defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        unsigned int len = maxsize - totalsize;
        buf = bitmap + totalsize;
        ALIGN_BUFFER(buf, len, sizeof(uint32_t));
        if (bm->width*4 > (int)len)
#endif
            return -6;
    }
#endif
    int row;
    /* loop to read rows and put them to buffer */
    for (row = rset.rowstart; row != rset.rowstop; row += rset.rowstep) {
#if (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)) && \
    defined(HAVE_BMP_SCALING) || defined(PLUGIN)
        if (bm->width > BM_MAX_WIDTH)
        {
#if defined(HAVE_LCD_COLOR)
            struct uint8_rgb *p = (struct uint8_rgb *)buf;
#else
            uint8_t* p = buf;
#endif
            do {
                int len = read_part_line(&ba);
                if (!len)
                    return -9;
                memcpy(p, ba.buf, len*sizeof(*p));
                p += len;
            } while (ba.cur_col);
        }
        else
#endif
        if (!read_part_line(&ba))
            return -9;
#ifndef PLUGIN
#if !defined(HAVE_LCD_COLOR) && \
        (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1))
        uint8_t* qp = buf;
#else
        struct uint8_rgb *qp = (struct uint8_rgb *)buf;
#endif
#endif
        /* Convert to destination format */
#if ((LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)) && \
    !defined(PLUGIN)
        if (format == FORMAT_NATIVE) {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            if (remote) {
                unsigned char dy = DITHERY(row);
#if (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
                /* iAudio X5/M5 remote */
                fb_remote_data *dest = (fb_remote_data *)bitmap
                                     + bm->width * (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;

                int col;
                for (col = 0; col < bm->width; col++) {
                    if (dither)
                        delta = DITHERXDY(col,dy);
#if !defined(HAVE_LCD_COLOR) && \
        (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1))
                    bright = *qp++;
#else
                    bright = brightness(*qp++);
#endif
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= vi_pattern[bright] << shift;
                }
#endif /* LCD_REMOTE_DEPTH / LCD_REMOTE_PIXELFORMAT */
            } else
#endif /* defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1 */
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) &&
    (LCD_REMOTE_DEPTH > 1) */
#if LCD_DEPTH > 1 || defined(PLUGIN)
            {
#if !defined(PLUGIN) && !defined(HAVE_JPEG) && !defined(HAVE_BMP_SCALING)
                output_row_8_native(row, buf, &ctx);
#else
                output_row_8(row, buf, &ctx);
#endif
            }
#endif
#if ((LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)) && \
    !defined(PLUGIN)
        }
#ifndef PLUGIN
        else
#endif
#endif
#ifndef PLUGIN
        {
            unsigned char *p = bitmap + bm->width * (row >> 3);
            unsigned char mask = BIT_N(row & 7);
            int col;
            for (col = 0; col < bm->width; col++, p++)
#if !defined(HAVE_LCD_COLOR) && \
        (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1))
                if (*qp++ < 128)
                    *p |= mask;
#else
                if (brightness(*qp++) < 128)
                    *p |= mask;
#endif
        }
#endif
    }
#ifdef HAVE_LCD_COLOR
    if (!ba.alpha_detected)
    {   /* if this has an alpha channel, totalsize accounts for it as well
         * subtract if no actual alpha information was found */
        if (bm->alpha_offset > 0)
            totalsize -= alphasize;
        bm->alpha_offset = 0;
    }
#endif
    return totalsize; /* return the used buffer size. */
}
