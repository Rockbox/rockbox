/* ARMv4 FFT butterflies for the CELT fixed-point kiss_fft.
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
 * On ARM7TDMI a single load costs 1S+1N+1I = 3 cycles and a store 2N = 2,
 * while a load multiple of n registers costs n+2 and a store multiple n+1.
 * Every extra word moved in a burst is two cycles cheaper to load and one
 * cheaper to store, and on the PP502x, where instructions and data share one
 * cache, a burst arbitrates against instruction fetch once instead of n
 * times.
 *
 * gcc cannot be steered into that from C.  It reorders the accesses while
 * scheduling and does not hand out ascending register pairs, so contiguous
 * loads come out as separate ldr.  Worse, the radix kernels need more live
 * values than ARM has registers, and gcc resolves that by spilling the
 * loop-invariant pointers and strides and reloading them every pass: the
 * compiled radix-4 body reloads eleven words from its frame per iteration.
 *
 * These routines invert that.  Every pointer, twiddle pointer and counter
 * stays in a register for the whole inner loop, and the transient butterfly
 * values are what goes to the frame, moved in ldm/stm bursts.  The spill is
 * genuinely unavoidable -- a radix-4 pass needs four transformed points plus
 * a complex multiply in flight, which is more than fourteen words -- but
 * here it is placed rather than accidental.
 *
 * Complex multiply register discipline follows the C_MUL asm already in
 * kiss_fft_armv4.h: the twiddle stays a 16-bit value in Rs, and the Q15
 * narrowing keeps all 32 bits of the product.  MULT16_32_Q15_armv4, used by
 * the C C_MULBYSCALAR, drops the low bit for speed, so the radix-3 twiddle
 * by epi3 comes out one LSB more accurate here than in the C path.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "kiss_fft_armv4_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

/* ------------------------------------------------------------------------
 * void kf_bfly4_armv4(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
 *                     int fstride, int m, int N, int mm)
 *
 *   r0 = Fout   r1 = tw   r2 = fstride   r3 = m   [sp+0] = N   [sp+4] = mm
 * ------------------------------------------------------------------------ */
    .global kf_bfly4_armv4
    .type   kf_bfly4_armv4, %function
kf_bfly4_armv4:
    push    {r4-r11, lr}
    cmp     r3, #1
    bne     .Lb4_general

/* Degenerate case: every twiddle is 1, so four consecutive complex points --
   eight consecutive words -- go in and eight come out with no multiplies at
   all.  This is the one kernel that fits entirely in registers, so it is one
   ldm of eight and one stm of eight per group. */
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

/* Frame, 52 bytes.  The first six words are the placed spill; everything
   from 24 on is outer-loop state, read once per i and never inside the
   inner loop.
      0  T1.r   4  T1.i   8  T0.r  12  T0.i  16  D.r  20  D.i
   The outer-loop half is ordered to match r0-r6 at the end of the prologue,
   so one stm stores it: 24 Fout, 28 tw, 32 fstride, 36 m, 40 N, 44 mm*8,
   48 m*8. */
.Lb4_general:
    sub     sp, sp, #52
    ldr     r4, [sp, #88]           @ N
    ldr     r5, [sp, #92]           @ mm
    mov     r2, r2, lsl #2          @ fstride in bytes (4 per kiss_twiddle_cpx)
    mov     r5, r5, lsl #3          @ mm * sizeof(kiss_fft_cpx)
    mov     r6, r3, lsl #3          @ M = m * sizeof(kiss_fft_cpx)
    add     ip, sp, #24
    stmia   ip, {r0-r6}

.Lb4_outer:
    ldr     r0, [sp, #24]           @ Fout for this i
    ldr     r5, [sp, #44]           @ mm*8
    add     r5, r0, r5
    str     r5, [sp, #24]           @ Fout for i+1
    ldr     r5, [sp, #48]           @ M
    add     r1, r0, r5              @ p1 = Fout + m
    add     r2, r1, r5              @ p2 = Fout + 2m
    add     r3, r2, r5              @ p3 = Fout + 3m
    ldr     r4, [sp, #28]           @ tw1 = twiddles
    mov     r5, r4                  @ tw3 = twiddles
    ldr     r7, [sp, #36]           @ inner count = m

/* Pass 0 twiddles by twiddles[0], which is 1, so it is peeled: no twiddle
   load and no multiply.  In fixed point twiddles[0] is 32767 rather than
   32768, so skipping it also drops a small systematic gain error. */
    ldm     r0, {r8, r9}            @ A
    ldm     r2, {r6, r11}           @ C
    add     r10, r8, r6             @ T0.r = A.r + C.r
    sub     r8, r8, r6              @ T1.r = A.r - C.r
    add     ip, r9, r11             @ T0.i
    sub     r9, r9, r11             @ T1.i
    stmia   sp, {r8, r9, r10, ip}
    ldm     r1, {r6, r11}           @ B
    ldm     r3, {r8, r9}            @ D
    b       .Lb4_combine

.Lb4_inner:
    ldr     ip, [sp, #32]           @ fstride, the one loop-invariant reload
    add     r4, r4, ip              @ tw1 += fstride
    add     r5, r5, ip, lsl #1
    add     r5, r5, ip              @ tw3 += 3*fstride
    /* tw2 sits halfway between the two.  Deriving it keeps a register free;
       written as a difference so the address arithmetic cannot overflow. */
    sub     ip, r5, r4
    add     ip, r4, ip, lsr #1

    /* C = tw2 * Fout[2m] */
    ldm     r2, {r8, r9}
    ldrsh   r10, [ip]
    ldrsh   ip, [ip, #2]
    smull   lr, r11, r9, r10
    smlal   lr, r11, r8, ip
    mov     lr, lr, lsr #15
    orr     r11, lr, r11, lsl #17   @ C.i
    rsb     ip, ip, #0
    smull   r10, r6, r8, r10
    smlal   r10, r6, r9, ip
    mov     r10, r10, lsr #15
    orr     r6, r10, r6, lsl #17    @ C.r

    ldm     r0, {r8, r9}            @ A
    add     r10, r8, r6             @ T0.r
    sub     r8, r8, r6              @ T1.r
    add     ip, r9, r11             @ T0.i
    sub     r9, r9, r11             @ T1.i
    stmia   sp, {r8, r9, r10, ip}   @ park T1, T0

    /* D = tw3 * Fout[3m] */
    ldm     r3, {r8, r9}
    ldrsh   r10, [r5]
    ldrsh   ip, [r5, #2]
    smull   lr, r11, r9, r10
    smlal   lr, r11, r8, ip
    mov     lr, lr, lsr #15
    orr     r11, lr, r11, lsl #17   @ D.i
    rsb     ip, ip, #0
    smull   r10, r6, r8, r10
    smlal   r10, r6, r9, ip
    mov     r10, r10, lsr #15
    orr     r6, r10, r6, lsl #17    @ D.r
    add     lr, sp, #16
    stmia   lr, {r6, r11}           @ park D

    /* B = tw1 * Fout[m] */
    ldm     r1, {r8, r9}
    ldrsh   r10, [r4]
    ldrsh   ip, [r4, #2]
    smull   lr, r11, r9, r10
    smlal   lr, r11, r8, ip
    mov     lr, lr, lsr #15
    orr     r11, lr, r11, lsl #17   @ B.i
    rsb     ip, ip, #0
    smull   r10, r6, r8, r10
    smlal   r10, r6, r9, ip
    mov     r10, r10, lsr #15
    orr     r6, r10, r6, lsl #17    @ B.r
    add     lr, sp, #16
    ldm     lr, {r8, r9}            @ D back

/* Common tail: B in (r6, r11), D in (r8, r9), T0 and T1 on the frame. */
.Lb4_combine:
    add     r10, r6, r8             @ T2.r = B.r + D.r
    sub     r6, r6, r8              @ T3.r = B.r - D.r
    add     ip, r11, r9             @ T2.i
    sub     r11, r11, r9            @ T3.i
    ldr     r8, [sp, #8]            @ T0.r
    ldr     r9, [sp, #12]           @ T0.i
    add     r8, r8, r10             @ out0.r = T0.r + T2.r
    add     r9, r9, ip              @ out0.i
    sub     r10, r8, r10, lsl #1    @ out2.r = out0.r - 2*T2.r
    sub     ip, r9, ip, lsl #1      @ out2.i
    stm     r0, {r8, r9}
    stm     r2, {r10, ip}
    ldm     sp, {r8, r9}            @ T1
    add     r10, r8, r11            @ out1.r = T1.r + T3.i
    sub     ip, r9, r6              @ out1.i = T1.i - T3.r
    stm     r1, {r10, ip}
    sub     r10, r8, r11            @ out3.r = T1.r - T3.i
    add     ip, r9, r6              @ out3.i = T1.i + T3.r
    stm     r3, {r10, ip}

    add     r0, r0, #8
    add     r1, r1, #8
    add     r2, r2, #8
    add     r3, r3, #8
    subs    r7, r7, #1
    bne     .Lb4_inner

    ldr     r5, [sp, #40]           @ N
    subs    r5, r5, #1
    str     r5, [sp, #40]
    bne     .Lb4_outer
    add     sp, sp, #52
    pop     {r4-r11, pc}
    .size   kf_bfly4_armv4, .-kf_bfly4_armv4


/* ------------------------------------------------------------------------
 * void kf_bfly3_armv4(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
 *                     int fstride, int m, int N, int mm)
 *
 * Frame, 36 bytes.  The outer-loop half is ordered to match r0-r6 at the end
 * of the prologue, so one stm stores it:
 *    0  s1.r  4  s1.i  8  Fout  12  tw  16  fstride  20  m  24  N
 *   28  mm*8  32  m*8
 * ------------------------------------------------------------------------ */
    .align  2
    .global kf_bfly3_armv4
    .type   kf_bfly3_armv4, %function
kf_bfly3_armv4:
    push    {r4-r11, lr}
    sub     sp, sp, #36
    ldr     r4, [sp, #72]           @ N
    ldr     r5, [sp, #76]           @ mm
    mov     r2, r2, lsl #2          @ fstride in bytes
    mov     r5, r5, lsl #3
    mov     r6, r3, lsl #3          @ M
    add     ip, sp, #8
    stmia   ip, {r0-r6}
    ldr     r7, .Lb3_epi3

.Lb3_outer:
    ldr     r0, [sp, #8]            @ Fout for this i
    ldr     r4, [sp, #28]           @ mm*8
    add     r4, r0, r4
    str     r4, [sp, #8]
    ldr     r4, [sp, #32]           @ M
    add     r1, r0, r4              @ p1 = Fout + m
    add     r2, r1, r4              @ p2 = Fout + 2m
    ldr     r3, [sp, #12]           @ tw1 = twiddles
    mov     r4, r3                  @ tw2 = twiddles
    ldr     r6, [sp, #20]           @ inner count = m

    /* Pass 0: twiddles[0] is 1, so it is peeled. */
    ldm     r1, {r8, r9}            @ s1 = Fout[m]
    ldm     r2, {r5, r11}           @ s2 = Fout[2m]
    b       .Lb3_combine

.Lb3_inner:
    ldr     ip, [sp, #16]           @ fstride
    add     r3, r3, ip              @ tw1 += fstride
    add     r4, r4, ip, lsl #1      @ tw2 += 2*fstride

    /* s1 = tw1 * Fout[m] */
    ldm     r1, {r8, r9}
    ldrsh   r10, [r3]
    ldrsh   ip, [r3, #2]
    smull   lr, r11, r9, r10
    smlal   lr, r11, r8, ip
    mov     lr, lr, lsr #15
    orr     r11, lr, r11, lsl #17   @ s1.i
    rsb     ip, ip, #0
    smull   r10, r5, r8, r10
    smlal   r10, r5, r9, ip
    mov     r10, r10, lsr #15
    orr     r5, r10, r5, lsl #17    @ s1.r
    stmia   sp, {r5, r11}           @ park s1

    /* s2 = tw2 * Fout[2m] */
    ldm     r2, {r8, r9}
    ldrsh   r10, [r4]
    ldrsh   ip, [r4, #2]
    smull   lr, r11, r9, r10
    smlal   lr, r11, r8, ip
    mov     lr, lr, lsr #15
    orr     r11, lr, r11, lsl #17   @ s2.i
    rsb     ip, ip, #0
    smull   r10, r5, r8, r10
    smlal   r10, r5, r9, ip
    mov     r10, r10, lsr #15
    orr     r5, r10, r5, lsl #17    @ s2.r
    ldm     sp, {r8, r9}            @ s1 back

/* Common tail: s1 in (r8, r9), s2 in (r5, r11). */
.Lb3_combine:
    add     r10, r8, r5             @ s3.r = s1.r + s2.r
    sub     r8, r8, r5              @ s0.r = s1.r - s2.r
    add     ip, r9, r11             @ s3.i
    sub     r9, r9, r11             @ s0.i

    smull   r5, r11, r8, r7         @ s0.r *= epi3.i
    mov     r5, r5, lsr #15
    orr     r8, r5, r11, lsl #17
    smull   r5, r11, r9, r7         @ s0.i *= epi3.i
    mov     r5, r5, lsr #15
    orr     r9, r5, r11, lsl #17

    ldm     r0, {r5, r11}           @ Fout[0]
    add     r5, r5, r10             @ Fout[0] += s3
    add     r11, r11, ip
    stm     r0, {r5, r11}
    /* Recover the pre-add value and take Fout[0] - s3/2 from it.  Cheaper
       than holding the original in a register we do not have. */
    sub     r5, r5, r10
    sub     r5, r5, r10, asr #1
    sub     r11, r11, ip
    sub     r11, r11, ip, asr #1

    add     r10, r5, r9             @ Fout[2m].r = t.r + s0.i
    sub     ip, r11, r8             @ Fout[2m].i = t.i - s0.r
    stm     r2, {r10, ip}
    sub     r10, r5, r9             @ Fout[m].r  = t.r - s0.i
    add     ip, r11, r8             @ Fout[m].i  = t.i + s0.r
    stm     r1, {r10, ip}

    add     r0, r0, #8
    add     r1, r1, #8
    add     r2, r2, #8
    subs    r6, r6, #1
    bne     .Lb3_inner

    ldr     r4, [sp, #24]           @ N
    subs    r4, r4, #1
    str     r4, [sp, #24]
    bne     .Lb3_outer
    add     sp, sp, #36
    pop     {r4-r11, pc}
.Lb3_epi3:
    .word   -28378
    .size   kf_bfly3_armv4, .-kf_bfly3_armv4

/* ------------------------------------------------------------------------
 * void kf_bfly5_armv4(kiss_fft_cpx *Fout, const kiss_twiddle_cpx *tw,
 *                     int fstride, int m, int N, int mm)
 *
 * The radix-5 body needs five points plus a complex multiply live at once,
 * which no register file this size can hold, so the four transformed points
 * are staged through the frame in ldm/stm pairs.  What stays resident is all
 * the bookkeeping: the output pointer, the point stride, the twiddle base
 * and offset, and the counter.  Addresses for the other four points and
 * their twiddles are derived from those with shifted adds rather than
 * carried in registers, trading one cycle each for four registers.
 *
 * Frame, 68 bytes:
 *    0  s7.r    4  s7.i    8  s10.r  12  s10.i  16  s9.r  20  s9.i
 *   32  s5.r   36  s5.i   40  s11.r  44  s11.i
 *   48  Fout   52  m      56  N      60  mm*8   64  fstride
 * The outer-loop half is ordered so r0, r3, r5, r6 and r7 store with one stm.
 * ------------------------------------------------------------------------ */
    .align  2
    .global kf_bfly5_armv4
    .type   kf_bfly5_armv4, %function

/* One complex multiply.  Data in r6 (real) and r7 (imag), twiddle real in r8
   and imag in ip, lr as the 64-bit low word.  Results land in \mr and \mi.
   The register choices satisfy the ARMv4 rule that RdHi, RdLo and Rm differ;
   the second smull deliberately reuses Rs as RdLo, exactly as the C_MUL asm
   in kiss_fft_armv4.h does. */
    .macro  CMUL5 mr, mi
    smull   lr, \mi, r7, r8
    smlal   lr, \mi, r6, ip
    mov     lr, lr, lsr #15
    orr     \mi, lr, \mi, lsl #17
    rsb     ip, ip, #0
    smull   r8, \mr, r6, r8
    smlal   r8, \mr, r7, ip
    mov     r8, r8, lsr #15
    orr     \mr, r8, \mr, lsl #17
    .endm

kf_bfly5_armv4:
    push    {r4-r11, lr}
    sub     sp, sp, #68
    ldr     r5, [sp, #104]          @ N
    ldr     r6, [sp, #108]          @ mm
    mov     r7, r2, lsl #2          @ fstride in bytes
    mov     r2, r1                  @ twiddle base
    mov     r1, r3, lsl #3          @ M = m * sizeof(kiss_fft_cpx)
    mov     r6, r6, lsl #3
    add     ip, sp, #48
    stmia   ip, {r0, r3, r5, r6, r7}

.Lb5_outer:
    ldr     r0, [sp, #48]           @ Fout for this i
    ldr     r6, [sp, #60]           @ mm*8
    add     r6, r0, r6
    str     r6, [sp, #48]           @ Fout for i+1
    ldr     r5, [sp, #52]           @ inner count = m
    mov     r3, #0                  @ twiddle offset

    /* Pass 0: every twiddle is 1, so the four points are used as they are. */
    add     ip, r0, r1
    ldm     ip, {r4, r11}           @ s1
    add     ip, r0, r1, lsl #2
    ldm     ip, {r9, r10}           @ s4
    add     r6, r4, r9              @ s7.r  = s1.r + s4.r
    add     r8, r11, r10            @ s7.i
    sub     r4, r4, r9              @ s10.r = s1.r - s4.r
    sub     r11, r11, r10           @ s10.i
    stmia   sp, {r6, r8}
    add     ip, sp, #8
    stmia   ip, {r4, r11}
    add     ip, r0, r1, lsl #1
    ldm     ip, {r4, r11}           @ s2
    add     ip, r0, r1
    add     ip, ip, r1, lsl #1
    ldm     ip, {r9, r10}           @ s3
    add     r6, r4, r9              @ s8.r  = s2.r + s3.r
    add     r8, r11, r10            @ s8.i
    sub     r4, r4, r9              @ s9.r  = s2.r - s3.r
    sub     r11, r11, r10           @ s9.i
    add     ip, sp, #16
    stmia   ip, {r4, r11}
    b       .Lb5_combine

.Lb5_inner:
    ldr     ip, [sp, #64]           @ fstride, the one loop-invariant reload
    add     r3, r3, ip              @ twiddle offset for this pass

    /* s1 = tw[u] * Fout[m] */
    add     ip, r0, r1
    ldm     ip, {r6, r7}
    add     ip, r2, r3
    ldrsh   r8, [ip]
    ldrsh   ip, [ip, #2]
    CMUL5   r4, r11

    /* s4 = tw[4u] * Fout[4m] */
    add     ip, r0, r1, lsl #2
    ldm     ip, {r6, r7}
    add     ip, r2, r3, lsl #2
    ldrsh   r8, [ip]
    ldrsh   ip, [ip, #2]
    CMUL5   r9, r10

    add     r6, r4, r9              @ s7.r
    add     r8, r11, r10            @ s7.i
    sub     r4, r4, r9              @ s10.r
    sub     r11, r11, r10           @ s10.i
    stmia   sp, {r6, r8}
    add     ip, sp, #8
    stmia   ip, {r4, r11}

    /* s2 = tw[2u] * Fout[2m] */
    add     ip, r0, r1, lsl #1
    ldm     ip, {r6, r7}
    add     ip, r2, r3, lsl #1
    ldrsh   r8, [ip]
    ldrsh   ip, [ip, #2]
    CMUL5   r4, r11

    /* s3 = tw[3u] * Fout[3m] */
    add     ip, r0, r1
    add     ip, ip, r1, lsl #1
    ldm     ip, {r6, r7}
    add     ip, r2, r3
    add     ip, ip, r3, lsl #1
    ldrsh   r8, [ip]
    ldrsh   ip, [ip, #2]
    CMUL5   r9, r10

    add     r6, r4, r9              @ s8.r
    add     r8, r11, r10            @ s8.i
    sub     r4, r4, r9              @ s9.r
    sub     r11, r11, r10           @ s9.i
    add     ip, sp, #16
    stmia   ip, {r4, r11}

/* Common tail: s8 in (r6, r8); s7, s10 and s9 on the frame. */
.Lb5_combine:
    ldm     sp, {r9, r10}           @ s7
    add     r4, r9, r6              @ sum  = s7 + s8
    add     r11, r10, r8
    sub     r9, r9, r6              @ diff = s7 - s8
    sub     r10, r10, r8

    ldm     r0, {r6, r7}            @ Fout[0]
    add     r6, r6, r4              @ Fout[0] += sum
    add     r7, r7, r11
    stm     r0, {r6, r7}
    /* Recover the pre-add value; the term common to both output pairs is
       Fout[0] - sum/4, and QUARTER_OF rounds rather than truncating. */
    sub     r6, r6, r4
    sub     r7, r7, r11
    add     r4, r4, #2
    add     r11, r11, #2
    sub     r6, r6, r4, asr #2      @ t.r
    sub     r7, r7, r11, asr #2     @ t.i

    /* diff *= yc, where yc = sqrt(5)/4 absorbs the four cosine products. */
    ldr     ip, .Lb5_yc
    smull   r4, r8, r9, ip
    mov     r4, r4, lsr #15
    orr     r9, r4, r8, lsl #17
    smull   r4, r8, r10, ip
    mov     r4, r4, lsr #15
    orr     r10, r4, r8, lsl #17

    add     r4, r6, r9              @ s5  = t + diff*yc
    add     r8, r7, r10
    sub     r6, r6, r9              @ s11 = t - diff*yc
    sub     r7, r7, r10
    add     ip, sp, #32
    stmia   ip, {r4, r8}
    add     ip, sp, #40
    stmia   ip, {r6, r7}

    /* The sine terms.  Each output component is a pair of products over the
       same operands, so they accumulate into one smull/smlal instead of
       narrowing twice. */
    add     ip, sp, #8
    ldm     ip, {r6, r7}            @ s10
    add     ip, sp, #16
    ldm     ip, {r8, r9}            @ s9
    ldr     r10, .Lb5_yai
    ldr     r11, .Lb5_ybi

    smull   lr, ip, r7, r10         @ s6.r = s10.i*ya.i + s9.i*yb.i
    smlal   lr, ip, r9, r11
    mov     lr, lr, lsr #15
    orr     r4, lr, ip, lsl #17

    rsb     r11, r11, #0
    smull   lr, ip, r9, r10         @ s12.r = s9.i*ya.i - s10.i*yb.i
    smlal   lr, ip, r7, r11
    mov     lr, lr, lsr #15
    orr     r7, lr, ip, lsl #17
    rsb     r11, r11, #0

    smull   lr, ip, r6, r10         @ s6.i = -(s10.r*ya.i + s9.r*yb.i)
    smlal   lr, ip, r8, r11
    mov     lr, lr, lsr #15
    orr     ip, lr, ip, lsl #17
    rsb     r9, ip, #0

    rsb     r10, r10, #0
    smull   lr, ip, r6, r11         @ s12.i = s10.r*yb.i - s9.r*ya.i
    smlal   lr, ip, r8, r10
    mov     lr, lr, lsr #15
    orr     r10, lr, ip, lsl #17

    add     ip, sp, #32
    ldm     ip, {r6, r8}            @ s5
    sub     r11, r6, r4             @ Fout[m]  = s5 - s6
    sub     ip, r8, r9
    add     r4, r6, r4              @ Fout[4m] = s5 + s6
    add     r9, r8, r9
    add     lr, r0, r1
    stmia   lr, {r11, ip}
    add     lr, r0, r1, lsl #2
    stmia   lr, {r4, r9}

    add     ip, sp, #40
    ldm     ip, {r6, r8}            @ s11
    add     r11, r6, r7             @ Fout[2m] = s11 + s12
    add     ip, r8, r10
    sub     r7, r6, r7              @ Fout[3m] = s11 - s12
    sub     r10, r8, r10
    add     lr, r0, r1, lsl #1
    stmia   lr, {r11, ip}
    add     lr, r0, r1
    add     lr, lr, r1, lsl #1
    stmia   lr, {r7, r10}

    add     r0, r0, #8
    subs    r5, r5, #1
    bne     .Lb5_inner

    ldr     r6, [sp, #56]           @ N
    subs    r6, r6, #1
    str     r6, [sp, #56]
    bne     .Lb5_outer
    add     sp, sp, #68
    pop     {r4-r11, pc}
.Lb5_yc:
    .word   18318                   @ sqrt(5)/4 in Q15
.Lb5_yai:
    .word   -31164                  @ -sin(2*pi/5)
.Lb5_ybi:
    .word   -19261                  @ -sin(4*pi/5)
    .size   kf_bfly5_armv4, .-kf_bfly5_armv4

    .section .note.GNU-stack,"",%progbits
