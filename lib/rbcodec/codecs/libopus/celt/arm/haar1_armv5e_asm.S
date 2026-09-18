/* ARMv5E haar1 for the CELT band recombination.
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
 * The compiled pair loop hands each ldrh straight to smulbb, two two-cycle
 * stalls per pair, and adds the rounding constant to both results.  That
 * stall accounts for all 15,216 of the interlock cycles the profile charges
 * to haar1.  Here both loads run ahead of their first use, and the rounding
 * term joins the shared product once:
 *
 *   (t1 + t2 + R) = (t1 + R) + t2,   (t1 - t2 + R) = (t1 + R) - t2
 *
 * Both loads stay.  haar1 averages a stride of about 2, so the two elements
 * are rarely adjacent and a packed word load would seldom apply.
 *
 * Bit-exact unless OPUS_ARM_FAST_ROUND is defined, which drops the rounding
 * term and truncates instead.  Measured against the rounded build, together
 * with the same change in exp_rotation1: 742 of 15,360 decoded samples
 * differ, at most 2 LSB, 83.1 dB.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "haar1_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* ------------------------------------------------------------------------
 * void haar1_armv5e(celt_norm *X, int N0, int stride)
 *
 * For each i < stride and j < N0/2, with a = X[2j*stride+i] and
 * b = X[(2j+1)*stride+i]:
 *   X[2j*stride+i]     = (23170*a + 23170*b + 16384) >> 15
 *   X[(2j+1)*stride+i] = (23170*a - 23170*b + 16384) >> 15
 *
 * r0 X, r1 pairs, r2 stride, r3 stride in bytes, r4 pair step in bytes,
 * r5 i, r6 walking pointer, r7 count, r8 and lr the two elements, r9 scratch,
 * ip the constant.
 * ------------------------------------------------------------------------ */
    .global haar1_armv5e
    .type   haar1_armv5e, %function
haar1_armv5e:
    push    {r4-r9, lr}
    movs    r1, r1, asr #1          @ N0 >>= 1
    ble     .Lh1_done
    cmp     r2, #0
    ble     .Lh1_done
    mov     r3, r2, lsl #1
    mov     r4, r2, lsl #2
    mov     ip, #0x5A00
    orr     ip, ip, #0x82           @ QCONST16(.70710678f,15) = 23170
    mov     r5, #0
.Lh1_outer:
    add     r6, r0, r5, lsl #1      @ &X[i]
    mov     r7, r1
.Lh1_pair:
    ldrsh   r8, [r6]                @ a
    ldrsh   lr, [r6, r3]            @ b
    subs    r7, r7, #1              @ loop test, and the gap before use
    smulbb  r8, ip, r8              @ t1 = 23170*a
    smulbb  lr, ip, lr              @ t2 = 23170*b
#ifndef OPUS_ARM_FAST_ROUND
    add     r8, r8, #0x4000         @ t1 + R, shared by both results
#endif
    add     r9, r8, lr
    sub     r8, r8, lr
    mov     r9, r9, asr #15
    mov     r8, r8, asr #15
    strh    r8, [r6, r3]            @ X[(2j+1)*stride+i]
    strh    r9, [r6], r4            @ X[2j*stride+i], then step a pair
    bne     .Lh1_pair
    add     r5, r5, #1
    cmp     r5, r2
    blt     .Lh1_outer
.Lh1_done:
    pop     {r4-r9, pc}
    .size   haar1_armv5e, .-haar1_armv5e

    .section .note.GNU-stack,"",%progbits
