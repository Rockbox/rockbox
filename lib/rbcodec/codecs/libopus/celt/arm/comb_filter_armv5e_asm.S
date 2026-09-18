/* ARMv5E inner loop for the CELT comb filter (pitch postfilter).
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2008 Xiph.Org Foundation
 * Copyright (c) 2008 Gregory Maxwell
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/celt.c are met.
 *
 * Why this exists
 * ---------------
 * The same register shortage as the ARMv4 kernel, described in
 * comb_filter_armv4_asm.S, and the same fix.  On this architecture 47.7% of
 * the function's executed memory traffic is stack rather than signal.
 *
 * Two things are cheaper here.  MAC16_32_Q16 is a single smlawb, so the
 * multiplies need no scratch pair and no pre-shifted gain, and two of the
 * three gains ride in one register as a halfword pair that smlawb and
 * smlawt select between.  That buys back enough room to keep the loop limit
 * resident too, so this loop touches the stack not at all.
 *
 * What does not help here is bursting.  The loop reads five consecutive
 * words at each of two places and writes five at a third, which looks like
 * three ideal multi-register transfers, but holding five results at once
 * needs five registers that the window and the gains have already spent,
 * and on this core an ldm of n costs the same n cycles as n separate loads
 * anyway.  So the memory traffic stays one word at a time on purpose.
 *
 * The multiply-accumulates run outer pair, inner pair, centre tap rather
 * than the C's order.  Each product is truncated inside its own
 * instruction, exactly as in the C, and summing the same three truncated
 * values in a different order gives the same 32-bit result, so this is
 * bit-exact.
 *
 * Note the loop bound.  Without CUSTOM_MODES the C loop is
 * "for (i=0;i<N-4;i+=5)" with no scalar tail, so it writes exactly
 * 5*(N/5) samples and leaves any remainder untouched.  This reproduces that
 * rather than fixing it.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "comb_filter_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* One output sample.
 *
 *   y[i] = SATURATE(x[i] + (g10*X2 + g11*(X1+X3) + g12*(X0+X4)) >> 16)
 *
 * where the window values are already doubled.  \new is the slot the fresh
 * delayed sample lands in and \dying the slot the next sample overwrites,
 * which is also the second term of the outer pair and, once that pair is
 * consumed, the scratch the inner pair is built in.
 */
    .macro  CF_ONE new, dying, center, p1a, p1b
    ldr     lr, [r2], #4            @ the fresh delayed sample, undoubled
    ldr     ip, [r1], #4            @ t = x[i]
    add     \dying, \dying, lr, lsl #1   @ outer pair, into the dying slot
    mov     \new, lr, lsl #1        @ and the window slot keeps it doubled
    smlawb  ip, \dying, r10, ip     @ t += (outer  * g12) >> 16
    add     \dying, \p1a, \p1b      @ inner pair
    smlawt  ip, \dying, r9, ip      @ t += (inner  * g11) >> 16
    smlawb  ip, \center, r9, ip     @ t += (centre * g10) >> 16
    cmp     ip, r3                  @ SATURATE(t, SIG_SAT), branchless and
    movgt   ip, r3                  @ with only the positive bound resident
    cmn     ip, r3
    rsblt   ip, r3, #0
    str     ip, [r0], #4
    .endm

/* ------------------------------------------------------------------------
 * void comb_filter_const_armv5e(opus_val32 *y, opus_val32 *x, int T, int N,
 *                               opus_val16 g10, opus_val16 g11,
 *                               opus_val16 g12)
 *
 * r0 y, r1 x, r2 the delayed read pointer, r3 SIG_SAT, r4-r8 the window,
 * r9 g10 packed under g11, r10 g12, r11 the loop limit, ip the accumulator,
 * lr scratch.  Fourteen registers, nothing spilled.
 * ------------------------------------------------------------------------ */
    .global comb_filter_const_armv5e
    .type   comb_filter_const_armv5e, %function
comb_filter_const_armv5e:
    push    {r4-r11, lr}
    ldrh    r9,  [sp, #36]          @ g10, zero-extended: smlawb reads 15:0
    ldrsh   lr,  [sp, #40]          @ g11
    ldrsh   r10, [sp, #44]          @ g12
    orr     r9, r9, lr, lsl #16     @ g10 low, g11 high
    cmp     r3, #5                  @ a short or empty span does nothing,
    blt     .Lcf5_done            @ and keeps N out of the unsigned divide
    /* groups = N/5, by the usual reciprocal, so the loop needs no counter */
    ldr     lr, .Lcf5_recip
    umull   ip, r11, r3, lr         @ RdHi, RdLo and Rm must all differ
    movs    r11, r11, lsr #2
    beq     .Lcf5_done
    add     r11, r11, r11, lsl #2   @ 5 * groups
    add     r11, r1, r11, lsl #2    @ x once the last group has been read
    /* Prime the window from x[-T-2 .. -T+1], doubled, and leave the read
       pointer at x[-T+2], which is the first fresh sample. */
    sub     r2, r1, r2, lsl #2      @ &x[-T]
    sub     r2, r2, #8              @ &x[-T-2]
    ldm     r2!, {r5, r6, r7, r8}   @ X4, X3, X2, X1 in address order
    mov     r5, r5, lsl #1
    mov     r6, r6, lsl #1
    mov     r7, r7, lsl #1
    mov     r8, r8, lsl #1
    ldr     r3, .Lcf5_sat
.Lcf5_loop:
    CF_ONE  r4, r5, r7, r8, r6      @ X0 = new, X4 dies, centre X2
    CF_ONE  r5, r6, r8, r4, r7      @ X4 = new, X3 dies, centre X1
    CF_ONE  r6, r7, r4, r5, r8      @ X3 = new, X2 dies, centre X0
    CF_ONE  r7, r8, r5, r6, r4      @ X2 = new, X1 dies, centre X4
    CF_ONE  r8, r4, r6, r7, r5      @ X1 = new, X0 dies, centre X3
    cmp     r1, r11
    bne     .Lcf5_loop
.Lcf5_done:
    pop     {r4-r11, pc}
    .size   comb_filter_const_armv5e, .-comb_filter_const_armv5e

    .align  2
.Lcf5_sat:
    .word   300000000               @ SIG_SAT
.Lcf5_recip:
    .word   0xCCCCCCCD              @ N/5 = (N * this) >> 34

/* ------------------------------------------------------------------------
 * void celt_sat_armv5e(celt_sig *x, int n)
 *
 * x[i] = SATURATE(x[i], SIG_SAT) in place, the guard celt_synthesis puts on
 * the IMDCT output before the postfilter reads it.  gcc compiles it one
 * sample at a time, and the ldr feeds the first compare directly, which
 * stalls.  Four at a time here: an ldm, then a sub that sets no flags and so
 * keeps the loaded values clear of their first use, four single-bound
 * clamps and an stm.  About 7.25 cycles a sample against 11.
 * ------------------------------------------------------------------------ */
    .macro  SAT_ONE reg
    cmp     \reg, r3
    movgt   \reg, r3
    cmn     \reg, r3
    rsblt   \reg, r3, #0
    .endm

    .align  2
    .global celt_sat_armv5e
    .type   celt_sat_armv5e, %function
celt_sat_armv5e:
    push    {r4-r7, lr}
    ldr     r3, .Lcf5_sat
    cmp     r1, #4
    blt     .Lsat5_tail
.Lsat5_four:
    ldmia   r0, {r4, r5, r6, r7}
    sub     r1, r1, #4              @ the gap, and no flags to disturb
    SAT_ONE r4
    SAT_ONE r5
    SAT_ONE r6
    SAT_ONE r7
    stmia   r0!, {r4, r5, r6, r7}
    cmp     r1, #4
    bge     .Lsat5_four
.Lsat5_tail:
    cmp     r1, #0
    ble     .Lsat5_done
.Lsat5_one:
    ldr     r4, [r0]
    sub     r1, r1, #1
    SAT_ONE r4
    str     r4, [r0], #4
    cmp     r1, #0
    bgt     .Lsat5_one
.Lsat5_done:
    pop     {r4-r7, pc}
    .size   celt_sat_armv5e, .-celt_sat_armv5e

    .section .note.GNU-stack,"",%progbits
