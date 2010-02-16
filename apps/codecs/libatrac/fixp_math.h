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
    #define fixmul16(X,Y) \
     ({ \
        int32_t low; \
        int32_t high; \
        asm volatile (                   /* calculates: result = (X*Y)>>16 */ \
           "smull  %0,%1,%2,%3 \n\t"     /* 64 = 32x32 multiply */ \
           "mov %0, %0, lsr #16 \n\t"    /* %0 = %0 >> 16 */ \
           "orr %0, %0, %1, lsl #16 \n\t"/* result = %0 OR (%1 << 16) */ \
           : "=&r"(low), "=&r" (high) \
           : "r"(X),"r"(Y)); \
        low; \
     })
     
    #define fixmul31(X,Y) \
     ({ \
        int32_t low; \
        int32_t high; \
        asm volatile (                   /* calculates: result = (X*Y)>>31 */ \
           "smull  %0,%1,%2,%3 \n\t"     /* 64 = 32x32 multiply */ \
           "mov %0, %0, lsr #31 \n\t"    /* %0 = %0 >> 31 */ \
           "orr %0, %0, %1, lsl #1 \n\t" /* result = %0 OR (%1 << 1) */ \
           : "=&r"(low), "=&r" (high) \
           : "r"(X),"r"(Y)); \
        low; \
     })
#elif defined(CPU_COLDFIRE)
    #define fixmul16(X,Y) \
    ({ \
        int32_t t1, t2; \
        asm volatile ( \
            "mac.l   %[x],%[y],%%acc0\n\t" /* multiply */ \
            "mulu.l  %[y],%[x]   \n\t"     /* get lower half, avoid emac stall */ \
            "movclr.l %%acc0,%[t1]   \n\t" /* get higher half */ \
            "moveq.l #15,%[t2]   \n\t" \
            "asl.l   %[t2],%[t1] \n\t"     /* hi <<= 15, plus one free */ \
            "moveq.l #16,%[t2]   \n\t" \
            "lsr.l   %[t2],%[x]  \n\t"     /* (unsigned)lo >>= 16 */ \
            "or.l    %[x],%[t1]  \n\t"     /* combine result */ \
            : /* outputs */ \
            [t1]"=&d"(t1), \
            [t2]"=&d"(t2) \
            : /* inputs */ \
            [x] "d" ((X)), \
            [y] "d" ((Y))); \
        t1; \
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
