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

#define OUT_OF_MEMORY   9900
#define FILE_TOO_LARGE  9910

/* PNG chunk types signatures */
/* critical chunks */
#define PNG_CHUNK_IHDR 0x49484452
#define PNG_CHUNK_PLTE 0x504c5445
#define PNG_CHUNK_IDAT 0x49444154
#define PNG_CHUNK_IEND 0x49454e44

/* ancillary chunks */
#define PNG_CHUNK_bKGD 0x624b4744
#define PNG_CHUNK_cHRM 0x6348524d
#define PNG_CHUNK_gAMA 0x67414d41
#define PNG_CHUNK_hIST 0x68495354
#define PNG_CHUNK_iCCP 0x69434350
#define PNG_CHUNK_pHYs 0x70485973
#define PNG_CHUNK_sBIT 0x73424954
#define PNG_CHUNK_sPLT 0x73504c54
#define PNG_CHUNK_sRGB 0x73524742
#define PNG_CHUNK_tEXt 0x74455874
#define PNG_CHUNK_tIME 0x74494d45
#define PNG_CHUNK_tRNS 0x74524e53
#define PNG_CHUNK_zTXt 0x7a545874

/* PNG color types */
#define PNG_COLORTYPE_GREY    0
#define PNG_COLORTYPE_RGB     2
#define PNG_COLORTYPE_PALETTE 3
#define PNG_COLORTYPE_GREYA   4
#define PNG_COLORTYPE_RGBA    6

/* PNG filter types */
#define PNG_FILTERTYPE_NONE    0
#define PNG_FILTERTYPE_SUB     1
#define PNG_FILTERTYPE_UP      2
#define PNG_FILTERTYPE_AVERAGE 3
#define PNG_FILTERTYPE_PAETH   4

#define PNG_ERROR_MIN 27
#define PNG_ERROR_MAX 74

/* Typedefs */
typedef struct LodePNG_InfoColor /*info about the color type of an image*/
{
    /*header (IHDR)*/
    unsigned colorType; /*color type*/
    unsigned bitDepth;  /*bits per sample*/

    /*palette (PLTE)*/
    unsigned char palette[256 * 4]; /*palette in RGBARGBA... order*/
    size_t palettesize; /* palette size in number of colors 
                         * (amount of bytes is 4 * palettesize)
                         */

    /*transparent color key (tRNS)*/
    unsigned key_defined; /*is a transparent color key given?*/
    unsigned key_r;       /*red component of color key*/
    unsigned key_g;       /*green component of color key*/
    unsigned key_b;       /*blue component of color key*/
} LodePNG_InfoColor;

typedef struct LodePNG_InfoPng /*information about the PNG image, except pixels and sometimes except width and height*/
{
    /*header (IHDR), palette (PLTE) and transparency (tRNS)*/
    unsigned width;             /*width of the image in pixels -  filled in by decoder)*/
    unsigned height;            /*height of the image in pixels - filled in by decoder)*/
    unsigned compressionMethod; /*compression method of the original file*/
    unsigned filterMethod;      /*filter method of the original file*/
    unsigned interlaceMethod;   /*interlace method of the original file*/
    LodePNG_InfoColor color;    /*color type and bits, palette, transparency*/

    /*suggested background color (bKGD)*/
    unsigned background_r;       /*red component of suggested background color*/
    unsigned background_g;       /*green component of suggested background color*/
    unsigned background_b;       /*blue component of suggested background color*/

} LodePNG_InfoPng;

typedef struct LodePNG_Decoder
{
    unsigned char *buf;      /* pointer to the buffer allocated for decoder
                              * filled by LodePNG_Decoder_init()
                              */
    size_t  buf_size;  /* size of the buffer decoder is free to use
                              * filled by LodePNG_Decoder_init()
                              */
    unsigned char *file;     /* ptr to raw png file loaded */
    size_t file_size;        /* size of the raw png file in mem */ 
    unsigned char *decoded_img; /* ptr to decoded PNG image in PNG pixel
                                 * format. set by decodeGeneric()
                                 */
    unsigned int native_img_size; /* size of the image in native pixel 
                                   * format 
                                   */
    LodePNG_InfoPng infoPng; /*info of the PNG image obtained after decoding*/
    long error;
} LodePNG_Decoder;

/* Public functions prototypes */
void LodePNG_Decoder_init(LodePNG_Decoder* decoder,
                          uint8_t *buf,
                          size_t buf_size);

void LodePNG_decode(LodePNG_Decoder* decoder,
                    uint8_t* in,
                    size_t insize,
                    void (*pf_progress)(int current, int total));

void LodePNG_inspect(LodePNG_Decoder* decoder, uint8_t *in, size_t inlength);

const char* LodePNG_perror(LodePNG_Decoder *decoder);
