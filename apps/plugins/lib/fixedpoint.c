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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <inttypes.h>
#include "fixedpoint.h"

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
 * @param a square root argument.
 * @param fracbits specifies number of fractional bits in argument.
 * @return Square root of argument in same fixed point format as input. 
 */
long fsqrt(long a, unsigned int fracbits)
{
    long b = a/2 + (1 << fracbits); /* initial approximation */
    unsigned n;
    const unsigned iterations = 4;
    
    for (n = 0; n < iterations; ++n)
        b = (b + (long)(((long long)(a) << fracbits)/b))/2;

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
