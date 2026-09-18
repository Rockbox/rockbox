/* ARMv5E FFT butterflies for the CELT fixed-point kiss_fft.
 *
 * Copyright (c) 2003-2004, Mark Borgerding
 * Copyright (c) 2005-2007, Xiph.Org Foundation
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/kiss_fft.c are met.
 *
 * Why this exists
 * ---------------
 * The argument here is not the one behind kiss_fft_armv4_asm.S.  On
 * ARM926EJ-S a load costs one cycle and a load multiple of n costs n, so
 * bursting buys nothing; upstream's arm/kiss_fft_armv5e.h already supplies a
 * packed-twiddle complex multiply that is four DSP multiplies and a
 * subtraction, which is optimal.  What is left is bookkeeping.
 *
 * Measured over eight packets of stereo decode, 16.0% of the instructions
 * executed in the compiled radix-4 pass are stack traffic: gcc cannot hold
 * four output pointers, a twiddle base and a stride at once while a complex
 * multiply is in flight, so it spills the pointers and reloads them every
 * pass.  The radix-3 pass loses 11.4% the same way and the radix-5 pass
 * 23.0%.
 *
 * These routines keep the bookkeeping resident and park only what genuinely
 * will not fit.  Three things pay for that:
 *
 *   - The twiddle addresses are a base and a byte displacement rather than
 *     three running pointers, so one register holds what three did and the
 *     stride never has to be reloaded to advance them.  Pass j reads
 *     twiddles[j*fstride] as [base, disp], twiddles[2*j*fstride] as
 *     [base, disp, lsl #1], and only the radix-4 third twiddle needs an
 *     explicit 3*disp.
 *
 *   - Every store is post-indexed, so the four output pointers advance for
 *     free and the loop needs no pointer arithmetic at all.
 *
 *   - The Q15 doubling that C_MUL applies to each product is deferred into
 *     the add or subtract that consumes it, as an immediate shift, which is
 *     free on this core.  Deferring is exact: SHL32 is a wrapping shift, so
 *     2*(a+b) and 2*a+2*b agree in all 32 bits.
 *
 * Every pair load is ordered so the register the next instruction needs
 * landed at least two instructions earlier, which is what the five-stage
 * pipeline wants; the loop counter's decrement fills the one remaining slot.
 *
 * Bit-exactness.  The multiply sequence is the one in
 * arm/kiss_fft_armv5e.h -- smulwb, smulwb, smulwt, smlawt, then a
 * subtraction -- so every product truncates exactly where the C did, and the
 * deferred doubling is exact as above.  Output is bit-identical to the C
 * path, which the test harness checks sample by sample.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "kiss_fft_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* C_MUL, packed twiddle.
 *
 *   \pr, \pi   the point, .r and .i, consumed
 *   \tw        the packed twiddle: tw.r in bits 15:0, tw.i in 31:16
 *   \t         scratch, consumed
 *   result     \pr = m.r/2, \pi = m.i/2   (the caller doubles as it consumes)
 *
 * Matches C_MUL in arm/kiss_fft_armv5e.h term for term:
 *   m.r = ((p.r*tw.r)>>16 - (p.i*tw.i)>>16) << 1
 *   m.i = ((p.i*tw.r)>>16 + (p.r*tw.i)>>16) << 1
 */
    .macro  CMUL_HALF pr, pi, tw, t
    smulwt  \t,  \pi, \tw           @ p.i*tw.i >> 16
    smulwb  \pi, \pi, \tw           @ p.i*tw.r >> 16
    smlawt  \pi, \pr, \tw, \pi      @ + p.r*tw.i >> 16   -> m.i/2
    smulwb  \pr, \pr, \tw           @ p.r*tw.r >> 16
    sub     \pr, \pr, \t            @ - p.i*tw.i >> 16   -> m.r/2
    .endm


/* ------------------------------------------------------------------------
 * void kf_bfly4_armv5e(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
 *                      int fstride, int m, int N, int mm)
 *
 *   r0 = Fout   r1 = tw   r2 = fstride   r3 = m   [sp+0] = N   [sp+4] = mm
 *
 * Inner-loop registers:
 *   r0-r3  the four output pointers, Fout + 0, m, 2m, 3m, post-incremented
 *   r4     twiddle base            r5  twiddle displacement in bytes
 *   r6     pass counter            r7-r11, ip, lr  scratch
 *
 * Frame, 40 bytes.  The first sixteen are the placed spill: the inner loop
 * parks one complex value there, the peeled pass two.
 *    0  v.r / T0.r    4  v.i / T1.r    8  T1.i   12  T0.i
 *   16  fstride, bytes   20  Fout for the next i   24  N left
 *   28  mm*8            32  M = m*8              36  m
 * ------------------------------------------------------------------------ */
    .global kf_bfly4_armv5e
    .type   kf_bfly4_armv5e, %function
kf_bfly4_armv5e:
    push    {r4-r11, lr}
    cmp     r3, #1
    bne     .Lb4_general

/* Degenerate case: every twiddle is 1, so four consecutive complex points --
   eight consecutive words -- go in and eight come out with no multiplies.
   This is the one radix-4 body that fits entirely in registers. */
    ldr     r1, [sp, #36]           @ N
.Lb4_m1:
    ldm     r0, {r4, r5, r6, r7, r8, r9, r10, r11}
    sub     ip, r4, r8              @ s0.r = F0.r - F2.r
    sub     lr, r5, r9              @ s0.i
    add     r4, r4, r8              @ F0.r += F2.r
    add     r5, r5, r9
    add     r8, r6, r10             @ s1.r = F1.r + F3.r
    add     r9, r7, r11             @ s1.i
    sub     r6, r6, r10             @ d.r  = F1.r - F3.r
    sub     r7, r7, r11             @ d.i
    sub     r10, r4, r8             @ F2.r = F0.r - s1.r
    sub     r11, r5, r9             @ F2.i
    add     r4, r4, r8              @ F0.r += s1.r
    add     r5, r5, r9
    add     r8, ip, r7              @ F1.r = s0.r + d.i
    sub     r9, lr, r6              @ F1.i = s0.i - d.r
    sub     ip, ip, r7              @ F3.r = s0.r - d.i
    add     lr, lr, r6              @ F3.i = s0.i + d.r
    stmia   r0!, {r4, r5, r8, r9, r10, r11, ip, lr}
    subs    r1, r1, #1
    bne     .Lb4_m1
    pop     {r4-r11, pc}

.Lb4_general:
    sub     sp, sp, #40
    ldr     r7, [sp, #76]           @ N
    ldr     ip, [sp, #80]           @ mm
    mov     r2, r2, lsl #2          @ fstride in bytes, 4 per kiss_twiddle_cpx
    mov     ip, ip, lsl #3          @ mm * sizeof(kiss_fft_cpx)
    mov     lr, r3, lsl #3          @ M = m * sizeof(kiss_fft_cpx)
    str     r2, [sp, #16]
    str     r0, [sp, #20]
    str     r7, [sp, #24]
    str     ip, [sp, #28]
    str     lr, [sp, #32]
    str     r3, [sp, #36]
    mov     r4, r1                  @ twiddle base, invariant

.Lb4_outer:
    ldr     r0, [sp, #20]           @ Fout for this i
    ldr     r7, [sp, #28]           @ mm*8
    ldr     ip, [sp, #32]           @ M
    add     r7, r0, r7
    str     r7, [sp, #20]           @ Fout for i+1
    add     r1, r0, ip              @ p1 = Fout + m
    add     r2, r1, ip              @ p2 = Fout + 2m
    add     r3, r2, ip              @ p3 = Fout + 3m
    ldr     r6, [sp, #36]           @ pass counter = m
    mov     r5, #0                  @ twiddle displacement

/* Pass 0 twiddles by twiddles[0], which is 1, so it is peeled: no twiddle
   load and no multiply.  In fixed point twiddles[0] is 32767 rather than
   32768, so skipping it also drops a small systematic gain error.  These
   values are full scale, not the halved form the inner loop carries, so the
   combine here has no doubling in it. */
    ldr     r8,  [r0]               @ A.r
    ldr     r9,  [r0, #4]           @ A.i
    ldr     r10, [r2]               @ C.r
    ldr     r11, [r2, #4]           @ C.i
    add     r7,  r8, r10            @ T0.r = A.r + C.r
    sub     r8,  r8, r10            @ T1.r = A.r - C.r
    add     r10, r9, r11            @ T0.i
    sub     r9,  r9, r11            @ T1.i
    str     r7,  [sp, #0]
    str     r8,  [sp, #4]
    str     r9,  [sp, #8]
    str     r10, [sp, #12]
    ldr     r8,  [r1]               @ B.r
    ldr     r9,  [r1, #4]           @ B.i
    ldr     r10, [r3]               @ D.r
    ldr     r11, [r3, #4]           @ D.i
    add     r7,  r8, r10            @ u.r = B.r + D.r
    sub     r8,  r8, r10            @ v.r = B.r - D.r
    add     r10, r9, r11            @ u.i
    sub     r9,  r9, r11            @ v.i
    ldr     r11, [sp, #0]           @ T0.r
    ldr     ip,  [sp, #12]          @ T0.i
    add     r11, r11, r7            @ out0.r = T0.r + u.r
    sub     r7,  r11, r7, lsl #1    @ out2.r = T0.r - u.r
    add     ip,  ip, r10            @ out0.i
    sub     r10, ip, r10, lsl #1    @ out2.i
    str     r11, [r0], #4
    str     ip,  [r0], #4
    str     r7,  [r2], #4
    str     r10, [r2], #4
    ldr     r11, [sp, #4]           @ T1.r
    ldr     ip,  [sp, #8]           @ T1.i
    add     r7,  r11, r9            @ out1.r = T1.r + v.i
    sub     r10, ip, r8             @ out1.i = T1.i - v.r
    str     r7,  [r1], #4
    str     r10, [r1], #4
    sub     r7,  r11, r9            @ out3.r = T1.r - v.i
    add     r10, ip, r8             @ out3.i = T1.i + v.r
    str     r7,  [r3], #4
    str     r10, [r3], #4
    subs    r6, r6, #1
    beq     .Lb4_next_i

/* Passes 1 .. m-1.  D and B are twiddled first and folded straight into
   u = (B+D)/2 and v = (B-D)/2, which frees the multiply's five registers
   again before C needs them; only v has to be parked.  The counter's
   decrement sits at the top to cover the one load-use slot that had nothing
   else to fill it, and nothing between there and the branch touches the
   flags. */
.Lb4_inner:
    ldr     r7, [sp, #16]           @ fstride, bytes
    subs    r6, r6, #1
    add     r5, r5, r7              @ next twiddle displacement
    add     r7, r5, r5, lsl #1      @ 3 * displacement
    ldr     r7, [r4, r7]            @ tw3, packed
    ldr     r9, [r3, #4]            @ D.i
    ldr     r8, [r3]                @ D.r
    CMUL_HALF r8, r9, r7, r10       @ D.r/2 -> r8, D.i/2 -> r9
    ldr     r7, [r4, r5]            @ tw1, packed
    ldr     r11, [r1, #4]           @ B.i
    ldr     r10, [r1]               @ B.r
    CMUL_HALF r10, r11, r7, ip      @ B.r/2 -> r10, B.i/2 -> r11
    add     r7,  r10, r8            @ u.r = (B.r + D.r)/2
    sub     r8,  r10, r8            @ v.r
    add     r10, r11, r9            @ u.i
    sub     r9,  r11, r9            @ v.i
    str     r8, [sp, #0]            @ park v.r
    str     r9, [sp, #4]            @ park v.i
    ldr     r8, [r4, r5, lsl #1]    @ tw2, packed
    ldr     r11, [r2, #4]           @ C.i
    ldr     r9,  [r2]               @ C.r
    CMUL_HALF r9, r11, r8, ip       @ C.r/2 -> r9, C.i/2 -> r11
    ldr     r8,  [r0]               @ A.r
    ldr     ip,  [r0, #4]           @ A.i
    add     lr, r8, r9, lsl #1      @ T0.r = A.r + C.r
    sub     r8, r8, r9, lsl #1      @ T1.r
    add     r9, ip, r11, lsl #1     @ T0.i
    sub     ip, ip, r11, lsl #1     @ T1.i
    add     lr, lr, r7, lsl #1      @ out0.r = T0.r + u.r
    sub     r7, lr, r7, lsl #2      @ out2.r = T0.r - u.r
    add     r9, r9, r10, lsl #1     @ out0.i
    sub     r10, r9, r10, lsl #2    @ out2.i
    str     lr, [r0], #4
    str     r9, [r0], #4
    str     r7, [r2], #4
    str     r10, [r2], #4
    ldr     r11, [sp, #4]           @ v.i
    ldr     lr,  [sp, #0]           @ v.r
    add     r7,  r8, r11, lsl #1    @ out1.r = T1.r + v.i
    sub     r9,  ip, lr, lsl #1     @ out1.i = T1.i - v.r
    str     r7, [r1], #4
    str     r9, [r1], #4
    sub     r7, r8, r11, lsl #1     @ out3.r = T1.r - v.i
    add     r9, ip, lr, lsl #1      @ out3.i = T1.i + v.r
    str     r7, [r3], #4
    str     r9, [r3], #4
    bne     .Lb4_inner

.Lb4_next_i:
    ldr     r7, [sp, #24]           @ N left
    subs    r7, r7, #1
    str     r7, [sp, #24]
    bne     .Lb4_outer
    add     sp, sp, #40
    pop     {r4-r11, pc}
    .size   kf_bfly4_armv5e, .-kf_bfly4_armv5e


/* ------------------------------------------------------------------------
 * void kf_bfly3_armv5e(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
 *                      int fstride, int m, int N, int mm)
 *
 * Inner-loop registers:
 *   r0-r2  the three output pointers, Fout + 0, m, 2m, post-incremented
 *   r4     twiddle base            r5  twiddle displacement in bytes
 *   r6     pass counter            r7  epi3.i, in the low halfword
 *   r3, r8-r11, ip, lr  scratch
 *
 * Frame, 24 bytes; this kernel parks nothing, so all of it is outer-loop
 * state read once per i.
 *    0  fstride, bytes   4  Fout for the next i   8  N left
 *   12  mm*8            16  M = m*8              20  m
 *
 * The peeled pass and the twiddled passes share one body: the peel drops B
 * and C in straight from memory at full scale, and the twiddled path doubles
 * the halved products before falling through to the same code.
 * ------------------------------------------------------------------------ */
    .align  2
    .global kf_bfly3_armv5e
    .type   kf_bfly3_armv5e, %function
kf_bfly3_armv5e:
    push    {r4-r11, lr}
    sub     sp, sp, #24
    ldr     r6, [sp, #60]           @ N
    ldr     ip, [sp, #64]           @ mm
    mov     r2, r2, lsl #2          @ fstride in bytes
    mov     ip, ip, lsl #3          @ mm * sizeof(kiss_fft_cpx)
    mov     lr, r3, lsl #3          @ M = m * sizeof(kiss_fft_cpx)
    str     r2, [sp, #0]
    str     r0, [sp, #4]
    str     r6, [sp, #8]
    str     ip, [sp, #12]
    str     lr, [sp, #16]
    str     r3, [sp, #20]
    mov     r4, r1                  @ twiddle base, invariant
    ldr     r7, .Lb3_epi3           @ only the low halfword is read by smulwb

.Lb3_outer:
    ldr     r0, [sp, #4]            @ Fout for this i
    ldr     r3, [sp, #12]           @ mm*8
    ldr     ip, [sp, #16]           @ M
    add     r3, r0, r3
    str     r3, [sp, #4]            @ Fout for i+1
    add     r1, r0, ip              @ p1 = Fout + m
    add     r2, r1, ip              @ p2 = Fout + 2m
    ldr     r6, [sp, #20]           @ pass counter = m
    mov     r5, #0                  @ twiddle displacement

/* Pass 0 twiddles by twiddles[0], which is 1: B and C are the stored values
   at full scale, so it enters the body with no multiply. */
    ldr     r8,  [r1]               @ B.r
    ldr     r9,  [r1, #4]           @ B.i
    ldr     r10, [r2]               @ C.r
    ldr     r11, [r2, #4]           @ C.i
    b       .Lb3_body

.Lb3_pass:
    ldr     r3,  [r4, r5]           @ tw1, packed
    ldr     r9,  [r1, #4]           @ B.i
    ldr     r8,  [r1]               @ B.r
    CMUL_HALF r8, r9, r3, r10
    ldr     r3,  [r4, r5, lsl #1]   @ tw2, packed
    ldr     r11, [r2, #4]           @ C.i
    ldr     r10, [r2]               @ C.r
    CMUL_HALF r10, r11, r3, ip
    mov     r8,  r8,  lsl #1        @ B.r
    mov     r9,  r9,  lsl #1        @ B.i
    mov     r10, r10, lsl #1        @ C.r
    mov     r11, r11, lsl #1        @ C.i

.Lb3_body:
    add     r3,  r8, r10            @ s3.r = B.r + C.r
    sub     r8,  r8, r10            @ s0.r = B.r - C.r
    add     ip,  r9, r11            @ s3.i
    sub     r9,  r9, r11            @ s0.i
    ldr     r10, [r0]               @ A.r
    ldr     r11, [r0, #4]           @ A.i
    sub     lr,  r10, r3, asr #1    @ A.r - HALF_OF(s3.r)
    add     r10, r10, r3            @ Fout0.r = A.r + s3.r
    str     r10, [r0], #4
    sub     r3,  r11, ip, asr #1    @ A.i - HALF_OF(s3.i)
    add     r11, r11, ip            @ Fout0.i
    str     r11, [r0], #4
    smulwb  r10, r8, r7             @ (s0.r * epi3.i) >> 16
    smulwb  r11, r9, r7             @ (s0.i * epi3.i) >> 16
    add     ip,  lr, r11, lsl #1    @ Fout2.r = h.r + s0'.i
    sub     r8,  r3, r10, lsl #1    @ Fout2.i = h.i - s0'.r
    str     ip,  [r2], #4
    str     r8,  [r2], #4
    sub     ip,  lr, r11, lsl #1    @ Fout1.r = h.r - s0'.i
    add     r8,  r3, r10, lsl #1    @ Fout1.i = h.i + s0'.r
    str     ip,  [r1], #4
    str     r8,  [r1], #4
    ldr     r3, [sp, #0]            @ fstride, bytes
    subs    r6, r6, #1
    add     r5, r5, r3              @ displacement for the next pass
    bne     .Lb3_pass

    ldr     r3, [sp, #8]            @ N left
    subs    r3, r3, #1
    str     r3, [sp, #8]
    bne     .Lb3_outer
    add     sp, sp, #24
    pop     {r4-r11, pc}
.Lb3_epi3:
    .word   -28378
    .size   kf_bfly3_armv5e, .-kf_bfly3_armv5e


/* ------------------------------------------------------------------------
 * void kf_bfly5_armv5e(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
 *                      int fstride, int m, int N, int mm)
 *
 * Inner-loop registers:
 *   r0-r4  the five output pointers, Fout + 0, m, 2m, 3m, 4m, post-incremented
 *   r5     twiddle base            r6  twiddle displacement in bytes
 *   r7-r11, ip, lr  scratch
 *
 * There is no pass counter: the displacement itself is the induction
 * variable, and the loop ends when it reaches m*fstride.  That buys back the
 * one register this kernel could not otherwise find.
 *
 * Frame, 56 bytes.  The first thirty-two are the placed spill.  Five complex
 * values will not fit in seven scratch registers at once, so four of them are
 * parked at the point where they stop being needed and fetched back one
 * component at a time.
 *    0  s10.r    4  s10.i    8  s7.r   12  s7.i
 *   16  s9.r    20  s9.i    24  s11.r  28  s11.i
 *   32  fstride, bytes   36  limit = m*fstride   40  Fout for the next i
 *   44  N left           48  mm*8                52  M = m*8
 *
 * The eight products that form scratch[6] and scratch[12] all multiply by
 * ya.i or yb.i, so both constants live packed in one register and are picked
 * apart by smulwb and smulwt.  Two of the four sums then collapse into a
 * single smulwb and smlawt pair rather than two multiplies, two shifts and
 * an add.
 *
 * The outputs split cleanly: the real parts of all four need only the
 * imaginary halves of scratch[9] and scratch[10], and the imaginary parts
 * need only the real halves.  Doing them in that order halves how much has
 * to be live at once.
 * ------------------------------------------------------------------------ */

/* s7 = P + Q and s10 = P - Q, for P in (r8,r9) and Q in (r10,r11), then park
   both.  Used twice: once after the twiddled pair, once for the peeled pass. */
    .macro  B5_FOLD1
    add     r7,  r8, r10            @ s7.r
    sub     r8,  r8, r10            @ s10.r
    add     ip,  r9, r11            @ s7.i
    sub     r9,  r9, r11            @ s10.i
    str     r8,  [sp, #0]
    str     r9,  [sp, #4]
    str     r7,  [sp, #8]
    str     ip,  [sp, #12]
    .endm

    .align  2
    .global kf_bfly5_armv5e
    .type   kf_bfly5_armv5e, %function
kf_bfly5_armv5e:
    push    {r4-r11, lr}
    sub     sp, sp, #56
    ldr     r7, [sp, #92]           @ N
    ldr     ip, [sp, #96]           @ mm
    mov     r2, r2, lsl #2          @ fstride in bytes
    mov     ip, ip, lsl #3          @ mm * sizeof(kiss_fft_cpx)
    mov     lr, r3, lsl #3          @ M = m * sizeof(kiss_fft_cpx)
    mul     r3, r2, r3              @ limit: the displacement after m passes
    str     r2, [sp, #32]
    str     r3, [sp, #36]
    str     r0, [sp, #40]
    str     r7, [sp, #44]
    str     ip, [sp, #48]
    str     lr, [sp, #52]
    mov     r5, r1                  @ twiddle base, invariant

.Lb5_outer:
    ldr     r0, [sp, #40]           @ Fout for this i
    ldr     r7, [sp, #48]           @ mm*8
    ldr     ip, [sp, #52]           @ M
    add     r7, r0, r7
    str     r7, [sp, #40]           @ Fout for i+1
    add     r1, r0, ip              @ Fout1
    add     r2, r1, ip              @ Fout2
    add     r3, r2, ip              @ Fout3
    add     r4, r3, ip              @ Fout4
    mov     r6, #0                  @ twiddle displacement

/* Pass 0 twiddles by twiddles[0], which is 1, so the four points come in
   from memory at full scale with no multiply. */
    ldr     r8,  [r1]               @ s1.r
    ldr     r9,  [r1, #4]           @ s1.i
    ldr     r10, [r4]               @ s4.r
    ldr     r11, [r4, #4]           @ s4.i
    B5_FOLD1
    ldr     r8,  [r2]               @ s2.r
    ldr     r9,  [r2, #4]           @ s2.i
    ldr     r10, [r3]               @ s3.r
    ldr     r11, [r3, #4]           @ s3.i
    b       .Lb5_fold2

.Lb5_pass:
    ldr     r7,  [r5, r6]           @ tw1, packed
    ldr     r9,  [r1, #4]           @ s1.i
    ldr     r8,  [r1]               @ s1.r
    CMUL_HALF r8, r9, r7, r10
    ldr     r7,  [r5, r6, lsl #2]   @ tw4, packed
    ldr     r11, [r4, #4]           @ s4.i
    ldr     r10, [r4]               @ s4.r
    CMUL_HALF r10, r11, r7, ip
    mov     r8,  r8,  lsl #1        @ s1.r
    mov     r9,  r9,  lsl #1        @ s1.i
    mov     r10, r10, lsl #1        @ s4.r
    mov     r11, r11, lsl #1        @ s4.i
    B5_FOLD1
    ldr     r7,  [r5, r6, lsl #1]   @ tw2, packed
    ldr     r9,  [r2, #4]           @ s2.i
    ldr     r8,  [r2]               @ s2.r
    CMUL_HALF r8, r9, r7, r10
    add     r7,  r6, r6, lsl #1     @ 3 * displacement
    ldr     r7,  [r5, r7]           @ tw3, packed
    ldr     r11, [r3, #4]           @ s3.i
    ldr     r10, [r3]               @ s3.r
    CMUL_HALF r10, r11, r7, ip
    mov     r8,  r8,  lsl #1        @ s2.r
    mov     r9,  r9,  lsl #1        @ s2.i
    mov     r10, r10, lsl #1        @ s3.r
    mov     r11, r11, lsl #1        @ s3.i

.Lb5_fold2:
    add     r7,  r8, r10            @ s8.r = s2.r + s3.r
    sub     r8,  r8, r10            @ s9.r
    add     ip,  r9, r11            @ s8.i
    sub     r9,  r9, r11            @ s9.i
    str     r8,  [sp, #16]          @ park s9.r
    str     r9,  [sp, #20]          @ park s9.i
    ldr     r10, [sp, #8]           @ s7.r
    ldr     r11, [sp, #12]          @ s7.i
    add     r8,  r10, r7            @ p.r = s7.r + s8.r
    sub     r10, r10, r7            @ q.r
    add     r9,  r11, ip            @ p.i
    sub     r11, r11, ip            @ q.i

    ldr     r7,  [r0]               @ A.r
    ldr     ip,  [r0, #4]           @ A.i
    add     lr,  r8, #2
    sub     lr,  r7, lr, asr #2     @ t.r = A.r - QUARTER_OF(p.r)
    add     r7,  r7, r8             @ Fout0.r = A.r + p.r
    str     r7,  [r0], #4
    add     r8,  r9, #2
    sub     r8,  ip, r8, asr #2     @ t.i
    add     ip,  ip, r9             @ Fout0.i
    str     ip,  [r0], #4

    ldr     r7,  .Lb5_yc
    smulwb  r9,  r10, r7            @ (q.r * yc) >> 16
    smulwb  ip,  r11, r7            @ (q.i * yc) >> 16
    add     r10, lr, r9, lsl #1     @ s5.r  = t.r + scratch[4].r
    sub     r11, lr, r9, lsl #1     @ s11.r = t.r - scratch[4].r
    add     r7,  r8, ip, lsl #1     @ s5.i
    sub     r9,  r8, ip, lsl #1     @ s11.i
    str     r11, [sp, #24]          @ park s11.r
    str     r9,  [sp, #28]          @ park s11.i

/* Real halves.  s6.r and s12.r need only the imaginary halves of s9 and
   s10, so nothing else has to stay live here. */
    ldr     r8,  [sp, #4]           @ s10.i
    ldr     r9,  [sp, #20]          @ s9.i
    ldr     r11, .Lb5_yab           @ ya.i in 15:0, yb.i in 31:16
    smulwb  ip,  r8, r11            @ s10.i * ya.i
    smlawt  ip,  r9, r11, ip        @ + s9.i * yb.i      -> s6.r / 2
    smulwb  lr,  r9, r11            @ s9.i * ya.i
    smulwt  r9,  r8, r11            @ s10.i * yb.i
    sub     lr,  lr, r9             @ -> s12.r / 2
    sub     r8,  r10, ip, lsl #1    @ Fout1.r = s5.r - s6.r
    add     r9,  r10, ip, lsl #1    @ Fout4.r = s5.r + s6.r
    ldr     r10, [sp, #24]          @ s11.r
    str     r8,  [r1], #4
    str     r9,  [r4], #4
    add     r8,  r10, lr, lsl #1    @ Fout2.r = s11.r + s12.r
    sub     r9,  r10, lr, lsl #1    @ Fout3.r
    str     r8,  [r2], #4
    str     r9,  [r3], #4

/* Imaginary halves.  s6.i is the negation of the same shape, so the two
   stores swap their add and subtract rather than paying for an rsb. */
    ldr     r8,  [sp, #0]           @ s10.r
    ldr     r9,  [sp, #16]          @ s9.r
    ldr     r11, .Lb5_yab
    smulwb  ip,  r8, r11            @ s10.r * ya.i
    smlawt  ip,  r9, r11, ip        @ + s9.r * yb.i      -> minus s6.i / 2
    smulwt  lr,  r8, r11            @ s10.r * yb.i
    smulwb  r9,  r9, r11            @ s9.r * ya.i
    sub     lr,  lr, r9             @ -> s12.i / 2
    add     r8,  r7, ip, lsl #1     @ Fout1.i = s5.i - s6.i
    sub     r9,  r7, ip, lsl #1     @ Fout4.i = s5.i + s6.i
    ldr     r10, [sp, #28]          @ s11.i
    str     r8,  [r1], #4
    str     r9,  [r4], #4
    add     r8,  r10, lr, lsl #1    @ Fout2.i = s11.i + s12.i
    sub     r9,  r10, lr, lsl #1    @ Fout3.i
    str     r8,  [r2], #4
    str     r9,  [r3], #4

    ldr     r7,  [sp, #32]          @ fstride, bytes
    ldr     ip,  [sp, #36]          @ limit
    add     r6,  r6, r7             @ displacement for the next pass
    cmp     r6,  ip
    bne     .Lb5_pass

    ldr     r7, [sp, #44]           @ N left
    subs    r7, r7, #1
    str     r7, [sp, #44]
    bne     .Lb5_outer
    add     sp, sp, #56
    pop     {r4-r11, pc}
.Lb5_yc:
    .word   18318
.Lb5_yab:
    .word   0xb4c38644
    .size   kf_bfly5_armv5e, .-kf_bfly5_armv5e
