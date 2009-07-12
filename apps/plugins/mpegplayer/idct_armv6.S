/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Jens Arnold
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


    .global     mpeg2_idct_copy
    .type       mpeg2_idct_copy, %function
    .global     mpeg2_idct_add
    .type       mpeg2_idct_add, %function

/* Custom calling convention:
 * r0 contains block pointer and is non-volatile
 * all non-volatile c context saved and restored on its behalf
 */
.idct:
    str     lr, [sp, #-4]!      @ lr is used
    add     r1, r0, #128        @ secondary, transposed temp buffer
    mov     r14, #8             @ loop counter

.row_loop:
    ldmia   r0!, {r2, r3, r10, r11} @ fetch f0, f2, f4, f6, f1, f3, f5, f7
    ldrd    r4, L_W1357         @ load  W1, W3, W5, W7

    smuad   r6, r4, r10         @ b0 = W1 * f1 + W3 * f3
    smlad   r6, r5, r11, r6     @    + W5 * f5 + W7 * f7

    smultt  r7, r5, r10         @ b1 = -W7 * f3
    smlabb  r7, r4, r11, r7     @    + -W1 * f5
    smlabt  r7, r5, r11, r7     @    + -W5 * f7
    rsb     r7, r7, #0
    smlatb  r7, r4, r10, r7     @    + W3 * f1

    smulbt  r8, r4, r10         @ b2 = -W1 * f3
    rsb     r8, r8, #0
    smlabb  r8, r5, r10, r8     @    + W5 * f1
    smlatb  r8, r5, r11, r8     @    + W7 * f5
    smlatt  r8, r4, r11, r8     @    + W3 * f7

    smusdx  r9, r10, r5         @ b3 = f1 * W7 - f3 * W5
    smlsdx  r9, r11, r4, r9     @    + f5 * W3 - f1 * W1

    ldrd    r4, L_W0246         @ load  W0, W2, W4, W6
    add     r2, r2, #1          @ f0 += 1

    smulbb  r10, r4, r2         @ a0' = W0 * f0
    smlabb  r10, r5, r3, r10    @     + W4 * f4
    smultt  r12, r4, r2         @ a3' = W2 * f2
    smlatt  r12, r5, r3, r12    @     + W6 * f6
    add     r10, r10, r12       @ a0  = a0' + a3'
    sub     r12, r10, r12, lsl #1  @ a3  = a0 - 2 * a3'

    smulbb  r11, r5, r3         @ a1' = -W4 * f4
    rsb     r11, r11, #0
    smlabb  r11, r4, r2, r11    @     + W0 * f0
    smultt  r3, r4, r3          @ a2' = -W2 * f6
    rsb     r3, r3, #0
    smlatt  r3, r5, r2, r3      @     + W6 * f2
    add     r11, r11, r3        @ a1  = a1' + a2'
    sub     r3, r11, r3, lsl #1 @ a2  = a1 - 2 * a2'
    
    sub     r2, r10, r6         @ block[7] = (a0 - b0)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #7*16]
    sub     r2, r11, r7         @ block[6] = (a1 - b1)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #6*16]
    sub     r2, r3, r8          @ block[5] = (a2 - b2)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #5*16]
    sub     r2, r12, r9         @ block[4] = (a3 - b3)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #4*16]
    add     r2, r12, r9         @ block[3] = (a3 + b3)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #3*16]
    add     r2, r3, r8          @ block[2] = (a2 + b2)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #2*16]
    add     r2, r11, r7         @ block[1] = (a1 + b1)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1, #1*16]
    add     r2, r10, r6         @ block[0] = (a0 + b0)
    mov     r2, r2, asr #12     @            >> 12
    strh    r2, [r1], #2        @ advance to next temp column
    
    subs    r14, r14, #1
    bne     .row_loop
    b       .col_start

    @placed here because of ldrd's offset limit
L_W1357:
    .short  2841
    .short  2408
    .short  1609
    .short   565

L_W0246:
    .short  2048
    .short  2676
    .short  2048
    .short  1108

.col_start:
    @ r0 now points to the temp buffer, where we need it.
    sub     r1, r1, #128+16     @ point r1 back to the input block
    mov     r14, #8             @ loop counter

.col_loop:
    ldmia   r0!, {r2, r3, r10, r11} @ fetch f0, f2, f4, f6, f1, f3, f5, f7
    ldrd    r4, L_W1357         @ load  W1, W3, W5, W7

    smuad   r6, r4, r10         @ b0 = W1 * f1 + W3 * f3
    smlad   r6, r5, r11, r6     @    + W5 * f5 + W7 * f7

    smultt  r7, r5, r10         @ b1 = -W7 * f3
    smlabb  r7, r4, r11, r7     @    + -W1 * f5
    smlabt  r7, r5, r11, r7     @    + -W5 * f7
    rsb     r7, r7, #0
    smlatb  r7, r4, r10, r7     @    + W3 * f1

    smulbt  r8, r4, r10         @ b2 = -W1 * f3
    rsb     r8, r8, #0
    smlabb  r8, r5, r10, r8     @    + W5 * f1
    smlatb  r8, r5, r11, r8     @    + W7 * f5
    smlatt  r8, r4, r11, r8     @    + W3 * f7

    smusdx  r9, r10, r5         @ b3 = f1 * W7 - f3 * W5
    smlsdx  r9, r11, r4, r9     @    + f5 * W3 - f1 * W1

    ldrd    r4, L_W0246         @ load  W0, W2, W4, W6
    add     r2, r2, #32         @ DC offset: 0.5

    smulbb  r10, r4, r2         @ a0' = W0 * f0
    smlabb  r10, r5, r3, r10    @     + W4 * f4
    smultt  r12, r4, r2         @ a3' = W2 * f2
    smlatt  r12, r5, r3, r12    @     + W6 * f6
    add     r10, r10, r12       @ a0  = a0' + a3'
    sub     r12, r10, r12, lsl #1  @ a3  = a0 - 2 * a3'

    smulbb  r11, r5, r3         @ a1' = -W4 * f4
    rsb     r11, r11, #0
    smlabb  r11, r4, r2, r11    @     + W0 * f0
    smultt  r3, r4, r3          @ a2' = -W2 * f6
    rsb     r3, r3, #0
    smlatt  r3, r5, r2, r3      @     + W6 * f2
    add     r11, r11, r3        @ a1  = a1' + a2'
    sub     r3, r11, r3, lsl #1 @ a2  = a1 - 2 * a2'
    
    sub     r2, r10, r6         @ block[7] = (a0 - b0)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #7*16]
    sub     r2, r11, r7         @ block[6] = (a1 - b1)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #6*16]
    sub     r2, r3, r8          @ block[5] = (a2 - b2)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #5*16]
    sub     r2, r12, r9         @ block[4] = (a3 - b3)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #4*16]
    add     r2, r12, r9         @ block[3] = (a3 + b3)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #3*16]
    add     r2, r3, r8          @ block[2] = (a2 + b2)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #2*16]
    add     r2, r11, r7         @ block[1] = (a1 + b1)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1, #1*16]
    add     r2, r10, r6         @ block[0] = (a0 + b0)
    mov     r2, r2, asr #17     @            >> 17
    strh    r2, [r1], #2        @ advance to next column

    subs    r14, r14, #1
    bne     .col_loop

    sub     r0, r0, #256        @ point r0 back to the input block
    ldr     pc, [sp], #4


mpeg2_idct_copy:
    stmfd  sp!, {r1-r2, r4-r11, lr}
    bl     .idct
    ldmfd  sp!, {r1-r2}

    add    r12, r0, #128
    ldrd   r4, [r0]
    mov    r8, #0
    mov    r9, #0
    mov    r10, #0
    mov    r11, #0
1:
    ldrd   r6, [r0, #8]
    usat16 r4, #8, r4
    strb   r4, [r1, #0]
    mov    r4, r4, lsr #16
    strb   r4, [r1, #1]
    usat16 r5, #8, r5
    strb   r5, [r1, #2]
    mov    r5, r5, lsr #16
    strb   r5, [r1, #3]
    ldrd   r4, [r0, #16]
    usat16 r6, #8, r6
    strb   r6, [r1, #4]
    mov    r6, r6, lsr #16
    strb   r6, [r1, #5]
    usat16 r7, #8, r7
    strb   r7, [r1, #6]
    mov    r7, r7, lsr #16
    strb   r7, [r1, #7]
    stmia  r0!, {r8-r11}
    add    r1, r1, r2
    cmp    r0, r12
    blo    1b

    ldmfd  sp!, {r4-r11, pc}

mpeg2_idct_add:
    cmp    r0, #129
    mov    r0, r1
    ldreqsh r1, [r0, #0]
    bne    1f
    and    r1, r1, #0x70
    cmp    r1, #0x40
    bne    3f
1:
    stmfd  sp!, {r2-r11, lr}
    bl     .idct
    ldmfd  sp!, {r1-r2}
    mov    r11, #0
    add    r12, r0, #128
2:
    ldmia  r0, {r3-r6}
    ldrb   r7, [r1, #0]
    ldrb   r8, [r1, #1]
    ldrb   r9, [r1, #2]
    ldrb   r10, [r1, #3]
    str    r11, [r0], #4        
    orr    r7, r7, r8, lsl #16
    sadd16 r3, r3, r7
    usat16 r3, #8, r3
    strb   r3, [r1, #0]
    mov    r3, r3, lsr #16
    strb   r3, [r1, #1]
    str    r11, [r0], #4     
    orr    r9, r9, r10, lsl #16
    sadd16 r4, r4, r9
    usat16 r4, #8, r4
    strb   r4, [r1, #2]
    mov    r4, r4, lsr #16
    strb   r4, [r1, #3]
    ldrb   r7, [r1, #4]
    ldrb   r8, [r1, #5]
    ldrb   r9, [r1, #6]
    ldrb   r10, [r1, #7]
    str    r11, [r0], #4
    orr    r7, r7, r8, lsl #16
    sadd16 r5, r5, r7
    usat16 r5, #8, r5
    strb   r5, [r1, #4]
    mov    r5, r5, lsr #16
    strb   r5, [r1, #5]
    str    r11, [r0], #4
    orr    r9, r9, r10, lsl #16
    sadd16 r6, r6, r9
    usat16 r6, #8, r6
    strb   r6, [r1, #6]
    mov    r6, r6, lsr #16
    strb   r6, [r1, #7]
    add    r1, r1, r2
    cmp    r0, r12
    blo    2b
    ldmfd  sp!, {r4-r11, pc}

3:
    stmfd  sp!, {r4-r5, lr}
    ldrsh  r1, [r0, #0]           /* r1 = block[0] */
    mov    r4, #0
    strh   r4, [r0, #0]           /* block[0] = 0 */
    strh   r4, [r0, #126]         /* block[63] = 0 */
    add    r1, r1, #64            /* r1 = DC << 7 */
    add    r0, r2, r3, asl #3
4:
    ldrb   r4, [r2, #0]
    ldrb   r5, [r2, #1]
    ldrb   r12, [r2, #2]
    ldrb   lr, [r2, #3]
    add    r4, r4, r1, asr #7
    usat   r4, #8, r4
    strb   r4, [r2, #0]
    add    r5, r5, r1, asr #7
    usat   r5, #8, r5
    strb   r5, [r2, #1]
    add    r12, r12, r1, asr #7
    usat   r12, #8, r12
    strb   r12, [r2, #2]
    add    lr, lr, r1, asr #7
    usat   lr, #8, lr
    strb   lr, [r2, #3]
    ldrb   r4, [r2, #4]
    ldrb   r5, [r2, #5]
    ldrb   r12, [r2, #6]
    ldrb   lr, [r2, #7]
    add    r4, r4, r1, asr #7
    usat   r4, #8, r4
    strb   r4, [r2, #4]
    add    r5, r5, r1, asr #7
    usat   r5, #8, r5
    strb   r5, [r2, #5]
    add    r12, r12, r1, asr #7
    usat   r12, #8, r12
    strb   r12, [r2, #6]
    add    lr, lr, r1, asr #7
    usat   lr, #8, lr
    strb   lr, [r2, #7]
    add    r2, r2, r3
    cmp    r2, r0
    blo    4b
    ldmfd  sp!, {r4-r5, pc}
