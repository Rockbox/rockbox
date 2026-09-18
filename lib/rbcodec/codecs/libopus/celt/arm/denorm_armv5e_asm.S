/* ARMv5E inner loop for denormalise_bands.
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2009 Xiph.Org Foundation
 * Copyright (c) 2008-2009 Gregory Maxwell
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/bands.c are met.
 *
 * Why this exists
 * ---------------
 * The compiled loop is seven instructions that cost twelve cycles per
 * element on the ARM926:
 *
 *     ldrh   r3, [r4], #2
 *     smulbb r3, r3, r1        @ uses r3 at once: a two-cycle stall
 *     add    r0, r0, #1
 *     asr    r3, r3, r7        @ register-specified shift, two cycles
 *     cmp    ip, r0
 *     str    r3, [lr], #4
 *     bgt    .L324             @ three
 *
 * celt_norm is opus_int16, so two coefficients are one word.  One ldr
 * fetches both, smulbb and smultb take the halves apart, and the pair of
 * results leaves as one stm.  A subs between the load and its first use
 * keeps the load clear of the stall and doubles as the loop test.  That is
 * about six and a half cycles per element.
 *
 * Bit-exact: smulbb and smultb multiply exactly the signed 16-bit operands
 * MULT16_16 casts to, and the shift is the same arithmetic shift.
 *
 * The word load needs x on a word boundary.  celt_norm arrays are only
 * halfword-aligned, so one scalar element runs first when x is not.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "denorm_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* ------------------------------------------------------------------------
 * void denorm_band_armv5e(celt_sig *f, const celt_norm *x, int n,
 *                         int g, int shift)
 *
 * f[i] = (x[i]*g) >> shift for i in [0,n), with 0 <= shift <= 31.  n is at
 * least 1 in every call from denormalise_bands, since band edges strictly
 * increase; a non-positive n writes nothing.
 * ------------------------------------------------------------------------ */
    .global denorm_band_armv5e
    .type   denorm_band_armv5e, %function
denorm_band_armv5e:
    push    {r4, r5, lr}
    ldr     ip, [sp, #12]           @ shift
    cmp     r2, #0
    ble     .Ldb5_done
    tst     r1, #2                  @ one element to reach a word boundary?
    beq     .Ldb5_aligned
    ldrsh   r4, [r1], #2
    sub     r2, r2, #1
    smulbb  r5, r4, r3
    mov     r5, r5, asr ip
    str     r5, [r0], #4
.Ldb5_aligned:
    subs    r2, r2, #2              @ r2 = elements left after the next pair
    blt     .Ldb5_tail
.Ldb5_pair:
    ldr     r4, [r1], #4            @ x[i] low halfword, x[i+1] high
    subs    r2, r2, #2              @ loop test, and the gap before r4 is used
    smulbb  r5, r4, r3              @ x[i]   * g
    smultb  lr, r4, r3              @ x[i+1] * g
    mov     r5, r5, asr ip
    mov     lr, lr, asr ip
    stmia   r0!, {r5, lr}           @ f[i] = r5, f[i+1] = lr
    bge     .Ldb5_pair
.Ldb5_tail:
    adds    r2, r2, #2              @ 0 or 1 left
    beq     .Ldb5_done
    ldrsh   r4, [r1], #2
    smulbb  r5, r4, r3
    mov     r5, r5, asr ip
    str     r5, [r0], #4
.Ldb5_done:
    pop     {r4, r5, pc}
    .size   denorm_band_armv5e, .-denorm_band_armv5e

    .section .note.GNU-stack,"",%progbits
