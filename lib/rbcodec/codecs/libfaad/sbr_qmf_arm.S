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

/* The window of sbr_qmf_synthesis_64() in sbr_qmf.c, for ARMv4 and up.
 *
 * void sbr_qmf_window_arm(real_t *out, const real_t *v, int v_index,
 *                         const real_t *c);
 *
 * v is a ring of ten 128-value slots, stored once; the newest slot starts
 * at v[v_index] (a multiple of 128) and older ones follow it, wrapping at
 * the end.  For k in 0..63:
 *
 *     out[k] = (sum over j in 0..9 of v[slot j + 64*(j & 1) + k] * c[10k + j])
 *              >> 32
 *
 * where slot j starts at v_index + 128*j, less 1280 once past the end.
 * There is one copy of the loop for each of the ten values of v_index,
 * so that each load has a constant offset. */

    .syntax unified
    .text
    .align  2

@ .Loff<j>: the byte offset of tap j from v + v_index + k
.macro tap_offset j, b
    .if (\j + \b) >= 10
    .set .Loff\j, (128*\j + 64*(\j & 1) - 1280)*4
    .else
    .set .Loff\j, (128*\j + 64*(\j & 1))*4
    .endif
.endm

@ Offsets out of an ldr's range go in r12 and lr (there are at most two)
.macro window_setup b
    .set .Lnfar, 0
    .irp j, 0,1,2,3,4,5,6,7,8,9
    tap_offset \j, \b
    .if (.Loff\j > 4095) || (.Loff\j < -4095)
    .if .Lnfar == 0
    ldr     r12, =.Loff\j
    .set .Lfar\j, 1
    .else
    ldr     lr, =.Loff\j
    .set .Lfar\j, 2
    .endif
    .set .Lnfar, .Lnfar + 1
    .else
    .set .Lfar\j, 0
    .endif
    .endr
.endm

.macro ldtap rd, j
    .if .Lfar\j == 0
    ldr     \rd, [r1, #.Loff\j]
    .elseif .Lfar\j == 1
    ldr     \rd, [r1, r12]
    .else
    ldr     \rd, [r1, lr]
    .endif
.endm

@ r0 = out, r1 = &v[v_index + k], r2 = &c[10k], r3 = &v[v_index + 64]
.macro window_loop b
.Lwin\b:
    window_setup \b
1:
    ldmia   r2!, {r4-r7}            @ c[10k] .. c[10k+3]
    ldtap   r8, 0
    ldtap   r9, 1
    smull   r10, r11, r8, r4
    ldtap   r8, 2
    smlal   r10, r11, r9, r5
    ldtap   r9, 3
    smlal   r10, r11, r8, r6
    ldtap   r8, 4
    smlal   r10, r11, r9, r7
    ldmia   r2!, {r4-r7}            @ c[10k+4] .. c[10k+7]
    ldtap   r9, 5
    smlal   r10, r11, r8, r4
    ldtap   r8, 6
    smlal   r10, r11, r9, r5
    ldtap   r9, 7
    smlal   r10, r11, r8, r6
    ldtap   r8, 8
    smlal   r10, r11, r9, r7
    ldmia   r2!, {r4-r5}            @ c[10k+8], c[10k+9]
    ldtap   r9, 9
    smlal   r10, r11, r8, r4
    add     r1, r1, #4
    smlal   r10, r11, r9, r5
    str     r11, [r0], #4
    cmp     r1, r3
    bne     1b
    ldmpc   regs=r4-r11
    .ltorg
.endm

    .global sbr_qmf_window_arm
    .type   sbr_qmf_window_arm, %function
sbr_qmf_window_arm:
    stmfd   sp!, {r4-r11, lr}
    add     r1, r1, r2, lsl #2      @ r1 = &v[v_index]
    mov     r12, r2, lsr #7         @ r12 = v_index / 128
    mov     r2, r3                  @ r2 = c
    add     r3, r1, #64*4
    add     pc, pc, r12, lsl #2
    nop
    b       .Lwin0
    b       .Lwin1
    b       .Lwin2
    b       .Lwin3
    b       .Lwin4
    b       .Lwin5
    b       .Lwin6
    b       .Lwin7
    b       .Lwin8
    b       .Lwin9

    window_loop 0
    window_loop 1
    window_loop 2
    window_loop 3
    window_loop 4
    window_loop 5
    window_loop 6
    window_loop 7
    window_loop 8
    window_loop 9
    .size   sbr_qmf_window_arm, .-sbr_qmf_window_arm
