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
#include "debug.h"
#include "lcd.h"
#include "file.h"
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
#include "system.h"
#include "bmp.h"
#include "resize.h"
#include "debug.h"
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

union rgb_union {
    struct { /* Little endian */
        unsigned char blue;
        unsigned char green;
        unsigned char red;
        unsigned char reserved;
    };
    uint32_t raw;
};

/* masks for supported BI_BITFIELDS encodings (16/32 bit) */
static const struct uint8_rgb bitfields[3][3] = {
    /* 15bit */
    {
        { .blue = 0x00, .green = 0x7c, .red = 0x00 },
        { .blue = 0xe0, .green = 0x03, .red = 0x00 },
        { .blue = 0x1f, .green = 0x00, .red = 0x00 },
    },
    /* 16bit */
    {
        { .blue = 0x00, .green = 0xf8, .red = 0x00 },
        { .blue = 0xe0, .green = 0x07, .red = 0x00 },
        { .blue = 0x1f, .green = 0x00, .red = 0x00 },
    },
    /* 32bit */
    {
        { .blue = 0x00, .green = 0x00, .red = 0xff },
        { .blue = 0x00, .green = 0xff, .red = 0x00 },
        { .blue = 0xff, .green = 0x00, .red = 0x00 },
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

static inline void set_rgb_union(struct uint8_rgb *dst, union rgb_union src)
{
    dst->red = src.red;
    dst->green = src.green;
    dst->blue = src.blue;
}

struct bmp_args {
    int fd;
    short padded_width;
    short read_width;
    short width;
    short depth;
    unsigned char buf[BM_MAX_WIDTH * 4];
    struct uint8_rgb *palette;
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    int cur_row;
    int cur_col;
    struct img_part part;
#endif
};

static unsigned int read_part_line(struct bmp_args *ba)
{
    const int padded_width = ba->padded_width;
    const int read_width = ba->read_width;
    const int width = ba->width;
    const int depth = ba->depth;
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    int cur_row = ba->cur_row;
    int cur_col = ba->cur_col;
#endif
    const int fd = ba->fd;
    uint8_t *ibuf;
    struct uint8_rgb *buf = (struct uint8_rgb *)(ba->buf);
    const struct uint8_rgb *palette = ba->palette;
    uint32_t component, data = data;
    int ret;
    int i, cols, len;

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
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
        BDEBUGF("cur_row: %d cur_col: %d cols: %d len: %d\n", cur_row, cur_col,
                cols, len);
        return 0;
    }
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
            buf++;
            ibuf += 2;
            break;
          case 32:
          case 24:
            buf->blue = *ibuf++;
            buf->green = *ibuf++;
            buf->red = *ibuf++;
            if (depth == 32)
                ibuf++;
            buf++;
            break;
        }
    }

#if !defined(HAVE_LCD_COLOR) && \
        (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1))
    ibuf = ba->buf;
    buf = (struct uint8_rgb*)ba->buf;
    while (ibuf < ba->buf + cols)
        *ibuf++ = brightness(*buf++);
#endif

#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
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
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
        cur_col = 0;
        BDEBUGF("read_part_line: completed row %d\n", cur_row);
        cur_row += 1;
    }

    ba->cur_row = cur_row;
    ba->cur_col = cur_col;
#endif
    return cols;
}

#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
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

static inline int rgbcmp(struct uint8_rgb rgb1, struct uint8_rgb rgb2)
{
    if ((rgb1.red == rgb2.red) && (rgb1.green == rgb2.green) &&
        (rgb1.blue == rgb2.blue))
        return 0;
    else
        return 1;
}

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
    int ret;

    unsigned char *bitmap = bm->data;
    struct uint8_rgb palette[256];
    struct rowset rset;
    struct dim src_dim;
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    bool remote = false;
    unsigned int resize = IMG_NORESIZE;
    bool dither = false;
    bool transparent = false;

#ifdef HAVE_REMOTE_LCD
    if (format & FORMAT_REMOTE) {
        remote = true;
#if LCD_REMOTE_DEPTH == 1
        format = FORMAT_MONO;
#endif
    }
#endif /* HAVE_REMOTE_LCD */

    if (format & FORMAT_RESIZE) {
        resize = IMG_RESIZE;
    }

    if (format & FORMAT_TRANSPARENT) {
        transparent = true;
    }
    if (format & FORMAT_DITHER) {
        dither = true;
    }
#else

    (void)format;
#endif /*(LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)*/

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
        resize &= ~IMG_RESIZE;
        resize |= IMG_NORESIZE;
        remote = 0;
    }
#else
    if (src_dim.width > BM_MAX_WIDTH)
            return -6;
#endif /*(LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)*/

#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
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

    format &= 1;

    if (!(resize & IMG_RESIZE)) {
#endif
        /* returning image size */
        bm->width = src_dim.width;
        bm->height = src_dim.height;

#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    }
#endif

    if (rset.rowstep > 0) {     /* Top-down BMP file */
        rset.rowstart = 0;
        rset.rowstop = bm->height;
    } else {              /* normal BMP */
        rset.rowstart = bm->height - 1;
        rset.rowstop = -1;
    }

    if (cformat)
        totalsize = cformat->get_size(bm);
    else
        totalsize = BM_SIZE(bm->width,bm->height,format,remote);

    /* Check if this fits the buffer */
    if (totalsize > maxsize) {
        DEBUGF("read_bmp_fd: Bitmap too large for buffer: "
               "%d bytes.\n", totalsize);
        return -6;
    }

    compression = letoh32(bmph.compression);
    if (depth <= 8) {
        numcolors = letoh32(bmph.clr_used);
        if (numcolors == 0)
            numcolors = 1 << depth;
    } else
        numcolors = (compression == 3) ? 3 : 0;

    if (numcolors > 0 && numcolors <= 256) {
        int i;
        union rgb_union pal;
        for (i = 0; i < numcolors; i++) {
            if (read(fd, &pal, sizeof(pal)) != (int)sizeof(pal))
            {
                DEBUGF("read_bmp_fd: Can't read color palette\n");
                return -7;
            }
            set_rgb_union(&palette[i], pal);
        }
    }

    switch (depth) {
      case 16:
#if LCD_DEPTH >= 16
        /* don't dither 16 bit BMP to LCD with same or larger depth */
#ifdef HAVE_REMOTE_LCD
        if (!remote)
#endif
            dither = false;
#endif
        if (compression == 0) { /* BI_RGB, i.e. 15 bit */
            depth = 15;
            break;
        }   /* else fall through */

      case 32:
        if (compression == 3) { /* BI_BITFIELDS */
            bool found;
            int i, j;

            /* (i == 0) is 15bit, (i == 1) is 16bit, (i == 2) is 32bit */
            for (i = 0; i < ARRAY_SIZE(bitfields); i++) {
                for (j = 0; j < ARRAY_SIZE(bitfields[0]); j++) {
                    if (!rgbcmp(palette[j], bitfields[i][j])) {
                        found = true;
                    } else {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    if (i == 0) /* 15bit */
                        depth = 15;
                    break;
                }
            }
            if (found)
                break;
        }   /* else fall through */

      default:
        if (compression != 0) { /* not BI_RGB */
            DEBUGF("read_bmp_fd: Unsupported compression (type %d)\n",
                   compression);
            return -8;
        }
        break;
    }

    /* Search to the beginning of the image data */
    lseek(fd, (off_t)letoh32(bmph.off_bits), SEEK_SET);

    memset(bitmap, 0, totalsize);

    struct bmp_args ba = {
        .fd = fd, .padded_width = padded_width, .read_width = read_width,
        .width = src_dim.width, .depth = depth, .palette = palette,
#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
        .cur_row = 0, .cur_col = 0, .part = {0,0}
#endif
    };

#if LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
    if (resize || cformat)
    {
        if (resize_on_load(bm, dither, &src_dim, &rset,
                           bitmap + totalsize, maxsize - totalsize,
                           cformat, store_part_bmp, &ba))
            return totalsize;
        else
            return 0;
    }

    int fb_width = BM_WIDTH(bm->width,bm->format,remote);
#endif /* LCD_DEPTH */

    int col, row;

    /* loop to read rows and put them to buffer */
    for (row = rset.rowstart; row != rset.rowstop; row += rset.rowstep) {
#if !defined(HAVE_LCD_COLOR) && \
        (LCD_DEPTH > 1 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1))
        uint8_t* qp = ba.buf;
#else
        struct uint8_rgb *qp = (struct uint8_rgb *)ba.buf;
#endif
        unsigned mask;
        unsigned char *p;
        if (!read_part_line(&ba))
           return -9;

        /* Convert to destination format */
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
        unsigned char dy = DITHERY(row);
        if (format == FORMAT_NATIVE) {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            if (remote) {
#if (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
                /* iAudio X5/M5 remote */
                fb_remote_data *dest = (fb_remote_data *)bitmap
                                     + bm->width * (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;

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
            {
#if LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
                /* greyscale iPods */
                fb_data *dest = (fb_data *)bitmap + fb_width * row;
                int shift = 6;
                int delta = 127;
                unsigned bright;
                unsigned data = 0;

                for (col = 0; col < bm->width; col++) {
                    if (dither)
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
                fb_data *dest = (fb_data *)bitmap + fb_width * (row >> 2);
                int shift = 2 * (row & 3);
                int delta = 127;
                unsigned bright;

                for (col = 0; col < bm->width; col++) {
                    if (dither)
                        delta = DITHERXDY(col,dy);
                    bright = *qp++;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= (~bright & 3) << shift;
                }
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
                /* iAudio M3 */
                fb_data *dest = (fb_data *)bitmap + fb_width * (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;

                for (col = 0; col < bm->width; col++) {
                    if (dither)
                        delta = DITHERXDY(col,dy);
                    bright = *qp++;
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= vi_pattern[bright] << shift;
                }
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
                /* iriver h300, colour iPods, X5 */
                fb_data *dest = (fb_data *)bitmap + fb_width * row;
                int delta = 127;
                unsigned r, g, b;
                struct uint8_rgb q0;

                for (col = 0; col < bm->width; col++) {
                    if (dither)
                        delta = DITHERXDY(col,dy);
                    q0 = *qp++;
                    r = (31 * q0.red + (q0.red >> 3) + delta) >> 8;
                    g = (63 * q0.green + (q0.green >> 2) + delta) >> 8;
                    b = (31 * q0.blue + (q0.blue >> 3) + delta) >> 8;
                    *dest++ = LCD_RGBPACK_LCD(r, g, b);
                }
#endif /* LCD_DEPTH */
            }
        } else
#endif /* (LCD_DEPTH > 1) ||
          defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */
        {
            p = bitmap + bm->width * (row >> 3);
            mask = 1 << (row & 7);

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
    }
    return totalsize; /* return the used buffer size. */
}
