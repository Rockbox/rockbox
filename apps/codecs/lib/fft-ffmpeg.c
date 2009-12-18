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
#include <codecs/lib/codeclib.h>


#define PRECISION       16

#define fixtof32(x)       (float)((float)(x) / (float)(1 << PRECISION))        //does not work on int64_t!
#define ftofix32(x)       ((fixed32)((x) * (float)(1 << PRECISION) + ((x) < 0 ? -0.5 : 0.5)))
#define itofix32(x)       ((x) << PRECISION)
#define fixtoi32(x)       ((x) >> PRECISION)
static void ff_fft_permute_c(FFTContext *s, FFTComplex *z);
#define MUL(a,b) \
({ int32_t _ta=(a), _tb=(b), _tc; \
   _tc=(_ta & 0xffff)*(_tb >> 16)+(_ta >> 16)*(_tb & 0xffff); (int32_t)(((_tc >> 14))+ (((_ta >> 16)*(_tb >> 16)) << 2 )); })

#define DECLARE_ALIGNED_16(type, arg) type arg __attribute__ ((aligned(16)))
#define M_PI_F          0x0003243f  /* pi 16.16 */
#define M_SQRT1_2_F     0x0000b505  /* 1/sqrt(2) 16.16 */
#define av_unused
#define FFT_SIZE 1024

int32_t flattabs[8+16+32+64+128+256+512+1024+2048];
int32_t * tabstabstabs[] = { flattabs, flattabs+8, flattabs+8+16,flattabs+8+16+32, flattabs+8+16+32+64,
                             flattabs+8+16+32+64+128, flattabs+8+16+32+64+128+256,
                             flattabs+8+16+32+64+128+256+512, flattabs+8+16+32+64+128+256+512+1024 };



/* Global variables for holding number of muls and adds */
int muls, adds;

/* cos(2*pi*x/n) for 0<=x<=n/4, followed by its reverse */
/*
DECLARE_ALIGNED_16(FFTSample, ff_cos_16[8]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_32[16]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_64[32]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_128[64]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_256[128]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_512[256]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_1024[512]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_2048[1024]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_4096[2048]);
*/
/*
DECLARE_ALIGNED_16(FFTSample, ff_cos_8192[4096]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_16384[8192]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_32768[16384]);
DECLARE_ALIGNED_16(FFTSample, ff_cos_65536[32768]);
*/
/* some of these should probably be in iram */
static FFTSample * ff_cos_16 = NULL;
static FFTSample * ff_cos_32 = NULL;
static FFTSample * ff_cos_64 = NULL;
static FFTSample * ff_cos_128 = NULL;
static FFTSample * ff_cos_256 = NULL;
static FFTSample * ff_cos_512 = NULL;
static FFTSample * ff_cos_1024 = NULL;
static FFTSample * ff_cos_2048 = NULL;
static FFTSample * ff_cos_4096 = NULL;

/*
FFTSample * const ff_cos_tabs[] = {
    ff_cos_16, ff_cos_32, ff_cos_64, ff_cos_128, ff_cos_256, ff_cos_512, ff_cos_1024,
    ff_cos_2048, ff_cos_4096, ff_cos_8192, ff_cos_16384, ff_cos_32768, ff_cos_65536,
};
*/

static FFTSample ** const ff_cos_tabs[] = {
    &ff_cos_16, &ff_cos_32, &ff_cos_64, &ff_cos_128, &ff_cos_256, &ff_cos_512, &ff_cos_1024,
    &ff_cos_2048, &ff_cos_4096
};

uint16_t revtab[1<<12];
static bool revtab_initialised = false;

FFTComplex exptab[1<<11], tmp_buf[1<<12];

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

//int ff_fft_init(FFTContext *s, int nbits, int inverse)
int ff_fft_init(void *arg_s, int nbits, int inverse)
{
    int i, j, n;
    FFTContext *s = (FFTContext *)(arg_s);
  
    fixed32 temp, phase;

    if (nbits < 2 || nbits > 12)
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
    
    /* FIXME these should really just be precalculated */
    for(j=4; j<=nbits; j++) {
        int m = 1<<j;
        //fixed32 freq = (M_PI_F / m ) << 1;
        FFTSample *tab = *(ff_cos_tabs[j-4]);
        
        /* if tab is NULL, table hasn't been built yet, fill it in now */
        /* effectively do this operation once and once only */
        if(NULL==tab)
        {
          tab = tabstabstabs[j-4];

          *(ff_cos_tabs[j-4]) = tab;
        
          for(i=0; i<=m/4; i++) {            
              phase = ((i<<16) / m)<<16;
              fsincos(phase, &temp);//tab[i] = cos(i*freq);
              tab[i] = temp>>15;
          }
            
          for(i=1; i<m/4; i++)
              tab[m/2-i] = tab[i];
        }
    }

    /* FIXME: there ought to be some shortcuts here, I just haven't
       thought of any yet.  Ideally we would be able to massage data
       so that it comes out in standard or standard-bitrev order
       rather than this funky thing */
    if( !revtab_initialised )
    {
      /* fully initialise the revtab for entire 1<<12 range */
      const int MAX_REVTAB=1<<12;
      
      for(i=0; i<MAX_REVTAB ; i++)
          s->revtab[-split_radix_permutation(i, MAX_REVTAB, s->inverse) & (MAX_REVTAB-1)] = i;
          
      revtab_initialised = true;
    }

    s->tmp_buf = tmp_buf;
    
    return 0;
}

static void ff_fft_permute_c(FFTContext *s, FFTComplex *z)
{
    int j, k, np;
    FFTComplex tmp;
    const uint16_t *revtab = s->revtab;
    np = 1 << s->nbits;
    
    const int revtab_shift = (12 - s->nbits);

    if (s->tmp_buf) {
        /* TODO: handle split-radix permute in a more optimal way, probably in-place */
        for(j=0;j<np;j++) s->tmp_buf[revtab[j]>>revtab_shift] = z[j];
        memcpy(z, s->tmp_buf, np * sizeof(FFTComplex));
        return;
    }

    /* reverse */
    for(j=0;j<np;j++) {
        k = revtab[j]>>revtab_shift;
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
    FFTSample t1, t2, t3, t4, t5, t6, t7, t8;

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
/*
DECL_FFT(8192,4096,2048)
DECL_FFT(16384,8192,4096)
DECL_FFT(32768,16384,8192)
DECL_FFT(65536,32768,16384)
*/

static void (*fft_dispatch[])(FFTComplex*) = {
    fft4, fft8, fft16, fft32, fft64, fft128, fft256, fft512, fft1024,
    fft2048, fft4096
    /*, fft8192, fft16384, fft32768, fft65536,*/
};

void ff_fft_calc_c(FFTContext *s, FFTComplex *z)
{
    fft_dispatch[s->nbits-2](z);
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
