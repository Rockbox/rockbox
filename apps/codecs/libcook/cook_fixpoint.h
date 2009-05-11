/*
 * COOK compatible decoder, fixed point implementation.
 * Copyright (c) 2007 Ian Braithwaite
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
 *
 */

/**
 * @file cook_float.h
 *
 * Cook AKA RealAudio G2 fixed point functions.
 *
 * Fixed point values are represented as 32 bit signed integers,
 * which can be added and subtracted directly in C (without checks for
 * overflow/saturation.
 * Two multiplication routines are provided:
 * 1) Multiplication by powers of two (2^-31 .. 2^31), implemented
 *    with C's bit shift operations.
 * 2) Multiplication by 16 bit fractions (0 <= x < 1), implemented
 *    in C using two 32 bit integer multiplications.
 */

/**
 * Initialise fixed point implementation.
 * Nothing to do for fixed point.
 *
 * @param q                     pointer to the COOKContext
 */
static inline int init_cook_math(COOKContext *q)
{
    return 0;
}

/**
 * Free resources used by floating point implementation.
 * Nothing to do for fixed point.
 *
 * @param q                     pointer to the COOKContext
 */
static inline void free_cook_math(COOKContext *q)
{
    return;
}


/**
 * Fixed point multiply by power of two.
 *
 * @param x                     fix point value
 * @param i                     integer power-of-two, -31..+31
 */
static inline FIXP fixp_pow2(FIXP x, int i)
{
  if (i < 0)
    return (x >> -i) + ((x >> (-i-1)) & 1);
  else
    return x << i;              /* no check for overflow */
}

/**
 * Fixed point multiply by fraction.
 *
 * @param a                     fix point value
 * @param b                     fix point fraction, 0 <= b < 1
 */
static inline FIXP fixp_mult_su(FIXP a, FIXPU b)
{
    int32_t hb = (a >> 16) * b;
    uint32_t lb = (a & 0xffff) * b;

    return hb + (lb >> 16) + ((lb & 0x8000) >> 15);
}


/**
 * The real requantization of the mltcoefs
 *
 * @param q                     pointer to the COOKContext
 * @param index                 index
 * @param quant_index           quantisation index for this band
 * @param subband_coef_index    array of indexes to quant_centroid_tab
 * @param subband_coef_sign     use random noise instead of predetermined value
 * @param mlt_ptr               pointer to the mlt coefficients
 */
static void scalar_dequant_math(COOKContext *q, int index,
                                int quant_index, int* subband_coef_index,
                                int* subband_coef_sign, REAL_T *mlt_p)
{
    /* Num. half bits to right shift */
    const int s = 33 - quant_index + av_log2(q->samples_per_channel);
    const FIXP *table = quant_tables[s & 1][index];
    FIXP f;
    int i;

    for(i=0 ; i<SUBBAND_SIZE ; i++) {
        f = table[subband_coef_index[i]];
        /* noise coding if subband_coef_index[i] == 0 */
        if (((subband_coef_index[i] == 0) && cook_random(q)) ||
            ((subband_coef_index[i] != 0) && subband_coef_sign[i]))
            f = -f;

        mlt_p[i] = (s >= 64) ? 0 : fixp_pow2(f, -(s/2));
    }
}


/**
 * The modulated lapped transform, this takes transform coefficients
 * and transforms them into timedomain samples.
 * A window step is also included.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to the mltcoefficients
 * @param outbuffer         pointer to the timedomain buffer
 * @param mlt_tmp           pointer to temporary storage space
 */
#include "cook_fixp_mdct.h"

static inline void imlt_math(COOKContext *q, FIXP *in)
{
    const int n = q->samples_per_channel;
    const int step = 4 << (10 - av_log2(n));
    int i = 0, j = step>>1;

    cook_mdct_backward(2 * n, in, q->mono_mdct_output);

    do {
        FIXP tmp = q->mono_mdct_output[i];
        
        q->mono_mdct_output[i] =
          fixp_mult_su(-q->mono_mdct_output[n + i], sincos_lookup[j]);
        q->mono_mdct_output[n + i] = fixp_mult_su(tmp, sincos_lookup[j+1]);
        j += step;
    } while (++i < n/2);
    do {
        FIXP tmp = q->mono_mdct_output[i];
        
        j -= step;
        q->mono_mdct_output[i] =
          fixp_mult_su(-q->mono_mdct_output[n + i], sincos_lookup[j+1]);
        q->mono_mdct_output[n + i] = fixp_mult_su(tmp, sincos_lookup[j]);
    } while (++i < n);
}


/**
 * Perform buffer overlapping.
 *
 * @param q                 pointer to the COOKContext
 * @param gain              gain correction to apply first to output buffer
 * @param buffer            data to overlap
 */
static inline void overlap_math(COOKContext *q, int gain, FIXP buffer[])
{
    int i;
    for(i=0 ; i<q->samples_per_channel ; i++) {
        q->mono_mdct_output[i] =
          fixp_pow2(q->mono_mdct_output[i], gain) + buffer[i];
    }
}


/**
 * the actual requantization of the timedomain samples
 *
 * @param q                 pointer to the COOKContext
 * @param buffer            pointer to the timedomain buffer
 * @param gain_index        index for the block multiplier
 * @param gain_index_next   index for the next block multiplier
 */
static inline void
interpolate_math(COOKContext *q, FIXP* buffer,
                 int gain_index, int gain_index_next)
{
    int i;
    int gain_size_factor = q->samples_per_channel / 8;

    if(gain_index == gain_index_next){              //static gain
        for(i = 0; i < gain_size_factor; i++) {
            buffer[i] = fixp_pow2(buffer[i], gain_index);
        }
    } else {                                        //smooth gain
        int step = (gain_index_next - gain_index)
                   << (7 - av_log2(gain_size_factor));
        int x = 0;

        for(i = 0; i < gain_size_factor; i++) {
            buffer[i] = fixp_mult_su(buffer[i], pow128_tab[x]);
            buffer[i] = fixp_pow2(buffer[i], gain_index+1);

            x += step;
            gain_index += (x + 128) / 128 - 1;
            x = (x + 128) % 128;
        }
    }
}


/**
 * Decoupling calculation for joint stereo coefficients.
 *
 * @param x                 mono coefficient
 * @param table             number of decoupling table
 * @param i                 table index
 */
static inline FIXP cplscale_math(FIXP x, int table, int i)
{
  return fixp_mult_su(x, cplscales[table-2][i]);
}


/**
 * Final converion from floating point values to
 * signed, 16 bit sound samples. Round and clip.
 *
 * @param q                 pointer to the COOKContext
 * @param out               pointer to the output buffer
 * @param chan              0: left or single channel, 1: right channel
 */
static inline void output_math(COOKContext *q, int16_t *out, int chan)
{
    int j;

    for (j = 0; j < q->samples_per_channel; j++) {
        out[chan + q->nb_channels * j] =
          av_clip(fixp_pow2(q->mono_mdct_output[j], -11), -32768, 32767);
    }
}
