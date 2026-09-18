/* Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/kiss_fft.c are met.
 */

/* ARMv4 exp_rotation1, the largest single function in an ARM7TDMI decode.
 *
 * The chain of Givens rotations it applies writes X[i+stride] and then reads
 * it back as the next iteration's x1, so at stride 1 -- 58% of the executed
 * iterations -- one of the two loads is redundant and one of the two stores
 * is dead: nothing reads that slot before the next iteration overwrites it,
 * so its store waits until the chain ends.  Both loops below carry that
 * value in a register, forwards as the next x1 and backwards as the next x2.
 *
 * What ARMv4 has to do that ARMv5E does not is truncate it.  The ARMv5E
 * kernel carries the full 32-bit shifted value because every later use is
 * the bottom halfword operand of smulbb, which reads exactly the bits that
 * would have been stored.  Here the use is mul, which reads all 32, so the
 * carried value has to be the halfword EXTRACT16 would have written:
 *
 *      mov  rC, rY, lsl #1          @ bit 30 to bit 31
 *      mov  rC, rC, asr #16         @ bits 30..15, sign extended
 *
 * which is PSHR32(y,15) narrowed to 16 bits, in the two instructions that
 * replace the one shift -- so the carry costs one cycle and saves four.
 *
 * The multiplies are the floor and are left alone.  Every operand is a
 * sign-extended halfword, so the multiplier finishes in two passes: the four
 * of them are 14 cycles of the loop's 27, measured, and no rearrangement
 * (including the three-multiply complex product) improves on that once the
 * multiplier is credited with stopping early.
 *
 * A larger stride gets the plain loop.  Its elements split into stride
 * independent chains that could each be carried the same way, but the chains
 * are about three steps long at the sizes CELT uses, and priming a carry and
 * flushing its deferred store costs more than the three steps save.
 *
 * Bit-exact against the C, forwards and backwards, at every stride.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "exp_rotation1_armv4_asm.S must be assembled in ARM mode"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(OPUS_ARM_ICODE) && !defined(OPUS_NO_ICODE)
    .section .icode,"ax",%progbits
#else
    .text
#endif
    .align  2

/* One carried step of the forward chain: x1 is in r7 and stays there as the
   next iteration's x1, and the store of X[i+1] waits until the chain ends
   because the next iteration reads it out of the register. */
    .macro  ER4_FWD
    ldrsh   r8, [r10, #2]           @ x2 = X[i+1]
    mul     ip, r4, r7              @ s*x1, and the gap before x2 is used
    mul     r9, r5, r8              @ ms*x2
    mla     ip, r3, r8, ip          @ c*x2 + s*x1
    mla     r9, r3, r7, r9          @ c*x1 - s*x2
    add     ip, ip, #0x4000
    add     r9, r9, #0x4000
    mov     r7, ip, lsl #1          @ X[i+1], carried as the next x1,
    mov     r7, r7, asr #16         @ narrowed as EXTRACT16 would store it
    mov     r9, r9, asr #15
    strh    r9, [r10], #2           @ X[i]
    .endm

/* The same backwards, where the carried value is X[i] and becomes the next
   iteration's x2. */
    .macro  ER4_BWD
    ldrsh   r7, [r10, #-2]!         @ x1 = X[i], r10 = &X[i]
    mul     r9, r5, r8              @ ms*x2, and the gap before x1 is used
    mul     ip, r4, r7              @ s*x1
    mla     r9, r3, r7, r9          @ c*x1 - s*x2
    mla     ip, r3, r8, ip          @ c*x2 + s*x1
    add     r9, r9, #0x4000
    add     ip, ip, #0x4000
    mov     r8, r9, lsl #1          @ X[i], carried as the next x2
    mov     r8, r8, asr #16
    mov     ip, ip, asr #15
    strh    ip, [r10, #2]           @ X[i+1]
    .endm

/* ------------------------------------------------------------------------
 * void exp_rotation1_armv4(celt_norm *X, int len, int stride,
 *                          opus_val16 c, opus_val16 s)
 *
 * Forward over i = 0 .. len-stride-1, then backward over
 * i = len-2*stride-1 .. 0, each step
 *   x1 = X[i], x2 = X[i+stride]
 *   X[i+stride] = (c*x2 + s*x1 + 16384) >> 15
 *   X[i]        = (c*x1 - s*x2 + 16384) >> 15
 *
 * r0 X, r1 len, r2 stride, r3 c, r4 s, r5 ms, r6 count, r7 x1, r8 x2,
 * r9 and ip the two accumulators, r10 the walking pointer.
 * ------------------------------------------------------------------------ */
    .global exp_rotation1_armv4
    .type   exp_rotation1_armv4, %function
exp_rotation1_armv4:
    push    {r4-r10, lr}
    ldrsh   r4, [sp, #32]           @ s
    rsb     r5, r4, #0              @ ms = NEG16(s), and narrowed to sixteen
    mov     r5, r5, lsl #16         @ bits so that it wraps where NEG16 does.
    mov     r5, r5, asr #16         @ mul reads all 32 bits, unlike smulbb,
                                    @ so the ARMv5E kernel can skip this and
                                    @ this one cannot.
    cmp     r2, #1
    bne     .Ler4_general

    /* ---- stride 1, forward: carry X[i+1] into the next x1 ---- */
    subs    r6, r1, #1              @ len-1 iterations
    ble     .Ler4_back1
    mov     r10, r0
    ldrsh   r7, [r10]               @ x1 = X[0]
    tst     r6, #1
    beq     .Ler4_f1                @ even already
    ER4_FWD                         @ peel one, so the pairs loop divides
    subs    r6, r6, #1
    beq     .Ler4_f1_end
.Ler4_f1:
    ER4_FWD
    ER4_FWD
    subs    r6, r6, #2
    bne     .Ler4_f1
.Ler4_f1_end:
    strh    r7, [r10]               @ the last X[i+1]

    /* ---- stride 1, backward: carry X[i] into the next x2 ---- */
.Ler4_back1:
    subs    r6, r1, #2              @ len-2 iterations
    ble     .Ler4_done
    add     r10, r0, r6, lsl #1     @ &X[len-2]
    ldrsh   r8, [r10]               @ x2 = X[len-2]
    tst     r6, #1
    beq     .Ler4_b1
    ER4_BWD
    subs    r6, r6, #1
    beq     .Ler4_b1_end
.Ler4_b1:
    ER4_BWD
    ER4_BWD
    subs    r6, r6, #2
    bne     .Ler4_b1
.Ler4_b1_end:
    strh    r8, [r10]               @ the last X[i], which is X[0]
    b       .Ler4_done

    /* ---- any other stride: both loads, both stores ---- */
.Ler4_general:
    mov     r2, r2, lsl #1          @ stride in bytes, numerically 2*stride
    subs    r6, r1, r2, lsr #1      @ len-stride iterations
    ble     .Ler4_backg
    mov     r10, r0
.Ler4_fg:
    ldrsh   r7, [r10]               @ x1
    ldrsh   r8, [r10, r2]           @ x2
    mul     ip, r4, r7              @ s*x1
    mul     r9, r5, r8              @ ms*x2
    mla     ip, r3, r8, ip          @ c*x2 + s*x1
    mla     r9, r3, r7, r9          @ c*x1 - s*x2
    add     ip, ip, #0x4000
    add     r9, r9, #0x4000
    mov     ip, ip, asr #15
    mov     r9, r9, asr #15
    strh    ip, [r10, r2]           @ X[i+stride]
    subs    r6, r6, #1
    strh    r9, [r10], #2           @ X[i]
    bne     .Ler4_fg
.Ler4_backg:
    subs    r6, r1, r2              @ len-2*stride iterations
    ble     .Ler4_done
    sub     r10, r6, #1
    add     r10, r0, r10, lsl #1    @ &X[len-2*stride-1]
.Ler4_bg:
    ldrsh   r7, [r10]
    ldrsh   r8, [r10, r2]
    mul     ip, r4, r7
    mul     r9, r5, r8
    mla     ip, r3, r8, ip
    mla     r9, r3, r7, r9
    add     ip, ip, #0x4000
    add     r9, r9, #0x4000
    mov     ip, ip, asr #15
    mov     r9, r9, asr #15
    strh    ip, [r10, r2]
    subs    r6, r6, #1
    strh    r9, [r10], #-2
    bne     .Ler4_bg
.Ler4_done:
    pop     {r4-r10, pc}
    .size   exp_rotation1_armv4, .-exp_rotation1_armv4

    .section .note.GNU-stack,"",%progbits
