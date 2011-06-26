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

#include "config.h"
#include "mad_iram.h"

    .section    ICODE_SECTION_MPA_ARM,"ax",%progbits

    .global synth_full_odd_sbsample
    .global synth_full_even_sbsample

    /*
    ;; r0 = pcm (pushed on the stack to free a register)
    ;; r1 = fo
    ;; r2 = fe
    ;; r3 = D0ptr
    ;; r4 = D1ptr    

    ;; r5 = loop counter
    ;; r6,r7 accumulator1
    ;; r8,r9 accumulator2
    */
     
synth_full_odd_sbsample:
    stmdb   sp!, {r0, r4-r11, lr}
    ldr     r4, [sp, #40]
    mov     r5, #15
    add     r2, r2, #32
.l:
    /* ;; PROD_O and odd half of SB_SAMPLE*/
    add     r3, r3, #128
    add     r4, r4, #128
    ldr     r7, [r3, #4]
    ldmia   r1!, {r0, r10, r11, lr}
    ldr     r9, [r4, #120]
    smull   r6, r7, r0, r7
    ldr     r12, [r3, #60]
    smull   r8, r9, r0, r9
    ldr     r0, [r3, #52]
    smlal   r6, r7, r10, r12
    ldr     r12, [r3, #44]
    smlal   r6, r7, r11, r0
    ldr     r0, [r4, #64]
    smlal   r6, r7, lr, r12
    ldr     r12, [r4, #72]
    smlal   r8, r9, r10, r0
    ldr     r0, [r4, #80]
    smlal   r8, r9, r11, r12
    smlal   r8, r9, lr, r0
    ldr     r0, [r3, #36]   
    
    ldmia   r1!, {r10, r11, r12, lr}
    smlal   r6, r7, r10, r0

    ldr     r0, [r4, #88]  /*;;1 cycle stall on arm9, but we free up r10*/
    smlal   r8, r9, r10, r0
    
    ldr     r0, [r3, #28]
    ldr     r10, [r3, #20]
    smlal   r6, r7, r11, r0
    ldr     r0, [r3, #12]
    smlal   r6, r7, r12, r10
    ldr     r10, [r4, #96]
    smlal   r6, r7, lr, r0
    ldr     r0, [r4, #104]
    smlal   r8, r9, r11, r10
    ldr     r10, [r4, #112]    
    smlal   r8, r9, r12, r0
    smlal   r8, r9, lr, r10
    
    rsbs    r6, r6, #0
    rsc     r7, r7, #0
    
    /* ;; PROD_A and even half of SB_SAMPLE*/
    ldr     r0, [r3, #0]
    ldmia   r2!, {r10, r11, r12, lr}
    smlal   r6, r7, r10, r0
    
    ldr     r0, [r4, #60]  /*;;1 cycle stall on arm9, but we free up r10*/
    smlal   r8, r9, r10, r0    
    ldr     r10, [r3, #56]
    ldr     r0, [r3, #48]
    smlal   r6, r7, r11, r10
    ldr     r10, [r3, #40]
    smlal   r6, r7, r12, r0
    ldr     r0, [r4, #68]
    smlal   r6, r7, lr, r10
    ldr     r10, [r4, #76]
    smlal   r8, r9, r11, r0
    ldr     r0, [r4, #84]    
    smlal   r8, r9, r12, r10
    smlal   r8, r9, lr, r0
    
    ldr     r0, [r3, #32]
    ldmia   r2!, {r10, r11, r12, lr}
    smlal   r6, r7, r10, r0

    ldr     r0, [r4, #92]  /*;;1 cycle stall on arm9, but we free up r10*/
    smlal   r8, r9, r10, r0
    
    ldr     r0, [r3, #24]
    ldr     r10, [r3, #16]
    smlal   r6, r7, r11, r0
    ldr     r0, [r3, #8]
    smlal   r6, r7, r12, r10
    ldr     r10, [r4, #100]
    smlal   r6, r7, lr, r0
    ldr     r0, [r4, #108]
    smlal   r8, r9, r11, r10
    ldr     r10, [r4, #116]
    smlal   r8, r9, r12, r0
    smlal   r8, r9, lr, r10

    ldr     r0, [sp]

    movs    r6, r6, lsr #16
    adc     r6, r6, r7, lsl #16
    str     r6, [r0, -r5, lsl #2]

    movs    r8, r8, lsr #16
    adc     r8, r8, r9, lsl #16
    str     r8, [r0, r5, lsl #2]
        
    subs    r5, r5, #1
    bne     .l

    ldmpc   regs="r0,r4-r11"

synth_full_even_sbsample:
    stmdb   sp!, {r0, r4-r11, lr}
    ldr     r4, [sp, #40]
    mov     r5, #15
    add     r2, r2, #32
.l2:
 /* ;; PROD_O and odd half of SB_SAMPLE*/
    add     r3, r3, #128
    add     r4, r4, #128
    ldr     r7, [r3, #0]
    ldmia   r1!, {r0, r10, r11, lr}
    ldr     r9, [r4, #60]
    smull   r6, r7, r0, r7
    ldr     r12, [r3, #56]
    smull   r8, r9, r0, r9
    ldr     r0, [r3, #48]
    smlal   r6, r7, r10, r12
    ldr     r12, [r3, #40]
    smlal   r6, r7, r11, r0
    ldr     r0, [r4, #68]
    smlal   r6, r7, lr, r12

    ldr     r12, [r4, #76]
    smlal   r8, r9, r10, r0
    ldr     r0, [r4, #84]
    smlal   r8, r9, r11, r12
    smlal   r8, r9, lr, r0
    
    ldr     r0, [r3, #32]
    ldmia   r1!, {r10, r11, r12, lr}

    smlal   r6, r7, r10, r0
    ldr     r0, [r4, #92]  
    smlal   r8, r9, r10, r0
    ldr     r0, [r3, #24]
    ldr     r10, [r3, #16]
    smlal   r6, r7, r11, r0
    ldr     r0, [r3, #8]
    smlal   r6, r7, r12, r10
    ldr     r10, [r4, #100]
    smlal   r6, r7, lr, r0
    ldr     r0, [r4, #108]
    smlal   r8, r9, r11, r10
    ldr     r10, [r4, #116]
    smlal   r8, r9, r12, r0
    smlal   r8, r9, lr, r10
    
    rsbs    r6, r6, #0
    rsc     r7, r7, #0

    ldr     r0, [r3, #4]
    ldmia   r2!, {r10, r11, r12, lr}
    smlal   r6, r7, r10, r0
    ldr     r0, [r4, #120] /*;;1 cycle stall on arm9, but we free up r10*/
    smlal   r8, r9, r10, r0
    ldr     r0, [r3, #60]
    ldr     r10, [r3, #52]
    smlal   r6, r7, r11, r0
    ldr     r0, [r3, #44]    
    smlal   r6, r7, r12, r10
    ldr     r10, [r4, #64]
    smlal   r6, r7, lr, r0

    ldr     r0, [r4, #72]
    smlal   r8, r9, r11, r10
    ldr     r10, [r4, #80]
    smlal   r8, r9, r12, r0

    smlal   r8, r9, lr, r10

    ldr     r0, [r3, #36]
    ldmia   r2!, {r10, r11, r12, lr}
    smlal   r6, r7, r10, r0
    ldr     r0, [r4, #88]  /*;;1 cycle stall on arm9, but we free up r10*/    
    smlal   r8, r9, r10, r0
    
    ldr     r0, [r3, #28]
    ldr     r10, [r3, #20]
    smlal   r6, r7, r11, r0
    ldr     r0, [r3, #12]
    smlal   r6, r7, r12, r10
    ldr     r10, [r4, #96]
    smlal   r6, r7, lr, r0
    ldr     r0, [r4, #104]
    smlal   r8, r9, r11, r10
    ldr     r10, [r4, #112]
    smlal   r8, r9, r12, r0
    smlal   r8, r9, lr, r10

    ldr     r0, [sp]

    movs    r6, r6, lsr #16
    adc     r6, r6, r7, lsl #16
    str     r6, [r0, -r5, lsl #2]

    movs    r8, r8, lsr #16
    adc     r8, r8, r9, lsl #16
    str     r8, [r0, r5, lsl #2]
        
    subs    r5, r5, #1
    bne     .l2

    ldmpc   regs="r0,r4-r11"

    .global III_aliasreduce

III_aliasreduce:
    stmdb   sp!, {r4-r11, lr}
    add     r1, r0, r1, lsl #2
    add     r0, r0, #72
.arl1:
    mov     r2, #8
    mov     r3, r0              @ a
    mov     r4, r0              @ b
    ldr     r5, =csa            @ cs/ca
.arl2:
    ldmdb   r3, {r6, r12}
    ldmia   r4, {r7, lr}

    ldmia   r5!, {r8, r9}
    smull   r10, r11, r7, r8
    smlal   r10, r11, r12, r9
    movs    r10, r10, lsr #28
    adc     r10, r10, r11, lsl #4

    rsb     r7, r7, #0
    smull   r11, r8, r12, r8
    smlal   r11, r8, r7, r9
    movs    r11, r11, lsr #28
    adc     r11, r11, r8, lsl #4
    
    ldmia   r5!, {r8, r9}
    smull   r12, r7, lr, r8
    smlal   r12, r7, r6, r9
    movs    r12, r12, lsr #28
    adc     r12, r12, r7, lsl #4
    stmia   r4!, {r10, r12}

    rsb     lr, lr, #0
    smull   r7, r10, r6, r8
    smlal   r7, r10, lr, r9
    movs    r7, r7, lsr #28
    adc     r7, r7, r10, lsl #4
    stmdb   r3!, {r7, r11}
    
    subs    r2, r2, #2
    bne     .arl2
    add     r0, r0, #72
    cmp     r0, r1
    blo     .arl1
    ldmpc   regs=r4-r11

csa:
    .word +0x0db84a81
    .word -0x083b5fe7
    .word +0x0e1b9d7f 
    .word -0x078c36d2 
    .word +0x0f31adcf
    .word -0x05039814  
    .word +0x0fbba815 
    .word -0x02e91dd1 
    .word +0x0feda417
    .word -0x0183603a  
    .word +0x0ffc8fc8 
    .word -0x00a7cb87 
    .word +0x0fff964c
    .word -0x003a2847  
    .word +0x0ffff8d3
    .word -0x000f27b4

    .global III_overlap
III_overlap:
    stmdb   sp!, {r4-r7, lr}
    add     r2, r2, r3, lsl #2
    mov     r3, #6
.ol:
    ldmia r0!, {r4, r5, r6}
    ldmia r1!, {r7, r12, lr}
    add   r4, r4, r7
    add   r5, r5, r12
    add   r6, r6, lr
    str   r4, [r2], #128
    str   r5, [r2], #128
    str   r6, [r2], #128
    subs  r3, r3, #1
    bne   .ol
    sub   r1, r1, #72
    ldmia r0!, {r4, r5, r6, r7, r12, lr}
    stmia r1!, {r4, r5, r6, r7, r12, lr}
    ldmia r0!, {r4, r5, r6, r7, r12, lr}
    stmia r1!, {r4, r5, r6, r7, r12, lr}
    ldmia r0!, {r4, r5, r6, r7, r12, lr}
    stmia r1!, {r4, r5, r6, r7, r12, lr}
    ldmpc regs=r4-r7
