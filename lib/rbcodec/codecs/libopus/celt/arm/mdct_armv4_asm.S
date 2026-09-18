/* ARMv4 inner loops for the CELT backward MDCT.
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2008 Xiph.Org Foundation
 * Copyright (c) 2008 Gregory Maxwell
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/mdct.c are met.
 *
 * Why this exists
 * ---------------
 * The three loops here are the whole cost of clt_mdct_backward, and what the
 * compiled versions lose is registers.  Each loop needs more live values
 * than gcc can hold, so it spills the loop-invariant pointers, strides and
 * limits and reloads them every pass: five stack accesses per iteration in
 * the post-rotation alone.  Here the bookkeeping stays resident, and the
 * only reload is the pre-rotation's input stride, which buys the seventh
 * working register the complex multiply needs.
 *
 * Holding the twiddle as a 16-bit value and accumulating the product pair
 * with smull/smlal is what makes that fit: seven live registers, where the
 * shifted MULT16_32_Q15 form needs nine.  The ARM7TDMI datasheet also
 * promises a 16-bit Rs will finish in two multiplier passes rather than
 * four, but device measurement shows no sign of that paying here.  Treat
 * the form as register economy, not as a multiplier win.
 *
 * ldm/stm helps only where the addressing allows it.  The post-rotation
 * reads and writes contiguous pairs, so it bursts.  The pre-rotation reads
 * the spectrum through a runtime stride and writes through the bit-reversal
 * table, so only its 8-byte output pair merges, and the TDAC mirror walks
 * two pointers in opposite directions and merges nothing.
 *
 * All three keep all 32 bits of each Q15 product, where MULT16_32_Q15_armv4
 * drops the low bit for speed.  The backward MDCT applies three such rounds
 * per sample, so this is worth about 9.5 dB rather than being incidental.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "mdct_armv4_asm.S must be assembled in ARM mode"
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

/* One Q15 complex multiply, the same register discipline as the C_MUL asm in
   arm/kiss_fft_armv4.h.  a is (\ar, \ai), b is (\br, \bi), \tt is scratch and
   the 64-bit low word.  Results: \mr = a.r*b.r - a.i*b.i, \mi = a.r*b.i +
   a.i*b.r.  \bi and \br are destroyed.  \mr must differ from \ar and \br,
   \mi from \ai and \tt.  \tt and \mr may be the same register: \tt is dead
   after the fourth instruction and \mr is not written until the sixth, and
   sharing them is what lets these loops keep their invariants resident. */
    .macro  CMULQ15 ar, ai, br, bi, tt, mr, mi
    smull   \tt, \mi, \ai, \br
    smlal   \tt, \mi, \ar, \bi
    mov     \tt, \tt, lsr #15
    orr     \mi, \tt, \mi, lsl #17
    rsb     \bi, \bi, #0
    smull   \br, \mr, \ar, \br
    smlal   \br, \mr, \ai, \bi
    mov     \br, \br, lsr #15
    orr     \mr, \br, \mr, lsl #17
    .endm


/* ------------------------------------------------------------------------
 * void mdct_prerot_armv4(const kiss_fft_scalar *xp1,
 *                        const kiss_fft_scalar *xp2,
 *                        const kiss_twiddle_scalar *t,
 *                        const opus_int16 *bitrev,
 *                        kiss_fft_scalar *yp, int N4, int step)
 *
 * yr = xp2*t[i] + xp1*t[N4+i];  yi = xp1*t[i] - xp2*t[N4+i]
 * yp[2*rev+1] = yr;  yp[2*rev] = yi;   xp1 += step;  xp2 -= step
 *
 * step is in bytes.  Both halves of the output are one 8-byte pair, so they
 * store together even though the address is scattered by the bitrev table.
 * ------------------------------------------------------------------------ */
    .global mdct_prerot_armv4
    .type   mdct_prerot_armv4, %function
mdct_prerot_armv4:
    push    {r4-r11, lr}
    ldr     r5, [sp, #36]           @ yp
    ldr     r6, [sp, #40]           @ N4, also the loop count
    ldr     lr, [sp, #44]           @ step, in bytes
    mov     r4, r6, lsl #1
    sub     r4, r4, #2              @ t[i] is read post-incremented, so the
                                    @ offset on to t[N4+i] is two less
    cmp     r6, #0
    ble     .Lpre_done
.Lpre_loop:
    ldr     r8, [r0], lr            @ x1 = *xp1++
    ldr     r9, [r1], -lr           @ x2 = *xp2--
    ldrsh   r10, [r2], #2           @ t[i]
    ldrsh   r11, [r2, r4]           @ t[N4+i], and cover t[i]'s I cycle
    CMULQ15 r8, r9, r10, r11, r7, r7, ip
    ldrsh   r10, [r3], #2           @ rev
    subs    r6, r6, #1              @ between the load and the address it
                                    @ forms, where the counter is free
    add     r10, r5, r10, lsl #3
    stm     r10, {r7, ip}           @ yi at 2*rev, yr at 2*rev+1
    bne     .Lpre_loop
.Lpre_done:
    pop     {r4-r11, pc}
    .size   mdct_prerot_armv4, .-mdct_prerot_armv4


/* ------------------------------------------------------------------------
 * void mdct_postrot_armv4(kiss_fft_scalar *yp0, kiss_fft_scalar *yp1,
 *                         const kiss_twiddle_scalar *t, int N4, int count)
 *
 * Walks the buffer from both ends at once, so each end contributes a
 * contiguous complex pair: two ldm in, and the writes pair up as well, apart
 * from yp0[0], which is stored early to free a register before the second
 * complex multiply.
 *
 * The read of yp1 must happen before yp1[1] is written, which is why the
 * store order here looks less symmetric than the arithmetic.
 * ------------------------------------------------------------------------ */
    .align  2

/* Unreachable once the prime factor transform is built, so it gives its IRAM
   back rather than sitting resident and unread.  Still assembled: a size the
   Good-Thomas path does not cover falls through to it, and correct-but-slow
   from DRAM is the right failure mode. */
    .text
    .align  2

    .global mdct_postrot_armv4
    .type   mdct_postrot_armv4, %function
mdct_postrot_armv4:
    push    {r4-r11, lr}
    ldr     r5, [sp, #36]           @ count
    mov     r4, r3, lsl #1          @ byte offset from t[k] to t[N4+k]
    sub     r6, r3, #1
    add     r3, r2, r6, lsl #1      @ tb = &t[N4-1], walks down
    cmp     r5, #0
    ble     .Lpost_done
.Lpost_loop:
    ldrsh   r7, [r2, r4]            @ t[N4+i]
    ldrsh   r6, [r2], #2            @ t[i]
    ldm     r0, {r8, r9}            @ im = yp0[0], re = yp0[1]
    /* yr = re*t0 + im*t1, yi = re*t1 - im*t0 */
    CMULQ15 r9, r8, r7, r6, lr, r11, ip
    ldm     r1, {r8, r9}            @ im = yp1[0], re = yp1[1]
    str     ip, [r0]                @ yp0[0] = yr
    ldrsh   r7, [r3, r4]            @ t[N2-i-1]
    ldrsh   r6, [r3], #-2           @ t[N4-i-1]
    /* mr is yi' and mi is yr', so the pair lands with yr' in the lower
       register and stores straight into yp1. */
    CMULQ15 r9, r8, r7, r6, lr, ip, r10
    stm     r1, {r10, r11}          @ yp1[0] = yr', yp1[1] = yi
    str     ip, [r0, #4]            @ yp0[1] = yi'
    add     r0, r0, #8
    sub     r1, r1, #8
    subs    r5, r5, #1
    bne     .Lpost_loop
.Lpost_done:
    pop     {r4-r11, pc}
    .size   mdct_postrot_armv4, .-mdct_postrot_armv4


/* ------------------------------------------------------------------------
 * void mdct_mirror_armv4(kiss_fft_scalar *xp1, kiss_fft_scalar *yp1,
 *                        const opus_val16 *wp1, const opus_val16 *wp2,
 *                        int count)
 *
 * The TDAC fold.  Two data pointers walk in opposite directions and two
 * window pointers do the same, so nothing here is contiguous and nothing
 * bursts; the win is entirely the multiplier operand form.
 * ------------------------------------------------------------------------ */
    .align  2

    .text
    .align  2

    .global mdct_mirror_armv4
    .type   mdct_mirror_armv4, %function
mdct_mirror_armv4:
    push    {r4-r11, lr}
    ldr     r4, [sp, #36]           @ count
    cmp     r4, #0
    ble     .Lmir_done
.Lmir_loop:
    ldrsh   r5, [r3], #-2           @ w2 = *wp2--
    ldrsh   r6, [r2], #2            @ w1 = *wp1++
    ldr     r7, [r0]                @ x1 = *xp1
    ldr     r8, [r1]                @ x2 = *yp1
    /* lo = w2*x2 - w1*x1, hi = w1*x2 + w2*x1 */
    CMULQ15 r8, r7, r5, r6, lr, r9, ip
    str     r9, [r1], #4            @ *yp1++ = lo
    str     ip, [r0], #-4           @ *xp1-- = hi
    subs    r4, r4, #1
    bne     .Lmir_loop
.Lmir_done:
    pop     {r4-r11, pc}
    .size   mdct_mirror_armv4, .-mdct_mirror_armv4


#ifdef OPUS_PFA
    .text

/* ------------------------------------------------------------------------
 * void mdct_postrot_pfa_armv4(const kiss_fft_scalar *S,
 *                             kiss_fft_scalar *yp0, kiss_fft_scalar *yp1,
 *                             const kiss_twiddle_scalar *t,
 *                             const opus_int16 *pmap, int N4)
 *
 * The post-rotation for the Good-Thomas transform.  Same arithmetic as
 * mdct_postrot_armv4, but the spectrum arrives permuted -- X[k] sits at
 * M*(k mod 15) + (k mod M) -- so each end is gathered through pmap, which
 * holds that index already scaled to a byte offset.  One table serves both
 * ends: the ascending half reads it forwards, the descending half backwards.
 *
 * Gathering is why this reads a different buffer than it writes, and that
 * turns out to simplify the kernel rather than complicate it.  The
 * mixed-radix version has to store yp0[0] early and defer yp1[1] because it
 * must read yp1 before overwriting it; here yp0 and yp1 are write-only, so
 * both halves of each complex multiply store as soon as they exist and
 * nothing stays live across the second one.
 *
 * No iteration counter: the two pmap pointers walk towards each other and
 * the loop ends when they cross, which is the same (N4+1)>>1 iterations for
 * the even N4 every CELT size has.
 *
 * N4*2, the offset from t[i] to t[N4+i], does fit in a register after all:
 * the complex multiply's 64-bit scratch and its real result can share one,
 * so the gather address register is free by the time the multiply needs a
 * temporary.  It used to be reloaded from the frame once per end, which cost
 * six cycles an iteration for want of that observation.
 * ------------------------------------------------------------------------ */
    .align  2
    .global mdct_postrot_pfa_armv4
    .type   mdct_postrot_pfa_armv4, %function
mdct_postrot_pfa_armv4:
    push    {r4-r11, lr}
    ldr     r5, [sp, #36]           @ pmap
    ldr     r6, [sp, #40]           @ N4
    mov     r7, r6, lsl #1          @ byte offset from t[k] to t[N4+k]
    sub     r6, r6, #1
    add     r4, r3, r6, lsl #1      @ tb  = &t[N4-1], walks down
    add     r6, r5, r6, lsl #1      @ pd  = &pmap[N4-1], walks down
    cmp     r5, r6
    bhi     .Lpost_pfa_done
.Lpost_pfa_loop:
    /* ascending end: X[i].  The gather index leads, so the address add and
       the ldm each have an unrelated load in front of them, and the pair
       lands a cycle before the multiply that wants it. */
    ldrsh   r12, [r5], #2           @ byte offset of X[i]
    ldrsh   r11, [r3, r7]           @ t1 = t[N4+i]
    add     r12, r0, r12
    ldm     r12, {r8, r9}           @ im = S[2p], re = S[2p+1]
    ldrsh   r10, [r3], #2           @ t0 = t[i]
    CMULQ15 r9, r8, r11, r10, r12, r12, lr
    str     lr,  [r1]               @ yp0[0] = yr
    str     r12, [r2, #4]           @ yp1[1] = yi

    /* descending end: X[N4-1-i] */
    ldrsh   r12, [r6], #-2          @ byte offset of X[N4-1-i]
    ldrsh   r11, [r4, r7]           @ t1 = t[N2-i-1]
    add     r12, r0, r12
    ldm     r12, {r8, r9}
    ldrsh   r10, [r4], #-2          @ t0 = t[N4-i-1]
    CMULQ15 r9, r8, r11, r10, r12, r12, lr
    str     lr,  [r2]               @ yp1[0] = yr'
    str     r12, [r1, #4]           @ yp0[1] = yi'

    add     r1, r1, #8
    sub     r2, r2, #8
    cmp     r5, r6
    bls     .Lpost_pfa_loop         @ bls, not blo, so an odd N4 still gets
                                    @ its middle pair, as the C loop does
.Lpost_pfa_done:
    pop     {r4-r11, pc}
    .size   mdct_postrot_pfa_armv4, .-mdct_postrot_pfa_armv4
#endif /* OPUS_PFA */

    .section .note.GNU-stack,"",%progbits
