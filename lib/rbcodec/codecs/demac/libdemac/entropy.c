/*

libdemac - A Monkey's Audio decoder


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
#include "demac_config.h"

#define MODEL_ELEMENTS 64

/*
  The following counts arrays for use with the range decoder are
  hard-coded in the Monkey's Audio decoder.
*/

static const int counts_3970[65] ICONST_ATTR_DEMAC =
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
static const int counts_diff_3970[64] ICONST_ATTR_DEMAC =
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

static const int counts_3980[65] ICONST_ATTR_DEMAC =
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

static const int counts_diff_3980[64] ICONST_ATTR_DEMAC =
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

Range decoder adapted from rangecod.c included in:

  http://www.compressconsult.com/rangecoder/rngcod13.zip

  rangecod.c     range encoding

  (c) Michael Schindler
  1997, 1998, 1999, 2000
  http://www.compressconsult.com/
  michael@compressconsult.com

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.


The encoding functions were removed, and functions turned into "static
inline" functions. Some minor cosmetic changes were made (e.g. turning
pre-processor symbols into upper-case, removing the rc parameter from
each function (and the RNGC macro)).

*/

/* BITSTREAM READING FUNCTIONS */

/* We deal with the input data one byte at a time - to ensure
   functionality on CPUs of any endianness regardless of any requirements
   for aligned reads.
*/

/* The decoder's state.  It lives in statics between calls, but while a
   block is decoded it is copied to locals, so that the compiler can keep
   it in registers: with every field in memory, loads and stores were about
   half of the decode time on ARM7TDMI.  That only works if everything
   that takes a pointer to it is inlined, which the compiler does not
   always choose to do by itself. */
#define DEMAC_INLINE static inline __attribute__((always_inline))
struct rangecoder_t
{
    uint32_t low;        /* low end of interval */
    uint32_t range;      /* length of interval */
    uint32_t help;       /* bytes_to_follow resp. intermediate value */
    unsigned int buffer; /* buffer for input/output */
    unsigned char* bytebuffer;
    int bytebufferoffset;
};

static struct rangecoder_t rc IBSS_ATTR_DEMAC;

DEMAC_INLINE void skip_byte(struct rangecoder_t* rc)
{
    rc->bytebufferoffset--;
    rc->bytebuffer += rc->bytebufferoffset & 4;
    rc->bytebufferoffset &= 3;
}

DEMAC_INLINE int read_byte(struct rangecoder_t* rc)
{
    int ch = rc->bytebuffer[rc->bytebufferoffset];

    skip_byte(rc);

    return ch;
}

/* RANGE DECODING FUNCTIONS */

/* SIZE OF RANGE ENCODING CODE VALUES. */

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int)1 << (CODE_BITS-1))
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS-2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

/* Start the decoder */
DEMAC_INLINE void range_start_decoding(struct rangecoder_t* rc)
{
    rc->buffer = read_byte(rc);
    rc->low = rc->buffer >> (8 - EXTRA_BITS);
    rc->range = (uint32_t) 1 << EXTRA_BITS;
}

DEMAC_INLINE void range_dec_normalize(struct rangecoder_t* rc)
{
    while (rc->range <= BOTTOM_VALUE)
    {
        rc->buffer = (rc->buffer << 8) | read_byte(rc);
        rc->low = (rc->low << 8) | ((rc->buffer >> 1) & 0xff);
        rc->range <<= 8;
    }
}

/* range / tot_f.  tot_f is the Rice pivot, which is small (below 1024
   for nearly every sample of ordinary 16-bit audio), so where a 32x32->64
   multiply is cheap its reciprocal comes from a table that is filled in as
   divisors turn up, and the division is a multiply and a correction.  The
   other division, by help, has no such pattern. */
#if defined(CPU_ARM) && ARM_ARCH >= 5 && !defined(ARM_HAVE_HW_DIV)
#define RECIP_ENTRIES 4096
/* recip[n] is (2^32 - 1) / n, which is never 0: 0 is "not worked out" */
static uint32_t recip[RECIP_ENTRIES];

DEMAC_INLINE uint32_t range_div(uint32_t range, uint32_t tot_f)
{
    uint32_t r, q;

    if (UNLIKELY(tot_f >= RECIP_ENTRIES))
        return UDIV32(range, tot_f);

    r = recip[tot_f];
    if (UNLIKELY(r == 0))
        r = recip[tot_f] = UDIV32(0xffffffff, tot_f);

    /* this is the quotient or one less */
    q = ((uint64_t)range * r) >> 32;
    if (range - q * tot_f >= tot_f)
        q++;
    return q;
}
#else
#define range_div(range, tot_f) UDIV32(range, tot_f)
#endif

/* Calculate culmulative frequency for next symbol. Does NO update!*/
/* tot_f is the total frequency                              */
/* or: totf is (code_value)1<<shift                                      */
/* returns the culmulative frequency                         */
DEMAC_INLINE int range_decode_culfreq(struct rangecoder_t* rc, int tot_f)
{
    range_dec_normalize(rc);
    rc->help = range_div(rc->range, tot_f);
    return UDIV32(rc->low, rc->help);
}

DEMAC_INLINE int range_decode_culshift(struct rangecoder_t* rc, int shift)
{
    range_dec_normalize(rc);
    rc->help = rc->range >> shift;
    return UDIV32(rc->low, rc->help);
}


/* Update decoding state                                     */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
DEMAC_INLINE void range_decode_update(struct rangecoder_t* rc,
                                       int sy_f, int lt_f)
{
    rc->low -= rc->help * lt_f;
    rc->range = rc->help * sy_f;
}


/* Decode a byte/short without modelling                     */
DEMAC_INLINE unsigned short range_decode_short(struct rangecoder_t* rc)
{   int tmp = range_decode_culshift(rc, 16);
    range_decode_update(rc, 1, tmp);
    return tmp;
}

/* Decode n bits (n <= 16) without modelling - based on range_decode_short */
DEMAC_INLINE int range_decode_bits(struct rangecoder_t* rc, int n)
{   int tmp = range_decode_culshift(rc, n);
    range_decode_update(rc, 1, tmp);
    return tmp;
}


/* Finish decoding                                           */
DEMAC_INLINE void range_done_decoding(struct rangecoder_t* rc)
{   range_dec_normalize(rc);    /* normalize to use up all bytes */
}

/*
  range_get_symbol() is based on the main decoding loop in simple_d.c from
  http://www.compressconsult.com/rangecoder/rngcod13.zip
  (c) Michael Schindler

  That divides low by help for the cumulative frequency cf and then looks
  for the last symbol whose count is not above it.  counts[n] <= cf is the
  same as counts[n] * help <= low, so the division is not needed: the search
  multiplies instead.  The first few symbols are by far the likeliest, so
  it is short.  No count is above 65536 and help is below it here, so the
  product cannot overflow.
*/
DEMAC_INLINE int range_get_symbol(struct rangecoder_t* rc,
                                   const int* counts, const int* counts_diff)
{
    int symbol;

    range_dec_normalize(rc);
    rc->help = rc->range >> 16;

    for (symbol = 0; (uint32_t)counts[symbol+1] * rc->help <= rc->low;
         symbol++);

    range_decode_update(rc, counts_diff[symbol], counts[symbol]);

    return symbol;
}

/* MAIN DECODING FUNCTIONS */

struct rice_t
{
  uint32_t k;
  uint32_t ksum;
};

static struct rice_t riceX IBSS_ATTR_DEMAC;
static struct rice_t riceY IBSS_ATTR_DEMAC;

DEMAC_INLINE void update_rice(struct rice_t* rice, int x)
{
    rice->ksum += ((x + 1) / 2) - ((rice->ksum + 16) >> 5);

    if (UNLIKELY(rice->k == 0)) {
        rice->k = 1;
    } else {
        uint32_t lim = 1 << (rice->k + 4);
        if (UNLIKELY(rice->ksum < lim)) {
            rice->k--;
        } else if (UNLIKELY(rice->ksum >= 2 * lim)) {
            rice->k++;
        }
    }
}

DEMAC_INLINE int entropy_decode3980(struct rangecoder_t* rc,
                                     struct rice_t* rice)
{
    int base, x, pivot, overflow;

    pivot = rice->ksum >> 5;
    if (UNLIKELY(pivot == 0))
        pivot=1;

    overflow = range_get_symbol(rc, counts_3980, counts_diff_3980);

    if (UNLIKELY(overflow == (MODEL_ELEMENTS-1))) {
        overflow = range_decode_short(rc) << 16;
        overflow |= range_decode_short(rc);
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

        base_hi = range_decode_culfreq(rc, (pivot >> lo_bits) + 1);
        range_decode_update(rc, 1, base_hi);

        base_lo = range_decode_culshift(rc, lo_bits);
        range_decode_update(rc, 1, base_lo);

        base = (base_hi << lo_bits) + base_lo;
    } else {
        /* Codepath for 16-bit streams */
        base = range_decode_culfreq(rc, pivot);
        range_decode_update(rc, 1, base);
    }

    x = base + (overflow * pivot);
    update_rice(rice, x);

    /* Convert to signed */
    if (x & 1)
        return (x >> 1) + 1;
    else
        return -(x >> 1);
}


DEMAC_INLINE int entropy_decode3970(struct rangecoder_t* rc,
                                     struct rice_t* rice)
{
    int x, tmpk;

    int overflow = range_get_symbol(rc, counts_3970, counts_diff_3970);

    if (UNLIKELY(overflow == (MODEL_ELEMENTS - 1))) {
        tmpk = range_decode_bits(rc, 5);
        overflow = 0;
    } else {
        tmpk = (rice->k < 1) ? 0 : rice->k - 1;
    }

    if (tmpk <= 16) {
        x = range_decode_bits(rc, tmpk);
    } else {
        x = range_decode_short(rc);
        x |= (range_decode_bits(rc, tmpk - 16) << 16);
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
    rc.bytebuffer = inbuffer;
    rc.bytebufferoffset = *firstbyte;

    /* Read the CRC */
    ape_ctx->CRC = read_byte(&rc);
    ape_ctx->CRC = (ape_ctx->CRC << 8) | read_byte(&rc);
    ape_ctx->CRC = (ape_ctx->CRC << 8) | read_byte(&rc);
    ape_ctx->CRC = (ape_ctx->CRC << 8) | read_byte(&rc);

    /* Read the frame flags if they exist */
    ape_ctx->frameflags = 0;
    if ((ape_ctx->fileversion > 3820) && (ape_ctx->CRC & 0x80000000)) {
        ape_ctx->CRC &= ~0x80000000;

        ape_ctx->frameflags = read_byte(&rc);
        ape_ctx->frameflags = (ape_ctx->frameflags << 8) | read_byte(&rc);
        ape_ctx->frameflags = (ape_ctx->frameflags << 8) | read_byte(&rc);
        ape_ctx->frameflags = (ape_ctx->frameflags << 8) | read_byte(&rc);
    }
    /* Keep a count of the blocks decoded in this frame */
    ape_ctx->blocksdecoded = 0;

    /* Initialise the rice structs */
    riceX.k = 10;
    riceX.ksum = (1 << riceX.k) * 16;
    riceY.k = 10;
    riceY.ksum = (1 << riceY.k) * 16;

    /* The first 8 bits of input are ignored. */
    skip_byte(&rc);

    range_start_decoding(&rc);

    /* Return the new state of the buffer */
    *bytesconsumed = (intptr_t)rc.bytebuffer - (intptr_t)inbuffer;
    *firstbyte = rc.bytebufferoffset;
}

void ICODE_ATTR_DEMAC entropy_decode(struct ape_ctx_t* ape_ctx,
                                     unsigned char* inbuffer, int* firstbyte,
                                     int* bytesconsumed,
                                     int32_t* decoded0, int32_t* decoded1,
                                     int blockstodecode)
{
    /* local copies, for the compiler to keep in registers */
    struct rangecoder_t r = rc;
    struct rice_t rx = riceX, ry = riceY;

    r.bytebuffer = inbuffer;
    r.bytebufferoffset = *firstbyte;

    ape_ctx->blocksdecoded += blockstodecode;

    if ((ape_ctx->frameflags & APE_FRAMECODE_LEFT_SILENCE)
        && ((ape_ctx->frameflags & APE_FRAMECODE_RIGHT_SILENCE)
            || (decoded1 == NULL))) {
        /* We are pure silence, just memset the output buffer. */
        memset(decoded0, 0, blockstodecode * sizeof(int32_t));
        if (decoded1 != NULL)
            memset(decoded1, 0, blockstodecode * sizeof(int32_t));
    } else {
        if (ape_ctx->fileversion > 3970) {
            while (LIKELY(blockstodecode--)) {
                *(decoded0++) = entropy_decode3980(&r, &ry);
                if (decoded1 != NULL)
                    *(decoded1++) = entropy_decode3980(&r, &rx);
            }
        } else {
            while (LIKELY(blockstodecode--)) {
                *(decoded0++) = entropy_decode3970(&r, &ry);
                if (decoded1 != NULL)
                    *(decoded1++) = entropy_decode3970(&r, &rx);
            }
        }
    }

    if (ape_ctx->blocksdecoded == ape_ctx->currentframeblocks)
    {
        range_done_decoding(&r);
    }

    rc = r;
    riceX = rx;
    riceY = ry;

    /* Return the new state of the buffer */
    *bytesconsumed = r.bytebuffer - inbuffer;
    *firstbyte = r.bytebufferoffset;
}
