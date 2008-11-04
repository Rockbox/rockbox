/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#include <inttypes.h>
#include <string.h>

#include "parser.h"
#include "entropy.h"
#include "rangecoding.h"   /* Range-coding (static inline) functions */
#include "demac_iram.h"

#define MODEL_ELEMENTS 64

/*
  The following counts arrays for use with the range decoder are
  hard-coded in the Monkey's Audio decoder.
*/

static const int counts_3970[65] ICONST_ATTR =
{
        0,14824,28224,39348,47855,53994,58171,60926,
    62682,63786,64463,64878,65126,65276,65365,65419,
    65450,65469,65480,65487,65491,65493,65494,65495,
    65496,65497,65498,65499,65500,65501,65502,65503,
    65504,65505,65506,65507,65508,65509,65510,65511,
    65512,65513,65514,65515,65516,65517,65518,65519,
    65520,65521,65522,65523,65524,65525,65526,65527,
    65528,65529,65530,65531,65532,65533,65534,65535,
    65536
};

/* counts_diff_3970[i] = counts_3970[i+1] - counts_3970[i] */
static const int counts_diff_3970[64] ICONST_ATTR =
{
    14824,13400,11124,8507,6139,4177,2755,1756,
    1104,677,415,248,150,89,54,31,
    19,11,7,4,2,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1
};

static const int counts_3980[65] ICONST_ATTR =
{
        0,19578,36160,48417,56323,60899,63265,64435,
    64971,65232,65351,65416,65447,65466,65476,65482,
    65485,65488,65490,65491,65492,65493,65494,65495,
    65496,65497,65498,65499,65500,65501,65502,65503,
    65504,65505,65506,65507,65508,65509,65510,65511,
    65512,65513,65514,65515,65516,65517,65518,65519,
    65520,65521,65522,65523,65524,65525,65526,65527,
    65528,65529,65530,65531,65532,65533,65534,65535,
    65536
};

/* counts_diff_3980[i] = counts_3980[i+1] - counts_3980[i] */

static const int counts_diff_3980[64] ICONST_ATTR =
{
    19578,16582,12257,7906,4576,2366,1170,536,
    261,119,65,31,19,10,6,3,
    3,2,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1
};

/*
  range_get_symbol_* functions based on main decoding loop in simple_d.c from
  http://www.compressconsult.com/rangecoder/rngcod13.zip
  (c) Michael Schindler
*/

static inline int range_get_symbol_3980(void)
{
    int symbol, cf;

    cf = range_decode_culshift(16);

    /* figure out the symbol inefficiently; a binary search would be much better */
    for (symbol = 0; counts_3980[symbol+1] <= cf; symbol++);

    range_decode_update(counts_diff_3980[symbol],counts_3980[symbol]);

    return symbol;
}

static inline int range_get_symbol_3970(void)
{
    int symbol, cf;

    cf = range_decode_culshift(16);

    /* figure out the symbol inefficiently; a binary search would be much better */
    for (symbol = 0; counts_3970[symbol+1] <= cf; symbol++);

    range_decode_update(counts_diff_3970[symbol],counts_3970[symbol]);

    return symbol;
}

/* MAIN DECODING FUNCTIONS */

struct rice_t
{
  uint32_t k;
  uint32_t ksum;
};

static struct rice_t riceX IBSS_ATTR;
static struct rice_t riceY IBSS_ATTR;

static inline void update_rice(struct rice_t* rice, int x)
{
    rice->ksum += ((x + 1) / 2) - ((rice->ksum + 16) >> 5);

    if (rice->k == 0) {
        rice->k = 1;
    } else {
        uint32_t lim = 1 << (rice->k + 4);
        if (rice->ksum < lim) {
            rice->k--;
        } else if (rice->ksum >= 2 * lim) {
            rice->k++;
        }
    }
}

static inline int entropy_decode3980(struct rice_t* rice)
{
    int base, x, pivot, overflow;

    pivot = rice->ksum >> 5;
    if (pivot == 0) pivot=1;

    overflow = range_get_symbol_3980();

    if (overflow == (MODEL_ELEMENTS-1)) {
        overflow = range_decode_short() << 16;
        overflow |= range_decode_short();
    }

    if (pivot >= 0x10000) {
        /* Codepath for 24-bit streams */
        int nbits, lo_bits, base_hi, base_lo;

        /* Count the number of bits in pivot */
        nbits = 17; /* We know there must be at least 17 bits */
        while ((pivot >> nbits) > 0) { nbits++; }

        /* base_lo is the low (nbits-16) bits of base
           base_hi is the high 16 bits of base
        */
        lo_bits = (nbits - 16);

        base_hi = range_decode_culfreq((pivot >> lo_bits) + 1);
        range_decode_update(1, base_hi);

        base_lo = range_decode_culshift(lo_bits);
        range_decode_update(1, base_lo);

        base = (base_hi << lo_bits) + base_lo;
    } else {
        /* Codepath for 16-bit streams */
        base = range_decode_culfreq(pivot);
        range_decode_update(1, base);
    }

    x = base + (overflow * pivot);
    update_rice(rice, x);

    /* Convert to signed */
    if (x & 1)
        return (x >> 1) + 1;
    else
        return -(x >> 1);
}


static inline int entropy_decode3970(struct rice_t* rice)
{
    int x, tmpk;

    int overflow = range_get_symbol_3970();

    if (overflow == (MODEL_ELEMENTS - 1)) {
        tmpk = range_decode_bits(5);
        overflow = 0;
    } else {
        tmpk = (rice->k < 1) ? 0 : rice->k - 1;
    }

    if (tmpk <= 16) {
        x = range_decode_bits(tmpk);
    } else {
        x = range_decode_short();
        x |= (range_decode_bits(tmpk - 16) << 16);
    }
    x += (overflow << tmpk);

    update_rice(rice, x);

    /* Convert to signed */
    if (x & 1)
        return (x >> 1) + 1;
    else
        return -(x >> 1);
}

void init_entropy_decoder(struct ape_ctx_t* ape_ctx,
                          unsigned char* inbuffer, int* firstbyte,
                          int* bytesconsumed)
{
    bytebuffer = inbuffer;
    bytebufferoffset = *firstbyte;

    /* Read the CRC */
    ape_ctx->CRC = read_byte();
    ape_ctx->CRC = (ape_ctx->CRC << 8) | read_byte();
    ape_ctx->CRC = (ape_ctx->CRC << 8) | read_byte();
    ape_ctx->CRC = (ape_ctx->CRC << 8) | read_byte();

    /* Read the frame flags if they exist */
    ape_ctx->frameflags = 0;
    if ((ape_ctx->fileversion > 3820) && (ape_ctx->CRC & 0x80000000)) {
        ape_ctx->CRC &= ~0x80000000;

        ape_ctx->frameflags = read_byte();
        ape_ctx->frameflags = (ape_ctx->frameflags << 8) | read_byte();
        ape_ctx->frameflags = (ape_ctx->frameflags << 8) | read_byte();
        ape_ctx->frameflags = (ape_ctx->frameflags << 8) | read_byte();
    }
    /* Keep a count of the blocks decoded in this frame */
    ape_ctx->blocksdecoded = 0;

    /* Initialise the rice structs */
    riceX.k = 10;
    riceX.ksum = (1 << riceX.k) * 16;
    riceY.k = 10;
    riceY.ksum = (1 << riceY.k) * 16;

    /* The first 8 bits of input are ignored. */
    skip_byte();

    range_start_decoding();

    /* Return the new state of the buffer */
    *bytesconsumed = (intptr_t)bytebuffer - (intptr_t)inbuffer;
    *firstbyte = bytebufferoffset;
}

int ICODE_ATTR_DEMAC entropy_decode(struct ape_ctx_t* ape_ctx,
                                    unsigned char* inbuffer, int* firstbyte,
                                    int* bytesconsumed,
                                    int32_t* decoded0, int32_t* decoded1,
                                    int blockstodecode)
{
    bytebuffer = inbuffer;
    bytebufferoffset = *firstbyte;

    ape_ctx->blocksdecoded += blockstodecode;

    if (ape_ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        /* We are pure silence, just memset the output buffer. */
        memset(decoded0, 0, blockstodecode * sizeof(int32_t));
        memset(decoded1, 0, blockstodecode * sizeof(int32_t));
    } else {
        if (ape_ctx->fileversion > 3970) {
            while (blockstodecode--) {
                *(decoded0++) = entropy_decode3980(&riceY);
                if (decoded1 != NULL)
                    *(decoded1++) = entropy_decode3980(&riceX);
            }
        } else {
            while (blockstodecode--) {
                *(decoded0++) = entropy_decode3970(&riceY);
                if (decoded1 != NULL)
                    *(decoded1++) = entropy_decode3970(&riceX);
            }
        }
    }

    if (ape_ctx->blocksdecoded == ape_ctx->currentframeblocks)
    {
        range_done_decoding();
    }

    /* Return the new state of the buffer */
    *bytesconsumed = bytebuffer - inbuffer;
    *firstbyte = bytebufferoffset;

    return(0);
}
