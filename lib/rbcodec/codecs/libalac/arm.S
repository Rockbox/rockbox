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

    .syntax unified

/* ALAC's adaptive FIR predictor for orders 4 to 8, for ARMv4 and later.
 *
 *   void alac_fir_adapt_N_arm(int32_t *x, int32_t *end, int16_t *coefs,
 *                             uint32_t packed);
 *
 * x points at the N+1 samples before the first output.  Each pass reads
 * the residual at x[N+1], replaces it with the decoded sample and advances
 * x by one, until x reaches end.  packed holds the quantisation shift q in
 * bits 0-7, 32 - sample size in bits 8-15 and 1 << (q - 1) in bits 16-31;
 * a register-specified shift uses only the bottom byte of the register, so
 * the one register serves for all three.
 *
 * As in alac.c, with d[j] = x[N-j] - x[0]:
 *
 *   out = x[0] + err + ((sum(d[j] * c[j]) + round) >> q), sign-extended
 *
 * then, while the error has not changed sign, for k = 1..N (j = N - k),
 * c[j] moves by the sign of d[j] (positive error) or against it, and
 * the error loses (|d[j]| >> q) * k, or (-|d[j]| >> q) * k when negative.
 *
 * The coefficients are held in registers, as 32-bit values; the caller
 * makes sure they cannot leave the int16 range during the call.  The
 * coefficient is the second multiply operand, where ARM7TDMI's early
 * termination makes a small value cheaper.
 */

/* In IRAM where the target has it for codecs; otherwise a section of its
   own, so that the link map, and a profile, can tell it apart. */
#ifdef USE_IRAM
    .section .icode,"ax",%progbits
#else
    .section .text.alac_fir_adapt,"ax",%progbits
#endif

/* Order 4: everything in registers.
 *   r0 x, r1 end, r2 packed, r3-r6 c[0..3], r7 x[0], r8-r11 x[1..4] and
 *   then d[3..0], r12 sum and output, lr residual. */
    .align 2
    .global alac_fir_adapt_4_arm
    .type alac_fir_adapt_4_arm, %function
alac_fir_adapt_4_arm:
    push    {r2, r4-r11, lr}
    mov     r12, r2
    mov     r2, r3
    ldrsh   r3, [r12]
    ldrsh   r4, [r12, #2]
    ldrsh   r5, [r12, #4]
    ldrsh   r6, [r12, #6]
    cmp     r0, r1
    bhs     .L4done
.L4loop:
    ldmia   r0, {r7-r11}            @ x[0..4]
    ldr     lr, [r0, #20]           @ residual
    sub     r11, r11, r7            @ d[0]
    sub     r10, r10, r7            @ d[1]
    mul     r12, r11, r3
    sub     r9, r9, r7              @ d[2]
    mla     r12, r10, r4, r12
    sub     r8, r8, r7              @ d[3]
    mla     r12, r9, r5, r12
    mla     r12, r8, r6, r12
    add     r0, r0, #4
    add     r12, r12, r2, lsr #16   @ + round
    add     r12, r7, r12, asr r2
    add     r12, r12, lr
    mov     r7, r2, lsr #8          @ 32 - sample size
    mov     r12, r12, lsl r7
    mov     r12, r12, asr r7
    str     r12, [r0, #16]
    cmp     lr, #0
    blt     .L4neg
    beq     .L4next
                                    @ positive error
    cmp     r8, #0                  @ k = 1
    addgt   r6, r6, #1
    sublt   r6, r6, #1
    rsblt   r8, r8, #0
    subs    lr, lr, r8, asr r2
    ble     .L4next
    cmp     r9, #0                  @ k = 2
    addgt   r5, r5, #1
    sublt   r5, r5, #1
    rsblt   r9, r9, #0
    mov     r9, r9, asr r2
    subs    lr, lr, r9, lsl #1
    ble     .L4next
    cmp     r10, #0                 @ k = 3
    addgt   r4, r4, #1
    sublt   r4, r4, #1
    rsblt   r10, r10, #0
    mov     r10, r10, asr r2
    add     r10, r10, r10, lsl #1
    subs    lr, lr, r10
    ble     .L4next
    cmp     r11, #0                 @ k = 4, the last: error not needed
    addgt   r3, r3, #1
    sublt   r3, r3, #1
.L4next:
    cmp     r0, r1
    blo     .L4loop
.L4done:
    pop     {r12}
    strh    r3, [r12]
    strh    r4, [r12, #2]
    strh    r5, [r12, #4]
    strh    r6, [r12, #6]
    pop     {r4-r11, pc}

.L4neg:
    cmp     r8, #0                  @ k = 1
    subgt   r6, r6, #1
    addlt   r6, r6, #1
    rsbgt   r8, r8, #0
    subs    lr, lr, r8, asr r2
    bge     .L4next
    cmp     r9, #0                  @ k = 2
    subgt   r5, r5, #1
    addlt   r5, r5, #1
    rsbgt   r9, r9, #0
    mov     r9, r9, asr r2
    subs    lr, lr, r9, lsl #1
    bge     .L4next
    cmp     r10, #0                 @ k = 3
    subgt   r4, r4, #1
    addlt   r4, r4, #1
    rsbgt   r10, r10, #0
    mov     r10, r10, asr r2
    add     r10, r10, r10, lsl #1
    subs    lr, lr, r10
    bge     .L4next
    cmp     r11, #0                 @ k = 4
    subgt   r3, r3, #1
    addlt   r3, r3, #1
    cmp     r0, r1
    blo     .L4loop
    b       .L4done
    .size alac_fir_adapt_4_arm, .-alac_fir_adapt_4_arm


/* Orders 5 to 8: the coefficients in registers, the history reloaded.
 * History sample k (1..N) pairs with c[N-k], which is held in r(2+k), so
 * that the code for a given k is the same whatever the order.
 *   r0 x (x + 4N during the adaptation), r1 scratch, r2 packed,
 *   r3-r10 c[N-1] down to c[0], r11 x[0], r12 sum, then scratch,
 *   lr residual.  [sp] end, [sp, #4] coefs. */

/* error -= t * k, t already shifted; t is clobbered */
.macro WEIGHT k, t
.if \k == 1
    subs    lr, lr, \t
.elseif \k == 2
    subs    lr, lr, \t, lsl #1
.elseif \k == 3
    add     \t, \t, \t, lsl #1
    subs    lr, lr, \t
.elseif \k == 4
    subs    lr, lr, \t, lsl #2
.elseif \k == 5
    add     \t, \t, \t, lsl #2
    subs    lr, lr, \t
.elseif \k == 6
    add     \t, \t, \t, lsl #1
    subs    lr, lr, \t, lsl #1
.elseif \k == 7
    rsb     \t, \t, \t, lsl #3
    subs    lr, lr, \t
.else
    subs    lr, lr, \t, lsl #3
.endif
.endm

/* One step of the adaptation for history sample k, already loaded in t,
 * with its coefficient in c.  pos is 1 for a positive error, 0 for
 * negative.  Leaves through next when the error is used up. */
.macro STEP n, k, t, c, pos, next
    subs    \t, \t, r11             @ d, and its sign
.if \pos
    addgt   \c, \c, #1
    sublt   \c, \c, #1
.else
    subgt   \c, \c, #1
    addlt   \c, \c, #1
.endif
.if \k < \n
  .if \pos
    rsblt   \t, \t, #0              @ |d|
  .else
    rsbgt   \t, \t, #0              @ -|d|
  .endif
  .if \k == 1
    subs    lr, lr, \t, asr r2
  .else
    mov     \t, \t, asr r2
    WEIGHT  \k, \t
  .endif
  .if \pos
    ble     \next
  .else
    bge     \next
  .endif
.endif
.endm

/* The adaptation for order n, in the direction pos; x[k] is at
 * [r0, #4 * (k - n)].  Each sample is loaded a step ahead, into the other
 * scratch register, to cover ARM9's load-use interlock. */
.macro ADAPT n, pos, next, done
    ldr     r1, [r0, #4 * (1 - \n)]
    ldr     r12, [r0, #4 * (2 - \n)]
    STEP    \n, 1, r1, r3, \pos, \next
    ldr     r1, [r0, #4 * (3 - \n)]
    STEP    \n, 2, r12, r4, \pos, \next
    ldr     r12, [r0, #4 * (4 - \n)]
    STEP    \n, 3, r1, r5, \pos, \next
    ldr     r1, [r0, #4 * (5 - \n)]
    STEP    \n, 4, r12, r6, \pos, \next
.if \n >= 6
    ldr     r12, [r0, #4 * (6 - \n)]
.endif
    STEP    \n, 5, r1, r7, \pos, \next
.if \n >= 6
  .if \n >= 7
    ldr     r1, [r0, #4 * (7 - \n)]
  .endif
    STEP    \n, 6, r12, r8, \pos, \next
.endif
.if \n >= 7
  .if \n >= 8
    ldr     r12, [r0, #4 * (8 - \n)]
  .endif
    STEP    \n, 7, r1, r9, \pos, \next
.endif
.if \n >= 8
    STEP    \n, 8, r12, r10, \pos, \next
.endif
    sub     r0, r0, #4 * (\n - 1)
    ldr     r1, [sp]
    cmp     r0, r1
    blo     1b
    b       \done
.endm

/* sum += (x[k] - x[0]) * c[n-k] + (x[k+1] - x[0]) * c[n-k-1] */
.macro FIR_PAIR k, ca, cb
    ldmib   r0!, {r1, lr}           @ x[k], x[k+1]
    sub     r1, r1, r11
.if \k == 1
    mul     r12, r1, \ca
.else
    mla     r12, r1, \ca, r12
.endif
    sub     lr, lr, r11
    mla     r12, lr, \cb, r12
.endm

/* load (op ldrsh) or store (op strh) c[n-k] in r(2+k) */
.macro COEFS op, n
    \op     r3, [r12, #2 * (\n - 1)]
    \op     r4, [r12, #2 * (\n - 2)]
    \op     r5, [r12, #2 * (\n - 3)]
    \op     r6, [r12, #2 * (\n - 4)]
    \op     r7, [r12, #2 * (\n - 5)]
.if \n >= 6
    \op     r8, [r12, #2 * (\n - 6)]
.endif
.if \n >= 7
    \op     r9, [r12, #2 * (\n - 7)]
.endif
.if \n >= 8
    \op     r10, [r12, #2 * (\n - 8)]
.endif
.endm

.macro FIR_ADAPT n
    .align 2
    .global alac_fir_adapt_\n\()_arm
    .type alac_fir_adapt_\n\()_arm, %function
alac_fir_adapt_\n\()_arm:
    push    {r1, r2, r4-r11, lr}
    mov     r12, r2
    mov     r2, r3
    COEFS   ldrsh, \n
    cmp     r0, r1
    bhs     8f
1:
    ldr     r11, [r0]               @ x[0]
    FIR_PAIR 1, r3, r4
    FIR_PAIR 3, r5, r6
.if \n >= 6
    FIR_PAIR 5, r7, r8
.endif
.if \n >= 8
    FIR_PAIR 7, r9, r10
.endif
.if \n & 1
    ldr     r1, [r0, #4]!           @ x[n], the odd one
    sub     r1, r1, r11
  .if \n == 5
    mla     r12, r1, r7, r12
  .else
    mla     r12, r1, r9, r12
  .endif
.endif
    ldr     lr, [r0, #4]            @ residual, at x[n+1]
    add     r12, r12, r2, lsr #16   @ + round
    add     r12, r11, r12, asr r2
    add     r12, r12, lr
    mov     r1, r2, lsr #8          @ 32 - sample size
    mov     r12, r12, lsl r1
    mov     r12, r12, asr r1
    str     r12, [r0, #4]
    cmp     lr, #0
    blt     3f
    beq     7f
    ADAPT   \n, 1, 7f, 8f           @ positive error
7:
    sub     r0, r0, #4 * (\n - 1)   @ back to x[1]: the next pass's x
    ldr     r1, [sp]
    cmp     r0, r1
    blo     1b
8:
    ldr     r12, [sp, #4]
    COEFS   strh, \n
    add     sp, sp, #8
    pop     {r4-r11, pc}
3:
    ADAPT   \n, 0, 7b, 8b           @ negative error
    .size alac_fir_adapt_\n\()_arm, .-alac_fir_adapt_\n\()_arm
.endm

    FIR_ADAPT 5
    FIR_ADAPT 6
    FIR_ADAPT 7
    FIR_ADAPT 8
