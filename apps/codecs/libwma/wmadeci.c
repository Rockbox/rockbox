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

/**
 * @file wmadec.c
 * WMA compatible decoder.
 */

#include <codecs.h>
#include <codecs/lib/codeclib.h>
#include "asf.h"
#include "wmadec.h"

/* These are for development and debugging and should not be changed unless
you REALLY know what you are doing ;) */
//#define IGNORE_OVERFLOW
#define FAST_FILTERS
#define PRECISION       16
#define PRECISION64     16

static fixed64 IntTo64(int x)
{
    fixed64 res = 0;
    unsigned char *p = (unsigned char *)&res;

#ifdef ROCKBOX_BIG_ENDIAN
    p[5] = x & 0xff;
    p[4] = (x & 0xff00)>>8;
    p[3] = (x & 0xff0000)>>16;
    p[2] = (x & 0xff000000)>>24;
#else
    p[2] = x & 0xff;
    p[3] = (x & 0xff00)>>8;
    p[4] = (x & 0xff0000)>>16;
    p[5] = (x & 0xff000000)>>24;
#endif
    return res;
}

static int IntFrom64(fixed64 x)
{
    int res = 0;
    unsigned char *p = (unsigned char *)&x;

#ifdef ROCKBOX_BIG_ENDIAN
    res = p[5] | (p[4]<<8) | (p[3]<<16) | (p[2]<<24);
#else
    res = p[2] | (p[3]<<8) | (p[4]<<16) | (p[5]<<24);
#endif
    return res;
}

static fixed32 Fixed32From64(fixed64 x)
{
  return x & 0xFFFFFFFF;
}

static fixed64 Fixed32To64(fixed32 x)
{
  return (fixed64)x;
}
#define fixtof64(x)       (float)((float)(x) / (float)(1 << PRECISION64))        //does not work on int64_t!
#define ftofix32(x)       ((fixed32)((x) * (float)(1 << PRECISION) + ((x) < 0 ? -0.5 : 0.5)))
#define itofix64(x)       (IntTo64(x))
#define itofix32(x)       ((x) << PRECISION)
#define fixtoi32(x)       ((x) >> PRECISION)
#define fixtoi64(x)       (IntFrom64(x))

#ifdef CPU_ARM
#define fixmul32(x, y)  \
    ({ int32_t __hi;  \
       uint32_t __lo;  \
       int32_t __result;  \
       asm ("smull   %0, %1, %3, %4\n\t"  \
            "movs    %0, %0, lsr %5\n\t"  \
            "adc    %2, %0, %1, lsl %6"  \
            : "=&r" (__lo), "=&r" (__hi), "=r" (__result)  \
            : "%r" (x), "r" (y),  \
              "M" (PRECISION), "M" (32 - PRECISION)  \
            : "cc");  \
       __result;  \
    })
#else
fixed32 fixmul32(fixed32 x, fixed32 y)
{
    fixed64 temp;
    float lol;
   // int filehandle = rb->open("/mul.txt", O_WRONLY|O_CREAT|O_APPEND);
    lol= fixtof64(x) * fixtof64(y);

    temp = x;
    temp *= y;

    temp >>= PRECISION;

    //rb->fdprintf(filehandle,"%d\n", (fixed32)temp);
    //rb->close(filehandle);

    return (fixed32)temp;
}

#endif
//thanks MAD!


//mgg: special fixmul32 that does a 16.16 x 1.31 multiply that returns a 16.16 value.
//this is needed because the fft constants are all normalized to be less then 1 and can't fit into a 16 bit number
#ifdef CPU_ARM

#  define fixmul32b(x, y)  \
    ({ int32_t __hi;  \
       uint32_t __lo;  \
       int32_t __result;  \
       asm ("smull    %0, %1, %3, %4\n\t"  \
        "movs    %0, %0, lsr %5\n\t"  \
        "adc    %2, %0, %1, lsl %6"  \
        : "=&r" (__lo), "=&r" (__hi), "=r" (__result)  \
        : "%r" (x), "r" (y),  \
          "M" (31), "M" (1)  \
        : "cc");  \
       __result;  \
    })
#else
static fixed32 fixmul32b(fixed32 x, fixed32 y)
{
    fixed64 temp;

    temp = x;
    temp *= y;

    temp >>= 31;        //16+31-16 = 31 bits

    return (fixed32)temp;
}

#endif




static fixed64 fixmul64byfixed(fixed64 x, fixed32 y)
{

    //return x * y;
     return (x * y);
 // return (fixed64) fixmul32(Fixed32From64(x),y);
}


fixed32 fixdiv32(fixed32 x, fixed32 y)
{
    fixed64 temp;

    if(x == 0)
        return 0;
    if(y == 0)
        return 0x7fffffff;
    temp = x;
    temp <<= PRECISION;
    return (fixed32)(temp / y);
}


static fixed64 fixdiv64(fixed64 x, fixed64 y)
{
    fixed64 temp;

    if(x == 0)
        return 0;
    if(y == 0)
        return 0x07ffffffffffffffLL;
    temp = x;
    temp <<= PRECISION64;
    return (fixed64)(temp / y);
}

static fixed32 fixsqrt32(fixed32 x)
{

    unsigned long r = 0, s, v = (unsigned long)x;

#define STEP(k) s = r + (1 << k * 2); r >>= 1; \
    if (s <= v) { v -= s; r |= (1 << k * 2); }

    STEP(15);
    STEP(14);
    STEP(13);
    STEP(12);
    STEP(11);
    STEP(10);
    STEP(9);
    STEP(8);
    STEP(7);
    STEP(6);
    STEP(5);
    STEP(4);
    STEP(3);
    STEP(2);
    STEP(1);
    STEP(0);

    return (fixed32)(r << (PRECISION / 2));
}


__inline fixed32 fixsin32(fixed32 x)
{

    fixed64 x2, temp;
    int     sign = 1;

    if(x < 0)
    {
        sign = -1;
        x = -x;
    }
    while (x > 0x19220)
    {
        x -= M_PI_F;
        sign = -sign;
    }
    if (x > 0x19220)
    {
        x = M_PI_F - x;
    }
    x2 = (fixed64)x * x;
    x2 >>= PRECISION;
    if(sign != 1)
    {
        x = -x;
    }
    /**
    temp = ftofix32(-.0000000239f) * x2;
    temp >>= PRECISION;
    **/
    temp = 0; // PJJ
    //temp = (temp + 0x0) * x2;    //MGG:  this can't possibly do anything?
    //temp >>= PRECISION;
    temp = (temp - 0xd) * x2;
    temp >>= PRECISION;
    temp = (temp + 0x222) * x2;
    temp >>= PRECISION;
    temp = (temp - 0x2aab) * x2;
    temp >>= PRECISION;
    temp += 0x10000;
    temp = temp * x;
    temp >>= PRECISION;

    return  (fixed32)(temp);
}

__inline fixed32 fixcos32(fixed32 x)
{
    return fixsin32(x - (M_PI_F>>1))*-1;
}


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



/*
__inline fixed32 fixasin32(fixed32 x)
{
    fixed64 temp;
    int     sign = 1;

    if(x > 0x10000 || x < 0xffff0000)
    {
        return 0;
    }
    if(x < 0)
    {
        sign = -1;
        x = -x;
    }
    temp = 0xffffffad * (fixed64)x;
    temp >>= PRECISION;
    temp = (temp + 0x1b5) * x;
    temp >>= PRECISION;
    temp = (temp - 0x460) * x;
    temp >>= PRECISION;
    temp = (temp + 0x7e9) * x;
    temp >>= PRECISION;
    temp = (temp - 0xcd8) * x;
    temp >>= PRECISION;
    temp = (temp + 0x16c7) * x;
    temp >>= PRECISION;
    temp = (temp - 0x36f0) * x;
    temp >>= PRECISION;
    temp = (temp + 0x19220) * fixsqrt32(0x10000 - x);
    temp >>= PRECISION;

    return sign * ((M_PI_F>>1) - (fixed32)temp);
}
*/

#define ALT_BITSTREAM_READER

#define unaligned32(a) (*(uint32_t*)(a))

uint16_t bswap_16(uint16_t x)
{
    uint16_t hi = x & 0xff00;
    uint16_t lo = x & 0x00ff;
    return (hi >> 8) | (lo << 8);
}

uint32_t bswap_32(uint32_t x)
{
    uint32_t b1 = x & 0xff000000;
    uint32_t b2 = x & 0x00ff0000;
    uint32_t b3 = x & 0x0000ff00;
    uint32_t b4 = x & 0x000000ff;
    return (b1 >> 24) | (b2 >> 8) | (b3 << 8) | (b4 << 24);
}

// PJJ : reinstate macro
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


typedef struct CoefVLCTable
{
    int n; /* total number of codes */
    const uint32_t *huffcodes; /* VLC bit values */
    const uint8_t *huffbits;   /* VLC bit size */
    const uint16_t *levels; /* table to build run/level tables */
}
CoefVLCTable;

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len);
int fft_calc(FFTContext *s, FFTComplex *z);


//static variables that replace malloced stuff
fixed32 stat0[2048], stat1[1024], stat2[512], stat3[256], stat4[128];    //these are the MDCT reconstruction windows

fixed32 *tcosarray[5], *tsinarray[5];
fixed32 tcos0[1024], tcos1[512], tcos2[256], tcos3[128], tcos4[64];        //these are the sin and cos rotations used by the MDCT
fixed32 tsin0[1024], tsin1[512], tsin2[256], tsin3[128], tsin4[64];

FFTComplex *exparray[5];                                    //these are the fft lookup tables
uint16_t *revarray[5];
FFTComplex  exptab0[512] IBSS_ATTR;//, exptab1[256], exptab2[128], exptab3[64], exptab4[32];    //folded these in!
uint16_t revtab0[1024], revtab1[512], revtab2[256], revtab3[128], revtab4[64];

uint16_t *runtabarray[2], *levtabarray[2];                                        //these are VLC lookup tables

uint16_t runtab0[1336], runtab1[1336], levtab0[1336], levtab1[1336];                //these could be made smaller since only one can be 1336


//may also be too large by ~ 1KB each?
static VLC_TYPE vlcbuf1[6144][2];
static VLC_TYPE vlcbuf2[3584][2];
static VLC_TYPE vlcbuf3[1536][2] IBSS_ATTR;    //small so lets try iram

//fixed32 window[BLOCK_MAX_SIZE * 2];

const fixed64 pow_table[] =
    {
        0x10000LL,0x11f3dLL,0x14249LL,0x1699cLL,0x195bcLL,0x1c73dLL,0x1fec9LL,0x23d1dLL,0x2830bLL,0x2d182LL,
        0x3298bLL,0x38c53LL,0x3fb28LL,0x47783LL,0x5030aLL,0x59f98LL,0x64f40LL,0x71457LL,0x7f17bLL,0x8e99aLL,
        0xa0000LL,0xb385eLL,0xc96d9LL,0xe2019LL,0xfd954LL,0x11c865LL,0x13f3dfLL,0x166320LL,0x191e6eLL,0x1c2f10LL,
        0x1f9f6eLL,0x237b39LL,0x27cf8bLL,0x2cab1aLL,0x321e65LL,0x383bf0LL,0x3f1882LL,0x46cb6aLL,0x4f6eceLL,0x592006LL,
        0x640000LL,0x7033acLL,0x7de47eLL,0x8d40f6LL,0x9e7d44LL,0xb1d3f4LL,0xc786b7LL,0xdfdf43LL,0xfb304bLL,0x119d69aLL,
        0x13c3a4eLL,0x162d03aLL,0x18e1b70LL,0x1beaf00LL,0x1f52feeLL,0x2325760LL,0x276f514LL,0x2c3f220LL,0x31a5408LL,
        0x37b403cLL,0x3e80000LL,0x46204b8LL,0x4eaece8LL,0x58489a0LL,0x630e4a8LL,0x6f24788LL,0x7cb4328LL,0x8beb8a0LL,
        0x9cfe2f0LL,0xb026200LL,0xc5a4710LL,0xddc2240LL,0xf8d1260LL,0x1172d600LL,0x1393df60LL,0x15f769c0LL,0x18a592c0LL,
        0x1ba77540LL,0x1f074840LL,0x22d08280LL,0x27100000LL,0x2bd42f40LL,0x312d4100LL,0x372d6000LL,0x3de8ee80LL,
        0x4576cb80LL,0x4df09f80LL,0x57733600LL,0x621edd80LL,0x6e17d480LL,0x7b86c700LL,0x8a995700LL,0x9b82b800LL,
        0xae7c5c00LL,0xc3c6b900LL,0xdbaa2200LL,0xf677bc00LL,0x1148a9400LL,0x13648d200LL,0x15c251800LL,0x186a00000LL,
        0x1b649d800LL,0x1ebc48a00LL,0x227c5c000LL,0x26b195000LL,0x2b6a3f000LL,0x30b663c00LL,0x36a801c00LL,0x3d534a400LL,
        0x44cee4800LL,0x4d343c800LL,0x569fd6000LL,0x6131b2800LL,0x6d0db9800LL,0x7a5c33800LL,0x894a55000LL,0x9a0ad6000LL,
        0xacd69d000LL,0xc1ed84000LL,0xd9972f000LL,0xf42400000LL,0x111ee28000LL,0x1335ad6000LL,0x158db98000LL,0x182efd4000LL,
        0x1b22676000LL,0x1e71fe6000LL,0x2229014000LL,0x26540e8000LL,0x2b014f0000LL,0x3040a5c000LL,0x3623e60000LL,0x3cbf0fc000LL,
        0x4428940000LL,0x4c79a08000LL,0x55ce758000LL,0x6046c58000LL,0x6c06220000LL,0x7934728000LL,0x87fe7d0000LL,0x9896800000LL,
        0xab34d90000LL,0xc018c60000LL,0xd7893f0000LL,0xf1d5e40000LL,0x10f580a0000LL,0x13073f00000LL,0x1559a0c0000LL,0x17f48900000LL,
        0x1ae0d160000LL,0x1e286780000LL,0x21d66fc0000LL,0x25f769c0000LL,0x2a995c80000LL,0x2fcc0440000LL,0x35a10940000LL,
        0x3c2c3b80000LL,0x4383d500000LL,0x4bc0c780000LL,0x54ff0e80000LL,0x5f5e1000000LL,0x6b010780000LL,0x780f7c00000LL,
        0x86b5c800000LL,0x9725ae00000LL,0xa9970600000LL,0xbe487500000LL,0xd5804700000LL,0xef8d5a00000LL,0x10cc82e00000LL,
        0x12d940c00000LL,0x152605c00000LL,0x17baa2200000LL,0x1a9fd9c00000LL,0x1ddf82a00000LL,0x2184a5c00000LL,0x259ba5400000LL,
        0x2a3265400000LL,0x2f587cc00000LL,0x351f69000000LL,0x3b9aca000000LL,0x42e0a4800000LL,0x4b09ad800000LL,
        0x54319d000000LL,0x5e778d000000LL,0x69fe64000000LL,0x76ed49800000LL,0x85702c000000LL,0x95b858000000LL,
        0xa7fd1c000000LL,0xbc7c87000000LL,0xd37c3a000000LL,0xed4a55000000LL,0x10a3e82000000LL,0x12abb1a000000LL,
        0x14f2e7a000000LL,0x1781474000000LL,0x1a5f7f4000000LL,0x1d974de000000LL,0x2133a18000000LL
    };

const fixed32 pow_10_to_yover16[] ICONST_ATTR=
{
0x10000,0x127a0,0x15562,0x18a39,0x1c73d,0x20db4,0x25f12,0x2bd09,0x3298b,0x3a6d9,0x4378b,0x4dea3,
0x59f98,0x67e6b,0x77fbb,0x8a8de,0xa0000,0xb8c3e,0xd55d1,0xf6636,0x11c865,0x148906,0x17b6b8,0x1b625b,
0x1f9f6e,0x248475,0x2a2b6e,0x30b25f,0x383bf0,0x40f02c,0x4afd4b,0x5698b0,0x640000,0x737a6b,0x855a26,
0x99fe1f,0xb1d3f4,0xcd5a3e,0xed232b,0x111d78a,0x13c3a4e,0x16d2c94,0x1a5b24e,0x1e6f7b0,0x2325760,
0x28961b4,0x2ede4ec,0x361f6dc,0x3e80000,0x482c830,0x5358580,0x603ed30,0x6f24788,0x8058670,0x9435fb0,
0xab26b70,0xc5a4710,0xe43bdc0,0x1078f700,0x1305ace0,0x15f769c0,0x195dd100,0x1d4af120
};

const fixed32 pow_a_table[] =
{
0x1004,0x1008,0x100c,0x1010,0x1014,0x1018,0x101c,0x1021,0x1025,0x1029,0x102d,0x1031,0x1036,0x103a,
0x103e,0x1043,0x1047,0x104b,0x1050,0x1054,0x1059,0x105d,0x1062,0x1066,0x106b,0x106f,0x1074,0x1078,
0x107d,0x1082,0x1086,0x108b,0x1090,0x1095,0x1099,0x109e,0x10a3,0x10a8,0x10ad,0x10b2,0x10b7,0x10bc,
0x10c1,0x10c6,0x10cb,0x10d0,0x10d5,0x10da,0x10df,0x10e5,0x10ea,0x10ef,0x10f5,0x10fa,0x10ff,0x1105,
0x110a,0x1110,0x1115,0x111b,0x1120,0x1126,0x112c,0x1131,0x1137,0x113d,0x1143,0x1149,0x114f,0x1155,
0x115a,0x1161,0x1167,0x116d,0x1173,0x1179,0x117f,0x1186,0x118c,0x1192,0x1199,0x119f,0x11a6,0x11ac,
0x11b3,0x11b9,0x11c0,0x11c7,0x11ce,0x11d4,0x11db,0x11e2,0x11e9,0x11f0,0x11f8,0x11ff,0x1206,0x120d,
0x1215,0x121c,0x1223,0x122b,0x1233,0x123a,0x1242,0x124a,0x1251,0x1259,0x1261,0x1269,0x1271,0x127a,
0x1282,0x128a,0x1293,0x129b,0x12a4,0x12ac,0x12b5,0x12be,0x12c7,0x12d0,0x12d9,0x12e2,0x12eb,0x12f4,
0x12fe,0x1307
};

const fixed64 lsp_pow_e_table[] =
{
    0xf333f9deLL,        0xf0518db9LL,        0x0LL,        0x7e656b4fLL,        0x7999fcefLL,        0xf828c6dcLL,        0x0LL,
    0x3f32b5a7LL,   0x3cccfe78LL,    0xfc14636eLL,    0x0LL,    0x9f995ad4LL,    0x9e667f3cLL,    0xfe0a31b7LL,
    0x0LL,    0x4fccad6aLL,    0x4f333f9eLL,    0x7f0518dcLL,    0x0LL,    0x27e656b5LL,    0x27999fcfLL,
    0xbf828c6eLL,    0x0LL,    0x13f32b5aLL,    0x13cccfe7LL,    0xdfc14637LL,    0x0LL,    0x89f995adLL,
    0x9e667f4LL,    0x6fe0a31bLL,    0x0LL,    0x44fccad7LL,    0x4f333faLL,    0x37f0518eLL,    0x0LL,
    0xa27e656bLL,    0x827999fdLL,    0x1bf828c7LL,    0x0LL,    0xd13f32b6LL,    0x413cccfeLL,    0xdfc1463LL,
    0x0LL,    0xe89f995bLL,    0xa09e667fLL,    0x6fe0a32LL,    0x0LL,    0x744fccadLL,    0x504f3340LL,
    0x837f0519LL,    0x0LL,    0xba27e657LL,    0xa82799a0LL,    0xc1bf828cLL,    0x0LL,    0x5d13f32bLL,
    0xd413ccd0LL,    0x60dfc146LL,    0x0LL,    0xae89f996LL,    0x6a09e668LL,    0x306fe0a3LL,    0x0LL,
    0xd744fccbLL,    0xb504f334LL,    0x9837f052LL,    0x80000000LL,    0x6ba27e65LL,    0x5a82799aLL,
    0x4c1bf829LL,    0x40000000LL,    0x35d13f33LL,    0x2d413ccdLL,    0x260dfc14LL,    0x20000000LL,
    0x1ae89f99LL,    0x16a09e66LL,    0x1306fe0aLL,    0x10000000LL,    0xd744fcdLL,    0xb504f33LL,
    0x9837f05LL,    0x8000000LL,    0x6ba27e6LL,    0x5a8279aLL,    0x4c1bf83LL,    0x4000000LL,
    0x35d13f3LL,    0x2d413cdLL,    0x260dfc1LL,    0x2000000LL,    0x1ae89faLL,    0x16a09e6LL,
    0x1306fe1LL,    0x1000000LL,    0xd744fdLL,    0xb504f3LL,    0x9837f0LL,    0x800000LL,
    0x6ba27eLL,    0x5a827aLL,    0x4c1bf8LL,    0x400000LL,    0x35d13fLL,    0x2d413dLL,
    0x260dfcLL,    0x200000LL,    0x1ae8a0LL,    0x16a09eLL,    0x1306feLL,    0x100000LL,
    0xd7450LL,    0xb504fLL,    0x9837fLL,    0x80000LL,    0x6ba28LL,    0x5a828LL,    0x4c1c0LL,
    0x40000LL,    0x35d14LL,    0x2d414LL,    0x260e0LL,    0x20000LL,    0x1ae8aLL,    0x16a0aLL,
    0x13070LL,    0x10000LL,    0xd745LL,    0xb505LL,    0x9838LL,    0x8000LL,    0x6ba2LL,
    0x5a82LL,    0x4c1cLL,    0x4000LL,    0x35d1LL,    0x2d41LL,    0x260eLL,    0x2000LL,
    0x1ae9LL,    0x16a1LL,    0x1307LL,    0x1000LL,    0xd74LL,    0xb50LL,    0x983LL,    0x800LL,
    0x6baLL,    0x5a8LL,    0x4c2LL,    0x400LL,    0x35dLL,    0x2d4LL,    0x261LL,    0x200LL,    0x1afLL,
    0x16aLL,    0x130LL,    0x100LL,    0xd7LL,    0xb5LL,    0x98LL,    0x80LL,    0x6cLL,    0x5bLL,
    0x4cLL,    0x40LL,    0x36LL,    0x2dLL,    0x26LL,    0x20LL,    0x1bLL,    0x17LL,    0x13LL,
    0x10LL,    0xdLL,    0xbLL,    0xaLL,    0x8LL,    0x7LL,    0x6LL,    0x5LL,    0x4LL,    0x3LL,
    0x3LL,    0x2LL,    0x2LL,    0x2LL,    0x1LL,    0x1LL,    0x1LL,    0x1LL,    0x1LL,    0x1LL,
    0x1LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,
    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,
    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,
    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,
    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,
    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,    0x0LL,
    0x0LL,    0x0LL
};

#include "wmadata.h" // PJJ

/**
 * The size of the FFT is 2^nbits. If inverse is TRUE, inverse FFT is
 * done
 */
int fft_inits(FFTContext *s, int nbits, int inverse)
{
    int i, j, m, n;
    fixed32 alpha, c1, s1, ct, st;
    int s2;

    s->nbits = nbits;
    n = 1 << nbits;

    //s->exptab = exparray[10-nbits];    //not needed

    //s->exptab = av_malloc((n >> 1) * sizeof(FFTComplex));
    //if (!s->exptab)
    //    goto fail;
    s->revtab = revarray[10-nbits];
    //s->revtab = av_malloc(n * sizeof(uint16_t));
    //if (!s->revtab)
    //    goto fail;
    s->inverse = inverse;

    s2 = inverse ? 1 : -1;

    if(nbits == 10){        //we folded all these stupid tables into the nbits==10 table, so don't make it for the others!
                    //should probably just remove exptab building out of this function and do it higher up for neatness
        for(i=0;i<(n/2);++i)
        {
          //we're going to redo this in CORDIC fixed format!     Hold onto your butts

          /*
          input to cordic is from 0 ->2pi with 0->0 and 2^32-1 ->2pi
          output, which is what we'll store the variables as is
            -1->-2^31 and 1->2^31-1

          */

          fixed32 ifix = itofix32(i);
          fixed32 nfix = itofix32(n);
          fixed32 res = fixdiv32(ifix,nfix);        //this is really bad here since nfix can be as large as 1024 !
                            //also, make this a shift, since its a fucking power of two divide
          alpha = fixmul32(TWO_M_PI_F, res);
          ct = fixcos32(alpha);                    //need to correct alpha for 0->2pi scale
          st = fixsin32(alpha);// * s2;

          s1 = fsincos(res<<16, &c1);        //does sin and cos in one pass!

        //I really have my doubts about the correctness of the alpha to cordic mapping here, but it seems to work well enough
        //double check this later!

          exptab0[i].re = c1;
          exptab0[i].im = s1*s2;
        }
    }
   // s->fft_calc = fft_calc;
    s->exptab1 = NULL;


    /* compute bit reverse table */

    for(i=0;i<n;i++)
    {
        m=0;
        for(j=0;j<nbits;j++)
        {
            m |= ((i >> j) & 1) << (nbits-j-1);

        }

        s->revtab[i]=m;
    }
    return 0;
//fail:
 //   av_freep(&s->revtab);
 //   av_freep(&s->exptab);
 //   av_freep(&s->exptab1);
    return -1;
}

/* butter fly op */
#define BF(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  fixed32 ax, ay, bx, by;\
  bx=pre1;\
  by=pim1;\
  ax=qre1;\
  ay=qim1;\
  pre = (bx + ax);\
  pim = (by + ay);\
  qre = (bx - ax);\
  qim = (by - ay);\
}

//this goddamn butterfly could overflow and i'd neve rknow it...
//holy shit it was the fucking butterfly oh god this is the worst thing ever



int fft_calc_unscaled(FFTContext *s, FFTComplex *z)
{
    int ln = s->nbits;
    int j, np, np2;
    int nblocks, nloops;
    register FFTComplex *p, *q;
   // FFTComplex *exptab = s->exptab;
    int l;
    fixed32 tmp_re, tmp_im;
    int tabshift = 10-ln;

    np = 1 << ln;


    /* pass 0 */

    p=&z[0];
    j=(np >> 1);
    do
    {
        BF(p[0].re, p[0].im, p[1].re, p[1].im,
           p[0].re, p[0].im, p[1].re, p[1].im);
        p+=2;
    }
    while (--j != 0);

    /* pass 1 */


    p=&z[0];
    j=np >> 2;
    if (s->inverse)
    {
        do
        {
            BF(p[0].re, p[0].im, p[2].re, p[2].im,
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im,
               p[1].re, p[1].im, -p[3].im, p[3].re);
            p+=4;
        }
        while (--j != 0);
    }
    else
    {
        do
        {
            BF(p[0].re, p[0].im, p[2].re, p[2].im,
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im,
               p[1].re, p[1].im, p[3].im, -p[3].re);
            p+=4;
        }
        while (--j != 0);
    }
    /* pass 2 .. ln-1 */

    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;
    do
    {
        p = z;
        q = z + nloops;
        for (j = 0; j < nblocks; ++j)
        {
            BF(p->re, p->im, q->re, q->im,
               p->re, p->im, q->re, q->im);

            p++;
            q++;
            for(l = nblocks; l < np2; l += nblocks)
            {
                CMUL(&tmp_re, &tmp_im, exptab0[(l<<tabshift)].re, exptab0[(l<<tabshift)].im, q->re, q->im);
                //CMUL(&tmp_re, &tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);
                BF(p->re, p->im, q->re, q->im,
                   p->re, p->im, tmp_re, tmp_im);
                p++;
                q++;
            }

            p += nloops;
            q += nloops;
        }
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    }
    while (nblocks != 0);
    return 0;
}

/*
//needless since we're statically allocated
void fft_end(FFTContext *s)
{
   // av_freep(&s->revtab);
   // av_freep(&s->exptab);
   // av_freep(&s->exptab1);
}
*/
/* VLC decoding */

#define GET_VLC(code, name, gb, table, bits, max_depth)\
{\
    int n, index, nb_bits;\
\
    index= SHOW_UBITS(name, gb, bits);\
    code = table[index][0];\
    n    = table[index][1];\
\
    if(max_depth > 1 && n < 0){\
        LAST_SKIP_BITS(name, gb, bits)\
        UPDATE_CACHE(name, gb)\
\
        nb_bits = -n;\
\
        index= SHOW_UBITS(name, gb, nb_bits) + code;\
        code = table[index][0];\
        n    = table[index][1];\
        if(max_depth > 2 && n < 0){\
            LAST_SKIP_BITS(name, gb, nb_bits)\
            UPDATE_CACHE(name, gb)\
\
            nb_bits = -n;\
\
            index= SHOW_UBITS(name, gb, nb_bits) + code;\
            code = table[index][0];\
            n    = table[index][1];\
        }\
    }\
    SKIP_BITS(name, gb, n)\
}


//#define DEBUG_VLC

#define GET_DATA(v, table, i, wrap, size) \
{\
    const uint8_t *ptr = (const uint8_t *)table + i * wrap;\
    switch(size) {\
    case 1:\
        v = *(const uint8_t *)ptr;\
        break;\
    case 2:\
        v = *(const uint16_t *)ptr;\
        break;\
    default:\
        v = *(const uint32_t *)ptr;\
        break;\
    }\
}

// deprecated, dont use get_vlc for new code, use get_vlc2 instead or use GET_VLC directly
static inline int get_vlc(GetBitContext *s, VLC *vlc)
{
    int code;
    VLC_TYPE (*table)[2]= vlc->table;

    OPEN_READER(re, s)
    UPDATE_CACHE(re, s)

    GET_VLC(code, re, s, table, vlc->bits, 3)

    CLOSE_READER(re, s)
    return code;
}

static int alloc_table(VLC *vlc, int size)
{
    int index;
    index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated)
    {
       // rb->splash(HZ*10, "OH CRAP, TRIED TO REALLOC A STATIC VLC TABLE!");
        vlc->table_allocated += (1 << vlc->bits);
       // vlc->table = av_realloc(vlc->table,
       //                         sizeof(VLC_TYPE) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

static int build_table(VLC *vlc, int table_nb_bits,
                       int nb_codes,
                       const void *bits, int bits_wrap, int bits_size,
                       const void *codes, int codes_wrap, int codes_size,
                       uint32_t code_prefix, int n_prefix)
{
    int i, j, k, n, table_size, table_index, nb, n1, index;
    uint32_t code;
    VLC_TYPE (*table)[2];

    table_size = 1 << table_nb_bits;
    table_index = alloc_table(vlc, table_size);
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];

    for(i=0;i<table_size;i++)
    {
        table[i][1] = 0; //bits
        table[i][0] = -1; //codes
    }

    /* first pass: map codes and compute auxillary table sizes */
    for(i=0;i<nb_codes;i++)
    {
        GET_DATA(n, bits, i, bits_wrap, bits_size);
        GET_DATA(code, codes, i, codes_wrap, codes_size);
        /* we accept tables with holes */
        if (n <= 0)
            continue;
        /* if code matches the prefix, it is in the table */
        n -= n_prefix;
        if (n > 0 && (code >> n) == code_prefix)
        {
            if (n <= table_nb_bits)
            {
                /* no need to add another table */
                j = (code << (table_nb_bits - n)) & (table_size - 1);
                nb = 1 << (table_nb_bits - n);
                for(k=0;k<nb;k++)
                {
                    if (table[j][1] /*bits*/ != 0)
                    {
                        // PJJ exit(-1);
                    }
                    table[j][1] = n; //bits
                    table[j][0] = i; //code
                    j++;
                }
            }
            else
            {
                n -= table_nb_bits;
                j = (code >> n) & ((1 << table_nb_bits) - 1);
                /* compute table size */
                n1 = -table[j][1]; //bits
                if (n > n1)
                    n1 = n;
                table[j][1] = -n1; //bits
            }
        }
    }

    /* second pass : fill auxillary tables recursively */
    for(i=0;i<table_size;i++)
    {
        n = table[i][1]; //bits
        if (n < 0)
        {
            n = -n;
            if (n > table_nb_bits)
            {
                n = table_nb_bits;
                table[i][1] = -n; //bits
            }
            index = build_table(vlc, n, nb_codes,
                                bits, bits_wrap, bits_size,
                                codes, codes_wrap, codes_size,
                                (code_prefix << table_nb_bits) | i,
                                n_prefix + table_nb_bits);
            if (index < 0)
                return -1;
            /* note: realloc has been done, so reload tables */
            table = &vlc->table[table_index];
            table[i][0] = index; //code
        }
    }
    return table_index;
}

/* Build VLC decoding tables suitable for use with get_vlc().

   'nb_bits' set thee decoding table size (2^nb_bits) entries. The
   bigger it is, the faster is the decoding. But it should not be too
   big to save memory and L1 cache. '9' is a good compromise.

   'nb_codes' : number of vlcs codes

   'bits' : table which gives the size (in bits) of each vlc code.

   'codes' : table which gives the bit pattern of of each vlc code.

   'xxx_wrap' : give the number of bytes between each entry of the
   'bits' or 'codes' tables.

   'xxx_size' : gives the number of bytes of each entry of the 'bits'
   or 'codes' tables.

   'wrap' and 'size' allows to use any memory configuration and types
   (byte/word/long) to store the 'bits' and 'codes' tables.
*/
int init_vlc(VLC *vlc, int nb_bits, int nb_codes,
             const void *bits, int bits_wrap, int bits_size,
             const void *codes, int codes_wrap, int codes_size)
{
    vlc->bits = nb_bits;
   // vlc->table = NULL;
   // vlc->table_allocated = 0;
    vlc->table_size = 0;

    if (build_table(vlc, nb_bits, nb_codes,
                    bits, bits_wrap, bits_size,
                    codes, codes_wrap, codes_size,
                    0, 0) < 0)
    {
       // av_free(vlc->table);
        return -1;
    }
    //dump_table("Tab 1",vlc->table[0],vlc->table_size);
    //dump_table("Tab 2",vlc->table[1],vlc->table_size);
    return 0;
}

/**
 * init MDCT or IMDCT computation.
 */
int ff_mdct_init(MDCTContext *s, int nbits, int inverse)
{
    int n, n4, i;
   // fixed32 alpha;


    memset(s, 0, sizeof(*s));
    n = 1 << nbits;            //nbits ranges from 12 to 8 inclusive
    s->nbits = nbits;
    s->n = n;
    n4 = n >> 2;
    s->tcos = tcosarray[12-nbits];
    s->tsin = tsinarray[12-nbits];
    //s->tcos = av_malloc(n4 * sizeof(fixed32));        //this allocates between 1024 and 64 elements
    //if (!s->tcos)
    //    goto fail;
    //s->tsin = av_malloc(n4 * sizeof(fixed32));
    //if (!s->tsin)
    //    goto fail;
//
    for(i=0;i<n4;i++)
    {
        //fixed32 pi2 = fixmul32(0x20000, M_PI_F);
        fixed32 ip = itofix32(i) + 0x2000;
        ip = ip >> nbits;
        //ip = fixdiv32(ip,itofix32(n)); // PJJ optimize
        //alpha = fixmul32(TWO_M_PI_F, ip);
        //s->tcos[i] = -fixcos32(alpha);        //alpha between 0 and pi/2
        //s->tsin[i] = -fixsin32(alpha);

    s->tsin[i] = - fsincos(ip<<16, &(s->tcos[i]));            //I can't remember why this works, but it seems to agree for ~24 bits, maybe more!
    s->tcos[i] *=-1;
  }
    if (fft_inits(&s->fft, s->nbits - 2, inverse) < 0)
        goto fail;
    return 0;
fail:
//    av_freep(&s->tcos);
//    av_freep(&s->tsin);
    return -1;
}

/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 * @param tmp N/2 samples
 */
void ff_imdct_calc(MDCTContext *s,
                   fixed32 *output,
                   const fixed32 *input,
                   FFTComplex *tmp)
{
    int k, n8, n4, n2, n, j,scale;
    const uint16_t *revtab = s->fft.revtab;
    const fixed32 *tcos = s->tcos;
    const fixed32 *tsin = s->tsin;
    const fixed32 *in1, *in2;
    FFTComplex *z = (FFTComplex *)tmp;

    n = 1 << s->nbits;

    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;


    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;

    for(k = 0; k < n4; k++)
    {
        j=revtab[k];
        CMUL(&z[j].re, &z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }

    for(k = 0; k < n4; k++){
        z[k].re >>=1;
        z[k].im >>=1;
    }

    //rb->splash(HZ, "in MDCT calc");
        scale = fft_calc_unscaled(&s->fft, z);
       //    scale = fft_calc(&s->fft, z);

    //rb->splash(HZ, "in MDCT calc2");

    /* post rotation + reordering */

    for(k = 0; k < n4; k++)
    {
        CMUL(&z[k].re, &z[k].im, (z[k].re), (z[k].im), tcos[k], tsin[k]);
    }

    for(k = 0; k < n8; k++)
    {
        fixed32 r1,r2,r3,r4,r1n,r2n,r3n;

        r1 = z[n8 + k].im;
        r1n = r1 * -1;
        r2 = z[n8-1-k].re;
        r2n = r2 * -1;
        r3 = z[k+n8].re;
        r3n = r3 * -1;
        r4 = z[n8-k-1].im;

        output[2*k] = r1n;
        output[n2-1-2*k] = r1;

        output[2*k+1] = r2;
        output[n2-1-2*k-1] = r2n;

        output[n2 + 2*k]= r3n;
        output[n-1- 2*k]= r3n;

        output[n2 + 2*k+1]= r4;
        output[n-2 - 2 * k] = r4;
    }




}

void ff_mdct_end(MDCTContext *s)
{
  (void)s;

   // av_freep(&s->tcos);
   // av_freep(&s->tsin);
   // fft_end(&s->fft);
}




/* XXX: use same run/length optimization as mpeg decoders */
static void init_coef_vlc(VLC *vlc,
                          uint16_t **prun_table, uint16_t **plevel_table,
                          const CoefVLCTable *vlc_table, int tab)
{
    int n = vlc_table->n;
    const uint8_t *table_bits = vlc_table->huffbits;
    const uint32_t *table_codes = vlc_table->huffcodes;
    const uint16_t *levels_table = vlc_table->levels;
    uint16_t *run_table, *level_table;
    const uint16_t *p;
    int i, l, j, level;


    init_vlc(vlc, 9, n, table_bits, 1, 1, table_codes, 4, 4);

    run_table = runtabarray[tab];
    //run_table = av_malloc(n * sizeof(uint16_t));            //max n should be 1336

    level_table= levtabarray[tab];
    //level_table = av_malloc(n * sizeof(uint16_t));
    p = levels_table;
    i = 2;
    level = 1;
    while (i < n)
    {
        l = *p++;
        for(j=0;j<l;++j)
        {
            run_table[i] = j;
            level_table[i] = level;
            ++i;
        }
        ++level;
    }
    *prun_table = run_table;
    *plevel_table = level_table;
}

int wma_decode_init(WMADecodeContext* s, asf_waveformatex_t *wfx)
{
    //WMADecodeContext *s = avctx->priv_data;
    int i, flags1, flags2;
    fixed32 *window;
    uint8_t *extradata;
    fixed64 bps1;
    fixed32 high_freq;
    fixed64 bps;
    int sample_rate1;
    int coef_vlc_table;
    //    int filehandle;

    s->sample_rate = wfx->rate;
    s->nb_channels = wfx->channels;
    s->bit_rate = wfx->bitrate;
    s->block_align = wfx->blockalign;

    if (wfx->codec_id == ASF_CODEC_ID_WMAV1){
        s->version = 1;
    }else{
        s->version = 2;
    }

    /* extract flag infos */
    flags1 = 0;
    flags2 = 0;
    extradata = wfx->data;
    if (s->version == 1 && wfx->datalen >= 4) {
        flags1 = extradata[0] | (extradata[1] << 8);
        flags2 = extradata[2] | (extradata[3] << 8);
    }else if (s->version == 2 && wfx->datalen >= 6){
        flags1 = extradata[0] | (extradata[1] << 8) |
                 (extradata[2] << 16) | (extradata[3] << 24);
        flags2 = extradata[4] | (extradata[5] << 8);
    }
    s->use_exp_vlc = flags2 & 0x0001;
    s->use_bit_reservoir = flags2 & 0x0002;
    s->use_variable_block_len = flags2 & 0x0004;

    /* compute MDCT block size */
    if (s->sample_rate <= 16000){
        s->frame_len_bits = 9;
    }else if (s->sample_rate <= 22050 ||
             (s->sample_rate <= 32000 && s->version == 1)){
        s->frame_len_bits = 10;
    }else{
        s->frame_len_bits = 11;
    }
    s->frame_len = 1 << s->frame_len_bits;
    if (s-> use_variable_block_len)
    {
        int nb_max, nb;
        nb = ((flags2 >> 3) & 3) + 1;
        if ((s->bit_rate / s->nb_channels) >= 32000)
        {
            nb += 2;
        }
        nb_max = s->frame_len_bits - BLOCK_MIN_BITS;        //max is 11-7
        if (nb > nb_max)
            nb = nb_max;
        s->nb_block_sizes = nb + 1;
    }
    else
    {
        s->nb_block_sizes = 1;
    }

    /* init rate dependant parameters */
    s->use_noise_coding = 1;
    high_freq = fixmul64byfixed(itofix64(s->sample_rate), 0x8000);


    /* if version 2, then the rates are normalized */
    sample_rate1 = s->sample_rate;
    if (s->version == 2)
    {
        if (sample_rate1 >= 44100)
            sample_rate1 = 44100;
        else if (sample_rate1 >= 22050)
            sample_rate1 = 22050;
        else if (sample_rate1 >= 16000)
            sample_rate1 = 16000;
        else if (sample_rate1 >= 11025)
            sample_rate1 = 11025;
        else if (sample_rate1 >= 8000)
            sample_rate1 = 8000;
    }

    fixed64 tmp = itofix64(s->bit_rate);
    fixed64 tmp2 = itofix64(s->nb_channels * s->sample_rate);
    bps = fixdiv64(tmp, tmp2);
    fixed64 tim = fixmul64byfixed(bps, s->frame_len);
    fixed64 tmpi = fixdiv64(tim,itofix64(8));
    s->byte_offset_bits = av_log2(fixtoi64(tmpi)) + 2;

    /* compute high frequency value and choose if noise coding should
       be activated */
    bps1 = bps;
    if (s->nb_channels == 2)
        bps1 = fixmul32(bps,0x1999a);
    if (sample_rate1 == 44100)
    {
        if (bps1 >= 0x9c29)
            s->use_noise_coding = 0;
        else
            high_freq = fixmul64byfixed(high_freq,0x6666);
    }
    else if (sample_rate1 == 22050)
    {
        if (bps1 >= 0x128f6)
            s->use_noise_coding = 0;
        else if (bps1 >= 0xb852)
            high_freq = fixmul64byfixed(high_freq,0xb333);
        else
            high_freq = fixmul64byfixed(high_freq,0x999a);
    }
    else if (sample_rate1 == 16000)
    {
        if (bps > 0x8000)
            high_freq = fixmul64byfixed(high_freq,0x8000);
        else
            high_freq = fixmul64byfixed(high_freq,0x4ccd);
    }
    else if (sample_rate1 == 11025)
    {
        high_freq = fixmul64byfixed(high_freq,0xb3333);
    }
    else if (sample_rate1 == 8000)
    {
        if (bps <= 0xa000)
        {
            high_freq = fixmul64byfixed(high_freq,0x8000);
        }
        else if (bps > 0xc000)
        {
            s->use_noise_coding = 0;
        }
        else
        {
            high_freq = fixmul64byfixed(high_freq,0xa666);
        }
    }
    else
    {
        if (bps >= 0xcccd)
        {
            high_freq = fixmul64byfixed(high_freq,0xc000);
        }
        else if (bps >= 0x999a)
        {
            high_freq = fixmul64byfixed(high_freq,0x999a);
        }
        else
        {
            high_freq = fixmul64byfixed(high_freq,0x8000);
        }
    }

    /* compute the scale factor band sizes for each MDCT block size */
    {
        int a, b, pos, lpos, k, block_len, i, j, n;
        const uint8_t *table;

        if (s->version == 1)
        {
            s->coefs_start = 3;
        }
        else
        {
            s->coefs_start = 0;
        }
        for(k = 0; k < s->nb_block_sizes; ++k)
        {
            block_len = s->frame_len >> k;

            if (s->version == 1)
            {
                lpos = 0;
                for(i=0;i<25;++i)
                {
                    a = wma_critical_freqs[i];
                    b = s->sample_rate;
                    pos = ((block_len * 2 * a)  + (b >> 1)) / b;
                    if (pos > block_len)
                        pos = block_len;
                    s->exponent_bands[0][i] = pos - lpos;
                    if (pos >= block_len)
                    {
                        ++i;
                        break;
                    }
                    lpos = pos;
                }
                s->exponent_sizes[0] = i;
            }
            else
            {
                /* hardcoded tables */
                table = NULL;
                a = s->frame_len_bits - BLOCK_MIN_BITS - k;
                if (a < 3)
                {
                    if (s->sample_rate >= 44100)
                        table = exponent_band_44100[a];
                    else if (s->sample_rate >= 32000)
                        table = exponent_band_32000[a];
                    else if (s->sample_rate >= 22050)
                        table = exponent_band_22050[a];
                }
                if (table)
                {
                    n = *table++;
                    for(i=0;i<n;++i)
                        s->exponent_bands[k][i] = table[i];
                    s->exponent_sizes[k] = n;
                }
                else
                {
                    j = 0;
                    lpos = 0;
                    for(i=0;i<25;++i)
                    {
                        a = wma_critical_freqs[i];
                        b = s->sample_rate;
                        pos = ((block_len * 2 * a)  + (b << 1)) / (4 * b);
                        pos <<= 2;
                        if (pos > block_len)
                            pos = block_len;
                        if (pos > lpos)
                            s->exponent_bands[k][j++] = pos - lpos;
                        if (pos >= block_len)
                            break;
                        lpos = pos;
                    }
                    s->exponent_sizes[k] = j;
                }
            }

            /* max number of coefs */
            s->coefs_end[k] = (s->frame_len - ((s->frame_len * 9) / 100)) >> k;
            /* high freq computation */
            fixed64 tmp = itofix64(block_len<<2);
            tmp = fixmul64byfixed(tmp,high_freq);
            fixed64 tmp2 = itofix64(s->sample_rate);
            tmp2 += 0x8000;
            s->high_band_start[k] = fixtoi64(fixdiv64(tmp,tmp2));

            /*
            s->high_band_start[k] = (int)((block_len * 2 * high_freq) /
                                          s->sample_rate + 0.5);*/

            n = s->exponent_sizes[k];
            j = 0;
            pos = 0;
            for(i=0;i<n;++i)
            {
                int start, end;
                start = pos;
                pos += s->exponent_bands[k][i];
                end = pos;
                if (start < s->high_band_start[k])
                    start = s->high_band_start[k];
                if (end > s->coefs_end[k])
                    end = s->coefs_end[k];
                if (end > start)
                    s->exponent_high_bands[k][j++] = end - start;
            }
            s->exponent_high_sizes[k] = j;
        }
    }

    /* init MDCT */
    tcosarray[0] = tcos0; tcosarray[1] = tcos1; tcosarray[2] = tcos2; tcosarray[3] = tcos3;tcosarray[4] = tcos4;
    tsinarray[0] = tsin0; tsinarray[1] = tsin1; tsinarray[2] = tsin2; tsinarray[3] = tsin3;tsinarray[4] = tsin4;

    exparray[0] = exptab0; //exparray[1] = exptab1; exparray[2] = exptab2; exparray[3] = exptab3; exparray[4] = exptab4;
    revarray[0]=revtab0; revarray[1]=revtab1; revarray[2]=revtab2; revarray[3]=revtab3; revarray[4]=revtab4;

    for(i = 0; i < s->nb_block_sizes; ++i)
    {
        ff_mdct_init(&s->mdct_ctx[i], s->frame_len_bits - i + 1, 1);
    }

    /*ffmpeg uses malloc to only allocate as many window sizes as needed.  However, we're really only interested in the worst case memory usage.
    * In the worst case you can have 5 window sizes, 128 doubling up 2048
    * Smaller windows are handled differently.
    * Since we don't have malloc, just statically allocate this
    */
        fixed32 *temp[5];
    temp[0] = stat0;
    temp[1] = stat1;
    temp[2] = stat2;
    temp[3] = stat3;
    temp[4] = stat4;

    /* init MDCT windows : simple sinus window */
    for(i = 0; i < s->nb_block_sizes; i++)
    {
        int n, j;
        fixed32 alpha;
        n = 1 << (s->frame_len_bits - i);
        //window = av_malloc(sizeof(fixed32) * n);
        window = temp[i];

        //fixed32 n2 = itofix32(n<<1);        //2x the window length
        //alpha = fixdiv32(M_PI_F, n2);        //PI / (2x Window length) == PI<<(s->frame_len_bits - i+1)
        //printf("two values of alpha %16.10lf %16.10lf\n", fixtof64(alpha), fixtof64(M_PI_F>>(s->frame_len_bits - i+1)));
        alpha = M_PI_F>>(s->frame_len_bits - i+1);
        for(j=0;j<n;++j)
        {
            fixed32 j2 = itofix32(j) + 0x8000;
            window[n - j - 1] = fixsin32(fixmul32(j2,alpha));        //alpha between 0 and pi/2

        }
        //printf("created window\n");
        s->windows[i] = window;
        //printf("assigned window\n");
    }

    s->reset_block_lengths = 1;

    if (s->use_noise_coding)
    {
        /* init the noise generator */
        if (s->use_exp_vlc)
        {
            s->noise_mult = 0x51f;
        }
        else
        {
            s->noise_mult = 0xa3d;
        }


        {
            unsigned int seed;
            fixed32 norm;
            seed = 1;
            norm = 0;   // PJJ: near as makes any diff to 0!
            for (i=0;i<NOISE_TAB_SIZE;++i)
            {
                seed = seed * 314159 + 1;
                s->noise_table[i] = itofix32((int)seed) * norm;
            }
        }

        init_vlc(&s->hgain_vlc, 9, sizeof(hgain_huffbits),
                 hgain_huffbits, 1, 1,
                 hgain_huffcodes, 2, 2);
    }

    if (s->use_exp_vlc)
    {
        s->exp_vlc.table = vlcbuf3;
        s->exp_vlc.table_allocated = 1536;
        init_vlc(&s->exp_vlc, 9, sizeof(scale_huffbits),
                 scale_huffbits, 1, 1,
                 scale_huffcodes, 4, 4);
    }
    else
    {
        wma_lsp_to_curve_init(s, s->frame_len);
    }

    /* choose the VLC tables for the coefficients */
    coef_vlc_table = 2;
    if (s->sample_rate >= 32000)
    {
        if (bps1 < 0xb852)
            coef_vlc_table = 0;
        else if (bps1 < 0x128f6)
            coef_vlc_table = 1;
    }

    runtabarray[0] = runtab0; runtabarray[1] = runtab1;
    levtabarray[0] = levtab0; levtabarray[1] = levtab1;

    s->coef_vlc[0].table = vlcbuf1;
    s->coef_vlc[0].table_allocated = 24576/4;
    s->coef_vlc[1].table = vlcbuf2;
    s->coef_vlc[1].table_allocated = 14336/4;

    init_coef_vlc(&s->coef_vlc[0], &s->run_table[0], &s->level_table[0],
                  &coef_vlcs[coef_vlc_table * 2], 0);
    init_coef_vlc(&s->coef_vlc[1], &s->run_table[1], &s->level_table[1],
                  &coef_vlcs[coef_vlc_table * 2 + 1], 1);

    //filehandle = rb->open("/log.txt", O_WRONLY|O_CREAT|O_APPEND);



    //rb->fdprintf(filehandle,"In init:\n\nsample rate %d\nbit_rate %d\n version %d\n", s->sample_rate, s->bit_rate, s->version );
    //rb->fdprintf(filehandle,"use_noise_coding %d \nframe_len %d\nframe_len_bits %d\n", s->use_noise_coding, s->frame_len, s->frame_len_bits);
    //rb->fdprintf(filehandle,"use_bit_reservoir %d\n use_variable_block_len %d\n use_exp_vlc %d\n",s->use_bit_reservoir, s->use_variable_block_len, s->use_exp_vlc);
    //rb->fdprintf(filehandle,"use_noise_coding %d\n byte_offset_bits %d\n use_exp_vlc %d\n",s->use_noise_coding, s->byte_offset_bits, s->use_exp_vlc);
    //rb->close(filehandle);



    return 0;
}

/* interpolate values for a bigger or smaller block. The block must
   have multiple sizes */
static void interpolate_array(fixed32 *scale, int old_size, int new_size)
{
    int i, j, jincr, k;
    fixed32 v;



    if (new_size > old_size)
    {
        jincr = new_size / old_size;
        j = new_size;
        for(i = old_size - 1; i >=0; --i)
        {
            v = scale[i];
            k = jincr;
            do
            {
                scale[--j] = v;
            }
            while (--k);
        }
    }
    else if (new_size < old_size)
    {
        j = 0;
        jincr = old_size / new_size;
        for(i = 0; i < new_size; ++i)
        {
            scale[i] = scale[j];
            j += jincr;
        }
    }
}

/* compute x^-0.25 with an exponent and mantissa table. We use linear
   interpolation to reduce the mantissa table size at a small speed
   expense (linear interpolation approximately doubles the number of
   bits of precision). */
static inline fixed32 pow_m1_4(WMADecodeContext *s, fixed32 x)
{
    union {
        fixed64 f;
        unsigned int v;
    } u, t;
    unsigned int e, m;
    fixed64 a, b;

    u.f = x;
    e = u.v >> 23;
    m = (u.v >> (23 - LSP_POW_BITS)) & ((1 << LSP_POW_BITS) - 1);
    /* build interpolation scale: 1 <= t < 2. */
    t.v = ((u.v << LSP_POW_BITS) & ((1 << 23) - 1)) | (127 << 23);
    a = s->lsp_pow_m_table1[m];
    b = s->lsp_pow_m_table2[m];
    return lsp_pow_e_table[e] * (a + b * t.f);
}

static void wma_lsp_to_curve_init(WMADecodeContext *s, int frame_len)
{
    fixed32 wdel, a, b;
    int i, m;

    wdel = fixdiv32(M_PI_F, itofix32(frame_len));
    for (i=0; i<frame_len; ++i)
    {
        s->lsp_cos_table[i] = 0x20000 * fixcos32(wdel * i);    //wdel*i between 0 and pi

    }


    /* NOTE: these two tables are needed to avoid two operations in
       pow_m1_4 */
    b = itofix32(1);
    int ix = 0;
    for(i=(1 << LSP_POW_BITS) - 1;i>=0;i--)
    {
        m = (1 << LSP_POW_BITS) + i;
        a = m * (0x8000 / (1 << LSP_POW_BITS)); //PJJ
        a = pow_a_table[ix++];  // PJJ : further refinement
        s->lsp_pow_m_table1[i] = 2 * a - b;
        s->lsp_pow_m_table2[i] = b - a;
        b = a;
    }
}

/* NOTE: We use the same code as Vorbis here */
/* XXX: optimize it further with SSE/3Dnow */
static void wma_lsp_to_curve(WMADecodeContext *s,
                             fixed32 *out,
                             fixed32 *val_max_ptr,
                             int n,
                             fixed32 *lsp)
{
    int i, j;
    fixed32 p, q, w, v, val_max;

    val_max = 0;
    for(i=0;i<n;++i)
    {
        p = 0x8000;
        q = 0x8000;
        w = s->lsp_cos_table[i];
        for (j=1;j<NB_LSP_COEFS;j+=2)
        {
            q *= w - lsp[j - 1];
            p *= w - lsp[j];
        }
        p *= p * (0x20000 - w);
        q *= q * (0x20000 + w);
        v = p + q;
        v = pow_m1_4(s, v); // PJJ
        if (v > val_max)
            val_max = v;
        out[i] = v;
    }
    *val_max_ptr = val_max;
}

/* decode exponents coded with LSP coefficients (same idea as Vorbis) */
static void decode_exp_lsp(WMADecodeContext *s, int ch)
{
    fixed32 lsp_coefs[NB_LSP_COEFS];
    int val, i;

    for (i = 0; i < NB_LSP_COEFS; ++i)
    {
        if (i == 0 || i >= 8)
            val = get_bits(&s->gb, 3);
        else
            val = get_bits(&s->gb, 4);
        lsp_coefs[i] = lsp_codebook[i][val];
    }

    wma_lsp_to_curve(s,
                     s->exponents[ch],
                     &s->max_exponent[ch],
                     s->block_len,
                     lsp_coefs);
}

/* decode exponents coded with VLC codes */
static int decode_exp_vlc(WMADecodeContext *s, int ch)
{
    int last_exp, n, code;
    const uint16_t *ptr, *band_ptr;
    fixed32 v, max_scale;
    fixed32 *q,*q_end;

    band_ptr = s->exponent_bands[s->frame_len_bits - s->block_len_bits];
    ptr = band_ptr;
    q = s->exponents[ch];
    q_end = q + s->block_len;
    max_scale = 0;


    if (s->version == 1)        //wmav1 only
    {
        last_exp = get_bits(&s->gb, 5) + 10;
        /* XXX: use a table */
        v = pow_10_to_yover16[last_exp];
        max_scale = v;
        n = *ptr++;
        do
        {
            *q++ = v;
        }
        while (--n);
    }
    last_exp = 36;

    while (q < q_end)
    {
        code = get_vlc(&s->gb, &s->exp_vlc);
        if (code < 0)
        {
            return -1;
        }
        /* NOTE: this offset is the same as MPEG4 AAC ! */
        last_exp += code - 60;
        /* XXX: use a table */
        v = pow_10_to_yover16[last_exp];
        if (v > max_scale)
        {
            max_scale = v;
        }
        n = *ptr++;
        do
        {
            *q++ = v;

        }
        while (--n);
    }

    s->max_exponent[ch] = max_scale;
    return 0;
}

/* return 0 if OK. return 1 if last block of frame. return -1 if
   unrecorrable error. */
static int wma_decode_block(WMADecodeContext *s)
{
    int n, v, a, ch, code, bsize;
    int coef_nb_bits, total_gain, parse_exponents;
    static fixed32 window[BLOCK_MAX_SIZE * 2];        //crap can't do this locally on the device!  its big as the whole stack
    int nb_coefs[MAX_CHANNELS];
    fixed32 mdct_norm;
//int filehandle = rb->open("/mul.txt", O_WRONLY|O_CREAT|O_APPEND);
//    rb->fdprintf(filehandle,"\nIn wma_decode_block:\n use_variable_block_len %d\n nb_block_sizes %d\n reset_block_lengths %d\n", s->use_variable_block_len, s->nb_block_sizes, s->reset_block_lengths );

//    printf("***decode_block: %d:%d (%d)\n", s->frame_count - 1, s->block_num, s->block_len);
    /* compute current block length */
    if (s->use_variable_block_len)
    {
        n = av_log2(s->nb_block_sizes - 1) + 1;

        if (s->reset_block_lengths)
        {
            s->reset_block_lengths = 0;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
            {
                return -2;
            }
            s->prev_block_len_bits = s->frame_len_bits - v;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes)
            {
                return -3;
            }
            s->block_len_bits = s->frame_len_bits - v;
        }
        else
        {
            /* update block lengths */
            s->prev_block_len_bits = s->block_len_bits;
            s->block_len_bits = s->next_block_len_bits;
        }
        v = get_bits(&s->gb, n);

        //rb->fdprintf(filehandle,"v %d \n prev_block_len_bits %d\n block_len_bits %d\n", v, s->prev_block_len_bits, s->block_len_bits);
        //rb->close(filehandle);

    LOGF("v was %d", v);
        if (v >= s->nb_block_sizes)
        {
         // rb->splash(HZ*4, "v was %d", v);        //5, 7
            return -4;        //this is it
        }
        else{
              //rb->splash(HZ, "passed v block (%d)!", v);
      }
        s->next_block_len_bits = s->frame_len_bits - v;
    }
    else
    {
        /* fixed block len */
        s->next_block_len_bits = s->frame_len_bits;
        s->prev_block_len_bits = s->frame_len_bits;
        s->block_len_bits = s->frame_len_bits;
    }
    /* now check if the block length is coherent with the frame length */
    s->block_len = 1 << s->block_len_bits;

    if ((s->block_pos + s->block_len) > s->frame_len)
    {
        return -5;
    }

    if (s->nb_channels == 2)
    {
        s->ms_stereo = get_bits(&s->gb, 1);
    }
    v = 0;
    for (ch = 0; ch < s->nb_channels; ++ch)
    {
        a = get_bits(&s->gb, 1);
        s->channel_coded[ch] = a;
        v |= a;
    }
    /* if no channel coded, no need to go further */
    /* XXX: fix potential framing problems */
    if (!v)
    {
        goto next;
    }

    bsize = s->frame_len_bits - s->block_len_bits;

    /* read total gain and extract corresponding number of bits for
       coef escape coding */
    total_gain = 1;
    for(;;)
    {
        a = get_bits(&s->gb, 7);
        total_gain += a;
        if (a != 127)
        {
            break;
        }
    }

    if (total_gain < 15)
        coef_nb_bits = 13;
    else if (total_gain < 32)
        coef_nb_bits = 12;
    else if (total_gain < 40)
        coef_nb_bits = 11;
    else if (total_gain < 45)
        coef_nb_bits = 10;
    else
        coef_nb_bits = 9;
    /* compute number of coefficients */
    n = s->coefs_end[bsize] - s->coefs_start;

    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        nb_coefs[ch] = n;
    }
    /* complex coding */

    if (s->use_noise_coding)
    {

        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                int i, n, a;
                n = s->exponent_high_sizes[bsize];
                for(i=0;i<n;++i)
                {
                    a = get_bits(&s->gb, 1);
                    s->high_band_coded[ch][i] = a;
                    /* if noise coding, the coefficients are not transmitted */
                    if (a)
                        nb_coefs[ch] -= s->exponent_high_bands[bsize][i];
                }
            }
        }
        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                int i, n, val, code;

                n = s->exponent_high_sizes[bsize];
                val = (int)0x80000000;
                for(i=0;i<n;++i)
                {
                    if (s->high_band_coded[ch][i])
                    {
                        if (val == (int)0x80000000)
                        {
                            val = get_bits(&s->gb, 7) - 19;
                        }
                        else
                        {
                            code = get_vlc(&s->gb, &s->hgain_vlc);
                            if (code < 0)
                            {
                                return -6;
                            }
                            val += code - 18;
                        }
                        s->high_band_values[ch][i] = val;
                    }
                }
            }
        }
    }

    /* exposant can be interpolated in short blocks. */
    parse_exponents = 1;
    if (s->block_len_bits != s->frame_len_bits)
    {
        parse_exponents = get_bits(&s->gb, 1);
    }

    if (parse_exponents)
    {

        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                if (s->use_exp_vlc)
                {
                    if (decode_exp_vlc(s, ch) < 0)
                    {
                        return -7;
                    }
                }
                else
                {
                    decode_exp_lsp(s, ch);
                }
            }
        }
    }
    else
    {
        for(ch = 0; ch < s->nb_channels; ++ch)
        {
            if (s->channel_coded[ch])
            {
                interpolate_array(s->exponents[ch],
                                  1 << s->prev_block_len_bits,
                                  s->block_len);
            }
        }
    }
//ok up to here!
//printf("got here!\n");
//rb->splash(HZ, "in wma_decode_block 2");
    /* parse spectral coefficients : just RLE encoding */
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            VLC *coef_vlc;
            int level, run, sign, tindex;
            int16_t *ptr, *eptr;
            const int16_t *level_table, *run_table;

            /* special VLC tables are used for ms stereo because
               there is potentially less energy there */
            tindex = (ch == 1 && s->ms_stereo);
            coef_vlc = &s->coef_vlc[tindex];
            run_table = s->run_table[tindex];
            level_table = s->level_table[tindex];
            /* XXX: optimize */
            ptr = &s->coefs1[ch][0];
            eptr = ptr + nb_coefs[ch];
            memset(ptr, 0, s->block_len * sizeof(int16_t));



            for(;;)
            {
                code = get_vlc(&s->gb, coef_vlc);
                if (code < 0)
                {
                    return -8;
                }
                if (code == 1)
                {
                    /* EOB */
                    break;
                }
                else if (code == 0)
                {
                    /* escape */
                    level = get_bits(&s->gb, coef_nb_bits);
                    /* NOTE: this is rather suboptimal. reading
                       block_len_bits would be better */
                    run = get_bits(&s->gb, s->frame_len_bits);
                }
                else
                {
                    /* normal code */
                    run = run_table[code];
                    level = level_table[code];
                }
                sign = get_bits(&s->gb, 1);
                if (!sign)
                    level = -level;
                ptr += run;
                if (ptr >= eptr)
                {
                    return -9;
                }
                *ptr++ = level;


                /* NOTE: EOB can be omitted */
                if (ptr >= eptr)
                    break;
            }
        }
        if (s->version == 1 && s->nb_channels >= 2)
        {
            align_get_bits(&s->gb);
        }
    }

    {
        int n4 = s->block_len >> 1;
        //mdct_norm = 0x10000;
        //mdct_norm = fixdiv32(mdct_norm,itofix32(n4));

        mdct_norm = 0x10000>>(s->block_len_bits-1);        //theres no reason to do a divide by two in fixed precision ...

        if (s->version == 1)
        {
            fixed32 tmp = fixsqrt32(itofix32(n4));
            mdct_norm *= tmp; // PJJ : exercise this path
        }
    }



//rb->splash(HZ, "in wma_decode_block 3");
    /* finally compute the MDCT coefficients */
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            int16_t *coefs1;
            fixed32 *exponents, *exp_ptr;
            fixed32 *coefs, atemp;
            fixed64 mult;
            fixed64 mult1;
            fixed32 noise;
            int i, j, n, n1, last_high_band;
           fixed32 exp_power[HIGH_BAND_MAX_SIZE];

            //double test, mul;

    //total_gain, coefs1, mdctnorm are lossless

            coefs1 = s->coefs1[ch];
            exponents = s->exponents[ch];
            mult = fixdiv64(pow_table[total_gain],Fixed32To64(s->max_exponent[ch]));
         //   mul = fixtof64(pow_table[total_gain])/(s->block_len/2)/fixtof64(s->max_exponent[ch]);

            mult = fixmul64byfixed(mult, mdct_norm);        //what the hell?  This is actually fixed64*2^16!
            coefs = s->coefs[ch];                                            //VLC exponenents are used to get MDCT coef here!

        n=0;

            if (s->use_noise_coding)
            {
                mult1 = mult;

                /* very low freqs : noise */
                for(i = 0;i < s->coefs_start; ++i)
                {
                    *coefs++ = fixmul32(fixmul32(s->noise_table[s->noise_index],(*exponents++)),Fixed32From64(mult1));
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }

                n1 = s->exponent_high_sizes[bsize];

                /* compute power of high bands */
                exp_ptr = exponents +
                          s->high_band_start[bsize] -
                          s->coefs_start;
                last_high_band = 0; /* avoid warning */
                for (j=0;j<n1;++j)
                {
                    n = s->exponent_high_bands[s->frame_len_bits -
                                               s->block_len_bits][j];
                    if (s->high_band_coded[ch][j])
                    {
                        fixed32 e2, v;
                        e2 = 0;
                        for(i = 0;i < n; ++i)
                        {
                            v = exp_ptr[i];
                            e2 += v * v;
                        }
                        exp_power[j] = fixdiv32(e2,n);
                        last_high_band = j;
                    }
                    exp_ptr += n;
                }

                /* main freqs and high freqs */
                for(j=-1;j<n1;++j)
                {
                    if (j < 0)
                    {
                        n = s->high_band_start[bsize] -
                            s->coefs_start;
                    }
                    else
                    {
                        n = s->exponent_high_bands[s->frame_len_bits -
                                                   s->block_len_bits][j];
                    }
                    if (j >= 0 && s->high_band_coded[ch][j])
                    {
                        /* use noise with specified power */
                        fixed32 tmp = fixdiv32(exp_power[j],exp_power[last_high_band]);
                        mult1 = (fixed64)fixsqrt32(tmp);
                        /* XXX: use a table */
                        mult1 = mult1 * pow_table[s->high_band_values[ch][j]];
                        mult1 = fixdiv64(mult1,fixmul32(s->max_exponent[ch],s->noise_mult));
                        mult1 = fixmul64byfixed(mult1,mdct_norm);
                        for(i = 0;i < n; ++i)
                        {
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            *coefs++ = fixmul32(fixmul32(*exponents,noise),Fixed32From64(mult1));
                            ++exponents;
                        }
                    }
                    else
                    {
                        /* coded values + small noise */
                        for(i = 0;i < n; ++i)
                        {
                            // PJJ: check code path
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            *coefs++ = fixmul32(fixmul32(((*coefs1++) + noise),*exponents),mult);
                            ++exponents;
                        }
                    }
                }

                /* very high freqs : noise */
                n = s->block_len - s->coefs_end[bsize];
                mult1 = fixmul32(mult,exponents[-1]);
                for (i = 0; i < n; ++i)
                {
                    *coefs++ = fixmul32(s->noise_table[s->noise_index],Fixed32From64(mult1));
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }
            }
            else
            {

                /* XXX: optimize more */
                for(i = 0;i < s->coefs_start; ++i)
                    *coefs++ = 0;            //why do we do this step?!
                n = nb_coefs[ch];




                for(i = 0;i < n; ++i)
                {

                atemp = (fixed32)(coefs1[i]*mult>>16);
                 //atemp= ftofix32(coefs1[i] * fixtof64(exponents[i]) * fixtof64(mult>>16));    //this "works" in the sense that the mdcts converge
                *coefs++=fixmul32(atemp,exponents[i]);  //this does not work


                //atemp = ftofix32( coefs1[i]*mul* fixtof64(exponents[i]) );    //this doesn't seem to help any at all.
//                *coefs++=atemp;

               }                                                    //coefs1 could underflow?
                n = s->block_len - s->coefs_end[bsize];
                for(i = 0;i < n; ++i)
                    *coefs++ = 0;
            }
        }
    }


//rb->splash(HZ, "in wma_decode_block 3b");
    if (s->ms_stereo && s->channel_coded[1])
    {
        fixed32 a, b;
        int i;

        /* nominal case for ms stereo: we do it before mdct */
        /* no need to optimize this case because it should almost
           never happen */
        if (!s->channel_coded[0])
        {
            memset(s->coefs[0], 0, sizeof(fixed32) * s->block_len);
            s->channel_coded[0] = 1;
        }

        for(i = 0; i < s->block_len; ++i)
        {
            a = s->coefs[0][i];
            b = s->coefs[1][i];
            s->coefs[0][i] = a + b;
            s->coefs[1][i] = a - b;
        }
    }
//rb->splash(HZ, "in wma_decode_block 3c");
    /* build the window : we ensure that when the windows overlap
       their squared sum is always 1 (MDCT reconstruction rule) */
    /* XXX: merge with output */
    {
        int i, next_block_len, block_len, prev_block_len, n;
        fixed32 *wptr;

        block_len = s->block_len;
        prev_block_len = 1 << s->prev_block_len_bits;
        next_block_len = 1 << s->next_block_len_bits;
    //rb->splash(HZ, "in wma_decode_block 3d");        //got here
        /* right part */
        wptr = window + block_len;
        if (block_len <= next_block_len)
        {
            for(i=0;i<block_len;++i)
                *wptr++ = s->windows[bsize][i];
        }
        else
        {
            /* overlap */
            n = (block_len / 2) - (next_block_len / 2);
            for(i=0;i<n;++i)
                *wptr++ = itofix32(1);
            for(i=0;i<next_block_len;++i)
                *wptr++ = s->windows[s->frame_len_bits - s->next_block_len_bits][i];
            for(i=0;i<n;++i)
                *wptr++ = 0;
        }
//rb->splash(HZ, "in wma_decode_block 3e");
        /* left part */
        wptr = window + block_len;
        if (block_len <= prev_block_len)
        {
            for(i=0;i<block_len;++i)
                *--wptr = s->windows[bsize][i];
        }
        else
        {
            /* overlap */
            n = (block_len / 2) - (prev_block_len / 2);
            for(i=0;i<n;++i)
                *--wptr = itofix32(1);
            for(i=0;i<prev_block_len;++i)
                *--wptr = s->windows[s->frame_len_bits - s->prev_block_len_bits][i];
            for(i=0;i<n;++i)
                *--wptr = 0;
        }
    }


    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        if (s->channel_coded[ch])
        {
            static fixed32 output[BLOCK_MAX_SIZE * 2];
            fixed32 *ptr;
            int i, n4, index, n;

            n = s->block_len;
            n4 = s->block_len >>1;
        //rb->splash(HZ, "in wma_decode_block 4");
            ff_imdct_calc(&s->mdct_ctx[bsize],
                          output,
                          s->coefs[ch],
                          s->mdct_tmp);

            /* XXX: optimize all that by build the window and
               multipying/adding at the same time */
            /* multiply by the window */
//already broken here!





            for(i=0;i<n * 2;++i)
            {

                output[i] = fixmul32(output[i], window[i]);
                //output[i] *= window[i];

            }


            /* add in the frame */
            index = (s->frame_len / 2) + s->block_pos - n4;
            ptr = &s->frame_out[ch][index];

            for(i=0;i<n * 2;++i)
            {
                *ptr += output[i];
                ++ptr;


            }


            /* specific fast case for ms-stereo : add to second
               channel if it is not coded */
            if (s->ms_stereo && !s->channel_coded[1])
            {
                ptr = &s->frame_out[1][index];
                for(i=0;i<n * 2;++i)
                {
                    *ptr += output[i];
                    ++ptr;
                }
            }
        }
    }
next:
    /* update block number */
    ++s->block_num;
    s->block_pos += s->block_len;
    if (s->block_pos >= s->frame_len)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* decode a frame of frame_len samples */
static int wma_decode_frame(WMADecodeContext *s, int16_t *samples)
{
    int ret, i, n, a, ch, incr;
    int16_t *ptr;
    fixed32 *iptr;
   // rb->splash(HZ, "in wma_decode_frame");

    /* read each block */
    s->block_num = 0;
    s->block_pos = 0;


    for(;;)
    {
        ret = wma_decode_block(s);
        if (ret < 0)
        {
            LOGF("wma_decode_block: %d",ret);
            //rb->splash(HZ*4, "wma_decode_block failed with ret %d", ret);
            return -1;
        }
        if (ret)
        {
            break;
        }
    }

    /* convert frame to integer */
    n = s->frame_len;
    incr = s->nb_channels;
    for(ch = 0; ch < s->nb_channels; ++ch)
    {
        ptr = samples + ch;
        iptr = s->frame_out[ch];

        for (i=0;i<n;++i)
        {
            a = fixtoi32(*iptr++)<<1;        //ugly but good enough for now





            if (a > 32767)
            {
                a = 32767;
            }
            else if (a < -32768)
            {
                a = -32768;
            }
            *ptr = a;
            ptr += incr;
        }
        /* prepare for next block */
        memmove(&s->frame_out[ch][0], &s->frame_out[ch][s->frame_len],
                s->frame_len * sizeof(fixed32));
        /* XXX: suppress this */
        memset(&s->frame_out[ch][s->frame_len], 0,
               s->frame_len * sizeof(fixed32));
    }

    return 0;
}

int wma_decode_superframe(WMADecodeContext* s,
                                 void *data,
                                 int *data_size,
                                 uint8_t *buf,
                                 int buf_size)
{
    //WMADecodeContext *s = avctx->priv_data;
    int nb_frames, bit_offset, i, pos, len;
    uint8_t *q;
    int16_t *samples;

    if (buf_size==0)
    {
        s->last_superframe_len = 0;
        return 0;
    }

    samples = data;
    init_get_bits(&s->gb, buf, buf_size*8);
    if (s->use_bit_reservoir)
    {
        /* read super frame header */
        get_bits(&s->gb, 4); /* super frame index */
        nb_frames = get_bits(&s->gb, 4) - 1;

        bit_offset = get_bits(&s->gb, s->byte_offset_bits + 3);
        if (s->last_superframe_len > 0)
        {
            /* add bit_offset bits to last frame */
            if ((s->last_superframe_len + ((bit_offset + 7) >> 3)) >
                    MAX_CODED_SUPERFRAME_SIZE)
            {
                goto fail;
            }
            q = s->last_superframe + s->last_superframe_len;
            len = bit_offset;
            while (len > 0)
            {
                *q++ = (get_bits)(&s->gb, 8);
                len -= 8;
            }
            if (len > 0)
            {
                *q++ = (get_bits)(&s->gb, len) << (8 - len);
            }

            /* XXX: bit_offset bits into last frame */
            init_get_bits(&s->gb, s->last_superframe, MAX_CODED_SUPERFRAME_SIZE*8);
            /* skip unused bits */
            if (s->last_bitoffset > 0)
                skip_bits(&s->gb, s->last_bitoffset);
            /* this frame is stored in the last superframe and in the
               current one */
            if (wma_decode_frame(s, samples) < 0)
            {
                goto fail;
            }
            samples += s->nb_channels * s->frame_len;
        }

        /* read each frame starting from bit_offset */
        pos = bit_offset + 4 + 4 + s->byte_offset_bits + 3;
        init_get_bits(&s->gb, buf + (pos >> 3), (MAX_CODED_SUPERFRAME_SIZE - (pos >> 3))*8);
        len = pos & 7;
        if (len > 0)
            skip_bits(&s->gb, len);

        s->reset_block_lengths = 1;
        for(i=0;i<nb_frames;++i)
        {
            if (wma_decode_frame(s, samples) < 0)
            {
                goto fail;
            }
            samples += s->nb_channels * s->frame_len;
        }

        /* we copy the end of the frame in the last frame buffer */
        pos = get_bits_count(&s->gb) + ((bit_offset + 4 + 4 + s->byte_offset_bits + 3) & ~7);
        s->last_bitoffset = pos & 7;
        pos >>= 3;
        len = buf_size - pos;
        if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0)
        {
            goto fail;
        }
        s->last_superframe_len = len;
        memcpy(s->last_superframe, buf + pos, len);
    }
    else
    {
        /* single frame decode */
        if (wma_decode_frame(s, samples) < 0)
        {
            goto fail;
        }
        samples += s->nb_channels * s->frame_len;
    }
    *data_size = (int8_t *)samples - (int8_t *)data;
    return s->block_align;
fail:
    /* when error, we reset the bit reservoir */
    s->last_superframe_len = 0;
    return -1;
}

/*void free_vlc(VLC *vlc)
{
    //av_free(vlc->table);
}
*/
int wma_decode_end(WMADecodeContext *s)
{
    (void)s;
/*    WMADecodeContext *s = avctx->priv_data;
    int i;

    for(i = 0; i < s->nb_block_sizes; ++i)
        ff_mdct_end(&s->mdct_ctx[i]);
 //   for(i = 0; i < s->nb_block_sizes; ++i)    //now statically allocated
 //       av_free(s->windows[i]);

    if (s->use_exp_vlc)
    {
        free_vlc(&s->exp_vlc);
    }
    if (s->use_noise_coding)
    {
        free_vlc(&s->hgain_vlc);
    }
    for(i = 0;i < 2; ++i)
    {
       // free_vlc(&s->coef_vlc[i]);
      //  av_free(s->run_table[i]);
       // av_free(s->level_table[i]);
    }
*/
    return 0;
}
