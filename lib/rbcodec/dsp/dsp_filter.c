/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Thom Johansen
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
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "dsp_filter.h"
#include "replaygain.h"

enum filter_shift
{
    FILTER_BISHELF_SHIFT = 5, /* For bishelf (bass/treble) */
    FILTER_PEAK_SHIFT = 4,    /* Each peaking filter */
    FILTER_SHELF_SHIFT = 6,   /* Each high/low shelving filter */
};

/** 
 * Calculate first order shelving filter. Filter is not directly usable by the
 * filter_process() function.
 * @param cutoff shelf midpoint frequency. See eq_pk_coefs for format.
 * @param A decibel value multiplied by ten, describing gain/attenuation of
 * shelf. Max value is 24 dB.
 * @param low true for low-shelf filter, false for high-shelf filter.
 * @param c pointer to coefficient storage. Coefficients are s4.27 format.
 */
void filter_shelf_coefs(unsigned long cutoff, long A, bool low, int32_t *c)
{
    long sin, cos;
    int32_t b0, b1, a0, a1; /* s3.28 */
    const long g = get_replaygain_int(A*5) << 4; /* 10^(db/40), s3.28 */

    sin = fp_sincos(cutoff/2, &cos);
    if (low) {
        const int32_t sin_div_g = fp_div(sin, g, 25);
        const int32_t sin_g = FRACMUL(sin, g);
        cos >>= 3;
        b0 = sin_g + cos;             /* 0.25 .. 4.10 */
        b1 = sin_g - cos;             /* -1 .. 3.98 */
        a0 = sin_div_g + cos;         /* 0.25 .. 4.10 */
        a1 = sin_div_g - cos;         /* -1 .. 3.98 */
    } else {
        const int32_t cos_div_g = fp_div(cos, g, 25);
        const int32_t cos_g = FRACMUL(cos, g);
        sin >>= 3;
        b0 = sin + cos_g;             /* 0.25 .. 4.10 */
        b1 = sin - cos_g;             /* -3.98 .. 1 */
        a0 = sin + cos_div_g;         /* 0.25 .. 4.10 */
        a1 = sin - cos_div_g;         /* -3.98 .. 1 */
    }

    const int32_t rcp_a0 = fp_div(1, a0, 57); /* 0.24 .. 3.98, s2.29 */
    *c++ = FRACMUL_SHL(b0, rcp_a0, 1);       /* 0.063 .. 15.85 */
    *c++ = FRACMUL_SHL(b1, rcp_a0, 1);       /* -15.85 .. 15.85 */
    *c++ = -FRACMUL_SHL(a1, rcp_a0, 1);      /* -1 .. 1 */
}

#ifdef HAVE_SW_TONE_CONTROLS
/** 
 * Calculate second order section filter consisting of one low-shelf and one
 * high-shelf section.
 * @param cutoff_low low-shelf midpoint frequency. See filter_pk_coefs for format.
 * @param cutoff_high high-shelf midpoint frequency.
 * @param A_low decibel value multiplied by ten, describing gain/attenuation of
 * low-shelf part. Max value is 24 dB.
 * @param A_high decibel value multiplied by ten, describing gain/attenuation of
 * high-shelf part. Max value is 24 dB.
 * @param A decibel value multiplied by ten, describing additional overall gain.
 * @param c pointer to coefficient storage. Coefficients are s4.27 format.
 */
void filter_bishelf_coefs(unsigned long cutoff_low, unsigned long cutoff_high,
                          long A_low, long A_high, long A,
                          struct dsp_filter *f)
{
    const long g = get_replaygain_int(A*10) << 7; /* 10^(db/20), s0.31 */
    int32_t c_ls[3], c_hs[3];

    filter_shelf_coefs(cutoff_low, A_low, true, c_ls);
    filter_shelf_coefs(cutoff_high, A_high, false, c_hs);
    c_ls[0] = FRACMUL(g, c_ls[0]);
    c_ls[1] = FRACMUL(g, c_ls[1]);

    /* now we cascade the two first order filters to one second order filter
     * which can be used by filter_process(). these resulting coefficients have a
     * really wide numerical range, so we use a fixed point format which will
     * work for the selected cutoff frequencies (in tone_controls.c) only.
     */
    const int32_t b0 = c_ls[0], b1 = c_ls[1], b2 = c_hs[0], b3 = c_hs[1];
    const int32_t a0 = c_ls[2], a1 = c_hs[2];

    int32_t *c = f->coefs;
    *c++ = FRACMUL_SHL(b0, b2, 4);
    *c++ = FRACMUL_SHL(b0, b3, 4) + FRACMUL_SHL(b1, b2, 4);
    *c++ = FRACMUL_SHL(b1, b3, 4);
    *c++ = a0 + a1;
    *c   = -FRACMUL_SHL(a0, a1, 4);

    f->shift = FILTER_BISHELF_SHIFT;
}
#endif /* HAVE_SW_TONE_CONTROLS */

/* Coef calculation taken from Audio-EQ-Cookbook.txt by Robert Bristow-Johnson.
 * Slightly faster calculation can be done by deriving forms which use tan()
 * instead of cos() and sin(), but the latter are far easier to use when doing
 * fixed point math, and performance is not a big point in the calculation part.
 * All the 'a' filter coefficients are negated so we can use only additions
 * in the filtering equation.
 */

/** 
 * Calculate second order section peaking filter coefficients.
 * @param cutoff a value from 0 to 0x80000000, where 0 represents 0 Hz and
 * 0x80000000 represents the Nyquist frequency (samplerate/2).
 * @param Q Q factor value multiplied by ten. Lower bound is artificially set
 * at 0.5.
 * @param db decibel value multiplied by ten, describing gain/attenuation at
 * peak freq. Max value is 24 dB.
 * @param c pointer to coefficient storage. Coefficients are s3.28 format.
 */
void filter_pk_coefs(unsigned long cutoff, unsigned long Q, long db,
                     struct dsp_filter *f)
{
    long cs;
    const long one = 1 << 28; /* s3.28 */
    const long A = get_replaygain_int(db*5) << 5; /* 10^(db/40), s2.29 */
    const long alpha = fp_sincos(cutoff, &cs)/(2*Q)*10 >> 1; /* s1.30 */
    int32_t a0, a1, a2; /* these are all s3.28 format */
    int32_t b0, b1, b2;
    const long alphadivA = fp_div(alpha, A, 27);
    const long alphaA = FRACMUL(alpha, A);

    /* possible numerical ranges are in comments by each coef */
    b0 = one + alphaA;                /* [1 .. 5] */
    b1 = a1 = -2*(cs >> 3);           /* [-2 .. 2] */
    b2 = one - alphaA;                /* [-3 .. 1] */
    a0 = one + alphadivA;             /* [1 .. 5] */
    a2 = one - alphadivA;             /* [-3 .. 1] */

    /* range of this is roughly [0.2 .. 1], but we'll never hit 1 completely */
    int32_t *c = f->coefs;
    const long rcp_a0 = fp_div(1, a0, 59); /* s0.31 */
    *c++ = FRACMUL(b0, rcp_a0);         /* [0.25 .. 4] */
    *c++ = FRACMUL(b1, rcp_a0);         /* [-2 .. 2] */
    *c++ = FRACMUL(b2, rcp_a0);         /* [-2.4 .. 1] */
    *c++ = FRACMUL(-a1, rcp_a0);        /* [-2 .. 2] */
    *c   = FRACMUL(-a2, rcp_a0);        /* [-0.6 .. 1] */

    f->shift = FILTER_PEAK_SHIFT;
}

/**
 * Calculate coefficients for lowshelf filter. Parameters are as for
 * filter_pk_coefs, but the coefficient format is s5.26 fixed point.
 */
void filter_ls_coefs(unsigned long cutoff, unsigned long Q, long db,
                     struct dsp_filter *f)
{
    long cs;
    const long one = 1 << 25; /* s6.25 */
    const long sqrtA = get_replaygain_int(db*5/2) << 2; /* 10^(db/80), s5.26 */
    const long A = FRACMUL_SHL(sqrtA, sqrtA, 8); /* s2.29 */
    const long alpha = fp_sincos(cutoff, &cs)/(2*Q)*10 >> 1; /* s1.30 */
    const long ap1 = (A >> 4) + one;
    const long am1 = (A >> 4) - one;
    const long ap1_cs = FRACMUL(ap1, cs);
    const long am1_cs = FRACMUL(am1, cs);
    const long twosqrtalpha = 2*FRACMUL(sqrtA, alpha);
    int32_t a0, a1, a2; /* these are all s6.25 format */
    int32_t b0, b1, b2;
    
    /* [0.1 .. 40] */
    b0 = FRACMUL_SHL(A, ap1 - am1_cs + twosqrtalpha, 2);
    /* [-16 .. 63.4] */
    b1 = FRACMUL_SHL(A, am1 - ap1_cs, 3);
    /* [0 .. 31.7] */
    b2 = FRACMUL_SHL(A, ap1 - am1_cs - twosqrtalpha, 2);
    /* [0.5 .. 10] */
    a0 = ap1 + am1_cs + twosqrtalpha;
    /* [-16 .. 4] */
    a1 = -2*(am1 + ap1_cs);
    /* [0 .. 8] */
    a2 = ap1 + am1_cs - twosqrtalpha;

    /* [0.1 .. 1.99] */
    int32_t *c = f->coefs;
    const long rcp_a0 = fp_div(1, a0, 55);    /* s1.30 */
    *c++ = FRACMUL_SHL(b0, rcp_a0, 2);       /* [0.06 .. 15.9] */
    *c++ = FRACMUL_SHL(b1, rcp_a0, 2);       /* [-2 .. 31.7] */
    *c++ = FRACMUL_SHL(b2, rcp_a0, 2);       /* [0 .. 15.9] */
    *c++ = FRACMUL_SHL(-a1, rcp_a0, 2);      /* [-2 .. 2] */
    *c++ = FRACMUL_SHL(-a2, rcp_a0, 2);      /* [0 .. 1] */

    f->shift = FILTER_SHELF_SHIFT;
}

/**
 * Calculate coefficients for highshelf filter. Parameters are as for
 * filter_pk_coefs, but the coefficient format is s5.26 fixed point.
 */
void filter_hs_coefs(unsigned long cutoff, unsigned long Q, long db,
                     struct dsp_filter *f)
{
    long cs;
    const long one = 1 << 25; /* s6.25 */
    const long sqrtA = get_replaygain_int(db*5/2) << 2; /* 10^(db/80), s5.26 */
    const long A = FRACMUL_SHL(sqrtA, sqrtA, 8); /* s2.29 */
    const long alpha = fp_sincos(cutoff, &cs)/(2*Q)*10 >> 1; /* s1.30 */
    const long ap1 = (A >> 4) + one;
    const long am1 = (A >> 4) - one;
    const long ap1_cs = FRACMUL(ap1, cs);
    const long am1_cs = FRACMUL(am1, cs);
    const long twosqrtalpha = 2*FRACMUL(sqrtA, alpha);
    int32_t a0, a1, a2; /* these are all s6.25 format */
    int32_t b0, b1, b2;

    /* [0.1 .. 40] */
    b0 = FRACMUL_SHL(A, ap1 + am1_cs + twosqrtalpha, 2);
    /* [-63.5 .. 16] */
    b1 = -FRACMUL_SHL(A, am1 + ap1_cs, 3);
    /* [0 .. 32] */
    b2 = FRACMUL_SHL(A, ap1 + am1_cs - twosqrtalpha, 2);
    /* [0.5 .. 10] */
    a0 = ap1 - am1_cs + twosqrtalpha;
    /* [-4 .. 16] */
    a1 = 2*(am1 - ap1_cs);
    /* [0 .. 8] */
    a2 = ap1 - am1_cs - twosqrtalpha;

    /* [0.1 .. 1.99] */
    int32_t *c = f->coefs;
    const long rcp_a0 = fp_div(1, a0, 55);    /* s1.30 */
    *c++ = FRACMUL_SHL(b0, rcp_a0, 2);       /* [0 .. 16] */
    *c++ = FRACMUL_SHL(b1, rcp_a0, 2);       /* [-31.7 .. 2] */
    *c++ = FRACMUL_SHL(b2, rcp_a0, 2);       /* [0 .. 16] */
    *c++ = FRACMUL_SHL(-a1, rcp_a0, 2);      /* [-2 .. 2] */
    *c   = FRACMUL_SHL(-a2, rcp_a0, 2);      /* [0 .. 1] */

    f->shift = FILTER_SHELF_SHIFT;
}

/**
 * Copy filter definition without destroying dst's history
 */
void filter_copy(struct dsp_filter *dst, const struct dsp_filter *src)
{
    memcpy(dst->coefs, src->coefs, sizeof (src->coefs));
    dst->shift = src->shift;
}

/**
 * Clear filter sample history
 */
void filter_flush(struct dsp_filter *f)
{
    memset(f->history, 0, sizeof (f->history));
}

/**
 * We realise the filters as a second order direct form 1 structure. Direct
 * form 1 was chosen because of better numerical properties for fixed point
 * implementations.
 */
#if (!defined(CPU_COLDFIRE) && !defined(CPU_ARM))
void filter_process(struct dsp_filter *f, int32_t * const buf[], int count,
                    unsigned int channels)
{
    /* Direct form 1 filtering code.
       y[n] = b0*x[i] + b1*x[i - 1] + b2*x[i - 2] + a1*y[i - 1] + a2*y[i - 2],
       where y[] is output and x[] is input.
     */
    unsigned int shift = f->shift;

    for (unsigned int c = 0; c < channels; c++) {
        for (int i = 0; i < count; i++) {
            long long acc = (long long) buf[c][i] * f->coefs[0];
            acc += (long long) f->history[c][0] * f->coefs[1];
            acc += (long long) f->history[c][1] * f->coefs[2];
            acc += (long long) f->history[c][2] * f->coefs[3];
            acc += (long long) f->history[c][3] * f->coefs[4];
            f->history[c][1] = f->history[c][0];
            f->history[c][0] = buf[c][i];
            f->history[c][3] = f->history[c][2];
            buf[c][i] = (acc << shift) >> 32;
            f->history[c][2] = buf[c][i];
        }
    }
}
#endif /* CPU */
