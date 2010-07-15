/*
 * Common bit i/o utils
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2010 Loren Merritt
 *
 * alternative bitstream reader & writer by Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/bitstream.c
 * bitstream api.
 */

#include "get_bits.h"
#include "put_bits.h"
#include <string.h>

#define av_log(...)

/* Taken from libavutil/common.h */
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))

/* av_reverse - taken from libavutil/mathematics.c*/
const uint8_t av_reverse[256]={
0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF,
};


const uint8_t ff_log2_run[32]={
 0, 0, 0, 0, 1, 1, 1, 1,
 2, 2, 2, 2, 3, 3, 3, 3,
 4, 4, 5, 5, 6, 6, 7, 7,
 8, 9,10,11,12,13,14,15
};

void align_put_bits(PutBitContext *s)
{
#ifdef ALT_BITSTREAM_WRITER
    put_bits(s,(  - s->index) & 7,0);
#else
    put_bits(s,s->bit_left & 7,0);
#endif
}

void ff_put_string(PutBitContext *pb, const char *string, int terminate_string)
{
    while(*string){
        put_bits(pb, 8, *string);
        string++;
    }
    if(terminate_string)
        put_bits(pb, 8, 0);
}

void ff_copy_bits(PutBitContext *pb, const uint8_t *src, int length)
{
    int words= length>>4;
    int bits= length&15;
    int i;

    if(length==0) return;
/* The following define was just added to make the codec (wma pro) compile */
#define CONFIG_SMALL 0
    if(CONFIG_SMALL || words < 16 || put_bits_count(pb)&7){
        for(i=0; i<words; i++) put_bits(pb, 16, AV_RB16(src + 2*i));
    }else{
        for(i=0; put_bits_count(pb)&31; i++)
            put_bits(pb, 8, src[i]);
        flush_put_bits(pb);
        memcpy(put_bits_ptr(pb), src+i, 2*words-i);
        skip_put_bytes(pb, 2*words-i);
    }

    put_bits(pb, bits, AV_RB16(src + 2*words)>>(16-bits));
}

/* VLC decoding */

//#define DEBUG_VLC

#define GET_DATA(v, table, i, wrap, size) \
{\
    const uint8_t *ptr = (const uint8_t *)table + i * wrap;\
    switch(size) {\
    case 1:\
        v = *(const uint8_t *)ptr;\
        break;\
    case 2:\
        v = *(const uint16_t *)ptr;\
        break;\
    default:\
        v = *(const uint32_t *)ptr;\
        break;\
    }\
}


static int alloc_table(VLC *vlc, int size, int use_static)
{
    int index;
    index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated) {
        if(use_static)
            return -1; //cant do anything, init_vlc() is used with too little memory
        vlc->table_allocated += (1 << vlc->bits);
        //vlc->table = av_realloc(vlc->table,
        //                        sizeof(VLC_TYPE) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

static inline uint32_t bitswap_32(uint32_t x) {
    return av_reverse[x&0xFF]<<24
         | av_reverse[(x>>8)&0xFF]<<16
         | av_reverse[(x>>16)&0xFF]<<8
         | av_reverse[x>>24];
}

typedef struct {
    uint8_t bits;
    uint16_t symbol;
    /** codeword, with the first bit-to-be-read in the msb
     * (even if intended for a little-endian bitstream reader) */
    uint32_t code;
} VLCcode;

static int compare_vlcspec(const void *a, const void *b)
{
    const VLCcode *sa=a, *sb=b;
    return (sa->code >> 1) - (sb->code >> 1);
}

/**
 * Build VLC decoding tables suitable for use with get_vlc().
 *
 * @param vlc            the context to be initted
 *
 * @param table_nb_bits  max length of vlc codes to store directly in this table
 *                       (Longer codes are delegated to subtables.)
 *
 * @param nb_codes       number of elements in codes[]
 *
 * @param codes          descriptions of the vlc codes
 *                       These must be ordered such that codes going into the same subtable are contiguous.
 *                       Sorting by VLCcode.code is sufficient, though not necessary.
 */
static int build_table(VLC *vlc, int table_nb_bits, int nb_codes,
                       VLCcode *codes, int flags)
{
    int table_size, table_index, index, code_prefix, symbol, subtable_bits;
    int i, j, k, n, nb, inc;
    uint32_t code;
    VLC_TYPE (*table)[2];

    table_size = 1 << table_nb_bits;
    table_index = alloc_table(vlc, table_size, flags & INIT_VLC_USE_NEW_STATIC);
#ifdef DEBUG_VLC
    av_log(NULL,AV_LOG_DEBUG,"new table index=%d size=%d\n",
           table_index, table_size);
#endif
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];

    for (i = 0; i < table_size; i++) {
        table[i][1] = 0; //bits
        table[i][0] = -1; //codes
    }

    /* first pass: map codes and compute auxillary table sizes */
    for (i = 0; i < nb_codes; i++) {
        n = codes[i].bits;
        code = codes[i].code;
        symbol = codes[i].symbol;
#if defined(DEBUG_VLC) && 0
        av_log(NULL,AV_LOG_DEBUG,"i=%d n=%d code=0x%x\n", i, n, code);
#endif
        if (n <= table_nb_bits) {
            /* no need to add another table */
            j = code >> (32 - table_nb_bits);
            nb = 1 << (table_nb_bits - n);
            inc = 1;
            if (flags & INIT_VLC_LE) {
                j = bitswap_32(code);
                inc = 1 << n;
            }
            for (k = 0; k < nb; k++) {
#ifdef DEBUG_VLC
                av_log(NULL, AV_LOG_DEBUG, "%4x: code=%d n=%d\n",
                       j, i, n);
#endif
                if (table[j][1] /*bits*/ != 0) {
                    av_log(NULL, AV_LOG_ERROR, "incorrect codes\n");
                    return -1;
                }
                table[j][1] = n; //bits
                table[j][0] = symbol;
                j += inc;
            }
        } else {
            /* fill auxiliary table recursively */
            n -= table_nb_bits;
            code_prefix = code >> (32 - table_nb_bits);
            subtable_bits = n;
            codes[i].bits = n;
            codes[i].code = code << table_nb_bits;
            for (k = i+1; k < nb_codes; k++) {
                n = codes[k].bits - table_nb_bits;
                if (n <= 0)
                    break;
                code = codes[k].code;
                if (code >> (32 - table_nb_bits) != (unsigned)code_prefix)
                    break;
                codes[k].bits = n;
                codes[k].code = code << table_nb_bits;
                subtable_bits = FFMAX(subtable_bits, n);
            }
            subtable_bits = FFMIN(subtable_bits, table_nb_bits);
            j = (flags & INIT_VLC_LE) ? bitswap_32(code_prefix) >> (32 - table_nb_bits) : (unsigned)code_prefix;
            table[j][1] = -subtable_bits;
#ifdef DEBUG_VLC
            av_log(NULL,AV_LOG_DEBUG,"%4x: n=%d (subtable)\n",
                   j, codes[i].bits + table_nb_bits);
#endif
            index = build_table(vlc, subtable_bits, k-i, codes+i, flags);
            if (index < 0)
                return -1;
            /* note: realloc has been done, so reload tables */
            table = &vlc->table[table_index];
            table[j][0] = index; //code
            i = k-1;
        }
    }
    return table_index;
}


/* Build VLC decoding tables suitable for use with get_vlc().

   'nb_bits' set thee decoding table size (2^nb_bits) entries. The
   bigger it is, the faster is the decoding. But it should not be too
   big to save memory and L1 cache. '9' is a good compromise.

   'nb_codes' : number of vlcs codes

   'bits' : table which gives the size (in bits) of each vlc code.

   'codes' : table which gives the bit pattern of of each vlc code.

   'symbols' : table which gives the values to be returned from get_vlc().

   'xxx_wrap' : give the number of bytes between each entry of the
   'bits' or 'codes' tables.

   'xxx_size' : gives the number of bytes of each entry of the 'bits'
   or 'codes' tables.

   'wrap' and 'size' allows to use any memory configuration and types
   (byte/word/long) to store the 'bits', 'codes', and 'symbols' tables.

   'use_static' should be set to 1 for tables, which should be freed
   with av_free_static(), 0 if free_vlc() will be used.
*/
int init_vlc_sparse(VLC *vlc, int nb_bits, int nb_codes,
             const void *bits, int bits_wrap, int bits_size,
             const void *codes, int codes_wrap, int codes_size,
             const void *symbols, int symbols_wrap, int symbols_size,
             int flags)
{
    VLCcode buf[nb_codes];
    int i, j;

    vlc->bits = nb_bits;
    if(flags & INIT_VLC_USE_NEW_STATIC){
        if(vlc->table_size && vlc->table_size == vlc->table_allocated){
            return 0;
        }else if(vlc->table_size){
            return -1; // fatal error, we are called on a partially initialized table
        }
    }else {
        vlc->table = NULL;
        vlc->table_allocated = 0;
        vlc->table_size = 0;
    }

#ifdef DEBUG_VLC
    av_log(NULL,AV_LOG_DEBUG,"build table nb_codes=%d\n", nb_codes);
#endif

    //assert(symbols_size <= 2 || !symbols);
    j = 0;
#define COPY(condition)\
    for (i = 0; i < nb_codes; i++) {\
        GET_DATA(buf[j].bits, bits, i, bits_wrap, bits_size);\
        if (!(condition))\
            continue;\
        GET_DATA(buf[j].code, codes, i, codes_wrap, codes_size);\
        if (flags & INIT_VLC_LE)\
            buf[j].code = bitswap_32(buf[j].code);\
        else\
            buf[j].code <<= 32 - buf[j].bits;\
        if (symbols)\
            GET_DATA(buf[j].symbol, symbols, i, symbols_wrap, symbols_size)\
        else\
            buf[j].symbol = i;\
        j++;\
    }
    COPY(buf[j].bits > nb_bits);
    // qsort is the slowest part of init_vlc, and could probably be improved or avoided
    qsort(buf, j, sizeof(VLCcode), compare_vlcspec);
    COPY(buf[j].bits && buf[j].bits <= nb_bits);
    nb_codes = j;

    if (build_table(vlc, nb_bits, nb_codes, buf, flags) < 0) {
        //av_freep(&vlc->table);
        return -1;
    }
    //if((flags & INIT_VLC_USE_NEW_STATIC) && vlc->table_size != vlc->table_allocated)
     //   av_log(NULL, AV_LOG_ERROR, "needed %d had %d\n", vlc->table_size, vlc->table_allocated);
    return 0;
}

