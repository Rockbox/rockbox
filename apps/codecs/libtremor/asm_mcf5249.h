/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 by Pedro Vasconcelos
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
/* asm routines for wide math on the MCF5249 */

#include "os_types.h"

#if defined(CPU_COLDFIRE)

#ifndef _V_WIDE_MATH
#define _V_WIDE_MATH

#define INCL_OPTIMIZED_VECTOR_FMUL_WINDOW
static inline void ff_vector_fmul_window_c(ogg_int32_t *dst, const ogg_int32_t *src0,
                                           const ogg_int32_t *src1, const ogg_int32_t *win, int len)
{
    /* len is always a power of 2 and always >= 16 so this is unrolled 4 times*/
    ogg_int32_t *dst0 = dst, *dst1 = dst + 2*len;
    const ogg_int32_t *win0 = win, *win1 = win + 2*len;
    src1 += len;
    asm volatile ("move.l (%[src0])+, %%d0\n\t"
                  "move.l -(%[win1]), %%d3\n\t"
                  "tst.l %[len]\n\t"
                  "bra.s 1f\n\t"
                  "0:\n\t"
                  "mac.l %%d0, %%d3, (%[win0])+, %%d2, %%acc0\n\t"
                  "mac.l %%d0, %%d2, -(%[src1]), %%d1, %%acc1\n\t"
                  "msac.l %%d1, %%d2, (%[src0])+, %%d0, %%acc0\n\t"
                  "mac.l %%d1, %%d3, -(%[win1]), %%d3, %%acc1\n\t"
                  "mac.l %%d0, %%d3, (%[win0])+, %%d2, %%acc2\n\t"
                  "mac.l %%d0, %%d2, -(%[src1]), %%d1, %%acc3\n\t"
                  "msac.l %%d1, %%d2, (%[src0])+, %%d0, %%acc2\n\t"
                  "mac.l %%d1, %%d3, -(%[win1]), %%d3, %%acc3\n\t"
                  "movclr.l %%acc0, %%d1\n\t"
                  "movclr.l %%acc2, %%d2\n\t"
                  "subq.l #8, %[dst1]\n\t"
                  "movclr.l %%acc1, %%d5\n\t"
                  "movclr.l %%acc3, %%d4\n\t"
                  "movem.l %%d4-%%d5, (%[dst1])\n\t"
                  "mac.l %%d0, %%d3, (%[win0])+, %%d5, %%acc0\n\t"
                  "mac.l %%d0, %%d5, -(%[src1]), %%d4, %%acc1\n\t"
                  "msac.l %%d4, %%d5, (%[src0])+, %%d0, %%acc0\n\t"
                  "mac.l %%d4, %%d3, -(%[win1]), %%d3, %%acc1\n\t"
                  "mac.l %%d0, %%d3, (%[win0])+, %%d5, %%acc2\n\t"
                  "mac.l %%d0, %%d5, -(%[src1]), %%d4, %%acc3\n\t"
                  "msac.l %%d4, %%d5, (%[src0])+, %%d0, %%acc2\n\t" /* will read one past end of src0 */
                  "mac.l %%d4, %%d3, -(%[win1]), %%d3, %%acc3\n\t"  /* will read one into win0 */
                  "movclr.l %%acc0, %%d4\n\t"
                  "movclr.l %%acc2, %%d5\n\t"
                  "movem.l %%d1-%%d2/%%d4-%%d5, (%[dst0])\n\t"
                  "lea (16, %[dst0]), %[dst0]\n\t"
                  "subq.l #8, %[dst1]\n\t"
                  "movclr.l %%acc1, %%d2\n\t"
                  "movclr.l %%acc3, %%d1\n\t"
                  "movem.l %%d1-%%d2, (%[dst1])\n\t"
                  "subq.l #4, %[len]\n\t"
                  "1:\n\t"
                  "bgt.s 0b\n\t"
                  : [dst0] "+a" (dst0), [dst1] "+a" (dst1),
                    [src0] "+a" (src0), [src1] "+a" (src1),
                    [win0] "+a" (win0), [win1] "+a" (win1),
                    [len] "+d" (len)
                  :: "d0", "d1", "d2", "d3", "d4", "d5", "cc", "memory" );
}

#define MB()

#endif
#endif
