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
#endif /* CPU_COLDIFRE */
