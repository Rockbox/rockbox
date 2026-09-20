/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 by Thom Johansen 
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

    .syntax unified

/* The following is an assembler optimised version of the LPC filtering
   routines needed for FLAC decoding. It is optimised for use with ARM 
   processors.
   All LPC filtering up to order 9 is done in specially optimised unrolled
   loops, while every order above this is handled by a slower default routine.
 */
#ifdef USE_IRAM
    .section .icode,"ax",%progbits
#else
    .text
#endif
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
    ldr r5, [r4]            @ load the one coef we need
    ldr r4, [r3], #4        @ load one history sample, r3 now points to residual
.loop1:
    mul r2, r4, r5          @ multiply coef by history sample
    ldr r4, [r3]            @ load residual
    add r4, r4, r2, asr r1  @ add result to residual
    str r4, [r3], #4        @ place r3 at next residual, we already have 
    subs r0, r0, #1         @ the current sample in r4 for the next iteration
    bne .loop1
    b .exit

.default:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest by jump table. */
    add r5, r4, r2, lsl #2   @ need to start in the other end of coefs
    mov r7, r2, lsr #2       @ r7 = coefs/4
    mov r14, #0              @ init accumulator
.dloop1:
    ldmdb r5!, { r8-r11 }
    ldmia r3!, { r6, r12 }
    mla r14, r6, r11, r14
    mla r14, r12, r10, r14
    ldmia r3!, { r6, r12 }
    mla r14, r6, r9, r14
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
    ldr r8, [r3], #4
    mla r14, r12, r8, r14  
.twoleft:
    ldr r12, [r5, #-4]!
    ldr r8, [r3], #4
    mla r14, r12, r8, r14  
.oneleft:
    ldr r12, [r5, #-4]!
    ldr r8, [r3], #4
    mla r14, r12, r8, r14  

.dsave:
    ldr r12, [r3]             @ load residual
    add r14, r12, r14, asr r1 @ shift sum by qlevel bits and add residual
    str r14, [r3], #4         @ store result
    sub r3, r3, r2, lsl #2    @ and wrap history pointer back to next first pos
    subs r0, r0, #1           @ are we done?
    bne .default              @ no, prepare for next sample

.exit:
    ldmpc regs=r4-r11


/* ---------------------------------------------------------------------------
   The 64-bit LPC filter.

   void flac_lpc_32_arm(int32_t *decoded, int coeffs[], int pred_order,
                        int qlevel, int len);

   Same contract as flac_lpc_32_c in decoder.c, which decode_subframe_lpc uses
   whenever bps + coeff_prec + av_log2(pred_order) exceeds 32 -- every 24-bit
   stream, and any 16-bit stream encoded at high coefficient precision:

       for (i = pred_order; i < len; i++, decoded++) {
           int64_t sum = 0;
           for (j = 0; j < pred_order; j++)
               sum += (int64_t)coeffs[pred_order-j-1] * decoded[j];
           decoded[j] += sum >> qlevel;
       }

   The oldest history sample multiplies the highest-numbered coefficient, the
   same convention lpc_decode_arm above uses.

   Orders 1..8 get their own loop with every coefficient held in a register:
   the C loop reloads all of them for every output sample although they are
   invariant for the whole call, and spills the loop bound to the stack on top
   of that.  Orders above 8 go through a chunked loop; the reference encoder's
   default maximum order is 8 and no test stream here exceeds it.

   Registers, all orders:

     r0      history pointer, walks on one sample per output
     r1:r2   64-bit accumulator (r1 low); on entry coeffs and pred_order
     r3, lr  history samples, and scratch in the tail
     r12     ((outputs remaining - 1) << 5) | qlevel
     r4...   coefficients, ascending: r4 = coeffs[0]
     ...r11  further coefficients, then spare history samples

   Packing the output counter and qlevel into one register is what makes order
   8 fit: eight coefficients, a 64-bit accumulator, a pointer and the counter
   are already thirteen of the fourteen usable registers.  Unpacking costs one
   AND per output sample and buys a second sample register, which is what keeps
   every load two instructions clear of the multiply that consumes it -- worth
   a cycle per tap on both cores.

   Multiply operand order is deliberate.  On ARM7TDMI the multiplier walks Rs
   eight bits at a time and stops once the rest of Rs is all sign, so the
   coefficient -- fifteen bits or fewer by construction, since coeff_prec is
   capped at 15 -- goes in Rs and the sample in Rm.  That is two passes rather
   than four whatever the sample width, which is why a 24-bit stream pays no
   more per multiply here than a 16-bit one.
 */

/* Placement.  The hot half of this filter wants IRAM on PP502x, but the
   codec's 80 KB window there is nearly full before it arrives: FLAC already
   keeps four 18 KB sample buffers in it.  So the orders that are a fraction
   of a percent of any real stream, and the generic loop that covers the rest,
   are left in DRAM -- a dense loop like this is cache-resident after its first
   pass anyway.  Define FLAC_LPC32_NO_IRAM to demote the whole thing on a
   target that needs the space back. */
#if defined(USE_IRAM) && !defined(FLAC_LPC32_NO_IRAM)
#define LPC32_HOT  .section .icode,"ax",%progbits
#else
#define LPC32_HOT  .text
#endif
#define LPC32_COLD .text

    /* Shift the accumulator down by qlevel, add the residual waiting at
       p[order], store it back, and step the window on by one sample.  The
       loads above leave r0 at p[order], so the store's post-index rewinds it.
       A register-specified shift by 32 yields zero on ARM, which is exactly
       what the qlevel == 0 case needs from the high word.  */
    .macro  LPCTAIL order
    ldr     r3, [r0]                  @ residual
    and     lr, r12, #31              @ qlevel
    mov     r1, r1, lsr lr
    rsb     lr, lr, #32
    orr     r1, r1, r2, lsl lr
    add     r1, r3, r1
    str     r1, [r0], #-(4 * \order - 4)
    subs    r12, r12, #32             @ one output done; the borrow out of the
    .endm                             @ count field is the loop's exit test

    /* The high-order tail.  Orders above 8 cannot hold their coefficients, so
       those loops keep the accumulator in r10:r11 and have r2 free for a
       fourth history sample -- the order is a constant there, so nothing
       needs to remember it. */
    .macro  HTAIL order
    ldr     r2, [r0]                  @ residual
    and     lr, r12, #31              @ qlevel
    mov     r10, r10, lsr lr
    rsb     lr, lr, #32
    orr     r10, r10, r11, lsl lr
    add     r10, r2, r10
    str     r10, [r0], #-(4 * \order - 4)
    subs    r12, r12, #32
    .endm

    /* One pass of four taps, for the orders that cannot keep their
       coefficients in registers.  Both operand streams burst: coefficients
       descend from the top, history ascends.  r2 is the fourth sample
       register, which is what lets the samples come in one ldm of four
       rather than three plus a single load. */
    .macro  HCHUNK first
    ldmdb   r3!, { r4-r7 }
    ldmia   r0!, { r2, r8, r9, lr }
    .ifnb \first
    smull   r10, r11, r2, r7          @ opens the accumulator, so it never
    .else                             @ has to be zeroed
    smlal   r10, r11, r2, r7
    .endif
    smlal   r10, r11, r8, r6
    smlal   r10, r11, r9, r5
    smlal   r10, r11, lr, r4
    .endm

    LPC32_HOT
    .align  2
    .global flac_lpc_32_arm
flac_lpc_32_arm:
    stmfd   sp!, { r4-r11, lr }
    ldr     r12, [sp, #36]            @ len
    subs    r12, r12, r2              @ outputs = len - pred_order
    ble     .Lx_ret
    sub     r12, r12, #1
    orr     r12, r3, r12, lsl #5
.Lx_entry:                            @ internal: r0, r1, r2 and r12 set up
    cmp     r2, #12
    addls   pc, pc, r2, lsl #2
    ldr     pc, .Lx_pgen              @ 13..32: the generic loop, in DRAM
@ jumptable:
    b       .Lx_ret                   @ order 0 cannot occur
    ldr     pc, .Lx_p1                @ 1..3 are a fraction of a percent of
    ldr     pc, .Lx_p2                @ any stream and the tail dominates them
    ldr     pc, .Lx_p3                @ anyway, so they sit in DRAM
    b       .Lx_order4
    b       .Lx_order5
    b       .Lx_order6
    b       .Lx_order7
    b       .Lx_order8
    ldr     pc, .Lx_pgen              @ 9  -- generic loop
    b       .Lx_order10
    ldr     pc, .Lx_pgen              @ 11 -- generic loop
@ order 12 is the last table slot, so it falls through.  10 and 12 get their
@ own loops because they are what the reference encoder's -8 actually emits;
@ 9 and 11 would too if there were IRAM to put them in.
.Lx_order12:
    add     r3, r1, #48               @ coefficient pointer, descending
.Lx_loop12:
    HCHUNK  first
    HCHUNK
    HCHUNK
    HTAIL   12
    bpl     .Lx_order12
    ldmpc   regs=r4-r11

.Lx_order10:
    add     r3, r1, #40
.Lx_loop10:
    ldmdb   r3!, { r4, r5 }           @ coeffs[8..9], the two odd taps
    ldmia   r0!, { r2, r8 }           @ p[0..1]
    smull   r10, r11, r2, r5
    smlal   r10, r11, r8, r4
    HCHUNK
    HCHUNK
    HTAIL   10
    bpl     .Lx_order10
    ldmpc   regs=r4-r11
@ order 8 is reached from the table above
.Lx_order8:
    ldmia   r1, { r4-r11 }            @ coeffs[0..7], resident for the call
.Lx_loop8:
    ldmia   r0!, { r3, lr }           @ p[0..1].  Eight resident coefficients
    smull   r1, r2, r3, r11           @ leave room for two samples, and a pair
    smlal   r1, r2, lr, r10           @ is still worth bursting: an ldm of n is
    ldmia   r0!, { r3, lr }           @ n+1 cycles against 2n for separate
    smlal   r1, r2, r3, r9            @ loads, so two words cost 3 rather than
    smlal   r1, r2, lr, r8            @ 4.  The transfer writes r3 before lr,
    ldmia   r0!, { r3, lr }           @ so consuming r3 first both avoids the
    smlal   r1, r2, r3, r7            @ stall and earns back the final I cycle.
    smlal   r1, r2, lr, r6
    ldmia   r0!, { r3, lr }           @ p[6..7]
    smlal   r1, r2, r3, r5
    smlal   r1, r2, lr, r4
    LPCTAIL 8
    bpl     .Lx_loop8
    ldmpc   regs=r4-r11

.Lx_order7:
    ldmia   r1, { r4-r10 }            @ coeffs[0..6]
.Lx_loop7:
    ldmia   r0!, { r3, r11, lr }      @ p[0..2]: 3+2 cycles against 3x2 for
    smull   r1, r2, r3, r10           @ single loads on ARM7TDMI
    smlal   r1, r2, r11, r9
    smlal   r1, r2, lr, r8
    ldmia   r0!, { r3, r11, lr }      @ p[3..5]
    smlal   r1, r2, r3, r7            @ r3 lands first, lr last, so reading r3
    smlal   r1, r2, r11, r6           @ straight after the ldm cannot stall
    ldr     r3, [r0], #4              @ p[6]
    smlal   r1, r2, lr, r5
    smlal   r1, r2, r3, r4
    LPCTAIL 7
    bpl     .Lx_loop7
    ldmpc   regs=r4-r11

.Lx_order6:
    ldmia   r1, { r4-r9 }             @ coeffs[0..5]
.Lx_loop6:
    ldmia   r0!, { r3, r10, r11, lr } @ p[0..3]: 4+2 against 4x2
    smull   r1, r2, r3, r9
    smlal   r1, r2, r10, r8
    smlal   r1, r2, r11, r7
    ldmia   r0!, { r3, r10 }          @ p[4..5]
    smlal   r1, r2, lr, r6            @ lr is still the first ldm's
    smlal   r1, r2, r3, r5
    smlal   r1, r2, r10, r4
    LPCTAIL 6
    bpl     .Lx_loop6
    ldmpc   regs=r4-r11

.Lx_order5:
    ldmia   r1, { r4-r8 }             @ coeffs[0..4]
.Lx_loop5:
    ldmia   r0!, { r3, r9, r10, r11, lr }   @ the whole window in 5+2 cycles
    smull   r1, r2, r3, r8
    smlal   r1, r2, r9, r7
    smlal   r1, r2, r10, r6
    smlal   r1, r2, r11, r5
    smlal   r1, r2, lr, r4
    LPCTAIL 5
    bpl     .Lx_loop5
    ldmpc   regs=r4-r11

.Lx_order4:
    ldmia   r1, { r4-r7 }             @ coeffs[0..3]
.Lx_loop4:
    ldmia   r0!, { r3, r8, r9, lr }
    smull   r1, r2, r3, r7
    smlal   r1, r2, r8, r6
    smlal   r1, r2, r9, r5
    smlal   r1, r2, lr, r4
    LPCTAIL 4
    bpl     .Lx_loop4
    ldmpc   regs=r4-r11

    LPC32_COLD
    .align  2
.Lx_order3:
    ldmia   r1, { r4-r6 }             @ coeffs[0..2]
.Lx_loop3:
    ldmia   r0!, { r3, r8, lr }
    smull   r1, r2, r3, r6
    smlal   r1, r2, r8, r5
    smlal   r1, r2, lr, r4
    LPCTAIL 3
    bpl     .Lx_loop3
    ldmpc   regs=r4-r11

.Lx_order2:
    ldmia   r1, { r4-r5 }             @ coeffs[0..1]
.Lx_loop2:
    ldmia   r0!, { r3, lr }
    smull   r1, r2, r3, r5
    smlal   r1, r2, lr, r4
    LPCTAIL 2
    bpl     .Lx_loop2
    ldmpc   regs=r4-r11

    /* Order 1 carries the sample it just reconstructed into the next
       iteration in r3: it is the only history the next output needs, so the
       loop holds one load and one multiply and never stalls. */
.Lx_order1:
    ldr     r4, [r1]                  @ the one coefficient
    ldr     r3, [r0], #4              @ p[0]
.Lx_loop1:
    smull   r1, r2, r3, r4
    ldr     r3, [r0]                  @ residual
    and     lr, r12, #31
    mov     r1, r1, lsr lr
    rsb     lr, lr, #32
    orr     r1, r1, r2, lsl lr
    add     r3, r3, r1                @ and this is the next p[0]
    str     r3, [r0], #4
    subs    r12, r12, #32
    bpl     .Lx_loop1
    ldmpc   regs=r4-r11

    /* Orders 9..32.  Four taps a pass, both operand streams bursted: the
       coefficients descend from coeffs + order and the history ascends, so the
       inner loop ends when the coefficient pointer reaches coeffs and needs no
       counter of its own.  order & 3 leftover taps go first, one at a time. */
.Lx_highgen:                          @ orders 9, 11 and 13..32
.Lx_dnext:
    add     r3, r1, r2, lsl #2        @ coefficient pointer, descending
    mov     r10, #0                   @ 64-bit accumulator (r10 low)
    mov     r11, #0
    ands    lr, r2, #3
    beq     .Lx_dchunk
.Lx_drem:
    ldr     r8, [r0], #4
    ldr     r9, [r3, #-4]!
    subs    lr, lr, #1
    smlal   r10, r11, r8, r9
    bne     .Lx_drem
.Lx_dchunk:
    ldmdb   r3!, { r4-r7 }            @ four coefficients, descending
    ldmia   r0!, { r8, r9, lr }       @ three samples, and the fourth below,
    smlal   r10, r11, r8, r7          @ because r8 is free again by then
    ldr     r8, [r0], #4
    smlal   r10, r11, r9, r6
    smlal   r10, r11, lr, r5
    smlal   r10, r11, r8, r4
    cmp     r3, r1
    bhi     .Lx_dchunk
    ldr     r3, [r0]                  @ residual, r0 is at p[order]
    and     lr, r12, #31
    mov     r10, r10, lsr lr
    rsb     lr, lr, #32
    orr     r10, r10, r11, lsl lr
    add     r10, r3, r10
    str     r10, [r0]
    sub     r0, r0, r2, lsl #2        @ back to p[0]...
    add     r0, r0, #4                @ ...and on one sample
    subs    r12, r12, #32
    bpl     .Lx_dnext
    ldmpc   regs=r4-r11

    LPC32_HOT
.Lx_ret:
    ldmpc   regs=r4-r11
.Lx_p1:   .word .Lx_order1
.Lx_p2:   .word .Lx_order2
.Lx_p3:   .word .Lx_order3
.Lx_pgen: .word .Lx_highgen

#if ARM_ARCH >= 5
/* ---------------------------------------------------------------------------
   The same filter for ARMv5E, using the packed 16-bit multiplies.

   void flac_lpc_32_arm_narrow(int32_t *decoded, int coeffs[], int pred_order,
                               int qlevel, int len);

   Only valid where every history sample fits a signed halfword, which is what
   bps <= 16 guarantees on entry: the warm-up samples come from get_sbits(bps),
   and every sample this loop reconstructs is checked before the next output
   reads it.  The caller selects it on bps; it falls back to the 32-bit kernel
   above by itself whenever a precondition fails, so it is never the reason an
   answer differs.

   Why this needs its own kernel, and why ARMv4 gets none:

     smlal is 32x32 -> 64 and costs three cycles on ARM926.  smlabb is
     16x16 + 32 -> 32 and costs one.  Two things have to hold before the cheap
     one is exact.  The sample must fit sixteen bits -- true only for bps <= 16,
     which is why a 24-bit stream cannot use this and stays on smlal.  And the
     sum must fit 32 bits, which holds iff sum|coeffs| * 32768 < 2^31, i.e.
     sum|coeffs| <= 65535.  That is a property of the subframe's coefficients,
     so it is tested once per call, not per sample.  Measured over the test
     corpus it holds for 94% of the 16-bit high-precision subframes.

     Splitting a wider sample into halves was measured and rejected: two
     smlalbb plus the extraction is five cycles against smlal's three, so for
     bps > 16 the 32-bit multiply is not merely adequate, it is optimal.

   Registers:

     r0      history pointer          r1    coeffs, kept for the fallback
     r2      32-bit accumulator       r3    scratch
     r4..r7  coefficients, packed two to a register, low half first
     r8..r11, lr   history samples    r12   packed count and qlevel
 */

    /* Pack coeffs[2i] and coeffs[2i+1] into one register, low index in the
       low half, so that smlabb selects the even coefficient and smlabt the
       odd one.  Three instructions once per call, not per sample. */
    .macro  PACK2 dst, even, odd
    mov     \dst, \even, lsl #16
    mov     \dst, \dst, lsr #16
    orr     \dst, \dst, \odd, lsl #16
    .endm

    /* Accumulator down by qlevel, plus the residual.  One register-specified
       shift folded into the add, where the 64-bit kernel needs a shift, a
       reverse subtract and an or. */
    .macro  NTAIL order
    ldr     r3, [r0]                  @ residual
    and     lr, r12, #31              @ qlevel
    add     r2, r3, r2, asr lr
    str     r2, [r0], #-(4 * \order - 4)
    .endm

    /* Does the sample just reconstructed still fit a signed halfword?  If not,
       the remaining outputs go through the 32-bit kernel.  Three cycles per
       output sample, against the eight per output that the cheap multiplies
       save at order 8. */
    .macro  NGUARD order
    add     r3, r2, #0x8000
    cmp     r3, #0x10000
    bcs     .Ln_bail\order
    .endm

    .macro  NBAIL order
.Ln_bail\order:
    subs    r12, r12, #32             @ that output was finished
    ldmpc   cond=mi, regs=r4-r11      @ and it was the last one
    mov     r2, #\order
    b       .Lx_entry
    .endm

    .align  2
    .global flac_lpc_32_arm_narrow
flac_lpc_32_arm_narrow:
    stmfd   sp!, { r4-r11, lr }
    ldr     r12, [sp, #36]            @ len
    subs    r12, r12, r2              @ outputs = len - pred_order
    ble     .Ln_ret
    sub     r12, r12, #1
    orr     r12, r3, r12, lsl #5
    @ sum|coeffs| <= 65535, or the 32-bit accumulator could overflow
    mov     r3, #0
    sub     lr, r2, #1
.Ln_sum:
    ldr     r4, [r1, lr, lsl #2]      @ walks coeffs[order-1] down to coeffs[0]
    cmp     r4, #0
    rsblt   r4, r4, #0
    add     r3, r3, r4
    subs    lr, lr, #1
    bge     .Ln_sum
    cmp     r3, #0x10000
    bcs     .Lx_entry                 @ too large: use the 32-bit kernel
    cmp     r2, #8
    addls   pc, pc, r2, lsl #2
    b       .Lx_entry                 @ orders 9 and up: 32-bit kernel
@ jumptable:
    b       .Ln_ret                   @ order 0 cannot occur
    b       .Lx_entry                 @ orders 1..4 are left to the 32-bit
    b       .Lx_entry                 @ kernel: they are absent from every
    b       .Lx_entry                 @ stream that reaches this path, and the
    b       .Lx_entry                 @ tail dominates them anyway
    b       .Ln_order5
    b       .Ln_order6
    b       .Ln_order7
@ order 8 falls through
.Ln_order8:
    ldmia   r1, { r8, r9, r10, r11 }  @ coeffs[0..3]
    PACK2   r4, r8, r9
    PACK2   r5, r10, r11
    add     lr, r1, #16
    ldmia   lr, { r8, r9, r10, r11 }  @ coeffs[4..7]
    PACK2   r6, r8, r9
    PACK2   r7, r10, r11
.Ln_loop8:
    ldmia   r0!, { r8, r9, r10, r11 } @ p[0..3]
    smulbt  r2, r8, r7                @ p[0] * coeffs[7]: one cycle, and it
    smlabb  r2, r9, r7, r2            @ needs no accumulator to be zeroed
    smlabt  r2, r10, r6, r2
    smlabb  r2, r11, r6, r2
    ldmia   r0!, { r8, r9, r10, r11 } @ p[4..7]
    smlabt  r2, r8, r5, r2
    smlabb  r2, r9, r5, r2
    smlabt  r2, r10, r4, r2
    smlabb  r2, r11, r4, r2
    NTAIL   8
    NGUARD  8
    subs    r12, r12, #32
    bpl     .Ln_loop8
    ldmpc   regs=r4-r11
    NBAIL   8

.Ln_order7:
    ldmia   r1, { r8, r9, r10, r11 }  @ coeffs[0..3]
    PACK2   r4, r8, r9
    PACK2   r5, r10, r11
    add     lr, r1, #16
    ldmia   lr, { r8, r9, r10 }       @ coeffs[4..6]
    PACK2   r6, r8, r9
    mov     r7, r10                   @ coeffs[6] on its own, low half
.Ln_loop7:
    ldmia   r0!, { r8, r9, r10, r11 } @ p[0..3]
    smulbb  r2, r8, r7                @ p[0] * coeffs[6]
    smlabt  r2, r9, r6, r2
    smlabb  r2, r10, r6, r2
    smlabt  r2, r11, r5, r2
    ldmia   r0!, { r8, r9, r10 }      @ p[4..6]
    smlabb  r2, r8, r5, r2
    smlabt  r2, r9, r4, r2
    smlabb  r2, r10, r4, r2
    NTAIL   7
    NGUARD  7
    subs    r12, r12, #32
    bpl     .Ln_loop7
    ldmpc   regs=r4-r11
    NBAIL   7

.Ln_order6:
    ldmia   r1, { r8, r9, r10, r11 }  @ coeffs[0..3]
    PACK2   r4, r8, r9
    PACK2   r5, r10, r11
    add     lr, r1, #16
    ldmia   lr, { r8, r9 }            @ coeffs[4..5]
    PACK2   r6, r8, r9
.Ln_loop6:
    ldmia   r0!, { r8, r9, r10, r11 } @ p[0..3]
    smulbt  r2, r8, r6                @ p[0] * coeffs[5]
    smlabb  r2, r9, r6, r2
    smlabt  r2, r10, r5, r2
    smlabb  r2, r11, r5, r2
    ldmia   r0!, { r8, r9 }           @ p[4..5]
    smlabt  r2, r8, r4, r2
    smlabb  r2, r9, r4, r2
    NTAIL   6
    NGUARD  6
    subs    r12, r12, #32
    bpl     .Ln_loop6
    ldmpc   regs=r4-r11
    NBAIL   6

.Ln_order5:
    ldmia   r1, { r8, r9, r10, r11 }  @ coeffs[0..3]
    PACK2   r4, r8, r9
    PACK2   r5, r10, r11
    ldr     r6, [r1, #16]             @ coeffs[4] on its own, low half
.Ln_loop5:
    ldmia   r0!, { r8, r9, r10, r11 } @ p[0..3]
    ldr     lr, [r0], #4              @ p[4], loaded well clear of its multiply
    smulbb  r2, r8, r6                @ p[0] * coeffs[4]
    smlabt  r2, r9, r5, r2
    smlabb  r2, r10, r5, r2
    smlabt  r2, r11, r4, r2
    smlabb  r2, lr, r4, r2
    NTAIL   5
    NGUARD  5
    subs    r12, r12, #32
    bpl     .Ln_loop5
    ldmpc   regs=r4-r11
    NBAIL   5

.Ln_ret:
    ldmpc   regs=r4-r11
#endif /* ARM_ARCH >= 5 */
