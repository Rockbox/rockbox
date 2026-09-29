/***********************************************************************
Copyright (c) 2006-2011, Skype Limited. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of Internet Society, IETF or IETF Trust, nor the
names of specific contributors, may be used to endorse or promote
products derived from this software without specific prior written
permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "resampler_private.h"
#include "stack_alloc.h"

#ifndef OPUS_NO_SILK_FIXED_PHASE
/* One output at phase p from eight consecutive samples: the same sum as the
   generic loop below, but as plain 16x16 C multiplies the compiler can
   schedule, where silk_SMLABB is inline asm on ARMv5E. */
static OPUS_INLINE opus_int16 silk_IIR_FIR_tap8(
    opus_int16 x0, opus_int16 x1, opus_int16 x2, opus_int16 x3,
    opus_int16 x4, opus_int16 x5, opus_int16 x6, opus_int16 x7,
    opus_int p
)
{
    const opus_int16 *a = silk_resampler_frac_FIR_12[ p ];
    const opus_int16 *b = silk_resampler_frac_FIR_12[ 11 - p ];
    opus_int32 res_Q15;

    res_Q15  = (opus_int32)x0 * a[ 0 ];
    res_Q15 += (opus_int32)x1 * a[ 1 ];
    res_Q15 += (opus_int32)x2 * a[ 2 ];
    res_Q15 += (opus_int32)x3 * a[ 3 ];
    res_Q15 += (opus_int32)x4 * b[ 3 ];
    res_Q15 += (opus_int32)x5 * b[ 2 ];
    res_Q15 += (opus_int32)x6 * b[ 1 ];
    res_Q15 += (opus_int32)x7 * b[ 0 ];
    return (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( res_Q15, 15 ) );
}

/* The steps silk_resampler_init produces for 16 and 8 kHz to 48 kHz visit
   only three phases, in a fixed cycle, for any batch up to
   RESAMPLER_MAX_BATCH_SIZE_IN: the rounding in the step never drifts across
   a phase boundary before the index restarts.  Checked exhaustively over
   every batch length.  The third output's window starts off2 samples later,
   and each cycle advances adv samples. */
static OPUS_INLINE opus_int16 *silk_IIR_FIR_cycle3(
    opus_int16 *out, const opus_int16 *buf, opus_int32 max_index_Q16,
    opus_int32 inc, opus_int p0, opus_int p1, opus_int p2, opus_int off2,
    opus_int adv
)
{
    opus_int32 index_Q16 = 0;

    while( index_Q16 < max_index_Q16 ) {
        opus_int16 x0 = buf[ 0 ], x1 = buf[ 1 ], x2 = buf[ 2 ], x3 = buf[ 3 ];
        opus_int16 x4 = buf[ 4 ], x5 = buf[ 5 ], x6 = buf[ 6 ], x7 = buf[ 7 ];
        *out++ = silk_IIR_FIR_tap8( x0, x1, x2, x3, x4, x5, x6, x7, p0 );
        if( ( index_Q16 += inc ) >= max_index_Q16 ) break;
        *out++ = silk_IIR_FIR_tap8( x0, x1, x2, x3, x4, x5, x6, x7, p1 );
        if( ( index_Q16 += inc ) >= max_index_Q16 ) break;
        if( off2 ) {
            *out++ = silk_IIR_FIR_tap8( x1, x2, x3, x4, x5, x6, x7, buf[ 8 ], p2 );
        } else {
            *out++ = silk_IIR_FIR_tap8( x0, x1, x2, x3, x4, x5, x6, x7, p2 );
        }
        index_Q16 += inc;
        buf += adv;
    }
    return out;
}

/* 12 kHz to 48 kHz steps by exactly half a sample: phases 0 and 6. */
static OPUS_INLINE opus_int16 *silk_IIR_FIR_cycle2(
    opus_int16 *out, const opus_int16 *buf, opus_int32 max_index_Q16
)
{
    opus_int32 index_Q16 = 0;

    while( index_Q16 < max_index_Q16 ) {
        opus_int16 x0 = buf[ 0 ], x1 = buf[ 1 ], x2 = buf[ 2 ], x3 = buf[ 3 ];
        opus_int16 x4 = buf[ 4 ], x5 = buf[ 5 ], x6 = buf[ 6 ], x7 = buf[ 7 ];
        *out++ = silk_IIR_FIR_tap8( x0, x1, x2, x3, x4, x5, x6, x7, 0 );
        if( ( index_Q16 += 32768 ) >= max_index_Q16 ) break;
        *out++ = silk_IIR_FIR_tap8( x0, x1, x2, x3, x4, x5, x6, x7, 6 );
        index_Q16 += 32768;
        buf += 1;
    }
    return out;
}

#if defined(OPUS_ARM_ASM_ARMV4_ONLY) && !defined(OPUS_ARM_NO_SILK_ASM)
/* The same cycles as shifts and adds of constants, in
   silk/arm/resampler_armv4_asm.S; each runs whole cycles only. */
#define SILK_IIR_FIR_ASM
#define SILK_IIR_FIR_16K    silk_IIR_FIR_16k_armv4, 43691, 3, 1, 2
#define SILK_IIR_FIR_8K     silk_IIR_FIR_8k_armv4, 21846, 3, 1, 1
#define SILK_IIR_FIR_12K    silk_IIR_FIR_12k_armv4, 32768, 2, 1, 1
#elif defined(OPUS_ARM_ASM_ARMV5E_AND_LATER) && !defined(OPUS_ARM_NO_SILK_ASM)
/* Packed 16x16 multiply-accumulates, in silk/arm/resampler_armv5e_asm.S.
   Each loads two samples a word, so the buffer must be word aligned, and
   at 8 and 12 kHz an iteration is two cycles, one word further on. */
#define SILK_IIR_FIR_ASM
#define SILK_IIR_FIR_ALIGNED
#define SILK_IIR_FIR_16K    silk_IIR_FIR_16k_armv5e, 43691, 3, 1, 2
#define SILK_IIR_FIR_8K     silk_IIR_FIR_8k_armv5e, 21846, 3, 2, 2
#define SILK_IIR_FIR_12K    silk_IIR_FIR_12k_armv5e, 32768, 2, 2, 2
#endif

#ifdef SILK_IIR_FIR_ASM
opus_int16 *silk_IIR_FIR_16k_armv4( opus_int16 *out, const opus_int16 *buf, opus_int32 iters );
opus_int16 *silk_IIR_FIR_8k_armv4( opus_int16 *out, const opus_int16 *buf, opus_int32 iters );
opus_int16 *silk_IIR_FIR_12k_armv4( opus_int16 *out, const opus_int16 *buf, opus_int32 iters );
opus_int16 *silk_IIR_FIR_16k_armv5e( opus_int16 *out, const opus_int16 *buf, opus_int32 iters );
opus_int16 *silk_IIR_FIR_8k_armv5e( opus_int16 *out, const opus_int16 *buf, opus_int32 iters );
opus_int16 *silk_IIR_FIR_12k_armv5e( opus_int16 *out, const opus_int16 *buf, opus_int32 iters );

/* Run the kernel over every whole iteration of cyc cycles of n outputs at
   step inc, each advancing adv samples, and leave the C the rest.  Cycle j
   is whole when its last output's index, (j*n + n-1)*inc, is below
   max_index_Q16. */
#define SILK_IIR_FIR_RUN( args ) SILK_IIR_FIR_RUN_( args )
#define SILK_IIR_FIR_RUN_( fn, inc, n, cyc, adv )                                   \
    do {                                                                            \
        opus_int32 iters = ( max_index_Q16 + (inc) - 1 ) / ( (n) * (inc) ) / (cyc); \
        if( iters > 0 ) {                                                           \
            out = fn( out, buf, iters );                                            \
            buf += (adv) * iters;                                                   \
            max_index_Q16 -= iters * (cyc) * (n) * (inc);                           \
        }                                                                           \
    } while( 0 )
#endif
#endif

static OPUS_INLINE opus_int16 *silk_resampler_private_IIR_FIR_INTERPOL(
    opus_int16  *out,
    opus_int16  *buf,
    opus_int32  max_index_Q16,
    opus_int32  index_increment_Q16
)
{
    opus_int32 index_Q16, res_Q15;
    opus_int16 *buf_ptr;
    opus_int32 table_index;

#ifndef OPUS_NO_SILK_FIXED_PHASE
    switch( index_increment_Q16 ) {
    case 43691: /* 16 kHz */
#ifdef SILK_IIR_FIR_ASM
        SILK_IIR_FIR_RUN( SILK_IIR_FIR_16K );
#endif
        return silk_IIR_FIR_cycle3( out, buf, max_index_Q16, 43691, 0, 8, 4, 1, 2 );
    case 21846: /* 8 kHz */
#ifdef SILK_IIR_FIR_ASM
        SILK_IIR_FIR_RUN( SILK_IIR_FIR_8K );
#endif
        return silk_IIR_FIR_cycle3( out, buf, max_index_Q16, 21846, 0, 4, 8, 0, 1 );
    case 32768: /* 12 kHz */
#ifdef SILK_IIR_FIR_ASM
        SILK_IIR_FIR_RUN( SILK_IIR_FIR_12K );
#endif
        return silk_IIR_FIR_cycle2( out, buf, max_index_Q16 );
    }
#endif

    /* Interpolate upsampled signal and store in output array */
    for( index_Q16 = 0; index_Q16 < max_index_Q16; index_Q16 += index_increment_Q16 ) {
        table_index = silk_SMULWB( index_Q16 & 0xFFFF, 12 );
        buf_ptr = &buf[ index_Q16 >> 16 ];

        res_Q15 = silk_SMULBB(          buf_ptr[ 0 ], silk_resampler_frac_FIR_12[      table_index ][ 0 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 1 ], silk_resampler_frac_FIR_12[      table_index ][ 1 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 2 ], silk_resampler_frac_FIR_12[      table_index ][ 2 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 3 ], silk_resampler_frac_FIR_12[      table_index ][ 3 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 4 ], silk_resampler_frac_FIR_12[ 11 - table_index ][ 3 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 5 ], silk_resampler_frac_FIR_12[ 11 - table_index ][ 2 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 6 ], silk_resampler_frac_FIR_12[ 11 - table_index ][ 1 ] );
        res_Q15 = silk_SMLABB( res_Q15, buf_ptr[ 7 ], silk_resampler_frac_FIR_12[ 11 - table_index ][ 0 ] );
        *out++ = (opus_int16)silk_SAT16( silk_RSHIFT_ROUND( res_Q15, 15 ) );
    }
    return out;
}
/* Upsample using a combination of allpass-based 2x upsampling and FIR interpolation */
void silk_resampler_private_IIR_FIR(
    void                            *SS,            /* I/O  Resampler state             */
    opus_int16                      out[],          /* O    Output signal               */
    const opus_int16                in[],           /* I    Input signal                */
    opus_int32                      inLen           /* I    Number of input samples     */
)
{
    silk_resampler_state_struct *S = (silk_resampler_state_struct *)SS;
    opus_int32 nSamplesIn;
    opus_int32 max_index_Q16, index_increment_Q16;
#ifdef SILK_IIR_FIR_ALIGNED
    VARDECL( opus_int16, buf_alloc );
    opus_int16 *buf;
#else
    VARDECL( opus_int16, buf );
#endif
    SAVE_STACK;

#ifdef SILK_IIR_FIR_ALIGNED
    /* One sample spare, to start on a word */
    ALLOC( buf_alloc, 2 * S->batchSize + RESAMPLER_ORDER_FIR_12 + 1, opus_int16 );
    buf = buf_alloc + ( ( (size_t)buf_alloc >> 1 ) & 1 );
#else
    ALLOC( buf, 2 * S->batchSize + RESAMPLER_ORDER_FIR_12, opus_int16 );
#endif

    /* Copy buffered samples to start of buffer */
    silk_memcpy( buf, S->sFIR.i16, RESAMPLER_ORDER_FIR_12 * sizeof( opus_int16 ) );

    /* Iterate over blocks of frameSizeIn input samples */
    index_increment_Q16 = S->invRatio_Q16;
    while( 1 ) {
        nSamplesIn = silk_min( inLen, S->batchSize );

        /* Upsample 2x */
        silk_resampler_private_up2_HQ( S->sIIR, &buf[ RESAMPLER_ORDER_FIR_12 ], in, nSamplesIn );

        max_index_Q16 = silk_LSHIFT32( nSamplesIn, 16 + 1 );         /* + 1 because 2x upsampling */
        out = silk_resampler_private_IIR_FIR_INTERPOL( out, buf, max_index_Q16, index_increment_Q16 );
        in += nSamplesIn;
        inLen -= nSamplesIn;

        if( inLen > 0 ) {
            /* More iterations to do; copy last part of filtered signal to beginning of buffer */
            silk_memcpy( buf, &buf[ nSamplesIn << 1 ], RESAMPLER_ORDER_FIR_12 * sizeof( opus_int16 ) );
        } else {
            break;
        }
    }

    /* Copy last part of filtered signal to the state for the next call */
    silk_memcpy( S->sFIR.i16, &buf[ nSamplesIn << 1 ], RESAMPLER_ORDER_FIR_12 * sizeof( opus_int16 ) );
    RESTORE_STACK;
}
