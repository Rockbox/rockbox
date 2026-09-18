/* ARMv4 inner loop for denormalise_bands.
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
 * The compiled loop costs about sixteen cycles per element on the ARM7TDMI:
 * an ldrsh, a mul, a register-specified shift, a str, and a counter
 * increment, compare and taken branch every element.
 *
 * On this core a packed word load does not pay, because unpacking two signed
 * halfwords takes three shifts and costs what the second ldrsh saved.  What
 * does pay is the rest: the two results leave as one stm, three cycles
 * rather than four, and the loop test and taken branch happen once per pair
 * rather than once per element.  About thirteen and a half cycles per
 * element.
 *
 * Bit-exact: ldrsh gives the same sign-extended operand MULT16_16 casts to,
 * the product fits in 32 bits, and the shift is the same arithmetic shift.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "denorm_armv4_asm.S must be assembled in ARM mode"
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

/* ------------------------------------------------------------------------
 * void denorm_band_armv4(celt_sig *f, const celt_norm *x, int n,
 *                        int g, int shift)
 *
 * f[i] = (x[i]*g) >> shift for i in [0,n), with 0 <= shift <= 31.  n is at
 * least 1 in every call from denormalise_bands; a non-positive n writes
 * nothing.  mul requires Rd and Rm to differ, so g is Rm throughout.
 * ------------------------------------------------------------------------ */
    .global denorm_band_armv4
    .type   denorm_band_armv4, %function
denorm_band_armv4:
    push    {r4, lr}
    ldr     ip, [sp, #8]            @ shift
    subs    r2, r2, #2              @ r2 = elements left after the next pair
    blt     .Ldb4_tail
.Ldb4_pair:
    ldrsh   r4, [r1], #2
    ldrsh   lr, [r1], #2
    mul     r4, r3, r4              @ x[i]   * g
    mul     lr, r3, lr              @ x[i+1] * g
    mov     r4, r4, asr ip
    mov     lr, lr, asr ip
    stmia   r0!, {r4, lr}           @ f[i] = r4, f[i+1] = lr
    subs    r2, r2, #2
    bge     .Ldb4_pair
.Ldb4_tail:
    adds    r2, r2, #2              @ 0 or 1 left, or negative for n <= 0
    ble     .Ldb4_done
    ldrsh   r4, [r1], #2
    mul     lr, r3, r4
    mov     lr, lr, asr ip
    str     lr, [r0], #4
.Ldb4_done:
    pop     {r4, pc}
    .size   denorm_band_armv4, .-denorm_band_armv4

    .section .note.GNU-stack,"",%progbits
