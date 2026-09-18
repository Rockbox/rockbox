/* ARMv5E exp_rotation1 for the CELT spreading rotation.
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
 * The compiled loop loads x1 and x2 with ldrsh and hands each straight to
 * smulbb, two two-cycle stalls per iteration, 18% of the function on the
 * ARM926.  Every load here sits at least two instructions ahead of its first
 * use.
 *
 * At stride 1, which is 82 of 148 calls, the loop also reloads the value it
 * just wrote: Xptr[stride] becomes the next Xptr[0].  Carrying it instead
 * saves one load per iteration, and since nothing reads that slot again
 * before the next iteration overwrites it, its store is deferred to the end
 * of the loop, saving one store too.
 *
 * Bit-exact.  The carried register holds the full 32-bit shifted value
 * rather than the 16-bit value EXTRACT16 would store, but every later use of
 * it is the bottom-halfword operand of smulbb or smlabb, which reads exactly
 * the bits that would have been stored, and the final strh stores them.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "exp_rotation1_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* ------------------------------------------------------------------------
 * void exp_rotation1_armv5e(celt_norm *X, int len, int stride,
 *                           opus_val16 c, opus_val16 s)
 *
 * Forward over i = 0 .. len-stride-1, then backward over
 * i = len-2*stride-1 .. 0, each step
 *   x1 = X[i], x2 = X[i+stride]
 *   X[i+stride] = (c*x2 + s*x1 + 16384) >> 15
 *   X[i]        = (c*x1 - s*x2 + 16384) >> 15
 *
 * r0 X, r1 len, r2 stride, r3 c, r4 s, r5 -s, r6 count, r7 x1, r8 x2,
 * r9 and ip the two results, r10 the walking pointer.
 * ------------------------------------------------------------------------ */
    .global exp_rotation1_armv5e
    .type   exp_rotation1_armv5e, %function
exp_rotation1_armv5e:
    push    {r4-r10, lr}
    ldr     r4, [sp, #32]           @ s
    rsb     r5, r4, #0              @ ms; its low halfword wraps as NEG16 does
    cmp     r2, #1
    bne     .Ler_general

    /* ---- stride 1, forward: carry X[i+1] into the next x1 ---- */
    subs    r6, r1, #1              @ len-1 iterations
    ble     .Ler_back1
    mov     r10, r0
    ldrsh   r7, [r10]               @ x1 = X[0]
.Ler_f1:
    ldrsh   r8, [r10, #2]           @ x2 = X[i+1]
    smulbb  r9, r3, r7              @ c*x1
    subs    r6, r6, #1              @ the gap before r8 is used
    smulbb  ip, r3, r8              @ c*x2
    smlabb  r9, r5, r8, r9          @ c*x1 - s*x2
    smlabb  ip, r4, r7, ip          @ c*x2 + s*x1
#ifndef OPUS_ARM_FAST_ROUND
    add     r9, r9, #0x4000
#endif
#ifndef OPUS_ARM_FAST_ROUND
    add     ip, ip, #0x4000
#endif
    mov     r9, r9, asr #15
    mov     r7, ip, asr #15         @ X[i+1], carried as the next x1
    strh    r9, [r10], #2           @ X[i]
    bne     .Ler_f1
    strh    r7, [r10]               @ the last X[i+1]

    /* ---- stride 1, backward: carry X[i] into the next x2 ---- */
.Ler_back1:
    subs    r6, r1, #2              @ len-2 iterations
    ble     .Ler_done
    add     r10, r0, r6, lsl #1     @ &X[len-2]
    ldrsh   r8, [r10]               @ x2 = X[len-2]
.Ler_b1:
    ldrsh   r7, [r10, #-2]!         @ x1 = X[i], r10 = &X[i]
    smulbb  ip, r3, r8              @ c*x2
    subs    r6, r6, #1
    smulbb  r9, r3, r7              @ c*x1
    smlabb  ip, r4, r7, ip          @ c*x2 + s*x1
    smlabb  r9, r5, r8, r9          @ c*x1 - s*x2
#ifndef OPUS_ARM_FAST_ROUND
    add     ip, ip, #0x4000
#endif
#ifndef OPUS_ARM_FAST_ROUND
    add     r9, r9, #0x4000
#endif
    mov     ip, ip, asr #15
    mov     r8, r9, asr #15         @ X[i], carried as the next x2
    strh    ip, [r10, #2]           @ X[i+1]
    bne     .Ler_b1
    strh    r8, [r10]               @ the last X[i], which is X[0]
    b       .Ler_done

    /* ---- any other stride: both loads, both stores ---- */
.Ler_general:
    mov     r2, r2, lsl #1          @ stride in bytes, numerically 2*stride
    subs    r6, r1, r2, lsr #1      @ len-stride iterations
    ble     .Ler_backg
    mov     r10, r0
.Ler_fg:
    ldrsh   r7, [r10]               @ x1
    ldrsh   r8, [r10, r2]           @ x2
    subs    r6, r6, #1
    smulbb  r9, r3, r7
    smulbb  ip, r3, r8
    smlabb  r9, r5, r8, r9          @ c*x1 - s*x2
    smlabb  ip, r4, r7, ip          @ c*x2 + s*x1
#ifndef OPUS_ARM_FAST_ROUND
    add     r9, r9, #0x4000
#endif
#ifndef OPUS_ARM_FAST_ROUND
    add     ip, ip, #0x4000
#endif
    mov     r9, r9, asr #15
    mov     ip, ip, asr #15
    strh    ip, [r10, r2]           @ X[i+stride]
    strh    r9, [r10], #2           @ X[i]
    bne     .Ler_fg
.Ler_backg:
    subs    r6, r1, r2              @ len-2*stride iterations
    ble     .Ler_done
    sub     r10, r6, #1
    add     r10, r0, r10, lsl #1    @ &X[len-2*stride-1]
.Ler_bg:
    ldrsh   r7, [r10]
    ldrsh   r8, [r10, r2]
    subs    r6, r6, #1
    smulbb  r9, r3, r7
    smulbb  ip, r3, r8
    smlabb  r9, r5, r8, r9
    smlabb  ip, r4, r7, ip
#ifndef OPUS_ARM_FAST_ROUND
    add     r9, r9, #0x4000
#endif
#ifndef OPUS_ARM_FAST_ROUND
    add     ip, ip, #0x4000
#endif
    mov     r9, r9, asr #15
    mov     ip, ip, asr #15
    strh    ip, [r10, r2]
    strh    r9, [r10], #-2
    bne     .Ler_bg
.Ler_done:
    pop     {r4-r10, pc}
    .size   exp_rotation1_armv5e, .-exp_rotation1_armv5e

    .section .note.GNU-stack,"",%progbits
