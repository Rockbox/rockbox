/****************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Michael Giacomelli
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

#include "wmadec.h"
#include "wmafixed.h"
#include <codecs.h>

fixed64 IntTo64(int x){
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

int IntFrom64(fixed64 x)
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

fixed32 Fixed32From64(fixed64 x)
{
  return x & 0xFFFFFFFF;
}

fixed64 Fixed32To64(fixed32 x)
{
  return (fixed64)x;
}


/*
 * Helper functions for wma_window.
 *
 *
 */

#ifdef CPU_ARM
inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *data,
                         const fixed32 *window, int n)
{
    /* Block sizes are always power of two */
    asm volatile (
        "0:"
        "ldmia %[d]!, {r0, r1};"
        "ldmia %[w]!, {r4, r5};"
        /* consume the first data and window value so we can use those
         * registers again */
        "smull r8, r9, r0, r4;"
        "ldmia %[dst], {r0, r4};"
        "add   r0, r0, r9, lsl #1;"  /* *dst=*dst+(r9<<1)*/
        "smull r8, r9, r1, r5;"
        "add   r1, r4, r9, lsl #1;"
        "stmia %[dst]!, {r0, r1};"
        "subs  %[n], %[n], #2;"
        "bne   0b;"
        : [d] "+r" (data), [w] "+r" (window), [dst] "+r" (dst), [n] "+r" (n)
        : : "r0", "r1", "r4", "r5", "r8", "r9", "memory", "cc");
}

inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1,
                         int len)
{
    /* Block sizes are always power of two */
    asm volatile (
        "add   %[s1], %[s1], %[n], lsl #2;"
        "0:"
        "ldmia %[s0]!, {r0, r1};"
        "ldmdb %[s1]!, {r4, r5};"
        "smull r8, r9, r0, r5;"
        "mov   r0, r9, lsl #1;"
        "smull r8, r9, r1, r4;"
        "mov   r1, r9, lsl #1;"
        "stmia %[dst]!, {r0, r1};"
        "subs  %[n], %[n], #2;"
        "bne   0b;"
        : [s0] "+r" (src0), [s1] "+r" (src1), [dst] "+r" (dst), [n] "+r" (len)
        : : "r0", "r1", "r4", "r5", "r8", "r9", "memory", "cc");
}

#elif defined(CPU_COLDFIRE)

inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *data,
                         const fixed32 *window, int n)
{
    /* Block sizes are always power of two. Smallest block is always way bigger
     * than four too.*/
    asm volatile (
        "0:"
        "movem.l (%[d]), %%d0-%%d3;"
        "movem.l (%[w]), %%d4-%%d5/%%a0-%%a1;"
        "mac.l %%d0, %%d4, %%acc0;"
        "mac.l %%d1, %%d5, %%acc1;"
        "mac.l %%d2, %%a0, %%acc2;"
        "mac.l %%d3, %%a1, %%acc3;"
        "lea.l (16, %[d]), %[d];"
        "lea.l (16, %[w]), %[w];"
        "movclr.l %%acc0, %%d0;"
        "movclr.l %%acc1, %%d1;"
        "movclr.l %%acc2, %%d2;"
        "movclr.l %%acc3, %%d3;"
        "movem.l (%[dst]), %%d4-%%d5/%%a0-%%a1;"
        "add.l %%d4, %%d0;"
        "add.l %%d5, %%d1;"
        "add.l %%a0, %%d2;"
        "add.l %%a1, %%d3;"
        "movem.l %%d0-%%d3, (%[dst]);"
        "lea.l (16, %[dst]), %[dst];"
        "subq.l #4, %[n];"
        "jne 0b;"
        : [d] "+a" (data), [w] "+a" (window), [dst] "+a" (dst), [n] "+d" (n)
        : : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "memory", "cc");
}

inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1,
                         int len)
{
    /* Block sizes are always power of two. Smallest block is always way bigger
     * than four too.*/
    asm volatile (
        "lea.l (-16, %[s1], %[n]*4), %[s1];"
        "0:"
        "movem.l (%[s0]), %%d0-%%d3;"
        "movem.l (%[s1]), %%d4-%%d5/%%a0-%%a1;"
        "mac.l %%d0, %%a1, %%acc0;"
        "mac.l %%d1, %%a0, %%acc1;"
        "mac.l %%d2, %%d5, %%acc2;"
        "mac.l %%d3, %%d4, %%acc3;"
        "lea.l (16, %[s0]), %[s0];"
        "lea.l (-16, %[s1]), %[s1];"
        "movclr.l %%acc0, %%d0;"
        "movclr.l %%acc1, %%d1;"
        "movclr.l %%acc2, %%d2;"
        "movclr.l %%acc3, %%d3;"
        "movem.l %%d0-%%d3, (%[dst]);"
        "lea.l (16, %[dst]), %[dst];"
        "subq.l #4, %[n];"
        "jne 0b;"
        : [s0] "+a" (src0), [s1] "+a" (src1), [dst] "+a" (dst), [n] "+d" (len)
        : : "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1", "memory", "cc");
}

#else

inline void vector_fmul_add_add(fixed32 *dst, const fixed32 *src0, const fixed32 *src1, int len){
    int i;
    for(i=0; i<len; i++)
        dst[i] = fixmul32b(src0[i], src1[i]) + dst[i];
}

inline void vector_fmul_reverse(fixed32 *dst, const fixed32 *src0, const fixed32 *src1, int len){
    int i;
    src1 += len-1;
    for(i=0; i<len; i++)
        dst[i] = fixmul32b(src0[i], src1[-i]);
}

#endif

/*
    Not performance senstitive code here

*/

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

fixed64 fixdiv64(fixed64 x, fixed64 y)
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

 fixed32 fixsqrt32(fixed32 x)
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
