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

#include "debug.h"
#include "lcd.h"
#include "file.h"

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


#ifdef LITTLE_ENDIAN
#define readshort(x) x
#define readlong(x) x

#else

/* Endian functions */
short readshort(void* value) {
    unsigned char* bytes = (unsigned char*) value;
    return bytes[0] | (bytes[1] << 8);
}

int readlong(void* value) {
    unsigned char* bytes = (unsigned char*) value;
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

#endif


/* Function to get a pixel from a line. (Tomas: maybe a better way?) */
int getpix(int px, unsigned char *bmpbuf) {
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
                  int *get_width,      /* in pixels */
                  int *get_height,     /* in pixels */
                  char *bitmap,
                  int maxsize) /* Maximum amount of bytes to write to bitmap */
{
    struct Fileheader fh;
    int bitmap_width, bitmap_height, PaddedWidth, PaddedHeight;
    int fd, row, col, ret;
    char bmpbuf[(LCD_WIDTH / 8 + 3) & ~3]; /* Buffer for one line */

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

    /* Exit if not monochrome */
    if (readshort(&fh.BitCount) != 1) {
        DEBUGF("error - Bitmap must be in 1 bit per pixel format. "
               "This one is: %d\n", readshort(&fh.BitCount));
        close(fd);
        return -4;
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
    bitmap_height = readlong(&fh.Height);
    bitmap_width = readlong(&fh.Width);
    /* Paddedwidth is for BMP files. */
    PaddedWidth = ((bitmap_width + 31) & (~0x1f)) / 8;
    /* PaddedHeight is for rockbox format. */
    PaddedHeight = (bitmap_height + 7) / 8;

    /* Check if this fits the buffer */
    if ((PaddedHeight * bitmap_width) > maxsize) {
        DEBUGF("error - Bitmap is too large to fit the supplied buffer: "
               "%d bytes.\n", (PaddedHeight * bitmap_width));
        close(fd);
        return -7;
    }

    /* Search to the beginning of the image data */
    lseek(fd, (off_t)readlong(&fh.OffBits), SEEK_SET);

    /* loop to read rows and put them to buffer */
    for (row = 0; row < bitmap_height; row++) {
        /* read one row */
        ret = read(fd, bmpbuf, PaddedWidth);
        if (ret != PaddedWidth) {
            DEBUGF("error reading image, read returned: %d expected was: "
                   "%d\n", ret, PaddedWidth);
            close(fd);
            return -8;
        }

        /* loop though the pixels in this line. */
        for (col = 0; col < bitmap_width; col++) {
            ret = getpix(col, bmpbuf);
            if (ret == 1) {
                bitmap[bitmap_width * ((bitmap_height - row - 1) / 8) + col]
                    &= ~ 1 << ((bitmap_height - row - 1) % 8);
            } else {
                bitmap[bitmap_width * ((bitmap_height - row - 1) / 8) + col]
                    |= 1 << ((bitmap_height - row - 1) % 8);
            }
        }
    }

    close(fd);

    /* returning image size: */
    *get_width = bitmap_width;
    *get_height = bitmap_height;

    return (PaddedHeight * bitmap_width); /* return the used buffer size. */

}
