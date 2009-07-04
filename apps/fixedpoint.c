/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
 *
 * Fixed point library for plugins
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

#include "fixedpoint.h"
#include <stdlib.h>
#include <stdbool.h>

#ifndef BIT_N
#define BIT_N(n) (1U << (n))
#endif

/** TAKEN FROM ORIGINAL fixedpoint.h */
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

/* Precalculated sine and cosine * 16384 (2^14) (fixed point 18.14) */
static const short sin_table[91] =
{
        0,   285,   571,   857,  1142,  1427,  1712,  1996,  2280,  2563,
     2845,  3126,  3406,  3685,  3963,  4240,  4516,  4790,  5062,  5334,
     5603,  5871,  6137,  6401,  6663,  6924,  7182,  7438,  7691,  7943,
     8191,  8438,  8682,  8923,  9161,  9397,  9630,  9860, 10086, 10310,
    10531, 10748, 10963, 11173, 11381, 11585, 11785, 11982, 12175, 12365,
    12550, 12732, 12910, 13084, 13254, 13420, 13582, 13740, 13894, 14043,
    14188, 14329, 14466, 14598, 14725, 14848, 14967, 15081, 15190, 15295,
    15395, 15491, 15582, 15668, 15749, 15825, 15897, 15964, 16025, 16082,
    16135, 16182, 16224, 16261, 16294, 16321, 16344, 16361, 16374, 16381,
    16384
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
long fsincos(unsigned long phase, long *cos) 
{
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

    if (cos)
        *cos = x;

    return y;
}

/**
 * Fixed point square root via Newton-Raphson.
 * @param x square root argument.
 * @param fracbits specifies number of fractional bits in argument.
 * @return Square root of argument in same fixed point format as input.
 *
 * This routine has been modified to run longer for greater precision,
 * but cuts calculation short if the answer is reached sooner.  In
 * general, the closer x is to 1, the quicker the calculation. 
 */
long fsqrt(long x, unsigned int fracbits)
{
    long b = x/2 + BIT_N(fracbits); /* initial approximation */
    long c;
    unsigned n;
    const unsigned iterations = 8;
    
    for (n = 0; n < iterations; ++n)
    {
        c = DIV64(x, b, fracbits);
        if (c == b) break;
        b = (b + c)/2;
    }

    return b;
}

/**
 * Fixed point sinus using a lookup table
 * don't forget to divide the result by 16384 to get the actual sinus value
 * @param val sinus argument in degree
 * @return sin(val)*16384
 */
long sin_int(int val)
{
    val = (val+360)%360;
    if (val < 181)
    {
        if (val < 91)/* phase 0-90 degree */
            return (long)sin_table[val];
        else/* phase 91-180 degree */
            return (long)sin_table[180-val];
    }
    else
    {
        if (val < 271)/* phase 181-270 degree */
            return -(long)sin_table[val-180];
        else/* phase 270-359 degree */
            return -(long)sin_table[360-val];
    }
    return 0;
}

/**
 * Fixed point cosinus using a lookup table
 * don't forget to divide the result by 16384 to get the actual cosinus value
 * @param val sinus argument in degree
 * @return cos(val)*16384
 */
long cos_int(int val)
{
    val = (val+360)%360;
    if (val < 181)
    {
        if (val < 91)/* phase 0-90 degree */
            return (long)sin_table[90-val];
        else/* phase 91-180 degree */
            return -(long)sin_table[val-90];
    }
    else
    {
        if (val < 271)/* phase 181-270 degree */
            return -(long)sin_table[270-val];
        else/* phase 270-359 degree */
            return (long)sin_table[val-270];
    }
    return 0;
}

/**
 * Fixed-point natural log
 * taken from http://www.quinapalus.com/efunc.html
 *  "The code assumes integers are at least 32 bits long. The (positive)
 *   argument and the result of the function are both expressed as fixed-point
 *   values with 16 fractional bits, although intermediates are kept with 28
 *   bits of precision to avoid loss of accuracy during shifts."
 */

long flog(int x) {
    long t,y;

    y=0xa65af;
    if(x<0x00008000) x<<=16,              y-=0xb1721;
    if(x<0x00800000) x<<= 8,              y-=0x58b91;
    if(x<0x08000000) x<<= 4,              y-=0x2c5c8;
    if(x<0x20000000) x<<= 2,              y-=0x162e4;
    if(x<0x40000000) x<<= 1,              y-=0x0b172;
    t=x+(x>>1); if((t&0x80000000)==0) x=t,y-=0x067cd;
    t=x+(x>>2); if((t&0x80000000)==0) x=t,y-=0x03920;
    t=x+(x>>3); if((t&0x80000000)==0) x=t,y-=0x01e27;
    t=x+(x>>4); if((t&0x80000000)==0) x=t,y-=0x00f85;
    t=x+(x>>5); if((t&0x80000000)==0) x=t,y-=0x007e1;
    t=x+(x>>6); if((t&0x80000000)==0) x=t,y-=0x003f8;
    t=x+(x>>7); if((t&0x80000000)==0) x=t,y-=0x001fe;
    x=0x80000000-x;
    y-=x>>15;
    return y;
}

/** MODIFIED FROM replaygain.c */
/* These math routines have 64-bit internal precision to avoid overflows.
 * Arguments and return values are 32-bit (long) precision.
 */
 
#define FP_MUL64(x, y) (((x) * (y)) >> (fracbits))
#define FP_DIV64(x, y) (((x) << (fracbits)) / (y))

static long long fp_exp10(long long x, unsigned int fracbits);
static long long fp_log10(long long n, unsigned int fracbits);

/* constants in fixed point format, 28 fractional bits */
#define FP28_LN2        (186065279LL)   /* ln(2)        */
#define FP28_LN2_INV    (387270501LL)   /* 1/ln(2)      */
#define FP28_EXP_ZERO   (44739243LL)    /* 1/6          */
#define FP28_EXP_ONE    (-745654LL)     /* -1/360       */
#define FP28_EXP_TWO    (12428LL)       /* 1/21600      */
#define FP28_LN10       (618095479LL)   /* ln(10)       */
#define FP28_LOG10OF2   (80807124LL)    /* log10(2)     */

#define TOL_BITS         2              /* log calculation tolerance */


/* The fpexp10 fixed point math routine is based
 * on oMathFP by Dan Carter (http://orbisstudios.com).
 */

/** FIXED POINT EXP10
 * Return 10^x as FP integer.  Argument is FP integer.
 */
static long long fp_exp10(long long x, unsigned int fracbits)
{
    long long k;
    long long z;
    long long R;
    long long xp;
    
    /* scale constants */
    const long long fp_one      = (1 << fracbits);
    const long long fp_half     = (1 << (fracbits - 1));
    const long long fp_two      = (2 << fracbits);
    const long long fp_mask     = (fp_one - 1);
    const long long fp_ln2_inv  = (FP28_LN2_INV     >> (28 - fracbits));
    const long long fp_ln2      = (FP28_LN2         >> (28 - fracbits));
    const long long fp_ln10     = (FP28_LN10        >> (28 - fracbits));
    const long long fp_exp_zero = (FP28_EXP_ZERO    >> (28 - fracbits));
    const long long fp_exp_one  = (FP28_EXP_ONE     >> (28 - fracbits));
    const long long fp_exp_two  = (FP28_EXP_TWO     >> (28 - fracbits));
    
    /* exp(0) = 1 */
    if (x == 0)
    {
        return fp_one;
    }
    
    /* convert from base 10 to base e */
    x = FP_MUL64(x, fp_ln10);
    
    /* calculate exp(x) */
    k = (FP_MUL64(abs(x), fp_ln2_inv) + fp_half) & ~fp_mask;
    
    if (x < 0)
    {
        k = -k;
    }
    
    x -= FP_MUL64(k, fp_ln2);
    z = FP_MUL64(x, x);
    R = fp_two + FP_MUL64(z, fp_exp_zero + FP_MUL64(z, fp_exp_one
        + FP_MUL64(z, fp_exp_two)));
    xp = fp_one + FP_DIV64(FP_MUL64(fp_two, x), R - x);
    
    if (k < 0)
    {
        k = fp_one >> (-k >> fracbits);
    }
    else
    {
        k = fp_one << (k >> fracbits);
    }
    
    return FP_MUL64(k, xp);
}


/** FIXED POINT LOG10
 * Return log10(x) as FP integer.  Argument is FP integer.
 */
static long long fp_log10(long long n, unsigned int fracbits)
{
    /* Calculate log2 of argument */

    long long log2, frac;
    const long long fp_one  = (1 << fracbits);
    const long long fp_two  = (2 << fracbits);
    const long tolerance    = (1 << ((fracbits / 2) + 2));
    
    if (n <=0) return FP_NEGINF;
    log2 = 0;

    /* integer part */
    while (n < fp_one)
    {
        log2 -= fp_one;
        n <<= 1;
    }
    while (n >= fp_two)
    {
        log2 += fp_one;
        n >>= 1;
    }
    
    /* fractional part */
    frac = fp_one;
    while (frac > tolerance)
    {
        frac >>= 1;
        n = FP_MUL64(n, n);
        if (n >= fp_two)
        {
            n >>= 1;
            log2 += frac;
        }
    }
    
    /* convert log2 to log10 */
    return FP_MUL64(log2, (FP28_LOG10OF2 >> (28 - fracbits)));
}


/** CONVERT FACTOR TO DECIBELS */
long fp_decibels(unsigned long factor, unsigned int fracbits)
{
    long long decibels;
    long long f = (long long)factor;
    bool neg;
    
    /* keep factor in signed long range */
    if (f >= (1LL << 31))
        f = (1LL << 31) - 1;
    
    /* decibels = 20 * log10(factor) */
    decibels = FP_MUL64((20LL << fracbits), fp_log10(f, fracbits));
    
    /* keep result in signed long range */
    if ((neg = (decibels < 0)))
        decibels = -decibels;
    if (decibels >= (1LL << 31))
        return neg ? FP_NEGINF : FP_INF;
    
    return neg ? (long)-decibels : (long)decibels;
}


/** CONVERT DECIBELS TO FACTOR */
long fp_factor(long decibels, unsigned int fracbits)
{
    bool neg;
    long long factor;
    long long db = (long long)decibels;
    
    /* if decibels is 0, factor is 1 */
    if (db == 0)
        return (1L << fracbits);
    
    /* calculate for positive decibels only */
    if ((neg = (db < 0)))
        db = -db;
    
    /* factor = 10 ^ (decibels / 20) */
    factor = fp_exp10(FP_DIV64(db, (20LL << fracbits)), fracbits);
    
    /* keep result in signed long range, return 0 if very small */
    if (factor >= (1LL << 31))
    {
        if (neg)
            return 0;
        else
            return FP_INF;
    }
    
    /* if negative argument, factor is 1 / result */
    if (neg)
        factor = FP_DIV64((1LL << fracbits), factor);
        
    return (long)factor;
}
