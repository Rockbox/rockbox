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
2008-02-24 Jens Arnold: minimalistic version for charcell displays
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inttypes.h"
#include "debug.h"
#include "lcd.h"
#include "file.h"
#include "config.h"
#include "system.h"
#include "bmp.h"
#include "lcd.h"

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
 * Reads a BMP file and puts the data in rockbox format in *data.
 *
 *****************************************************************************/
int read_bmp_file(const char* filename,
                  unsigned char *bitmap,
                  int maxsize)
{
    struct bmp_header bmph;
    int fd, ret;
    int width, height, depth;
    int row, rowstart, rowstop, rowstep;
    unsigned char invert;
    unsigned char bmpbuf[4]; /* Buffer for one line */
    uint32_t palette[2];

    fd = open(filename, O_RDONLY);

    /* Exit if file opening failed */
    if (fd < 0) 
    {
        DEBUGF("read_bmp_file: can't open '%s', rc: %d\n", filename, fd);
        return fd * 10 - 1;
    }

    /* read fileheader */
    ret = read(fd, &bmph, sizeof(struct bmp_header));
    if (ret < 0)
        return ret * 10 - 2;

    if (ret != sizeof(struct bmp_header)) {
        DEBUGF("read_bmp_file: can't read BMP header.");
        return -3;
    }

    width = readlong(&bmph.width);
    if (width > 8) 
    {
        DEBUGF("read_bmp_file: Bitmap too wide (%d pixels, max is 8)\n", width);
        return -4;
    }

    depth = readshort(&bmph.bit_count);
    if (depth != 1) 
    {
        DEBUGF("read_bmp_fd: Wrong depth (%d, must be 1)\n", depth);
        return -5;
    }

    height = readlong(&bmph.height);
    if (height < 0) 
    {   /* Top-down BMP file */
        height = -height;
        rowstart = 0;
        rowstop = height;
        rowstep = 1;
    } 
    else 
    {   /* normal BMP */
        rowstart = height - 1;
        rowstop = -1;
        rowstep = -1;
    }

    /* Check if this fits the buffer */
    if (height > maxsize) 
    {
        DEBUGF("read_bmp_fd: Bitmap too high for buffer: %d bytes.\n", height);
        return -6;
    }

    if (read(fd, palette, 2 * sizeof(uint32_t))
        != 2 * (int)sizeof(uint32_t))
    {
        DEBUGF("read_bmp_fd: Can't read color palette\n");
        return -7;
    }
    invert = (brightness((union rgb_union)palette[1]) 
             > brightness((union rgb_union)palette[0]))
             ? 0xff : 0x00;

    /* Search to the beginning of the image data */
    lseek(fd, (off_t)readlong(&bmph.off_bits), SEEK_SET);
    memset(bitmap, 0, height);

    /* loop to read rows and put them to buffer */
    for (row = rowstart; row != rowstop; row += rowstep)
    {
        /* read one row */
        ret = read(fd, bmpbuf, 4);
        if (ret != 4) 
        {
            DEBUGF("read_bmp_fd: error reading image, read returned: %d "
                   "expected: 4\n", ret);
            return -9;
        }
        bitmap[row] = bmpbuf[0] ^ invert;
    }

    DEBUGF("totalsize: %d\n", totalsize);

    close(fd);
    return height; /* return the used buffer size. */
}
