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

#define MB()

static inline ogg_int32_t MULT32(ogg_int32_t x, ogg_int32_t y) {

  asm volatile ("mac.l %[x], %[y], %%acc0;"    /* multiply & shift  */
                "movclr.l %%acc0, %[x];"       /* move & clear acc */
                "asr.l #1, %[x];"              /* no overflow test */
                : [x] "+&d" (x)
                : [y] "r" (y)
                : "cc");
  return x;
}

static inline ogg_int32_t MULT31(ogg_int32_t x, ogg_int32_t y) {

  asm volatile ("mac.l %[x], %[y], %%acc0;" /* multiply */
                "movclr.l %%acc0, %[x];"    /* move and clear */
                : [x] "+&r" (x)
                : [y] "r" (y)
                : "cc");
  return x;
}


static inline ogg_int32_t MULT31_SHIFT15(ogg_int32_t x, ogg_int32_t y) {
  ogg_int32_t r;

  asm volatile ("mac.l %[x], %[y], %%acc0;"  /* multiply */
                "mulu.l %[y], %[x];"         /* get lower half, avoid emac stall */
                "movclr.l %%acc0, %[r];"     /* get higher half */
                "swap %[r];"                 /* hi<<16, plus one free */
                "lsr.l #8, %[x];"            /* (unsigned)lo >> 15 */
                "lsr.l #7, %[x];"
                "move.w %[x], %[r];"         /* logical-or results */
                : [r] "=&d" (r), [x] "+d" (x)
                : [y] "d" (y)
                : "cc");
  return r;
}

#ifndef _V_VECT_OPS
#define _V_VECT_OPS

/* asm versions of vector operations for block.c, window.c */
/* assumes MAC is initialized & accumulators cleared */
static inline 
void vect_add_right_left(ogg_int32_t *x, const ogg_int32_t *y, int n)
{
  /* align to 16 bytes */
  while(n>0 && (int)x&15) {
    *x++ += *y++;
    n--;
  }
  asm volatile ("bra 1f;"
                "0:"                          /* loop start */
                "movem.l (%[x]), %%d0-%%d3;"  /* fetch values */
                "movem.l (%[y]), %%a0-%%a3;"
                /* add */
                "add.l %%a0, %%d0;"
                "add.l %%a1, %%d1;"
                "add.l %%a2, %%d2;"
                "add.l %%a3, %%d3;"
                /* store and advance */
                "movem.l %%d0-%%d3, (%[x]);"  
                "lea.l (4*4, %[x]), %[x];"
                "lea.l (4*4, %[y]), %[y];"
                "subq.l #4, %[n];"     /* done 4 elements */
                "1: cmpi.l #4, %[n];"
                "bge 0b;"
                : [n] "+d" (n), [x] "+a" (x), [y] "+a" (y)
                : : "%d0", "%d1", "%d2", "%d3", "%a0", "%a1", "%a2", "%a3",
                    "cc", "memory");
  /* add final elements */
  while (n>0) {
    *x++ += *y++;
    n--;
  }
}
static inline 
void vect_add_left_right(ogg_int32_t *x, const ogg_int32_t *y, int n)
{
    /* coldfire asm has symmetrical versions of vect_add_right_left
       and vect_add_left_right  (since symmetrical versions of
       vect_mult_fw and vect_mult_bw  i.e.  both use MULT31) */
    vect_add_right_left(x, y, n );
}

static inline 
void vect_copy(ogg_int32_t *x, const ogg_int32_t *y, int n)
{
  /* align to 16 bytes */
  while(n>0 && (int)x&15) {
    *x++ = *y++;
    n--;
  }  
  asm volatile ("bra 1f;"
                "0:"                                    /* loop start */
                "movem.l (%[y]), %%d0-%%d3;"            /* fetch values */
                "movem.l %%d0-%%d3, (%[x]);"            /* store */
                "lea.l (4*4, %[x]), %[x];"              /* advance */
                "lea.l (4*4, %[y]), %[y];"
                "subq.l #4, %[n];"                      /* done 4 elements */
                "1: cmpi.l #4, %[n];"
                "bge 0b;"
                : [n] "+d" (n), [x] "+a" (x), [y] "+a" (y)
                : : "%d0", "%d1", "%d2", "%d3", "cc", "memory");
  /* copy final elements */
  while (n>0) {
    *x++ = *y++;
    n--;
  }
}

static inline 
void vect_mult_fw(ogg_int32_t *data, LOOKUP_T *window, int n)
{
  /* ensure data is aligned to 16-bytes */
  while(n>0 && (int)data&15) {
    *data = MULT31(*data, *window);
    data++;
    window++;
    n--;
  }
  asm volatile ("movem.l (%[d]), %%d0-%%d3;"  /* loop start */
                "movem.l (%[w]), %%a0-%%a3;"  /* pre-fetch registers */
                "lea.l (4*4, %[w]), %[w];"
                "bra 1f;"               /* jump to loop condition */
                "0:" /* loop body */
                /* multiply and load next window values */
                "mac.l %%d0, %%a0, (%[w])+, %%a0, %%acc0;"
                "mac.l %%d1, %%a1, (%[w])+, %%a1, %%acc1;"
                "mac.l %%d2, %%a2, (%[w])+, %%a2, %%acc2;"
                "mac.l %%d3, %%a3, (%[w])+, %%a3, %%acc3;"              
                "movclr.l %%acc0, %%d0;"  /* get the products */
                "movclr.l %%acc1, %%d1;"
                "movclr.l %%acc2, %%d2;"
                "movclr.l %%acc3, %%d3;"
                /* store and advance */
                "movem.l %%d0-%%d3, (%[d]);"  
                "lea.l (4*4, %[d]), %[d];"
                "movem.l (%[d]), %%d0-%%d3;"
                "subq.l #4, %[n];"     /* done 4 elements */
                "1: cmpi.l #4, %[n];"
                "bge 0b;"
                /* multiply final elements */
                "tst.l %[n];"
                "beq 1f;"      /* n=0 */
                "mac.l %%d0, %%a0, %%acc0;"
                "movclr.l %%acc0, %%d0;"
                "move.l %%d0, (%[d])+;"
                "subq.l #1, %[n];"
                "beq 1f;"     /* n=1 */
                "mac.l %%d1, %%a1, %%acc0;"
                "movclr.l %%acc0, %%d1;"
                "move.l %%d1, (%[d])+;"
                "subq.l #1, %[n];"
                "beq 1f;"     /* n=2 */
                /* otherwise n = 3 */
                "mac.l %%d2, %%a2, %%acc0;"
                "movclr.l %%acc0, %%d2;"
                "move.l %%d2, (%[d])+;"
                "1:"
                : [n] "+d" (n), [d] "+a" (data), [w] "+a" (window)
                : : "%d0", "%d1", "%d2", "%d3", "%a0", "%a1", "%a2", "%a3",
                    "cc", "memory");
}

static inline 
void vect_mult_bw(ogg_int32_t *data, LOOKUP_T *window, int n)
{
  /* ensure at least data is aligned to 16-bytes */
  while(n>0 && (int)data&15) {
    *data = MULT31(*data, *window);
    data++;
    window--;
    n--;
  }
  asm volatile ("lea.l (-3*4, %[w]), %[w];"     /* loop start */
                "movem.l (%[d]), %%d0-%%d3;"    /* pre-fetch registers */
                "movem.l (%[w]), %%a0-%%a3;"
                "bra 1f;"               /* jump to loop condition */
                "0:" /* loop body */
                /* multiply and load next window value */
                "mac.l %%d0, %%a3, -(%[w]), %%a3, %%acc0;"
                "mac.l %%d1, %%a2, -(%[w]), %%a2, %%acc1;"
                "mac.l %%d2, %%a1, -(%[w]), %%a1, %%acc2;"
                "mac.l %%d3, %%a0, -(%[w]), %%a0, %%acc3;"              
                "movclr.l %%acc0, %%d0;"  /* get the products */
                "movclr.l %%acc1, %%d1;"
                "movclr.l %%acc2, %%d2;"
                "movclr.l %%acc3, %%d3;"
                /* store and advance */
                "movem.l %%d0-%%d3, (%[d]);"  
                "lea.l (4*4, %[d]), %[d];"
                "movem.l (%[d]), %%d0-%%d3;"
                "subq.l #4, %[n];"     /* done 4 elements */
                "1: cmpi.l #4, %[n];"
                "bge 0b;"
                /* multiply final elements */
                "tst.l %[n];"
                "beq 1f;"      /* n=0 */
                "mac.l %%d0, %%a3, %%acc0;"
                "movclr.l %%acc0, %%d0;"
                "move.l %%d0, (%[d])+;"
                "subq.l #1, %[n];"
                "beq 1f;"     /* n=1 */
                "mac.l %%d1, %%a2, %%acc0;"
                "movclr.l %%acc0, %%d1;"
                "move.l %%d1, (%[d])+;"
                "subq.l #1, %[n];"
                "beq 1f;"     /* n=2 */
                /* otherwise n = 3 */
                "mac.l %%d2, %%a1, %%acc0;"
                "movclr.l %%acc0, %%d2;"
                "move.l %%d2, (%[d])+;"
                "1:"
                : [n] "+d" (n), [d] "+a" (data), [w] "+a" (window)
                : : "%d0", "%d1", "%d2", "%d3", "%a0", "%a1", "%a2", "%a3",
                    "cc", "memory");
}

#endif

#endif
#endif
