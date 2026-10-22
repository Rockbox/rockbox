/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael Giacomelli
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

/* The pre- and post-twiddles of dct4_kernel() in sbr_dct.c, for ARMv4 and
 * up.  Each butterfly is three MUL_C()s, (a * b + 2^27) >> 28, with the
 * data as the multiplier's Rs operand: it is usually narrower than the
 * coefficients, and ARM7TDMI multiplies finish early when Rs is small.
 * The results are those of the C code, bit for bit. */

    .syntax unified
    .text
    .align  2

/* void dct4_pre_arm(const int32_t *real, const int32_t *imag,
 *                   FFTComplex *xc, const int32_t *tab,
 *                   const uint32_t *revtab);
 *
 * for i in 0..31:
 *     tmp = MUL_C(real[i] + imag[i], tab[3i])
 *     xc[revtab[i]].im = MUL_C(real[i], tab[3i+1]) + tmp
 *     xc[revtab[i]].re = MUL_C(imag[i], tab[3i+2]) + tmp
 */
    .global dct4_pre_arm
    .type   dct4_pre_arm, %function
dct4_pre_arm:
    stmfd   sp!, {r4-r11, lr}
    ldr     r4, [sp, #36]           @ r4 = revtab
    add     r5, r0, #32*4           @ r5 = end of real
1:
    ldr     r7, [r0], #4            @ r7 = x_re
    ldr     r6, [r1], #4            @ r6 = x_im
    ldmia   r3!, {r8, r9, r10}      @ the three coefficients
    add     lr, r7, r6
    smull   r11, r12, r8, lr
    ldr     r8, [r4], #4            @ r8 = revtab[i]
    movs    r11, r11, lsr #28
    adc     lr, r11, r12, lsl #4    @ lr = tmp
    smull   r11, r12, r9, r7
    add     r8, r2, r8, lsl #3      @ r8 = &xc[revtab[i]]
    movs    r11, r11, lsr #28
    adc     r7, r11, r12, lsl #4
    smull   r11, r12, r10, r6
    add     r7, r7, lr              @ r7 = im
    movs    r11, r11, lsr #28
    adc     r6, r11, r12, lsl #4
    add     r6, r6, lr              @ r6 = re
    stmia   r8, {r6, r7}
    cmp     r0, r5
    bne     1b
    ldmpc   regs=r4-r11
    .size   dct4_pre_arm, .-dct4_pre_arm

/* void dct4_post_arm(int32_t *real, int32_t *imag, const FFTComplex *xc,
 *                    const int32_t *tab);
 *
 * real[0] = xc[0].re, imag[0] = xc[0].im
 * for i in 1..31 but 16, with x = xc[32 - i]:
 *     tmp = MUL_C(x.re + x.im, tab[3i])
 *     imag[i] = MUL_C(x.re, tab[3i+1]) + tmp
 *     real[i] = MUL_C(x.im, tab[3i+2]) + tmp
 * and with x = xc[16] and c = tab[48] (sqrt(1/2)):
 *     imag[16] = MUL_C(x.im - x.re, c)
 *     real[16] = MUL_C(x.re + x.im, c)
 */
.macro post_butterfly
    ldmdb   r2!, {r4, r5}           @ r4 = x.re, r5 = x.im
    ldmia   r3!, {r6, r7, r8}
    add     lr, r4, r5
    smull   r10, r11, r6, lr
    smull   r6, r12, r7, r4
    movs    r10, r10, lsr #28
    adc     lr, r10, r11, lsl #4    @ lr = tmp
    smull   r10, r11, r8, r5
    movs    r6, r6, lsr #28
    adc     r4, r6, r12, lsl #4
    add     r4, r4, lr              @ r4 = imag[i]
    movs    r10, r10, lsr #28
    adc     r5, r10, r11, lsl #4
    add     r5, r5, lr              @ r5 = real[i]
    str     r4, [r1], #4
    str     r5, [r0], #4
.endm

    .global dct4_post_arm
    .type   dct4_post_arm, %function
dct4_post_arm:
    stmfd   sp!, {r4-r11, lr}
    ldmia   r2, {r4, r5}
    str     r4, [r0], #4            @ real[0]
    str     r5, [r1], #4            @ imag[0]
    add     r3, r3, #3*4            @ tab + 3
    add     r9, r2, #17*8           @ r9 = &xc[17], where i = 15 stops
    add     r2, r2, #32*8           @ r2 = &xc[32]
1:
    post_butterfly                  @ i = 1..15
    cmp     r2, r9
    bne     1b

    ldmdb   r2!, {r4, r5}           @ i = 16: xc[16]
    ldmia   r3!, {r6, r7, r8}       @ r6 = sqrt(1/2)
    sub     r7, r5, r4
    add     r8, r4, r5
    smull   r10, r11, r6, r7
    smull   r4, r12, r6, r8
    movs    r10, r10, lsr #28
    adc     r10, r10, r11, lsl #4
    movs    r4, r4, lsr #28
    adc     r4, r4, r12, lsl #4
    str     r10, [r1], #4           @ imag[16]
    str     r4, [r0], #4            @ real[16]

    sub     r9, r2, #15*8           @ r9 = &xc[1], where i = 31 stops
2:
    post_butterfly                  @ i = 17..31
    cmp     r2, r9
    bne     2b
    ldmpc   regs=r4-r11
    .size   dct4_post_arm, .-dct4_post_arm
