/* ARMv5E dual_inner_prod for the CELT fixed-point decoder.
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2009 Xiph.Org Foundation
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/pitch.h are met.
 *
 * Why this exists
 * ---------------
 * The decoder reaches dual_inner_prod from one place, stereo_merge, which
 * calls it as dual_inner_prod(Y, X, Y, N, &xp, &side).  Over eight packets of
 * stereo decode that is 6,400 elements costing 12.1 cycles each, against two
 * multiplies of real work.
 *
 * The cost is the loads.  The C reads three signed halfwords per element, and
 * on ARM926EJ-S an LDRSH result is two cycles late rather than one, so the
 * loop stalls on every one of them.  One LDR fetches two coefficients
 * instead, and smlabb and smlatt take the halves apart without a separate
 * sign extension -- which is also why this is worth doing here and not on
 * ARMv4, where the extraction would cost exactly what the extra loads save.
 *
 * Alignment.  A packed load needs a word-aligned address, and celt_norm is a
 * halfword, so a band can start on an odd element.  The three pointers can
 * only be packed together if they share their alignment, so the kernel checks
 * that first and falls back to the scalar loop if they disagree; when they
 * agree it steps one element at a time until the address is word-aligned and
 * then runs packed.  Both fallbacks compute exactly what the fast path does,
 * in the same order.
 *
 * Bit-exactness.  MAC16_16 is smlabb and the sum is a plain 32-bit
 * accumulation, so the order of accumulation is the only thing that could
 * differ, and this keeps it: element 0 then 1 within each word, low
 * addresses first.  Output is bit-identical to the C, which the test harness
 * checks sample by sample.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "pitch_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* ------------------------------------------------------------------------
 * void dual_inner_prod_armv5e(const opus_val16 *x, const opus_val16 *y01,
 *                             const opus_val16 *y02, int N,
 *                             opus_val32 *xy1, opus_val32 *xy2)
 *
 *   r0 = x   r1 = y01   r2 = y02   r3 = N   [sp+0] = xy1   [sp+4] = xy2
 * ------------------------------------------------------------------------ */
    .global dual_inner_prod_armv5e
    .type   dual_inner_prod_armv5e, %function
dual_inner_prod_armv5e:
    push    {r4-r8, lr}
    mov     r7, #0                  @ xy01
    mov     r8, #0                  @ xy02
    cmp     r3, #0
    ble     .Ldip_done

    /* The three streams can only be read in pairs if they agree on their
       alignment; two bits of exclusive-or settle it. */
    eor     ip, r0, r1
    eor     lr, r0, r2
    orr     ip, ip, lr
    tst     ip, #3
    bne     .Ldip_scalar

.Ldip_align:                        @ step singly until x is word-aligned
    tst     r0, #3
    beq     .Ldip_pairs
    ldrsh   r4, [r0], #2
    ldrsh   r5, [r1], #2
    ldrsh   r6, [r2], #2
    smlabb  r7, r4, r5, r7
    smlabb  r8, r4, r6, r8
    subs    r3, r3, #1
    bne     .Ldip_align
    b       .Ldip_done

.Ldip_pairs:
    subs    r3, r3, #2
    bmi     .Ldip_tail
.Ldip_loop:
    ldr     r4, [r0], #4            @ x[i], x[i+1]
    ldr     r5, [r1], #4            @ y01[i], y01[i+1]
    ldr     r6, [r2], #4            @ y02[i], y02[i+1]
    smlabb  r7, r4, r5, r7
    smlabb  r8, r4, r6, r8
    smlatt  r7, r4, r5, r7
    smlatt  r8, r4, r6, r8
    subs    r3, r3, #2
    bpl     .Ldip_loop
.Ldip_tail:
    tst     r3, #1                  @ one element left when N was odd
    beq     .Ldip_done
    ldrsh   r4, [r0]
    ldrsh   r5, [r1]
    ldrsh   r6, [r2]
    smlabb  r7, r4, r5, r7
    smlabb  r8, r4, r6, r8
    b       .Ldip_done

.Ldip_scalar:
    ldrsh   r4, [r0], #2
    ldrsh   r5, [r1], #2
    ldrsh   r6, [r2], #2
    smlabb  r7, r4, r5, r7
    smlabb  r8, r4, r6, r8
    subs    r3, r3, #1
    bne     .Ldip_scalar

.Ldip_done:
    ldr     ip, [sp, #24]           @ xy1
    ldr     lr, [sp, #28]           @ xy2
    str     r7, [ip]
    str     r8, [lr]
    pop     {r4-r8, pc}
    .size   dual_inner_prod_armv5e, .-dual_inner_prod_armv5e
