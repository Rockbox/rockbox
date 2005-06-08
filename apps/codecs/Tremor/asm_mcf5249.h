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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/* asm routines for wide math on the MCF5249 */

#include "os_types.h"

#if CONFIG_CPU == MCF5249 && !defined(SIMULATOR)

#ifndef _V_WIDE_MATH
#define _V_WIDE_MATH

#define MB()

static inline void mcf5249_init_mac(void) {
  int r;
  asm volatile ("move.l #0x20, %%macsr;");  /* frac, truncate, no saturation */
}

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
                "movclr.l %%acc0, %[r];"     /* get higher half */
                "mulu.l %[y], %[x];"         /* get lower half */
                "asl.l #8, %[r];"            /* hi<<16, plus one free */
                "asl.l #8, %[r];"
                "lsr.l #8, %[x];"            /* (unsigned)lo >> 15 */
                "lsr.l #7, %[x];"
                "or.l %[x], %[r];"           /* logical-or results */
                : [r] "=&d" (r), [x] "+d" (x)
                : [y] "d" (y)
                : "cc");
  return r;
}


static inline 
void XPROD31(ogg_int32_t  a, ogg_int32_t  b,   
             ogg_int32_t  t, ogg_int32_t  v,
             ogg_int32_t *x, ogg_int32_t *y)
{ 
  asm volatile ("mac.l %[a], %[t], %%acc0;"
                "mac.l %[b], %[v], %%acc0;"
                "mac.l %[b], %[t], %%acc1;"
                "msac.l %[a], %[v], %%acc1;"
                "movclr.l %%acc0, %[a];"
                "move.l %[a], (%[x]);"
                "movclr.l %%acc1, %[a];"
                "move.l %[a], (%[y]);"
                : [a] "+&r" (a)
                : [x] "a" (x), [y] "a" (y),
                  [b] "r" (b), [t] "r" (t), [v] "r" (v)
                : "cc", "memory");
}


static inline
void XNPROD31(ogg_int32_t  a, ogg_int32_t  b,   
              ogg_int32_t  t, ogg_int32_t  v,
              ogg_int32_t *x, ogg_int32_t *y)
{
  asm volatile ("mac.l %[a], %[t], %%acc0;"
                "msac.l %[b], %[v], %%acc0;"
                "mac.l %[b], %[t], %%acc1;"
                "mac.l %[a], %[v], %%acc1;"
                "movclr.l %%acc0, %[a];"
                "move.l %[a], (%[x]);"
                "movclr.l %%acc1, %[a];"
                "move.l %[a], (%[y]);"
                : [a] "+&r" (a)
                : [x] "a" (x), [y] "a" (y),
                  [b] "r" (b), [t] "r" (t), [v] "r" (v)
                : "cc", "memory");
}




#if 1 /* Canonical definition */
#define XPROD32(_a, _b, _t, _v, _x, _y)         \
  { (_x)=MULT32(_a,_t)+MULT32(_b,_v);           \
    (_y)=MULT32(_b,_t)-MULT32(_a,_v); }
#else
/* Thom Johansen suggestion; this could loose the lsb by overflow
   but does it matter in practice? */
#define XPROD32(_a, _b, _t, _v, _x, _y)     \
  asm volatile ("mac.l %[a], %[t], %%acc0;" \
                "mac.l %[b], %[v], %%acc0;" \
                "mac.l %[b], %[t], %%acc1;" \
                "msac.l %[a], %[v], %%acc1;" \
                "movclr.l %%acc0, %[x];" \
                "asr.l #1, %[x];" \
                "movclr.l %%acc1, %[y];" \
                "asr.l #1, %[y];" \
                : [x] "=&d" (_x), [y] "=&d" (_y) \
                : [a] "r" (_a), [b] "r" (_b), \
                  [t] "r" (_t), [v] "r" (_v) \
                : "cc");
#endif 


/* asm versions of vector multiplication for window.c */
/* assumes MAC is initialized & accumulators cleared */
static inline 
void mcf5249_vect_mult_fw(ogg_int32_t *data, LOOKUP_T *window, int n)
{
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
void mcf5249_vect_mult_bw(ogg_int32_t *data, LOOKUP_T *window, int n)
{
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


static inline 
void mcf5249_vect_zero(ogg_int32_t *ptr, int n)
{
  asm volatile ("clr.l %%d0;"
                "clr.l %%d1;"
                "clr.l %%d2;"
                "clr.l %%d3;"
                /* loop start */
                "tst.l %[n];"
                "bra 1f;"
                "0: movem.l %%d0-%%d3, (%[ptr]);"
                "lea (4*4, %[ptr]), %[ptr];"
                "subq.l #4, %[n];"
                "1: bgt 0b;"
                /* remaing elements */
                "tst.l %[n];"
                "beq 1f;"      /* n=0 */
                "clr.l (%[ptr])+;"
                "subq.l #1, %[n];"
                "beq 1f;"     /* n=1 */
                "clr.l (%[ptr])+;"
                "subq.l #1, %[n];"
                "beq 1f;"     /* n=2 */
                /* otherwise n = 3 */
                "clr.l (%[ptr])+;"
                "1:"
                : [n] "+d" (n), [ptr] "+a" (ptr)
                :
                : "%d0","%d1","%d2","%d3","cc","memory");
}

#endif

#ifndef _V_CLIP_MATH
#define _V_CLIP_MATH

/* this is portable C and simple; why not use this as default? */
static inline ogg_int32_t CLIP_TO_15(register ogg_int32_t x) {
  register ogg_int32_t hi=32767, lo=-32768;
  return (x>=hi ? hi : (x<=lo ? lo : x));
}

#endif
#endif
