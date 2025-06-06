/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by David Bryant
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

/* This is an assembly optimized version of the following WavPack function:
 *
 * void decorr_stereo_pass_cont_arml (struct decorr_pass *dpp,
 *                                    long *buffer, long sample_count);
 *
 * It performs a single pass of stereo decorrelation on the provided buffer.
 * Note that this version of the function requires that the 8 previous stereo
 * samples are visible and correct. In other words, it ignores the "samples_*"
 * fields in the decorr_pass structure and gets the history data directly
 * from the buffer. It does, however, return the appropriate history samples
 * to the decorr_pass structure before returning.
 *
 * This is written to work on a ARM7TDMI processor. This version uses the
 * 64-bit multiply-accumulate instruction and so can be used with all
 * WavPack files. However, for optimum performance with 16-bit WavPack
 * files, there is a faster version that only uses the 32-bit MLA
 * instruction.
 */

#include "config.h"

        .syntax         unified
        .text
        .align
        .global         decorr_stereo_pass_cont_arml

/*
 * on entry:
 *
 * r0 = struct decorr_pass *dpp
 * r1 = long *buffer
 * r2 = long sample_count
 */

decorr_stereo_pass_cont_arml:

        stmfd   sp!, {r4 - r8, r10, r11, lr}
        mov     r5, r0                  @ r5 = dpp
        mov     r11, #512               @ r11 = 512 for rounding
        ldrsh   r6, [r0, #2]            @ r6 = dpp->delta
        ldrsh   r4, [r0, #4]            @ r4 = dpp->weight_A
        ldrsh   r0, [r0, #6]            @ r0 = dpp->weight_B
        cmp     r2, #0                  @ exit if no samples to process
        beq     common_exit

        mov     r0, r0, asl #18         @ for 64-bit math we use weights << 18
        mov     r4, r4, asl #18
        mov     r6, r6, asl #18
        add     r7, r1, r2, asl #3      @ r7 = buffer ending position
        ldrsh   r2, [r5, #0]            @ r2 = dpp->term
        cmp     r2, #0
        blt     minus_term

        ldr     lr, [r1, #-16]          @ load 2 sample history from buffer
        ldr     r10, [r1, #-12]         @  for terms 2, 17, and 18
        ldr     r8, [r1, #-8]
        ldr     r3, [r1, #-4]

        cmp     r2, #18
        beq     term_18_loop
        mov     lr, lr, asl #4
        mov     r10, r10, asl #4
        cmp     r2, #2
        beq     term_2_loop
        cmp     r2, #17
        beq     term_17_loop
        b       term_default_loop

minus_term:
        mov     r10, #(1024 << 18)      @ r10 = -1024 << 18 for weight clipping
        rsb     r10, r10, #0            @  (only used for negative terms)
        cmn     r2, #1
        beq     term_minus_1
        cmn     r2, #2
        beq     term_minus_2
        cmn     r2, #3
        beq     term_minus_3
        b       common_exit

/*
 ******************************************************************************
 * Loop to handle term = 17 condition
 *
 * r0 = dpp->weight_B           r8 = previous left sample
 * r1 = bptr                    r9 = 
 * r2 = current sample          r10 = second previous left sample << 4
 * r3 = previous right sample   r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = current decorrelation value
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr = second previous right sample << 4
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_17_loop:
        rsbs    ip, lr, r8, asl #5      @ decorr value = (2 * prev) - 2nd prev
        mov     lr, r8, asl #4          @ previous becomes 2nd previous
        ldr     r2, [r1], #4            @ get sample & update pointer
        mov     r11, #0x80000000
        mov     r8, r2
        smlalne r11, r8, r4, ip
        strne   r8, [r1, #-4]           @ if change possible, store sample back
        cmpne   r2, #0
        beq     .L325
        teq     ip, r2                  @ update weight based on signs
        submi   r4, r4, r6
        addpl   r4, r4, r6

.L325:  rsbs    ip, r10, r3, asl #5     @ do same thing for right channel
        mov     r10, r3, asl #4
        ldr     r2, [r1], #4
        mov     r11, #0x80000000
        mov     r3, r2
        smlalne r11, r3, r0, ip
        strne   r3, [r1, #-4]
        cmpne   r2, #0
        beq     .L329
        teq     ip, r2
        submi   r0, r0, r6
        addpl   r0, r0, r6

.L329:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_17_loop
        mov     lr, lr, asr #4
        mov     r10, r10, asr #4
        b       store_1718              @ common exit for terms 17 & 18

/*
 ******************************************************************************
 * Loop to handle term = 18 condition
 *
 * r0 = dpp->weight_B           r8 = previous left sample
 * r1 = bptr                    r9 = 
 * r2 = current sample          r10 = second previous left sample
 * r3 = previous right sample   r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = decorrelation value
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr = second previous right sample
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_18_loop:
        rsb     ip, lr, r8              @ decorr value =
        mov     lr, r8                  @  ((3 * prev) - 2nd prev) >> 1
        add     ip, lr, ip, asr #1
        movs    ip, ip, asl #4
        ldr     r2, [r1], #4            @ get sample & update pointer
        mov     r11, #0x80000000
        mov     r8, r2
        smlalne r11, r8, r4, ip
        strne   r8, [r1, #-4]           @ if change possible, store sample back
        cmpne   r2, #0
        beq     .L337
        teq     ip, r2                  @ update weight based on signs
        submi   r4, r4, r6
        addpl   r4, r4, r6

.L337:  rsb     ip, r10, r3             @ do same thing for right channel
        mov     r10, r3
        add     ip, r10, ip, asr #1
        movs    ip, ip, asl #4
        ldr     r2, [r1], #4
        mov     r11, #0x80000000
        mov     r3, r2
        smlalne r11, r3, r0, ip
        strne   r3, [r1, #-4]
        cmpne   r2, #0
        beq     .L341
        teq     ip, r2
        submi   r0, r0, r6
        addpl   r0, r0, r6

.L341:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_18_loop

/* common exit for terms 17 & 18 */

store_1718:
        str     r3, [r5, #40]           @ store sample history into struct
        str     r8, [r5, #8]
        str     r10, [r5, #44]
        str     lr, [r5, #12]
        b       common_exit             @ and return

/*
 ******************************************************************************
 * Loop to handle term = 2 condition
 * (note that this case can be handled by the default term handler (1-8), but
 * this special case is faster because it doesn't have to read memory twice)
 *
 * r0 = dpp->weight_B           r8 = previous left sample
 * r1 = bptr                    r9 = 
 * r2 = current sample          r10 = second previous left sample << 4
 * r3 = previous right sample   r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = decorrelation value
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr = second previous right sample << 4
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_2_loop:
        movs    ip, lr                  @ get decorrelation value & test
        ldr     r2, [r1], #4            @ get sample & update pointer
        mov     lr, r8, asl #4          @ previous becomes 2nd previous
        mov     r11, #0x80000000
        mov     r8, r2
        smlalne r11, r8, r4, ip
        strne   r8, [r1, #-4]           @ if change possible, store sample back
        cmpne   r2, #0
        beq     .L225
        teq     ip, r2                  @ update weight based on signs
        submi   r4, r4, r6
        addpl   r4, r4, r6

.L225:  movs    ip, r10                 @ do same thing for right channel
        ldr     r2, [r1], #4
        mov     r10, r3, asl #4
        mov     r11, #0x80000000
        mov     r3, r2
        smlalne r11, r3, r0, ip
        strne   r3, [r1, #-4]
        cmpne   r2, #0
        beq     .L229
        teq     ip, r2
        submi   r0, r0, r6
        addpl   r0, r0, r6

.L229:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_2_loop

        b       default_term_exit       @ this exit updates all dpp->samples

/*
 ******************************************************************************
 * Loop to handle default term condition
 *
 * r0 = dpp->weight_B           r8 = result accumulator
 * r1 = bptr                    r9 = 
 * r2 = dpp->term               r10 =
 * r3 = decorrelation value     r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = current sample
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr =
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_default_loop:
        ldr     r3, [r1, -r2, asl #3]   @ get decorrelation value based on term
        ldr     ip, [r1], #4            @ get original sample and bump ptr
        movs    r3, r3, asl #4
        mov     r11, #0x80000000
        mov     r8, ip
        smlalne r11, r8, r4, r3
        strne   r8, [r1, #-4]           @ if possibly changed, store updated sample
        cmpne   ip, #0
        beq     .L350
        teq     ip, r3                  @ update weight based on signs
        submi   r4, r4, r6
        addpl   r4, r4, r6

.L350:  ldr     r3, [r1, -r2, asl #3]   @ do the same thing for right channel
        ldr     ip, [r1], #4
        movs    r3, r3, asl #4
        mov     r11, #0x80000000
        mov     r8, ip
        smlalne r11, r8, r0, r3
        strne   r8, [r1, #-4]
        cmpne   ip, #0
        beq     .L354
        teq     ip, r3
        submi   r0, r0, r6
        addpl   r0, r0, r6

.L354:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_default_loop

/*
 * This exit is used by terms 1-8 to store the previous 8 samples into the decorr
 * structure (even if they are not all used for the given term)
 */

default_term_exit:
        ldrsh   r3, [r5, #0]
        sub     ip, r3, #1
        mov     lr, #7

.L358:  and     r3, ip, #7
        add     r3, r5, r3, asl #2
        ldr     r2, [r1, #-4]
        str     r2, [r3, #40]
        ldr     r2, [r1, #-8]!
        str     r2, [r3, #8]
        sub     ip, ip, #1
        sub     lr, lr, #1
        cmn     lr, #1
        bne     .L358
        b       common_exit

/*
 ******************************************************************************
 * Loop to handle term = -1 condition
 *
 * r0 = dpp->weight_B           r8 =
 * r1 = bptr                    r9 = 
 * r2 = intermediate result     r10 = -1024 (for clipping)
 * r3 = previous right sample   r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = current sample
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr = updated left sample
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_minus_1:
        ldr     r3, [r1, #-4]

term_minus_1_loop:
        ldr     ip, [r1], #8            @ for left channel the decorrelation value
        movs    r3, r3, asl #4          @  is the previous right sample (in r3)
        mov     r11, #0x80000000
        mov     lr, ip
        smlalne r11, lr, r4, r3
        strne   lr, [r1, #-8]
        cmpne   ip, #0
        beq     .L361
        teq     ip, r3                  @ update weight based on signs
        submi   r4, r4, r6
        addpl   r4, r4, r6
        cmp     r4, #(1024 << 18)
        movgt   r4, #(1024 << 18)
        cmp     r4, r10
        movlt   r4, r10

.L361:  ldr     r2, [r1, #-4]           @ for right channel the decorrelation value
        movs    lr, lr, asl #4
        mov     r11, #0x80000000
        mov     r3, r2
        smlalne r11, r3, r0, lr
        strne   r3, [r1, #-4]
        cmpne   r2, #0
        beq     .L369
        teq     r2, lr
        submi   r0, r0, r6
        addpl   r0, r0, r6
        cmp     r0, #(1024 << 18)               @ then clip weight to +/-1024
        movgt   r0, #(1024 << 18)
        cmp     r0, r10
        movlt   r0, r10

.L369:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_minus_1_loop

        str     r3, [r5, #8]            @ else store right sample and exit
        b       common_exit

/*
 ******************************************************************************
 * Loop to handle term = -2 condition
 * (note that the channels are processed in the reverse order here)
 *
 * r0 = dpp->weight_B           r8 =
 * r1 = bptr                    r9 = 
 * r2 = intermediate result     r10 = -1024 (for clipping)
 * r3 = previous left sample    r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = current sample
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr = updated right sample
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_minus_2:
        ldr     r3, [r1, #-8]

term_minus_2_loop:
        ldr     ip, [r1, #4]            @ for right channel the decorrelation value
        movs    r3, r3, asl #4          @  is the previous left sample (in r3)
        mov     r11, #0x80000000
        mov     lr, ip
        smlalne r11, lr, r0, r3
        strne   lr, [r1, #4]
        cmpne   ip, #0
        beq     .L380
        teq     ip, r3                  @ update weight based on signs
        submi   r0, r0, r6
        addpl   r0, r0, r6
        cmp     r0, #(1024 << 18)               @ then clip weight to +/-1024
        movgt   r0, #(1024 << 18)
        cmp     r0, r10
        movlt   r0, r10

.L380:  ldr     r2, [r1], #8            @ for left channel the decorrelation value
        movs    lr, lr, asl #4
        mov     r11, #0x80000000
        mov     r3, r2
        smlalne r11, r3, r4, lr
        strne   r3, [r1, #-8]
        cmpne   r2, #0
        beq     .L388
        teq     r2, lr
        submi   r4, r4, r6
        addpl   r4, r4, r6
        cmp     r4, #(1024 << 18)
        movgt   r4, #(1024 << 18)
        cmp     r4, r10
        movlt   r4, r10

.L388:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_minus_2_loop

        str     r3, [r5, #40]           @ else store left channel and exit
        b       common_exit

/*
 ******************************************************************************
 * Loop to handle term = -3 condition
 *
 * r0 = dpp->weight_B           r8 = previous left sample
 * r1 = bptr                    r9 = 
 * r2 = current left sample     r10 = -1024 (for clipping)
 * r3 = previous right sample   r11 = lo accumulator (for rounding)
 * r4 = dpp->weight_A           ip = intermediate result
 * r5 = dpp                     sp =
 * r6 = dpp->delta              lr =
 * r7 = eptr                    pc =
 *******************************************************************************
 */

term_minus_3:
        ldr     r3, [r1, #-4]           @ load previous samples
        ldr     r8, [r1, #-8]

term_minus_3_loop:
        ldr     ip, [r1], #4
        movs    r3, r3, asl #4
        mov     r11, #0x80000000
        mov     r2, ip
        smlalne r11, r2, r4, r3
        strne   r2, [r1, #-4]
        cmpne   ip, #0
        beq     .L399
        teq     ip, r3                  @ update weight based on signs
        submi   r4, r4, r6
        addpl   r4, r4, r6
        cmp     r4, #(1024 << 18)       @ then clip weight to +/-1024
        movgt   r4, #(1024 << 18)
        cmp     r4, r10
        movlt   r4, r10

.L399:  movs    ip, r8, asl #4          @ ip = previous left we use now
        mov     r8, r2                  @ r8 = current left we use next time
        ldr     r2, [r1], #4
        mov     r11, #0x80000000
        mov     r3, r2
        smlalne r11, r3, r0, ip
        strne   r3, [r1, #-4]
        cmpne   r2, #0
        beq     .L407
        teq     ip, r2
        submi   r0, r0, r6
        addpl   r0, r0, r6
        cmp     r0, #(1024 << 18)
        movgt   r0, #(1024 << 18)
        cmp     r0, r10
        movlt   r0, r10

.L407:  cmp     r7, r1                  @ loop back if more samples to do
        bhi     term_minus_3_loop

        str     r3, [r5, #8]            @ else store previous samples & exit
        str     r8, [r5, #40]

/*
 * Before finally exiting we must store weights back for next time
 */

common_exit:
        mov     r0, r0, asr #18         @ restore weights to real magnitude
        mov     r4, r4, asr #18
        strh    r4, [r5, #4]
        strh    r0, [r5, #6]
        ldmpc   regs="r4-r8, r10-r11"

