/* ARMv4 inner loop for the CELT comb filter (pitch postfilter).
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
 * comb_filter_const_c is a 5-tap symmetric FIR on a delayed copy of the
 * signal, unrolled five ways so the delay window rotates through registers
 * instead of shifting.  That needs five window values, three gains, three
 * pointers and a saturation bound live at once, which is twelve of the
 * fourteen registers before there is anywhere to accumulate.  gcc gives up
 * and spills the gains, so the compiled loop reloads all three of them and
 * the input pointer from the stack for every single output sample:
 *
 *     ldr   r1, [sp, #4]      @ the x pointer
 *     ldr   r8, [r1]
 *     ldr   r10, [sp, #8]     @ g10
 *     smull r1, r9, r0, r10
 *     ...
 *     ldr   fp, [sp, #24]     @ g11
 *     ...
 *     ldr   r4, [sp, #12]     @ g12
 *
 * Nineteen instructions per sample, four of them reloading invariants, and
 * 59.5% of the function's executed memory traffic is stack rather than
 * signal.  Here everything stays resident and the only stack access is one
 * reload of the loop limit per group of five.
 *
 * Two things make that fit.  The multiply-accumulates are reordered to run
 * outer pair, inner pair, centre tap; each product is truncated inside its
 * own instruction exactly as the C does, and summing the same three
 * truncated values in a different order gives the same 32-bit result, so
 * this is bit-exact rather than merely close.  And the window register that
 * the next output is about to overwrite is dead as soon as the outer pair
 * has been formed, so it serves as the scratch the multiplies need.
 *
 * Note the loop bound.  Without CUSTOM_MODES the C loop is
 * "for (i=0;i<N-4;i+=5)" with no scalar tail, so it writes exactly
 * 5*(N/5) samples and leaves any remainder untouched.  This reproduces that
 * rather than fixing it.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "comb_filter_armv4_asm.S must be assembled in ARM mode"
#endif

/* These kernels are 2,892 bytes that carry 43% of the cycles an ARMv4 decode
 * spends, which makes them the densest thing in the codec to put in IRAM.
 * config.h sets OPUS_ARM_ICODE on the targets whose codec IRAM window has
 * room; everywhere else this stays in .text.  The firmware config.h it reads
 * that from is assembly-safe, and libopus/config.h guards the rest of its
 * includes with __ASSEMBLER__ so this include costs nothing here. */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

    .text
    .align  2

/* One output sample.
 *
 *   y[i] = SATURATE(x[i] + (g10*X2 + g11*(X1+X3) + g12*(X0+X4)) >> 16)
 *
 * where the window values are already doubled.  \new is the slot the fresh
 * delayed sample lands in, \dying the slot the next sample will overwrite,
 * which is also the second term of the outer pair and, once that pair has
 * been consumed, the scratch register for the multiplies.
 *
 * MULT16_32_Q16_armv4 puts the 16-bit operand in the top half of Rs and
 * keeps the high word of the product, so the gains are pre-shifted.  Using
 * the gain as Rm and the data as Rs lets Rs double as RdLo, which ARMv4
 * permits and which is what frees the third register.
 */
    .macro  CF_ONE new, dying, center, p1a, p1b
    ldr     lr, [r2], #4            @ the fresh delayed sample, undoubled
    ldr     ip, [r1], #4            @ t = x[i]
    add     \dying, \dying, lr, lsl #1   @ outer pair, into the dying slot
    mov     \new, lr, lsl #1        @ and the window slot keeps it doubled
    smull   \dying, lr, r11, \dying @ lr = (g12 * outer) >> 16
    add     ip, ip, lr
    add     \dying, \p1a, \p1b      @ inner pair
    smull   \dying, lr, r10, \dying @ lr = (g11 * inner) >> 16
    add     ip, ip, lr
    smull   \dying, lr, r9, \center @ lr = (g10 * centre) >> 16
    add     ip, ip, lr
    cmp     ip, r3                  @ SATURATE(t, SIG_SAT), branchless and
    movgt   ip, r3                  @ with only the positive bound resident
    cmn     ip, r3
    rsblt   ip, r3, #0
    str     ip, [r0], #4
    .endm

/* ------------------------------------------------------------------------
 * void comb_filter_const_armv4(opus_val32 *y, opus_val32 *x, int T, int N,
 *                              opus_val16 g10, opus_val16 g11, opus_val16 g12)
 *
 * r0 y, r1 x, r2 the delayed read pointer, r3 SIG_SAT, r4-r8 the window,
 * r9-r11 the pre-shifted gains, ip the accumulator, lr scratch.  The loop
 * limit is the one thing that does not fit, and it is read once per group.
 * ------------------------------------------------------------------------ */
    .global comb_filter_const_armv4
    .type   comb_filter_const_armv4, %function
comb_filter_const_armv4:
    push    {r4-r11, lr}
    sub     sp, sp, #4
    ldrsh   r9,  [sp, #40]          @ g10
    ldrsh   r10, [sp, #44]          @ g11
    ldrsh   r11, [sp, #48]          @ g12
    mov     r9,  r9,  lsl #16
    mov     r10, r10, lsl #16
    mov     r11, r11, lsl #16
    cmp     r3, #5                  @ a short or empty span does nothing,
    blt     .Lcf_done            @ and keeps N out of the unsigned divide
    /* groups = N/5, by the usual reciprocal, so the loop needs no counter */
    ldr     lr, .Lcf_recip
    umull   ip, r4, r3, lr          @ RdHi, RdLo and Rm must all differ
    movs    r4, r4, lsr #2
    beq     .Lcf_done
    add     r4, r4, r4, lsl #2      @ 5 * groups
    add     r4, r1, r4, lsl #2      @ x once the last group has been read
    str     r4, [sp]
    /* Prime the window from x[-T-2 .. -T+1], doubled, and leave the read
       pointer at x[-T+2], which is the first fresh sample. */
    sub     r2, r1, r2, lsl #2      @ &x[-T]
    sub     r2, r2, #8              @ &x[-T-2]
    ldm     r2!, {r5, r6, r7, r8}   @ X4, X3, X2, X1 in address order
    mov     r5, r5, lsl #1
    mov     r6, r6, lsl #1
    mov     r7, r7, lsl #1
    mov     r8, r8, lsl #1
    ldr     r3, .Lcf_sat
.Lcf_loop:
    CF_ONE  r4, r5, r7, r8, r6      @ X0 = new, X4 dies, centre X2
    CF_ONE  r5, r6, r8, r4, r7      @ X4 = new, X3 dies, centre X1
    CF_ONE  r6, r7, r4, r5, r8      @ X3 = new, X2 dies, centre X0
    CF_ONE  r7, r8, r5, r6, r4      @ X2 = new, X1 dies, centre X4
    CF_ONE  r8, r4, r6, r7, r5      @ X1 = new, X0 dies, centre X3
    ldr     lr, [sp]
    cmp     r1, lr
    bne     .Lcf_loop
.Lcf_done:
    add     sp, sp, #4
    pop     {r4-r11, pc}
    .size   comb_filter_const_armv4, .-comb_filter_const_armv4

    .align  2
.Lcf_sat:
    .word   300000000               @ SIG_SAT
.Lcf_recip:
    .word   0xCCCCCCCD              @ N/5 = (N * this) >> 34

/* ------------------------------------------------------------------------
 * void celt_sat_armv4(celt_sig *x, int n)
 *
 * x[i] = SATURATE(x[i], SIG_SAT) in place, the guard celt_synthesis puts on
 * the IMDCT output before the postfilter reads it.  gcc compiles it one
 * sample at a time: ldr, two compare-and-move clamps against register-held
 * bounds, a compare, str and a taken branch, about 13 cycles a sample.  Four
 * at a time, one ldm and one stm replace four ldr and four str, and the
 * clamp needs only the positive bound.  About 8 cycles a sample.
 * ------------------------------------------------------------------------ */
    .macro  SAT_ONE reg
    cmp     \reg, r3
    movgt   \reg, r3
    cmn     \reg, r3
    rsblt   \reg, r3, #0
    .endm

    .align  2
    .global celt_sat_armv4
    .type   celt_sat_armv4, %function
celt_sat_armv4:
    push    {r4-r7, lr}
    ldr     r3, .Lcf_sat
    cmp     r1, #4
    blt     .Lsat4_tail
.Lsat4_four:
    ldmia   r0, {r4, r5, r6, r7}
    sub     r1, r1, #4              @ no flags: the clamps below set them
    SAT_ONE r4
    SAT_ONE r5
    SAT_ONE r6
    SAT_ONE r7
    stmia   r0!, {r4, r5, r6, r7}
    cmp     r1, #4
    bge     .Lsat4_four
.Lsat4_tail:
    cmp     r1, #0
    ble     .Lsat4_done
.Lsat4_one:
    ldr     r4, [r0]
    sub     r1, r1, #1
    SAT_ONE r4
    str     r4, [r0], #4
    cmp     r1, #0
    bgt     .Lsat4_one
.Lsat4_done:
    pop     {r4-r7, pc}
    .size   celt_sat_armv4, .-celt_sat_armv4

    .section .note.GNU-stack,"",%progbits
