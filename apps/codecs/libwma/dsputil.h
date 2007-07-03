/*
 * DSP utils
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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
 * @file dsputil.h
 * DSP utils.
 * note, many functions in here may use MMX which trashes the FPU state, it is
 * absolutely necessary to call emms_c() between dsp & float/double code
 */

#ifndef DSPUTIL_H
#define DSPUTIL_H

#include "common.h"

//#define DEBUG

void dsputil_static_init(void);

/* FFT computation */

/* NOTE: soon integer code will be added, so you must use the
   FFTSample type */
typedef float FFTSample;

typedef struct FFTComplex {
    FFTSample re, im;
} FFTComplex;

typedef struct FFTContext {
    int nbits;
    int inverse;
    uint16_t *revtab;
    FFTComplex *exptab;
    FFTComplex *exptab1; /* only used by SSE code */
    void (*fft_calc)(struct FFTContext *s, FFTComplex *z);
} FFTContext;

int fft_inits(FFTContext *s, int nbits, int inverse);
void fft_permute(FFTContext *s, FFTComplex *z);
void fft_calc_c(FFTContext *s, FFTComplex *z);
void fft_calc_sse(FFTContext *s, FFTComplex *z);
void fft_calc_altivec(FFTContext *s, FFTComplex *z);

static inline void fft_calc(FFTContext *s, FFTComplex *z)
{
    s->fft_calc(s, z);
}
void fft_end(FFTContext *s);

/* MDCT computation */

typedef struct MDCTContext {
    int n;  /* size of MDCT (i.e. number of input data * 2) */
    int nbits; /* n = 2^nbits */
    /* pre/post rotation tables */
    FFTSample *tcos;
    FFTSample *tsin;
    FFTContext fft;
} MDCTContext;

int ff_mdct_init(MDCTContext *s, int nbits, int inverse);
void ff_imdct_calc(MDCTContext *s, FFTSample *output,
                const FFTSample *input, FFTSample *tmp);
void ff_mdct_calc(MDCTContext *s, FFTSample *out,
               const FFTSample *input, FFTSample *tmp);
void ff_mdct_end(MDCTContext *s);

#define WARPER8_16(name8, name16)\
static int name16(void /*MpegEncContext*/ *s, uint8_t *dst, uint8_t *src, int stride, int h){\
    return name8(s, dst           , src           , stride, h)\
          +name8(s, dst+8         , src+8         , stride, h);\
}

#define WARPER8_16_SQ(name8, name16)\
static int name16(void /*MpegEncContext*/ *s, uint8_t *dst, uint8_t *src, int stride, int h){\
    int score=0;\
    score +=name8(s, dst           , src           , stride, 8);\
    score +=name8(s, dst+8         , src+8         , stride, 8);\
    if(h==16){\
        dst += 8*stride;\
        src += 8*stride;\
        score +=name8(s, dst           , src           , stride, 8);\
        score +=name8(s, dst+8         , src+8         , stride, 8);\
    }\
    return score;\
}

#endif
