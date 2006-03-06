/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Thom Johansen 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
/* The following is an assembler optimised version of the LPC filtering
   routines needed for FLAC decoding. It is optimised for use with ARM 
   processors.
   All LPC filtering up to order 9 is done in specially optimised unrolled
   loops, while every order above this is handled by a slower default routine.
 */
    .section .icode,"ax",%progbits
    .global lpc_decode_arm
lpc_decode_arm:
    stmdb sp!, { r4-r11, lr }
    ldr r4, [sp, #36]
    /* r0 = blocksize, r1 = qlevel, r2 = pred_order
       r3 = data, r4 = coeffs
     */
     
    /* the data pointer always lags behind history pointer by 'pred_order'
       samples. since we have one loop for each order, we can hard code this
       and free a register by not saving data pointer. 
     */ 
    sub r3, r3, r2, lsl #2    @ r3 = history
    cmp r0, #0                @ no samples to process
    beq .exit
    cmp r2, #9                @ check if order is too high for unrolled loops
    addls pc, pc, r2, lsl #2  @ jump to our unrolled decode loop if it exists
@ jumptable:
    b .default                @ order too high, go to default routine
    b .exit                   @ zero order filter isn't possible, exit function
    b .order1
    b .order2
    b .order3
    b .order4
    b .order5
    b .order6
    b .order7
    b .order8

@ last jump table entry coincides with target, so leave it out
.order9:
    ldmia r4, { r5-r12, r14 } @ fetch coefs
.loop9:
    ldr r4, [r3], #4          @ load first history sample
    mul r2, r4, r14           @ multiply with last coef
    ldr r4, [r3], #4          @ rinse and repeat while accumulating sum in r2
    mla r2, r4, r12, r2
    ldr r4, [r3], #4
    mla r2, r4, r11, r2
    ldr r4, [r3], #4
    mla r2, r4, r10, r2
    ldr r4, [r3], #4
    mla r2, r4, r9, r2
    ldr r4, [r3], #4
    mla r2, r4, r8, r2
    ldr r4, [r3], #4
    mla r2, r4, r7, r2
    ldr r4, [r3], #4
    mla r2, r4, r6, r2
    ldr r4, [r3], #4
    mla r2, r4, r5, r2
    ldr r4, [r3]              @ r4 = residual
    add r2, r4, r2, asr r1    @ shift sum by qlevel bits and add residual 
    str r2, [r3], #-8*4       @ save result and wrap history pointer back
    subs r0, r0, #1           @ check if we're done
    bne .loop9                @ nope, jump back
    b .exit
    
.order8:
    ldmia r4, { r5-r12 }
.loop8:
    @ we have more registers to spare here, so start block reading
    ldmia r3!, { r4, r14 }
    mul r2, r4, r12
    mla r2, r14, r11, r2
    ldmia r3!, { r4, r14 }
    mla r2, r4, r10, r2
    mla r2, r14, r9, r2
    ldmia r3!, { r4, r14 }
    mla r2, r4, r8, r2
    mla r2, r14, r7, r2
    ldmia r3!, { r4, r14 }
    mla r2, r4, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-7*4
    subs r0, r0, #1
    bne .loop8
    b .exit

.order7:
    ldmia r4, { r5-r11 }
.loop7:
    ldmia r3!, { r4, r12, r14 }
    mul r2, r4, r11
    mla r2, r12, r10, r2
    mla r2, r14, r9, r2
    ldmia r3!, { r4, r12, r14 }
    mla r2, r4, r8, r2
    mla r2, r12, r7, r2
    mla r2, r14, r6, r2
    ldr r4, [r3], #4
    mla r2, r4, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-6*4
    subs r0, r0, #1
    bne .loop7
    b .exit

.order6:
    ldmia r4, { r5-r10 }
.loop6:
    ldmia r3!, { r4, r11-r12, r14 }
    mul r2, r4, r10
    mla r2, r11, r9, r2
    mla r2, r12, r8, r2
    mla r2, r14, r7, r2
    ldmia r3!, { r4, r11 }
    mla r2, r4, r6, r2
    mla r2, r11, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-5*4
    subs r0, r0, #1
    bne .loop6
    b .exit

.order5:
    ldmia r4, { r5-r9 }
.loop5:
    ldmia r3!, { r4, r10-r12, r14 }
    mul r2, r4, r9
    mla r2, r10, r8, r2
    mla r2, r11, r7, r2
    mla r2, r12, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-4*4
    subs r0, r0, #1
    bne .loop5
    b .exit

.order4:
    ldmia r4, { r5-r8 }
.loop4:
    ldmia r3!, { r4, r11-r12, r14 }
    mul r2, r4, r8
    mla r2, r11, r7, r2
    mla r2, r12, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-3*4
    subs r0, r0, #1
    bne .loop4
    b .exit

.order3:
    ldmia r4, { r5-r7 }
.loop3:
    ldmia r3!, { r4, r12, r14 }
    mul r2, r4, r7
    mla r2, r12, r6, r2
    mla r2, r14, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-2*4
    subs r0, r0, #1
    bne .loop3
    b .exit

.order2:
    ldmia r4, { r5-r6 }
.loop2:
    ldmia r3!, { r4, r14 }
    mul r2, r4, r6
    mla r2, r14, r5, r2
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #-1*4
    subs r0, r0, #1
    bne .loop2
    b .exit

.order1:
    ldr r5, [r4]
    ldr r4, [r3], #4 
.loop1:
    mul r2, r4, r5
    ldr r4, [r3]
    add r2, r4, r2, asr r1
    str r2, [r3], #4
    subs r0, r0, #1
    bne .loop1
    b .exit

.default:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest by jump table. */
    add r5, r4, r2, lsl #2   @ need to start in the other end of coefs
    mov r6, r3               @ working copy of history pointer
    mov r7, r2, lsr #2       @ r7 = coefs/4
    mov r14, #0              @ init accumulator 
.dloop1:
    ldmdb r5!, { r8-r11 }
    ldr r12, [r6], #4
    mla r14, r12, r11, r14
    ldr r12, [r6], #4
    mla r14, r12, r10, r14
    ldr r12, [r6], #4
    mla r14, r12, r9, r14
    ldr r12, [r6], #4
    mla r14, r12, r8, r14
    subs r7, r7, #1
    bne .dloop1

    and r7, r2, #3            @ get remaining samples to be filtered
    add pc, pc, r7, lsl #2    @ jump into accumulator chain
@ jumptable:
    b .dsave @ padding
    b .dsave
    b .oneleft
    b .twoleft
@ implicit .threeleft 
    ldr r12, [r5, #-4]!
    ldr r8, [r6], #4
    mla r14, r12, r8, r14  
.twoleft:
    ldr r12, [r5, #-4]!
    ldr r8, [r6], #4
    mla r14, r12, r8, r14  
.oneleft:
    ldr r12, [r5, #-4]!
    ldr r8, [r6], #4
    mla r14, r12, r8, r14  

.dsave:
    ldr r12, [r6]             @ load residual
    add r14, r12, r14, asr r1 @ shift sum by qlevel bits and add residual
    str r14, [r6]             @ store result
    add r3, r3, #4            @ increment history pointer
    subs r0, r0, #1           @ are we done?
    bne .default              @ no, prepare for next sample

.exit:
    ldmia sp!, { r4-r11, pc }
