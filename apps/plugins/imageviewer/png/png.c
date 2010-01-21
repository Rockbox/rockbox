/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$id $
 *
 * Copyright (C) 2009 by Christophe Gouiran <bechris13250 -at- gmail -dot- com>
 *
 * Based on lodepng, a lightweight png decoder/encoder
 * (c) 2005-2008 Lode Vandevenne
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
LodePNG version 20080927

Copyright (c) 2005-2008 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

/*
The manual and changelog can be found in the header file "lodepng.h"
You are free to name this file lodepng.cpp or lodepng.c depending on your usage.
*/

#include "plugin.h"
#include "lcd.h"
#include <lib/pluginlib_bmp.h>
#include "zlib.h"
#include "png.h"

/* ////////////////////////////////////////////////////////////////////////// */
/* LodeFlate & LodeZlib Setting structs                                       */
/* ////////////////////////////////////////////////////////////////////////// */

typedef struct LodePNG_InfoColor /*info about the color type of an image*/
{
    /*header (IHDR)*/
    unsigned colorType; /*color type*/
    unsigned bitDepth;  /*bits per sample*/

    /*palette (PLTE)*/
    unsigned char palette[256 * 4]; /*palette in RGBARGBA... order*/
    size_t palettesize; /*palette size in number of colors (amount of bytes is 4 * palettesize)*/

    /*transparent color key (tRNS)*/
    unsigned key_defined; /*is a transparent color key given?*/
    unsigned key_r;       /*red component of color key*/
    unsigned key_g;       /*green component of color key*/
    unsigned key_b;       /*blue component of color key*/
} LodePNG_InfoColor;

typedef struct LodePNG_Time /*LodePNG's encoder does not generate the current time. To make it add a time chunk the correct time has to be provided*/
{
    unsigned      year;    /*2 bytes*/
    unsigned char month;   /*1-12*/
    unsigned char day;     /*1-31*/
    unsigned char hour;    /*0-23*/
    unsigned char minute;  /*0-59*/
    unsigned char second;  /*0-60 (to allow for leap seconds)*/
} LodePNG_Time;

typedef struct LodePNG_InfoPng /*information about the PNG image, except pixels and sometimes except width and height*/
{
    /*header (IHDR), palette (PLTE) and transparency (tRNS)*/
    unsigned width;             /*width of the image in pixels (ignored by encoder, but filled in by decoder)*/
    unsigned height;            /*height of the image in pixels (ignored by encoder, but filled in by decoder)*/
    unsigned compressionMethod; /*compression method of the original file*/
    unsigned filterMethod;      /*filter method of the original file*/
    unsigned interlaceMethod;   /*interlace method of the original file*/
    LodePNG_InfoColor color;    /*color type and bits, palette, transparency*/

    /*suggested background color (bKGD)*/
    unsigned background_defined; /*is a suggested background color given?*/
    unsigned background_r;       /*red component of suggested background color*/
    unsigned background_g;       /*green component of suggested background color*/
    unsigned background_b;       /*blue component of suggested background color*/

    /*time chunk (tIME)*/
    unsigned char time_defined; /*if 0, no tIME chunk was or will be generated in the PNG image*/
    LodePNG_Time time;

    /*phys chunk (pHYs)*/
    unsigned      phys_defined; /*is pHYs chunk defined?*/
    unsigned      phys_x;
    unsigned      phys_y;
    unsigned char phys_unit; /*may be 0 (unknown unit) or 1 (metre)*/

} LodePNG_InfoPng;

typedef struct LodePNG_InfoRaw /*contains user-chosen information about the raw image data, which is independent of the PNG image*/
{
    LodePNG_InfoColor color;
} LodePNG_InfoRaw;

typedef struct LodePNG_DecodeSettings
{
    unsigned color_convert; /*whether to convert the PNG to the color type you want. Default: yes*/
} LodePNG_DecodeSettings;

typedef struct LodePNG_Decoder
{
    LodePNG_DecodeSettings settings;
    LodePNG_InfoRaw infoRaw;
    LodePNG_InfoPng infoPng; /*info of the PNG image obtained after decoding*/
    long error;
    char error_msg[128];
} LodePNG_Decoder;

#define VERSION_STRING "20080927"

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
static fb_data *disp[9];
/* up to here currently used by image(s) */
static fb_data *disp_buf;

/* my memory pool (from the mp3 buffer) */
static char print[128]; /* use a common snprintf() buffer */

unsigned char *memory, *memory_max; /* inffast.c needs memory_max */
static size_t memory_size;

static unsigned char *image; /* where we put the content of the file */
static size_t image_size;

static fb_data *converted_image; /* the (color) converted image */
static size_t converted_image_size;

static unsigned char *decoded_image; /* the decoded image */
static size_t decoded_image_size;

static LodePNG_Decoder _decoder;

/*
The two functions below (LodePNG_decompress and LodePNG_compress) directly call the
LodeZlib_decompress and LodeZlib_compress functions. The only purpose of the functions
below, is to provide the ability to let LodePNG use a different Zlib encoder by only
changing the two functions below, instead of changing it inside the vareous places
in the other LodePNG functions.

*out must be NULL and *outsize must be 0 initially, and after the function is done,
*out must point to the decompressed data, *outsize must be the size of it, and must
be the size of the useful data in bytes, not the alloc size.
*/

static unsigned LodePNG_decompress(unsigned char* out, size_t* outsize, const unsigned char* in, size_t insize, char *error_msg)
{
    z_stream stream;
    int err;

    rb->strcpy(error_msg, "");

    stream.next_in = (Bytef*)in;
    stream.avail_in = (uInt)insize;

    stream.next_out = out;
    stream.avail_out = (uInt)*outsize;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    err = inflateInit(&stream);
    if (err != Z_OK) return err;

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
            return Z_DATA_ERROR;
        return err;
    }
    *outsize = stream.total_out;

    err = inflateEnd(&stream);
    if (stream.msg != Z_NULL)
        rb->strcpy(error_msg, stream.msg);
    return err;

}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Reading and writing single bits and bytes from/to stream for LodePNG   / */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned char readBitFromReversedStream(size_t* bitpointer, const unsigned char* bitstream)
{
    unsigned char result = (unsigned char)((bitstream[(*bitpointer) >> 3] >> (7 - ((*bitpointer) & 0x7))) & 1);
    (*bitpointer)++;
    return result;
}

static unsigned readBitsFromReversedStream(size_t* bitpointer, const unsigned char* bitstream, size_t nbits)
{
    unsigned result = 0;
    size_t i;
    for (i = nbits - 1; i < nbits; i--) result += (unsigned)readBitFromReversedStream(bitpointer, bitstream) << i;
    return result;
}

static void setBitOfReversedStream0(size_t* bitpointer, unsigned char* bitstream, unsigned char bit)
{
    /*the current bit in bitstream must be 0 for this to work*/
    if (bit) bitstream[(*bitpointer) >> 3] |=  (bit << (7 - ((*bitpointer) & 0x7))); /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/
    (*bitpointer)++;
}

static void setBitOfReversedStream(size_t* bitpointer, unsigned char* bitstream, unsigned char bit)
{
    /*the current bit in bitstream may be 0 or 1 for this to work*/
    if (bit == 0) bitstream[(*bitpointer) >> 3] &=  (unsigned char)(~(1 << (7 - ((*bitpointer) & 0x7))));
    else bitstream[(*bitpointer) >> 3] |=  (1 << (7 - ((*bitpointer) & 0x7)));
    (*bitpointer)++;
}

static unsigned LodePNG_read32bitInt(const unsigned char* buffer)
{
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG chunks                                                             / */
/* ////////////////////////////////////////////////////////////////////////// */

unsigned LodePNG_chunk_length(const unsigned char* chunk) /*get the length of the data of the chunk. Total chunk length has 12 bytes more.*/
{
    return LodePNG_read32bitInt(&chunk[0]);
}

void LodePNG_chunk_type(char type[5], const unsigned char* chunk) /*puts the 4-byte type in null terminated string*/
{
    unsigned i;
    for (i = 0; i < 4; i++) type[i] = chunk[4 + i];
    type[4] = 0; /*null termination char*/
}

unsigned char LodePNG_chunk_type_equals(const unsigned char* chunk, const char* type) /*check if the type is the given type*/
{
    if (type[4] != 0) return 0;
    return (chunk[4] == type[0] && chunk[5] == type[1] && chunk[6] == type[2] && chunk[7] == type[3]);
}

/*properties of PNG chunks gotten from capitalization of chunk type name, as defined by the standard*/
unsigned char LodePNG_chunk_critical(const unsigned char* chunk) /*0: ancillary chunk, 1: it's one of the critical chunk types*/
{
    return((chunk[4] & 32) == 0);
}

unsigned char LodePNG_chunk_private(const unsigned char* chunk) /*0: public, 1: private*/
{
    return((chunk[6] & 32) != 0);
}

unsigned char LodePNG_chunk_safetocopy(const unsigned char* chunk) /*0: the chunk is unsafe to copy, 1: the chunk is safe to copy*/
{
    return((chunk[7] & 32) != 0);
}

unsigned char* LodePNG_chunk_data(unsigned char* chunk) /*get pointer to the data of the chunk*/
{
    return &chunk[8];
}

const unsigned char* LodePNG_chunk_data_const(const unsigned char* chunk) /*get pointer to the data of the chunk*/
{
    return &chunk[8];
}

unsigned LodePNG_chunk_check_crc(const unsigned char* chunk) /*returns 0 if the crc is correct, error code if it's incorrect*/
{
    unsigned length = LodePNG_chunk_length(chunk);
    unsigned CRC = LodePNG_read32bitInt(&chunk[length + 8]);
    unsigned checksum = crc32(0L, &chunk[4], length + 4); /*the CRC is taken of the data and the 4 chunk type letters, not the length*/
    if (CRC != checksum) return 1;
    else return 0;
}

unsigned char* LodePNG_chunk_next(unsigned char* chunk) /*don't use on IEND chunk, as there is no next chunk then*/
{
    unsigned total_chunk_length = LodePNG_chunk_length(chunk) + 12;
    return &chunk[total_chunk_length];
}

const unsigned char* LodePNG_chunk_next_const(const unsigned char* chunk) /*don't use on IEND chunk, as there is no next chunk then*/
{
    unsigned total_chunk_length = LodePNG_chunk_length(chunk) + 12;
    return &chunk[total_chunk_length];
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Color types and such                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

/*return type is a LodePNG error code*/
static unsigned checkColorValidity(unsigned colorType, unsigned bd) /*bd = bitDepth*/
{
    switch (colorType)
    {
    case 0:
        if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; break; /*grey*/
    case 2:
        if (!(                                 bd == 8 || bd == 16)) return 37; break; /*RGB*/
    case 3:
        if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8            )) return 37; break; /*palette*/
    case 4:
        if (!(                                 bd == 8 || bd == 16)) return 37; break; /*grey + alpha*/
    case 6:
        if (!(                                 bd == 8 || bd == 16)) return 37; break; /*RGBA*/
    default:
        return 31;
    }
    return 0; /*allowed color type / bits combination*/
}

static unsigned getNumColorChannels(unsigned colorType)
{
    switch (colorType)
    {
    case 0:
        return 1; /*grey*/
    case 2:
        return 3; /*RGB*/
    case 3:
        return 1; /*palette*/
    case 4:
        return 2; /*grey + alpha*/
    case 6:
        return 4; /*RGBA*/
    }
    return 0; /*unexisting color type*/
}

static unsigned getBpp(unsigned colorType, unsigned bitDepth)
{
    return getNumColorChannels(colorType) * bitDepth; /*bits per pixel is amount of channels * bits per channel*/
}

/* ////////////////////////////////////////////////////////////////////////// */

void LodePNG_InfoColor_init(LodePNG_InfoColor* info)
{
    info->key_defined = 0;
    info->key_r = info->key_g = info->key_b = 0;
    info->colorType = 6;
    info->bitDepth = 8;
    memset(info->palette, 0, 256 * 4 * sizeof(unsigned char));
    info->palettesize = 0;
}

void LodePNG_InfoColor_cleanup(LodePNG_InfoColor* info)
{
    info->palettesize = 0;
}

unsigned LodePNG_InfoColor_getBpp(const LodePNG_InfoColor* info) { return getBpp(info->colorType, info->bitDepth); } /*calculate bits per pixel out of colorType and bitDepth*/
unsigned LodePNG_InfoColor_isGreyscaleType(const LodePNG_InfoColor* info) { return info->colorType == 0 || info->colorType == 4; }

unsigned LodePNG_InfoColor_equal(const LodePNG_InfoColor* info1, const LodePNG_InfoColor* info2)
{
    return info1->colorType == info2->colorType
           && info1->bitDepth  == info2->bitDepth; /*palette and color key not compared*/
}

void LodePNG_InfoPng_init(LodePNG_InfoPng* info)
{
    info->width = info->height = 0;
    LodePNG_InfoColor_init(&info->color);
    info->interlaceMethod = 0;
    info->compressionMethod = 0;
    info->filterMethod = 0;
    info->background_defined = 0;
    info->background_r = info->background_g = info->background_b = 0;

    info->time_defined = 0;
    info->phys_defined = 0;
}

void LodePNG_InfoPng_cleanup(LodePNG_InfoPng* info)
{
    LodePNG_InfoColor_cleanup(&info->color);
}

unsigned LodePNG_InfoColor_copy(LodePNG_InfoColor* dest, const LodePNG_InfoColor* source)
{
    size_t i;
    LodePNG_InfoColor_cleanup(dest);
    *dest = *source;
    for (i = 0; i < source->palettesize * 4; i++) dest->palette[i] = source->palette[i];
    return 0;
}

unsigned LodePNG_InfoPng_copy(LodePNG_InfoPng* dest, const LodePNG_InfoPng* source)
{
    unsigned error = 0;
    LodePNG_InfoPng_cleanup(dest);
    *dest = *source;
    LodePNG_InfoColor_init(&dest->color);
    error = LodePNG_InfoColor_copy(&dest->color, &source->color); if (error) return error;
    return error;
}

void LodePNG_InfoPng_swap(LodePNG_InfoPng* a, LodePNG_InfoPng* b)
{
    LodePNG_InfoPng temp = *a;
    *a = *b;
    *b = temp;
}

void LodePNG_InfoRaw_init(LodePNG_InfoRaw* info)
{
    LodePNG_InfoColor_init(&info->color);
}

void LodePNG_InfoRaw_cleanup(LodePNG_InfoRaw* info)
{
    LodePNG_InfoColor_cleanup(&info->color);
}

unsigned LodePNG_InfoRaw_copy(LodePNG_InfoRaw* dest, const LodePNG_InfoRaw* source)
{
    unsigned error = 0;
    LodePNG_InfoRaw_cleanup(dest);
    *dest = *source;
    LodePNG_InfoColor_init(&dest->color);
    error = LodePNG_InfoColor_copy(&dest->color, &source->color); if (error) return error;
    return error;
}

/* ////////////////////////////////////////////////////////////////////////// */

/*
converts from any color type to 24-bit or 32-bit (later maybe more supported). return value = LodePNG error code
the out buffer must have (w * h * bpp + 7) / 8 bytes, where bpp is the bits per pixel of the output color type (LodePNG_InfoColor_getBpp)
for < 8 bpp images, there may _not_ be padding bits at the end of scanlines.
*/
unsigned LodePNG_convert(fb_data* out, const unsigned char* in, LodePNG_InfoColor* infoOut, LodePNG_InfoColor* infoIn, unsigned w, unsigned h)
{
    size_t i, j, bp = 0; /*bitpointer, used by less-than-8-bit color types*/
    size_t x, y;
    unsigned char c;

    if (!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "color conversion in progress");
        rb->lcd_puts(0, 3, print);
        rb->lcd_update();
    }

    /*cases where in and out already have the same format*/
    if (LodePNG_InfoColor_equal(infoIn, infoOut))
    {

        i = 0;
        j = 0;
        for (y = 0 ; y < h ; y++) {
            for (x = 0 ; x < w ; x++) {
                unsigned char r = in[i++];
                unsigned char g = in[i++];
                unsigned char b = in[i++];
                out[j++] = LCD_RGBPACK(r,g,b);
            }
        }
        return 0;
    }

    if ((infoOut->colorType == 2 || infoOut->colorType == 6) && infoOut->bitDepth == 8)
    {
        if (infoIn->bitDepth == 8)
        {
            switch (infoIn->colorType)
            {
            case 0: /*greyscale color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c=in[i];
                        //unsigned char r = in[i];
                        //unsigned char g = in[i];
                        //unsigned char b = in[i];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            case 2: /*RGB color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        j = 3 * i;
                        unsigned char r = in[j];
                        unsigned char g = in[j + 1];
                        unsigned char b = in[j + 2];
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            case 3: /*indexed color (palette)*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        if (in[i] >= infoIn->palettesize) return 46;
                        j = in[i] << 2;
                        unsigned char r = infoIn->palette[j];
                        unsigned char g = infoIn->palette[j + 1];
                        unsigned char b = infoIn->palette[j + 2];
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            case 4: /*greyscale with alpha*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[i << 1];
                        //unsigned char r = in[i<<1];
                        //unsigned char g = in[i<<1];
                        //unsigned char b = in[i<<1];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            case 6: /*RGB with alpha*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        j = i << 2;
                        unsigned char r = in[j];
                        unsigned char g = in[j + 1];
                        unsigned char b = in[j + 2];
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            default:
                break;
            }
        }
        else if (infoIn->bitDepth == 16)
        {
            switch (infoIn->colorType)
            {
            case 0: /*greyscale color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[i << 1];
                        //unsigned char r = in[2 * i];
                        //unsigned char g = in[2 * i];
                        //unsigned char b = in[2 * i];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            case 2: /*RGB color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        j = 6 * i;
                        unsigned char r = in[j];
                        unsigned char g = in[j + 2];
                        unsigned char b = in[j + 4];
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            case 4: /*greyscale with alpha*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[i << 2];
                        //unsigned char r = in[4 * i];
                        //unsigned char g = in[4 * i];
                        //unsigned char b = in[4 * i];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            case 6: /*RGB with alpha*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        j = i << 3;
                        unsigned char r = in[j];
                        unsigned char g = in[j + 2];
                        unsigned char b = in[j + 4];
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            default:
                break;
            }
        }
        else /*infoIn->bitDepth is less than 8 bit per channel*/
        {
            switch (infoIn->colorType)
            {
            case 0: /*greyscale color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
                        value = (value * 255) / ((1 << infoIn->bitDepth) - 1); /*scale value from 0 to 255*/
                        unsigned char r = (unsigned char)value;
                        unsigned char g = (unsigned char)value;
                        unsigned char b = (unsigned char)value;
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            case 3: /*indexed color (palette)*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
                        if (value >= infoIn->palettesize) return 47;
                        j = value << 2;
                        unsigned char r = infoIn->palette[j];
                        unsigned char g = infoIn->palette[j + 1];
                        unsigned char b = infoIn->palette[j + 2];
                        out[i++] = LCD_RGBPACK(r,g,b);
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    else if (LodePNG_InfoColor_isGreyscaleType(infoOut) && infoOut->bitDepth == 8) /*conversion from greyscale to greyscale*/
    {
        if (!LodePNG_InfoColor_isGreyscaleType(infoIn)) return 62;
        if (infoIn->bitDepth == 8)
        {
            switch (infoIn->colorType)
            {
            case 0: /*greyscale color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[i];
                        //unsigned char r = in[i];
                        //unsigned char g = in[i];
                        //unsigned char b = in[i];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            case 4: /*greyscale with alpha*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[(i << 1) + 1];
                        //unsigned char r = in[2 * i + 1];
                        //unsigned char g = in[2 * i + 1];
                        //unsigned char b = in[2 * i + 1];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            default:
                return 31;
            }
        }
        else if (infoIn->bitDepth == 16)
        {
            switch (infoIn->colorType)
            {
            case 0: /*greyscale color*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[i << 1];
                        //unsigned char r = in[2 * i];
                        //unsigned char g = in[2 * i];
                        //unsigned char b = in[2 * i];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            case 4: /*greyscale with alpha*/
                i = 0;
                for (y = 0 ; y < h ; y++) {
                    for (x = 0 ; x < w ; x++) {
                        c = in[i << 2];
                        //unsigned char r = in[4 * i];
                        //unsigned char g = in[4 * i];
                        //unsigned char b = in[4 * i];
                        out[i++] = LCD_RGBPACK(c,c,c);
                    }
                }
                break;
            default:
                return 31;
            }
        }
        else /*infoIn->bitDepth is less than 8 bit per channel*/
        {
            if (infoIn->colorType != 0) return 31; /*colorType 0 is the only greyscale type with < 8 bits per channel*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                for (x = 0 ; x < w ; x++) {
                    unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
                    value = (value * 255) / ((1 << infoIn->bitDepth) - 1); /*scale value from 0 to 255*/
                    unsigned char r = (unsigned char)value;
                    unsigned char g = (unsigned char)value;
                    unsigned char b = (unsigned char)value;
                    out[i++] = LCD_RGBPACK(r,g,b);
                }
            }
        }
    }
    else return 59;

    return 0;
}

/*Paeth predicter, used by PNG filter type 4*/
static int paethPredictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;

    if (pa <= pb && pa <= pc) return a;
    else if (pb <= pc) return b;
    else return c;
}

/*shared values used by multiple Adam7 related functions*/

static const unsigned ADAM7_IX[7] = { 0, 4, 0, 2, 0, 1, 0 }; /*x start values*/
static const unsigned ADAM7_IY[7] = { 0, 0, 4, 0, 2, 0, 1 }; /*y start values*/
static const unsigned ADAM7_DX[7] = { 8, 8, 4, 4, 2, 2, 1 }; /*x delta values*/
static const unsigned ADAM7_DY[7] = { 8, 8, 8, 4, 4, 2, 2 }; /*y delta values*/

static void Adam7_getpassvalues(unsigned passw[7], unsigned passh[7], size_t filter_passstart[8], size_t padded_passstart[8], size_t passstart[8], unsigned w, unsigned h, unsigned bpp)
{
    /*the passstart values have 8 values: the 8th one actually indicates the byte after the end of the 7th (= last) pass*/
    unsigned i;

    /*calculate width and height in pixels of each pass*/
    for (i = 0; i < 7; i++)
    {
        passw[i] = (w + ADAM7_DX[i] - ADAM7_IX[i] - 1) / ADAM7_DX[i];
        passh[i] = (h + ADAM7_DY[i] - ADAM7_IY[i] - 1) / ADAM7_DY[i];
        if (passw[i] == 0) passh[i] = 0;
        if (passh[i] == 0) passw[i] = 0;
    }

    filter_passstart[0] = padded_passstart[0] = passstart[0] = 0;
    for (i = 0; i < 7; i++)
    {
        filter_passstart[i + 1] = filter_passstart[i] + ((passw[i] && passh[i]) ? passh[i] * (1 + (passw[i] * bpp + 7) / 8) : 0); /*if passw[i] is 0, it's 0 bytes, not 1 (no filtertype-byte)*/
        padded_passstart[i + 1] = padded_passstart[i] + passh[i] * ((passw[i] * bpp + 7) / 8); /*bits padded if needed to fill full byte at end of each scanline*/
        passstart[i + 1] = passstart[i] + (passh[i] * passw[i] * bpp + 7) / 8; /*only padded at end of reduced image*/
    }
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Decoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*read the information from the header and store it in the LodePNG_Info. return value is error*/
void LodePNG_inspect(LodePNG_Decoder* decoder, const unsigned char* in, size_t inlength)
{
    if (inlength == 0 || in == 0) { decoder->error = 48; return; } /*the given data is empty*/
    if (inlength < 29) { decoder->error = 27; return; } /*error: the data length is smaller than the length of the header*/

    /*when decoding a new PNG image, make sure all parameters created after previous decoding are reset*/
    LodePNG_InfoPng_cleanup(&decoder->infoPng);
    LodePNG_InfoPng_init(&decoder->infoPng);
    decoder->error = 0;

    if (in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71 || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10) { decoder->error = 28; return; } /*error: the first 8 bytes are not the correct PNG signature*/
    if (in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R') { decoder->error = 29; return; } /*error: it doesn't start with a IHDR chunk!*/

    /*read the values given in the header*/
    decoder->infoPng.width = LodePNG_read32bitInt(&in[16]);
    decoder->infoPng.height = LodePNG_read32bitInt(&in[20]);
    decoder->infoPng.color.bitDepth = in[24];
    decoder->infoPng.color.colorType = in[25];
    decoder->infoPng.compressionMethod = in[26];
    decoder->infoPng.filterMethod = in[27];
    decoder->infoPng.interlaceMethod = in[28];

    unsigned CRC = LodePNG_read32bitInt(&in[29]);
    unsigned checksum = crc32(0L, &in[12], 17);
    if (CRC != checksum) { decoder->error = 57; return; }

    if (decoder->infoPng.compressionMethod != 0) { decoder->error = 32; return; } /*error: only compression method 0 is allowed in the specification*/
    if (decoder->infoPng.filterMethod != 0)      { decoder->error = 33; return; } /*error: only filter method 0 is allowed in the specification*/
    if (decoder->infoPng.interlaceMethod > 1)    { decoder->error = 34; return; } /*error: only interlace methods 0 and 1 exist in the specification*/

    decoder->error = checkColorValidity(decoder->infoPng.color.colorType, decoder->infoPng.color.bitDepth);
}

static unsigned unfilterScanline(unsigned char* recon, const unsigned char* scanline, const unsigned char* precon, size_t bytewidth, unsigned char filterType, size_t length)
{
    /*
    For PNG filter method 0
    unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte, the filter works byte per byte (bytewidth = 1)
    precon is the previous unfiltered scanline, recon the result, scanline the current one
    the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
    recon and scanline MAY be the same memory address! precon must be disjoint.
    */

    size_t i;
    switch (filterType)
    {
    case 0:
        //for(i = 0; i < length; i++) recon[i] = scanline[i];
        memcpy(recon, scanline, length * sizeof(unsigned char));
        break;
    case 1:
        //for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
        memcpy(recon, scanline, bytewidth * sizeof(unsigned char));
        for (i = bytewidth; i <    length; i++) recon[i] = scanline[i] + recon[i - bytewidth];
        break;
    case 2:
        if (precon) for (i = 0; i < length; i++) recon[i] = scanline[i] + precon[i];
        else       //for(i = 0; i < length; i++) recon[i] = scanline[i];
            memcpy(recon, scanline, length * sizeof(unsigned char));
        break;
    case 3:
        if (precon)
        {
            for (i =         0; i < bytewidth; i++) recon[i] = scanline[i] + precon[i] / 2;
            for (i = bytewidth; i <    length; i++) recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
        }
        else
        {
            //for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
            memcpy(recon, scanline, bytewidth * sizeof(unsigned char));
            for (i = bytewidth; i <    length; i++) recon[i] = scanline[i] + recon[i - bytewidth] / 2;
        }
        break;
    case 4:
        if (precon)
        {
            for (i =         0; i < bytewidth; i++) recon[i] = (unsigned char)(scanline[i] + paethPredictor(0, precon[i], 0));
            for (i = bytewidth; i <    length; i++) recon[i] = (unsigned char)(scanline[i] + paethPredictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]));
        }
        else
        {
            //for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
            memcpy(recon, scanline, bytewidth * sizeof(unsigned char));
            for (i = bytewidth; i <    length; i++) recon[i] = (unsigned char)(scanline[i] + paethPredictor(recon[i - bytewidth], 0, 0));
        }
        break;
    default:
        return 36; /*error: unexisting filter type given*/
    }
    return 0;
}

static unsigned unfilter(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
    /*
    For PNG filter method 0
    this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 it's called 7 times)
    out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
    w and h are image dimensions or dimensions of reduced image, bpp is bits per pixel
    in and out are allowed to be the same memory address!
    */

    unsigned y;
    unsigned char* prevline = 0;

    size_t bytewidth = (bpp + 7) / 8; /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
    size_t linebytes = (w * bpp + 7) / 8;

    for (y = 0; y < h; y++)
    {
        size_t outindex = linebytes * y;
        size_t inindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
        unsigned char filterType = in[inindex];

        unsigned error = unfilterScanline(&out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes);
        if (error) return error;

        prevline = &out[outindex];
    }

    return 0;
}

static void Adam7_deinterlace(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
    /*Note: this function works on image buffers WITHOUT padding bits at end of scanlines with non-multiple-of-8 bit amounts, only between reduced images is padding
    out must be big enough AND must be 0 everywhere if bpp < 8 in the current implementation (because that's likely a little bit faster)*/
    unsigned passw[7], passh[7]; size_t filter_passstart[8], padded_passstart[8], passstart[8];
    unsigned i;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

    if (bpp >= 8)
    {
        for (i = 0; i < 7; i++)
        {
            unsigned x, y, b;
            size_t bytewidth = bpp / 8;
            for (y = 0; y < passh[i]; y++)
                for (x = 0; x < passw[i]; x++)
                {
                    size_t pixelinstart = passstart[i] + (y * passw[i] + x) * bytewidth;
                    size_t pixeloutstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
                    for (b = 0; b < bytewidth; b++)
                    {
                        out[pixeloutstart + b] = in[pixelinstart + b];
                    }
                }
        }
    }
    else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
    {
        for (i = 0; i < 7; i++)
        {
            unsigned x, y, b;
            unsigned ilinebits = bpp * passw[i];
            unsigned olinebits = bpp * w;
            size_t obp, ibp; /*bit pointers (for out and in buffer)*/
            for (y = 0; y < passh[i]; y++)
                for (x = 0; x < passw[i]; x++)
                {
                    ibp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
                    obp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
                    for (b = 0; b < bpp; b++)
                    {
                        unsigned char bit = readBitFromReversedStream(&ibp, in);
                        setBitOfReversedStream0(&obp, out, bit); /*note that this function assumes the out buffer is completely 0, use setBitOfReversedStream otherwise*/
                    }
                }
        }
    }
}

static void removePaddingBits(unsigned char* out, const unsigned char* in, size_t olinebits, size_t ilinebits, unsigned h)
{
    /*
    After filtering there are still padding bits if scanlines have non multiple of 8 bit amounts. They need to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers for the Adam7 code, the color convert code and the output to the user.
    in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must have >= ilinebits*h bits, out must have >= olinebits*h bits, olinebits must be <= ilinebits
    also used to move bits after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
    only useful if (ilinebits - olinebits) is a value in the range 1..7
    */
    unsigned y;
    size_t diff = ilinebits - olinebits;
    size_t obp = 0, ibp = 0; /*bit pointers*/
    for (y = 0; y < h; y++)
    {
        size_t x;
        for (x = 0; x < olinebits; x++)
        {
            unsigned char bit = readBitFromReversedStream(&ibp, in);
            setBitOfReversedStream(&obp, out, bit);
        }
        ibp += diff;
    }
}

/*out must be buffer big enough to contain full image, and in must contain the full decompressed data from the IDAT chunks*/
static unsigned postProcessScanlines(unsigned char* out, unsigned char* in, const LodePNG_Decoder* decoder) /*return value is error*/
{
    /*
    This function converts the filtered-padded-interlaced data into pure 2D image buffer with the PNG's colortype. Steps:
    *) if no Adam7: 1) unfilter 2) remove padding bits (= posible extra bits per scanline if bpp < 8)
    *) if adam7: 1) 7x unfilter 2) 7x remove padding bits 3) Adam7_deinterlace
    NOTE: the in buffer will be overwritten with intermediate data!
    */
    unsigned bpp = LodePNG_InfoColor_getBpp(&decoder->infoPng.color);
    unsigned w = decoder->infoPng.width;
    unsigned h = decoder->infoPng.height;
    unsigned error = 0;
    if (bpp == 0) return 31; /*error: invalid colortype*/

    if (decoder->infoPng.interlaceMethod == 0)
    {
        if (bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
        {
            error = unfilter(in, in, w, h, bpp);
            if (error) return error;
            removePaddingBits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
        }
        else error = unfilter(out, in, w, h, bpp); /*we can immediatly filter into the out buffer, no other steps needed*/
    }
    else /*interlaceMethod is 1 (Adam7)*/
    {
        unsigned passw[7], passh[7]; size_t filter_passstart[8], padded_passstart[8], passstart[8];
        unsigned i;

        Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

        for (i = 0; i < 7; i++)
        {
            error = unfilter(&in[padded_passstart[i]], &in[filter_passstart[i]], passw[i], passh[i], bpp);
            if (error) return error;
            if (bpp < 8) /*TODO: possible efficiency improvement: if in this reduced image the bits fit nicely in 1 scanline, move bytes instead of bits or move not at all*/
            {
                /*remove padding bits in scanlines; after this there still may be padding bits between the different reduced images: each reduced image still starts nicely at a byte*/
                removePaddingBits(&in[passstart[i]], &in[padded_passstart[i]], passw[i] * bpp, ((passw[i] * bpp + 7) / 8) * 8, passh[i]);
            }
        }

        Adam7_deinterlace(out, in, w, h, bpp);
    }

    return error;
}

/*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/
static void decodeGeneric(LodePNG_Decoder* decoder, unsigned char* in, size_t size, void (*pf_progress)(int current, int total))
{
    if (pf_progress != NULL)
        pf_progress(0, 100);
    unsigned char IEND = 0;
    const unsigned char* chunk;
    size_t i;
    unsigned char *idat = memory;
    size_t idat_size = 0;

    /*for unknown chunk order*/
    unsigned unknown = 0;
    unsigned critical_pos = 1; /*1 = after IHDR, 2 = after PLTE, 3 = after IDAT*/

    /*provide some proper output values if error will happen*/
    decoded_image_size = 0;

    if (size == 0 || in == 0) { decoder->error = 48; return; } /*the given data is empty*/

    LodePNG_inspect(decoder, in, size); /*reads header and resets other parameters in decoder->infoPng*/
    if (decoder->error) return;

    chunk = &in[33]; /*first byte of the first chunk after the header*/

    while (!IEND) /*loop through the chunks, ignoring unknown chunks and stopping at IEND chunk. IDAT data is put at the start of the in buffer*/
    {
        unsigned chunkLength;
        const unsigned char* data; /*the data in the chunk*/

        if ((size_t)((chunk - in) + 12) > size || chunk < in) { decoder->error = 30; break; } /*error: size of the in buffer too small to contain next chunk*/
        chunkLength = LodePNG_chunk_length(chunk); /*length of the data of the chunk, excluding the length bytes, chunk type and CRC bytes*/
        if (chunkLength > 2147483647) { decoder->error = 63; break; }
        if ((size_t)((chunk - in) + chunkLength + 12) > size || (chunk + chunkLength + 12) < in) { decoder->error = 35; break; } /*error: size of the in buffer too small to contain next chunk*/
        data = LodePNG_chunk_data_const(chunk);

        /*IDAT chunk, containing compressed image data*/
        if (LodePNG_chunk_type_equals(chunk, "IDAT"))
        {
            size_t oldsize = idat_size;
            idat_size += chunkLength;
            if (idat + idat_size >= image) { decoder->error = OUT_OF_MEMORY; break; }
            memcpy(idat+oldsize, data, chunkLength * sizeof(unsigned char));
            critical_pos = 3;
        }
        /*IEND chunk*/
        else if (LodePNG_chunk_type_equals(chunk, "IEND"))
        {
            IEND = 1;
        }
        /*palette chunk (PLTE)*/
        else if (LodePNG_chunk_type_equals(chunk, "PLTE"))
        {
            unsigned pos = 0;
            decoder->infoPng.color.palettesize = chunkLength / 3;
            if (decoder->infoPng.color.palettesize > 256) { decoder->error = 38; break; } /*error: palette too big*/
            for (i = 0; i < decoder->infoPng.color.palettesize; i++)
            {
                decoder->infoPng.color.palette[4 * i + 0] = data[pos++]; /*R*/
                decoder->infoPng.color.palette[4 * i + 1] = data[pos++]; /*G*/
                decoder->infoPng.color.palette[4 * i + 2] = data[pos++]; /*B*/
                decoder->infoPng.color.palette[4 * i + 3] = 255; /*alpha*/
            }
            critical_pos = 2;
        }
        /*palette transparency chunk (tRNS)*/
        else if (LodePNG_chunk_type_equals(chunk, "tRNS"))
        {
            if (decoder->infoPng.color.colorType == 3)
            {
                if (chunkLength > decoder->infoPng.color.palettesize) { decoder->error = 39; break; } /*error: more alpha values given than there are palette entries*/
                for (i = 0; i < chunkLength; i++) decoder->infoPng.color.palette[4 * i + 3] = data[i];
            }
            else if (decoder->infoPng.color.colorType == 0)
            {
                if (chunkLength != 2) { decoder->error = 40; break; } /*error: this chunk must be 2 bytes for greyscale image*/
                decoder->infoPng.color.key_defined = 1;
                decoder->infoPng.color.key_r = decoder->infoPng.color.key_g = decoder->infoPng.color.key_b = 256 * data[0] + data[1];
            }
            else if (decoder->infoPng.color.colorType == 2)
            {
                if (chunkLength != 6) { decoder->error = 41; break; } /*error: this chunk must be 6 bytes for RGB image*/
                decoder->infoPng.color.key_defined = 1;
                decoder->infoPng.color.key_r = 256 * data[0] + data[1];
                decoder->infoPng.color.key_g = 256 * data[2] + data[3];
                decoder->infoPng.color.key_b = 256 * data[4] + data[5];
            }
            else { decoder->error = 42; break; } /*error: tRNS chunk not allowed for other color models*/
        }
        /*background color chunk (bKGD)*/
        else if (LodePNG_chunk_type_equals(chunk, "bKGD"))
        {
            if (decoder->infoPng.color.colorType == 3)
            {
                if (chunkLength != 1) { decoder->error = 43; break; } /*error: this chunk must be 1 byte for indexed color image*/
                decoder->infoPng.background_defined = 1;
                decoder->infoPng.background_r = decoder->infoPng.background_g = decoder->infoPng.background_g = data[0];
            }
            else if (decoder->infoPng.color.colorType == 0 || decoder->infoPng.color.colorType == 4)
            {
                if (chunkLength != 2) { decoder->error = 44; break; } /*error: this chunk must be 2 bytes for greyscale image*/
                decoder->infoPng.background_defined = 1;
                decoder->infoPng.background_r = decoder->infoPng.background_g = decoder->infoPng.background_b = 256 * data[0] + data[1];
            }
            else if (decoder->infoPng.color.colorType == 2 || decoder->infoPng.color.colorType == 6)
            {
                if (chunkLength != 6) { decoder->error = 45; break; } /*error: this chunk must be 6 bytes for greyscale image*/
                decoder->infoPng.background_defined = 1;
                decoder->infoPng.background_r = 256 * data[0] + data[1];
                decoder->infoPng.background_g = 256 * data[2] + data[3];
                decoder->infoPng.background_b = 256 * data[4] + data[5];
            }
        }
        else if (LodePNG_chunk_type_equals(chunk, "tIME"))
        {
            if (chunkLength != 7) { decoder->error = 73; break; }
            decoder->infoPng.time_defined = 1;
            decoder->infoPng.time.year = 256 * data[0] + data[+ 1];
            decoder->infoPng.time.month = data[2];
            decoder->infoPng.time.day = data[3];
            decoder->infoPng.time.hour = data[4];
            decoder->infoPng.time.minute = data[5];
            decoder->infoPng.time.second = data[6];
        }
        else if (LodePNG_chunk_type_equals(chunk, "pHYs"))
        {
            if (chunkLength != 9) { decoder->error = 74; break; }
            decoder->infoPng.phys_defined = 1;
            decoder->infoPng.phys_x = 16777216 * data[0] + 65536 * data[1] + 256 * data[2] + data[3];
            decoder->infoPng.phys_y = 16777216 * data[4] + 65536 * data[5] + 256 * data[6] + data[7];
            decoder->infoPng.phys_unit = data[8];
        }
        else /*it's not an implemented chunk type, so ignore it: skip over the data*/
        {
            if (LodePNG_chunk_critical(chunk)) { decoder->error = 69; break; } /*error: unknown critical chunk (5th bit of first byte of chunk type is 0)*/
            unknown = 1;
        }

        if (!unknown) /*check CRC if wanted, only on known chunk types*/
        {
            long time = *rb->current_tick;
            if (LodePNG_chunk_check_crc(chunk)) { decoder->error = 57; break; }
            time = *rb->current_tick-time;
        }

        if (!IEND) chunk = LodePNG_chunk_next_const(chunk);
    }

    if (!decoder->error)
    {
        unsigned char *scanlines = idat + idat_size;
        size_t scanlines_size = (size_t)memory_max - idat_size;
        long time = *rb->current_tick;
        decoder->error = LodePNG_decompress(scanlines, &scanlines_size, idat, idat_size, decoder->error_msg); /*decompress with the Zlib decompressor*/
        if (pf_progress) pf_progress(100, 100);
        time = *rb->current_tick-time;

        if (!decoder->error)
        {
            decoded_image_size = (decoder->infoPng.height * decoder->infoPng.width * LodePNG_InfoColor_getBpp(&decoder->infoPng.color) + 7) / 8;
            if (decoded_image_size > memory_size) { decoder->error = OUT_OF_MEMORY; return; }
            decoded_image = memory_max - decoded_image_size;
            if (scanlines + scanlines_size >= decoded_image) { decoder->error = OUT_OF_MEMORY; return; }
            memset(decoded_image, 0, decoded_image_size * sizeof(unsigned char));
            if (!running_slideshow)
            {
                rb->snprintf(print, sizeof(print), "unfiltering scanlines");
                rb->lcd_puts(0, 3, print);
                rb->lcd_update();
            }
            decoder->error = postProcessScanlines(decoded_image, scanlines, decoder);
        }
    }
}

void LodePNG_decode(LodePNG_Decoder* decoder, unsigned char* in, size_t insize, void (*pf_progress)(int current, int total))
{
    decodeGeneric(decoder, in, insize, pf_progress);
    if (decoder->error) return;

    /*TODO: check if this works according to the statement in the documentation: "The converter can convert from greyscale input color type, to 8-bit greyscale or greyscale with alpha"*/
    if (!(decoder->infoRaw.color.colorType == 2 || decoder->infoRaw.color.colorType == 6) && !(decoder->infoRaw.color.bitDepth == 8)) { decoder->error = 56; return; }
    converted_image = (fb_data *)((intptr_t)(memory + 3) & ~3);
    converted_image_size = decoder->infoPng.width*decoder->infoPng.height;
    if ((unsigned char *)(converted_image + converted_image_size) >= decoded_image) { decoder->error = OUT_OF_MEMORY; }
    if (!decoder->error) decoder->error = LodePNG_convert(converted_image, decoded_image, &decoder->infoRaw.color, &decoder->infoPng.color, decoder->infoPng.width, decoder->infoPng.height);
}

void LodePNG_DecodeSettings_init(LodePNG_DecodeSettings* settings)
{
    settings->color_convert = 1;
}

void LodePNG_Decoder_init(LodePNG_Decoder* decoder)
{
    LodePNG_DecodeSettings_init(&decoder->settings);
    LodePNG_InfoRaw_init(&decoder->infoRaw);
    LodePNG_InfoPng_init(&decoder->infoPng);
    decoder->error = 1;
}

void LodePNG_Decoder_cleanup(LodePNG_Decoder* decoder)
{
    LodePNG_InfoRaw_cleanup(&decoder->infoRaw);
    LodePNG_InfoPng_cleanup(&decoder->infoPng);
}

#define PNG_ERROR_MIN 27
#define PNG_ERROR_MAX 74
static const unsigned char *png_error_messages[PNG_ERROR_MAX-PNG_ERROR_MIN+1] =
{
    "png file smaller than a png header", /*27*/
    "incorrect png signature",  /*28*/
    "first chunk is not IHDR",  /*29*/
    "chunk length too large",   /*30*/
    "illegal PNG color type or bpp",    /*31*/
    "illegal PNG compression method",   /*32*/
    "illegal PNG filter method",    /*33*/
    "illegal PNG interlace method", /*34*/
    "chunk length of a chunk is too large or the chunk too small",  /*35*/
    "illegal PNG filter type encountered",  /*36*/
    "illegal bit depth for this color type given",  /*37*/
    "the palette is too big (more than 256 colors)",    /*38*/
    "more palette alpha values given in tRNS, than there are colors in the palette",    /*39*/
    "tRNS chunk has wrong size for greyscale image",    /*40*/
    "tRNS chunk has wrong size for RGB image",  /*41*/
    "tRNS chunk appeared while it was not allowed for this color type", /*42*/
    "bKGD chunk has wrong size for palette image",  /*43*/
    "bKGD chunk has wrong size for greyscale image",    /*44*/
    "bKGD chunk has wrong size for RGB image",  /*45*/
    "value encountered in indexed image is larger than the palette size",   /*46*/
    "value encountered in indexed image is larger than the palette size",   /*47*/
    "input file is empty",  /*48*/
    NULL,   /*49*/
    NULL,   /*50*/
    NULL,   /*51*/
    NULL,   /*52*/
    NULL,   /*53*/
    NULL,   /*54*/
    NULL,   /*55*/
    NULL,   /*56*/
    "invalid CRC",  /*57*/
    NULL,   /*58*/
    "conversion to unexisting or unsupported color type or bit depth",  /*59*/
    NULL,   /*60*/
    NULL,   /*61*/
    NULL,   /*62*/
    "png chunk too long",   /*63*/
    NULL,   /*64*/
    NULL,   /*65*/
    NULL,   /*66*/
    NULL,   /*67*/
    NULL,   /*68*/
    "unknown critical chunk",   /*69*/
    NULL,   /*70*/
    NULL,   /*71*/
    NULL,   /*72*/
    "invalid tIME chunk size",  /*73*/
    "invalid pHYs chunk size",  /*74*/
};

bool img_ext(const char *ext)
{
    if (!ext)
        return false;
    if (!rb->strcasecmp(ext,".png"))
        return true;
    else
        return false;
}

void draw_image_rect(struct image_info *info,
                     int x, int y, int width, int height)
{
    fb_data **pdisp = (fb_data**)info->data;
    rb->lcd_bitmap_part(*pdisp, info->x + x, info->y + y, info->width,
                        x + MAX(0, (LCD_WIDTH-info->width)/2),
                        y + MAX(0, (LCD_HEIGHT-info->height)/2),
                        width, height);
}

int img_mem(int ds)
{
    LodePNG_Decoder *decoder = &_decoder;
    return (decoder->infoPng.width/ds) * (decoder->infoPng.height/ds) * FB_DATA_SZ;
}

int load_image(char *filename, struct image_info *info,
               unsigned char *buf, ssize_t *buf_size)
{
    int fd;
    long time = 0; /* measured ticks */
    int w, h; /* used to center output */
    LodePNG_Decoder *decoder = &_decoder;

    memset(&disp, 0, sizeof(disp));
    LodePNG_Decoder_init(decoder);

    memory = buf;
    memory_size = *buf_size;
    memory_max = memory + memory_size;

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ, "err opening %s:%d", filename, fd);
        return PLUGIN_ERROR;
    }
    image_size = rb->filesize(fd);

    DEBUGF("reading file '%s'\n", filename);

    if (!running_slideshow) {
        rb->snprintf(print, sizeof(print), "%s:", rb->strrchr(filename,'/')+1);
        rb->lcd_puts(0, 0, print);
        rb->lcd_update();
    }

    if (image_size > memory_size) {
        decoder->error = FILE_TOO_LARGE;
        rb->close(fd);

    } else {
        if (!running_slideshow) {
            rb->snprintf(print, sizeof(print), "loading %lu bytes", image_size);
            rb->lcd_puts(0, 1, print);
            rb->lcd_update();
        }

        image = memory_max - image_size;
        rb->read(fd, image, image_size);
        rb->close(fd);

        if (!running_slideshow) {
            rb->snprintf(print, sizeof(print), "decoding image");
            rb->lcd_puts(0, 2, print);
            rb->lcd_update();
        }
#ifdef DISK_SPINDOWN
        else if (immediate_ata_off) {
            /* running slideshow and time is long enough: power down disk */
            rb->storage_sleep();
        }
#endif

        decoder->settings.color_convert = 1;
        decoder->infoRaw.color.colorType = 2;
        decoder->infoRaw.color.bitDepth = 8;

        LodePNG_inspect(decoder, image, image_size);

        if (!decoder->error) {

            if (!running_slideshow) {
                rb->snprintf(print, sizeof(print), "image %dx%d",
                             decoder->infoPng.width, decoder->infoPng.height);
                rb->lcd_puts(0, 2, print);
                rb->lcd_update();

                rb->snprintf(print, sizeof(print), "decoding %d*%d",
                             decoder->infoPng.width, decoder->infoPng.height);
                rb->lcd_puts(0, 3, print);
                rb->lcd_update();
            }

            /* the actual decoding */
            time = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
            LodePNG_decode(decoder, image, image_size, cb_progress);
            rb->cpu_boost(false);
#else
            LodePNG_decode(decoder, image, image_size, cb_progress);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
        }
    }

    time = *rb->current_tick - time;

    if (!running_slideshow && !decoder->error)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    if (decoder->error) {
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        if (plug_buf && (decoder->error == FILE_TOO_LARGE
            || decoder->error == OUT_OF_MEMORY || decoder->error == Z_MEM_ERROR))
            return PLUGIN_OUTOFMEM;
#endif

        if (decoder->error >= PNG_ERROR_MIN && decoder->error <= PNG_ERROR_MAX
            && png_error_messages[decoder->error-PNG_ERROR_MIN] != NULL)
        {
            rb->splash(HZ, png_error_messages[decoder->error-PNG_ERROR_MIN]);
        }
        else
        {
            switch (decoder->error) {
            case PLUGIN_ABORT:
                break;
            case OUT_OF_MEMORY:
            case Z_MEM_ERROR:
                rb->splash(HZ, "Out of Memory");break;
            case FILE_TOO_LARGE:
                rb->splash(HZ, "File too large");break;
            case Z_DATA_ERROR:
                rb->splash(HZ, decoder->error_msg);break;
            default:
                rb->splashf(HZ, "other error : %ld", decoder->error);break;
            }
        }

        if (decoder->error == PLUGIN_ABORT)
            return PLUGIN_ABORT;
        else
            return PLUGIN_ERROR;
    }

    disp_buf = (fb_data *)((intptr_t)(converted_image + converted_image_size + 3) & ~3);
    info->x_size = decoder->infoPng.width;
    info->y_size = decoder->infoPng.height;
    *buf_size = memory_max - (unsigned char*)disp_buf;
    return PLUGIN_OK;
}

int get_image(struct image_info *info, int ds)
{
    fb_data **p_disp = &disp[ds]; /* short cut */
    LodePNG_Decoder *decoder = &_decoder;

    info->width = decoder->infoPng.width / ds;
    info->height = decoder->infoPng.height / ds;
    info->data = p_disp;

    if (*p_disp != NULL)
    {
        /* we still have it */
        return PLUGIN_OK;
    }

    /* assign image buffer */
    if (ds > 1) {
        if (!running_slideshow)
        {
            rb->snprintf(print, sizeof(print), "resizing %d*%d",
                         info->width, info->height);
            rb->lcd_puts(0, 3, print);
            rb->lcd_update();
        }
        struct bitmap bmp_src, bmp_dst;

        int size = info->width * info->height;

        if ((unsigned char *)(disp_buf + size) >= memory_max) {
            /* have to discard the current */
            int i;
            for (i=1; i<=8; i++)
                disp[i] = NULL; /* invalidate all bitmaps */
            /* start again from the beginning of the buffer */
            disp_buf = (fb_data *)((intptr_t)(converted_image + converted_image_size + 3) & ~3);
        }

        *p_disp = disp_buf;
        disp_buf = (fb_data *)((intptr_t)(disp_buf + size + 3) & ~3);

        bmp_src.width = decoder->infoPng.width;
        bmp_src.height = decoder->infoPng.height;
        bmp_src.data = (unsigned char *)converted_image;

        bmp_dst.width = info->width;
        bmp_dst.height = info->height;
        bmp_dst.data = (unsigned char *)*p_disp;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
        smooth_resize_bitmap(&bmp_src, &bmp_dst);
        rb->cpu_boost(false);
#else
        smooth_resize_bitmap(&bmp_src, &bmp_dst);
#endif /*HAVE_ADJUSTABLE_CPU_FREQ*/
    } else {
        *p_disp = converted_image;
    }

    return PLUGIN_OK;
}
