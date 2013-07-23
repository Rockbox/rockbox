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
/****************************************************************************
 *
 * Converts BMP files to Rockbox bitmap format
 *
 * 1999-05-03 Linus Nielsen Feltzing
 *
 * 2005-07-06 Jens Arnold
 *            added reading of 4, 16, 24 and 32 bit bmps
 *            added 2 new target formats (playergfx and iriver 4-grey)
 * 
 * 2013-08-29 Lorenzo Miori
 *            BGR 24 bit conversion support only (stripped)
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define debugf printf

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#else
#define STRUCT_PACKED
#pragma pack (push, 2)
#endif

struct Fileheader
{
    unsigned short Type;          /* signature - 'BM' */
    unsigned int  Size;          /* file size in bytes */
    unsigned short Reserved1;     /* 0 */
    unsigned short Reserved2;     /* 0 */
    unsigned int  OffBits;       /* offset to bitmap */
    unsigned int  StructSize;    /* size of this struct (40) */
    unsigned int  Width;         /* bmap width in pixels */
    unsigned int  Height;        /* bmap height in pixels */
    unsigned short Planes;        /* num planes - always 1 */
    unsigned short BitCount;      /* bits per pixel */
    unsigned int  Compression;   /* compression flag */
    unsigned int  SizeImage;     /* image size in bytes */
    int           XPelsPerMeter; /* horz resolution */
    int           YPelsPerMeter; /* vert resolution */
    unsigned int  ClrUsed;       /* 0 -> color table size */
    unsigned int  ClrImportant;  /* important color count */
} STRUCT_PACKED;

struct RGBQUAD
{
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    unsigned char rgbReserved;
} STRUCT_PACKED;

short readshort(void* value)
{
    unsigned char* bytes = (unsigned char*) value;
    return bytes[0] | (bytes[1] << 8);
}

int readint(void* value)
{
    unsigned char* bytes = (unsigned char*) value;
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

unsigned char brightness(struct RGBQUAD color)
{
    return (3 * (unsigned int)color.rgbRed + 6 * (unsigned int)color.rgbGreen
              + (unsigned int)color.rgbBlue) / 10;
}

#ifndef O_BINARY
#define O_BINARY 0 /* systems that don't have O_BINARY won't make a difference
                      on text and binary files */
#endif

/****************************************************************************
 * read_bmp_file()
 *
 * Reads an uncompressed BMP file and puts the data in a 4-byte-per-pixel
 * (RGBQUAD) array. Returns 0 on success.
 *
 ***************************************************************************/

int read_bmp_file(char* filename,
                  int *get_width,  /* in pixels */
                  int *get_height, /* in pixels */
                  struct RGBQUAD **bitmap)
{
    struct Fileheader fh;
    struct RGBQUAD palette[256];

    int fd = open(filename, O_RDONLY| O_BINARY);
    unsigned short data;
    unsigned char *bmp;
    int width, height;
    int padded_width;
    int size;
    int row, col, i;
    int numcolors, compression;
    int depth;

    if (fd == -1)
    {
        debugf("error - can't open '%s'\n", filename);
        return 1;
    }
    if (read(fd, &fh, sizeof(struct Fileheader)) !=
        sizeof(struct Fileheader))
    {
        debugf("error - can't Read Fileheader Stucture\n");
        close(fd);
        return 2;
    }

    compression = readint(&fh.Compression);

    if (compression != 0)
    {
        debugf("error - Unsupported compression %d\n", compression);
        close(fd);
        return 3;
    }

    depth = readshort(&fh.BitCount);

    if (depth <= 8)
    {
        numcolors = readint(&fh.ClrUsed);
        if (numcolors == 0)
            numcolors = 1 << depth;

        if (read(fd, &palette[0], numcolors * sizeof(struct RGBQUAD))
            != (int)(numcolors * sizeof(struct RGBQUAD)))
        {
            debugf("error - Can't read bitmap's color palette\n");
            close(fd);
            return 4;
        }
    }

    width = readint(&fh.Width);
    height = readint(&fh.Height);
    padded_width = ((width * depth + 31) / 8) & ~3; /* aligned 4-bytes boundaries */

    size = padded_width * height; /* read this many bytes */
    bmp = (unsigned char *)malloc(size);
    *bitmap = (struct RGBQUAD *)malloc(width * height * sizeof(struct RGBQUAD));

    if ((bmp == NULL) || (*bitmap == NULL))
    {
        debugf("error - Out of memory\n");
        close(fd);
        return 5;
    }

    if (lseek(fd, (off_t)readint(&fh.OffBits), SEEK_SET) < 0)
    {
        debugf("error - Can't seek to start of image data\n");
        close(fd);
        return 6;
    }
    if (read(fd, (unsigned char*)bmp, (int)size) != size)
    {
        debugf("error - Can't read image\n");
        close(fd);
        return 7;
    }

    close(fd);
    *get_width = width;
    *get_height = height;

    switch (depth)
    {
      case 1:
        for (row = 0; row < height; row++)
            for (col = 0; col < width; col++)
            {
                data = (bmp[(height - 1 - row) * padded_width + col / 8]
                        >> (~col & 7)) & 1;
                (*bitmap)[row * width + col] = palette[data];
            }
        break;

      case 4:
        for (row = 0; row < height; row++)
            for (col = 0; col < width; col++)
            {
                data = (bmp[(height - 1 - row) * padded_width + col / 2]
                        >> (4 * (~col & 1))) & 0x0F;
                (*bitmap)[row * width + col] = palette[data];
            }
        break;

      case 8:
        for (row = 0; row < height; row++)
            for (col = 0; col < width; col++)
            {
                data = bmp[(height - 1 - row) * padded_width + col];
                (*bitmap)[row * width + col] = palette[data];
            }
        break;

      case 16:
        for (row = 0; row < height; row++)
            for (col = 0; col < width; col++)
            {
                data = readshort(&bmp[(height - 1 - row) * padded_width + 2 * col]);
                (*bitmap)[row * width + col].rgbRed =
                    ((data >> 7) & 0xF8) | ((data >> 12) & 0x07);
                (*bitmap)[row * width + col].rgbGreen =
                    ((data >> 2) & 0xF8) | ((data >> 7) & 0x07);
                (*bitmap)[row * width + col].rgbBlue =
                    ((data << 3) & 0xF8) | ((data >> 2) & 0x07);
                (*bitmap)[row * width + col].rgbReserved = 0;
            }
        break;

      case 24:
        for (row = 0; row < height; row++)
            for (col = 0; col < width; col++)
            {
                i = (height - 1 - row) * padded_width + 3 * col;
                (*bitmap)[row * width + col].rgbRed = bmp[i+2];
                (*bitmap)[row * width + col].rgbGreen = bmp[i+1];
                (*bitmap)[row * width + col].rgbBlue = bmp[i];
                (*bitmap)[row * width + col].rgbReserved = 0;
            }
        break;

      case 32:
        for (row = 0; row < height; row++)
            for (col = 0; col < width; col++)
            {
                i = (height - 1 - row) * padded_width + 4 * col;
                (*bitmap)[row * width + col].rgbRed = bmp[i+2];
                (*bitmap)[row * width + col].rgbGreen = bmp[i+1];
                (*bitmap)[row * width + col].rgbBlue = bmp[i];
                (*bitmap)[row * width + col].rgbReserved = 0;
            }
        break;

      default: /* should never happen */
        debugf("error - Unsupported bitmap depth %d.\n", depth);
        return 8;
    }

    free(bmp);

    return 0; /* success */
}

void generate_raw_file(struct RGBQUAD *t_bitmap, 
                       int t_width, int t_height, int t_depth)
{
    FILE *f;
    int i, a;
    unsigned char lo,hi;

    f = stdout;

    for (i = 0; i < t_height; i++)
    {
        for (a = 0; a < t_width; a++)
        {
            fwrite(&(t_bitmap->rgbBlue), 1,1,f);
            fwrite(&(t_bitmap->rgbGreen), 1,1,f);
            fwrite(&(t_bitmap->rgbRed), 1,1,f);
            t_bitmap++;
        }
    }
}

void print_usage(void)
{
    printf("Usage: bmp2raw <bitmap file>\n");
}

int main(int argc, char **argv)
{
    struct RGBQUAD *bitmap = NULL;
    unsigned char *bmp_filename = NULL;
    int width = 0;
    int height = 0;

    if (argc > 1)
    {
        bmp_filename = argv[1];
    }
    else
    {
        print_usage();
        exit(1);
    }

    if (!bmp_filename)
    {
        print_usage();
        exit(1);
    }

    if (read_bmp_file(bmp_filename, &width, &height, &bitmap))
        exit(1);

    generate_raw_file(bitmap, width, height, 24);

    return 0;
}
