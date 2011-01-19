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
 * Copyright (c) 2010 Marcin Bukat
 *  - pixel format conversion & transparency handling
 *  - adaptation of tinf (tiny inflate library)
 *  - code refactoring & cleanups
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

/* supported chunk types:
 * critical:
 *     IHDR
 *     PLTE
 *     IDAT
 *     IEND
 *
 * ancillary:
 *     tRNS
 *     bKGD
 */

#include "plugin.h"
#include "lcd.h"
#include <lib/pluginlib_bmp.h>
#include "tinf.h"
#include "bmp.h"
#include "png_decoder.h"
#if LCD_DEPTH < 8
#include <lib/grey.h>
#endif

#ifndef resize_bitmap
#if defined(HAVE_LCD_COLOR)
#define resize_bitmap   smooth_resize_bitmap
#else
#define resize_bitmap   grey_resize_bitmap
#endif
#endif

static const char *png_error_messages[PNG_ERROR_MAX-PNG_ERROR_MIN+1] =
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

static unsigned LodePNG_decompress(unsigned char* out,
                                   size_t* outsize,
                                   const unsigned char* in,
                                   size_t insize)
{
    int err;
    err = tinf_zlib_uncompress((void *)out,
                               (unsigned int*)outsize,
                               (const void*)in,
                               (unsigned int)insize);
    return err;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Reading and writing single bits and bytes from/to stream for LodePNG   / */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned char readBitFromReversedStream(size_t* bitpointer,
                                               const unsigned char* bitstream)
{
    unsigned char result = (unsigned char)((bitstream[(*bitpointer) >> 3] >>
                           (7 - ((*bitpointer) & 0x7))) & 1);
    (*bitpointer)++;
    return result;
}

static unsigned readBitsFromReversedStream(size_t* bitpointer,
                                           const unsigned char* bitstream,
                                           size_t nbits)
{
    unsigned result = 0;
    size_t i;
    for (i = nbits - 1; i < nbits; i--)
        result += (unsigned)readBitFromReversedStream(bitpointer, bitstream)<<i;

    return result;
}

static void setBitOfReversedStream0(size_t* bitpointer, 
                                    unsigned char* bitstream,
                                    unsigned char bit)
{
    /* the current bit in bitstream must be 0 for this to work
     * earlier bit of huffman code is in a lesser significant bit
     * of an earlier byte
     */
    if (bit)
        bitstream[(*bitpointer) >> 3] |=  (bit << (7 - ((*bitpointer) & 0x7)));

    (*bitpointer)++;
}

static void setBitOfReversedStream(size_t* bitpointer,
                                   unsigned char* bitstream,
                                   unsigned char bit)
{
    /* the current bit in bitstream may be 0 or 1 for this to work */
    if (bit == 0)
        bitstream[(*bitpointer) >> 3] &= 
            (unsigned char)(~(1 << (7 - ((*bitpointer) & 0x7))));
    else
        bitstream[(*bitpointer) >> 3] |=  (1 << (7 - ((*bitpointer) & 0x7)));

    (*bitpointer)++;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG chunks                                                             / */
/* ////////////////////////////////////////////////////////////////////////// */

/* get the length of the data of the chunk.
 * Total chunk length has 12 bytes more.
 */
static unsigned LodePNG_chunk_length(const uint8_t* chunk)
{
    return chunk[0]<<24|chunk[1]<<16|chunk[2]<<8|chunk[3];
}

/* check if the type is the given type */
static bool LodePNG_chunk_type_equals(const uint8_t* chunk, uint32_t type)
{
    /* chunk type field: A 4-byte chunk type code. For convenience in 
     * description and in examining PNG files, type codes are restricted
     * to consist of uppercase and lowercase ASCII letters 
     * (A-Z and a-z, or 65-90 and 97-122 decimal). However, encoders and
     * decoders must treat the codes as fixed binary values, not character
     * strings."
     */
    return ((uint32_t)(chunk[4]<<24|chunk[5]<<16|chunk[6]<<8|chunk[7]) == (uint32_t)type);
}

/* properties of PNG chunks gotten from capitalization of chunk type name,
 * as defined by the standard
 * 0: ancillary chunk
 * 1: critical chunk type
 */
static inline bool LodePNG_chunk_critical(const uint8_t* chunk)
{
    return((chunk[4] & 32) == 0);
}

/* 0: public, 1: private */
static inline bool LodePNG_chunk_private(const uint8_t* chunk) 
{
    return((chunk[6] & 32) != 0);
}

/* get pointer to the data of the chunk */
static inline const uint8_t* LodePNG_chunk_data(const uint8_t* chunk)
{
    return &chunk[8];
}

/* returns 0 if the crc is correct, error code if it's incorrect */
static bool LodePNG_chunk_check_crc(const uint8_t* chunk)
{
    uint32_t length = LodePNG_chunk_length(chunk);
    uint32_t crc = chunk[length + 8]<<24|chunk[length + 8 + 1]<<16|
                   chunk[length + 8 + 2]<<8|chunk[length + 8 + 3];

    /* the CRC is taken of the data and the 4 chunk type letters,
     * not the length
     */
    uint32_t checksum = tinf_crc32(chunk + 4, length + 4);
    return (crc == checksum);
}

/* don't use on IEND chunk, as there is no next chunk then */
static const uint8_t* LodePNG_chunk_next(const uint8_t* chunk)
{
    uint32_t total_chunk_length = LodePNG_chunk_length(chunk) + 12;
    return &chunk[total_chunk_length];
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Color types and such                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

/* return type is a LodePNG error code
 * bd - bit depth
 */
static uint8_t checkColorValidity(uint8_t colorType, uint8_t bd) 
{
    switch (colorType)
    {
    case PNG_COLORTYPE_GREY:
        if (!(bd == 1 || bd == 2 ||
              bd == 4 || bd == 8 ||
              bd == 16))
            return 37;
        break; /*grey*/

    case PNG_COLORTYPE_RGB:
        if (!(bd == 8 || bd == 16))
            return 37;
        break; /*RGB*/

    case PNG_COLORTYPE_PALETTE:
        if (!(bd == 1 || bd == 2 ||
              bd == 4 || bd == 8 ))
            return 37;
        break; /*palette*/

    case PNG_COLORTYPE_GREYA:
        if (!( bd == 8 || bd == 16 ))
            return 37;
        break; /*grey + alpha*/

    case PNG_COLORTYPE_RGBA:
        if (!( bd == 8 || bd == 16 ))
            return 37;
        break; /*RGBA*/

    default:
        return 31;
    }
    return 0; /*allowed color type / bits combination*/
}

static uint8_t getNumColorChannels(uint8_t colorType)
{
    switch (colorType)
    {
    case PNG_COLORTYPE_GREY:
        return 1; /*grey*/
    case PNG_COLORTYPE_RGB:
        return 3; /*RGB*/
    case PNG_COLORTYPE_PALETTE:
        return 1; /*palette*/
    case PNG_COLORTYPE_GREYA:
        return 2; /*grey + alpha*/
    case PNG_COLORTYPE_RGBA:
        return 4; /*RGBA*/
    }
    return 0; /*unexisting color type*/
}

static uint8_t getBpp(uint8_t colorType, uint8_t bitDepth)
{
    /* bits per pixel is amount of channels * bits per channel */
    return getNumColorChannels(colorType) * bitDepth;
}

static void LodePNG_InfoColor_init(LodePNG_InfoColor* info)
{
    info->key_defined = 0;
    info->key_r = info->key_g = info->key_b = 0;
    info->colorType = PNG_COLORTYPE_RGBA;
    info->bitDepth = 8;
    memset(info->palette, 0, 256 * 4 * sizeof(unsigned char));
    info->palettesize = 0;
}

static void LodePNG_InfoColor_cleanup(LodePNG_InfoColor* info)
{
    info->palettesize = 0;
}

static void LodePNG_InfoPng_init(LodePNG_InfoPng* info)
{
    info->width = info->height = 0;
    LodePNG_InfoColor_init(&info->color);
    info->interlaceMethod = 0;
    info->compressionMethod = 0;
    info->filterMethod = 0;
#ifdef HAVE_LCD_COLOR
    info->background_r = info->background_g = info->background_b = 0;
#else
    info->background_r = info->background_g = info->background_b = 255;
#endif
}

void LodePNG_InfoPng_cleanup(LodePNG_InfoPng* info)
{
    LodePNG_InfoColor_cleanup(&info->color);
}

/*
 * Convert from every colortype to rockbox native pixel format (color targets) or
 * greylib pixel format (grey targets)
 */
static void LodePNG_convert(LodePNG_Decoder *decoder)
{

    LodePNG_InfoPng *infoIn = &decoder->infoPng;
    const uint8_t *in = decoder->decoded_img;
    uint8_t *out = decoder->buf;
    uint16_t w = infoIn->width;
    uint16_t h = infoIn->height;
    size_t i, j, bp = 0; /*bitpointer, used by less-than-8-bit color types*/
    size_t x, y;
    uint16_t value, alpha, alpha_complement;

    /* line buffer for pixel format transformation */
#ifdef HAVE_LCD_COLOR
    struct uint8_rgb *line_buf = (struct uint8_rgb *)(out + w * h * FB_DATA_SZ);
#else
    uint8_t *line_buf = (unsigned char *)(out + w * h);
#endif

    struct bitmap bm = {
        .width = w,
        .height = h,
        .data = (unsigned char*)out,
    };

    struct scaler_context ctx = {
        .bm = &bm,
        .dither = 0,
    };

#if LCD_DEPTH < 8
    const struct custom_format *cformat = &format_grey;
#else
    const struct custom_format *cformat = &format_native;
#endif

    void (*output_row_8)(uint32_t, void*, struct scaler_context*) = cformat->output_row_8;

#ifdef HAVE_LCD_COLOR
    struct uint8_rgb *pixel;
#else
    unsigned char *pixel;
#endif

#ifdef HAVE_LCD_COLOR
    if (infoIn->color.bitDepth == 8)
    {
        switch (infoIn->color.colorType)
        {
        case PNG_COLORTYPE_GREY: /*greyscale color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                /* reset line buf */
                pixel = line_buf;

                for (x = 0; x < w ; x++) {
                    value = in[i++];
                    if (infoIn->color.key_defined)
                        if ( (uint8_t)value == (uint8_t)infoIn->color.key_r )
                            value = infoIn->background_r; /* full transparent */

                    pixel->red = (uint8_t)value;
                    pixel->green = (uint8_t)value;
                    pixel->blue = (uint8_t)value;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGB: /*RGB color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = 3*i++;

                    /* tRNs & bKGD */
                    if (infoIn->color.key_defined &&
                        in[j] == (uint8_t)infoIn->color.key_r &&
                        in[j + 1] == (uint8_t)infoIn->color.key_g &&
                        in[j + 2] == (uint8_t)infoIn->color.key_b)
                    {
                        pixel->red = (uint8_t)infoIn->background_r;
                        pixel->green = (uint8_t)infoIn->background_g;
                        pixel->blue = (uint8_t)infoIn->background_b;
                    }
                    else
                    {
                        pixel->red = in[j];
                        pixel->green = in[j + 1];
                        pixel->blue = in[j + 2];
                    }
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_PALETTE: /*indexed color (palette)*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                /* reset line buf */
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    if (in[i] >= infoIn->color.palettesize)
                    {
                        decoder->error = 46;
                        return;
                    }

                    j = in[i++]<<2;
                    alpha = infoIn->color.palette[j + 3];
                    alpha_complement = (256 - alpha);

                    /* tRNS and bKGD */
                    pixel->red = (infoIn->color.palette[j] * alpha +
                                  alpha_complement*infoIn->background_r)>>8;
                    pixel->green = (infoIn->color.palette[j + 1] * alpha +
                                    alpha_complement*infoIn->background_g)>>8;
                    pixel->blue = (infoIn->color.palette[j + 2] * alpha + 
                                   alpha_complement*infoIn->background_b)>>8;
                    pixel++;
                }
                output_row_8(y,(void *)(line_buf),&ctx);
            }
            break;
        case PNG_COLORTYPE_GREYA: /*greyscale with alpha*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    alpha = in[(i << 1) + 1];
                    alpha_complement = (256 - alpha)*infoIn->background_r;
                    value = (alpha * in[i++ << 1] + alpha_complement)>>8;
                    pixel->red = (uint8_t)(value);
                    pixel->green = (uint8_t)value;
                    pixel->blue = (uint8_t)value;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGBA: /*RGB with alpha*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = i++ << 2;
                    alpha = in[j + 3];
                    alpha_complement = (256 - alpha);
                    pixel->red = (in[j] * alpha + 
                                  alpha_complement*infoIn->background_r)>>8;
                    pixel->green = (in[j + 1] * alpha +
                                    alpha_complement*infoIn->background_g)>>8;
                    pixel->blue = (in[j + 2] * alpha +
                                   alpha_complement*infoIn->background_b)>>8;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        default:
            break;
        }
    }
    else if (infoIn->color.bitDepth == 16)
    {
        switch (infoIn->color.colorType)
        {
        case PNG_COLORTYPE_GREY: /*greyscale color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    value = (in[i<<1]<<8)|in[(i << 1) + 1];
                    i++;

                    /* tRNS and bKGD */
                    if (infoIn->color.key_defined &&
                        value == infoIn->color.key_r)
                        value = infoIn->background_r<<8;

                    pixel->red = 
                    pixel->green =
                    pixel->blue = (uint8_t)(value>>8);
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGB: /*RGB color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = 6 * i++;

                    /* tRNS and bKGD */
                    if (infoIn->color.key_defined &&
                        ((uint16_t)(in[j]<<8|in[j + 1])  == 
                            infoIn->color.key_r) &&
                        ((uint16_t)(in[j + 2]<<8|in[j + 3]) == 
                            infoIn->color.key_g) &&
                        ((uint16_t)(in[j + 4]<<8|in[j + 5]) == 
                            infoIn->color.key_b))
                    {
                        pixel->red = (uint8_t)infoIn->background_r;
                        pixel->green = (uint8_t)infoIn->background_g;
                        pixel->blue = (uint8_t)infoIn->background_b;
                    }
                    else
                    {
                        pixel->red = in[j];
                        pixel->green = in[j + 2];
                        pixel->blue = in[j + 4];
                    }
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_GREYA: /*greyscale with alpha*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    alpha = in[(i << 2) + 2];
                    alpha_complement = (256-alpha)*infoIn->background_r;
                    value = (in[i++ << 2] * alpha + alpha_complement)>>8;
                    pixel->red = (uint8_t)value;
                    pixel->green = (uint8_t)value;
                    pixel->blue = (uint8_t)value;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGBA: /*RGB with alpha*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = i++ << 3;
                    alpha = in[j + 6];
                    alpha_complement = (256-alpha);
                    pixel->red = (in[j] * alpha +
                                  alpha_complement*infoIn->background_r)>>8;
                    pixel->green = (in[j + 2] * alpha +
                                    alpha_complement*infoIn->background_g)>>8;
                    pixel->blue = (in[j + 4] * alpha +
                                   alpha_complement*infoIn->background_b)>>8;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        default:
            break;
        }
    }
    else /*infoIn->bitDepth is less than 8 bit per channel*/
    {
        switch (infoIn->color.colorType)
        {
        case PNG_COLORTYPE_GREY: /*greyscale color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    value = readBitsFromReversedStream(&bp, in, infoIn->color.bitDepth);

                    /* tRNS and bKGD */ 
                    if (infoIn->color.key_defined)
                        if ( value == infoIn->color.key_r )
                            value = infoIn->background_r; /* full transparent */

                    /* scale value from 0 to 255 */
                    value = (value * 255) / ((1 << infoIn->color.bitDepth) - 1);

                    pixel->red = (uint8_t)value;
                    pixel->green = (uint8_t)value;
                    pixel->blue = (uint8_t)value;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_PALETTE: /*indexed color (palette)*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    value = readBitsFromReversedStream(&bp, in, infoIn->color.bitDepth);
                    if (value >= infoIn->color.palettesize)
                    {
                        decoder->error = 47;
                        return;
                    }

                    j = value << 2;

                    /* tRNS and bKGD */
                    alpha = infoIn->color.palette[j + 3];
                    alpha_complement = (256 - alpha);
                    pixel->red = (alpha * infoIn->color.palette[j] + 
                                  alpha_complement*infoIn->background_r)>>8;
                    pixel->green = (alpha * infoIn->color.palette[j + 1] + 
                                    alpha_complement*infoIn->background_g)>>8;
                    pixel->blue = (alpha * infoIn->color.palette[j + 2] + 
                                   alpha_complement*infoIn->background_b)>>8;
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        default:
            break;
        }
    }
#else /* greyscale targets */
    struct uint8_rgb px_rgb; /* for rgb(a) -> greyscale conversion */
    uint8_t background_grey; /* for rgb background -> greyscale background */

    if (infoIn->color.bitDepth == 8)
    {
        switch (infoIn->color.colorType)
        {
        case PNG_COLORTYPE_GREY: /*greyscale color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++ ) {
                    value = in[i++];

                    /* transparent color defined in tRNS chunk */
                    if (infoIn->color.key_defined)
                        if ( (uint8_t)value == (uint8_t)infoIn->color.key_r )
                            value = infoIn->background_r;

                    *pixel++ = (uint8_t)value;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGB: /*RGB color*/
            /* convert background rgb color to greyscale */
            px_rgb.red = infoIn->background_r;
            px_rgb.green = infoIn->background_g;
            px_rgb.blue = infoIn->background_b;
            background_grey = brightness(px_rgb);

            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = 3*i++;

                    /* tRNs & bKGD */
                    if (infoIn->color.key_defined &&
                        in[j] == (uint8_t)infoIn->color.key_r &&
                        in[j + 1] == (uint8_t)infoIn->color.key_g &&
                        in[j + 2] == (uint8_t)infoIn->color.key_b)
                    {
                        *pixel = background_grey;
                    }
                    else
                    {
                        /* rgb -> greyscale */
                        px_rgb.red = in[j];
                        px_rgb.green = in[j + 1];
                        px_rgb.blue = in[j + 2];
                        *pixel = brightness(px_rgb);
                    }
                    pixel++;

                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_PALETTE: /*indexed color (palette)*/
            i = 0;
            /* calculate grey value of rgb background */
            px_rgb.red = infoIn->background_r;
            px_rgb.green = infoIn->background_g;
            px_rgb.blue = infoIn->background_b;
            background_grey = brightness(px_rgb);

            for (y = 0 ; y < h ; y++) {
                /* reset line buf */
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    if (in[i] >= infoIn->color.palettesize)
                    {
                        decoder->error = 46;
                        return;
                    }

                    j = in[i++] << 2;
                    alpha = infoIn->color.palette[j + 3];
                    alpha_complement = (256 - alpha);

                    /* tRNS and bKGD */
                    px_rgb.red = (alpha * infoIn->color.palette[j] +
                                  alpha_complement*background_grey)>>8;
                    px_rgb.green = (alpha * infoIn->color.palette[j + 1] +
                                    alpha_complement*background_grey)>>8;
                    px_rgb.blue = (alpha * infoIn->color.palette[j + 2] + 
                                   alpha_complement*background_grey)>>8;

                    *pixel++ = brightness(px_rgb);
                }
                output_row_8(y,(void *)(line_buf),&ctx);
            }
            break;
        case PNG_COLORTYPE_GREYA: /*greyscale with alpha*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    alpha = in[(i << 1) + 1];
                    alpha_complement = ((256 - alpha)*infoIn->background_r);
                    *pixel++ = (alpha * in[i++ << 1] + alpha_complement)>>8;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGBA: /*RGB with alpha*/
            px_rgb.red = infoIn->background_r;
            px_rgb.green = infoIn->background_g;
            px_rgb.blue = infoIn->background_b;
            background_grey = brightness(px_rgb);

            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = i++ << 2;
                    alpha = in[j + 3];
                    alpha_complement = ((256 - alpha)*background_grey);

                    px_rgb.red = in[j];
                    px_rgb.green = in[j + 1];
                    px_rgb.blue = in[j + 2];
                    *pixel++ = (alpha * brightness(px_rgb) + 
                                alpha_complement)>>8;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        default:
            break;
        }
    }
    else if (infoIn->color.bitDepth == 16)
    {
        switch (infoIn->color.colorType)
        {
        case PNG_COLORTYPE_GREY: /*greyscale color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    /* specification states that we have to compare
                     * colors for simple transparency in 16bits
                     * even if we scale down to 8bits later
                     */
                    value = in[i<<1]<<8|in[(i << 1) + 1];
                    i++;

                    /* tRNS and bKGD */
                    if (infoIn->color.key_defined &&
                        value == infoIn->color.key_r)
                        value = infoIn->background_r<<8;

                    /* we take upper 8bits */
                    *pixel++ = (uint8_t)(value>>8);
                }

                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGB: /*RGB color*/
            i = 0;
            px_rgb.red = infoIn->background_r;
            px_rgb.green = infoIn->background_g;
            px_rgb.blue = infoIn->background_b;
            background_grey = brightness(px_rgb);

            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = 6 * i++;

                    /* tRNS and bKGD */
                    if (infoIn->color.key_defined &&
                        (uint16_t)(in[j]<<8|in[j + 1]) == 
                            infoIn->color.key_r &&
                        (uint16_t)(in[j + 2]<<8|in[j + 3]) ==
                            infoIn->color.key_g &&
                        (uint16_t)(in[j + 4]<<8|in[j + 5]) ==
                            infoIn->color.key_b)
                    {
                        *pixel = background_grey;
                    }
                    else
                    {
                        /* we take only upper byte of 16bit value */
                        px_rgb.red = in[j];
                        px_rgb.green = in[j + 2];
                        px_rgb.blue = in[j + 4];
                        *pixel = brightness(px_rgb);
                    }
                    pixel++;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_GREYA: /*greyscale with alpha*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    alpha = in[(i << 2) + 2];
                    alpha_complement = (256 - alpha)*infoIn->background_r;
                    *pixel++ = (alpha * in[i++ << 2] + alpha_complement)>>8;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_RGBA: /*RGB with alpha*/
            px_rgb.red = infoIn->background_r;
            px_rgb.green = infoIn->background_g;
            px_rgb.blue = infoIn->background_b;
            background_grey = brightness(px_rgb);

            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    j = i++ << 3;
                    alpha = in[j + 6];
                    alpha_complement = (256 - alpha)*background_grey;
                    px_rgb.red = in[j];
                    px_rgb.green = in[j + 2];
                    px_rgb.blue = in[j + 4];
                    *pixel++ = (alpha * brightness(px_rgb) + alpha_complement)>>8;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        default:
            break;
        }
    }
    else /*infoIn->bitDepth is less than 8 bit per channel*/
    {
        switch (infoIn->color.colorType)
        {
        case PNG_COLORTYPE_GREY: /*greyscale color*/
            i = 0;
            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    value = readBitsFromReversedStream(&bp, in, infoIn->color.bitDepth);

                    /* tRNS and bKGD */
                    if (infoIn->color.key_defined)
                        if ( value == infoIn->color.key_r )
                            value = infoIn->background_r; /* full transparent */

                    /*scale value from 0 to 255*/
                    value = (value * 255) / ((1 << infoIn->color.bitDepth) - 1);

                    *pixel++ = (unsigned char)value;
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        case PNG_COLORTYPE_PALETTE: /*indexed color (palette)*/
            i = 0;
            px_rgb.red = infoIn->background_r;
            px_rgb.green = infoIn->background_g;
            px_rgb.blue = infoIn->background_b;
            uint8_t background_grey = brightness(px_rgb);

            for (y = 0 ; y < h ; y++) {
                pixel = line_buf;
                for (x = 0 ; x < w ; x++) {
                    value = readBitsFromReversedStream(&bp, in, infoIn->color.bitDepth);
                    if (value >= infoIn->color.palettesize)
                    {
                        decoder->error = 47;
                        return;
                    }

                    j = value << 2;

                    /* tRNS and bKGD */
                    alpha = infoIn->color.palette[j + 3];
                    alpha_complement = (256 - alpha) * background_grey;

                    px_rgb.red = (alpha * infoIn->color.palette[j] + 
                                  alpha_complement)>>8;
                    px_rgb.green = (alpha * infoIn->color.palette[j + 1] + 
                                    alpha_complement)>>8;
                    px_rgb.blue = (alpha * infoIn->color.palette[j + 2] + 
                                   alpha_complement)>>8;
                    *pixel++ = brightness(px_rgb);
                }
                output_row_8(y,(void *)line_buf,&ctx);
            }
            break;
        default:
            break;
        }
    }
#endif
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

static const uint8_t ADAM7_IX[7] = { 0, 4, 0, 2, 0, 1, 0 }; /*x start values*/
static const uint8_t ADAM7_IY[7] = { 0, 0, 4, 0, 2, 0, 1 }; /*y start values*/
static const uint8_t ADAM7_DX[7] = { 8, 8, 4, 4, 2, 2, 1 }; /*x delta values*/
static const uint8_t ADAM7_DY[7] = { 8, 8, 8, 4, 4, 2, 2 }; /*y delta values*/

static void Adam7_getpassvalues(uint16_t passw[7],
                                uint16_t passh[7], 
                                size_t filter_passstart[8], 
                                size_t padded_passstart[8], 
                                size_t passstart[8], 
                                uint16_t w,
                                uint16_t h,
                                uint8_t bpp)
{
    /* the passstart values have 8 values: 
     * the 8th one actually indicates the byte after the end 
     * of the 7th (= last) pass
     */
    uint8_t i;

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
        /* if passw[i] is 0, it's 0 bytes, not 1 (no filtertype-byte) */
        filter_passstart[i + 1] = filter_passstart[i] + ((passw[i] && passh[i])?
                                  passh[i] * (1 + (passw[i] * bpp + 7) / 8):0);

        /* bits padded if needed to fill full byte at end of each scanline */
        padded_passstart[i + 1] = padded_passstart[i] + 
                                  passh[i] * ((passw[i] * bpp + 7) / 8);

        /* only padded at end of reduced image */
        passstart[i + 1] = passstart[i] + (passh[i] * passw[i] * bpp + 7) / 8;
    }
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Decoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */



static uint8_t unfilterScanline(uint8_t* recon,
                                const uint8_t* scanline,
                                const uint8_t* precon,
                                size_t bytewidth,
                                uint8_t filterType,
                                size_t length)
{
    /* For PNG filter method 0
     * unfilter a PNG image scanline by scanline. when the pixels are smaller
     * than 1 byte, the filter works byte per byte (bytewidth = 1)
     *
     * precon is the previous unfiltered scanline, recon the result,
     * scanline the current one
     *
     * the incoming scanlines do NOT include the filtertype byte, that one is
     * given in the parameter filterType instead
     *
     * recon and scanline MAY be the same memory address! precon must be
     * disjoint.
     */

    /* storage space for cached portion of scanline */
    unsigned char cache[512+16];

    /* ptr to second element of the cache */
    unsigned char *cache_1 = cache + bytewidth;
    unsigned char *p_cache = cache + 256 + 8; /* half way */
    unsigned char *p_cache_1 = p_cache + bytewidth;

    size_t i;
    switch (filterType)
    {
    case PNG_FILTERTYPE_NONE:
        /* for(i = 0; i < length; i++) recon[i] = scanline[i]; */
        memcpy(recon, scanline, length * sizeof(uint8_t));
        break;
    case PNG_FILTERTYPE_SUB:
        /*
        for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
        for (i = bytewidth; i < length; i++)
            recon[i] = scanline[i] + recon[i - bytewidth];
        */

        /* first pixel */
        memcpy(cache, scanline, bytewidth * sizeof(unsigned char));
        scanline += bytewidth;

        while ((length - bytewidth) >> 9) /* length >> 9 */
        {
            /* cache part of scanline */
            memcpy(cache_1, scanline, 512);

            /* filtering */
            for (i=bytewidth; i < 512 + bytewidth; i++)
                cache[i] += cache[i - bytewidth];

            /* copy part of filtered scanline */
            memcpy(recon, cache, 512);

            /* adjust pointers */
            recon += 512;
            scanline += 512;
            length -= 512;

            /* copy last pixel back to the begining of the cache */
            memcpy(cache, cache + 512, bytewidth * sizeof(unsigned char));
        }

        /* less than our cache size */
        if (length)
        {
            /* cache last part of the scanline */
            memcpy(cache_1, scanline, length - bytewidth);

            /* filtering */
            for (i=bytewidth; i < length; i++)
                cache[i] += cache[i - bytewidth];

            /* copy remaining part of the filtered scanline */
            memcpy(recon, cache, length * sizeof(unsigned char));
        }
        break;
    case PNG_FILTERTYPE_UP:
        /*
        if (precon) for (i = 0; i < length; i++)
            recon[i] = scanline[i] + precon[i];
        */
        if (precon)
        {
            while (length >> 8)
            {
                memcpy(cache, scanline, 256);
                memcpy(p_cache, precon, 256);

                for (i=0; i < 256; i++)
                    cache[i] += p_cache[i];

                memcpy(recon, cache, 256);

                scanline += 256;
                recon += 256;
                precon += 256;
                length -= 256;
            }

            if (length)
            {
                memcpy(cache, scanline, length);
                memcpy(p_cache, precon, length);

                for (i=0; i < length; i++)
                    cache[i] += p_cache[i];

                memcpy(recon, cache, length);
            }
        }
        else
            /* for(i = 0; i < length; i++) recon[i] = scanline[i]; */
            memcpy(recon, scanline, length * sizeof(uint8_t));
        break;
    case PNG_FILTERTYPE_AVERAGE:
        if (precon)
        {
            /*
            for (i = 0; i < bytewidth; i++)
                recon[i] = scanline[i] + precon[i] / 2;
            for (i = bytewidth; i < length; i++)
                recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
            */
            memcpy(cache, scanline, bytewidth * sizeof(unsigned char));
            memcpy(p_cache, precon, bytewidth * sizeof(unsigned char));

            for (i = 0; i < bytewidth; i++)
                cache[i] += p_cache[i]>>1;

            scanline += bytewidth;
            precon += bytewidth;

            while ((length - bytewidth)>> 8) /* length/256 */
            {
                memcpy(cache_1, scanline, 256);
                memcpy(p_cache_1, precon, 256);

                for (i=bytewidth; i < 256 + bytewidth; i++)
                    cache[i] += (cache[i - bytewidth] + p_cache[i])>>1;

                memcpy(recon, cache, 256);

                recon += 256;
                scanline += 256;
                precon += 256;
                length -= 256;

                memcpy(cache, cache + 256, bytewidth * sizeof(unsigned char));
                memcpy(p_cache, p_cache + 256, bytewidth * sizeof(unsigned char));
            }

            /* less than our cache size */
            if (length)
            {
                /* cache last part of the scanline */
                memcpy(cache_1, scanline, length - bytewidth);
                memcpy(p_cache_1, precon, length - bytewidth);

                /* filtering */
                for (i=bytewidth; i < length; i++)
                    cache[i] += (cache[i - bytewidth] + p_cache[i])>>1;

                /* copy remaining part of the filtered scanline */
                memcpy(recon, cache, length * sizeof(unsigned char));
            }
        }
        else
        {
            /*
            for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
            for (i = bytewidth; i < length; i++)
                recon[i] = scanline[i] + recon[i - bytewidth] / 2;
            */

            /* first pixel */
            memcpy(cache, scanline, bytewidth * sizeof(unsigned char));
            scanline += bytewidth;

            while ((length - bytewidth) >> 9) /* length/512 */
            {
                /* cache part of scanline */
                memcpy(cache_1, scanline, 512);

                /* filtering */
                for (i=bytewidth; i < 512 + bytewidth; i++)
                    cache[i] += (cache[i - bytewidth])>>1;

                /* copy part of filtered scanline */
                memcpy(recon, cache, 512);

                /* adjust pointers */
                recon += 512;
                scanline += 512;
                length -= 512;

                /* copy last pixel back to the begining of the cache */
                memcpy(cache, cache + 512, bytewidth * sizeof(unsigned char));
            }

            /* less than our cache size */
            if (length)
            {
                /* cache last part of the scanline */
                memcpy(cache_1, scanline, length - bytewidth);

                /* filtering */
                for (i=bytewidth; i < length; i++)
                    cache[i] += (cache[i - bytewidth])>>1;

                /* copy remaining part of the filtered scanline */
                memcpy(recon, cache, length * sizeof(unsigned char));
            }
        }
        break;
    case PNG_FILTERTYPE_PAETH:
        if (precon)
        {
            /*
            for (i = 0; i < bytewidth; i++)
                recon[i] = (uint8_t)(scanline[i] + 
                                     paethPredictor(0, precon[i], 0));
            for (i = bytewidth; i < length; i++)
                recon[i] = (uint8_t)(scanline[i] + 
                                     paethPredictor(recon[i - bytewidth],
                                                    precon[i],
                                                    precon[i - bytewidth]));
            */

            memcpy(cache, scanline, bytewidth * sizeof(unsigned char));
            memcpy(p_cache, precon, bytewidth * sizeof(unsigned char));

            for (i = 0; i < bytewidth; i++)
                cache[i] += paethPredictor(0, p_cache[i], 0);

            scanline += bytewidth;
            precon += bytewidth;

            while ((length - bytewidth)>> 8) /* length/256 */
            {
                memcpy(cache_1, scanline, 256);
                memcpy(p_cache_1, precon, 256);

                for (i=bytewidth; i < 256 + bytewidth; i++)
                    cache[i] += paethPredictor(cache[i - bytewidth], 
                                               p_cache[i], 
                                               p_cache[i - bytewidth]);

                memcpy(recon, cache, 256);

                recon += 256;
                scanline += 256;
                precon += 256;
                length -= 256;

                memcpy(cache, cache + 256, bytewidth * sizeof(unsigned char));
                memcpy(p_cache, p_cache + 256, bytewidth * sizeof(unsigned char));
            }

            /* less than our cache size */
            if (length)
            {
                /* cache last part of the scanline */
                memcpy(cache_1, scanline, length - bytewidth);
                memcpy(p_cache_1, precon, length - bytewidth);

                /* filtering */
                for (i=bytewidth; i < length; i++)
                    cache[i] += paethPredictor(cache[i - bytewidth],
                                               p_cache[i],
                                               p_cache[i - bytewidth]);

                /* copy remaining part of the filtered scanline */
                memcpy(recon, cache, length * sizeof(unsigned char));
            }
        }
        else
        {
            /*
            for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
            for (i = bytewidth; i < length; i++)
                recon[i] = (uint8_t)(scanline[i] +
                                     paethPredictor(recon[i - bytewidth],
                                                    0, 0));
            */

            memcpy(cache, scanline, bytewidth * sizeof(unsigned char));
            scanline += bytewidth;

            while ((length - bytewidth) >> 9) /* length/512 */
            {
                /* cache part of scanline */
                memcpy(cache_1, scanline, 512);

                /* filtering */
                for (i=bytewidth; i < 512 + bytewidth; i++)
                    cache[i] += paethPredictor(cache[i - bytewidth], 0, 0);

                /* copy part of filtered scanline */
                memcpy(recon, cache, 512);

                /* adjust pointers */
                recon += 512;
                scanline += 512;
                length -= 512;

                /* copy last pixel back to the begining of the cache */
                memcpy(cache, cache + 512, bytewidth * sizeof(unsigned char));
            }

            /* less than our cache size */
            if (length)
            {
                /* cache last part of the scanline */
                memcpy(cache_1, scanline, length - bytewidth);

                /* filtering */
                for (i=bytewidth; i < length; i++)
                    cache[i] += paethPredictor(cache[i - bytewidth], 0, 0);

                /* copy remaining part of the filtered scanline */
                memcpy(recon, cache, length * sizeof(unsigned char));
            }
        }
        break;
    default:
        return 36; /*error: unexisting filter type given*/
    }
    return 0;
}

static uint8_t unfilter(uint8_t* out,
                        const uint8_t* in,
                        uint16_t w,
                        uint16_t h,
                        uint8_t bpp)
{
    /* For PNG filter method 0
     * this function unfilters a single image (e.g. without interlacing this is
     * called once, with Adam7 it's called 7 times)
     *
     * out must have enough bytes allocated already, in must have the
     * scanlines + 1 filtertype byte per scanline
     *
     * w and h are image dimensions or dimensions of reduced image,
     * bpp is bits per pixel
     *
     * in and out are allowed to be the same memory address!
     */

    uint16_t y;
    uint8_t* prevline = 0;

    /* bytewidth is used for filtering, is 1 when bpp < 8,
     * number of bytes per pixel otherwise
     */
    size_t bytewidth = (bpp + 7) / 8; 
    size_t linebytes = (w * bpp + 7) / 8;

    for (y = 0; y < h; y++)
    {
        size_t outindex = linebytes * y;

        /* the extra filterbyte added to each row */
        size_t inindex = (1 + linebytes) * y;
        uint8_t filterType = in[inindex];

        uint8_t error = unfilterScanline(&out[outindex], &in[inindex + 1],
                                         prevline, bytewidth, filterType,
                                         linebytes);
        if (error)
            return error;

        prevline = &out[outindex];
    }

    return 0;
}

static void Adam7_deinterlace(uint8_t* out,
                              const uint8_t* in,
                              uint16_t w,
                              uint16_t h,
                              uint8_t bpp)
{
    /* Note: this function works on image buffers WITHOUT padding bits at end
     * of scanlines with non-multiple-of-8 bit amounts, only between reduced
     * images is padding
     * out must be big enough AND must be 0 everywhere if bpp < 8
     * in the current implementation (because that's likely a little bit faster)
     */
    uint16_t passw[7], passh[7];
    size_t filter_passstart[8], padded_passstart[8], passstart[8];
    uint8_t i;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart,
                        passstart, w, h, bpp);

    if (bpp >= 8)
    {
        for (i = 0; i < 7; i++)
        {
            uint16_t x, y, b;
            size_t bytewidth = bpp >> 3;
            for (y = 0; y < passh[i]; y++)
                for (x = 0; x < passw[i]; x++)
                {
                    size_t pixelinstart = passstart[i] + 
                                          (y * passw[i] + x) * bytewidth;
                    size_t pixeloutstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w +
                                             ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
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
            uint16_t x, y, b;
            uint32_t ilinebits = bpp * passw[i];
            uint32_t olinebits = bpp * w;
            size_t obp, ibp; /*bit pointers (for out and in buffer)*/
            for (y = 0; y < passh[i]; y++)
                for (x = 0; x < passw[i]; x++)
                {
                    ibp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
                    obp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + 
                           (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
                    for (b = 0; b < bpp; b++)
                    {
                        uint8_t bit = readBitFromReversedStream(&ibp, in);
                        /* note that this function assumes the out buffer 
                         * is completely 0, use setBitOfReversedStream
                         * otherwise*/
                        setBitOfReversedStream0(&obp, out, bit);
                    }
                }
        }
    }
}

static void removePaddingBits(uint8_t* out,
                              const uint8_t* in,
                              size_t olinebits,
                              size_t ilinebits,
                              uint16_t h)
{
    /* After filtering there are still padding bits if scanlines have
     * non multiple of 8 bit amounts. They need to be removed 
     * (except at last scanline of (Adam7-reduced) image) before working
     * with pure image buffers for the Adam7 code, the color convert code
     * and the output to the user.
     *
     * in and out are allowed to be the same buffer, in may also be higher
     * but still overlapping; in must have >= ilinebits*h bits,
     * out must have >= olinebits*h bits, olinebits must be <= ilinebits
     * also used to move bits after earlier such operations happened, e.g.
     * in a sequence of reduced images from Adam7
     * only useful if (ilinebits - olinebits) is a value in the range 1..7
     */
    uint16_t y;
    size_t diff = ilinebits - olinebits;
    size_t obp = 0, ibp = 0; /*bit pointers*/
    for (y = 0; y < h; y++)
    {
        size_t x;
        for (x = 0; x < olinebits; x++)
        {
            uint8_t bit = readBitFromReversedStream(&ibp, in);
            setBitOfReversedStream(&obp, out, bit);
        }
        ibp += diff;
    }
}

/* out must be buffer big enough to contain full image,
 * and in must contain the full decompressed data from the IDAT chunks
 */
static uint8_t postProcessScanlines(uint8_t* out,
                                    uint8_t* in,
                                    const LodePNG_Decoder* decoder) 
{
   /*return value is error*/

   /* This function converts the filtered-padded-interlaced data into pure 2D
    * image buffer with the PNG's colortype.
    * Steps:
    * I) if no Adam7:
    *     1) unfilter
    *     2) remove padding bits (= posible extra bits per scanline if bpp < 8)
    * II) if adam7:
    *     1) 7x unfilter
    *     2) 7x remove padding bits 
    *     3) Adam7_deinterlace
    *
    * NOTE: the in buffer will be overwritten with intermediate data!
    */
    uint8_t bpp = getBpp(decoder->infoPng.color.colorType,
                          decoder->infoPng.color.bitDepth);
    uint16_t w = decoder->infoPng.width;
    uint16_t h = decoder->infoPng.height;
    uint8_t error = 0;

    if (bpp == 0)
        return 31; /*error: invalid colortype*/

    if (decoder->infoPng.interlaceMethod == 0)
    {
        if (bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
        {
            error = unfilter(in, in, w, h, bpp);
            if (error) return error;
            removePaddingBits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
        }
        else
            /* we can immediatly filter into the out buffer,
             * no other steps needed
             */
            error = unfilter(out, in, w, h, bpp);
        }
    else /*interlaceMethod is 1 (Adam7)*/
    {
        uint16_t passw[7], passh[7];
        size_t filter_passstart[8], padded_passstart[8], passstart[8];
        uint8_t i;

        Adam7_getpassvalues(passw,
                            passh,
                            filter_passstart,
                            padded_passstart,
                            passstart,
                            w,
                            h,
                            bpp);

        for (i = 0; i < 7; i++)
        {
            error = unfilter(&in[padded_passstart[i]],
                             &in[filter_passstart[i]],
                             passw[i],
                             passh[i],
                             bpp);
            if (error)
                return error;
            if (bpp < 8)
            /* TODO: possible efficiency improvement: if in this reduced
             * image the bits fit nicely in 1 scanline, move bytes instead
             * of bits or move not at all
             */
            {
                /* remove padding bits in scanlines; after this there still
                 * may be padding bits between the different reduced images:
                 * each reduced image still starts nicely at a byte
                 */
                removePaddingBits(&in[passstart[i]],
                                  &in[padded_passstart[i]],
                                  passw[i] * bpp,
                                  ((passw[i] * bpp + 7) / 8) * 8,
                                  passh[i]);
            }
        }

        Adam7_deinterlace(out, in, w, h, bpp);
    }

    return error;
}

/* read a PNG, the result will be in the same color type as the PNG
 * (hence "generic")
 */
static void decodeGeneric(LodePNG_Decoder* decoder)
{
    uint8_t *in = decoder->file;

    uint8_t IEND = 0;
    const uint8_t* chunk;
    size_t i;

    size_t chunkLength;  /* chunk length */
    const uint8_t* data; /*the data in the chunk*/

    uint8_t *idat = decoder->buf; /* allocated buffer */

    size_t idat_size = 0;

    signed long free_mem = decoder->buf_size;

    /* for unknown chunk order */
    bool unknown = false;
    uint8_t critical_pos = 1; /*1 = after IHDR, 2 = after PLTE, 3 = after IDAT*/

    if (decoder->file_size == 0 || in == NULL)
    {
        /* the given data is empty */
        decoder->error = 48;
        return;
    }

    chunk = in + 33; /*first byte of the first chunk after the header*/

    /* loop through the chunks, ignoring unknown chunks and stopping at IEND
     *  chunk. IDAT data is put at the start of the in buffer
     */
    while (!IEND)     {

        /* minimal size of chunk is 12 bytes */
        if ((size_t)((chunk - in) + 12) > decoder->file_size || chunk < in)
        {
            /* error: size of the in buffer too small to contain next chunk */
            decoder->error = 30;
            break;
        }

        /* length of the data of the chunk, excluding the length bytes,
         * chunk type and CRC bytes
         *
         * data field of the chunk is restricted to 2^31-1 bytes in size
         */
        chunkLength = LodePNG_chunk_length(chunk);

        if (chunkLength > 2147483647)
        {
            decoder->error = 63;
            break;
        }

        /* check if chunk fits in buffer */
        if ((size_t)((chunk - in) + chunkLength + 12) > decoder->file_size ||
            (chunk + chunkLength + 12) < in)
        {
            /* error: size of the in buffer too small to contain next chunk */
            decoder->error = 35;
            break;
         }
         data = LodePNG_chunk_data(chunk);

        /* IDAT chunk, containing compressed image data
         * there may be more than 1 IDAT chunk, complete
         * compressed stream is concatenation of consecutive
         * chunks data
         */
        if (LodePNG_chunk_type_equals(chunk, PNG_CHUNK_IDAT))
        {
            free_mem -= chunkLength;

            if (free_mem < 0)
            {
                decoder->error = OUT_OF_MEMORY;
                break;
            }
            /* copy compressed data */
            memcpy(idat+idat_size, data, chunkLength * sizeof(uint8_t));
            idat_size += chunkLength;
            critical_pos = 3;
        }
        /*IEND chunk*/
        else if (LodePNG_chunk_type_equals(chunk, PNG_CHUNK_IEND))
        {
            IEND = 1;
        }
        /*palette chunk (PLTE)*/
        else if (LodePNG_chunk_type_equals(chunk, PNG_CHUNK_PLTE))
        {
            uint32_t pos = 0;
            decoder->infoPng.color.palettesize = chunkLength / 3;
            if (decoder->infoPng.color.palettesize > 256)
            {
                /*error: palette too big*/
                decoder->error = 38;
                break;
            }

            for (i = 0; i < decoder->infoPng.color.palettesize; i++)
            {
                decoder->infoPng.color.palette[(i<<2)] = data[pos++]; /*R*/
                decoder->infoPng.color.palette[(i<<2) | 1] = data[pos++]; /*G*/
                decoder->infoPng.color.palette[(i<<2) | 2] = data[pos++]; /*B*/
                decoder->infoPng.color.palette[(i<<2) | 3] = 255; /*alpha*/
            }
            critical_pos = 2;
        }
        /*palette transparency chunk (tRNS)*/
        else if (LodePNG_chunk_type_equals(chunk, PNG_CHUNK_tRNS))
        {
            if (decoder->infoPng.color.colorType == PNG_COLORTYPE_PALETTE)
            {
                if (chunkLength > decoder->infoPng.color.palettesize)
                {
                    /* error: more alpha values given than there are palette
                     * entries
                     */
                    decoder->error = 39;
                    break;
                }
                for (i = 0; i < chunkLength; i++)
                    /* copy alpha informations for palette colors */
                    decoder->infoPng.color.palette[(i<<2) | 3] = data[i];
            }
            else if (decoder->infoPng.color.colorType == PNG_COLORTYPE_GREY)
            {
                if (chunkLength != 2)
                {
                    /* error: this chunk must be 2 bytes for greyscale image */
                    decoder->error = 40;
                    break;
                }
                /* transparent color definition */
                decoder->infoPng.color.key_defined = 1;
                decoder->infoPng.color.key_r = 
                decoder->infoPng.color.key_g = 
                decoder->infoPng.color.key_b = data[0]<<8|data[1];
            }
            else if (decoder->infoPng.color.colorType == PNG_COLORTYPE_RGB)
            {
                if (chunkLength != 6)
                {
                    /* error: this chunk must be 6 bytes for RGB image */
                    decoder->error = 41;
                    break;
                }
                /* transparent color definition */ 
                decoder->infoPng.color.key_defined = 1;
                decoder->infoPng.color.key_r = data[0]<<8|data[1];
                decoder->infoPng.color.key_g = data[2]<<8|data[3];
                decoder->infoPng.color.key_b = data[4]<<8|data[5];
            }
            else 
            {
                /* error: tRNS chunk not allowed for other color models */
                decoder->error = 42;
                break;
            }
        }
        /*background color chunk (bKGD)*/
        else if (LodePNG_chunk_type_equals(chunk, PNG_CHUNK_bKGD))
        {
            if (decoder->infoPng.color.colorType == PNG_COLORTYPE_PALETTE)
            {
                if (chunkLength != 1)
                {
                    /* error: this chunk must be 1 byte for indexed color image */
                    decoder->error = 43;
                    break;
                }
                decoder->infoPng.background_r = 
                    decoder->infoPng.color.palette[(data[0]<<2)];

                decoder->infoPng.background_g =
                    decoder->infoPng.color.palette[(data[0]<<2) | 1];

                decoder->infoPng.background_b =
                    decoder->infoPng.color.palette[(data[0]<<2) | 2];

            }
            else if (decoder->infoPng.color.colorType == PNG_COLORTYPE_GREY || 
                     decoder->infoPng.color.colorType == PNG_COLORTYPE_GREYA)
            {
                if (chunkLength != 2)
                {
                    /* error: this chunk must be 2 bytes for greyscale image */
                    decoder->error = 44;
                    break;
                }
                decoder->infoPng.background_r = 
                decoder->infoPng.background_g = 
                decoder->infoPng.background_b = data[0];
            }
            else if (decoder->infoPng.color.colorType == PNG_COLORTYPE_RGB || 
                     decoder->infoPng.color.colorType == PNG_COLORTYPE_RGBA)
            {
                if (chunkLength != 6)
                {
                    /* error: this chunk must be 6 bytes for greyscale image */
                    decoder->error = 45;
                    break;
                }
                decoder->infoPng.background_r = data[0];
                decoder->infoPng.background_g = data[2];
                decoder->infoPng.background_b = data[4];
            }
        }
        else
        {
            /* it's not an implemented chunk type,
             * so ignore it (unless it is critical)
             * skip over the data
             */
            if (LodePNG_chunk_critical(chunk))
            {
                /* error: unknown critical chunk 
                 * (5th bit of first byte of chunk type is 0)
                 */
                decoder->error = 69;
                break;
            }
            unknown = true;
        }

        if (!unknown) /*check CRC if wanted, only on known chunk types*/
        {
            if (!LodePNG_chunk_check_crc(chunk))
            {
                decoder->error = 57;
                break;
            }
        }

        if (!IEND)
            chunk = LodePNG_chunk_next(chunk);
    }

    if (!decoder->error)
    {
        /* ptr to buffer just after concatenated IDATs */
        uint8_t *scanlines = idat + idat_size;
        size_t scanline_size = free_mem;

        /* decompress with the Zlib decompressor
         * decompressor updates scanlines_size to actual size
         * of decompressed data
         */
        decoder->error = LodePNG_decompress(scanlines,
                                            &scanline_size,
                                            idat,
                                            idat_size);

        free_mem -= scanline_size;
        /* possible memory saving (at cost of memcpy)
         * memcpy(decoder->buf - scanlines_size, 
         *        scanlines,
         *        scanlines_size * sizeof(uint8_t));
         * this will free compressed IDATs and 
         * will trash raw PNG file (it is trashed anyway
         */
        if (!decoder->error)
        {
            /* size of decoded image in bytes rounded up */
            size_t decoded_img_size = (decoder->infoPng.height * 
                                       decoder->infoPng.width * 
                                       getBpp(decoder->infoPng.color.colorType,
                                              decoder->infoPng.color.bitDepth) + 
                                       7) / 8;

            /* at this time buffer contains:
             * compressed IDATs
             * decompressed IDATs
             * png raw file at the end of the buffer (not needed any more )
             */
            free_mem -= decoded_img_size;

            if (free_mem < 0)
            {
                decoder->error = OUT_OF_MEMORY;
                return;
            }

            /* ptr to decoded png image
             * this will overwrite raw png file loaded into memory
             * decoded image is put in the end of allocated buffer
             */
            decoder->decoded_img = decoder->buf +
                                   decoder->buf_size - 
                                   decoded_img_size;

            /* clear memory as filters assume 0'ed memory */
            memset(decoder->decoded_img,0,decoded_img_size*sizeof(uint8_t));

            decoder->error = postProcessScanlines(decoder->decoded_img,
                                                  scanlines,
                                                  decoder);
        }
    }
}

/* Public functions */

/* read the information from the header and store it in the decoder
 * context struct
 * value is error
 */
void LodePNG_inspect(LodePNG_Decoder* decoder, uint8_t *in, size_t inlength)
{
    uint32_t header_crc, checksum;
    if (inlength == 0 || in == NULL)
    {
        /* the given data is empty */
        decoder->error = 48;
        return;
    }

    if (inlength < 29)
    {
        /*error: the data length is smaller than the length of the header*/
        decoder->error = 27;
        return;
    }

    /* when decoding a new PNG image, make sure all parameters created after
     * previous decoding are reset
     */
    LodePNG_InfoPng_cleanup(&decoder->infoPng);
    LodePNG_InfoPng_init(&decoder->infoPng);
    decoder->error = 0;

    decoder->file = in;
    decoder->file_size = inlength;

    if (in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71 ||
        in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10) 
    {
        /* error: the first 8 bytes are not the correct PNG signature */
        decoder->error = 28;
        return;
    }

    if (in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R')
    {
        /* error: it doesn't start with a IHDR chunk! */
        decoder->error = 29;
        return;
    }

    /* read the values given in the header */
    decoder->infoPng.width = in[16]<<24|in[17]<<16|in[18]<<8|in[19];
    decoder->infoPng.height = in[20]<<24|in[21]<<16|in[22]<<8|in[23];
    decoder->infoPng.color.bitDepth = in[24];
    decoder->infoPng.color.colorType = in[25];
    decoder->infoPng.compressionMethod = in[26];
    decoder->infoPng.filterMethod = in[27];
    decoder->infoPng.interlaceMethod = in[28];

    /* get the value from the chunk's crc field */
    header_crc = in[29]<<24|in[30]<<16|in[31]<<8|in[32];

    /* calculate crc of the header chunk */
    checksum = tinf_crc32(in + 12, 17);

    if (header_crc != checksum)
    {
        decoder->error = 57;
        return;
    }

    if (decoder->infoPng.compressionMethod != 0)
    {
        /* error: only compression method 0 is allowed in the specification */
        decoder->error = 32;
        return;
    }

    if (decoder->infoPng.filterMethod != 0)
    {
       /* error: only filter method 0 is allowed in the specification */
        decoder->error = 33;
        return;
    }

    if (decoder->infoPng.interlaceMethod > 1)
    {
        /* error: only interlace methods 0 and 1 exist in the specification */
        decoder->error = 34;
        return;
    }

    /* check validity of colortype and bitdepth combination */
    decoder->error = checkColorValidity(decoder->infoPng.color.colorType,
                                        decoder->infoPng.color.bitDepth);
}

void LodePNG_decode(LodePNG_Decoder* decoder,
                    uint8_t* in,
                    size_t insize,
                    void (*pf_progress)(int current, int total))
{
    size_t line_buf_size;
    /* parse header */
    LodePNG_inspect(decoder, in, insize);
    
    
    /* Check memory available against worst case where
     * we have to have decoded PNG image
     * and converted to the native pixel format image
     * in buffer at the same time (do we realy need that much?)
     */
    
    size_t decoded_img_size = (decoder->infoPng.height *
                               decoder->infoPng.width *
                               getBpp(decoder->infoPng.color.colorType,
                                      decoder->infoPng.color.bitDepth) +
                               7) / 8;

    /* one line more as temp buffer for conversion */
#ifdef HAVE_LCD_COLOR
    decoder->native_img_size = decoder->infoPng.width *
                               decoder->infoPng.height * FB_DATA_SZ;
    line_buf_size = decoder->infoPng.width * sizeof(struct uint8_rgb);
#else
    decoder->native_img_size = decoder->infoPng.width *
                               decoder->infoPng.height;
    line_buf_size = decoder->infoPng.width;
#endif

    if (decoded_img_size + decoder->native_img_size + line_buf_size
        > decoder->buf_size)
    {
        decoder->error = OUT_OF_MEMORY;
        return;
    }

    if (pf_progress != NULL)
        pf_progress(0, 100);

    long time = *rb->current_tick;
    /* put decoded png data (pure 2D array of pixels in format
     * defined by PNG header at the end of the allocated buffer
     */
    decodeGeneric(decoder);
    if (decoder->error) return;

    if (pf_progress != NULL)
        pf_progress(50, 100);

    /* convert decoded png data into native rockbox
     * pixel format (native LCD data for color
     * or greylib pixel format for greyscale)
     *
     * converted image will be put at the begining
     * of the allocated buffer
     */
    LodePNG_convert(decoder);

    /* correct aspect ratio */
#if (LCD_PIXEL_ASPECT_HEIGHT != 1 || LCD_PIXEL_ASPECT_WIDTH != 1)
    struct bitmap img_src, img_dst;    /* scaler vars */
    struct dim dim_src, dim_dst;       /* recalc_dimensions vars */
    unsigned int c_native_img_size;    /* size of the image after correction */

    dim_src.width = decoder->infoPng.width;
    dim_src.height = decoder->infoPng.height;

    dim_dst.width = decoder->infoPng.width;
    dim_dst.height = decoder->infoPng.height;

    /* defined in apps/recorder/resize.c */
    if (!recalc_dimension(&dim_dst, &dim_src))
    {
        /* calculate 'corrected' image size */
#ifdef HAVE_LCD_COLOR
        c_native_img_size = dim_dst.width * dim_dst.height * FB_DATA_SZ;
#else
        c_native_img_size = dim_dst.width * dim_dst.height;
#endif
        /* check memory constraints
         * do the correction only if there is enough
         * free memory
         */
        if ( decoder->native_img_size + c_native_img_size <=
             decoder->buf_size )
        {
            img_src.width = dim_src.width;
            img_src.height = dim_src.height;
            img_src.data = (unsigned char *)decoder->buf;

            img_dst.width = dim_dst.width;
            img_dst.height = dim_dst.height;
            img_dst.data = (unsigned char *)decoder->buf +
                           decoder->native_img_size;

            /* scale the bitmap to correct physical
             * pixel dimentions
             */
            resize_bitmap(&img_src, &img_dst);

            /* update decoder struct */
            decoder->infoPng.width = img_dst.width;
            decoder->infoPng.height = img_dst.height;
            decoder->native_img_size = c_native_img_size;

            /* copy back corrected image to the begining of the buffer */
            memcpy(img_src.data, img_dst.data, decoder->native_img_size);
        }
    }
#endif /* (LCD_PIXEL_ASPECT_HEIGHT != 1 || LCD_PIXEL_ASPECT_WIDTH != 1) */

    time = *rb->current_tick - time;
    if (pf_progress) pf_progress(100, 100);
}

void LodePNG_Decoder_init(LodePNG_Decoder* decoder,
                          uint8_t *buf,
                          size_t buf_size)
{
    LodePNG_InfoPng_init(&decoder->infoPng);
    decoder->error = 0;
    decoder->buf = buf;
    decoder->buf_size = buf_size;
    decoder->decoded_img = NULL;
    decoder->file = NULL;
    decoder->file_size = 0;
}

const char* LodePNG_perror(LodePNG_Decoder *decoder)
{
    if (decoder->error >= PNG_ERROR_MIN && decoder->error <= PNG_ERROR_MAX)
        return png_error_messages[decoder->error-PNG_ERROR_MIN];
    else
        return NULL;
}
