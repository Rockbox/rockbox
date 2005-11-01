/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id$
**/

#ifndef __FIXED_H__
#define __FIXED_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32_WCE) && defined(_ARM_)
#include <cmnintrin.h>
#endif

#define COEF_BITS 28
#define COEF_PRECISION (1 << COEF_BITS)
#define REAL_BITS 14 // MAXIMUM OF 14 FOR FIXED POINT SBR
#define REAL_PRECISION (1 << REAL_BITS)

/* FRAC is the fractional only part of the fixed point number [0.0..1.0) */
#define FRAC_SIZE 32 /* frac is a 32 bit integer */
#define FRAC_BITS 31
#define FRAC_PRECISION ((uint32_t)(1 << FRAC_BITS))
#define FRAC_MAX 0x7FFFFFFF

typedef int32_t real_t;


#define REAL_CONST(A) (((A) >= 0) ? ((real_t)((A)*(REAL_PRECISION)+0.5)) : ((real_t)((A)*(REAL_PRECISION)-0.5)))
#define COEF_CONST(A) (((A) >= 0) ? ((real_t)((A)*(COEF_PRECISION)+0.5)) : ((real_t)((A)*(COEF_PRECISION)-0.5)))
#define FRAC_CONST(A) (((A) == 1.00) ? ((real_t)FRAC_MAX) : (((A) >= 0) ? ((real_t)((A)*(FRAC_PRECISION)+0.5)) : ((real_t)((A)*(FRAC_PRECISION)-0.5))))
//#define FRAC_CONST(A) (((A) >= 0) ? ((real_t)((A)*(FRAC_PRECISION)+0.5)) : ((real_t)((A)*(FRAC_PRECISION)-0.5)))

#define Q2_BITS 22
#define Q2_PRECISION (1 << Q2_BITS)
#define Q2_CONST(A) (((A) >= 0) ? ((real_t)((A)*(Q2_PRECISION)+0.5)) : ((real_t)((A)*(Q2_PRECISION)-0.5)))

#if defined(CPU_COLDFIRE) && !defined(SIMULATOR)

static INLINE real_t MUL_F(real_t A, real_t B)
{
    asm volatile (
        "mac.l %[A], %[B], %%acc0\n\t"
        "movclr.l %%acc0, %[A]"
        : [A] "+&r" (A) : [B] "r" (B)
    );
    return A;
}

#if 0
/* this currently provokes an ICE in gcc 3.4.x */
static INLINE real_t MUL_C(real_t A, real_t B)
{
    asm volatile (
        "mac.l %[A], %[B], %%acc0\n\t"
        "movclr.l %%acc0, %[A]\n\t"
        "asl.l #3, %[A]"
        : [A] "+&d" (A) : [B] "r" (B)
    );
    return A;
}
#else
/* keep this one around until the ICE is solved */
#define MUL_C(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (COEF_BITS-1))) >> COEF_BITS)
#endif

/* MUL_R needs too many shifts for us to just operate on the top 32 bits the
   emac unit gives as usual, so we do a full 64 bit mul here. */
static INLINE real_t MUL_R(real_t x, real_t y)
{
    real_t t1, t2;
    asm volatile (
        "mac.l   %[x],%[y],%%acc0\n" /* multiply */
        "mulu.l  %[y],%[x]   \n"     /* get lower half, avoid emac stall */
        "movclr.l %%acc0,%[t1]   \n" /* get higher half */
        "moveq.l #17,%[t2]   \n"
        "asl.l   %[t2],%[t1] \n"     /* hi <<= 17, plus one free */
        "moveq.l #14,%[t2]   \n"
        "lsr.l   %[t2],%[x]  \n"     /* (unsigned)lo >>= 14 */
        "or.l    %[x],%[t1]  \n"     /* combine result */
        : /* outputs */
        [t1]"=&d"(t1),
        [t2]"=&d"(t2),
        [x] "+d" (x)
        : /* inputs */
        [y] "d"  (y)
    );
    return t1;
}

static INLINE void ComplexMult(real_t *y1, real_t *y2,
    real_t x1, real_t x2, real_t c1, real_t c2)
{
    asm volatile(
        "mac.l %[x1], %[c1], %%acc0\n\t"
        "mac.l %[x2], %[c2], %%acc0\n\t"
        "mac.l %[x2], %[c1], %%acc1\n\t"
        "msac.l %[x1], %[c2], %%acc1\n\t"
        "movclr.l %%acc0, %[x1]\n\t"
        "move.l %[x1], (%[y1])\n\t"
        "movclr.l %%acc1, %[x1]\n\t"
        "move.l %[x1], (%[y2])"
        : [x1] "+&r" (x1)
        : [x2] "r" (x2), [y1] "a" (y1), [y2] "a" (y2),
          [c1] "r" (c1), [c2] "r" (c2)
        : "memory"
    );
}

  /* the following see little or no use, so just ignore them for now */
  #define MUL_Q2(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (Q2_BITS-1))) >> Q2_BITS)
  #define MUL_SHIFT6(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (6-1))) >> 6)
  #define MUL_SHIFT23(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (23-1))) >> 23)

#elif defined(__GNUC__) && defined (__arm__)

/* taken from MAD */
#define arm_mul(x, y, SCALEBITS) \
({ \
    uint32_t __hi; \
    uint32_t __lo; \
    uint32_t __result; \
    asm("smull  %0, %1, %3, %4\n\t" \
        "movs   %0, %0, lsr %5\n\t" \
        "adc    %2, %0, %1, lsl %6" \
        : "=&r" (__lo), "=&r" (__hi), "=r" (__result) \
        : "%r" (x), "r" (y), \
        "M" (SCALEBITS), "M" (32 - (SCALEBITS)) \
        : "cc"); \
        __result; \
})

static INLINE real_t MUL_R(real_t A, real_t B)
{
    return arm_mul(A, B, REAL_BITS);
}

static INLINE real_t MUL_C(real_t A, real_t B)
{
    return arm_mul(A, B, COEF_BITS);
}

static INLINE real_t MUL_Q2(real_t A, real_t B)
{
    return arm_mul(A, B, Q2_BITS);
}

static INLINE real_t MUL_SHIFT6(real_t A, real_t B)
{
    return arm_mul(A, B, 6);
}

static INLINE real_t MUL_SHIFT23(real_t A, real_t B)
{
    return arm_mul(A, B, 23);
}

static INLINE real_t _MulHigh(real_t x, real_t y)
{
    uint32_t __lo;
    uint32_t __hi;
    asm("smull\t%0, %1, %2, %3"
        : "=&r"(__lo),"=&r"(__hi)
        : "%r"(x),"r"(y)
        : "cc");
    return __hi;
}

static INLINE real_t MUL_F(real_t A, real_t B)
{
    return _MulHigh(A, B) << (FRAC_SIZE-FRAC_BITS);
}

/* Complex multiplication */
static INLINE void ComplexMult(real_t *y1, real_t *y2,
    real_t x1, real_t x2, real_t c1, real_t c2)
{
    int32_t tmp, yt1, yt2;
    asm("smull %0, %1, %4, %6\n\t"
        "smlal %0, %1, %5, %7\n\t"
        "rsb   %3, %4, #0\n\t"
        "smull %0, %2, %5, %6\n\t"
        "smlal %0, %2, %3, %7"
        : "=&r" (tmp), "=&r" (yt1), "=&r" (yt2), "=r" (x1)
        : "3" (x1), "r" (x2), "r" (c1), "r" (c2)
        : "cc" );
    *y1 = yt1 << (FRAC_SIZE-FRAC_BITS);
    *y2 = yt2 << (FRAC_SIZE-FRAC_BITS);
}

#else

  /* multiply with real shift */
  #define MUL_R(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (REAL_BITS-1))) >> REAL_BITS)
  /* multiply with coef shift */
  #define MUL_C(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (COEF_BITS-1))) >> COEF_BITS)
  /* multiply with fractional shift */
#if defined(_WIN32_WCE) && defined(_ARM_)
  /* eVC for PocketPC has an intrinsic function that returns only the high 32 bits of a 32x32 bit multiply */
  static INLINE real_t MUL_F(real_t A, real_t B)
  {
      return _MulHigh(A,B) << (32-FRAC_BITS);
  }
#else
  #define _MulHigh(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (FRAC_SIZE-1))) >> FRAC_SIZE)
  #define MUL_F(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (FRAC_BITS-1))) >> FRAC_BITS)
#endif
  #define MUL_Q2(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (Q2_BITS-1))) >> Q2_BITS)
  #define MUL_SHIFT6(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (6-1))) >> 6)
  #define MUL_SHIFT23(A,B) (real_t)(((int64_t)(A)*(int64_t)(B)+(1 << (23-1))) >> 23)

/* Complex multiplication */
static INLINE void ComplexMult(real_t *y1, real_t *y2,
    real_t x1, real_t x2, real_t c1, real_t c2)
{
    *y1 = (_MulHigh(x1, c1) + _MulHigh(x2, c2))<<(FRAC_SIZE-FRAC_BITS);
    *y2 = (_MulHigh(x2, c1) - _MulHigh(x1, c2))<<(FRAC_SIZE-FRAC_BITS);
}

#endif



#ifdef __cplusplus
}
#endif
#endif
