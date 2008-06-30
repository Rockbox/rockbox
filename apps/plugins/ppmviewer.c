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
#include "bmp.h"

#if defined(HAVE_LCD_COLOR)

PLUGIN_HEADER

/* Magic constants. */
#define PPM_MAGIC1 'P'
#define PPM_MAGIC2 '3'
#define RPPM_MAGIC2 '6'
#define PPM_FORMAT (PPM_MAGIC1 * 256 + PPM_MAGIC2)
#define RPPM_FORMAT (PPM_MAGIC1 * 256 + RPPM_MAGIC2)

#define PPM_OVERALLMAXVAL 65535
#define PPM_MAXSIZE (300*1024)/sizeof(fb_data)

#define ppm_error(...) rb->splash(HZ*2, __VA_ARGS__ )

static fb_data buffer[PPM_MAXSIZE];
static fb_data lcd_buf[LCD_WIDTH * LCD_HEIGHT];

static const struct plugin_api* rb; /* global api struct pointer */

int ppm_read_magic_number(int fd)
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

char ppm_getc(int fd)
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

int ppm_getuint(int fd)
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

int ppm_getrawbyte(int fd)
{
    unsigned char by;

    if (!rb->read(fd, &by, 1)) {
        ppm_error("EOF. Read error while reading a one-byte sample.");
        return PLUGIN_ERROR;
    }

    return (int)by;
}

int ppm_getrawsample(int fd, int const maxval)
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

int read_ppm_init_rest(int fd, 
                       int * const cols, 
                       int * const rows, 
                       int * const maxval)
{
    /* Read size. */
    *cols = ppm_getuint(fd);
    *rows = ppm_getuint(fd);

    if ((long unsigned int)(*cols * *rows) > PPM_MAXSIZE) {
        ppm_error("Imagesize (%ld pixels) is too large. "\
                  "The maximum allowed is %ld.",
                  (long unsigned int)(*cols * *rows), 
                  (long unsigned int)PPM_MAXSIZE);
        return PLUGIN_ERROR;
    }

    /* Read maxval. */
    *maxval = ppm_getuint(fd);
    
    if (*maxval > PPM_OVERALLMAXVAL) {
        ppm_error("maxval of input image (%u) is too large. "\
                  "The maximum allowed by the PPM is %u.",
                  *maxval, PPM_OVERALLMAXVAL);
        return PLUGIN_ERROR;
    }
    if (*maxval == 0) {
        ppm_error("maxval of input image is zero.");
        return PLUGIN_ERROR;
    }
    return 1;
}

void read_ppm_init(int fd, 
                   int * const cols, 
                   int * const rows, 
                   int * const maxval, 
                   int * const format)
{
    /* Check magic number. */
    *format = ppm_read_magic_number( fd );
    
    if (*format == PLUGIN_ERROR) return;
    switch (*format) {
        case PPM_FORMAT:
        case RPPM_FORMAT:
            if(read_ppm_init_rest(fd, cols, rows, maxval) == PLUGIN_ERROR) {
                *format = PLUGIN_ERROR;
            }
            break;

        default:
            ppm_error( "Bad magic number - not a ppm or rppm file." );
            *format = PLUGIN_ERROR;
    }
}

int read_ppm_row(int fd, 
                 int const row, 
                 int const cols, 
                 int const maxval, 
                 int const format)
{
    int col;
    int r, g, b;
    switch (format) {
	    case PPM_FORMAT:
            for (col = 0; col < cols; ++col) {
                r = ppm_getuint(fd);
                g = ppm_getuint(fd);
                b = ppm_getuint(fd);

                if (r == PLUGIN_ERROR || g == PLUGIN_ERROR ||
                    b == PLUGIN_ERROR)
                {
                    return PLUGIN_ERROR;
                } 
                buffer[(cols * row) + col] = LCD_RGBPACK(
                    (255 / maxval) * r,
                    (255 / maxval) * g,
                    (255 / maxval) * b);
            }
            break;

	    case RPPM_FORMAT:
            for (col = 0; col < cols; ++col) {
                r = ppm_getrawsample(fd, maxval);
                g = ppm_getrawsample(fd, maxval);
                b = ppm_getrawsample(fd, maxval);

                if (r == PLUGIN_ERROR || g == PLUGIN_ERROR ||
                    b == PLUGIN_ERROR)
                {
                    return PLUGIN_ERROR;
                } 
                buffer[(cols * row) + col] = LCD_RGBPACK(
                    (255 / maxval) * r,
                    (255 / maxval) * g,
                    (255 / maxval) * b);
	        }
	        break;

	    default:
            ppm_error("What?!");
	        return PLUGIN_ERROR;
	}
	return 1;
}

int read_ppm(int fd, 
             int * const cols, 
             int * const rows, 
             int * const maxval)
{
    int row;
    int format;

    read_ppm_init(fd, cols, rows, maxval, &format);
    
    if(format == PLUGIN_ERROR) {
        return PLUGIN_ERROR;
    }
    
    for (row = 0; row < *rows; ++row) {
        if( read_ppm_row(fd, row, *cols, *maxval, format) == PLUGIN_ERROR) {
            return PLUGIN_ERROR;
        }
    }
    return 1;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    static char filename[MAX_PATH];
    int fd;

    int cols; 
    int rows; 
    int maxval; 
   
    int result;
    
    struct bitmap small_bitmap, orig_bitmap;
    
    if(!parameter) return PLUGIN_ERROR;

    rb = api;

    rb->strcpy(filename, parameter);
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        ppm_error("Couldnt open file: %s, %d", filename, fd);
        return PLUGIN_ERROR;
    }

    result = read_ppm(fd, &cols, &rows, &maxval);

    rb->close(fd);
    if(result == PLUGIN_ERROR) return PLUGIN_ERROR;

    orig_bitmap.width = cols;
    orig_bitmap.height = rows;
    orig_bitmap.data = (char*)buffer;
    
    if (cols > LCD_WIDTH || rows > LCD_HEIGHT)
    {
        if (cols > LCD_WIDTH) {
            small_bitmap.width = LCD_WIDTH;
            small_bitmap.height = 
                (int)(((float)LCD_WIDTH / (float)cols) * (float)rows);
            
        } else { /* rows > LCD_HEIGHT */
            
            small_bitmap.width = 
                (int)(((float)LCD_HEIGHT / (float)rows) * (float)cols);
            small_bitmap.height = LCD_HEIGHT;            
        } 
        small_bitmap.data = (char*)lcd_buf;

        smooth_resize_bitmap( &orig_bitmap, &small_bitmap );
        
        rb->lcd_bitmap((fb_data*)small_bitmap.data, 0, 0,
                       small_bitmap.width, small_bitmap.height);
    } else {
        rb->lcd_bitmap((fb_data*)orig_bitmap.data, 0, 0, cols, rows);   
    }
    rb->lcd_update();
    rb->button_get(true);
    
    return PLUGIN_OK;
}

#endif
