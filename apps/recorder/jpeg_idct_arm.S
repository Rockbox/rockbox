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
    .global jpeg_idct1h
    .type   jpeg_idct1h, %function
    .global jpeg_idct2v
    .type   jpeg_idct2v, %function
    .global jpeg_idct2h
    .type   jpeg_idct2h, %function
    .global jpeg_idct4v
    .type   jpeg_idct4v, %function
    .global jpeg_idct4h
    .type   jpeg_idct4h, %function
    .global jpeg_idct8v
    .type   jpeg_idct8v, %function
    .global jpeg_idct8h
    .type   jpeg_idct8h, %function

jpeg_idct1h:
/* In the common case of one pass through the loop, the extra add should be
   cheaper than saving registers to stack and loading a the value 4112. */
1:
    ldrsh  r12, [r0]
    add    r12, r12, #4096
    add    r12, r12, #16
#if ARM_ARCH < 6
    mov    r12, r12, asr #5
    cmp    r12, #255
    mvnhi  r12, r12, asr #31
#else
    usat   r12, #8,  r12, asr #5
#endif
    strb   r12, [r1]
    add    r0,  r0,  #16
    add    r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    bx     lr
    .size jpeg_idct1h, .-jpeg_idct1h

jpeg_idct2v:
#if ARM_ARCH < 6
/* Use SWAR tricks to fake partitioned add and subtract. This is slightly faster
   than loading two values in each register and using shifts and strh, and
   requires fewer fixup operations than splitting the values, calculating, and
   merging.
*/
    stmdb  sp!, { r4, lr }
1:
    ldr    r2,  [r0]
    ldr    r3,  [r0, #16]
    eor    r12, r2,  r3
    and    r12, r12, #0x8000
    bic    r3,  r3,  #0x8000
    bic    r4,  r2,  #0x8000
    add    r4,  r4,  r3
    eor    r4,  r4,  r12
    orr    r2,  r2,  #0x8000
    sub    r2,  r2,  r3
    eor    r2,  r2,  r12
    eor    r2,  r2,  #0x8000
    str    r4,  [r0]
    str    r2,  [r0, #16]
    add    r0,  r0,  #4
    cmp    r0,  r1
    bcc    1b
    ldmia  sp!, { r4, pc }
#else
/* ARMv6 offers partitioned adds and subtracts, used here to unroll the loop
   to two columns.
*/
1:
    ldr    r2,  [r0]
    ldr    r3,  [r0, #16]
    sadd16 r12, r2,  r3
    ssub16 r2,  r2,  r3
    str    r12, [r0]
    str    r2,  [r0, #16]
    add    r0,  r0,  #4
    cmp    r0,  r1
    bcc    1b
    bx     lr
#endif
    .size jpeg_idct2v, .-jpeg_idct2v

jpeg_idct2h:
#if ARM_ARCH < 6
/* Using LDR and shifts here would costs two more ops, and is no faster as
   results can not be stored merged.
*/
    stmdb  sp!, { r4-r5, lr }
#if ARM_ARCH < 5
    ldr    r14, =4112
#else
    ldrsh  r14, .Lpool4+2
#endif
1:
    ldrsh  r12, [r0]
    ldrsh  r4,  [r0, #2]
    add    r12, r12, r14
    add    r5,  r12, r4
    sub    r4,  r12, r4
    mov    r5,  r5,  asr #5
    mov    r4,  r4,  asr #5
    cmp    r5,  #255
    mvnhi  r5,  r5,  asr #31
    cmp    r4,  #255
    mvnhi  r4,  r4,  asr #31
#ifdef HAVE_LCD_COLOR
    strb   r5,  [r1]
    strb   r4,  [r1, #4]
#else
    strb   r5,  [r1]
    strb   r4,  [r1, #1]
#endif
    add    r0,  r0,  #16
    add    r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    ldmia  sp!, { r4-r5, pc }
#else
    stmdb  sp!, { r4, lr }
    ldrsh  r14, .Lpool4+2
1:
    ldr    r12, [r0]
    sadd16 r12, r12, r14
    saddsubx    r12, r12, r12
    usat   r4,  #8,  r12, asr #21
    sxth   r12, r12
    usat   r12, #8,  r12, asr #5
#ifdef HAVE_LCD_COLOR
    strb   r4,  [r1]
    strb   r12, [r1, #4]
#else
    strb   r4,  [r1]
    strb   r12, [r1, #1]
#endif
    add    r0,  r0,  #16
    add    r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    ldmia  sp!, { r4, pc }
#endif
    .size jpeg_idct2h, .-jpeg_idct2h

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
    cmp    r0,  r1
    bcc    1b
    ldmia  sp!, { r4-r7, pc }
#elif ARM_ARCH < 6
    stmdb sp!, { r4-r8, lr }
    mov    r8,  #1024
    ldrd   r4,  .Lpool4
1:
    ldrsh  r14, [r0, #48]
    ldrsh  r3,  [r0, #16]
    ldrsh  r12, [r0, #32]
    ldrsh  r2,  [r0]
    add    r6,  r3,  r14           /* r6  = z1 = d1 + d3 */
    add    r7,  r2,  r12           /* r7  = tmp10 >> 2 = d0 + d2 */
    smlabb r6,  r5,  r6,  r8       /* z1 *= 4433 */
    sub    r2,  r2,  r12           /* r2  = tmp12 >> 2= d0 - d2 */
    smlatb r3,  r5,  r3,  r6       /* r3  = tmp2 = z1 + z2 * 6270 */ 
    smlabb r14, r4,  r14, r6       /* r14 = tmp0 = z1 - z3 * 15137 */
    mov    r7,  r7,  lsl #2
    mov    r2,  r2,  lsl #2
    add    r12, r7,  r3,  asr #11   /* r12 = o0 */
    sub    r7,  r7,  r3,  asr #11   /* r7  = o3 */
    add    r3,  r2,  r14, asr #11   /* r3  = o1 */
    sub    r2,  r2,  r14, asr #11   /* r2  = o2 */
    strh   r12, [r0]
    strh   r7,  [r0, #48]
    strh   r3,  [r0, #16]
    strh   r2,  [r0, #32]
    add    r0,  r0,  #2
    cmp    r0,  r1
    bcc    1b
    ldmia sp!, { r4-r8, pc }
#else
    stmdb  sp!, { r4-r10, lr }
    ldrd   r2,  .Lpool4
    mov    r12, #1024
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
    smlabb r9,  r3,  r6,  r12      /* r9 = z1[0] = (d1 * d3) * 4433 + 1024 */
    smlabt r6,  r3,  r6,  r12      /* r6 = z1[1] = (d1 * d3) * 4433 + 1024 */
    smlatb r10, r3,  r5,  r9       /* r10 = tmp2[0] = z1 + d1 * 6270 */
    smlabb r14, r2,  r7,  r9       /* r14 = tmp0[0] = z1 - d3 * 15137 */
    smlatt r5,  r3,  r5,  r6       /* r5  = tmp2[1] */
    smlabt r6,  r2,  r7,  r6       /* r6  = tmp0[1] */
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

#if ARM_ARCH > 4
    .align 4
.Lpool4:
    .short -15137
    .short 4112
    .short 4433
    .short 6270

    .align 2
#endif

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
    cmp    r0,  r2
    bcc    1b
    ldmia sp!, { r4-r10, pc }
#elif ARM_ARCH < 6 || 1
    stmdb  sp!, { r4-r9, lr }
    ldrd   r4,  .Lpool4
1:
    ldrsh  r7,  [r0, #6]
    ldrsh  r14, [r0, #2]
    ldrsh  r12, [r0]
    ldrsh  r6,  [r0, #4]
    add    r8,  r14, r7             /* r8 = z1 = d1 + d3 */
    add    r12, r12, r4,  lsr #16
    smulbb r8,  r5,  r8             /* z1 *= 4433 */
    add    r9,  r12, r6             /* r9 = tmp10 >> 13 = d0 + d2 */
    smlatb r14, r5,  r14, r8        /* r14= tmp2 = z1 + z2 * 6270 */ 
    smlabb r7,  r4,  r7,  r8        /* r7 = tmp0 = z1 - z3 * 15137 */
    sub    r12, r12, r6             /* r12= tmp12 >> 13 = d0 - d2 */
    add    r6,  r14, r9,  lsl #13   /* r6 = o0 */
    rsb    r9,  r14, r9,  lsl #13   /* r9 = o3 */
    add    r14, r7,  r12, lsl #13   /* r14= o1 */
    rsb    r12, r7,  r12, lsl #13   /* r12= o2 */
    mov    r6,  r6,  asr #18
    mov    r14, r14, asr #18
    mov    r12, r12, asr #18
    mov    r9,  r9,  asr #18
    cmp    r6,  #255
    mvnhi  r6,  r6,  asr #31
    cmp    r14, #255
    mvnhi  r14, r14, asr #31
    cmp    r12, #255
    mvnhi  r12, r12, asr #31
    cmp    r9,  #255
    mvnhi  r9,  r9,  asr #31
#ifdef HAVE_LCD_COLOR
    strb   r6,  [r1]
    strb   r14, [r1, #4]
    strb   r12, [r1, #8]
    strb   r9,  [r1, #12]
#else
    strb   r6,  [r1]
    strb   r14, [r1, #1]
    strb   r12, [r1, #2]
    strb   r9,  [r1, #3]
#endif
    add    r0,  r0,  #16
    add    r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    ldmia sp!, { r4-r9, pc }
#else
    stmdb sp!, { r4-r9, lr }
    ldrd   r4,  .Lpool4
    mov    r9,  r4,  lsr #16
1:
    ldmia  r0,  { r12, r14 }
    sadd16 r12, r12, r9
    sadd16 r6,  r12, r14            /* r6lo = d0 + d2, r6hi = d1 + d3 */
    ssub16 r7,  r12, r14            /* r7lo = d0 - d2 */
    smulbt r8,  r5,  r6
    sxth   r6,  r6
    smlatt r12, r5,  r12, r8        /* r12= tmp2 = z1 + z2 * 6270 */
    smlabt r14, r4,  r14, r8        /* r14= tmp0 = z1 - z3 * 15137 */
    sxth   r7,  r7
    add    r8,  r12, r6,  lsl #13   /* r8 = o0 */
    rsb    r6,  r12, r6,  lsl #13   /* r6 = o3 */
    add    r12, r14, r7,  lsl #13   /* r12= o1 */
    rsb    r14, r14, r7,  lsl #13   /* r14= o2 */
    usat  r8,  #8,  r8,  asr #18
    usat  r6,  #8,  r6,  asr #18
    usat  r12, #8,  r12, asr #18
    usat  r14, #8,  r14, asr #18
#ifdef HAVE_LCD_COLOR
    strb   r8,  [r1]
    strb   r6,  [r1, #12]
    strb   r12, [r1, #4]
    strb   r14, [r1, #8]
#else
    strb   r8,  [r1]
    strb   r6,  [r1, #3]
    strb   r12, [r1, #1]
    strb   r14, [r1, #2]
#endif
    add   r0,  r0,  #16
    add   r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    ldmia sp!, { r4-r9, pc }
#endif
    .size jpeg_idct4h, .-jpeg_idct4h

jpeg_idct8v:
    stmdb  sp!, { r4-r11, lr }
    add    r2,  r0,  #128
1:
    ldmia  r0!, { r4-r7 }
    mov    r8,  r4,  lsl #16
    orrs   r9,  r6,  r7
    orreqs r9,  r5,  r4, lsr #16
    bne    2f
    mov    r8,  r8,  asr #14
    strh   r8,  [r2]
    strh   r8,  [r2, #16]
    strh   r8,  [r2, #32]
    strh   r8,  [r2, #48]
    strh   r8,  [r2, #64]
    strh   r8,  [r2, #80]
    strh   r8,  [r2, #96]
    strh   r8,  [r2, #112]
    cmp    r0,  r1
    add    r2,  r2,  #2
    bcc    1b
    ldmia  sp!, { r4-r11, pc }
2:
    ldr    r14, =4433
    ldr    r12, =-15137
    mov    r10, r5,  lsl #16
    mov    r11, r7,  lsl #16
    mov    r10, r10, asr #16        /* r10 = z2 = d2 */
    mov    r11, r11, asr #16        /* r11 = z3 = d6 */
    add    r8,  r8,  #8192
    add    r9,  r10, r11
    mov    r8,  r8,  asr #3         /* r8  = z4 = (d0 << 13) + 1024 */
    mul    r9,  r14, r9             /* r9  = z1 = (z2 + z3) * 4433 */
    ldr    r14, =6270
    mla    r11, r12, r11, r9         /* r11 = tmp2 = z1 - z3 * 15137 */
    mla    r10, r14, r10, r9         /* r10 = tmp3 = z1 + z2 * 6270 */
    mov    r9,  r6,  lsl #16         /* r9  = z5 << 3 = d4 << 16 */
    add    r12, r8,  r9,  asr #3     /* r12 = tmp0 = z4 + z5 */
    sub    r14, r8,  r9,  asr #3     /* r14 = tmp1 = z4 - z5 */
    add    r8,  r12, r10             /* r8  = tmp10 = tmp0 + tmp3 */
    sub    r9,  r12, r10             /* r9  = tmp13 = tmp0 - tmp3 */
    add    r10, r14, r11             /* r10 = tmp11 = tmp1 + tmp2 */
    sub    r11, r14, r11             /* r11 = tmp12 = tmp1 - tmp2 */
    stmdb  sp,  { r8-r11 }           /* tmp10 tmp13 tmp11 tmp12 */
    mov    r4,  r4,  asr #16         /* r4  = tmp3 = d1 */
    mov    r5,  r5,  asr #16         /* r5  = tmp2 = d3 */
    mov    r6,  r6,  asr #16         /* r6  = tmp1 = d5 */
    mov    r7,  r7,  asr #16         /* r7  = tmp0 = d7 */
    ldr    r10, =9633
    ldr    r11, =-16069
    add    r12, r5,  r7              /* r12 = z3 = tmp0 + tmp2 */
    add    r14, r4,  r6              /* r14 = z4 = tmp1 + tmp3 */
    add    r9,  r12, r14             /* r9  = z3 + z4 */
    mul    r9,  r10, r9              /* r9  = z5 = (z3 + z4) * 9633 */
    ldr    r10, =-3196
    mla    r12, r11, r12, r9         /* r12 = z3 = z5 - z3 * 16069 */
    ldr    r11, =-7373
    mla    r14, r10, r14, r9         /* r14 = z4 = z5 - z4 * 3196 */
    ldr    r10, =2446
    add    r9,  r4,  r7              /* r9  = tmp0 + tmp3 */
    mla    r8,  r11, r9,  r12        /* r8  = z1 + z3 */
    mla    r9,  r11, r9,  r14        /* r9  = z1 + z4 */
    ldr    r11, =12299
    mla    r7,  r10, r7,  r8         /* r7  = tmp0 = z1 + z3 + tmp0 * 2446 */
    ldr    r10, =-20995
    mla    r4,  r11, r4,  r9         /* r4  = tmp3 = z1 + z4 + tmp0 * 12299 */
    ldr    r11, =25172
    add    r9,  r5,  r6              /* r9  = tmp1 + tmp2 */
    mla    r12, r10, r9, r12         /* r12 = z2 + z3 */
    mla    r14, r10, r9, r14         /* r14 = z2 + z4 */
    ldr    r10, =16819
    mla    r5,  r11, r5, r12         /* r5  = tmp2 = z2 + z3 + tmp2 * 25172 */
    mla    r6,  r10, r6, r14         /* r6  = tmp1 = z2 + z4 + tmp1 * 16819 */
    ldmdb  sp,  { r8-r11 }
    add    r12, r8,  r4              /* o0 */
    sub    r14, r8,  r4              /* o7 */
    add    r8,  r9,  r7              /* o3 */
    sub    r9,  r9,  r7              /* o4 */
    add    r4,  r10, r5              /* O1 */
    sub    r5,  r10, r5              /* o6 */
    add    r10, r11, r6              /* o2 */
    sub    r11, r11, r6              /* o5 */
    /* output in order: r12 r4  r10 r8  r9  r11 r5  r14 */
    mov    r12, r12, asr #11
    mov    r4,  r4,  asr #11
    mov    r10, r10, asr #11
    mov    r8,  r8,  asr #11
    mov    r9,  r9,  asr #11
    mov    r11, r11, asr #11
    mov    r5,  r5,  asr #11
    mov    r14, r14, asr #11
    strh   r12, [r2]
    strh   r4,  [r2, #16]
    strh   r10, [r2, #32]
    strh   r8,  [r2, #48]
    strh   r9,  [r2, #64]
    strh   r11, [r2, #80]
    strh   r5,  [r2, #96]
    strh   r14, [r2, #112]
    cmp    r0,  r1
    add    r2,  r2,  #2
    bcc    1b
    ldmia  sp!, { r4-r11, pc }
    .size jpeg_idct8v, .-jpeg_idct8v

jpeg_idct8h:
    stmdb  sp!, { r4-r11, lr }
1:
    ldmia  r0!, { r4-r7 }
    ldr    r14, =4112
    mov    r8,  r4,  lsl #16
    add    r8,  r8,  r14, lsl #16
    orrs   r9,  r6,  r7
    orreqs r9,  r5,  r4, lsr #16
    bne    2f
    mov    r8,  r8,  asr #21
    cmp    r8,  #255
    mvnhi  r8,  r8,  asr #31
#ifdef HAVE_LCD_COLOR
    strb   r8,  [r1]
    strb   r8,  [r1, #4]
    strb   r8,  [r1, #8]
    strb   r8,  [r1, #12]
    strb   r8,  [r1, #16]
    strb   r8,  [r1, #20]
    strb   r8,  [r1, #24]
    strb   r8,  [r1, #28]
#else
    strb   r8,  [r1]
    strb   r8,  [r1, #1]
    strb   r8,  [r1, #2]
    strb   r8,  [r1, #3]
    strb   r8,  [r1, #4]
    strb   r8,  [r1, #5]
    strb   r8,  [r1, #6]
    strb   r8,  [r1, #7]
#endif
    add    r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    ldmia  sp!, { r4-r11, pc }
2:
    ldr    r14, =4433
    ldr    r12, =-15137
    mov    r10, r5,  lsl #16
    mov    r11, r7,  lsl #16
    mov    r10, r10, asr #16        /* r10 = z2 = d2 */
    mov    r11, r11, asr #16        /* r11 = z3 = d6 */
    add    r9,  r10, r11
    mov    r8,  r8,  asr #3         /* r8  = z4 = (d0 + 4112) << 13 */
    mul    r9,  r14, r9             /* r9  = z1 = (z2 + z3) * 4433 */
    ldr    r14, =6270
    mla    r11, r12, r11, r9         /* r11 = tmp2 = z1 - z3 * 15137 */
    mla    r10, r14, r10, r9         /* r10 = tmp3 = z1 + z2 * 6270 */
    mov    r9,  r6,  lsl #16         /* r9  = z5 << 3 = d4 << 16 */
    add    r12, r8,  r9,  asr #3     /* r12 = tmp0 = z4 + z5 */
    sub    r14, r8,  r9,  asr #3     /* r14 = tmp1 = z4 - z5 */
    add    r8,  r12, r10             /* r8  = tmp10 = tmp0 + tmp3 */
    sub    r9,  r12, r10             /* r9  = tmp13 = tmp0 - tmp3 */
    add    r10, r14, r11             /* r10 = tmp11 = tmp1 + tmp2 */
    sub    r11, r14, r11             /* r11 = tmp12 = tmp1 - tmp2 */
    stmdb  sp,  { r8-r11 }           /* tmp10 tmp13 tmp11 tmp12 */
    mov    r4,  r4,  asr #16         /* r4  = tmp3 = d1 */
    mov    r5,  r5,  asr #16         /* r5  = tmp2 = d3 */
    mov    r6,  r6,  asr #16         /* r6  = tmp1 = d5 */
    mov    r7,  r7,  asr #16         /* r7  = tmp0 = d7 */
    ldr    r10, =9633
    ldr    r11, =-16069
    add    r12, r5,  r7              /* r12 = z3 = tmp0 + tmp2 */
    add    r14, r4,  r6              /* r14 = z4 = tmp1 + tmp3 */
    add    r9,  r12, r14             /* r9  = z3 + z4 */
    mul    r9,  r10, r9              /* r9  = z5 = (z3 + z4) * 9633 */
    ldr    r10, =-3196
    mla    r12, r11, r12, r9         /* r12 = z3 = z5 - z3 * 16069 */
    ldr    r11, =-7373
    mla    r14, r10, r14, r9         /* r14 = z4 = z5 - z4 * 3196 */
    ldr    r10, =2446
    add    r9,  r4,  r7              /* r9  = tmp0 + tmp3 */
    mla    r8,  r11, r9,  r12        /* r8  = z1 + z3 */
    mla    r9,  r11, r9,  r14        /* r9  = z1 + z4 */
    ldr    r11, =12299
    mla    r7,  r10, r7,  r8         /* r7  = tmp0 = z1 + z3 + tmp0 * 2446 */
    ldr    r10, =-20995
    mla    r4,  r11, r4,  r9         /* r4  = tmp3 = z1 + z4 + tmp0 * 12299 */
    ldr    r11, =25172
    add    r9,  r5,  r6              /* r9  = tmp1 + tmp2 */
    mla    r12, r10, r9, r12         /* r12 = z2 + z3 */
    mla    r14, r10, r9, r14         /* r14 = z2 + z4 */
    ldr    r10, =16819
    mla    r5,  r11, r5, r12         /* r5  = tmp2 = z2 + z3 + tmp2 * 25172 */
    mla    r6,  r10, r6, r14         /* r6  = tmp1 = z2 + z4 + tmp1 * 16819 */
    ldmdb  sp,  { r8-r11 }
    add    r12, r8,  r4              /* o0 */
    sub    r14, r8,  r4              /* o7 */
    add    r8,  r9,  r7              /* o3 */
    sub    r9,  r9,  r7              /* o4 */
    add    r4,  r10, r5              /* O1 */
    sub    r5,  r10, r5              /* o6 */
    add    r10, r11, r6              /* o2 */
    sub    r11, r11, r6              /* o5 */
    /* output in order: r12 r4  r10 r8  r9  r11 r5  r14 */
#if ARM_ARCH < 6
    mov    r12, r12, asr #18
    cmp    r12, #255
    mvnhi  r12, r12, asr #31
    mov    r4,  r4,  asr #18
    cmp    r4,  #255
    mvnhi  r4,  r4,  asr #31
    mov    r10, r10, asr #18
    cmp    r10, #255
    mvnhi  r10, r10, asr #31
    mov    r8,  r8,  asr #18
    cmp    r8,  #255
    mvnhi  r8,  r8,  asr #31
    mov    r9,  r9,  asr #18
    cmp    r9,  #255
    mvnhi  r9,  r9,  asr #31
    mov    r11, r11, asr #18
    cmp    r11, #255
    mvnhi  r11, r11, asr #31
    mov    r5,  r5,  asr #18
    cmp    r5,  #255
    mvnhi  r5,  r5,  asr #31
    mov    r14, r14, asr #18
    cmp    r14, #255
    mvnhi  r14, r14, asr #31
#else
    usat   r12, #8,  r12, asr #18
    usat   r4,  #8,  r4,  asr #18
    usat   r10, #8,  r10, asr #18
    usat   r8,  #8,  r8,  asr #18
    usat   r9,  #8,  r9,  asr #18
    usat   r11, #8,  r11, asr #18
    usat   r5,  #8,  r5,  asr #18
    usat   r14, #8,  r14, asr #18
#endif
#ifdef HAVE_LCD_COLOR
    strb   r12, [r1]
    strb   r4,  [r1, #4]
    strb   r10, [r1, #8]
    strb   r8,  [r1, #12]
    strb   r9,  [r1, #16]
    strb   r11, [r1, #20]
    strb   r5,  [r1, #24]
    strb   r14, [r1, #28]
#else
    strb   r12, [r1]
    strb   r4,  [r1, #1]
    strb   r10, [r1, #2]
    strb   r8,  [r1, #3]
    strb   r9,  [r1, #4]
    strb   r11, [r1, #5]
    strb   r5,  [r1, #6]
    strb   r14, [r1, #7]
#endif
    add    r1,  r1,  r3
    cmp    r0,  r2
    bcc    1b
    ldmia  sp!, { r4-r11, pc }
    .size jpeg_idct8h, .-jpeg_idct8h
