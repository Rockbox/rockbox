/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG assembly IDCT
*
* Copyright (C) 2009 Andrew Mahone asm versions of the C IDCT algorithms used
*                                  jpeg_load.c with 
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
#include "config.h"

    .section .text
    .align   2
    .global jpeg_idct4v
    .type   jpeg_idct4v, %function
    .global jpeg_idct4h
    .type   jpeg_idct4h, %function

jpeg_idct4v:
#if ARM_ARCH < 5
    stmdb  sp!, { r4-r7, lr }
    ldr    r14, =-15137
    ldr    r12, =6270
1:
    ldrsh  r4,  [r0, #32]
    ldrsh  r2,  [r0]
    ldrsh  r5,  [r0, #48]
    ldrsh  r3,  [r0, #16]
    add    r6,  r2,  r4            /* r6 = tmp10 >> 2 = d0 + d2 */
    sub    r2,  r2,  r4            /* r2 = tmp12 >> 2= d0 - d2 */
    add    r4,  r3,  r5            /* r4 = z1 = d1 + d3 */
    add    r7,  r4,  r4,  lsl #3
    rsb    r4,  r4,  r7,  lsl #4
    rsb    r4,  r4,  r4,  lsl #5   /* z1 *= 4433 */
    add    r4,  r4,  #1024
    mla    r3,  r12, r3,  r4       /* r3 = tmp2 = z1 + z2 * 6270 */ 
    mla    r5,  r14, r5,  r4       /* r5 = tmp0 = z1 - z3 * 15137 */
    mov    r6,  r6,  lsl #2        /* r6 <<= 2 */
    mov    r2,  r2,  lsl #2        /* r2 <<= 2 */
    add    r7,  r6,  r3,  asr #11   /* r7 = o0 */
    sub    r3,  r6,  r3,  asr #11   /* r3 = o3 */
    add    r6,  r2,  r5,  asr #11   /* r6 = o1 */
    sub    r2,  r2,  r5,  asr #11   /* r2 = o2 */
    strh   r7,  [r0]
    strh   r3,  [r0, #48]
    strh   r6,  [r0, #16]
    strh   r2,  [r0, #32]
    add    r0,  r0,  #2
    teq    r0,  r1
    bne    1b
    ldmia  sp!, { r4-r7, pc }
#elif ARM_ARCH < 6
    stmdb sp!, { r4-r8, lr }
    ldr    r8,  =1024
    ldr    r14, =4433
    ldr    r12, =3302955134
1:
    ldrsh  r5,  [r0, #48]
    ldrsh  r3,  [r0, #16]
    ldrsh  r4,  [r0, #32]
    ldrsh  r2,  [r0]
    add    r6,  r3,  r5            /* r6 = z1 = d1 + d3 */
    add    r7,  r2,  r4            /* r7 = tmp10 >> 2 = d0 + d2 */
    smlabb r6, r14, r6,  r8        /* z1 *= 4433 */
    sub    r2,  r2,  r4            /* r2 = tmp12 >> 2= d0 - d2 */
    smlabb r3,  r12, r3,  r6       /* r3 = tmp2 = z1 + z2 * 6270 */ 
    smlatb r5,  r12, r5,  r6       /* r5 = tmp0 = z1 - z3 * 15137 */
    mov    r7,  r7,  lsl #2
    mov    r2,  r2,  lsl #2
    add    r4,  r7,  r3,  asr #11   /* r4 = o0 */
    sub    r7,  r7,  r3,  asr #11   /* r7 = o3 */
    add    r3,  r2,  r5,  asr #11   /* r3 = o1 */
    sub    r2,  r2,  r5,  asr #11   /* r2 = o2 */
    strh   r4,  [r0]
    strh   r7,  [r0, #48]
    strh   r3,  [r0, #16]
    strh   r2,  [r0, #32]
    add    r0,  r0,  #2
    teq    r0,  r1
    bne    1b
    ldmia sp!, { r4-r8, pc }
#else
    stmdb  sp!, { r4-r10, lr }
    ldr    r2,  =1024
    ldr    r3,  =4433
    ldr    r12, =3302955134
1:
    ldr    r6,  [r0, #32]
    ldr    r4,  [r0]
    ldr    r7,  [r0, #48]
    ldr    r5,  [r0, #16]
    /* this part is being done in parallel on two columns */
    sadd16 r8,  r4,  r6            /* r8 = d0 + d2 */
    ssub16 r4,  r4,  r6            /* r4 = d0 - d2 */
    sadd16 r6,  r5,  r7            /* r6 = d1 + d3 */
    /* there is no parallel shift operation, but we can fake it with bic
       and lsl */
    bic    r8,  r8,  #0xc000
    bic    r4,  r4,  #0xc000
    /* multiplication expands values beyond 16 bits, so this part needs to be
       split. the values will be merged below so that the rest of the addition
       can be done in parallel */
    smlabb r9,  r3,  r6,  r2       /* r9 = z1[0] = (d1 * d3) * 4433 + 1024 */
    smlabt r6,  r3,  r6,  r2       /* r6 = z1[1] = (d1 * d3) * 4433 + 1024 */
    smlabb r10, r12, r5,  r9       /* r10 = tmp2[0] = z1 + d1 * 6270 */
    smlatb r14, r12, r7,  r9       /* r14 = tmp0[0] = z1 - d3 * 15137 */
    smlabt r5,  r12, r5,  r6       /* r5  = tmp2[1] */
    smlatt r6,  r12, r7,  r6       /* r6  = tmp0[1] */
    mov    r8,  r8,  lsl #2        /* complete the parallel shift started */
    mov    r4,  r4,  lsl #2        /* with the earlier bic instructions */
    /* tmp2 are in r10, r5; tmp0 are in r14, r6 */
    /* tmp10, tmp12 are in r4, r8 */
    mov    r10, r10, asr #11
    mov    r14, r14, asr #11
    pkhbt  r5,  r10, r5,  lsl #5 /* parallel tmp2 */ 
    pkhbt  r6,  r14, r6,  lsl #5 /* parallel tmp0 */
    sadd16 r10, r8,  r5 /* d0 */
    ssub16 r5,  r8,  r5 /* d3 */
    sadd16 r14, r4,  r6 /* d1 */
    ssub16 r6,  r4,  r6 /* d2 */
    str    r10, [r0]
    str    r5,  [r0, #48]
    str    r14, [r0, #16]
    str    r6,  [r0, #32]
    add    r0,  r0,  #4
    cmp    r0,  r1
    bcc    1b
    ldmia  sp!, { r4-r10, pc }
#endif
    .size jpeg_idct4v, .-jpeg_idct4v

jpeg_idct4h:
#if ARM_ARCH < 5
    stmdb  sp!, { r4-r10, lr }
    ldr    r10, =-15137
    ldr    r14, =4112
    ldr    r12, =6270
1:
    ldrsh  r4,  [r0]
    ldrsh  r6,  [r0, #4]
    ldrsh  r7,  [r0, #6]
    ldrsh  r5,  [r0, #2]
    add    r4,  r4,  r14
    add    r8,  r4,  r6            /* r8 = tmp10 >> 2 = d0 + d2 */
    sub    r4,  r4,  r6            /* r4 = tmp12 >> 2= d0 - d2 */
    add    r6,  r5,  r7            /* r6 = z1 = d1 + d3 */
    add    r9,  r6,  r6,  lsl #3
    rsb    r6,  r6,  r9,  lsl #4
    rsb    r6,  r6,  r6,  lsl #5   /* z1 *= 4433 */
    mla    r7,  r10, r7,  r6       /* r5 = tmp0 = z1 - z3 * 15137 */
    mla    r5,  r12, r5,  r6       /* r3 = tmp2 = z1 + z2 * 6270 */ 
    add    r9,  r5,  r8,  lsl #13  /* r7 = o0 */
    rsb    r5,  r5,  r8,  lsl #13  /* r3 = o3 */
    add    r8,  r7,  r4,  lsl #13  /* r6 = o1 */
    rsb    r4,  r7,  r4,  lsl #13  /* r2 = o2 */
    mov    r9,  r9,  asr #18
    mov    r8,  r8,  asr #18
    mov    r4,  r4,  asr #18
    mov    r5,  r5,  asr #18
    cmp    r9,  #255
    mvnhi  r9,  r9,  asr #31
    cmp    r8,  #255
    mvnhi  r8,  r8,  asr #31
    cmp    r4,  #255
    mvnhi  r4,  r4,  asr #31
    cmp    r5,  #255
    mvnhi  r5,  r5,  asr #31
#ifdef HAVE_LCD_COLOR
    strb   r9,  [r1]
    strb   r8,  [r1, #4]
    strb   r4,  [r1, #8]
    strb   r5,  [r1, #12]
#else
    strb   r9,  [r1]
    strb   r8,  [r1, #1]
    strb   r4,  [r1, #2]
    strb   r5,  [r1, #3]
#endif
    add    r0,  r0,  #16
    add    r1,  r1,  r3
    teq    r0,  r2
    bne    1b
    ldmia sp!, { r4-r10, pc }
#elif ARM_ARCH < 6
    stmdb  sp!, { r4-r10, lr }
    ldr    r10, =4433
    ldr    r14, =4112
    ldr    r12, =3302955134
1:
    ldrsh  r7,  [r0, #6]
    ldrsh  r5,  [r0, #2]
    ldrsh  r4,  [r0]
    ldrsh  r6,  [r0, #4]
    add    r8,  r5,  r7             /* r8 = z1 = d1 + d3 */
    add    r4,  r4,  r14
    smulbb r8, r10, r8              /* z1 *= 4433 */
    add    r9,  r4,  r6             /* r9 = tmp10 >> 13 = d0 + d2 */
    smlabb r5,  r12, r5,  r8        /* r5 = tmp2 = z1 + z2 * 6270 */ 
    smlatb r7,  r12, r7,  r8        /* r7 = tmp0 = z1 - z3 * 15137 */
    sub    r4,  r5,  r6             /* r4 = tmp12 >> 13 = d0 - d2 */
    add    r6,  r5,  r9,  lsl #13   /* r6 = o0 */
    rsb    r9,  r5,  r9,  lsl #13   /* r9 = o3 */
    add    r5,  r7,  r4,  lsl #13   /* r5 = o1 */
    rsb    r4,  r7,  r4,  lsl #13   /* r4 = o2 */
    mov    r6,  r6,  asr #18
    mov    r5,  r5,  asr #18
    mov    r4,  r4,  asr #18
    mov    r9,  r9,  asr #18
    cmp    r6,  #255
    mvnhi  r6,  r6,  asr #31
    cmp    r5,  #255
    mvnhi  r5,  r5,  asr #31
    cmp    r4,  #255
    mvnhi  r4,  r4,  asr #31
    cmp    r9,  #255
    mvnhi  r9,  r9,  asr #31
#ifdef HAVE_LCD_COLOR
    strb   r6,  [r1]
    strb   r5,  [r1, #4]
    strb   r4,  [r1, #8]
    strb   r9,  [r1, #12]
#else
    strb   r6,  [r1]
    strb   r5,  [r1, #1]
    strb   r4,  [r1, #2]
    strb   r9,  [r1, #3]
#endif
    add    r0,  r0,  #16
    add    r1,  r1,  r3
    teq    r0,  r2
    bne    1b
    ldmia sp!, { r4-r10, pc }
#else
    stmdb sp!, { r4-r9, lr }
    ldr    r9,  =4433
    ldr    r14, =4112
    ldr    r12, =3302955134
1:
    ldmia  r0,  { r4-r5 }
    sadd16 r4,  r4,  r14
    sadd16 r6,  r4,  r5             /* r6lo = d0 + d2, r6hi = d1 + d3 */
    ssub16 r7,  r4,  r5             /* r7lo = d0 - d2 */
    smulbt r8,  r9,  r6
    sxth   r6,  r6
    smlabt r4,  r12, r4,  r8        /* r4 = tmp2 = z1 + z2 * 6270 */
    smlatt r5,  r12, r5,  r8        /* r5 = tmp0 = z1 - z3 * 15137 */
    sxth   r7,  r7
    add    r8,  r4,  r6,  lsl #13   /* r8 = o0 */
    rsb    r6,  r4,  r6,  lsl #13   /* r6 = o3 */
    add    r4,  r5,  r7,  lsl #13   /* r4 = o1 */
    rsb    r5,  r5,  r7,  lsl #13   /* r5 = o2 */
    usat  r8,  #8,  r8,  asr #18
    usat  r6,  #8,  r6,  asr #18
    usat  r4,  #8,  r4,  asr #18
    usat  r5,  #8,  r5,  asr #18
#ifdef HAVE_LCD_COLOR
    strb   r8,  [r1]
    strb   r6,  [r1, #12]
    strb   r4,  [r1, #4]
    strb   r5,  [r1, #8]
#else
    strb   r8,  [r1]
    strb   r6,  [r1, #3]
    strb   r4,  [r1, #1]
    strb   r5,  [r1, #2]
#endif
    add   r0,  r0,  #16
    add   r1,  r1,  r3
    teq   r0,  r2
    bne   1b
    ldmia sp!, { r4-r9, pc }
#endif
    .size jpeg_idct4h, .-jpeg_idct4h
