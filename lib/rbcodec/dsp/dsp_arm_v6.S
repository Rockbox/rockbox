/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Michael Sevakis
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

/****************************************************************************
 *  void sample_output_mono(int count, struct dsp_data *data,
 *                          const int32_t *src[], int16_t *dst)
 */
    .section .text, "ax", %progbits
    .align  2
    .global sample_output_mono
    .type   sample_output_mono, %function
sample_output_mono:
    @ input: r0 = count, r1 = data, r2 = src, r3 = dst
    stmfd   sp!, { r4, lr }            @
                                       @
    ldr     r1, [r1]                   @ r1 = data->output_scale
    ldr     r2, [r2]                   @ r2 = src[0]
                                       @
    mov     r4, #1                     @ r4 = 1 << (scale - 1)
    mov     r4, r4, lsl r1             @
    subs    r0, r0, #1                 @ odd: end at 0; even: end at -1
    mov     r4, r4, lsr #1             @
    beq     2f                         @ Zero? Only one sample!
                                       @
1:                                     @
    ldmia   r2!, { r12, r14 }          @ load Mi0, Mi1
    qadd    r12, r12, r4               @ round, scale, saturate and
    qadd    r14, r14, r4               @ pack Mi0 to So0, Mi1 to So1
    mov     r12, r12, asr r1           @
    mov     r14, r14, asr r1           @
    ssat    r12, #16, r12              @
    ssat    r14, #16, r14              @
    pkhbt   r12, r12, r12, asl #16     @
    pkhbt   r14, r14, r14, asl #16     @
    subs    r0, r0, #2                 @
    stmia   r3!, { r12, r14 }          @ store So0, So1
    bgt     1b                         @
                                       @
    ldmltfd sp!, { r4, pc }            @ if count was even, we're done
                                       @
2:                                     @
    ldr     r12, [r2]                  @ round, scale, saturate
    qadd    r12, r12, r4               @ and pack Mi to So
    mov     r12, r12, asr r1           @
    ssat    r12, #16, r12              @
    pkhbt   r12, r12, r12, asl #16     @
    str     r12, [r3]                  @ store So
                                       @       
    ldmfd   sp!, { r4, pc }            @
    .size   sample_output_mono, .-sample_output_mono

/****************************************************************************
 * void sample_output_stereo(int count, struct dsp_data *data,
 *                           const int32_t *src[], int16_t *dst)
 */
    .section .text, "ax", %progbits
    .align  2
    .global sample_output_stereo
    .type   sample_output_stereo, %function
sample_output_stereo:
    @ input: r0 = count, r1 = data, r2 = src, r3 = dst
    stmfd   sp!, { r4-r7, lr }         @
                                       @
    ldr     r1, [r1]                   @ r1 = data->output_scale    
    ldmia   r2, { r2, r4 }             @ r2 = src[0], r4 = src[1]
                                       @
    mov     r5, #1                     @ r5 = 1 << (scale - 1)
    mov     r5, r5, lsl r1             @
    subs    r0, r0, #1                 @ odd: end at 0; even: end at -1
    mov     r5, r5, lsr #1             @
    beq     2f                         @ Zero? Only one sample!
                                       @
1:                                     @
    ldmia   r2!, { r6, r7 }            @ r6, r7 = Li0, Li1
    ldmia   r4!, { r12, r14 }          @ r12, r14 = Ri0, Ri1
    qadd    r6, r6, r5                 @ round, scale, saturate and pack
    qadd    r7, r7, r5                 @ Li0+Ri0 to So0, Li1+Ri1 to So1
    qadd    r12, r12, r5               @
    qadd    r14, r14, r5               @
    mov     r6, r6, asr r1             @ 
    mov     r7, r7, asr r1             @
    mov     r12, r12, asr r1           @
    mov     r14, r14, asr r1           @
    ssat    r6, #16, r6                @
    ssat    r12, #16, r12              @
    ssat    r7, #16, r7                @
    ssat    r14, #16, r14              @
    pkhbt   r6, r6, r12, asl #16       @
    pkhbt   r7, r7, r14, asl #16       @
    subs    r0, r0, #2                 @
    stmia   r3!, { r6, r7 }            @ store So0, So1
    bgt     1b                         @
                                       @
    ldmltfd sp!, { r4-r7, pc }         @ if count was even, we're done
                                       @
2:                                     @
    ldr     r6, [r2]                   @ r6 = Li
    ldr     r12, [r4]                  @ r12 = Ri
    qadd    r6, r6, r5                 @ round, scale, saturate
    qadd    r12, r12, r5               @ and pack Li+Ri to So
    mov     r6, r6, asr r1             @
    mov     r12, r12, asr r1           @
    ssat    r6, #16, r6                @
    ssat    r12, #16, r12              @
    pkhbt   r6, r6, r12, asl #16       @
    str     r6, [r3]                   @ store So
                                       @
    ldmfd   sp!, { r4-r7, pc }         @
    .size   sample_output_stereo, .-sample_output_stereo
