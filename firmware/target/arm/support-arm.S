/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 * Copyright (C) 2009 by Andrew Mahone
 *
 * Optimised replacements for libgcc functions
 *
 * Based on: libgcc routines for ARM cpu, additional algorithms from ARM System
 *           Developer's Guide
 * Division routines, written by Richard Earnshaw, (rearnsha@armltd.co.uk)
 * Copyright 1995, 1996, 1998, 1999, 2000, 2003, 2004, 2005
 * Free Software Foundation, Inc.
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

#include <config.h>

.macro ARM_SDIV32_PRE numerator, divisor, sign
    /* sign[31] = divisor sign */
    ands    \sign, \divisor, #1<<31
    rsbeq   \divisor, \divisor, #0
    /* sign[31] = result sign, sign[0:30], C = numerator sign */
    eors    \sign, \sign, \numerator, asr #32
    rsbcs   \numerator, \numerator, #0
.endm

.macro ARM_SDIV32_POST quotient, remainder, sign
    movs    \sign, \sign, lsl #1
.ifnc "", "\quotient"
    rsbcs   \quotient, \quotient, #0
.endif
.ifnc "", "\remainder"
    rsbmi   \remainder, \remainder, #0
.endif
.endm

#if ARM_ARCH < 5
.macro ARMV4_UDIV32_BODY numerator, divisor, quotient, remainder, tmp, bits, div0label, return
.ifnc "", "\div0label"
    rsbs    \divisor, \divisor, #0
    beq     \div0label
.else
    rsb     \divisor, \divisor, #0
.endif
    /* This SWAR divider requires a numerator less than 1<<31, because it must
       be able to shift the remainder left at each step without shifting out
       topmost bit. Since a shift might be needed for the aligned remainder to
       exceed the divisor, the topmost bit must be unset at the start to avoid
       this overflow case. The original numerator is saved so that the result
       can be corrected after the reduced division completes. */
    cmn     \numerator, \divisor
.ifc "", "\quotient"
.ifc "\numerator", "\remainder"
.if \return
    bxcc    lr
.else
    b 99f
.endif
.else
    bcc     20f
.endif
.else
    bcc     20f
.endif
    movs    \tmp, \numerator
    movmi   \numerator, \numerator, lsr #1
    mov     \bits, #30
.set shift, 16
.rept 5
    cmn     \divisor, \numerator, lsr #shift
    subcs   \bits, \bits, #shift
    movcs   \divisor, \divisor, lsl #shift
.set shift, shift >> 1
.endr
    adds    \numerator, \numerator, \divisor
    subcc   \numerator, \numerator, \divisor
    add     pc, pc, \bits, lsl #3
    nop
.rept 30
    adcs    \numerator, \divisor, \numerator, lsl #1
    subcc   \numerator, \numerator, \divisor
.endr
    adc     \numerator, \numerator, \numerator
    movs    \tmp, \tmp, asr #1
    rsb     \bits, \bits, #31
    bmi     10f
.ifc "", "\quotient"
    mov     \remainder, \numerator, lsr \bits
.else
.ifc "", "\remainder"
    mov     \divisor, \numerator, lsr \bits
    eor     \quotient, \numerator, \divisor, lsl \bits
.else
    mov     \remainder, \numerator, lsr \bits
    eor     \quotient, \numerator, \remainder, lsl \bits
.endif
.endif
.ifne \return        
    bx      lr
.else
    b       99f
.endif
10:
    mov     \tmp, \numerator, lsr \bits
    eor     \numerator, \numerator, \tmp, lsl \bits
    sub     \bits, \bits, #1
    adc     \tmp, \tmp, \tmp
    adds    \tmp, \tmp, \divisor, asr \bits
.ifnc "", "\quotient"
    adc     \quotient, \numerator, \numerator
.endif
.ifnc "", "\remainder"
    subcc   \remainder, \tmp, \divisor, asr \bits
    movcs   \remainder, \tmp
.endif
.ifne \return        
    bx      lr
.else
    b       99f
.endif
20:
.ifnc "", "\remainder"
.ifnc "\remainder", "\numerator"
    mov     \remainder, \numerator
.endif
.endif
.ifnc "", "\quotient"
    mov   \quotient, #0
.endif
.ifne \return        
    bx      lr
.else
99:
.endif
.endm

.macro ARMV4_SDIV32_BODY numerator, divisor, quotient, remainder, bits, sign, div0label, return
    /* When this is wrapped for signed division, the wrapper code will handle
       inverting the divisor, and also the zero divisor test. */
    ARM_SDIV32_PRE \numerator, \divisor, \sign
.ifnc "",   "\div0label"
    tst     \divisor, \divisor
    beq     \div0label
.endif
    /* This SWAR divider requires a numerator less than 1<<31, because it must
       be able to shift the remainder left at each step without shifting out
       topmost bit. With signed inputs, whose absolute value may not exceed
       1<<31,this may be accomplished simply by subtracting the divisor before
       beginning division, and adding 1 to the quotient. */
    adds    \numerator, \numerator, \divisor
    bcc     20f
    mov     \bits, #30
.set shift, 16
.rept 5
    cmn     \divisor, \numerator, lsr #shift
    subcs   \bits, \bits, #shift
    movcs   \divisor, \divisor, lsl #shift
.set shift, shift >> 1
.endr
    adds    \numerator, \numerator, \divisor
    subcc   \numerator, \numerator, \divisor
    add     pc, pc, \bits, lsl #3
    nop
.rept 30
    adcs    \numerator, \divisor, \numerator, lsl #1
    subcc   \numerator, \numerator, \divisor
.endr
    rsb     \bits, \bits, #31
    adc     \numerator, \numerator, \numerator
.ifc "", "\quotient"
    mov     \remainder, \numerator, lsr \bits
.else
.ifc "", "\remainder"
    mov     \divisor, \numerator, lsr \bits
    add     \numerator, \numerator, #1
    sub     \quotient, \numerator, \divisor, lsl \bits
.else
    mov     \remainder, \numerator, lsr \bits
    add     \numerator, \numerator, #1
    sub     \quotient, \numerator, \remainder, lsl \bits
.endif
.endif
.ifne \return        
    ARM_SDIV32_POST \quotient, \remainder, \sign
    bx      lr
.else
    b       99f
.endif
20:
.ifnc "", "\remainder"
    sub     \remainder, \numerator, \divisor
.endif
.ifnc "", "\quotient"
    mov     \quotient, #0
.endif
.ifne \return        
    ARM_SDIV32_POST "", \remainder, \sign
    bx      lr
.else
99:
    ARM_SDIV32_POST \quotient, \remainder, \sign
.endif
.endm

#else
.macro ARMV5_UDIV32_BODY numerator, divisor, quotient, remainder, bits, inv, neg, div0label, return
    cmp     \numerator, \divisor
    clz     \bits, \divisor
    bcc     30f
    mov     \inv, \divisor, lsl \bits
    add     \neg, pc, \inv, lsr #25
    /* Test whether divisor is 2^N */
    cmp     \inv, #1<<31
    /* Load approximate reciprocal */
    ldrhib  \inv, [\neg, #.L_udiv_est_table-.-64]
    bls     20f
    subs    \bits, \bits, #7
    rsb     \neg, \divisor, #0
    /* Scale approximate reciprocal, or else branch to large-divisor path */
    movpl   \divisor, \inv, lsl \bits
    bmi     10f
    /* Newton-Raphson iteration to improve reciprocal accuracy */
    mul     \inv, \divisor, \neg
    smlawt  \divisor, \divisor, \inv, \divisor
    mul     \inv, \divisor, \neg
    /* Complete N-R math and produce approximate quotient. Use smmla/smmul on
       ARMv6. */
#if ARM_ARCH >= 6
    tst     \numerator, \numerator
    smmla   \divisor, \divisor, \inv, \divisor
    /* Branch to large-numerator handler, or else use smmul if sign bit is not
       set. */
    bmi     40f
    smmul   \inv, \numerator, \divisor
#else
    /* ARMv5e lacks smmul, so always uses umull. */
    mov     \bits, #0
    smlal   \bits, \divisor, \inv, \divisor
    umull   \bits, \inv, \numerator, \divisor
#endif
    /* Calculate remainder and correct result. */
    add     \numerator, \numerator, \neg
.ifnc "", "\remainder"
    mla     \remainder, \inv, \neg, \numerator
.ifnc "", "\quotient"
    mov     \quotient, \inv
    cmn     \remainder, \neg
    subcs   \remainder, \remainder, \neg
    addpl   \remainder, \remainder, \neg, lsl #1
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
.else
    cmn     \remainder, \neg
    subcs   \remainder, \remainder, \neg
    addpl   \remainder, \remainder, \neg, lsl #1
.endif
.else
    mla     \divisor, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \divisor, \neg
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
.endif
.if \return
    bx      lr
.else
    b       99f
.endif
10:
    /* Very large divisors can be handled without further improving the
       reciprocal. First the reciprocal must be reduced to ensure that it
       underestimates the correct value. */
    rsb     \bits, \bits, #0
    sub     \inv, \inv, #4
    mov     \divisor, \inv, lsr \bits
    /* Calculate approximate quotient and remainder */
    umull   \bits, \inv, \numerator, \divisor
    /* Correct quotient and remainder */
.ifnc "", "\remainder"
    mla     \remainder, \inv, \neg, \numerator
.ifnc "", "\quotient"
    mov     \quotient, \inv
    cmn     \neg, \remainder, lsr #1
    addcs   \remainder, \remainder, \neg, lsl #1
    addcs   \quotient, \quotient, #2
    cmn     \neg, \remainder
    addcs   \remainder, \remainder, \neg
    addcs   \quotient, \quotient, #1
.else
    cmn     \neg, \remainder, lsr #1
    addcs   \remainder, \remainder, \neg, lsl #1
    cmn     \neg, \remainder
    addcs   \remainder, \remainder, \neg
.endif
.else
    mla     \divisor, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \neg, \divisor, lsr #1
    addcs   \divisor, \divisor, \neg, lsl #1
    addcs   \quotient, \quotient, #2
    cmn     \neg, \divisor
    addcs   \quotient, \quotient, #1
.endif
.if \return
    bx      lr
.else
    b       99f
.endif
20:
    /* Handle division by powers of two by shifting right. Mod is handled
       by using divisor-1 as a bitmask. */
.ifnc "", "\remainder"
.ifnc "", "\div0label"
    bne     \div0label
.endif
.ifnc "", "\quotient"
    sub     \divisor, \divisor, #1
    rsb     \bits, \bits, #31
    and     \remainder, \numerator, \divisor
    mov     \quotient, \numerator, lsr \bits
.else
    sub     \divisor, \divisor, #1
    and     \remainder, \numerator, \divisor
.endif
.else
    rsb     \bits, \bits, #31
.ifnc "", "\div0label"
    bne     \div0label
.endif
    mov     \quotient, \numerator, lsr \bits
.endif
.if \return
    bx      lr
.else
    b       99f
.endif
30:
    /* Handle numerator < divisor - quotient is zero, remainder is numerator,
       which must be restored to its original value on ARMv6. */
.ifnc "", "\remainder"
    mov     \remainder, \numerator
.endif
.ifnc "", "\quotient"
    mov     \quotient, #0
.endif
.if \return
    bx      lr
.endif
#if ARM_ARCH >= 6
40:
    /* Handle large (sign bit set) numerators. Works exactly as the ARMv5e code
       above 10:. */
    umull   \bits, \inv, \numerator, \divisor
    add     \numerator, \numerator, \neg
.ifnc "", "\remainder"
    mla     \remainder, \inv, \neg, \numerator
.ifnc "", "\quotient"
    mla     \remainder, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \remainder, \neg
    subcs   \remainder, \remainder, \neg
    addpl   \remainder, \remainder, \neg, lsl #1
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
.else
    cmn     \remainder, \neg
    subcs   \remainder, \remainder, \neg
    addpl   \remainder, \remainder, \neg, lsl #1
.endif
.else
    mla     \divisor, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \divisor, \neg
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
.endif
.if \return
    bx      lr
.else
    b       99f
.endif
#endif
99:
.endm

.macro ARMV5_SDIV32_BODY numerator, divisor, quotient, remainder, bits, inv, neg, sign, div0label, return
    /* sign[31] = divisor sign */
    ands    \sign, \divisor, #1<<31
    rsbne   \divisor, \divisor, #0
    /* sign[31] = result sign, sign[0:30], C = numerator sign */
    eors    \sign, \sign, \numerator, asr #32
    clz     \bits, \divisor
    rsbcs   \numerator, \numerator, #0
    /* On ARMv6, subtract divisor before performing division, which ensures
       numerator sign bit is clear and smmul may be used in place of umull. The
       fixup for the results can be fit entirely into existing delay slots on
       the main division paths. It costs 1c in the num<div path if the
       the remainder is to be produced in the numerator's register, and 1c in
       the power-of-2-divisor path only if producing both remainder and
       quotient. */
#if ARM_ARCH >= 6
    subs    \numerator, \numerator, \divisor
#else
    cmp     \numerator, \divisor
#endif
    movcs   \inv, \divisor, lsl \bits
    bcc     30f
    /* Test whether divisor is 2^N */
    cmp     \inv, #1<<31
    add     \inv, pc, \inv, lsr #25
    bls     20f
    /* Load approximate reciprocal */
    ldrb    \inv, [\inv, #.L_udiv_est_table-.-64]
    subs    \bits, \bits, #7
    rsb     \neg, \divisor, #0
    /* Scale approximate reciprocal, or else branch to large-divisor path */
    movpl   \divisor, \inv, lsl \bits
    bmi     10f
    /* Newton-Raphson iteration to improve reciprocal accuracy */
    mul     \inv, \divisor, \neg
    smlawt  \divisor, \divisor, \inv, \divisor
    mul     \inv, \divisor, \neg
    /* Complete N-R math and produce approximate quotient. Use smmla/smmul on
       ARMv6. */
#if ARM_ARCH >= 6
    smmla   \divisor, \divisor, \inv, \divisor
    smmul   \inv, \numerator, \divisor
#else
    mov     \bits, #0
    smlal   \bits, \divisor, \inv, \divisor
    umull   \bits, \inv, \numerator, \divisor
#endif
    /* Calculate remainder and correct quotient. */
    add     \numerator, \numerator, \neg
.ifnc "", "\remainder"
    mla     \remainder, \inv, \neg, \numerator
.ifnc "", "\quotient"
#if ARM_ARCH >= 6
    add     \quotient, \inv, #1
#else
    mov     \quotient, \inv
#endif
    cmn     \remainder, \neg
    subcs   \remainder, \remainder, \neg
    addpl   \remainder, \remainder, \neg, lsl #1
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
.else
    cmn     \remainder, \neg
    subcs   \remainder, \remainder, \neg
    addpl   \remainder, \remainder, \neg, lsl #1
.endif
.else
    mla     \divisor, \inv, \neg, \numerator
#if ARM_ARCH >= 6
    add     \quotient, \inv, #1
#else
    mov     \quotient, \inv
#endif
    cmn     \divisor, \neg
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
.endif
    ARM_SDIV32_POST \quotient, \remainder, \sign
.ifnc "", "\return"
    \return
.else
    b       99f
.endif
10:
    /* Very large divisors can be handled without further improving the
       reciprocal. First the reciprocal must be reduced to ensure that it
       underestimates the correct value. */
    rsb     \bits, \bits, #0
    sub     \inv, \inv, #4
    mov     \divisor, \inv, lsr \bits
    /* Calculate approximate quotient and remainder */
#if ARM_ARCH >= 6
    smmul   \inv, \numerator, \divisor
#else
    umull   \bits, \inv, \numerator, \divisor
#endif
    /* Correct quotient and remainder */
.ifnc "", "\remainder"
    mla     \remainder, \inv, \neg, \numerator
.ifnc "", "\quotient"
#if ARM_ARCH >= 6
    add     \quotient, \inv, #1
#else
    mov     \quotient, \inv
#endif
    cmn     \neg, \remainder, lsr #1
    addcs   \remainder, \remainder, \neg, lsl #1
    addcs   \quotient, \quotient, #2
    cmn     \neg, \remainder
    addcs   \remainder, \remainder, \neg
    addcs   \quotient, \quotient, #1
.else
    cmn     \neg, \remainder, lsr #1
    addcs   \remainder, \remainder, \neg, lsl #1
    cmn     \neg, \remainder
    addcs   \remainder, \remainder, \neg
.endif
.else
    mla     \divisor, \inv, \neg, \numerator
#if ARM_ARCH >= 6
    add     \quotient, \inv, #1
#else
    mov     \quotient, \inv
#endif
    cmn     \neg, \divisor, lsr #1
    addcs   \divisor, \divisor, \neg, lsl #1
    addcs   \quotient, \quotient, #2
    cmn     \neg, \divisor
    addcs   \quotient, \quotient, #1
.endif
    ARM_SDIV32_POST \quotient, \remainder, \sign
.ifnc "", "\return"
    \return
.else
    b       99f
.endif
20:
    /* Handle division by powers of two by shifting right. Mod is handled
       by using divisor-1 as a bitmask. */
.ifnc "", "\div0label"
    bne     \div0label
.endif
.ifnc "", "\remainder"
.ifnc "", "\quotient"
    rsb     \bits, \bits, #31
#if ARM_ARCH >= 6
    add     \numerator, \numerator, \divisor
#endif
    sub     \divisor, \divisor, #1
    and     \remainder, \numerator, \divisor
    mov     \quotient, \numerator, lsr \bits
.else
    sub     \divisor, \divisor, #1
    and     \remainder, \numerator, \divisor
.endif
.else
    rsb     \bits, \bits, #31
#if ARM_ARCH >= 6
    add     \numerator, \numerator, \divisor
#endif
    mov     \quotient, \numerator, lsr \bits
.endif
    ARM_SDIV32_POST \quotient, \remainder, \sign
.ifnc "", "\return"
    \return
.else
    b       99f
.endif
30:
    /* Handle numerator < divisor - quotient is zero, remainder is numerator,
       which must be restored to its original value on ARMv6. */
.ifnc "", "\remainder"
#if ARM_ARCH >= 6
    add     \remainder, \numerator, \divisor
#else
.ifnc "\remainder", "\numerator"
    mov     \remainder, \numerator
.endif
#endif
.endif
.ifnc "", "\quotient"
    mov     \quotient, #0
.endif
    ARM_SDIV32_POST "", \remainder, \sign
.ifnc "", "\return"
    \return
.endif
99:
.endm
#endif    

    .section .text

__div0_wrap_s:
    sub sp, sp, #4
    mov     r0, #0
    b       __div0
    .size __div0_wrap_s, . - __div0_wrap_s

__div0_wrap:
    str     lr, [sp, #-4]!
    mov     r0, #0
    b       __div0
    .size __div0_wrap, . - __div0_wrap

#ifndef __ARM_EABI__
    .global __divsi3
    .type   __divsi3,%function
    .global __udivsi3
    .type   __udivsi3,%function
    .global __udivsi3
    .type   __udivsi3,%function
#else
/* The div+mod averagess a fraction of a cycle worse for signed values, and
   slightly better for unsigned, so just alias div to divmod. */
    .global __aeabi_uidivmod
    .type   __aeabi_uidivmod,%function
    .global __aeabi_uidiv
    .type   __aeabi_uidiv,%function
    .set    __aeabi_uidiv,__aeabi_uidivmod
    .global __aeabi_idivmod
    .type   __aeabi_idivmod,%function
    .global __aeabi_idiv
    .type   __aeabi_idiv,%function
    .set    __aeabi_idiv,__aeabi_idivmod
#endif


#if ARM_ARCH < 5
    .global __clzsi2
    .type   __clzsi2, %function

__clzsi2:
    orr r0, r0, r0, lsr #8
    orr r0, r0, r0, lsr #4
    orr r0, r0, r0, lsr #2
    orr r0, r0, r0, lsr #1
    bic r0, r0, r0, lsr #16
    rsb r0, r0, r0, lsl #14
    rsb r0, r0, r0, lsl #11
    rsb r0, r0, r0, lsl #9
    ldrb r0, [pc, r0, lsr #26]
    bx lr
    .byte 32, 20, 19,  0,  0, 18,  0,  7, 10, 17,  0,  0, 14,  0,  6,  0
    .byte  0,  9,  0, 16,  0,  0,  1, 26,  0, 13,  0,  0, 24,  5,  0,  0
    .byte  0, 21,  0,  8, 11,  0, 15,  0,  0,  0,  0,  2, 27,  0, 25,  0
    .byte 22,  0, 12,  0,  0,  3, 28,  0, 23,  0,  4, 29,  0,  0, 30, 31
    .size __clzsi2, .-__clzsi2

#ifndef __ARM_EABI__
__udivsi3:
    ARMV4_UDIV32_BODY r0, r1, r0, "", r2, r3, __div0_wrap, 1
    .size __udivsi3, . - __udivsi3

__divsi3:
    ARMV4_SDIV32_BODY r0, r1, r0, "", r2, r3, __div0_wrap, 1
    .size __divsi3, . - __divsi3

#else
__aeabi_uidivmod:
    ARMV4_UDIV32_BODY r0, r1, r0, r1, r2, r3, __div0_wrap, 1
    .size __aeabi_uidivmod, . - __aeabi_uidivmo

__aeabi_idivmod
    ARMV4_SDIV32_BODY r0, r1, r0, r1, r2, r3, __div0_wrap, 1
    .size __aeabi_idivmod, . - __aeabi_idivmod
#endif

#else
#ifndef __ARM_EABI__
__udivsi3:
    ARMV5_UDIV32_BODY r0, r1, r0, "", r2, r3, ip, __div0_wrap, 1
    .size __udivsi3, . - __udivsi3

__divsi3:
    ARMV5_SDIV32_BODY r0, r1, r0, "", r2, lr, ip, r3, __div0_wrap_s, "ldr pc, [sp, #-4]"
    .size __divsi3, . - __divsi3

#else
__aeabi_uidivmod:
    ARMV5_UDIV32_BODY r0, r1, r0, r1, r2, r3, ip, __div0_wrap, 1
    .size __aeabi_uidivmod, . - __aeabi_uidivmo

__aeabi_idivmod
    ARMV5_SDIV32_BODY r0, r1, r0, r1, r2, lr, ip, r3, __div0_wrap_s, "ldr pc, [sp, #-4]"
    .size __aeabi_idivmod, . - __aeabi_idivmod
#endif

.L_udiv_est_table:
    .byte 0xff, 0xfc, 0xf8, 0xf4, 0xf0, 0xed, 0xea, 0xe6
    .byte 0xe3, 0xe0, 0xdd, 0xda, 0xd7, 0xd4, 0xd2, 0xcf
    .byte 0xcc, 0xca, 0xc7, 0xc5, 0xc3, 0xc0, 0xbe, 0xbc
    .byte 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac
    .byte 0xaa, 0xa8, 0xa7, 0xa5, 0xa3, 0xa2, 0xa0, 0x9f
    .byte 0x9d, 0x9c, 0x9a, 0x99, 0x97, 0x96, 0x94, 0x93
    .byte 0x92, 0x90, 0x8f, 0x8e, 0x8d, 0x8c, 0x8a, 0x89
    .byte 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81
#endif
