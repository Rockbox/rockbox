/* ARMv5E Good-Thomas 15-point DFT for the CELT backward MDCT.
 *
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/kiss_fft.c are met.
 *
 * Why this exists
 * ---------------
 * The same transform as pfa_armv4_asm.S -- five 3-point DFTs then three
 * 5-point DFTs, co-prime so nothing sits between them -- but this core
 * changes what the arithmetic costs, and with it the register budget.
 *
 * A Q15 narrowing is one smulwb rather than smull/shift/orr, so each of the
 * forty multiplies drops from three instructions to one and needs one
 * scratch register rather than a pair.  Where the C adds two products,
 * smlawb folds the add in and is bit-exact with it, because both truncate
 * at Q16 before the sum and the shift distributes.  Where it subtracts them
 * the two stay separate: smlaw only adds, so fusing would mean negating a
 * twiddle, and truncation is not symmetric about zero.
 *
 * The Q15 doubling is deferred.  MULT16_32_Q15 on this core is smulwb
 * followed by a shift left, and every consumer of a product here is an add
 * or a subtract, so the shift folds into it as a free immediate.  That is
 * exact rather than approximate: a +/- (b << 1) is the same value as
 * a +/- SHL32(b, 1) in all 32 bits.
 *
 * Between them those two savings free enough registers that the 5-point DFT
 * needs no frame at all -- where the ARMv4 version has to park its sum and
 * difference pair across the multiplies, this one keeps everything live.
 * ya.i and yb.i share one word, picked apart by the b and t halves of
 * smulwb and smulwt, which is the same packing the radix-5 pass in
 * kiss_fft_armv5e_asm.S uses.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "pfa_armv5e_asm.S must be assembled in ARM mode"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

    .text
    .align  2

#define VOFF     0
#define SV_OUT   120
#define SV_STEP3 124
#define SV_FIVE  128
#define FRAME    136

/* Advance the output pointer by d groups of three elements; see the ARMv4
   kernel for why every delta is a small multiple of one stride. */
    .macro  PFA5_ADV d
    .if \d == 2
    add     r0, r0, r3, lsl #1
    .elseif \d == 1
    add     r0, r0, r3
    .elseif \d == -3
    sub     r0, r0, r3, lsl #1
    sub     r0, r0, r3
    .elseif \d == -4
    sub     r0, r0, r3, lsl #2
    .else
    .error  "unexpected PFA store delta"
    .endif
    .endm

/* One 5-point DFT over v[voff..voff+4].
 *   r0 = output, at this DFT's base   r1 = ya.i packed with yb.i
 *   r2 = sqrt(5)/4                    r3 = three output elements, in bytes
 */
    .macro  PFA5_DFT5 voff, d1, d2, d3, d4
    add     r12, sp, #(\voff+8)
    ldmia   r12, {r4,r5,r6,r7,r8,r9}    @ x1, x2, x3
    add     r12, sp, #(\voff+32)
    ldm     r12, {r10,r11}              @ x4

    add     r12, r4, r10                @ s7.r  = x1.r + x4.r
    sub     r4,  r4, r10                @ s10.r = x1.r - x4.r
    add     r10, r5, r11                @ s7.i
    sub     r5,  r5, r11                @ s10.i
    add     r11, r6, r8                 @ s8.r  = x2.r + x3.r
    sub     r6,  r6, r8                 @ s9.r  = x2.r - x3.r
    add     r8,  r7, r9                 @ s8.i
    sub     r7,  r7, r9                 @ s9.i

    add     r9,  r12, r11               @ s3.r = s7.r + s8.r
    sub     r12, r12, r11               @ s4.r = s7.r - s8.r
    add     r11, r10, r8                @ s3.i
    sub     r10, r10, r8                @ s4.i

    ldr     lr, [sp, #(\voff+0)]        @ x0.r, ahead of the two multiplies
    smulwb  r12, r12, r2                @ s4.r * sqrt(5)/4, left at Q16
    smulwb  r10, r10, r2                @ s4.i * sqrt(5)/4

    add     r8, lr, r9                  @ X0.r
    str     r8, [r0]
    ldr     r8, [sp, #(\voff+4)]        @ x0.i, into the scratch that frees
    add     r9, r9, #2
    sub     r9, lr, r9, asr #2          @ QUARTER_OF rounds, as kf_bfly5 does
    add     lr, r8, r11                 @ X0.i
    str     lr, [r0, #4]
    add     r11, r11, #2
    sub     r11, r8, r11, asr #2

    add     r8,  r9,  r12, lsl #1       @ s5.r   (the deferred doubling)
    sub     r9,  r9,  r12, lsl #1       @ s11.r
    add     lr,  r11, r10, lsl #1       @ s5.i
    sub     r11, r11, r10, lsl #1       @ s11.i

    /* s6.r = s10.i*ya + s9.i*yb, so smlawt carries the add. */
    smulwb  r10, r5, r1
    smlawt  r10, r7, r1, r10
    /* s12.r = s9.i*ya - s10.i*yb: a difference, so the products stay apart. */
    smulwb  r12, r7, r1
    smulwt  r5,  r5, r1
    sub     r12, r12, r5

    /* s6.i is the negation of this sum; the sign folds into the consumers. */
    smulwb  r5, r4, r1
    smlawt  r5, r6, r1, r5
    /* s12.i = s10.r*yb - s9.r*ya */
    smulwt  r4, r4, r1
    smulwb  r6, r6, r1
    sub     r4, r4, r6

    sub     r6, r8,  r10, lsl #1        @ X1.r = s5.r - s6.r
    add     r7, lr,  r5,  lsl #1        @ X1.i = s5.i - s6.i
    PFA5_ADV \d1
    stmia   r0, {r6, r7}

    add     r6, r8,  r10, lsl #1        @ X4.r
    sub     r7, lr,  r5,  lsl #1        @ X4.i
    PFA5_ADV \d2
    stmia   r0, {r6, r7}

    add     r6, r9,  r12, lsl #1        @ X2.r = s11.r + s12.r
    add     r7, r11, r4,  lsl #1        @ X2.i
    PFA5_ADV \d3
    stmia   r0, {r6, r7}

    sub     r6, r9,  r12, lsl #1        @ X3.r
    sub     r7, r11, r4,  lsl #1        @ X3.i
    PFA5_ADV \d4
    stmia   r0, {r6, r7}
    .endm

/* ------------------------------------------------------------------------
 * void pfa_fft15_armv5e(const kiss_fft_cpx *in, kiss_fft_cpx *out,
 *                       int ostride)
 * ------------------------------------------------------------------------ */
    .global pfa_fft15_armv5e
    .type   pfa_fft15_armv5e, %function
pfa_fft15_armv5e:
    push    {r4-r11, lr}
    sub     sp, sp, #FRAME

    mov     r2, r2, lsl #3              @ ostride in bytes
    str     r1, [sp, #SV_OUT]
    add     r3, r2, r2, lsl #1
    str     r3, [sp, #SV_STEP3]
    add     r3, r2, r2, lsl #2
    str     r3, [sp, #SV_FIVE]

/* ---- Five 3-point DFTs ------------------------------------------------ */
    add     r1, sp, #VOFF
    add     r2, sp, #(VOFF+40)
    add     r3, sp, #(VOFF+80)
    ldr     lr, .Lpfa5_epi3
    mov     r12, #5

.Lpfa5_b3:
    ldmia   r0!, {r4,r5,r6,r7,r8,r9}    @ x0, x1, x2
    add     r10, r6, r8                 @ s3.r
    sub     r6,  r6, r8                 @ s0.r
    add     r11, r7, r9                 @ s3.i
    sub     r7,  r7, r9                 @ s0.i
    sub     r8, r4, r10, asr #1         @ xm.r = x0.r - HALF_OF(s3.r)
    sub     r9, r5, r11, asr #1         @ xm.i
    add     r4, r4, r10                 @ X0.r
    add     r5, r5, r11                 @ X0.i
    smulwb  r6, r6, lr                  @ s0.r * epi3, left at Q16
    smulwb  r7, r7, lr                  @ s0.i * epi3
    stmia   r1!, {r4,r5}                @ v[g]
    sub     r10, r8, r7, lsl #1
    add     r11, r9, r6, lsl #1
    stmia   r2!, {r10,r11}              @ v[5+g]
    add     r10, r8, r7, lsl #1
    sub     r11, r9, r6, lsl #1
    stmia   r3!, {r10,r11}              @ v[10+g]
    subs    r12, r12, #1
    bne     .Lpfa5_b3

/* ---- Three 5-point DFTs ----------------------------------------------- */
    ldr     r1, .Lpfa5_yab
    ldr     r2, .Lpfa5_yc
    ldr     r3, [sp, #SV_STEP3]

    ldr     r0, [sp, #SV_OUT]
    PFA5_DFT5 (VOFF+0),  2, 1, 1, -3

    ldr     r0, [sp, #SV_OUT]
    ldr     r12, [sp, #SV_FIVE]
    add     r0, r0, r12, lsl #1
    PFA5_DFT5 (VOFF+40), -3, 1, 1, 2

    ldr     r0, [sp, #SV_OUT]
    ldr     r12, [sp, #SV_FIVE]
    add     r0, r0, r12
    PFA5_DFT5 (VOFF+80), 2, 1, -4, 2

    add     sp, sp, #FRAME
    pop     {r4-r11, pc}
    .size   pfa_fft15_armv5e, .-pfa_fft15_armv5e

    .align  2
.Lpfa5_epi3:
    .word   -28378                      @ -sin(2*pi/3), in the low half
.Lpfa5_yab:
    .word   0xb4c38644                  @ ya.i = -31164 low, yb.i = -19261 top
.Lpfa5_yc:
    .word    18318                      @ sqrt(5)/4

    .section .note.GNU-stack,"",%progbits
