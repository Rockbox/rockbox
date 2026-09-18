/* ARMv5E inner loop for normalise_residual.
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2009 Xiph.Org Foundation
 * Copyright (c) 2007-2009 Timothy B. Terriberry
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/vq.c are met.
 *
 * Why this exists
 * ---------------
 * The compiled loop is eight instructions per element: an ldr handed
 * straight to smulbb, which stalls a cycle, the rounding add, a
 * register-specified shift at two cycles, a separate counter increment and
 * compare, the strh, and a taken branch.
 *
 * Here the counter is the loop test, each load runs ahead of its use, and
 * the loop unrolls by two so the taken branch happens once per pair.  The
 * register shift stays: the shift is set per call from the energy, so there
 * is no fixed form to specialise to.
 *
 * Bit-exact.  iy is int, and smulbb reads exactly the low halfword that
 * MULT16_16 casts it to.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "normres_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* ------------------------------------------------------------------------
 * void normres_scale_armv5e(celt_norm *X, const int *iy, int N, int g,
 *                           int shift)
 *
 * X[i] = (g*iy[i] + (1<<shift>>1)) >> shift for i in [0,N), 1 <= shift <= 31.
 * N is at least 1, as in the do-while it replaces; a non-positive N writes
 * nothing.
 *
 * r0 X, r1 iy, r2 count, r3 g, r4 rounding term, ip shift, r5 and lr the
 * two elements of a pair.
 * ------------------------------------------------------------------------ */
    .global normres_scale_armv5e
    .type   normres_scale_armv5e, %function
normres_scale_armv5e:
    push    {r4, r5, lr}
    ldr     ip, [sp, #12]           @ shift
    mov     r4, #1
    mov     r4, r4, lsl ip
    mov     r4, r4, lsr #1          @ 1<<shift>>1
    subs    r2, r2, #2              @ r2 = elements left after the next pair
    blt     .Lnr_tail
.Lnr_pair:
    ldr     r5, [r1], #4
    ldr     lr, [r1], #4
    subs    r2, r2, #2              @ loop test, and the gap before use
    smulbb  r5, r3, r5
    smulbb  lr, r3, lr
    add     r5, r5, r4
    add     lr, lr, r4
    mov     r5, r5, asr ip
    mov     lr, lr, asr ip
    strh    r5, [r0], #2
    strh    lr, [r0], #2
    bge     .Lnr_pair
.Lnr_tail:
    adds    r2, r2, #2              @ 0 or 1 left, or negative for N <= 0
    ble     .Lnr_done
    ldr     r5, [r1], #4
    smulbb  r5, r3, r5
    add     r5, r5, r4
    mov     r5, r5, asr ip
    strh    r5, [r0], #2
.Lnr_done:
    pop     {r4, r5, pc}
    .size   normres_scale_armv5e, .-normres_scale_armv5e

    .section .note.GNU-stack,"",%progbits
