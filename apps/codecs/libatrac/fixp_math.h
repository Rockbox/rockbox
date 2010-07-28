/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Mohamed Tarek
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
#include <stdlib.h>
#include <inttypes.h>

/* Macros for converting between various fixed-point representations and floating point. */
#define ONE_16 (1L << 16)
#define fixtof64(x)       (float)((float)(x) / (float)(1 << 16))        //does not work on int64_t!
#define ftofix32(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5 : 0.5)))
#define ftofix31(x)       ((int32_t)((x) * (float)(1 << 31) + ((x) < 0 ? -0.5 : 0.5)))
#define fix31tof64(x)     (float)((float)(x) / (float)(1 << 31))

/* Fixed point math routines for use in atrac3.c */

#if defined(CPU_ARM)
    /* Calculates: result = (X*Y)>>16 */
    #define fixmul16(X,Y) \
     ({ \
        int32_t lo; \
        int32_t hi; \
        asm volatile ( \
           "smull %[lo], %[hi], %[x], %[y] \n\t" /* multiply */ \
           "mov   %[lo], %[lo], lsr #16    \n\t" /* lo >>= 16 */ \
           "orr   %[lo], %[lo], %[hi], lsl #16"  /* lo |= (hi << 16) */ \
           : [lo]"=&r"(lo), [hi]"=&r"(hi) \
           : [x]"r"(X), [y]"r"(Y)); \
        lo; \
     })
     
    /* Calculates: result = (X*Y)>>31 */
    /* Use scratch register r12 */
    #define fixmul31(X,Y) \
     ({ \
        int32_t lo; \
        int32_t hi; \
        asm volatile ( \
           "smull %[lo], %[hi], %[x], %[y] \n\t" /* multiply */ \
           "mov   %[lo], %[lo], lsr #31    \n\t" /* lo >>= 31 */ \
           "orr   %[lo], %[lo], %[hi], lsl #1"   /* lo |= (hi << 1) */ \
           : [lo]"=&r"(lo), [hi]"=&r"(hi) \
           : [x]"r"(X), [y]"r"(Y)); \
        lo; \
     })
#elif defined(CPU_COLDFIRE)
    /* Calculates: result = (X*Y)>>16 */
    #define fixmul16(X,Y) \
    ({ \
        int32_t t, x = (X); \
        asm volatile ( \
            "mac.l    %[x],%[y],%%acc0\n\t" /* multiply */ \
            "mulu.l   %[y],%[x]       \n\t" /* get lower half, avoid emac stall */ \
            "movclr.l %%acc0,%[t]     \n\t" /* get higher half */ \
            "lsr.l    #1,%[t]         \n\t" /* hi >>= 1 to compensate emac shift */ \
            "move.w   %[t],%[x]       \n\t" /* combine halfwords */\
            "swap     %[x]            \n\t" \
            : [t]"=&d"(t), [x] "+d" (x) \
            : [y] "d" ((Y))); \
        x; \
    })

    #define fixmul31(X,Y) \
    ({ \
       int32_t t; \
       asm volatile ( \
          "mac.l %[x], %[y], %%acc0\n\t"   /* multiply */ \
          "movclr.l %%acc0, %[t]\n\t"      /* get higher half as result */ \
          : [t] "=d" (t) \
          : [x] "r" ((X)), [y] "r" ((Y))); \
       t; \
    })
#else
    static inline int32_t fixmul16(int32_t x, int32_t y)
    {
        int64_t temp;
        temp = x;
        temp *= y;
    
        temp >>= 16;
    
        return (int32_t)temp;
    }
    
    static inline int32_t fixmul31(int32_t x, int32_t y)
    {
        int64_t temp;
        temp = x;
        temp *= y;
    
        temp >>= 31;        //16+31-16 = 31 bits
    
        return (int32_t)temp;
    }
#endif
