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

    .text
    .align  2

/* One Q15 complex multiply, the same register discipline as the C_MUL asm in
   arm/kiss_fft_armv4.h.  a is (\ar, \ai), b is (\br, \bi), \tt is scratch and
   the 64-bit low word.  Results: \mr = a.r*b.r - a.i*b.i, \mi = a.r*b.i +
   a.i*b.r.  \bi and \br are destroyed.  \mr must differ from \ar and \br,
   \mi from \ai and \tt. */
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
    sub     sp, sp, #12
    ldr     r5, [sp, #48]           @ yp
    ldr     r6, [sp, #52]           @ N4, also the loop count
    ldr     r4, [sp, #56]           @ step, in bytes
    str     r4, [sp]                @ the one loop-invariant reload
    mov     r4, r6, lsl #1          @ byte offset from t[i] to t[N4+i]
    cmp     r6, #0
    ble     .Lpre_done
.Lpre_loop:
    ldr     r7, [sp]
    ldr     r8, [r0], r7            @ x1 = *xp1++
    ldr     r9, [r1], -r7           @ x2 = *xp2--
    ldrsh   r11, [r2, r4]           @ t[N4+i]
    ldrsh   r10, [r2], #2           @ t[i]
    CMULQ15 r8, r9, r10, r11, lr, r7, ip
    ldrsh   r10, [r3], #2           @ rev
    add     r10, r5, r10, lsl #3
    stm     r10, {r7, ip}           @ yi at 2*rev, yr at 2*rev+1
    subs    r6, r6, #1
    bne     .Lpre_loop
.Lpre_done:
    add     sp, sp, #12
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

    .section .note.GNU-stack,"",%progbits
