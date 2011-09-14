/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Nils Wallménius
 *
 * Coldfire v2 optimisations for ffmpeg's fft (used in fft-ffmpeg.c)
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

#ifdef CPU_COLDFIRE
#define FFT_FFMPEG_INCL_OPTIMISED_FFT4
static inline void fft4(FFTComplex * z)
{
    asm volatile ("movem.l (%[z]), %%d0-%%d7\n\t"
                  "move.l  %%d0, %%a0\n\t"
                  "add.l   %%d2, %%d0\n\t" /* d0 == t1 */
                  "neg.l   %%d2\n\t"
                  "add.l   %%a0, %%d2\n\t" /* d2 == t3, a0 free */
                  "move.l  %%d6, %%a0\n\t"
                  "sub.l   %%d4, %%d6\n\t" /* d6 == t8 */
                  "add.l   %%d4, %%a0\n\t" /* a0 == t6 */

                  "move.l  %%d0, %%d4\n\t"
                  "sub.l   %%a0, %%d4\n\t" /* z[2].re done */
                  "add.l   %%a0, %%d0\n\t" /* z[0].re done, a0 free */

                  "move.l  %%d5, %%a0\n\t"
                  "sub.l   %%d7, %%d5\n\t" /* d5 == t7 */
                  "add.l   %%d7, %%a0\n\t" /* a0 == t5 */

                  "move.l  %%d1, %%d7\n\t"
                  "sub.l   %%d3, %%d7\n\t" /* d7 == t4 */
                  "add.l   %%d3, %%d1\n\t" /* d1 == t2 */

                  "move.l  %%d7, %%d3\n\t"
                  "sub.l   %%d6, %%d7\n\t" /* z[3].im done */
                  "add.l   %%d6, %%d3\n\t" /* z[1].im done */

                  "move.l  %%d2, %%d6\n\t"
                  "sub.l   %%d5, %%d6\n\t" /* z[3].re done */
                  "add.l   %%d5, %%d2\n\t" /* z[1].re done */

                  "move.l  %%d1, %%d5\n\t"
                  "sub.l   %%a0, %%d5\n\t" /* z[2].im done */
                  "add.l   %%a0, %%d1\n\t" /* z[0].im done */

                  "movem.l %%d0-%%d7, (%[z])\n\t"
                  : :[z] "a" (z)
                  : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
                    "a0", "cc", "memory");
                  
}

#define FFT_FFMPEG_INCL_OPTIMISED_FFT8
static inline void fft8(FFTComplex *z)
{
    asm volatile ("movem.l (4*8, %[z]), %%d0-%%d7\n\t"
                  "move.l  %%d0, %%a1\n\t"
                  "add.l   %%d2, %%a1\n\t" /* a1 == t1 */
                  "sub.l   %%d2, %%d0\n\t" /* d0 == z[5].re */
                  
                  "move.l  %%d1, %%a2\n\t"
                  "add.l   %%d3, %%a2\n\t" /* a2 == t2 */
                  "sub.l   %%d3, %%d1\n\t" /* d1 == z[5].im */
                  
                  "move.l  %%d4, %%d2\n\t"
                  "add.l   %%d6, %%d2\n\t" /* d2 == t3 */
                  "sub.l   %%d6, %%d4\n\t" /* d4 == z[7].re */
                  
                  "move.l  %%d5, %%d3\n\t"
                  "add.l   %%d7, %%d3\n\t" /* d3 == t4 */
                  "sub.l   %%d7, %%d5\n\t" /* d5 == z[7].im */
                  
                  "move.l  %%d2, %%a4\n\t"
                  "sub.l   %%a1, %%a4\n\t" /* a4 == t8 */
                  "add.l   %%d2, %%a1\n\t" /* a1 == t1, d2 free */
                  
                  "move.l  %%a2, %%a3\n\t"
                  "sub.l   %%d3, %%a3\n\t" /* a3 == t7 */
                  "add.l   %%d3, %%a2\n\t" /* a2 == t2, d3 free */
                  
                  /* emac block from TRANSFORM_EQUAL, do this now
                     so we don't need to store and load z[5] and z[7] */
                  "move.l  %[_cPI2_8], %%d2\n\t"
                  "mac.l   %%d2, %%d0, %%acc0\n\t"
                  "mac.l   %%d2, %%d1, %%acc1\n\t"
                  "mac.l   %%d2, %%d4, %%acc2\n\t"
                  "mac.l   %%d2, %%d5, %%acc3\n\t"
                  
                  /* fft4, clobbers all d regs and a0 */
                  "movem.l (%[z]), %%d0-%%d7\n\t"
                  "move.l  %%d0, %%a0\n\t"
                  "add.l   %%d2, %%d0\n\t" /* d0 == t1 */
                  "neg.l   %%d2\n\t"
                  "add.l   %%a0, %%d2\n\t" /* d2 == t3, a0 free */
                  "move.l  %%d6, %%a0\n\t"
                  "sub.l   %%d4, %%d6\n\t" /* d6 == t8 */
                  "add.l   %%d4, %%a0\n\t" /* a0 == t6 */

                  "move.l  %%d0, %%d4\n\t"
                  "sub.l   %%a0, %%d4\n\t" /* z[2].re done */
                  "add.l   %%a0, %%d0\n\t" /* z[0].re done, a0 free */

                  "move.l  %%d5, %%a0\n\t"
                  "sub.l   %%d7, %%d5\n\t" /* d5 == t7 */
                  "add.l   %%d7, %%a0\n\t" /* a0 == t5 */

                  "move.l  %%d1, %%d7\n\t"
                  "sub.l   %%d3, %%d7\n\t" /* d7 == t4 */
                  "add.l   %%d3, %%d1\n\t" /* d1 == t2 */

                  "move.l  %%d7, %%d3\n\t"
                  "sub.l   %%d6, %%d7\n\t" /* z[3].im done */
                  "add.l   %%d6, %%d3\n\t" /* z[1].im done */

                  "move.l  %%d2, %%d6\n\t"
                  "sub.l   %%d5, %%d6\n\t" /* z[3].re done */
                  "add.l   %%d5, %%d2\n\t" /* z[1].re done */

                  "move.l  %%d1, %%d5\n\t"
                  "sub.l   %%a0, %%d5\n\t" /* z[2].im done */
                  "add.l   %%a0, %%d1\n\t" /* z[0].im done */
                  /* end of fft4, but don't store yet */

                  "move.l  %%d0, %%a0\n\t"
                  "add.l   %%a1, %%d0\n\t"
                  "sub.l   %%a1, %%a0\n\t" /* z[4].re, z[0].re done, a1 free */

                  "move.l  %%d1, %%a1\n\t"
                  "add.l   %%a2, %%d1\n\t"
                  "sub.l   %%a2, %%a1\n\t" /* z[4].im, z[0].im done, a2 free */

                  "move.l  %%d4, %%a2\n\t"
                  "add.l   %%a3, %%d4\n\t"
                  "sub.l   %%a3, %%a2\n\t" /* z[6].re, z[2].re done, a3 free */

                  "move.l  %%d5, %%a3\n\t"
                  "add.l   %%a4, %%d5\n\t"
                  "sub.l   %%a4, %%a3\n\t" /* z[6].im, z[2].im done, a4 free */

                  "movem.l %%d0-%%d1, (%[z])\n\t"      /* save z[0] */
                  "movem.l %%d4-%%d5, (2*8, %[z])\n\t" /* save z[2] */
                  "movem.l %%a0-%%a1, (4*8, %[z])\n\t" /* save z[4] */
                  "movem.l %%a2-%%a3, (6*8, %[z])\n\t" /* save z[6] */

                  /* TRANSFORM_EQUAL */
                  "movclr.l %%acc0, %%d0\n\t"
                  "movclr.l %%acc1, %%d1\n\t"
                  "movclr.l %%acc2, %%d4\n\t"
                  "movclr.l %%acc3, %%d5\n\t"

                  "move.l  %%d1, %%a0\n\t"
                  "add.l   %%d0, %%a0\n\t" /* a0 == t1 */
                  "sub.l   %%d0, %%d1\n\t" /* d1 == t2 */

                  "move.l  %%d4, %%d0\n\t"
                  "add.l   %%d5, %%d0\n\t" /* d0 == t6 */
                  "sub.l   %%d5, %%d4\n\t" /* d4 == t5 */

                  "move.l  %%d4, %%a1\n\t"
                  "sub.l   %%a0, %%a1\n\t" /* a1 == temp1 */
                  "add.l   %%a0, %%d4\n\t" /* d4 == temp2 */

                  "move.l  %%d2, %%a2\n\t"
                  "sub.l   %%d4, %%a2\n\t" /* a2 == z[5].re */
                  "add.l   %%d4, %%d2\n\t" /* z[1].re done */

                  "move.l  %%d7, %%d5\n\t"
                  "sub.l   %%a1, %%d5\n\t" /* d5 == z[7].im */
                  "add.l   %%a1, %%d7\n\t" /* z[3].im done */

                  "move.l  %%d1, %%a0\n\t"
                  "sub.l   %%d0, %%a0\n\t" /* a0 == temp1 */
                  "add.l   %%d0, %%d1\n\t" /* d1 == temp2 */

                  "move.l  %%d6, %%d4\n\t"
                  "sub.l   %%a0, %%d4\n\t" /* d4 == z[7].re */
                  "add.l   %%a0, %%d6\n\t" /* z[3].re done */

                  "move.l  %%d3, %%a3\n\t"
                  "sub.l   %%d1, %%a3\n\t" /* a3 == z[5].im */
                  "add.l   %%d1, %%d3\n\t" /* z[1].im done */

                  "movem.l %%d2-%%d3, (1*8, %[z])\n\t" /* save z[1] */
                  "movem.l %%d6-%%d7, (3*8, %[z])\n\t" /* save z[3] */
                  "movem.l %%a2-%%a3, (5*8, %[z])\n\t" /* save z[5] */
                  "movem.l %%d4-%%d5, (7*8, %[z])\n\t" /* save z[7] */
                  : :[z] "a" (z), [_cPI2_8] "i" (cPI2_8)
                  : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
                    "a0", "a1", "a2", "a3", "a4", "cc", "memory");
}

#define FFT_FFMPEG_INCL_OPTIMISED_TRANSFORM

static inline FFTComplex* TRANSFORM(FFTComplex * z, unsigned int n, FFTSample wre, FFTSample wim)
{
    asm volatile ("move.l   (%[z2]),   %%d5\n\t"
                  "mac.l    %%d5,      %[wre], (4, %[z2]), %%d4, %%acc0\n\t"
                  "mac.l    %%d4,      %[wim], %%acc0\n\t"
                  "mac.l    %%d4,      %[wre], (%[z3]), %%d6, %%acc1\n\t"
                  "msac.l   %%d5,      %[wim], (4, %[z3]), %%d7, %%acc1\n\t"
                  "mac.l    %%d6,      %[wre], (%[z])+, %%d4, %%acc2\n\t"
                  "msac.l   %%d7,      %[wim], (%[z])+, %%d5, %%acc2\n\t"
                  "mac.l    %%d7,      %[wre], %%acc3\n\t"
                  "mac.l    %%d6,      %[wim], %%acc3\n\t"

                  "movclr.l %%acc0,    %[wre]\n\t"     /* t1 */
                  "movclr.l %%acc2,    %[wim]\n\t"     /* t5 */

                  "move.l   %%d4,      %%d6\n\t"
                  "move.l   %[wim],    %%d7\n\t"
                  "sub.l    %[wre],    %[wim]\n\t"     /* t5 = t5-t1 */
                  "add.l    %[wre],    %%d7\n\t"
                  "sub.l    %%d7,      %%d6\n\t"       /* d6 = a0re - (t5+t1) => a2re */
                  "add.l    %%d7,      %%d4\n\t"       /* d4 = a0re + (t5+t1) => a0re */

                  "movclr.l %%acc3,    %%d7\n\t"       /* t6 */
                  "movclr.l %%acc1,    %%d3\n\t"       /* t2 */
                  
                  "move.l   %%d3,      %[wre]\n\t"
                  "add.l    %%d7,      %[wre]\n\t"
                  "sub.l    %%d7,      %%d3\n\t"       /* t2 = t6-t2 */
                  "move.l   %%d5,      %%d7\n\t"
                  "sub.l    %[wre],    %%d7\n\t"       /* d7 = a0im - (t2+t6) => a2im */

                  "movem.l  %%d6-%%d7, (%[z2])\n\t"    /* store z2 */
                  "add.l    %[wre],    %%d5\n\t"       /* d5 = a0im + (t2+t6) => a0im */
                  "movem.l  %%d4-%%d5, (-8, %[z])\n\t"     /* store z0 */

                  "movem.l  (%[z1]),   %%d4-%%d5\n\t"  /* load z1 */
                  "move.l   %%d4,      %%d6\n\t"

                  "sub.l    %%d3,      %%d6\n\t"       /* d6 = a1re - (t2-t6) => a3re */
                  "add.l    %%d3,      %%d4\n\t"       /* d4 = a1re + (t2-t6) => a1re */

                  "move.l   %%d5,      %%d7\n\t"
                  "sub.l    %[wim],    %%d7\n\t"
                  "movem.l  %%d6-%%d7, (%[z3])\n\t"    /* store z3 */
                  "add.l    %[wim],    %%d5\n\t"
                  "movem.l  %%d4-%%d5, (%[z1])\n\t"    /* store z1 */

                  : [wre] "+r" (wre), [wim] "+r" (wim), /* we clobber these after using them */
                    [z] "+a" (z)
                  : [z1] "a" (&z[n]), [z2] "a" (&z[2*n]), [z3] "a" (&z[3*n])
                  : "d3", "d4", "d5", "d6", "d7", "cc", "memory");
    return z;
}

static inline FFTComplex* TRANSFORM_W01(FFTComplex * z, unsigned int n, const FFTSample * w)
{
    return TRANSFORM(z, n, w[0], w[1]);
}

static inline FFTComplex* TRANSFORM_W10(FFTComplex * z, unsigned int n, const FFTSample * w)
{
    return TRANSFORM(z, n, w[1], w[0]);
}

static inline FFTComplex* TRANSFORM_ZERO(FFTComplex * z, unsigned int n)
{
    asm volatile("movem.l (%[z]), %%d4-%%d5\n\t" /* load z0 */
                 "move.l  %%d4, %%d6\n\t"
                 "movem.l (%[z2]), %%d2-%%d3\n\t" /* load z2 */
                 "movem.l (%[z3]), %%d0-%%d1\n\t" /* load z0 */
                 "move.l  %%d0, %%d7\n\t"
                 "sub.l   %%d2, %%d0\n\t"
                 "add.l   %%d2, %%d7\n\t"
                 "sub.l   %%d7, %%d6\n\t" /* d6 = a0re - (t5+t1) => a2re */
                 "add.l   %%d7, %%d4\n\t" /* d4 = a0re + (t5+t1) => a0re */

                 "move.l  %%d5, %%d7\n\t"
                 "move.l  %%d3, %%d2\n\t"
                 "add.l   %%d1, %%d2\n\t"
                 "sub.l   %%d2, %%d7\n\t" /* d7 = a0im - (t2+t6) => a2im */
                 "movem.l %%d6-%%d7, (%[z2])\n\t" /* store z2 */
                 "add.l   %%d2, %%d5\n\t" /* d5 = a0im + (t2+t6) => a0im */
                 "movem.l %%d4-%%d5, (%[z])\n\t" /* store z0 */

                 "movem.l (%[z1]), %%d4-%%d5\n\t" /* load z1 */
                 "move.l  %%d4, %%d6\n\t"
                 "sub.l   %%d1, %%d3\n\t"
                 "sub.l   %%d3, %%d6\n\t" /* d6 = a1re - (t2-t6) => a3re */
                 "add.l   %%d3, %%d4\n\t" /* d4 = a1re + (t2-t6) => a1re */

                 "move.l  %%d5, %%d7\n\t"
                 "sub.l   %%d0, %%d7\n\t"
                 "movem.l %%d6-%%d7, (%[z3])\n\t" /* store z3 */
                 "add.l   %%d0, %%d5\n\t"

                 "movem.l %%d4-%%d5, (%[z1])\n\t" /* store z1 */

                 :
                 : [z] "a" (z), [z1] "a" (&z[n]), [z2] "a" (&z[2*n]), [z3] "a" (&z[3*n])
                 : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
    return z+1;
}

static inline FFTComplex* TRANSFORM_EQUAL(FFTComplex * z, unsigned int n)
{
    asm volatile ("movem.l  (%[z2]),    %%d0-%%d1\n\t"
                  "move.l   %[_cPI2_8], %%d2\n\t"
                  "mac.l    %%d0,       %%d2, (%[z3]),    %%d0, %%acc0\n\t"
                  "mac.l    %%d1,       %%d2, (4, %[z3]), %%d1, %%acc1\n\t"
                  "mac.l    %%d0,       %%d2, (%[z]),     %%d4, %%acc2\n\t"
                  "mac.l    %%d1,       %%d2, (4, %[z]),  %%d5, %%acc3\n\t"

                  "movclr.l %%acc0,    %%d0\n\t"
                  "movclr.l %%acc1,    %%d1\n\t"
                  "movclr.l %%acc2,    %%d2\n\t"
                  "movclr.l %%acc3,    %%d3\n\t"
                  
                  "move.l   %%d0, %%d7\n\t"
                  "add.l    %%d1, %%d0\n\t"            /* d0 == t1 */
                  "sub.l    %%d7, %%d1\n\t"            /* d1 == t2 */

                  "move.l   %%d3, %%d7\n\t"
                  "add.l    %%d2, %%d3\n\t"            /* d3 == t6 */
                  "sub.l    %%d7, %%d2\n\t"            /* d2 == t5 */

                  "move.l   %%d4,      %%d6\n\t"
                  "move.l   %%d2,      %%d7\n\t"
                  "sub.l    %%d0,      %%d2\n\t"       /* t5 = t5-t1 */
                  "add.l    %%d0,      %%d7\n\t"
                  "sub.l    %%d7,      %%d6\n\t"       /* d6 = a0re - (t5+t1) => a2re */
                  "add.l    %%d7,      %%d4\n\t"       /* d4 = a0re + (t5+t1) => a0re */

                  "move.l   %%d1,      %%d0\n\t"
                  "add.l    %%d3,      %%d0\n\t"
                  "sub.l    %%d3,      %%d1\n\t"       /* t2 = t6-t2 */
                  "move.l   %%d5,      %%d7\n\t"
                  "sub.l    %%d0,    %%d7\n\t"         /* d7 = a0im - (t2+t6) => a2im */

                  "movem.l  %%d6-%%d7, (%[z2])\n\t"    /* store z2 */
                  "add.l    %%d0,    %%d5\n\t"         /* d5 = a0im + (t2+t6) => a0im */
                  "movem.l  %%d4-%%d5, (%[z])\n\t"     /* store z0 */

                  "movem.l  (%[z1]),   %%d4-%%d5\n\t"  /* load z1 */
                  "move.l   %%d4,      %%d6\n\t"

                  "sub.l    %%d1,      %%d6\n\t"       /* d6 = a1re - (t2-t6) => a3re */
                  "add.l    %%d1,      %%d4\n\t"       /* d4 = a1re + (t2-t6) => a1re */

                  "move.l   %%d5,      %%d7\n\t"
                  "sub.l    %%d2,    %%d7\n\t"
                  "movem.l  %%d6-%%d7, (%[z3])\n\t"    /* store z3 */
                  "add.l    %%d2,    %%d5\n\t"
                  "movem.l  %%d4-%%d5, (%[z1])\n\t"    /* store z1 */

                  :: [z] "a" (z), [z1] "a" (&z[n]), [z2] "a" (&z[2*n]), [z3] "a" (&z[3*n]),
                    [_cPI2_8] "i" (cPI2_8)
                  : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");

    return z+1;
}

#endif /* CPU_COLDIFRE */
