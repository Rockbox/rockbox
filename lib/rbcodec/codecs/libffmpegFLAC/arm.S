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
   Orders up to 8 are done in unrolled loops with every coefficient held in a
   register.  Above that there are too few registers to hold them, so orders 9
   and up compute two outputs per pass instead, which lets each coefficient
   and each history sample loaded serve two multiplies: unrolled for orders 9
   to 12, which -8 uses, and a loop for the rest.  The generic routine only
   takes a final odd sample.
 */
/* Two LPC outputs per pass for a fixed order, unrolled: the .pair loop
   below with the order known, so no tap loop and no parity test, and c[0]
   kept in r12 for the last term of y1.  On entry r2 is the order, r3 the
   history and r4 the coefficients. */
.macro PAIR_STEP                      @ two taps of y0 and two of y1
    mla r10, r6, r9, r10              @ y0 += c[m] * s[m]
    ldmia r5!, { r8, r9 }             @ c[m+1], c[m+2]
    mla r11, r6, r8, r11              @ y1 += c[m+1] * s[m]
    ldmdb r3!, { r6, r7 }             @ s[m+2], s[m+1]
    mla r10, r7, r8, r10              @ y0 += c[m+1] * s[m+1]
    mla r11, r7, r9, r11              @ y1 += c[m+2] * s[m+1]
.endm

.macro PAIR_ORDER order
    add r14, r3, #\order * 4          @ first output, holding its residual
    ldr r12, [r4]                     @ c[0]
    subs r0, r0, #2
    blt .pairunrolledlast
1:
    mov r3, r14
    add r5, r4, #4
    ldr r6, [r3, #-4]!                @ newest history sample
    mul r10, r6, r12                  @ the first step, with c[0] from r12
    ldmia r5!, { r8, r9 }
    mul r11, r6, r8
    ldmdb r3!, { r6, r7 }
    mla r10, r7, r8, r10
    mla r11, r7, r9, r11
    .rept (\order - 1) / 2 - 1
    PAIR_STEP
    .endr
.if \order & 1
    mla r10, r6, r9, r10              @ odd: y0's last tap
.else
    mla r10, r6, r9, r10              @ even: two taps of y0, one of y1 left
    ldr r8, [r5]                      @ c[order-1]
    mla r11, r6, r8, r11
    ldr r7, [r3, #-4]                 @ the oldest sample
    mla r10, r7, r8, r10
.endif
    ldmia r14, { r6, r7 }             @ the two residuals
    add r6, r6, r10, asr r1           @ y0
    mla r11, r6, r12, r11             @ y1's last term needs y0
    add r7, r7, r11, asr r1           @ y1
    stmia r14!, { r6, r7 }
    subs r0, r0, #2
    bge 1b
    b .pairunrolledlast
.endm

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
    cmp r2, #12               @ check if order is too high for unrolled loops
    addls pc, pc, r2, lsl #2  @ jump to our unrolled decode loop if it exists
@ jumptable:
    b .pair                   @ order too high, go to the two-output loop
    b .exit                   @ zero order filter isn't possible, exit function
    b .order1
    b .order2
    b .order3
    b .order4
    b .order5
    b .order6
    b .order7
    b .order8
    b .order9
    b .order10
    b .order11

@ last jump table entry coincides with target, so leave it out
.order12:
    PAIR_ORDER 12
.order11:
    PAIR_ORDER 11
.order10:
    PAIR_ORDER 10
.order9:
    PAIR_ORDER 9
.pairunrolledlast:                @ r2 is still the order here
    adds r0, r0, #2               @ 0 or 1 left
    beq .exit
    sub r3, r14, r2, lsl #2       @ the generic routine takes the last one
    b .default

.pair:
    /* Two outputs per pass, y0 at r14 and y1 after it.  y0 sums c[m] times
       the sample m+1 back from it; y1 sums c[m] times the sample m back from
       y0, so walking m upwards one history sample serves y0 with c[m] and y1
       with c[m+1].  y1's last term is c[0] times y0 itself, added once y0 is
       known.  Coefficients go up through r5, samples down through r3.
         r0 outputs left   r1 qlevel        r2 end of the coefficient walk
         r4 coeffs         r10, r11 sums    r12 order    r14 y0 */
    mov r12, r2
    sub r2, r12, #1
    mov r2, r2, lsr #1        @ (order-1)/2 passes of two taps
    add r2, r4, r2, lsl #3
    add r2, r2, #4            @ where r5 stands when they are done
    add r14, r3, r12, lsl #2  @ first output, holding its residual
    subs r0, r0, #2
    blt .pairlast
.pairloop:
    mov r3, r14
    mov r5, r4
    ldr r6, [r3, #-4]!        @ newest history sample
    ldr r9, [r5], #4          @ c[0]
    mov r10, #0
    mov r11, #0
.pairtaps:
    mla r10, r6, r9, r10      @ y0 += c[m] * s[m]
    ldmia r5!, { r8, r9 }     @ c[m+1], c[m+2]
    mla r11, r6, r8, r11      @ y1 += c[m+1] * s[m]
    ldmdb r3!, { r6, r7 }     @ s[m+2], s[m+1]
    mla r10, r7, r8, r10      @ y0 += c[m+1] * s[m+1]
    cmp r5, r2
    mla r11, r7, r9, r11      @ y1 += c[m+2] * s[m+1]
    bne .pairtaps
    tst r12, #1
    mlane r10, r6, r9, r10    @ odd order: y0's last tap, and done
    beq .paireven
.pairout:
    ldr r8, [r4]              @ c[0]
    ldmia r14, { r6, r7 }     @ the two residuals
    add r6, r6, r10, asr r1   @ y0
    mla r11, r6, r8, r11      @ y1's last term needs y0
    add r7, r7, r11, asr r1   @ y1
    stmia r14!, { r6, r7 }
    subs r0, r0, #2
    bge .pairloop
.pairlast:
    adds r0, r0, #2           @ 0 or 1 left
    beq .exit
    mov r2, r12               @ the generic routine takes the last one
    sub r3, r14, r12, lsl #2
    b .default
.paireven:                    @ even order: two taps of y0 and one of y1 left
    mla r10, r6, r9, r10
    ldr r8, [r5]              @ c[order-1]
    mla r11, r6, r8, r11
    ldr r7, [r3, #-4]         @ the oldest sample
    mla r10, r7, r8, r10
    b .pairout

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
   of that.  Orders 9..12, which -8 uses, cannot hold theirs, so they compute
   two outputs per pass instead and every coefficient and sample loaded serves
   two multiplies.  Orders above 12 go through a generic loop.

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
/* Its own section, not plain .text: plugin.lds takes *(.text*) so placement is
   unchanged, but the link map then reports the cold half as a range of its
   own, which is what lets a profile charge it to this filter instead of to
   whichever C function the linker happened to put in front of it. */
#define LPC32_COLD .section .text.flac_lpc_32_cold,"ax",%progbits

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

    /* Orders 9..12: two outputs per pass, y0 at p[order] and y1 after it.
       History ascends from p[0], coefficients descend from coeffs[order-1],
       so h[k] meets c(k) = coeffs[order-1-k] in y0 and c(k-1) in y1.  y1's
       last term is coeffs[0] times y0 itself, added once y0 is known, so
       coeffs[0] is kept in r2.  Sums in r4:r5 (y0) and r6:r7 (y1), history
       in r8 and r9, coefficients in r10, r11 and lr.

       One step: two taps of each sum.  The coefficient that carries over to
       the next step alternates between r10 and r11, so the pair loaded here
       never lands in a register the next instruction reads. */
    .macro  PAIR64_STEP cur, nxt
    smlal   r4, r5, r9, \cur          @ y0 += h[k] * c(k)
    ldmia   r0!, { r8, r9 }           @ h[k+1], h[k+2]
    ldmdb   r3!, { \nxt, lr }         @ c(k+2), c(k+1)
    smlal   r6, r7, r8, \cur          @ y1 += h[k+1] * c(k)
    smlal   r4, r5, r8, lr            @ y0 += h[k+1] * c(k+1)
    smlal   r6, r7, r9, lr            @ y1 += h[k+2] * c(k+1)
    .endm

    /* A whole order.  r12 counts outputs as everywhere else; an odd one left
       over goes to the generic loop, in DRAM, which \far says how to reach. */
    .macro  PAIR64 order, far
    ldr     r2, [r1]                  @ coeffs[0]
    subs    r12, r12, #32             @ count field: outputs left - 2
    bmi     2f
1:
    add     r3, r1, #4 * \order
    ldr     r9, [r0], #4              @ h[0]
    ldr     r10, [r3, #-4]!           @ c(0) = coeffs[order-1]
    smull   r4, r5, r9, r10           @ the first step opens both sums
    ldmia   r0!, { r8, r9 }
    ldmdb   r3!, { r11, lr }
    smull   r6, r7, r8, r10
    smlal   r4, r5, r8, lr
    smlal   r6, r7, r9, lr
    PAIR64_STEP r11, r10
    PAIR64_STEP r10, r11
    PAIR64_STEP r11, r10
  .if (\order - 1) / 2 == 5
    PAIR64_STEP r10, r11
  .endif
  .if \order & 1                      @ odd: y0's last tap
    .if (\order - 1) / 2 == 5
    smlal   r4, r5, r9, r11
    .else
    smlal   r4, r5, r9, r10
    .endif
  .else                               @ even: two taps of y0, one of y1 left
    ldr     r8, [r0], #4              @ h[order-1]
    .if (\order - 1) / 2 == 5
    smlal   r4, r5, r9, r11
    smlal   r6, r7, r8, r11
    .else
    smlal   r4, r5, r9, r10
    smlal   r6, r7, r8, r10
    .endif
    smlal   r4, r5, r8, r2            @ ... c(order-1) is coeffs[0]
  .endif
    and     lr, r12, #31              @ qlevel
    rsb     r3, lr, #32               @ a shift by 32 gives 0, as qlevel 0
    ldmia   r0, { r10, r11 }          @ the two residuals, at p[order]
    mov     r4, r4, lsr lr
    orr     r4, r4, r5, lsl r3
    add     r10, r10, r4              @ y0
    smlal   r6, r7, r10, r2           @ y1's last term needs y0
    mov     r6, r6, lsr lr
    orr     r6, r6, r7, lsl r3
    add     r11, r11, r6              @ y1
    stmia   r0, { r10, r11 }
    sub     r0, r0, #4 * \order - 8   @ the window moves on two samples
    subs    r12, r12, #64
    bpl     1b
2:
    adds    r12, r12, #32             @ count field back to outputs left - 1
    bmi     3f
    mov     r2, #\order               @ one left: the generic loop takes it
  .ifb \far
    ldr     pc, .Lx_pgen
  .else
    b       .Lx_highgen
  .endif
3:
    ldmpc   regs=r4-r11
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
    ldr     pc, .Lx_p9                @ 9 and 11 are rarer, so in DRAM
    b       .Lx_order10
    ldr     pc, .Lx_p11
@ order 12 is the last table slot, so it falls through.  10 and 12 are what
@ the reference encoder's -8 mostly emits, so they get the IRAM.
.Lx_order12:
    PAIR64  12
.Lx_order10:
    PAIR64  10
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

.Lx_order9:
    PAIR64  9, far
.Lx_order11:
    PAIR64  11, far

    /* Orders 13..32, and the odd last output of 9..12.  Four taps a pass,
       both operand streams bursted: the coefficients descend from coeffs +
       order and the history ascends, so the inner loop ends when the
       coefficient pointer reaches coeffs and needs no counter of its own.
       order & 3 leftover taps go first, one at a time. */
.Lx_highgen:                          @ orders 13..32, and one odd output
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
.Lx_p9:   .word .Lx_order9
.Lx_p11:  .word .Lx_order11

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

/* ---------------------------------------------------------------------------
   Rice residual decoding, the inner loop of decode_residuals.

   int flac_rice_decode_arm(int32_t *out, int n, int k, GetBitContext *gb);

   Decodes up to n residuals with Rice parameter k, returns how many it
   actually produced and leaves gb->index exactly where C would have left it.
   The caller decodes whatever is left -- so every case this does not handle
   is handled by the C it was extracted from, and nothing here can change what
   a stream decodes to.

   Why it is worth writing.  ALT_BITSTREAM_READER keeps no cache: every
   UPDATE_CACHE rebuilds the 32-bit window out of four unaligned byte loads,
   once per residual, which measured 13.97 cycles per sample on ARM7TDMI --
   24% of decode_residuals and the single largest line left in the decoder.
   This keeps the window in two registers and refills it from *aligned* words,
   which is what upstream FFmpeg's A32_BITSTREAM_READER does and what it
   selects for ARCH_ARM && !HAVE_FAST_UNALIGNED; Rockbox has that selection
   commented out.

   It decodes only the case that occurs, and bails out otherwise:

     - the leading-zero run must be 7 or fewer.  Measured over four streams,
       99.91% to 99.97% of residuals qualify; the distribution is 43% zero,
       74% one or fewer, 96% three or fewer.
     - k must be 17 or less, which the caller checks once per partition.  With
       both, a code is at most 25 bits, and the window always holds more.
     - it stops rather than read past buffer_end.

   The cache is two whole words of the stream and a bit position in the
   first.  Each residual reads a 32-bit window at that position and adds the
   code's length to it; nothing is shifted along.  Once the position passes
   32 the second word becomes the first and the next word is loaded, so the
   position is 0-31 at every decode and the window has more than 32 bits.

   Registers through the loop:

     r0 out      r1 residuals left   r2 k          r3 scratch
     r4 first word   r5 second word  r6 bit position in r4
     r7 next word to load, aligned   r8 leading-zero table
     r9 31-k     r10 k+1             r11 buffer_end-4    ip, lr scratch
 */

/* One residual: the 32-bit window at the bit position, its unary run, the
   k bits after it, the sign folded back out, and the position moved past the
   code.  Bails to \bail if the run is longer than 7, leaving it for C. */
.macro RICE_DECODE bail
    rsb     r3, r6, #32
    mov     ip, r4, lsl r6
    orr     ip, ip, r5, lsr r3        @ a shift by 32 gives 0, so r6 = 0 works
#if ARM_ARCH >= 5
    clz     lr, ip                    @ leading zeros, the unary run
#else
    mov     lr, ip, lsr #24
    ldrb    lr, [r8, lr]              @ 8 means "8 or more": bail
#endif
    cmp     lr, #7
    bhi     \bail
    rsb     r3, lr, r9                @ 31 - k - z
    mov     r3, ip, lsr r3            @ the 1 bit, then the k low bits
    sub     ip, lr, #1
    add     r3, r3, ip, lsl r2        @ + (z-1) << k  == quotient<<k | rest
    movs    r3, r3, lsr #1            @ fold the sign back out: halve, and
    mvncs   r3, r3                    @ complement if the low bit was set
    str     r3, [r0], #4              @ z<=7 and k<=17 bound the code at 2^20,
                                      @ so the 0x80000000 the caller rejects
                                      @ cannot arise here
    add     lr, lr, r10               @ bits this code used: z + 1 + k
    add     r6, r6, lr
.endm

/* A word as loaded, little-endian, into stream order. */
.macro RICE_BSWAP reg, tmp
    eor     \tmp, \reg, \reg, ror #16
    bic     \tmp, \tmp, #0xff0000
    mov     \reg, \reg, ror #8
    eor     \reg, \reg, \tmp, lsr #8
.endm

/* The position has passed 32: the second word becomes the first, and the
   next word is loaded behind it. */
.macro RICE_FILL
    mov     r4, r5
    ldr     r5, [r7], #4
    sub     r6, r6, #32
    RICE_BSWAP r5, r3
.endm

/* This lives in DRAM by default: the PP502x codec IRAM window has no room
   for it, and a dense loop like this one is cache-resident in any case (§8).
   Define FLAC_RICE_IRAM to move it, once there is room. */
#if defined(USE_IRAM) && defined(FLAC_RICE_IRAM)
    .section .icode,"ax",%progbits
#else
    .section .text.flac_rice_decode,"ax",%progbits
#endif
    .align  2
    .global flac_rice_decode_arm
flac_rice_decode_arm:
    stmfd   sp!, { r4-r11, lr }
    subs    r1, r1, #0
    ble     .Lr_none
    cmp     r2, #17                   @ above this, leave it to C; the caller
    bhi     .Lr_none                  @ checks too, but do not depend on it
    ldr     r12, [r3, #8]             @ gb->index, in bits
    ldr     lr, [r3]                  @ gb->buffer
    stmfd   sp!, { r1, r3, lr }       @ n, gb, buffer
    ldr     r11, [r3, #4]             @ gb->buffer_end
    sub     r11, r11, #4              @ last address a word load may start at

    add     r7, lr, r12, lsr #3       @ byte holding the next bit
    and     r3, r7, #3                @ ... and how far into its word it is
    bic     r7, r7, #3
    and     r6, r12, #7
    add     r6, r6, r3, lsl #3        @ bit position within that word
    add     r3, r7, #4                @ both words must lie inside the buffer
    cmp     r3, r11
    bhi     .Lr_abort
    ldr     r4, [r7], #4
    ldr     r5, [r7], #4
    RICE_BSWAP r4, r3
    RICE_BSWAP r5, r3

    rsb     r9, r2, #31               @ 31-k
    add     r10, r2, #1               @ k+1
#if ARM_ARCH < 5
    adr     r8, .Lr_lztab
#endif

.Lr_pair:
    /* Two residuals per pass, so the count and the loop branch are paid once
       per pair.  r1 counts what is left after the pair in hand.  The position
       is checked between the two as well as after them: one code can carry
       it past 32, and the window is only good below that. */
    subs    r1, r1, #2
    blt     .Lr_last                  @ one or none left
    RICE_DECODE .Lr_bail2
    cmp     r6, #32
    bhs     .Lr_fill1                 @ the less common case, out of line
.Lr_second:
    RICE_DECODE .Lr_bail1
    cmp     r6, #32
    blo     .Lr_pair
    cmp     r7, r11                   @ refill, if there is a word to read
    bhi     .Lr_done
    RICE_FILL
    b       .Lr_pair

.Lr_fill1:
    cmp     r7, r11
    addhi   r1, r1, #1                @ the second of the pair is not done
    bhi     .Lr_done
    RICE_FILL
    b       .Lr_second

.Lr_last:                             @ r1 is -1 with one left, -2 with none
    adds    r1, r1, #1
    beq     .Lr_second                @ one: decode it with r1 = 0 after
    mov     r1, #0
    b       .Lr_done
.Lr_bail2:                            @ a run too long for the assembly:
    add     r1, r1, #1                @ give back the residuals not decoded,
.Lr_bail1:                            @ both of the pair, or just the second
    add     r1, r1, #1

.Lr_done:
    ldmfd   sp!, { r2, r3, lr }       @ n, gb, buffer
    sub     r0, r7, lr                @ r4 was loaded from r7 - 8, so the
    mov     r0, r0, lsl #3            @ index is 8*(r7-buffer) - 64 + r6
    sub     r0, r0, #64
    add     r0, r0, r6
    str     r0, [r3, #8]              @ gb->index
    sub     r0, r2, r1                @ residuals decoded
    ldmpc   regs=r4-r11
.Lr_abort:                            @ nothing decoded, gb untouched
    add     sp, sp, #12
.Lr_none:
    mov     r0, #0
    ldmpc   regs=r4-r11

#if ARM_ARCH < 5
.Lr_lztab:
    .byte  8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4
    .byte  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    .byte  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    .byte  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
    .byte  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .byte  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .byte  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .byte  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif

/* Channel decorrelation and the shift to FLAC_OUTPUT_DEPTH, the loop at the
   end of flac_decode_frame():

   void flac_decorrelate_<mode>_arm(int32_t *ch0, int32_t *ch1, int n,
                                    int scale);

   Both channels are rewritten in place, as left and right, each shifted up
   by scale.  The shift is folded into the arithmetic, (a<<s) + (b<<s) for
   (a+b)<<s and so on, which is the same modulo 2^32 and saves an
   instruction per pair.  Four pairs go per pass, through ldm and stm, and a
   scalar loop takes the rest of a block that is not a multiple of four.

   Dense enough to stay cache-resident, so it lives in DRAM (§8). */

/* One pair, in place: \a is channel 0 and \b channel 1 on entry.  Mid/side
   leaves left in \b and right in \a; every other mode leaves left in \a. */
.macro DECORR_PAIR mode, a, b
.ifc \mode,indep
    mov     \a, \a, lsl r3
    mov     \b, \b, lsl r3
.endif
.ifc \mode,ls                         @ left, side
    mov     \a, \a, lsl r3            @ left
    sub     \b, \a, \b, lsl r3        @ right = left - side
.endif
.ifc \mode,rs                         @ side, right
    mov     \b, \b, lsl r3            @ right
    add     \a, \b, \a, lsl r3        @ left = side + right
.endif
.ifc \mode,ms                         @ mid, side
    sub     \a, \a, \b, asr #1        @ mid - side/2
    mov     \a, \a, lsl r3            @ right
    add     \b, \a, \b, lsl r3        @ left = right + side
.endif
.endm

.macro DECORR_FN name, mode
    .align  2
    .global \name
\name:
    stmfd   sp!, { r4-r11, lr }
    subs    r2, r2, #4
    blt     2f
1:
    ldmia   r0, { r4-r7 }
    ldmia   r1, { r8-r11 }
    DECORR_PAIR \mode, r4, r8
    DECORR_PAIR \mode, r5, r9
    DECORR_PAIR \mode, r6, r10
    DECORR_PAIR \mode, r7, r11
.ifc \mode,ms
    stmia   r0!, { r8-r11 }
    stmia   r1!, { r4-r7 }
.else
    stmia   r0!, { r4-r7 }
    stmia   r1!, { r8-r11 }
.endif
    subs    r2, r2, #4
    bge     1b
2:
    adds    r2, r2, #4
    beq     4f
3:
    ldr     r4, [r0]
    ldr     r8, [r1]
    DECORR_PAIR \mode, r4, r8
.ifc \mode,ms
    str     r8, [r0], #4
    str     r4, [r1], #4
.else
    str     r4, [r0], #4
    str     r8, [r1], #4
.endif
    subs    r2, r2, #1
    bne     3b
4:
    ldmpc   regs=r4-r11
.endm

    .section .text.flac_decorrelate,"ax",%progbits
    DECORR_FN flac_decorrelate_indep_arm, indep
    DECORR_FN flac_decorrelate_ls_arm, ls
    DECORR_FN flac_decorrelate_rs_arm, rs
    DECORR_FN flac_decorrelate_ms_arm, ms
