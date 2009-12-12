/*
 * FFT/IFFT transforms
 * Copyright (c) 2008 Loren Merritt
 * Copyright (c) 2002 Fabrice Bellard
 * Partly based on libdjbfft by D. J. Bernstein
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
 */

/**
 * @file libavcodec/fft.c
 * FFT/IFFT transforms.
 */

#include "fft.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#define PRECISION       16

#define fixtof32(x)       (float)((float)(x) / (float)(1 << PRECISION))        //does not work on int64_t!
#define ftofix32(x)       ((fixed32)((x) * (float)(1 << PRECISION) + ((x) < 0 ? -0.5 : 0.5)))
#define itofix32(x)       ((x) << PRECISION)
#define fixtoi32(x)       ((x) >> PRECISION)
static inline fixed32 fixmul32(fixed32 x, fixed32 y);
static inline fixed32 fixdiv32(fixed32 x, fixed32 y);
#define MUL(a,b) \
({ int32_t _ta=(a), _tb=(b), _tc; \
   _tc=(_ta & 0xffff)*(_tb >> 16)+(_ta >> 16)*(_tb & 0xffff); (int32_t)(((_tc >> 14))+ (((_ta >> 16)*(_tb >> 16)) << 2 )); })

#define DECLARE_ALIGNED_16(type, arg) type arg __attribute__ ((aligned(16)))
#define M_PI_F          0x0003243f  /* pi 16.16 */
#define M_SQRT1_2_F     0x0000b505  /* 1/sqrt(2) 16.16 */
#define av_malloc malloc
#define av_unused
#define av_cold
#define av_freep free
#define FFT_SIZE 1024

/* Global variables for holding number of muls and adds */
int muls, adds;

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


/* cos(2*pi*x/n) for 0<=x<=n/4, followed by its reverse */
DECLARE_ALIGNED_16(FFTSample, ff_cos_16[8]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_32[16]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_64[32]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_128[64]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_256[128]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_512[256]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_1024[512]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_2048[1024]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_4096[2048]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_8192[4096]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_16384[8192]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_32768[16384]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_65536[32768]);
FFTSample * const ff_cos_tabs[] = {
    ff_cos_16, ff_cos_32, ff_cos_64, ff_cos_128, ff_cos_256, ff_cos_512, ff_cos_1024,
    ff_cos_2048, ff_cos_4096, ff_cos_8192, ff_cos_16384, ff_cos_32768, ff_cos_65536,
};
uint16_t revtab[1<<16];
FFTComplex exptab[1<<15], tmp_buf[1<<16];

static int split_radix_permutation(int i, int n, int inverse)
{
    int m;
    if(n <= 2) return i&1;
    m = n >> 1;
    if(!(i&m))            return split_radix_permutation(i, m, inverse)*2;
    m >>= 1;
    if(inverse == !(i&m)) return split_radix_permutation(i, m, inverse)*4 + 1;
    else                  return split_radix_permutation(i, m, inverse)*4 - 1;
}

av_cold int ff_fft_init(FFTContext *s, int nbits, int inverse)
{
    int i, j, n;
  
    fixed32 temp, phase;

    if (nbits < 2 || nbits > 16)
        return -1;
    s->nbits = nbits;
    n = 1 << nbits;

    s->tmp_buf = NULL;
    s->exptab  = exptab;
    s->revtab = revtab;
    s->inverse = inverse;

    s->fft_permute = ff_fft_permute_c;
    s->fft_calc    = ff_fft_calc_c;
    s->split_radix = 1;
  /*s->imdct_calc  = ff_imdct_calc_c;
    s->imdct_half  = ff_imdct_half_c;
    s->mdct_calc   = ff_mdct_calc_c;
    s->exptab1     = NULL;
    s->split_radix = 1;*/

  /*if (ARCH_ARM)     ff_fft_init_arm(s);
    if (HAVE_ALTIVEC) ff_fft_init_altivec(s);
    if (HAVE_MMX)     ff_fft_init_mmx(s);*/
    
    for(j=4; j<=nbits; j++) {
        int m = 1<<j;
        //fixed32 freq = (M_PI_F / m ) << 1;
        FFTSample *tab = ff_cos_tabs[j-4];
        for(i=0; i<=m/4; i++) {            
            phase = ((i<<16) / m)<<16;
            fsincos(phase, &temp);//tab[i] = cos(i*freq);
            tab[i] = temp>>15;
        }
            
        for(i=1; i<m/4; i++)
            tab[m/2-i] = tab[i];
    }
    for(i=0; i<n; i++)
        s->revtab[-split_radix_permutation(i, n, s->inverse) & (n-1)] = i;
    s->tmp_buf = tmp_buf;
    

    return 0;
}

void ff_fft_permute_c(FFTContext *s, FFTComplex *z)
{
    int j, k, np;
    FFTComplex tmp;
    const uint16_t *revtab = s->revtab;
    np = 1 << s->nbits;

    if (s->tmp_buf) {
        /* TODO: handle split-radix permute in a more optimal way, probably in-place */
        for(j=0;j<np;j++) s->tmp_buf[revtab[j]] = z[j];
        memcpy(z, s->tmp_buf, np * sizeof(FFTComplex));
        return;
    }

    /* reverse */
    for(j=0;j<np;j++) {
        k = revtab[j];
        if (k < j) {
            tmp = z[k];
            z[k] = z[j];
            z[j] = tmp;
        }
    }
}

#define sqrthalf M_SQRT1_2_F

#define BF(x,y,a,b) {\
    x = a - b;\
    y = a + b;\
    adds += 2;\
}

#define BUTTERFLIES(a0,a1,a2,a3) {\
    BF(t3, t5, t5, t1);\
    BF(a2.re, a0.re, a0.re, t5);\
    BF(a3.im, a1.im, a1.im, t3);\
    BF(t4, t6, t2, t6);\
    BF(a3.re, a1.re, a1.re, t4);\
    BF(a2.im, a0.im, a0.im, t6);\
}

// force loading all the inputs before storing any.
// this is slightly slower for small data, but avoids store->load aliasing
// for addresses separated by large powers of 2.
#define BUTTERFLIES_BIG(a0,a1,a2,a3) {\
    FFTSample r0=a0.re, i0=a0.im, r1=a1.re, i1=a1.im;\
    BF(t3, t5, t5, t1);\
    BF(a2.re, a0.re, r0, t5);\
    BF(a3.im, a1.im, i1, t3);\
    BF(t4, t6, t2, t6);\
    BF(a3.re, a1.re, r1, t4);\
    BF(a2.im, a0.im, i0, t6);\
}

#define TRANSFORM(a0,a1,a2,a3,wre,wim) {\
    t1 = fixmul32(a2.re, wre) + fixmul32(a2.im, wim);\
    t2 = fixmul32(a2.im, wre) - fixmul32(a2.re, wim);\
    t5 = fixmul32(a3.re, wre) - fixmul32(a3.im, wim);\
    t6 = fixmul32(a3.im, wre) + fixmul32(a3.re, wim);\
    muls += 8;\
    BUTTERFLIES(a0,a1,a2,a3)\
}

/*optimized for wre==wim*/
#define TRANSFORM_EQUAL(a0,a1,a2,a3,w) {\
    t3 = fixmul32(a2.re, w);\
    t4 = fixmul32(a2.im, w);\
    t7 = fixmul32(a3.re, w);\
    t8 = fixmul32(a3.im, w);\
    t1 = t3 + t4;\
    t2 = t4 - t3;\
    t5 = t7 - t8;\
    t6 = t8 + t7;\
    BUTTERFLIES(a0,a1,a2,a3)\
}

#define TRANSFORM_ZERO(a0,a1,a2,a3) {\
    t1 = a2.re;\
    t2 = a2.im;\
    t5 = a3.re;\
    t6 = a3.im;\
    BUTTERFLIES(a0,a1,a2,a3)\
}

/* z[0...8n-1], w[1...2n-1] */
#define PASS(name)\
static void name(FFTComplex *z, const FFTSample *wre, unsigned int n)\
{\
    FFTSample t1, t2, t3, t4, t5, t6;\
    int o1 = 2*n;\
    int o2 = 4*n;\
    int o3 = 6*n;\
    muls += 3;\
    const FFTSample *wim = wre+o1;\
    n--;\
    adds += 2;\
\
    TRANSFORM_ZERO(z[0],z[o1],z[o2],z[o3]);\
    TRANSFORM(z[1],z[o1+1],z[o2+1],z[o3+1],wre[1],wim[-1]);\
    do {\
        z += 2;\
        wre += 2;\
        wim -= 2;\
        TRANSFORM(z[0],z[o1],z[o2],z[o3],wre[0],wim[0]);\
        TRANSFORM(z[1],z[o1+1],z[o2+1],z[o3+1],wre[1],wim[-1]);\
	adds += 4;\
    } while(--n);\
}

PASS(pass)
#undef BUTTERFLIES
#define BUTTERFLIES BUTTERFLIES_BIG
PASS(pass_big)

#define DECL_FFT(n,n2,n4)\
static void fft##n(FFTComplex *z)\
{\
    fft##n2(z);\
    fft##n4(z+n4*2);\
    fft##n4(z+n4*3);\
    pass(z,ff_cos_##n,n4/2);\
}

static void fft4(FFTComplex *z)
{
    FFTSample t1, t2, t3, t4, t5, t6, t7, t8;

    BF(t3, t1, z[0].re, z[1].re);
    BF(t8, t6, z[3].re, z[2].re);
    BF(z[2].re, z[0].re, t1, t6);
    BF(t4, t2, z[0].im, z[1].im);
    BF(t7, t5, z[2].im, z[3].im);
    BF(z[3].im, z[1].im, t4, t8);
    BF(z[3].re, z[1].re, t3, t7);
    BF(z[2].im, z[0].im, t2, t5);
}

static void fft8(FFTComplex *z)
{
    FFTSample t1, t2, t3, t4, t5, t6, t7, t8;

    fft4(z);

    BF(t1, z[5].re, z[4].re, -z[5].re);
    BF(t2, z[5].im, z[4].im, -z[5].im);
    BF(t3, z[7].re, z[6].re, -z[7].re);
    BF(t4, z[7].im, z[6].im, -z[7].im);
    BF(t8, t1, t3, t1);
    BF(t7, t2, t2, t4);
    BF(z[4].re, z[0].re, z[0].re, t1);
    BF(z[4].im, z[0].im, z[0].im, t2);
    BF(z[6].re, z[2].re, z[2].re, t7);
    BF(z[6].im, z[2].im, z[2].im, t8);

    //TRANSFORM(z[1],z[3],z[5],z[7],sqrthalf,sqrthalf);
    TRANSFORM_EQUAL(z[1],z[3],z[5],z[7],sqrthalf)
}

#ifndef CONFIG_SMALL
static void fft16(FFTComplex *z)
{
    FFTSample t1, t2, t3, t4, t5, t6;

    fft8(z);
    fft4(z+8);
    fft4(z+12);

    TRANSFORM_ZERO(z[0],z[4],z[8],z[12]);
    //TRANSFORM(z[2],z[6],z[10],z[14],sqrthalf,sqrthalf);
    TRANSFORM_EQUAL(z[2],z[6],z[10],z[14],sqrthalf);
    TRANSFORM(z[1],z[5],z[9],z[13],ff_cos_16[1],ff_cos_16[3]);
    TRANSFORM(z[3],z[7],z[11],z[15],ff_cos_16[3],ff_cos_16[1]);
}
#else
DECL_FFT(16,8,4)
#endif
DECL_FFT(32,16,8)
DECL_FFT(64,32,16)
DECL_FFT(128,64,32)
DECL_FFT(256,128,64)
DECL_FFT(512,256,128)
#ifndef CONFIG_SMALL
#define pass pass_big
#endif
DECL_FFT(1024,512,256)
DECL_FFT(2048,1024,512)
DECL_FFT(4096,2048,1024)
DECL_FFT(8192,4096,2048)
DECL_FFT(16384,8192,4096)
DECL_FFT(32768,16384,8192)
DECL_FFT(65536,32768,16384)

static void (*fft_dispatch[])(FFTComplex*) = {
    fft4, fft8, fft16, fft32, fft64, fft128, fft256, fft512, fft1024,
    fft2048, fft4096, fft8192, fft16384, fft32768, fft65536,
};

void ff_fft_calc_c(FFTContext *s, FFTComplex *z)
{
    fft_dispatch[s->nbits-2](z);
}

/* Fixed-point arithmetic functions */
static inline fixed32 fixmul32(fixed32 x, fixed32 y)
{
    fixed64 temp;
    temp = x;
    temp *= y;

    temp >>= PRECISION;

    return (fixed32)temp;
}

static inline fixed32 fixdiv32(fixed32 x, fixed32 y)
{
    fixed64 temp;
    temp = x << PRECISION;
    temp /= y;

    return (fixed32)temp;
}

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


#if 0
int main (void)
{
    int             j;
    const long      N = FFT_SIZE;
    double          r[FFT_SIZE] = {0.0}, i[FFT_SIZE] = {0.0};
    long            n;
    double          t;
    double          amp, phase;
    clock_t         start, end;
    double          exec_time = 0;
    FFTContext      s;
    FFTComplex      z[FFT_SIZE];
    memset(z, 0, 64*sizeof(FFTComplex));

    /* Generate saw-tooth test data */
    for (n = 0; n < FFT_SIZE; n++)
    {
        t = (2 * M_PI * n)/N;
        /*z[n].re =  1.1      + sin(      t) +                
                   0.5      * sin(2.0 * t) +
                  (1.0/3.0) * sin(3.0 * t) +
                   0.25     * sin(4.0 * t) +
                   0.2      * sin(5.0 * t) +
                  (1.0/6.0) * sin(6.0 * t) +
                  (1.0/7.0) * sin(7.0 * t) ;*/
        z[n].re  =  ftofix32(cos(2*M_PI*n/64));
        //printf("z[%d] = %f\n", n, z[n].re);
        //getchar();
    }

    ff_fft_init(&s, 10, 1);
//start = clock();
//for(n = 0; n < 1000000; n++)
    ff_fft_permute_c(&s, z);
    ff_fft_calc_c(&s, z);
//end   = clock();
//exec_time = (((double)end-(double)start)/CLOCKS_PER_SEC);
    for(j = 0; j < FFT_SIZE; j++)
    {
        printf("%8.4f\n", sqrt(pow(fixtof32(z[j].re),2)+ pow(fixtof32(z[j].im), 2)));	
        //getchar();
    }
    printf("muls = %d, adds = %d\n", muls, adds);
//printf(" Time elapsed = %f\n", exec_time);
    //ff_fft_end(&s);

}
#endif
