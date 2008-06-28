/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Thom Johansen
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

/* uncomment this to make filtering calculate lower bits after shifting.
 * without this, "shift" of the lower bits will be lost here.
 */
/* #define HIGH_PRECISION */

/*
 * void eq_filter(int32_t **x, struct eqfilter *f, unsigned num,
 *                unsigned channels, unsigned shift)
 */
#if CONFIG_CPU == PP5002
    .section    .icode,"ax",%progbits
#else
    .text
#endif
    .global eq_filter
eq_filter:
    ldr r12, [sp]             @ get shift parameter
    stmdb sp!, { r0-r11, lr } @ save all params and clobbered regs 
    ldmia r1!, { r4-r8 }      @ load coefs
    mov r10, r1               @ loop prelude expects filter struct addr in r10

.filterloop:
    ldr r9, [sp]            @ get pointer to this channels data
    add r0, r9, #4
    str r0, [sp]            @ save back pointer to next channels data
    ldr r9, [r9]            @ r9 = x[]
    ldr r14, [sp, #8]       @ r14 = numsamples
    ldmia r10, { r0-r3 }    @ load history, r10 should be filter struct addr
    str r10, [sp, #4]       @ save it for loop end

    /* r0-r3 = history, r4-r8 = coefs, r9 = x[], r10..r11 = accumulator,
     * r12 = shift amount, r14 = number of samples.
     */
.loop:
    /* Direct form 1 filtering code.
     * y[n] = b0*x[i] + b1*x[i - 1] + b2*x[i - 2] + a1*y[i - 1] + a2*y[i - 2],
     * where y[] is output and x[] is input. This is performed out of order to
     * reuse registers, we're pretty short on regs.
     */
    smull r10, r11, r6, r1     @ acc = b2*x[i - 2]
    mov r1, r0                 @ fix input history
    smlal r10, r11, r5, r0     @ acc += b1*x[i - 1]
    ldr r0, [r9]               @ load input and fix history in same operation
    smlal r10, r11, r4, r0     @ acc += b0*x[i]
    smlal r10, r11, r7, r2     @ acc += a1*y[i - 1]
    smlal r10, r11, r8, r3     @ acc += a2*y[i - 2]
    mov r3, r2                 @ fix output history
    mov r2, r11, asl r12       @ get upper part of result and shift left
#ifdef HIGH_PRECISION
    rsb r11, r12, #32          @ get shift amount for lower part
    orr r2, r2, r10, lsr r11   @ then mix in correctly shifted lower part
#endif
    str r2, [r9], #4           @ save result
    subs r14, r14, #1          @ are we done with this channel?
    bne .loop

    ldr r10, [sp, #4]          @ load filter struct pointer
    stmia r10!, { r0-r3 }      @ save back history
    ldr r11, [sp, #12]         @ load number of channels
    subs r11, r11, #1          @ all channels processed?
    strne r11, [sp, #12]
    bne .filterloop

    add sp, sp, #16            @ compensate for temp storage
    ldmia sp!, { r4-r11, pc }

