/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Thom Johansen 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "eq.h"

/* Coef calculation taken from Audio-EQ-Cookbook.txt by Robert Bristow-Johnson.
   Slightly faster calculation can be done by deriving forms which use tan()
   instead of cos() and sin(), but the latter are far easier to use when doing
   fixed point math, and performance is not a big point in the calculation part.
   All the 'a' filter coefficients are negated so we can use only additions
   in the filtering equation.
   We realise the filters as a second order direct form 1 structure. Direct
   form 1 was chosen because of better numerical properties for fixed point
   implementations.
 */

#define DIV64(x, y, z) (long)(((long long)(x) << (z))/(y))
/* This macro requires the EMAC unit to be in fractional mode
   when the coef generator routines are called. If this can't be guaranteeed,
   then add "&& 0" below. This will use a slower coef calculation on Coldfire.
 */
#if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
#define FRACMUL(x, y) \
({ \
    long t; \
    asm volatile ("mac.l    %[a], %[b], %%acc0\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})
#else
#define FRACMUL(x, y) ((long)(((((long long) (x)) * ((long long) (y))) >> 31)))
#endif

/* TODO: replaygain.c has some fixed point routines. perhaps we could reuse
   them? */

/* 128 sixteen bit sine samples + guard point */
short sinetab[] = {
    0, 1607, 3211, 4807, 6392, 7961, 9511, 11038, 12539, 14009, 15446, 16845,
    18204, 19519, 20787, 22004, 23169, 24278, 25329, 26318, 27244, 28105, 28897,
    29621, 30272, 30851, 31356, 31785, 32137, 32412, 32609,32727, 32767, 32727,
    32609, 32412, 32137, 31785, 31356, 30851, 30272, 29621, 28897, 28105, 27244,
    26318, 25329, 24278, 23169, 22004, 20787, 19519, 18204, 16845, 15446, 14009,
    12539, 11038, 9511, 7961, 6392, 4807, 3211, 1607, 0, -1607, -3211, -4807,
    -6392, -7961, -9511, -11038, -12539, -14009, -15446, -16845, -18204, -19519,
    -20787, -22004, -23169, -24278, -25329, -26318, -27244, -28105, -28897,
    -29621, -30272, -30851, -31356, -31785, -32137, -32412, -32609, -32727,
    -32767, -32727, -32609, -32412, -32137, -31785, -31356, -30851, -30272,
    -29621, -28897, -28105, -27244, -26318, -25329, -24278, -23169, -22004,
    -20787, -19519, -18204, -16845, -15446, -14009, -12539, -11038, -9511,
    -7961, -6392, -4807, -3211, -1607, 0
};

/* Good quality sine calculated by linearly interpolating
 * a 128 sample sine table. First harmonic has amplitude of about -84 dB.
 * phase has range from 0 to 0xffffffff, representing 0 and
 * 2*pi respectively.
 * Return value is a signed value from LONG_MIN to LONG_MAX, representing
 * -1 and 1 respectively. 
 */
static long fsin(unsigned long phase)
{
    unsigned int pos = phase >> 25;
    unsigned short frac = (phase & 0x01ffffff) >> 9;
    short diff = sinetab[pos + 1] - sinetab[pos];
    
    return (sinetab[pos] << 16) + frac*diff;
}

static inline long fcos(unsigned long phase)
{
    return fsin(phase + 0xffffffff/4);
}

/* Fixed point square root via Newton-Raphson.
 * Output is in same fixed point format as input. 
 * fracbits specifies number of fractional bits in argument.
 */
static long fsqrt(long a, unsigned int fracbits)
{
    long b = a/2 + (1 << fracbits); /* initial approximation */
    unsigned n;
    const unsigned iterations = 4;
    
    for (n = 0; n < iterations; ++n)
        b = (b + DIV64(a, b, fracbits))/2;

    return b;
}

short dbtoatab[49] = {
    2058, 2180, 2309, 2446, 2591, 2744, 2907, 3079, 3261, 3455, 3659, 3876,
    4106, 4349, 4607, 4880, 5169, 5475, 5799, 6143, 6507, 6893, 7301, 7734,
    8192, 8677, 9192, 9736, 10313, 10924, 11572, 12257, 12983, 13753, 14568,
    15431, 16345, 17314, 18340, 19426, 20577, 21797, 23088, 24456, 25905, 27440,
    29066, 30789, 32613
};

/* Function for converting dB to squared amplitude factor (A = 10^(dB/40)).
   Range is -24 to 24 dB. If gain values outside of this is needed, the above
   table needs to be extended.
   Parameter format is s15.16 fixed point. Return format is s2.29 fixed point.
 */
static long dbtoA(long db)
{
    const unsigned long bias = 24 << 16;
    unsigned short frac = (db + bias) & 0x0000ffff;
    unsigned short pos = (db + bias) >> 16;
    short diff = dbtoatab[pos + 1] - dbtoatab[pos];
    
    return (dbtoatab[pos] << 16) + frac*diff;
}

/* Calculate second order section peaking filter coefficients.
   cutoff is a value from 0 to 0x80000000, where 0 represents 0 hz and
   0x80000000 represents nyquist (samplerate/2).
   Q is an unsigned 16.16 fixed point number, lower bound is artificially set
   at 0.5.
   db is s15.16 fixed point and describes gain/attenuation at peak freq.
   c is a pointer where the coefs will be stored.
 */
void eq_pk_coefs(unsigned long cutoff, unsigned long Q, long db, long *c)
{
    const long one = 1 << 28; /* s3.28 */
    const long A = dbtoA(db);
    const long alpha = DIV64(fsin(cutoff), 2*Q, 15); /* s1.30 */
    long a0, a1, a2; /* these are all s3.28 format */
    long b0, b1, b2;

    /* possible numerical ranges listed after each coef */
    b0 = one + FRACMUL(alpha, A);     /* [1.25..5] */
    b1 = a1 = -2*(fcos(cutoff) >> 3); /* [-2..2] */
    b2 = one - FRACMUL(alpha, A);     /* [-3..0.75] */
    a0 = one + DIV64(alpha, A, 27);   /* [1.25..5] */
    a2 = one - DIV64(alpha, A, 27);   /* [-3..0.75] */

    c[0] = DIV64(b0, a0, 28);
    c[1] = DIV64(b1, a0, 28);
    c[2] = DIV64(b2, a0, 28);
    c[3] = DIV64(-a1, a0, 28);
    c[4] = DIV64(-a2, a0, 28);
}

/* Calculate coefficients for lowshelf filter */
void eq_ls_coefs(unsigned long cutoff, unsigned long Q, long db, long *c)
{
    const long one = 1 << 24; /* s7.24 */
    const long A = dbtoA(db);
    const long alpha = DIV64(fsin(cutoff), 2*Q, 15); /* s1.30 */
    const long ap1 = (A >> 5) + one;
    const long am1 = (A >> 5) - one;
    const long twosqrtalpha = 2*(FRACMUL(fsqrt(A >> 5, 24), alpha) << 1);
    long a0, a1, a2; /* these are all s7.24 format */
    long b0, b1, b2;
    long cs = fcos(cutoff);

    b0 = FRACMUL(A, ap1 - FRACMUL(am1, cs) + twosqrtalpha) << 2;
    b1 = FRACMUL(A, am1 - FRACMUL(ap1, cs)) << 3; 
    b2 = FRACMUL(A, ap1 - FRACMUL(am1, cs) - twosqrtalpha) << 2; 
    a0 = ap1 + FRACMUL(am1, cs) + twosqrtalpha; 
    a1 = -2*((am1 + FRACMUL(ap1, cs))); 
    a2 = ap1 + FRACMUL(am1, cs) - twosqrtalpha;

    c[0] = DIV64(b0, a0, 24);
    c[1] = DIV64(b1, a0, 24);
    c[2] = DIV64(b2, a0, 24);
    c[3] = DIV64(-a1, a0, 24);
    c[4] = DIV64(-a2, a0, 24);
}

/* Calculate coefficients for highshelf filter */
void eq_hs_coefs(unsigned long cutoff, unsigned long Q, long db, long *c)
{
    const long one = 1 << 24; /* s7.24 */
    const long A = dbtoA(db);
    const long alpha = DIV64(fsin(cutoff), 2*Q, 15); /* s1.30 */
    const long ap1 = (A >> 5) + one;
    const long am1 = (A >> 5) - one;
    const long twosqrtalpha = 2*(FRACMUL(fsqrt(A >> 5, 24), alpha) << 1);
    long a0, a1, a2; /* these are all s7.24 format */
    long b0, b1, b2;
    long cs = fcos(cutoff);

    b0 = FRACMUL(A, ap1 + FRACMUL(am1, cs) + twosqrtalpha) << 2;
    b1 = -FRACMUL(A, am1 + FRACMUL(ap1, cs)) << 3;
    b2 = FRACMUL(A, ap1 + FRACMUL(am1, cs) - twosqrtalpha) << 2;
    a0 = ap1 - FRACMUL(am1, cs) + twosqrtalpha;
    a1 = 2*((am1 - FRACMUL(ap1, cs)));
    a2 = ap1 - FRACMUL(am1, cs) - twosqrtalpha;

    c[0] = DIV64(b0, a0, 24);
    c[1] = DIV64(b1, a0, 24);
    c[2] = DIV64(b2, a0, 24);
    c[3] = DIV64(-a1, a0, 24);
    c[4] = DIV64(-a2, a0, 24);
}

#if (!defined(CPU_COLDFIRE) && !defined(CPU_ARM)) || defined(SIMULATOR)
void eq_filter(long **x, struct eqfilter *f, unsigned num,
               unsigned channels, unsigned shift)
{
    /* TODO: Implement generic filtering routine. */
    (void)x;
    (void)f;
    (void)num;
    (void)channels;
    (void)shift;
}
#endif

