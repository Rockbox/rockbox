/* ARMv4 Good-Thomas 15-point DFT for the CELT backward MDCT.
 *
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/kiss_fft.c are met.
 *
 * Why this exists
 * ---------------
 * 15 = 3*5 and the two are co-prime, so this is Good-Thomas inside
 * Good-Thomas: five 3-point DFTs, then three 5-point DFTs, and nothing
 * between them.  A 15-point complex DFT for 40 real multiplies, where the
 * mixed-radix chain it replaces spends 16 twiddle multiplies per point on
 * the radix-5 pass alone.
 *
 * The arithmetic is the arithmetic the existing butterflies already use, so
 * nothing new is being claimed about accuracy: epi3 is the radix-3 constant
 * from kf_bfly3, and the radix-5 half keeps the sqrt(5)/4 collapse from
 * kf_bfly5, where cos(2pi/5)+cos(4pi/5) is exactly -1/2 -- and the Q15
 * constants honour it exactly, 10126 - 26510 == -16384 -- so four cosine
 * products become one multiply by yc.  Q15 narrowing is the exact
 * smull/shift/orr form the other ARMv4 kernels use, which keeps all 32 bits
 * of the product where MULT16_32_Q15_armv4 drops the low one.
 *
 * Register pressure is the whole difficulty.  A 5-point DFT needs eight live
 * words across its eight multiplies plus two scratch, and ARM has fourteen
 * registers to hold that, the output pointer, its stride and two constants.
 * It fits only because the sum and difference pairs s5 and s11 are parked on
 * the frame across the multiplies -- sixteen bytes, to the same cache line
 * every call -- and because yc borrows the register ya.i will need later,
 * reloading it afterwards for the price of one ldr per 5-point DFT.
 *
 * The output permutation costs nothing.  Output k2 of the k1'th 5-point DFT
 * belongs at 15-point index (10*k1 + 6*k2) mod 15; walking that from k1 in
 * steps of 3 visits every one of the five, so the destinations are a single
 * post-incremented pointer and the permutation is only a choice of which
 * register to store.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "pfa_armv4_asm.S must be assembled in ARM mode"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

    .text
    .align  2

/* Frame: v[15] complex at 0..119, the s5/s11 park at 120..135, then the
   saved output pointer, the 3-element output step and the 5-element stride
   the three 5-point bases are derived from. */
#define VOFF     0
#define PARK     120
#define SV_OUT   136
#define SV_STEP3 140
#define SV_FIVE  144
#define FRAME    152

/* Q15 narrowing that keeps every bit of the product: see kiss_fft_armv4.h. */
    .macro  QNARROW dst, lo, hi
    mov     \lo, \lo, lsr #15
    orr     \dst, \lo, \hi, lsl #17
    .endm

/* Advance the output pointer by d groups of three elements.  The five
   destinations of one 5-point DFT are (10*k1 + 6*k2) mod 15, which walking
   k2 in the order 0,1,4,2,3 visits as steps of 3 with one wrap, so every
   delta is a small multiple of r3 and none needs a multiply. */
    .macro  PFA_ADV d
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

/* One 5-point DFT over v[voff..voff+4], writing through r0.
 *
 *   r0 = output, already at this DFT's base   r1 = ya.i   r2 = yb.i
 *   r3 = three output elements, in bytes
 *
 * Ten multiplies: two for the sqrt(5)/4 term that stands in for the four
 * cosine products, eight for the sines.  The four sine products of each
 * half share their operands between s6 and s12, so both come out of one
 * pass over s9 and s10 and neither has to be kept alive across the other.
 */
    .macro  PFA_DFT5 voff, d1, d2, d3, d4
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

    /* s4 *= sqrt(5)/4.  yc borrows ya.i's register; one ldr puts it back,
       which is cheaper than spending a register on a constant used twice.
       It loads ahead of the four sums below, which is what keeps its I
       cycle off the multiply. */
    ldr     r1, .Lpfa_yc
    add     r9,  r12, r11               @ s3.r = s7.r + s8.r
    sub     r12, r12, r11               @ s4.r = s7.r - s8.r
    add     r11, r10, r8                @ s3.i
    sub     r10, r10, r8                @ s4.i

    smull   r8, lr, r12, r1
    QNARROW r12, r8, lr                 @ s4.r
    smull   r8, lr, r10, r1
    QNARROW r10, r8, lr                 @ s4.i
    ldr     lr, [sp, #(\voff+0)]        @ x0.r
    ldr     r1, .Lpfa_yai               @ and this covers its I cycle

    /* X0 = x0 + s3 goes out immediately, then s3 becomes x0 - s3/4.  x0.r
       is loaded a line early so the constant reload above covers it, and
       x0.i goes into the scratch that storing X0.r frees, which puts three
       instructions between that load and its use. */
    add     r8, lr, r9                  @ X0.r
    str     r8, [r0]
    ldr     r8, [sp, #(\voff+4)]        @ x0.i
    add     r9, r9, #2
    sub     r9, lr, r9, asr #2          @ QUARTER_OF rounds, as kf_bfly5 does
    add     lr, r8, r11                 @ X0.i
    str     lr, [r0, #4]
    add     r11, r11, #2
    sub     r11, r8, r11, asr #2

    /* s5 and s11 are not touched by the eight multiplies below and there is
       no register to keep them in, so they wait on the frame -- sixteen
       bytes, the same line every call. */
    add     r8,  r9,  r12               @ s5.r
    sub     r9,  r9,  r12               @ s11.r
    add     lr,  r11, r10               @ s5.i
    sub     r11, r11, r10               @ s11.i
    mov     r10, lr                     @ stm stores in register order, not
    add     r12, sp, #PARK              @ list order, so s5.i must sit in r10
    stmia   r12, {r8, r9, r10, r11}

    /* s6.r =  s10.i*ya.i + s9.i*yb.i      s12.r = s9.i*ya.i - s10.i*yb.i */
    smull   r10, r12, r5, r1
    QNARROW r8,  r10, r12
    smull   r10, r12, r7, r2
    QNARROW r9,  r10, r12
    smull   r10, r12, r7, r1
    QNARROW r11, r10, r12
    smull   r10, r12, r5, r2
    QNARROW lr,  r10, r12
    add     r5, r8, r9                  @ s6.r
    sub     r7, r11, lr                 @ s12.r

    /* s6.i = -(s10.r*ya.i + s9.r*yb.i)    s12.i = s10.r*yb.i - s9.r*ya.i */
    smull   r10, r12, r4, r1
    QNARROW r8,  r10, r12
    smull   r10, r12, r6, r2
    QNARROW r9,  r10, r12
    smull   r10, r12, r4, r2
    QNARROW r11, r10, r12
    smull   r10, r12, r6, r1
    QNARROW lr,  r10, r12
    add     r4, r8, r9
    rsb     r4, r4, #0                  @ s6.i
    sub     r6, r11, lr                 @ s12.i

    add     r12, sp, #PARK
    ldmia   r12, {r8, r9, r10, r11}     @ s5.r, s11.r, s5.i, s11.i

    sub     r12, r8,  r5                @ X1 = s5 - s6
    sub     lr,  r10, r4
    PFA_ADV \d1
    stmia   r0, {r12, lr}

    add     r12, r8,  r5                @ X4 = s5 + s6
    add     lr,  r10, r4
    PFA_ADV \d2
    stmia   r0, {r12, lr}

    add     r12, r9,  r7                @ X2 = s11 + s12
    add     lr,  r11, r6
    PFA_ADV \d3
    stmia   r0, {r12, lr}

    sub     r12, r9,  r7                @ X3 = s11 - s12
    sub     lr,  r11, r6
    PFA_ADV \d4
    stmia   r0, {r12, lr}
    .endm

/* ------------------------------------------------------------------------
 * void pfa_fft15_armv4(const kiss_fft_cpx *in, kiss_fft_cpx *out,
 *                      int ostride)
 *
 *   r0 = in (15 contiguous complex)   r1 = out   r2 = ostride, in elements
 * ------------------------------------------------------------------------ */
    .global pfa_fft15_armv4
    .type   pfa_fft15_armv4, %function
pfa_fft15_armv4:
    push    {r4-r11, lr}
    sub     sp, sp, #FRAME

    mov     r2, r2, lsl #3              @ ostride in bytes
    str     r1, [sp, #SV_OUT]
    add     r3, r2, r2, lsl #1          @ 3 elements
    str     r3, [sp, #SV_STEP3]
    add     r3, r2, r2, lsl #2          @ 5 elements
    str     r3, [sp, #SV_FIVE]

/* ---- Five 3-point DFTs -------------------------------------------------
 * Group g reads in[3g..3g+2] and writes v[g], v[5+g], v[10+g], so the
 * 5-point DFTs below each read a contiguous run of five.
 */
    add     r1, sp, #VOFF               @ &v[0]
    add     r2, sp, #(VOFF+40)          @ &v[5]
    add     r3, sp, #(VOFF+80)          @ &v[10]
    ldr     lr, .Lpfa_epi3
    mov     r12, #5

.Lpfa_b3:
    ldmia   r0!, {r4,r5,r6,r7,r8,r9}    @ x0, x1, x2
    add     r10, r6, r8                 @ s3.r = x1.r + x2.r
    sub     r6,  r6, r8                 @ s0.r = x1.r - x2.r
    add     r11, r7, r9                 @ s3.i
    sub     r7,  r7, r9                 @ s0.i
    sub     r8, r4, r10, asr #1         @ xm.r = x0.r - HALF_OF(s3.r)
    sub     r9, r5, r11, asr #1         @ xm.i
    add     r4, r4, r10                 @ X0.r = x0.r + s3.r
    add     r5, r5, r11                 @ X0.i
    smull   r10, r11, r6, lr            @ s0.r * epi3
    QNARROW r6, r10, r11
    smull   r10, r11, r7, lr            @ s0.i * epi3
    QNARROW r7, r10, r11
    stmia   r1!, {r4,r5}                @ v[g]
    sub     r10, r8, r7
    add     r11, r9, r6
    stmia   r2!, {r10,r11}              @ v[5+g]
    add     r10, r8, r7
    sub     r11, r9, r6
    stmia   r3!, {r10,r11}              @ v[10+g]
    subs    r12, r12, #1
    bne     .Lpfa_b3

/* ---- Three 5-point DFTs ------------------------------------------------ */
    ldr     r1, .Lpfa_yai
    ldr     r2, .Lpfa_ybi
    ldr     r3, [sp, #SV_STEP3]

    ldr     r0, [sp, #SV_OUT]           @ k1 = 0: base is out itself
    PFA_DFT5 (VOFF+0),  2, 1, 1, -3

    ldr     r0, [sp, #SV_OUT]
    ldr     r12, [sp, #SV_FIVE]
    add     r0, r0, r12, lsl #1         @ k1 = 1: base is out + 10 elements
    PFA_DFT5 (VOFF+40), -3, 1, 1, 2

    ldr     r0, [sp, #SV_OUT]
    ldr     r12, [sp, #SV_FIVE]
    add     r0, r0, r12                 @ k1 = 2: base is out + 5 elements
    PFA_DFT5 (VOFF+80), 2, 1, -4, 2

    add     sp, sp, #FRAME
    pop     {r4-r11, pc}
    .size   pfa_fft15_armv4, .-pfa_fft15_armv4

    .align  2
.Lpfa_epi3:
    .word   -28378                      @ -sin(2*pi/3), as kf_bfly3 uses
.Lpfa_yai:
    .word   -31164                      @ ya.i
.Lpfa_ybi:
    .word   -19261                      @ yb.i
.Lpfa_yc:
    .word    18318                      @ sqrt(5)/4
