/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#ifndef _JPEG_JPEG_DECODER_H
#define _JPEG_JPEG_DECODER_H

#define HUFF_LOOKAHEAD 8 /* # of bits of lookahead */

struct derived_tbl
{
    /* Basic tables: (element [0] of each array is unused) */
    long mincode[17]; /* smallest code of length k */
    long maxcode[18]; /* largest code of length k (-1 if none) */
    /* (maxcode[17] is a sentinel to ensure huff_DECODE terminates) */
    int valptr[17]; /* huffval[] index of 1st symbol of length k */

    /* Back link to public Huffman table (needed only in slow_DECODE) */
    int* pub;

    /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
    the input data stream.  If the next Huffman code is no more
    than HUFF_LOOKAHEAD bits long, we can obtain its length and
    the corresponding symbol directly from these tables. */
    int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
    unsigned char look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
};

#define QUANT_TABLE_LENGTH  64

/* for type of Huffman table */
#define DC_LEN 28
#define AC_LEN 178

struct huffman_table
{   /* length and code according to JFIF format */
    int huffmancodes_dc[DC_LEN];
    int huffmancodes_ac[AC_LEN];
};

struct frame_component
{
    int ID;
    int horizontal_sampling;
    int vertical_sampling;
    int quanttable_select;
};

struct scan_component
{
    int ID;
    int DC_select;
    int AC_select;
};

struct bitstream
{
    unsigned long get_buffer; /* current bit-extraction buffer */
    int bits_left; /* # of unused bits in it */
    unsigned char* next_input_byte;
    unsigned char* input_end; /* upper limit +1 */
};

struct jpeg
{
    int x_size, y_size; /* size of image (can be less than block boundary) */
    int x_phys, y_phys; /* physical size, block aligned */
    int x_mbl; /* x dimension of MBL */
    int y_mbl; /* y dimension of MBL */
    int blocks; /* blocks per MB */
    int restart_interval; /* number of MCUs between RSTm markers */
    int store_pos[4]; /* for Y block ordering */

    unsigned char* p_entropy_data;
    unsigned char* p_entropy_end;

    int quanttable[4][QUANT_TABLE_LENGTH]; /* raw quantization tables 0-3 */
    int qt_idct[2][QUANT_TABLE_LENGTH]; /* quantization tables for IDCT */

    struct huffman_table hufftable[2]; /* Huffman tables  */
    struct derived_tbl dc_derived_tbls[2]; /* Huffman-LUTs */
    struct derived_tbl ac_derived_tbls[2];

    struct frame_component frameheader[3]; /* Component descriptor */
    struct scan_component scanheader[3]; /* currently not used */

    int mcu_membership[6]; /* info per block */
    int tab_membership[6];
    int subsample_x[3]; /* info per component */
    int subsample_y[3];
};


/* possible return flags for process_markers() */
#define HUFFTAB   0x0001 /* with huffman table */
#define QUANTTAB  0x0002 /* with quantization table */
#define APP0_JFIF 0x0004 /* with APP0 segment following JFIF standard */
#define FILL_FF   0x0008 /* with 0xFF padding bytes at begin/end */
#define SOF0      0x0010 /* with SOF0-Segment */
#define DHT       0x0020 /* with Definition of huffman tables */
#define SOS       0x0040 /* with Start-of-Scan segment */
#define DQT       0x0080 /* with definition of quantization table */

/* various helper functions */
void default_huff_tbl(struct jpeg* p_jpeg);
void build_lut(struct jpeg* p_jpeg);
int process_markers(unsigned char* p_src, long size, struct jpeg* p_jpeg);

/* the main decode function */
#ifdef HAVE_LCD_COLOR
int jpeg_decode(struct jpeg* p_jpeg, unsigned char* p_pixel[3],
                int downscale, void (*pf_progress)(int current, int total));
#else
int jpeg_decode(struct jpeg* p_jpeg, unsigned char* p_pixel[1], int downscale,
                void (*pf_progress)(int current, int total));
#endif


#endif /* _JPEG_JPEG_DECODER_H */
