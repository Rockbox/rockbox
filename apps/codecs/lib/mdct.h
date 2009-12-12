/*
 * WMA compatible decoder
 * Copyright (c) 2002 The FFmpeg Project.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#include "types.h"
#include "fft.h"

#define itofix32(x)       ((x) << PRECISION)

typedef struct MDCTContext
{
    int n;  /* size of MDCT (i.e. number of input data * 2) */
    int nbits; /* n = 2^nbits */
    /* pre/post rotation tables */
    fixed32 *tcos;
    fixed32 *tsin;
    FFTContext fft;
}
MDCTContext;

int ff_mdct_init(MDCTContext *s, int nbits, int inverse);
void ff_imdct_calc(MDCTContext *s, fixed32 *output, fixed32 *input);
int mdct_init_global(void);

static inline fixed32 fixmul32(fixed32 x, fixed32 y)
{
    fixed64 temp;
    temp = x;
    temp *= y;

    temp >>= PRECISION;

    return (fixed32)temp;
}

static inline fixed32 fixmul32b(fixed32 x, fixed32 y)
{
    fixed64 temp;

    temp = x;
    temp *= y;

    temp >>= 31;        //16+31-16 = 31 bits

    return (fixed32)temp;
}

#ifdef CPU_ARM
static inline
void CMUL(fixed32 *x, fixed32 *y,
          fixed32  a, fixed32  b,
          fixed32  t, fixed32  v)
{
    /* This version loses one bit of precision. Could be solved at the cost
     * of 2 extra cycles if it becomes an issue. */
    int x1, y1, l;
    asm(
        "smull    %[l], %[y1], %[b], %[t] \n"
        "smlal    %[l], %[y1], %[a], %[v] \n"
        "rsb      %[b], %[b], #0          \n"
        "smull    %[l], %[x1], %[a], %[t] \n"
        "smlal    %[l], %[x1], %[b], %[v] \n"
        : [l] "=&r" (l), [x1]"=&r" (x1), [y1]"=&r" (y1), [b] "+r" (b)
        : [a] "r" (a),   [t] "r" (t),    [v] "r" (v)
        : "cc"
    );
    *x = x1 << 1;
    *y = y1 << 1;
}
#elif defined CPU_COLDFIRE
static inline
void CMUL(fixed32 *x, fixed32 *y,
          fixed32  a, fixed32  b,
          fixed32  t, fixed32  v)
{
  asm volatile ("mac.l %[a], %[t], %%acc0;"
                "msac.l %[b], %[v], %%acc0;"
                "mac.l %[b], %[t], %%acc1;"
                "mac.l %[a], %[v], %%acc1;"
                "movclr.l %%acc0, %[a];"
                "move.l %[a], (%[x]);"
                "movclr.l %%acc1, %[a];"
                "move.l %[a], (%[y]);"
                : [a] "+&r" (a)
                : [x] "a" (x), [y] "a" (y),
                  [b] "r" (b), [t] "r" (t), [v] "r" (v)
                : "cc", "memory");
}
#else
static inline
void CMUL(fixed32 *pre,
          fixed32 *pim,
          fixed32 are,
          fixed32 aim,
          fixed32 bre,
          fixed32 bim)
{
    //int64_t x,y;
    fixed32 _aref = are;
    fixed32 _aimf = aim;
    fixed32 _bref = bre;
    fixed32 _bimf = bim;
    fixed32 _r1 = fixmul32b(_bref, _aref);
    fixed32 _r2 = fixmul32b(_bimf, _aimf);
    fixed32 _r3 = fixmul32b(_bref, _aimf);
    fixed32 _r4 = fixmul32b(_bimf, _aref);
    *pre = _r1 - _r2;
    *pim = _r3 + _r4;

}
#endif
/* Inverse gain of circular cordic rotation in s0.31 format. */
static const long cordic_circular_gain = 0xb2458939; /* 0.607252929 */
static long fsincos(unsigned long phase, fixed32 *cos);

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
 *
 *        Gives at least 24 bits precision (last 2-8 bits or so are probably off)
 */

static long fsincos(unsigned long phase, fixed32 *cos)
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
