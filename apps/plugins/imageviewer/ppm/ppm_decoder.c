/*****************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Alexander Papst
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

#include "plugin.h"
#include "lib/pluginlib_bmp.h"
#include "ppm_decoder.h"

static int ppm_read_magic_number(int fd)
{
    char i1, i2;
    if(!rb->read(fd, &i1, 1) || !rb->read(fd, &i2, 1))
    {
        ppm_error( "Error reading magic number from ppm image stream. "\
                   "Most often, this means your input file is empty." );
        return PLUGIN_ERROR;
    }        
    return i1 * 256 + i2;
}

static char ppm_getc(int fd)
{
    char ch;

    if (!rb->read(fd, &ch, 1)) {
        ppm_error("EOF. Read error reading a byte");
        return PLUGIN_ERROR;
    }
       
    if (ch == '#') {
        do {
            if (!rb->read(fd, &ch, 1)) {
                ppm_error("EOF. Read error reading a byte");
                return PLUGIN_ERROR;
            }
        } while (ch != '\n' && ch != '\r');
    }
    return ch;
}

static int ppm_getuint(int fd)
{
    char ch;
    int i;
    int digitVal;

    do {
        ch = ppm_getc(fd);
    } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

    if (ch < '0' || ch > '9') {
        ppm_error("Junk (%c) in file where an integer should be.", ch);
        return PLUGIN_ERROR;
    }

    i = 0;

    do {
        digitVal = ch - '0';

        if (i > INT_MAX/10 - digitVal) {
            ppm_error("ASCII decimal integer in file is "\
                      "too large to be processed.");
            return PLUGIN_ERROR;
        }

        i = i * 10 + digitVal;
        ch = ppm_getc(fd);
        
    } while (ch >= '0' && ch <= '9');

    return i;
}

static int ppm_getrawbyte(int fd)
{
    unsigned char by;

    if (!rb->read(fd, &by, 1)) {
        ppm_error("EOF. Read error while reading a one-byte sample.");
        return PLUGIN_ERROR;
    }

    return (int)by;
}

static int ppm_getrawsample(int fd, int const maxval)
{
    if (maxval < 256) {
        /* The sample is just one byte. Read it. */
        return(ppm_getrawbyte(fd));
    } else {
        /* The sample is two bytes. Read both. */
        unsigned char byte_pair[2];

        if (!rb->read(fd, byte_pair, 2)) {
            ppm_error("EOF. Read error while reading a long sample.");
            return PLUGIN_ERROR;
        }
        return((byte_pair[0]<<8) | byte_pair[1]);
    }
}

/* Read from the file header dimensions as well as max
 * int value used
 */
static int read_ppm_init_rest(int fd, struct ppm_info *ppm)
{
    /* Read size. */
    ppm->x = ppm_getuint(fd);
    ppm->y = ppm_getuint(fd);

#ifdef HAVE_LCD_COLOR
    ppm->native_img_size = ppm->x * ppm->y * FB_DATA_SZ;
#endif

    if (ppm->native_img_size > ppm->buf_size) {
        ppm_error("Imagesize (%ld pixels) is too large. "\
                  "The maximum allowed is %ld.",
                  (long)ppm->native_img_size, 
                  (long)ppm->buf_size);
        return PLUGIN_ERROR;
    }

    /* Read maxval. */
    ppm->maxval = ppm_getuint(fd);
    
    if (ppm->maxval > PPM_OVERALLMAXVAL) {
        ppm_error("maxval of input image (%u) is too large. "\
                  "The maximum allowed by the PPM is %u.",
                  ppm->maxval, PPM_OVERALLMAXVAL);
        return PLUGIN_ERROR;
    }
    if (ppm->maxval == 0) {
        ppm_error("maxval of input image is zero.");
        return PLUGIN_ERROR;
    }
    return 1;
}

static void read_ppm_init(int fd, struct ppm_info *ppm)
{
    /* Check magic number. */
    ppm->format = ppm_read_magic_number( fd );
    
    if (ppm->format == PLUGIN_ERROR) return;
    switch (ppm->format) {
        case PPM_FORMAT:
        case RPPM_FORMAT:
            if(read_ppm_init_rest(fd, ppm) == PLUGIN_ERROR) {
                ppm->format = PLUGIN_ERROR;
            }
            break;

        default:
            ppm_error( "Bad magic number - not a ppm or rppm file." );
            ppm->format = PLUGIN_ERROR;
    }
}

#if   defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define BUFADDR(x, y, width, height) ( ppm->buf + (height*(x) + (y))*FB_DATA_SZ)
#else
#define BUFADDR(x, y, width, height) ( ppm->buf + (width*(y) + (x))*FB_DATA_SZ)
#endif

static int read_ppm_row(int fd, struct ppm_info *ppm, int row) 
{

    int col;
    int r, g, b;
    switch (ppm->format) {
        case PPM_FORMAT:
            for (col = 0; col < ppm->x; ++col) {
                r = ppm_getuint(fd);
                g = ppm_getuint(fd);
                b = ppm_getuint(fd);

                if (r == PLUGIN_ERROR || g == PLUGIN_ERROR ||
                    b == PLUGIN_ERROR)
                {
                    return PLUGIN_ERROR;
                } 
                *(fb_data *)BUFADDR(col, row, ppm->x, ppm->y) = LCD_RGBPACK(
                    (255 * r)/ppm->maxval,
                    (255 * g)/ppm->maxval,
                    (255 * b)/ppm->maxval);
            }
            break;

        case RPPM_FORMAT:
            for (col = 0; col < ppm->x; ++col) {
                r = ppm_getrawsample(fd, ppm->maxval);
                g = ppm_getrawsample(fd, ppm->maxval);
                b = ppm_getrawsample(fd, ppm->maxval);

                if (r == PLUGIN_ERROR || g == PLUGIN_ERROR ||
                    b == PLUGIN_ERROR)
                {
                    return PLUGIN_ERROR;
                }
                *(fb_data *)BUFADDR(col, row, ppm->x, ppm->y) = LCD_RGBPACK(
                    (255 * r)/ppm->maxval,
                    (255 * g)/ppm->maxval,
                    (255 * b)/ppm->maxval);
            }
            break;

        default:
            ppm_error("What?!");
            return PLUGIN_ERROR;
    }
    return 1;
}

/* public */
int read_ppm(int fd, struct ppm_info *ppm)
{
    int row;

    read_ppm_init(fd, ppm);
    
    if(ppm->format == PLUGIN_ERROR) {
        return PLUGIN_ERROR;
    }
    
    for (row = 0; row < ppm->y; ++row) {
        if( read_ppm_row(fd, ppm, row) == PLUGIN_ERROR) {
            return PLUGIN_ERROR;
        }
    }
    return 1;
}
