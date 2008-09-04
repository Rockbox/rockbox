/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: mdct_arm.S 18084 2008-07-17 17:43:49Z saratoga $
 *
 * Copyright (C) 2007 by Tomasz Malesinski
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
/* Codecs should not normally do this, but we need to check a macro, and
 * codecs.h would confuse the assembler. */

#define cPI3_8 (0x30fbc54d)
#define cPI2_8 (0x5a82799a)
#define cPI1_8 (0x7641af3d)

#ifdef USE_IRAM
    .section    .icode,"ax",%progbits
#else
    .text
#endif
    .align

    .global mdct_butterfly_32
    .global mdct_butterfly_generic_loop

mdct_butterfly_8:
    add     r9,  r5,  r1                @ x4 + x0
    sub     r5,  r5,  r1                @ x4 - x0
    add     r7,  r6,  r2                @ x5 + x1
    sub     r6,  r6,  r2                @ x5 - x1
    add     r8,  r10, r3                @ x6 + x2
    sub     r10, r10, r3                @ x6 - x2
    add     r12, r11, r4                @ x7 + x3
    sub     r11, r11, r4                @ x7 - x3

    add     r1,  r10, r6                @ y0 = (x6 - x2) + (x5 - x1)
    sub     r2,  r11, r5                @ y1 = (x7 - x3) - (x4 - x0)
    sub     r3,  r10, r6                @ y2 = (x6 - x2) - (x5 - x1)
    add     r4,  r11, r5                @ y3 = (x7 - x3) + (x4 - x0)
    sub     r5,  r8,  r9                @ y4 = (x6 + x2) - (x4 + x0)
    sub     r6,  r12, r7                @ y5 = (x7 + x3) - (x5 + x1)
    add     r10, r8,  r9                @ y6 = (x6 + x2) + (x4 + x0)
    add     r11, r12, r7                @ y7 = (x7 + x3) + (x5 + x1)
    stmia   r0, {r1, r2, r3, r4, r5, r6, r10, r11}

    mov     pc, lr

mdct_butterfly_16:
    str     lr, [sp, #-4]!
    add     r1, r0, #8*4

    ldmia   r0, {r2, r3, r4, r5}
    ldmia   r1, {r6, r7, r8, r9}
    add     r6, r6, r2                  @ y8 = x8 + x0
    rsb     r2, r6, r2, asl #1          @ x0 - x8
    add     r7, r7, r3                  @ y9 = x9 + x1
    rsb     r3, r7, r3, asl #1          @ x1 - x9
    add     r8, r8, r4                  @ y10 = x10 + x2
    sub     r11, r8, r4, asl #1         @ x10 - x2
    add     r9, r9, r5                  @ y11 = x11 + x3
    rsb     r10, r9, r5, asl #1         @ x3 - x11

    stmia   r1!, {r6, r7, r8, r9}
    
    add     r2, r2, r3                  @ (x0 - x8) + (x1 - x9)
    rsb     r3, r2, r3, asl #1          @ (x1 - x9) - (x0 - x8)

    ldr     r12, =cPI2_8
    smull   r8, r5, r2, r12
    mov     r5, r5, asl #1
    smull   r8, r6, r3, r12
    mov     r6, r6, asl #1

    stmia   r0!, {r5, r6, r10, r11}

    ldmia   r0, {r2, r3, r4, r5}
    ldmia   r1, {r6, r7, r8, r9}
    add     r6, r6, r2                  @ y12 = x12 + x4
    sub     r2, r6, r2, asl #1          @ x12 - x4
    add     r7, r7, r3                  @ y13 = x13 + x5
    sub     r3, r7, r3, asl #1          @ x13 - x5
    add     r8, r8, r4                  @ y10 = x14 + x6
    sub     r10, r8, r4, asl #1         @ x14 - x6
    add     r9, r9, r5                  @ y11 = x15 + x7
    sub     r11, r9, r5, asl #1         @ x15 - x7

    stmia   r1, {r6, r7, r8, r9}
    
    sub     r2, r2, r3                  @ (x12 - x4) - (x13 - x5)
    add     r3, r2, r3, asl #1          @ (x12 - x4) + (x13 - x5)

    smull   r8, r5, r2, r12
    mov     r5, r5, asl #1
    smull   r8, r6, r3, r12
    mov     r6, r6, asl #1
    @ no stmia here, r5, r6, r10, r11 are passed to mdct_butterfly_8

    sub     r0, r0, #4*4
    ldmia   r0, {r1, r2, r3, r4}
    bl      mdct_butterfly_8
    add     r0, r0, #8*4
    ldmia   r0, {r1, r2, r3, r4, r5, r6, r10, r11}
    bl      mdct_butterfly_8

    ldr     pc, [sp], #4

mdct_butterfly_32:
    stmdb   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}

    add     r1, r0, #16*4

    ldmia   r0, {r2, r3, r4, r5}
    ldmia   r1, {r6, r7, r8, r9}
    add     r6, r6, r2                  @ y16 = x16 + x0
    rsb     r2, r6, r2, asl #1          @ x0 - x16
    add     r7, r7, r3                  @ y17 = x17 + x1
    rsb     r3, r7, r3, asl #1          @ x1 - x17
    add     r8, r8, r4                  @ y18 = x18 + x2
    rsb     r4, r8, r4, asl #1          @ x2 - x18
    add     r9, r9, r5                  @ y19 = x19 + x3
    rsb     r5, r9, r5, asl #1          @ x3 - x19

    stmia   r1!, {r6, r7, r8, r9}

    ldr     r12, =cPI1_8
    ldr     lr, =cPI3_8
    smull   r10, r6, r2, r12
    smlal   r10, r6, r3, lr
    rsb     r2, r2, #0
    smull   r10, r7, r3, r12
    smlal   r10, r7, r2, lr
    mov     r6, r6, asl #1
    mov     r7, r7, asl #1

    add     r4, r4, r5                  @ (x3 - x19) + (x2 - x18) 
    rsb     r5, r4, r5, asl #1          @ (x3 - x19) - (x2 - x18)

    ldr     r11, =cPI2_8
    smull   r10, r8, r4, r11
    mov     r8, r8, asl #1
    smull   r10, r9, r5, r11
    mov     r9, r9, asl #1

    stmia   r0!, {r6, r7, r8, r9}
    
    ldmia   r0, {r2, r3, r4, r5}
    ldmia   r1, {r6, r7, r8, r9}
    add     r6, r6, r2                  @ y20 = x20 + x4
    rsb     r2, r6, r2, asl #1          @ x4 - x20
    add     r7, r7, r3                  @ y21 = x21 + x5
    rsb     r3, r7, r3, asl #1          @ x5 - x21
    add     r8, r8, r4                  @ y22 = x22 + x6
    sub     r4, r8, r4, asl #1          @ x22 - x6
    add     r9, r9, r5                  @ y23 = x23 + x7
    rsb     r5, r9, r5, asl #1          @ x7 - x23

    stmia   r1!, {r6, r7, r8, r9}

    smull   r10, r6, r2, lr
    smlal   r10, r6, r3, r12
    rsb     r2, r2, #0
    smull   r10, r7, r3, lr
    smlal   r10, r7, r2, r12
    mov     r6, r6, asl #1
    mov     r7, r7, asl #1

    mov     r8, r5
    mov     r9, r4
    stmia   r0!, {r6, r7, r8, r9}

    ldmia   r0, {r2, r3, r4, r5}
    ldmia   r1, {r6, r7, r8, r9}
    add     r6, r6, r2                  @ y24 = x24 + x8
    sub     r2, r6, r2, asl #1          @ x24 - x8
    add     r7, r7, r3                  @ y25 = x25 + x9
    sub     r3, r7, r3, asl #1          @ x25 - x9
    add     r8, r8, r4                  @ y26 = x26 + x10
    sub     r4, r8, r4, asl #1          @ x26 - x10
    add     r9, r9, r5                  @ y27 = x27 + x11
    sub     r5, r9, r5, asl #1          @ x27 - x11

    stmia   r1!, {r6, r7, r8, r9}

    smull   r10, r7, r2, r12
    smlal   r10, r7, r3, lr
    rsb     r3, r3, #0
    smull   r10, r6, r3, r12
    smlal   r10, r6, r2, lr
    mov     r6, r6, asl #1
    mov     r7, r7, asl #1

    sub     r4, r4, r5                  @ (x26 - x10) - (x27 - x11) 
    add     r5, r4, r5, asl #1          @ (x26 - x10) + (x27 - x11)

    ldr     r11, =cPI2_8
    smull   r10, r8, r4, r11
    mov     r8, r8, asl #1
    smull   r10, r9, r5, r11
    mov     r9, r9, asl #1

    stmia   r0!, {r6, r7, r8, r9}

    ldmia   r0, {r2, r3, r4, r5}
    ldmia   r1, {r6, r7, r8, r9}
    add     r6, r6, r2                  @ y28 = x28 + x12
    sub     r2, r6, r2, asl #1          @ x28 - x12
    add     r7, r7, r3                  @ y29 = x29 + x13
    sub     r3, r7, r3, asl #1          @ x29 - x13
    add     r8, r8, r4                  @ y30 = x30 + x14
    sub     r4, r8, r4, asl #1          @ x30 - x14
    add     r9, r9, r5                  @ y31 = x31 + x15
    sub     r5, r9, r5, asl #1          @ x31 - x15

    stmia   r1, {r6, r7, r8, r9}

    smull   r10, r7, r2, lr
    smlal   r10, r7, r3, r12
    rsb     r3, r3, #0
    smull   r10, r6, r3, lr
    smlal   r10, r6, r2, r12
    mov     r6, r6, asl #1
    mov     r7, r7, asl #1

    mov     r8, r4
    mov     r9, r5
    stmia   r0, {r6, r7, r8, r9}

    sub     r0, r0, #12*4
    str     r0, [sp, #-4]!
    bl      mdct_butterfly_16

    ldr     r0, [sp], #4
    add     r0, r0, #16*4
    bl      mdct_butterfly_16

    ldmia   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}

    @ mdct_butterfly_generic_loop(x1, x2, T0, step, Ttop)
mdct_butterfly_generic_loop:
    stmdb   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
    str     r2, [sp, #-4]
    ldr     r4, [sp, #40]
1:
    ldmdb   r0, {r6, r7, r8, r9}
    ldmdb   r1, {r10, r11, r12, r14}

    add     r6, r6, r10
    sub     r10, r6, r10, asl #1
    add     r7, r7, r11
    rsb     r11, r7, r11, asl #1
    add     r8, r8, r12
    sub     r12, r8, r12, asl #1
    add     r9, r9, r14
    rsb     r14, r9, r14, asl #1

    stmdb   r0!, {r6, r7, r8, r9}

    ldmia   r2, {r6, r7}
    smull   r5, r8, r14, r6
    smlal   r5, r8, r12, r7
    rsb     r14, r14, #0
    smull   r5, r9, r12, r6
    smlal   r5, r9, r14, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    add     r2, r2, r3, asl #2

    ldmia   r2, {r6, r7}
    smull   r5, r8, r11, r6
    smlal   r5, r8, r10, r7
    rsb     r11, r11, #0
    smull   r5, r9, r10, r6
    smlal   r5, r9, r11, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    add     r2, r2, r3, asl #2

    cmp     r2, r4
    blo     1b

    ldr     r4, [sp, #-4]
1:
    ldmdb   r0, {r6, r7, r8, r9}
    ldmdb   r1, {r10, r11, r12, r14}

    add     r6, r6, r10
    sub     r10, r6, r10, asl #1
    add     r7, r7, r11
    sub     r11, r7, r11, asl #1
    add     r8, r8, r12
    sub     r12, r8, r12, asl #1
    add     r9, r9, r14
    sub     r14, r9, r14, asl #1

    stmdb   r0!, {r6, r7, r8, r9}

    ldmia   r2, {r6, r7}
    smull   r5, r9, r14, r6
    smlal   r5, r9, r12, r7
    rsb     r14, r14, #0
    smull   r5, r8, r12, r6
    smlal   r5, r8, r14, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    sub     r2, r2, r3, asl #2

    ldmia   r2, {r6, r7}
    smull   r5, r9, r11, r6
    smlal   r5, r9, r10, r7
    rsb     r11, r11, #0
    smull   r5, r8, r10, r6
    smlal   r5, r8, r11, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    sub     r2, r2, r3, asl #2

    cmp     r2, r4
    bhi     1b

    ldr     r4, [sp, #40]
1:
    ldmdb   r0, {r6, r7, r8, r9}
    ldmdb   r1, {r10, r11, r12, r14}

    add     r6, r6, r10
    rsb     r10, r6, r10, asl #1
    add     r7, r7, r11
    rsb     r11, r7, r11, asl #1
    add     r8, r8, r12
    rsb     r12, r8, r12, asl #1
    add     r9, r9, r14
    rsb     r14, r9, r14, asl #1

    stmdb   r0!, {r6, r7, r8, r9}

    ldmia   r2, {r6, r7}
    smull   r5, r8, r12, r6
    smlal   r5, r8, r14, r7
    rsb     r12, r12, #0
    smull   r5, r9, r14, r6
    smlal   r5, r9, r12, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    add     r2, r2, r3, asl #2

    ldmia   r2, {r6, r7}
    smull   r5, r8, r10, r6
    smlal   r5, r8, r11, r7
    rsb     r10, r10, #0
    smull   r5, r9, r11, r6
    smlal   r5, r9, r10, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    add     r2, r2, r3, asl #2

    cmp     r2, r4
    blo     1b

    ldr     r4, [sp, #-4]
1:
    ldmdb   r0, {r6, r7, r8, r9}
    ldmdb   r1, {r10, r11, r12, r14}

    add     r6, r6, r10
    sub     r10, r6, r10, asl #1
    add     r7, r7, r11
    rsb     r11, r7, r11, asl #1
    add     r8, r8, r12
    sub     r12, r8, r12, asl #1
    add     r9, r9, r14
    rsb     r14, r9, r14, asl #1

    stmdb   r0!, {r6, r7, r8, r9}

    ldmia   r2, {r6, r7}
    smull   r5, r9, r12, r6
    smlal   r5, r9, r14, r7
    rsb     r12, r12, #0
    smull   r5, r8, r14, r6
    smlal   r5, r8, r12, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    sub     r2, r2, r3, asl #2

    ldmia   r2, {r6, r7}
    smull   r5, r9, r10, r6
    smlal   r5, r9, r11, r7
    rsb     r10, r10, #0
    smull   r5, r8, r11, r6
    smlal   r5, r8, r10, r7

    mov     r8, r8, asl #1
    mov     r9, r9, asl #1
    stmdb   r1!, {r8, r9}
    sub     r2, r2, r3, asl #2

    cmp     r2, r4
    bhi     1b

    ldmia   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}

