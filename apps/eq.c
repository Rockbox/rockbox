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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <inttypes.h>
#include "config.h"
#include "dsp.h"
#include "eq.h"
#include "replaygain.h"

/* Inverse gain of circular cordic rotation in s0.31 format. */
static const long cordic_circular_gain = 0xb2458939; /* 0.607252929 */

/* Table of values of atan(2^-i) in 0.32 format fractions of pi where pi = 0xffffffff / 2 */
static const unsigned long atan_table[] = {
    0x1fffffff, /* +0.785398163 (or pi/4) */
    0x12e4051d, /* +0.463647609 */
    0x09fb385b, /* +0.244978663 */
    0x051111d4, /* +0.124354995 */
    0x028b0d43, /* +0.062418810 */
    0x0145d7e1, /* +0.031239833 */
    0x00a2f61e, /* +0.015623729 */
    0x00517c55, /* +0.007812341 */
    0x0028be53, /* +0.003906230 */
    0x00145f2e, /* +0.001953123 */
    0x000a2f98, /* +0.000976562 */
    0x000517cc, /* +0.000488281 */
    0x00028be6, /* +0.000244141 */
    0x000145f3, /* +0.000122070 */
    0x0000a2f9, /* +0.000061035 */
    0x0000517c, /* +0.000030518 */
    0x000028be, /* +0.000015259 */
    0x0000145f, /* +0.000007629 */
    0x00000a2f, /* +0.000003815 */
    0x00000517, /* +0.000001907 */
    0x0000028b, /* +0.000000954 */
    0x00000145, /* +0.000000477 */
    0x000000a2, /* +0.000000238 */
    0x00000051, /* +0.000000119 */
    0x00000028, /* +0.000000060 */
    0x00000014, /* +0.000000030 */
    0x0000000a, /* +0.000000015 */
    0x00000005, /* +0.000000007 */
    0x00000002, /* +0.000000004 */
    0x00000001, /* +0.000000002 */
    0x00000000, /* +0.000000001 */
    0x00000000, /* +0.000000000 */
};

/**
 * Implements sin and cos using CORDIC rotation.
 *
 * @param phase has range from 0 to 0xffffffff, representing 0 and
 *        2*pi respectively.
 * @param cos return address for cos
 * @return sin of phase, value is a signed value from LONG_MIN to LONG_MAX,
 *         representing -1 and 1 respectively. 
 */
static long fsincos(unsigned long phase, long *cos) {
    int32_t x, x1, y, y1;
    unsigned long z, z1;
    int i;

    /* Setup initial vector */
    x = cordic_circular_gain;
    y = 0;
    z = phase;

    /* The phase has to be somewhere between 0..pi for this to work right */
    if (z < 0xffffffff / 4) {
        /* z in first quadrant, z += pi/2 to correct */
        x = -x;
        z += 0xffffffff / 4;
    } else if (z < 3 * (0xffffffff / 4)) {
        /* z in third quadrant, z -= pi/2 to correct */
        z -= 0xffffffff / 4;
    } else {
        /* z in fourth quadrant, z -= 3pi/2 to correct */
        x = -x;
        z -= 3 * (0xffffffff / 4);
    }

    /* Each iteration adds roughly 1-bit of extra precision */
    for (i = 0; i < 31; i++) {
        x1 = x >> i;
        y1 = y >> i;
        z1 = atan_table[i];

        /* Decided which direction to rotate vector. Pivot point is pi/2 */
        if (z >= 0xffffffff / 4) {
            x -= y1;
            y += x1;
            z -= z1;
        } else {
            x += y1;
            y -= x1;
            z += z1;
        }
    }

    *cos = x;

    return y;
}

/**
 * Calculate first order shelving filter coefficients.
 * Note that the filter is not compatible with the eq_filter routine.
 * @param cutoff a value from 0 to 0x80000000, where 0 represents 0 Hz and
 * 0x80000000 represents the Nyquist frequency (samplerate/2).
 * @param ad gain at 0 Hz. s3.27 fixed point.
 * @param an gain at Nyquist frequency. s3.27 fixed point.
 * @param c pointer to coefficient storage. The coefs are s0.31 format.
 */
void filter_shelf_coefs(unsigned long cutoff, long ad, long an, int32_t *c)
{
    const long one = 1 << 27;
    long a0, a1;
    long b0, b1;
    long s, cs;
    s = fsincos(cutoff, &cs) >> 4;
    cs = one + (cs >> 4);

    /* For max A = 4 (24 dB) */
    b0 = FRACMUL_SHL(ad, s, 4) + FRACMUL_SHL(an, cs, 4);
    b1 = FRACMUL_SHL(ad, s, 4) - FRACMUL_SHL(an, cs, 4);
    a0 = s + cs;
    a1 = s - cs;

    c[0] = DIV64(b0, a0, 31);
    c[1] = DIV64(b1, a0, 31);
    c[2] = -DIV64(a1, a0, 31);
}

/** 
 * Calculate second order section filter consisting of one low-shelf and one
 * high-shelf section.
 * @param cutoff_low low-shelf midpoint frequency. See eq_pk_coefs for format.
 * @param cutoff_high high-shelf midpoint frequency.
 * @param A_low decibel value multiplied by ten, describing gain/attenuation of
 * low-shelf part. Max value is 24 dB.
 * @param A_high decibel value multiplied by ten, describing gain/attenuation of
 * high-shelf part. Max value is 24 dB.
 * @param A decibel value multiplied by ten, describing additional overall gain.
 * @param c pointer to coefficient storage. Coefficients are s4.27 format.
 */
void filter_bishelf_coefs(unsigned long cutoff_low, unsigned long cutoff_high,
                          long A_low, long A_high, long A, int32_t *c)
{
    long sin1, cos2;        /* s0.31 */
    long cos1, sin2;        /* s3.28 */
    int32_t b0, b1, b2, b3; /* s3.28 */
    int32_t a0, a1, a2, a3;
    const long gd = get_replaygain_int(A_low*5) << 4; /* 10^(db/40), s3.28 */
    const long gn = get_replaygain_int(A_high*5) << 4; /* 10^(db/40), s3.28 */
    const long g = get_replaygain_int(A*10) << 7; /* 10^(db/20), s0.31 */

    sin1 = fsincos(cutoff_low/2, &cos1);
    sin2 = fsincos(cutoff_high/2, &cos2) >> 3;
    cos1 >>= 3;

    /* lowshelf filter, ranges listed are for all possible cutoffs */
    b0 = FRACMUL(sin1, gd) + cos1;   /* 0.25 .. 4.10 */
    b1 = FRACMUL(sin1, gd) - cos1;   /* -1 .. 3.98 */
    a0 = DIV64(sin1, gd, 25) + cos1; /* 0.25 .. 4.10 */
    a1 = DIV64(sin1, gd, 25) - cos1; /* -1 .. 3.98 */

    /* highshelf filter */
    b2 = sin2 + FRACMUL(cos2, gn);   /* 0.25 .. 4.10 */
    b3 = sin2 - FRACMUL(cos2, gn);   /* -3.98 .. 1 */
    a2 = sin2 + DIV64(cos2, gn, 25); /* 0.25 .. 4.10 */
    a3 = sin2 - DIV64(cos2, gn, 25); /* -3.98 .. 1 */

    /* now we cascade the two first order filters to one second order filter
     * which can be used by eq_filter(). these resulting coefficients have a
     * really wide numerical range, so we use a fixed point format which will
     * work for the selected cutoff frequencies (in dsp.c) only.
     */
    const int32_t rcp_a0 = DIV64(1, FRACMUL(a0, a2), 53); /* s3.28 */
    *c++ = FRACMUL(g, FRACMUL_SHL(FRACMUL(b0, b2), rcp_a0, 5));
    *c++ = FRACMUL(g, FRACMUL_SHL(FRACMUL(b0, b3) + FRACMUL(b1, b2), rcp_a0, 5));
    *c++ = FRACMUL(g, FRACMUL_SHL(FRACMUL(b1, b3), rcp_a0, 5));
    *c++ = -FRACMUL_SHL(FRACMUL(a0, a3) + FRACMUL(a1, a2), rcp_a0, 5);
    *c++ = -FRACMUL_SHL(FRACMUL(a1, a3), rcp_a0, 5);
}

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
void eq_pk_coefs(unsigned long cutoff, unsigned long Q, long db, int32_t *c)
{
    long cs;
    const long one = 1 << 28; /* s3.28 */
    const long A = get_replaygain_int(db*5) << 5; /* 10^(db/40), s2.29 */
    const long alpha = fsincos(cutoff, &cs)/(2*Q)*10 >> 1; /* s1.30 */
    int32_t a0, a1, a2; /* these are all s3.28 format */
    int32_t b0, b1, b2;
    const long alphadivA = DIV64(alpha, A, 27);

    /* possible numerical ranges are in comments by each coef */
    b0 = one + FRACMUL(alpha, A);     /* [1 .. 5] */
    b1 = a1 = -2*(cs >> 3);           /* [-2 .. 2] */
    b2 = one - FRACMUL(alpha, A);     /* [-3 .. 1] */
    a0 = one + alphadivA;             /* [1 .. 5] */
    a2 = one - alphadivA;             /* [-3 .. 1] */

    /* range of this is roughly [0.2 .. 1], but we'll never hit 1 completely */
    const long rcp_a0 = DIV64(1, a0, 59); /* s0.31 */
    *c++ = FRACMUL(b0, rcp_a0);         /* [0.25 .. 4] */
    *c++ = FRACMUL(b1, rcp_a0);         /* [-2 .. 2] */
    *c++ = FRACMUL(b2, rcp_a0);         /* [-2.4 .. 1] */
    *c++ = FRACMUL(-a1, rcp_a0);        /* [-2 .. 2] */
    *c++ = FRACMUL(-a2, rcp_a0);        /* [-0.6 .. 1] */
}

/**
 * Calculate coefficients for lowshelf filter. Parameters are as for
 * eq_pk_coefs, but the coefficient format is s5.26 fixed point.
 */
void eq_ls_coefs(unsigned long cutoff, unsigned long Q, long db, int32_t *c)
{
    long cs;
    const long one = 1 << 25; /* s6.25 */
    const long sqrtA = get_replaygain_int(db*5/2) << 2; /* 10^(db/80), s5.26 */
    const long A = FRACMUL_SHL(sqrtA, sqrtA, 8); /* s2.29 */
    const long alpha = fsincos(cutoff, &cs)/(2*Q)*10 >> 1; /* s1.30 */
    const long ap1 = (A >> 4) + one;
    const long am1 = (A >> 4) - one;
    const long twosqrtalpha = 2*FRACMUL(sqrtA, alpha);
    int32_t a0, a1, a2; /* these are all s6.25 format */
    int32_t b0, b1, b2;
    
    /* [0.1 .. 40] */
    b0 = FRACMUL_SHL(A, ap1 - FRACMUL(am1, cs) + twosqrtalpha, 2);
    /* [-16 .. 63.4] */
    b1 = FRACMUL_SHL(A, am1 - FRACMUL(ap1, cs), 3);
    /* [0 .. 31.7] */
    b2 = FRACMUL_SHL(A, ap1 - FRACMUL(am1, cs) - twosqrtalpha, 2);
    /* [0.5 .. 10] */
    a0 = ap1 + FRACMUL(am1, cs) + twosqrtalpha;
    /* [-16 .. 4] */
    a1 = -2*((am1 + FRACMUL(ap1, cs)));
    /* [0 .. 8] */
    a2 = ap1 + FRACMUL(am1, cs) - twosqrtalpha;

    /* [0.1 .. 1.99] */
    const long rcp_a0 = DIV64(1, a0, 55); /* s1.30 */
    *c++ = FRACMUL_SHL(b0, rcp_a0, 2);       /* [0.06 .. 15.9] */
    *c++ = FRACMUL_SHL(b1, rcp_a0, 2);       /* [-2 .. 31.7] */
    *c++ = FRACMUL_SHL(b2, rcp_a0, 2);       /* [0 .. 15.9] */
    *c++ = FRACMUL_SHL(-a1, rcp_a0, 2);      /* [-2 .. 2] */
    *c++ = FRACMUL_SHL(-a2, rcp_a0, 2);      /* [0 .. 1] */
}

/**
 * Calculate coefficients for highshelf filter. Parameters are as for
 * eq_pk_coefs, but the coefficient format is s5.26 fixed point.
 */
void eq_hs_coefs(unsigned long cutoff, unsigned long Q, long db, int32_t *c)
{
    long cs;
    const long one = 1 << 25; /* s6.25 */
    const long sqrtA = get_replaygain_int(db*5/2) << 2; /* 10^(db/80), s5.26 */
    const long A = FRACMUL_SHL(sqrtA, sqrtA, 8); /* s2.29 */
    const long alpha = fsincos(cutoff, &cs)/(2*Q)*10 >> 1; /* s1.30 */
    const long ap1 = (A >> 4) + one;
    const long am1 = (A >> 4) - one;
    const long twosqrtalpha = 2*FRACMUL(sqrtA, alpha);
    int32_t a0, a1, a2; /* these are all s6.25 format */
    int32_t b0, b1, b2;

    /* [0.1 .. 40] */
    b0 = FRACMUL_SHL(A, ap1 + FRACMUL(am1, cs) + twosqrtalpha, 2);
    /* [-63.5 .. 16] */
    b1 = -FRACMUL_SHL(A, am1 + FRACMUL(ap1, cs), 3);
    /* [0 .. 32] */
    b2 = FRACMUL_SHL(A, ap1 + FRACMUL(am1, cs) - twosqrtalpha, 2);
    /* [0.5 .. 10] */
    a0 = ap1 - FRACMUL(am1, cs) + twosqrtalpha;
    /* [-4 .. 16] */
    a1 = 2*((am1 - FRACMUL(ap1, cs)));
    /* [0 .. 8] */
    a2 = ap1 - FRACMUL(am1, cs) - twosqrtalpha;

    /* [0.1 .. 1.99] */
    const long rcp_a0 = DIV64(1, a0, 55); /* s1.30 */
    *c++ = FRACMUL_SHL(b0, rcp_a0, 2);       /* [0 .. 16] */
    *c++ = FRACMUL_SHL(b1, rcp_a0, 2);       /* [-31.7 .. 2] */
    *c++ = FRACMUL_SHL(b2, rcp_a0, 2);       /* [0 .. 16] */
    *c++ = FRACMUL_SHL(-a1, rcp_a0, 2);      /* [-2 .. 2] */
    *c++ = FRACMUL_SHL(-a2, rcp_a0, 2);      /* [0 .. 1] */
}

/* We realise the filters as a second order direct form 1 structure. Direct
 * form 1 was chosen because of better numerical properties for fixed point
 * implementations.
 */

#if (!defined(CPU_COLDFIRE) && !defined(CPU_ARM)) || defined(SIMULATOR)
void eq_filter(int32_t **x, struct eqfilter *f, unsigned num,
               unsigned channels, unsigned shift)
{
    unsigned c, i;
    long long acc;

    /* Direct form 1 filtering code.
       y[n] = b0*x[i] + b1*x[i - 1] + b2*x[i - 2] + a1*y[i - 1] + a2*y[i - 2],
       where y[] is output and x[] is input.
     */

    for (c = 0; c < channels; c++) {
        for (i = 0; i < num; i++) {
            acc  = (long long) x[c][i] * f->coefs[0];
            acc += (long long) f->history[c][0] * f->coefs[1];
            acc += (long long) f->history[c][1] * f->coefs[2];
            acc += (long long) f->history[c][2] * f->coefs[3];
            acc += (long long) f->history[c][3] * f->coefs[4];
            f->history[c][1] = f->history[c][0];
            f->history[c][0] = x[c][i];
            f->history[c][3] = f->history[c][2];
            x[c][i] = (acc << shift) >> 32;
            f->history[c][2] = x[c][i];
        }
    }
}
#endif

