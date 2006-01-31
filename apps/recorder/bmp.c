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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "lcd.h"
#include "file.h"
#include "config.h"
#include "bmp.h"
#include "lcd.h"

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#else
#define STRUCT_PACKED
#pragma pack (push, 2)
#endif

/* Struct from original code. */
struct Fileheader {
    unsigned short Type;        /* signature - 'BM' */
    unsigned long Size;         /* file size in bytes */
    unsigned short Reserved1;   /* 0 */
    unsigned short Reserved2;   /* 0 */
    unsigned long OffBits;      /* offset to bitmap */
    unsigned long StructSize;   /* size of this struct (40) */
    unsigned long Width;        /* bmap width in pixels */
    unsigned long Height;       /* bmap height in pixels */
    unsigned short Planes;      /* num planes - always 1 */
    unsigned short BitCount;    /* bits per pixel */
    unsigned long Compression;  /* compression flag */
    unsigned long SizeImage;    /* image size in bytes */
    long XPelsPerMeter;         /* horz resolution */
    long YPelsPerMeter;         /* vert resolution */
    unsigned long ClrUsed;      /* 0 -> color table size */
    unsigned long ClrImportant; /* important color count */
} STRUCT_PACKED;

struct rgb_quad { /* Little endian */
  unsigned char blue; 
  unsigned char green; 
  unsigned char red;
  unsigned char reserved;
} STRUCT_PACKED; 

/* big endian functions */
static short readshort(short *value) {
    unsigned char* bytes = (unsigned char*) value;
    return bytes[0] | (bytes[1] << 8);
}

static long readlong(long *value) {
    unsigned char* bytes = (unsigned char*) value;
    return (long)bytes[0] | ((long)bytes[1] << 8) |
        ((long)bytes[2] << 16) | ((long)bytes[3] << 24);
}

unsigned char brightness(struct rgb_quad color)
{
    return (3 * (unsigned int)color.red + 6 * (unsigned int)color.green
              + (unsigned int)color.blue) / 10;
}

/* Function to get a pixel from a line. (Tomas: maybe a better way?) */
inline int getpix(int px, unsigned char *bmpbuf) {
    int a = (px / 8);
    int b = (7 - (px % 8));

    return (bmpbuf[a] & (1 << b)) != 0;
}


/******************************************************************************
 * read_bmp_file()
 *
 * Reads a monochrome BMP file and puts the data in rockbox format in *bitmap.
 *
 *****************************************************************************/
int read_bmp_file(char* filename,
                  struct bitmap *bm,
                  int maxsize,
                  int format)
{
    struct Fileheader fh;
    int width, height, PaddedWidth, PaddedHeight;
    int fd, row, col, ret;
    struct rgb_quad palette[256];
    int invert_pixel = 0;
    int numcolors;
    int depth;
    int totalsize;
    char *bitmap = bm->data;
    
    unsigned char bmpbuf[LCD_WIDTH*sizeof(struct rgb_quad)]; /* Buffer for one line */

#if LCD_DEPTH == 1
    (void)format;
#else
    bool transparent;
    
    if(format & FORMAT_TRANSPARENT) {
        transparent = true;
        format &= ~FORMAT_TRANSPARENT;
    }
#endif
        
    
    fd = open(filename, O_RDONLY);

    /* Exit if file opening failed */
    if (fd < 0) {
        DEBUGF("error - can't open '%s' open returned: %d\n", filename, fd);
        return (fd * 10) - 1; 
    }

    /* read fileheader */
    ret = read(fd, &fh, sizeof(struct Fileheader));
    if(ret < 0) {
        close(fd);
        return (ret * 10 - 2);
    }
    
    if(ret != sizeof(struct Fileheader)) {
        DEBUGF("error - can't read Fileheader structure.");
        close(fd);
        return -3;
    }

    /* Exit if too wide */
    if (readlong(&fh.Width) > LCD_WIDTH) {
        DEBUGF("error - Bitmap is too wide (%d pixels, max is %d)\n",
                        readlong(&fh.Width), LCD_WIDTH);
        close(fd);
        return -5;
    }

    /* Exit if too high */
    if (readlong(&fh.Height) > LCD_HEIGHT) {
        DEBUGF("error - Bitmap is too high (%d pixels, max is %d)\n",
                        readlong(&fh.Height), LCD_HEIGHT);
        close(fd);
        return -6;
    }

    /* Calculate image size */
    height = readlong(&fh.Height);
    width = readlong(&fh.Width);
    depth = readshort(&fh.BitCount);

    /* 4-byte boundary aligned */
    PaddedWidth = ((width * depth + 31) / 8) & ~3;

#if LCD_DEPTH > 1
        if(format == FORMAT_ANY) {
            if(depth == 1)
                format = FORMAT_MONO;
            else
                format = FORMAT_NATIVE;
        }
#endif

    /* PaddedHeight is for rockbox format. */
    if(format == FORMAT_MONO) {
        PaddedHeight = (height + 7) / 8;
        totalsize = PaddedHeight * width;
    } else {
#if LCD_DEPTH == 2
        PaddedHeight = (height + 3) / 4;
#else
        PaddedHeight = height;
#endif
        totalsize = PaddedHeight * width * sizeof(fb_data);
    }

    /* Check if this fits the buffer */
    
    if (totalsize > maxsize) {
        DEBUGF("error - Bitmap is too large to fit the supplied buffer: "
               "%d bytes.\n", (PaddedHeight * width));
        close(fd);
        return -7;
    }

    if (depth <= 8)
    {
        numcolors = readlong(&fh.ClrUsed);
        if (numcolors == 0)
            numcolors = 1 << depth;

        if(read(fd, palette, numcolors * sizeof(struct rgb_quad))
           != numcolors * (int)sizeof(struct rgb_quad))
        {
            DEBUGF("error - Can't read bitmap's color palette\n");
            close(fd);
            return -8;
        }
    }

    /* Use the darker palette color as foreground on mono bitmaps */
    if(readshort(&fh.BitCount) == 1) {
        if(brightness(palette[0]) > brightness(palette[1]))
           invert_pixel = 1;
    }
    
    /* Search to the beginning of the image data */
    lseek(fd, (off_t)readlong(&fh.OffBits), SEEK_SET);

#if LCD_DEPTH == 2
    if(format == FORMAT_NATIVE)
        memset(bitmap, 0, totalsize);
#endif
    
#if LCD_DEPTH > 1
    fb_data *dest = (fb_data *)bitmap;
#endif
    
    /* loop to read rows and put them to buffer */
    for (row = 0; row < height; row++) {
        unsigned char *p;
        
        /* read one row */
        ret = read(fd, bmpbuf, PaddedWidth);
        if (ret != PaddedWidth) {
            DEBUGF("error reading image, read returned: %d expected was: "
                   "%d\n", ret, PaddedWidth);
            close(fd);
            return -9;
        }

        switch(depth) {
        case 1:
#if LCD_DEPTH > 1
            if(format == FORMAT_MONO) {
#endif
                /* Mono -> Mono */
                for (col = 0; col < width; col++) {
                    ret = getpix(col, bmpbuf) ^ invert_pixel;
                    if (ret == 1) {
                        bitmap[width * ((height - row - 1) / 8) + col]
                            &= ~ 1 << ((height - row - 1) % 8);
                    } else {
                        bitmap[width * ((height - row - 1) / 8) + col]
                            |= 1 << ((height - row - 1) % 8);
                    }
                }
#if LCD_DEPTH == 2
            } else {
                /* Mono -> 2gray (iriver H1xx) */
                for (col = 0; col < width; col++) {
                    ret = brightness(palette[getpix(col, bmpbuf)]);
                    
                    if (ret > 96) {
                        bitmap[width * ((height - row - 1) / 8) + col]
                            &= ~ 1 << ((height - row - 1) % 8);
                    } else {
                        bitmap[width * ((height - row - 1) / 8) + col]
                            |= 1 << ((height - row - 1) % 8);
                    }
                }
            }
#elif LCD_DEPTH == 16
            } else {
                /* Mono -> RGB16 */
                for (col = 0; col < width; col++) {
                    ret = getpix(col, bmpbuf);
                    unsigned short rgb = (((palette[ret].red >> 3) << 11) |
                                          ((palette[ret].green >> 2) << 5) |
                                          ((palette[ret].blue >> 3)));
                    dest[width * (height - row - 1) + col] = rgb;
                }
            }
#endif
            break;

        case 24:
            p = bmpbuf;
#if LCD_DEPTH > 1
            if(format == FORMAT_MONO) {
#endif
                /* RGB24 -> mono */
                for (col = 0; col < width; col++) {
                    struct rgb_quad rgb;
                    rgb.red = p[2];
                    rgb.green = p[1];
                    rgb.blue = p[0];
                    ret = brightness(rgb);
                    if (ret > 96) {
                        bitmap[width * ((height - row - 1) / 8) + col]
                            &= ~ 1 << ((height - row - 1) % 8);
                    } else {
                        bitmap[width * ((height - row - 1) / 8) + col]
                            |= 1 << ((height - row - 1) % 8);
                    }
                    p += 3;
                }
#if LCD_DEPTH == 2
            } else {
                /* RGB24 -> 2gray (iriver H1xx) */
                for (col = 0; col < width; col++) {
                    struct rgb_quad rgb;
                    rgb.red = p[2];
                    rgb.green = p[1];
                    rgb.blue = p[0];
                    ret = brightness(rgb);

                    dest[((height - row - 1)/4) * width + col] |=
                        (~ret & 0xC0) >> (2 * (~(height - row - 1) & 3));
                    p += 3;
                }
            }
#elif LCD_DEPTH == 16
            } else {
                /* RGB24 -> RGB16 */
                for (col = 0; col < width; col++) {
                    unsigned short rgb = LCD_RGBPACK(p[2],p[1],p[0]);
                    dest[width * (height - row - 1) + col] = rgb;
                    p += 3;
                }
            }
#endif
            break;
        }
    }

    close(fd);

    /* returning image size: */
    bm->width = width;
    bm->height = height;
#if LCD_DEPTH > 1
    bm->format = format;
#endif

DEBUGF("totalsize: %d\n", totalsize);
    return totalsize; /* return the used buffer size. */
}
