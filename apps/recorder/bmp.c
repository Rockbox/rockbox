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
#ifndef __PCTOOL__
#include "config.h"
#include "system.h"
#include "bmp.h"
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

/* masks for supported BI_BITFIELDS encodings (16/32 bit), little endian */
static const unsigned char bitfields[3][12] = {
    { 0x00,0x7c,0x00,0,  0xe0,0x03,0x00,0,  0x1f,0x00,0x00,0 }, /* 15 bit */
    { 0x00,0xf8,0x00,0,  0xe0,0x07,0x00,0,  0x1f,0x00,0x00,0 }, /* 16 bit */
    { 0x00,0x00,0xff,0,  0x00,0xff,0x00,0,  0xff,0x00,0x00,0 }, /* 32 bit */
};

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
/* canonical ordered dither matrix */
static const unsigned char dither_matrix[16][16] = {
    {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
    { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
    {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
    { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
    {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
    { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
    {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
    { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
    {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
    { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
    {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
    { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
    {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
    { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
    {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
    { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
};
#endif

#if ((LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)) \
 || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH == 2) \
     && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED))
static const unsigned short vi_pattern[4] = {
    0x0101, 0x0100, 0x0001, 0x0000
};
#endif

/* little endian functions */
static inline unsigned readshort(uint16_t *value)
{
    unsigned char* bytes = (unsigned char*) value;
    return (unsigned)bytes[0] | ((unsigned)bytes[1] << 8);
}

static inline uint32_t readlong(uint32_t *value)
{
    unsigned char* bytes = (unsigned char*) value;
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}
                            
static inline unsigned brightness(union rgb_union color)
{
    return (3 * (unsigned)color.red + 6 * (unsigned)color.green
              + (unsigned)color.blue) / 10;
}

/******************************************************************************
 * read_bmp_file()
 *
 * Reads a BMP file and puts the data in rockbox format in *bitmap.
 *
 *****************************************************************************/
int read_bmp_file(const char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format)
{
    int fd, ret;
    fd = open(filename, O_RDONLY);

    /* Exit if file opening failed */
    if (fd < 0) {
        DEBUGF("read_bmp_file: can't open '%s', rc: %d\n", filename, fd);
        return fd * 10 - 1;
    }

    ret = read_bmp_fd(fd, bm, maxsize, format);
    close(fd);
    return ret;
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
                int format)
{
    struct bmp_header bmph;
    int width, height, padded_width;
    int dst_height, dst_width;
    int depth, numcolors, compression, totalsize;
    int row, col, ret;
    int rowstart, rowstop, rowstep;

    unsigned char *bitmap = bm->data;
    uint32_t bmpbuf[LCD_WIDTH]; /* Buffer for one line */
    uint32_t palette[256];
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    bool transparent = false;
    bool dither = false;
#ifdef HAVE_REMOTE_LCD
    bool remote = false;
    
    if (format & FORMAT_REMOTE) {
        remote = true;
#if LCD_REMOTE_DEPTH == 1
        format = FORMAT_MONO;
#else
        format &= ~FORMAT_REMOTE;
#endif
    }
#endif /* HAVE_REMOTE_LCD */
    if (format & FORMAT_TRANSPARENT) {
        transparent = true;
        format &= ~FORMAT_TRANSPARENT;
    }
    if (format & FORMAT_DITHER) {
        dither = true;
        format &= ~FORMAT_DITHER;
    }
#else

    (void)format;
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */

    /* read fileheader */
    ret = read(fd, &bmph, sizeof(struct bmp_header));
    if (ret < 0) {
        return ret * 10 - 2;
    }

    if (ret != sizeof(struct bmp_header)) {
        DEBUGF("read_bmp_fd: can't read BMP header.");
        return -3;
    }

    width = readlong(&bmph.width);
    if (width > LCD_WIDTH) {
        DEBUGF("read_bmp_fd: Bitmap too wide (%d pixels, max is %d)\n",
                        width, LCD_WIDTH);
        return -4;
    }

    height = readlong(&bmph.height);
    if (height < 0) {     /* Top-down BMP file */
        height = -height;
        rowstart = 0;
        rowstop = height;
        rowstep = 1;
    } else {              /* normal BMP */
        rowstart = height - 1;
        rowstop = -1;
        rowstep = -1;
    }

    depth = readshort(&bmph.bit_count);
    padded_width = ((width * depth + 31) >> 3) & ~3;  /* 4-byte boundary aligned */

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    if (format == FORMAT_ANY) {
        if (depth == 1)
            format = FORMAT_MONO;
        else
            format = FORMAT_NATIVE;
    }
    bm->format = format;
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */
    /* returning image size */
    bm->width = width;
    bm->height = height;

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    if (format == FORMAT_NATIVE) {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
        if (remote) {
#if (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
            dst_width  = width;
            dst_height = (height + 7) >> 3;
#endif /* LCD_REMOTE_DEPTH / LCD_REMOTE_PIXELFORMAT */
            totalsize = dst_width * dst_height * sizeof(fb_remote_data);
        } else
#endif /* defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1 */
        {
#if LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
            dst_width  = (width + 3) >> 2;
            dst_height = height;
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
            dst_width  = width;
            dst_height = (height + 3) >> 2;
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
            dst_width  = width;
            dst_height = (height + 7) >> 3;
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
            dst_width  = width;
            dst_height = height;
#endif /* LCD_DEPTH */
            totalsize  = dst_width * dst_height * sizeof(fb_data);
        }
    } else
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */
    {
        dst_width  = width;
        dst_height = (height + 7) >> 3;
        totalsize  = dst_width * dst_height;
    }

    /* Check if this fits the buffer */
    if (totalsize > maxsize) {
        DEBUGF("read_bmp_fd: Bitmap too large for buffer: "
               "%d bytes.\n", totalsize);
        return -6;
    }

    compression = readlong(&bmph.compression);
    if (depth <= 8) {
        numcolors = readlong(&bmph.clr_used);
        if (numcolors == 0)
            numcolors = 1 << depth;
    } else
        numcolors = (compression == 3) ? 3 : 0;
        
    if (numcolors > 0 && numcolors <= 256) {
        if (read(fd, palette, numcolors * sizeof(uint32_t))
            != numcolors * (int)sizeof(uint32_t))
        {
            DEBUGF("read_bmp_fd: Can't read color palette\n");
            return -7;
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
            if (!memcmp(palette, bitfields[0], 12)) {    /* 15 bit */
                depth = 15;
                break;
            }
            if (!memcmp(palette, bitfields[1], 12)       /* 16 bit */
                || !memcmp(palette, bitfields[2], 12))   /* 32 bit */
            {
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

    /* Search to the beginning of the image data */
    lseek(fd, (off_t)readlong(&bmph.off_bits), SEEK_SET);

    memset(bitmap, 0, totalsize);

    /* loop to read rows and put them to buffer */
    for (row = rowstart; row != rowstop; row += rowstep) {
        unsigned data, mask;
        unsigned char *p;
        uint16_t *p2;
        uint32_t *rp;
        union rgb_union *qp;
        union rgb_union q0, q1;   

        /* read one row */
        ret = read(fd, bmpbuf, padded_width);
        if (ret != padded_width) {
            DEBUGF("read_bmp_fd: error reading image, read returned: %d "
                   "expected: %d\n", ret, padded_width);
            return -9;
        }

        /* convert whole line in-place to XRGB8888 (little endian) */
        rp = bmpbuf + width;
        switch (depth) {
          case 1:
            q0.raw = palette[0];
            q1.raw = palette[1];
            p = (unsigned char*)bmpbuf + ((width + 7) >> 3);
            mask = 0x80 >> ((width + 7) & 7);
            while (p > (unsigned char*)bmpbuf) {
                data = *(--p);
                for (; mask <= 0x80; mask <<= 1)
                    *(--rp) = (data & mask) ? q1.raw : q0.raw;
                mask = 0x01;
            }
            break;

          case 4:
            if (width & 1)
                rp++;
            p = (unsigned char*)bmpbuf + ((width + 1) >> 1);
            while (p > (unsigned char*)bmpbuf) {
                data = *(--p);
                *(--rp) = palette[data & 0x0f];
                *(--rp) = palette[data >> 4];
            }
            break;

          case 8:
            p = (unsigned char*)bmpbuf + width;
            while (p > (unsigned char*)bmpbuf)
                *(--rp) = palette[*(--p)];
            break;
            
          case 15:
          case 16:
            p2 = (uint16_t *)bmpbuf + width;
            while (p2 > (uint16_t *)bmpbuf) {
                unsigned component, rgb;

                data = letoh16(*(--p2));
                /* blue */
                component = (data << 3) & 0xf8;
#ifdef ROCKBOX_BIG_ENDIAN
                rgb = (component | (component >> 5)) << 8;
                /* green */
                data >>= 2;
                if (depth == 15) {
                    component = data & 0xf8;
                    rgb |= component | (component >> 5);
                } else {
                    data >>= 1;
                    component = data & 0xfc;
                    rgb |= component | (component >> 6);
                }
                /* red */
                data >>= 5;
                component = data & 0xf8;
                rgb = (rgb << 8) | component | (component >> 5);
                *(--rp) = rgb << 8;
#else /* little endian */
                rgb = component | (component >> 5);
                /* green */
                data >>= 2;
                if (depth == 15) {
                    component = data & 0xf8;
                    rgb |= (component | (component >> 5)) << 8;
                } else {
                    data >>= 1;
                    component = data & 0xfc;
                    rgb |= (component | (component >> 6)) << 8;
                }
                /* red */
                data >>= 5;
                component = data & 0xf8;
                rgb |= (component | (component >> 5)) << 16;
                *(--rp) = rgb;
#endif
            }
            break;

          case 24:
            p = (unsigned char*)bmpbuf + 3 * width;
            while (p > (unsigned char*)bmpbuf) {
                data = *(--p);
                data = (data << 8) | *(--p);
                data = (data << 8) | *(--p);
                *(--rp) = htole32(data);
            }
            break;

          case 32: /* already in desired format */
            break;
        }
        
        /* Convert to destination format */
        qp = (union rgb_union *)bmpbuf;
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
        if (format == FORMAT_NATIVE) {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
            if (remote) {
#if (LCD_REMOTE_DEPTH == 2) && (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED)
                /* iAudio X5/M5 remote */
                fb_remote_data *dest = (fb_remote_data *)bitmap
                                     + dst_width * (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;
                
                for (col = 0; col < width; col++) {
                    if (dither)
                        delta = dither_matrix[row & 0xf][col & 0xf];
                    bright = brightness(*qp++);
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
                fb_data *dest = (fb_data *)bitmap + dst_width * row;
                int shift = 6;
                int delta = 127;
                unsigned bright;
                unsigned data = 0;
            
                for (col = 0; col < width; col++) {
                    if (dither)
                        delta = dither_matrix[row & 0xf][col & 0xf];
                    bright = brightness(*qp++);
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
                fb_data *dest = (fb_data *)bitmap + dst_width * (row >> 2);
                int shift = 2 * (row & 3);
                int delta = 127;
                unsigned bright;

                for (col = 0; col < width; col++) {
                    if (dither)
                        delta = dither_matrix[row & 0xf][col & 0xf];
                    bright = brightness(*qp++);
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= (~bright & 3) << shift;
                }
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
                /* iAudio M3 */
                fb_data *dest = (fb_data *)bitmap + dst_width * (row >> 3);
                int shift = row & 7;
                int delta = 127;
                unsigned bright;
                
                for (col = 0; col < width; col++) {
                    if (dither)
                        delta = dither_matrix[row & 0xf][col & 0xf];
                    bright = brightness(*qp++);
                    bright = (3 * bright + (bright >> 6) + delta) >> 8;
                    *dest++ |= vi_pattern[bright] << shift;
                }
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
                /* iriver h300, colour iPods, X5 */
                fb_data *dest = (fb_data *)bitmap + dst_width * row;
                int delta = 127;
                unsigned r, g, b;

                for (col = 0; col < width; col++) {
                    if (dither)
                        delta = dither_matrix[row & 0xf][col & 0xf];
                    q0 = *qp++;
                    r = (31 * q0.red + (q0.red >> 3) + delta) >> 8;
                    g = (63 * q0.green + (q0.green >> 2) + delta) >> 8;
                    b = (31 * q0.blue + (q0.blue >> 3) + delta) >> 8;
                    *dest++ = LCD_RGBPACK_LCD(r, g, b);
                }
#endif /* LCD_DEPTH */
            }
        } else
#endif /* (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1) */
        {
            p = bitmap + dst_width * (row >> 3);
            mask = 1 << (row & 7);

            for (col = 0; col < width; col++, p++)
                if (brightness(*qp++) < 128)
                    *p |= mask;
        }
    }

    return totalsize; /* return the used buffer size. */
}
