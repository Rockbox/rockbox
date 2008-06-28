/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "mad_iram.h"

    .global dct32

    .section    ICODE_SECTION_MPA_ARM,"ax",%progbits

dct32:
    stmdb    r13!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
    sub      r13, r13, #144
    str      r0, [r13, #12]
    str      r1, [r13, #8]
    str      r2, [r13, #4]
    str      r3, [r13]
    add      r0, r13, #16
    add      r1, r0, #128
    ldr      r2, =bitrev
.shuffle:
    ldr      r5, [r13, #12]
    ldr      r3, [r2], #4
    sub      r4, r5, r3, lsl #4
    add      r3, r5, r3, lsl #4
    ldr      r6, [r3]
    ldr      r8, [r4, #124]
    add      r6, r6, r8
    sub      r8, r6, r8, lsl #1
    ldr      r7, [r3, #8]
    ldr      lr, [r4, #116]
    add      r7, r7, lr
    sub      lr, r7, lr, lsl #1
    ldr      r10, [r3, #64]
    ldr      r9, [r4, #60]
    add      r10, r10, r9
    sub      r9, r10, r9, lsl #1
    ldr      r11, [r3, #72]
    ldr      r12, [r4, #52]
    add      r11, r11, r12
    sub      r12, r11, r12, lsl #1
    add      r6, r6, r10
    sub      r10, r6, r10, lsl #1
    add      r7, r7, r11
    sub      r11, r7, r11, lsl #1
    add      r8, r8, r12
    sub      r12, r8, r12, lsl #1
    add      lr, lr, r9
    sub      r9, lr, r9, lsl #1
    stmia    r0!, {r6, r7, r8, r9, r10, r11, r12, lr}
    cmp      r0, r1
    bne      .shuffle
    ldr      r0, =189812531
    add      r1, r13, #16
    add      r3, r1, #128
.l2:
    add      r2, r1, #32
    ldmia    r2, {r4, r5, r8, r9}
    ldmia    r1, {r6, r7, r10, r11}
    add      r6, r6, r4
    sub      r4, r6, r4, lsl #1
    add      r7, r7, r5
    sub      r5, r7, r5, lsl #1
    stmia    r2!, {r4, r5}
    stmia    r1!, {r6, r7}
    add      r9, r9, r8
    sub      r8, r9, r8, lsl #1
    smull    r4, r6, r9, r0
    movs     r4, r4, lsr #28
    adc      r4, r4, r6, lsl #4
    smull    r5, r6, r8, r0
    movs     r5, r5, lsr #28
    adc      r5, r5, r6, lsl #4
    add      r10, r10, r4
    sub      r4, r10, r4, lsl #1
    add      r11, r11, r5
    sub      r5, r11, r5, lsl #1
    stmia    r2!, {r4, r5}
    stmia    r1!, {r10, r11}
    ldmia    r2, {r5, r6, r8, r11}
    ldmia    r1, {r4, r7, r9, r10}
    add      r4, r4, r6
    sub      r6, r4, r6, lsl #1
    add      r7, r7, r5
    sub      r5, r7, r5, lsl #1
    stmia    r2!, {r6, r7}
    stmia    r1!, {r4, r5}
    add      r11, r11, r8
    sub      r8, r11, r8, lsl #1
    smull    r5, r4, r8, r0
    movs     r5, r5, lsr #28
    adc      r5, r5, r4, lsl #4
    smull    r6, r4, r11, r0
    movs     r6, r6, lsr #28
    adc      r6, r6, r4, lsl #4
    add      r9, r9, r5
    sub      r5, r9, r5, lsl #1
    sub      r10, r10, r6
    add      r6, r10, r6, lsl #1
    stmia    r2!, {r5, r6}
    stmia    r1!, {r9, r10}
    add      r1, r1, #32
    cmp      r1, r3
    bne      .l2
    add      r2, r13, #16
    add      r3, r2, #64
    ldr      r0, =sincos
    add      r1, r0, #128
.lbut8:
    ldmia    r3, {r7, r8}
    ldmia    r0, {r9, r10}
    add      r0, r0, #16
    smull    r6, r5, r7, r9
    smlal    r6, r5, r10, r8
    movs     r6, r6, lsr #28
    adc      r6, r6, r5, lsl #4
    smull    r10, r5, r7, r10
    rsb      r9, r9, #0
    smlal    r10, r5, r8, r9
    movs     r10, r10, lsr #28
    adc      r5, r10, r5, lsl #4
    ldmia    r2, {r7, r8}
    add      r7, r7, r5
    sub      r5, r7, r5, lsl #1
    add      r8, r8, r6
    sub      r6, r8, r6, lsl #1
    stmia    r3!, {r5, r6}
    stmia    r2!, {r7, r8}
    cmp      r0, r1
    bne      .lbut8
    add      r1, r13, #16
    ldr      r2, =sincos
    ldr      r3, =sincos2
    ldr      r0, [r13, #8]
    mov      r0, r0, lsl #2
    ldr      r4, [r13, #4]
    add      r4, r4, r0
    ldr      r5, [r13]
    add      r5, r5, #480
    add      r5, r5, r0
    mov      r0, #0
.l4:
    rsb      r12, r0, #16
    and      r12, r12, #15
    add      lr, r13, #16
    add      r12, lr, r12, lsl #3
    ldmia    r1!, {r10, r11}
    ldmia    r12, {r6, r7}
    add      r6, r6, r10
    sub      r10, r6, r10, lsl #1
    add      r11, r11, r7
    sub      r7, r11, r7, lsl #1
    ldmia    r2!, {r12, lr}
    smull    r9, r8, r11, r12
    smlal    r9, r8, lr, r10
    movs     r9, r9, lsr #28
    adc      r9, r9, r8, lsl #4
    smull    lr, r8, r11, lr
    rsb      r12, r12, #0
    smlal    lr, r8, r10, r12
    movs     lr, lr, lsr #28
    adc      r8, lr, r8, lsl #4
    add      r6, r6, r8
    sub      r8, r6, r8, lsl #1
    add      r7, r7, r9
    sub      r9, r7, r9, lsl #1
    add      lr, r3, #128
    ldmia    lr, {r10, r11}
    smull    lr, r12, r8, r11
    smlal    lr, r12, r9, r10
    movs     lr, lr, lsr #28
    adc      r12, lr, r12, lsl #4
    str      r12, [r4], #32
    cmp      r0, #0
    cmpne    r0, #8
    beq      .skip1
    smull    lr, r12, r8, r10
    rsb      r9, r9, #0
    smlal    lr, r12, r9, r11
    movs     lr, lr, lsr #28
    adc      r12, lr, r12, lsl #4
    add      lr, r5, r0, lsl #6
    str      r12, [lr, #-512]
.skip1:
    ldmia    r3!, {r10, r11}
    smull    lr, r12, r7, r10
    smlal    lr, r12, r6, r11
    movs     lr, lr, lsr #28
    adc      r12, lr, r12, lsl #4
    str      r12, [r5], #-32
    cmp      r0, #0
    cmpne    r0, #8
    beq      .skip2
    smull    lr, r12, r6, r10
    rsb      r7, r7, #0
    smlal    lr, r12, r7, r11
    movs     lr, lr, lsr #28
    adc      r12, lr, r12, lsl #4
    sub      lr, r4, r0, lsl #6
    str      r12, [lr, #480]
.skip2:
    add      r0, r0, #1
    cmp      r0, #9
    bne      .l4
    add      r13, r13, #144
    ldmia    r13!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}
bitrev:
    .word 0x0
    .word 0x2
    .word 0x1
    .word 0x3

sincos:
    .word 0x0
    .word 0x10000000
    .word -0x31f1708
    .word 0xfb14be8
    .word -0x61f78aa
    .word 0xec835e8
    .word -0x8e39d9d
    .word 0xd4db315
    .word -0xb504f33
    .word 0xb504f33
    .word -0xd4db315
    .word 0x8e39d9d
    .word -0xec835e8
    .word 0x61f78aa
    .word -0xfb14be8
    .word 0x31f1708
    .word -0x10000000
    .word 0x0
    .word -0xfb14be8
    .word -0x31f1708
    .word -0xec835e8
    .word -0x61f78aa
    .word -0xd4db315
    .word -0x8e39d9d
    .word -0xb504f33
    .word -0xb504f33
    .word -0x8e39d9d
    .word -0xd4db315
    .word -0x61f78aa
    .word -0xec835e8
    .word -0x31f1708
    .word -0xfb14be8

sincos2:
    .word 0x0
    .word 0x8000000
    .word 0x647d98
    .word 0x7fd8879
    .word 0xc8bd36
    .word 0x7f62369
    .word 0x12c8107
    .word 0x7e9d560
    .word 0x18f8b84
    .word 0x7d8a5f4
    .word 0x1f19f98
    .word 0x7c29fbf
    .word 0x25280c6
    .word 0x7a7d056
    .word 0x2b1f34f
    .word 0x7884841
    .word 0x30fbc55
    .word 0x7641af4
    .word 0x36ba201
    .word 0x73b5ebd
    .word 0x3c56ba7
    .word 0x70e2cbc
    .word 0x41ce1e6
    .word 0x6dca0d1
    .word 0x471cece
    .word 0x6a6d98a
    .word 0x4c3fdff
    .word 0x66cf812
    .word 0x5133cc9
    .word 0x62f201b
    .word 0x55f5a4d
    .word 0x5ed77c9
    .word 0x5a8279a
    .word 0x5a8279a
    .word 0x5ed77c9
    .word 0x55f5a4d
    .word 0x62f201b
    .word 0x5133cc9
    .word 0x66cf812
    .word 0x4c3fdff
    .word 0x6a6d98a
    .word 0x471cece
    .word 0x6dca0d1
    .word 0x41ce1e6
    .word 0x70e2cbc
    .word 0x3c56ba7
    .word 0x73b5ebd
    .word 0x36ba201
    .word 0x7641af4
    .word 0x30fbc55
    .word 0x7884841
    .word 0x2b1f34f
    .word 0x7a7d056
    .word 0x25280c6
    .word 0x7c29fbf
    .word 0x1f19f98
    .word 0x7d8a5f4
    .word 0x18f8b84
    .word 0x7e9d560
    .word 0x12c8107
    .word 0x7f62369
    .word 0xc8bd36
    .word 0x7fd8879
    .word 0x647d98
