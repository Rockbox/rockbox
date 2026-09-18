/* ARMv5E inner loops for the CELT backward MDCT.
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
 * On ARMv5E the compiled rotations already get the 32x16 multiply, because
 * MULT16_32_Q15 resolves to smulwb in arm/fixed_armv5e.h.  What they never
 * get is the rest of the instruction set: mdct.o from a Sansa Clip+ build
 * contains 34 smulwb and not one smulwt, smlaw, ldrd or multi-register
 * transfer.  Three things follow.
 *
 * The twiddles are loaded one halfword at a time.  A kiss_twiddle_scalar is
 * 16 bits, so t[i] and t[i+1] are one aligned word; unrolling by two lets a
 * single ldr fetch both and smulwb/smulwt pick between them.  That halves
 * the twiddle loads, and an ldr also resolves a cycle sooner than an ldrsh.
 *
 * Each product pair is narrowed twice.  S_MUL(a,t0)+S_MUL(b,t1) compiles to
 * two smulwb, two shifts and an add, but smulwb followed by smlawb leaves
 * the sum at Q16 and needs one shift.  This is bit-exact with the C: both
 * products are still truncated separately inside their own instruction, and
 * the shift distributes over the sum.
 *
 * The subtraction is deliberately not fused.  smlaw only adds, so fusing it
 * would mean negating a twiddle, and truncation is not symmetric about
 * zero: trunc(a)+trunc(-b) carries a whole unit of bias where
 * trunc(a)-trunc(b) carries none.  It would also cost an rsb per twiddle,
 * which the packed form cannot supply.  Two smulw and a shifted subtract
 * stay.
 *
 * Accuracy is therefore identical to the C path on this architecture, not
 * better as on ARMv4.  The exact smull/smlal narrowing the ARMv4 kernels
 * use is worth about 9.5 dB here too, but it costs a register pair and
 * three issue cycles against one, which is a different trade on this core.
 *
 * Word loads need a word-aligned table and an even N4.  The static CELT
 * modes give both, but neither is guaranteed by the API, so each kernel
 * tests its pointers and falls through to a scalar loop that doubles as the
 * odd-count tail.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "mdct_armv5e_asm.S must be assembled in ARM mode"
#endif

/* config.h decides whether the prime factor transform is built, and the
   post-rotation kernel below only exists when it is.  libopus/config.h
   guards its own includes with __ASSEMBLER__, so this costs nothing here. */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

    .text
    .align  2

/* The scalar paths below are the odd-count tail and the fallback for a table
   the unroll cannot address as words.  Neither runs on the static CELT modes,
   so build with -DMDCT_ARMV5E_FORCE_SCALAR to route everything through them
   and confirm they still agree with the C. */
    .macro  FORCE_SCALAR
#ifdef MDCT_ARMV5E_FORCE_SCALAR
    orr     ip, ip, #1
#endif
    .endm


/* ------------------------------------------------------------------------
 * void mdct_prerot_armv5e(const kiss_fft_scalar *xp1,
 *                         const kiss_fft_scalar *xp2,
 *                         const kiss_twiddle_scalar *t,
 *                         const opus_int16 *bitrev,
 *                         kiss_fft_scalar *yp, int N4, int step)
 *
 * yr = xp2*t[i] + xp1*t[N4+i];  yi = xp1*t[i] - xp2*t[N4+i]
 * yp[2*rev+1] = yr;  yp[2*rev] = yi;   xp1 += step;  xp2 -= step
 *
 * step is in bytes.  The output pair is 8 contiguous bytes even though the
 * bitrev table scatters where it lands, so it stores as one stm.
 *
 * r10 holds t[i] packed with t[i+1] and r11 holds t[N4+i] with t[N4+i+1];
 * \sel picks the half.  The scalar path loads with ldrsh and passes b,
 * which reads the same bits.
 * ------------------------------------------------------------------------ */
    .macro  PREROT_ONE sel
    ldr     r5, [r1], -r6           @ x2 = *xp2, xp2 -= step
    ldr     r9, [r0], r6            @ x1 = *xp1, xp1 += step, and x2's
                                    @ interlock is covered by it
    smulw\sel  lr, r5, r10          @ x2 * t[i]
    smlaw\sel  lr, r9, r11, lr      @ + x1 * t[N4+i]        -> yr at Q16
    smulw\sel  ip, r9, r10          @ x1 * t[i]
    smulw\sel  r9, r5, r11          @ x2 * t[N4+i]
    ldrsh   r5, [r3], #2            @ rev = *bitrev++
    sub     ip, ip, r9
    mov     lr, lr, lsl #1          @ yr
    mov     ip, ip, lsl #1          @ yi
    add     r5, r4, r5, lsl #3
    stm     r5, {ip, lr}            @ yp[2*rev] = yi, yp[2*rev+1] = yr
    .endm

    .global mdct_prerot_armv5e
    .type   mdct_prerot_armv5e, %function
mdct_prerot_armv5e:
    push    {r4-r11, lr}
    sub     sp, sp, #4
    ldr     r4, [sp, #40]           @ yp
    ldr     r5, [sp, #44]           @ N4
    ldr     r6, [sp, #48]           @ step, in bytes
    mov     r7, r5, lsl #1          @ byte offset from t[i] to t[N4+i]
    and     r9, r5, #1              @ odd sample left over by the unroll
    mov     r8, r5, lsr #1          @ pairs
    orr     ip, r2, r7              @ word-addressable table and stride?
    FORCE_SCALAR
    tst     ip, #3
    movne   r8, #0                  @ no: run the whole thing scalar
    movne   r9, r5
    str     r9, [sp]                @ scalar count, reloaded once
    cmp     r8, #0
    beq     .Lpre5_scalar
.Lpre5_pairs:
    ldr     r11, [r2, r7]           @ t[N4+i] : t[N4+i+1]
    ldr     r10, [r2], #4           @ t[i]    : t[i+1]
    PREROT_ONE b
    PREROT_ONE t
    subs    r8, r8, #1
    bne     .Lpre5_pairs
.Lpre5_scalar:
    ldr     r8, [sp]
    cmp     r8, #0
    beq     .Lpre5_done
.Lpre5_sloop:
    ldrsh   r11, [r2, r7]           @ t[N4+i]
    ldrsh   r10, [r2], #2           @ t[i]
    PREROT_ONE b
    subs    r8, r8, #1
    bne     .Lpre5_sloop
.Lpre5_done:
    add     sp, sp, #4
    pop     {r4-r11, pc}
    .size   mdct_prerot_armv5e, .-mdct_prerot_armv5e


/* ------------------------------------------------------------------------
 * void mdct_postrot_armv5e(kiss_fft_scalar *yp0, kiss_fft_scalar *yp1,
 *                          const kiss_twiddle_scalar *t, int N4, int count)
 *
 * Walks the buffer from both ends at once, so each end reads a contiguous
 * complex pair with one ldm.  Four twiddle streams run at once: t[i] and
 * t[N4+i] climb, t[N4-i-1] and t[N2-i-1] descend.  Both directions pack,
 * the descending ones in reverse, so the unrolled pair reads them t first
 * and b second.
 *
 * Every read happens before every write, which is what makes the doubled
 * middle pair produced by an odd N4 come out the same as the C.
 * ------------------------------------------------------------------------ */
    .macro  POSTROT_ONE sel, dsel
    ldm     r0, {r8, r9}            @ im = yp0[0], re = yp0[1]
    smulw\sel  lr, r9, r10          @ re * t0
    smlaw\sel  lr, r8, r11, lr      @ + im * t1             -> yr at Q16
    smulw\sel  ip, r9, r11          @ re * t1
    smulw\sel  r8, r8, r10          @ im * t0
    mov     lr, lr, lsl #1          @ yr
    sub     ip, ip, r8
    mov     ip, ip, lsl #1          @ yi
    ldm     r1, {r8, r9}            @ im = yp1[0], re = yp1[1]
    str     lr, [r0]                @ yp0[0] = yr
    smulw\dsel lr, r9, r5           @ re * t0b
    smlaw\dsel lr, r8, r6, lr       @ + im * t1b            -> yr2 at Q16
    smulw\dsel r9, r9, r6           @ re * t1b
    smulw\dsel r8, r8, r5           @ im * t0b
    mov     lr, lr, lsl #1          @ yr2
    sub     r9, r9, r8
    str     lr, [r1]                @ yp1[0] = yr2
    str     ip, [r1, #4]            @ yp1[1] = yi
    mov     r9, r9, lsl #1          @ yi2
    str     r9, [r0, #4]            @ yp0[1] = yi2
    add     r0, r0, #8
    sub     r1, r1, #8
    .endm

    .align  2
    .global mdct_postrot_armv5e
    .type   mdct_postrot_armv5e, %function
mdct_postrot_armv5e:
    push    {r4-r11, lr}
    sub     sp, sp, #4
    ldr     r4, [sp, #40]           @ count; r4 counts, r8 and r9 are scratch
    mov     r7, r3, lsl #1          @ byte offset from t[k] to t[N4+k]
    sub     r8, r3, #1
    add     r3, r2, r8, lsl #1      @ descending pointer, at t[N4-i-1]
    and     r9, r4, #1              @ odd iteration left over
    eor     ip, r3, #2              @ its word sits two bytes below
    orr     ip, ip, r2
    orr     ip, ip, r7
    FORCE_SCALAR
    tst     ip, #3
    movne   r9, r4                  @ not word-addressable: go scalar
    mov     r4, r4, lsr #1          @ pairs
    movne   r4, #0
    str     r9, [sp]
    cmp     r4, #0
    beq     .Lpost5_scalar
.Lpost5_pairs:
    ldr     r10, [r2]               @ t[i]      : t[i+1]
    ldr     r11, [r2, r7]           @ t[N4+i]   : t[N4+i+1]
    add     lr, r3, r7
    ldr     r5, [r3, #-2]           @ t[N4-i-2] : t[N4-i-1]
    ldr     r6, [lr, #-2]           @ t[N2-i-2] : t[N2-i-1]
    add     r2, r2, #4
    sub     r3, r3, #4
    POSTROT_ONE b, t
    POSTROT_ONE t, b
    subs    r4, r4, #1
    bne     .Lpost5_pairs
.Lpost5_scalar:
    ldr     r4, [sp]
    cmp     r4, #0
    beq     .Lpost5_done
.Lpost5_sloop:
    ldrsh   r11, [r2, r7]           @ t[N4+i]
    ldrsh   r10, [r2], #2           @ t[i]
    ldrsh   r6, [r3, r7]            @ t[N2-i-1]
    ldrsh   r5, [r3], #-2           @ t[N4-i-1]
    POSTROT_ONE b, b
    subs    r4, r4, #1
    bne     .Lpost5_sloop
.Lpost5_done:
    add     sp, sp, #4
    pop     {r4-r11, pc}
    .size   mdct_postrot_armv5e, .-mdct_postrot_armv5e


/* ------------------------------------------------------------------------
 * void mdct_mirror_armv5e(kiss_fft_scalar *xp1, kiss_fft_scalar *yp1,
 *                         const opus_val16 *wp1, const opus_val16 *wp2,
 *                         int count)
 *
 * The TDAC fold.  The two data pointers walk in opposite directions and
 * never meet, so nothing there bursts, but the two window pointers pack
 * exactly as the twiddles do.
 * ------------------------------------------------------------------------ */
    .macro  MIRROR_ONE sel, dsel
    ldr     r9, [r0]                @ x1 = *xp1
    ldr     ip, [r1]                @ x2 = *yp1
    smulw\dsel lr, ip, r6           @ x2 * w2
    smulw\sel  r8, r9, r5           @ x1 * w1
    sub     lr, lr, r8
    mov     lr, lr, lsl #1
    str     lr, [r1], #4            @ *yp1++ = w2*x2 - w1*x1
    smulw\sel  lr, ip, r5           @ x2 * w1
    smlaw\dsel lr, r9, r6, lr       @ + x1 * w2
    mov     lr, lr, lsl #1
    str     lr, [r0], #-4           @ *xp1-- = w1*x2 + w2*x1
    .endm

    .align  2
    .global mdct_mirror_armv5e
    .type   mdct_mirror_armv5e, %function
mdct_mirror_armv5e:
    push    {r4-r11, lr}
    sub     sp, sp, #4
    ldr     r4, [sp, #40]           @ count; r4 counts, r8 and r9 are scratch
    and     r9, r4, #1
    eor     ip, r3, #2
    orr     ip, ip, r2
    FORCE_SCALAR
    tst     ip, #3
    movne   r9, r4
    mov     r4, r4, lsr #1
    movne   r4, #0
    str     r9, [sp]
    cmp     r4, #0
    beq     .Lmir5_scalar
.Lmir5_pairs:
    ldr     r5, [r2], #4            @ w1[i]    : w1[i+1]
    ldr     r6, [r3, #-2]           @ w2[-i-1] : w2[-i]
    sub     r3, r3, #4
    MIRROR_ONE b, t
    MIRROR_ONE t, b
    subs    r4, r4, #1
    bne     .Lmir5_pairs
.Lmir5_scalar:
    ldr     r4, [sp]
    cmp     r4, #0
    beq     .Lmir5_done
.Lmir5_sloop:
    ldrsh   r5, [r2], #2            @ w1
    ldrsh   r6, [r3], #-2           @ w2
    MIRROR_ONE b, b
    subs    r4, r4, #1
    bne     .Lmir5_sloop
.Lmir5_done:
    add     sp, sp, #4
    pop     {r4-r11, pc}
    .size   mdct_mirror_armv5e, .-mdct_mirror_armv5e

#ifdef OPUS_PFA
    .text
    .align 2



/* One post-rotated output.
 *
 *   \sel  picks which half of the packed twiddle pair to use
 *   \pm   the pmap pointer, post-incremented by \pi
 *   \b0/\o0  where yr goes, \b1/\o1 where yi goes
 *
 * r10 holds t[k] packed with its neighbour and r11 holds t[N4+k] with its
 * neighbour, so two loads serve two outputs.  The gather is an ldrsh of the
 * byte offset and one ldm, which fetches both halves of the complex value in
 * a single transaction.
 */
/* The gather index arrives in r12, and this body loads the next one before
   it needs its own loaded pair -- an index is a halfword, which interlocks
   for two cycles if the address add follows it, and the ldm interlocks for
   one if the multiply does.  Rotating the load one body ahead covers both,
   and it is the only place to put it: every other register is live.

   That in turn is why yr stores before yi is computed.  Holding the index
   in r12 costs the body its second scratch, so the accumulator has to be
   free again before the imaginary half starts.

   \npm and \npi name the pointer the NEXT body reads, or none at the end of
   the chain, where the loop head reloads instead. */
    .macro  PR5_PFA sel, npm, npi, b0, o0, b1, o1
    add     r12, r0, r12
    ldm     r12, {r9, lr}           @ r9 = im = S[2p], lr = re = S[2p+1];
                                    @ ldm fills in register order, not the
                                    @ order written, so r9 takes the lower
                                    @ address whichever way the list reads
    .ifnc \npm, none
    ldrsh   r12, [\npm], #\npi      @ the next body's index
    .endif
    smulw\sel  r8, lr, r10          @ re * t0
    smlaw\sel  r8, r9, r11, r8      @ + im * t1            -> yr at Q16
    mov     r8, r8, lsl #1
    str     r8, [\b0, #\o0]
    smulw\sel  r8, lr, r11          @ re * t1
    smulw\sel  lr, r9, r10          @ im * t0
    sub     r8, r8, lr
    mov     r8, r8, lsl #1
    str     r8, [\b1, #\o1]
    .endm

/* The same body for the scalar fallback, which loads its own index: it runs
   one output per end, so there is no next body to rotate against. */
    .macro  PR5_PFA_S sel, pm, pi, b0, o0, b1, o1
    ldrsh   r12, [\pm], #\pi
    add     r12, r0, r12
    ldm     r12, {r9, lr}
    smulw\sel  r8, lr, r10
    smlaw\sel  r8, r9, r11, r8
    smulw\sel  r12, lr, r11
    smulw\sel  lr, r9, r10
    mov     r8, r8, lsl #1
    sub     r12, r12, lr
    str     r8, [\b0, #\o0]
    mov     r12, r12, lsl #1
    str     r12, [\b1, #\o1]
    .endm

/* ------------------------------------------------------------------------
 * void mdct_postrot_pfa_armv5e(const kiss_fft_scalar *S,
 *                              kiss_fft_scalar *yp0, kiss_fft_scalar *yp1,
 *                              const kiss_twiddle_scalar *t,
 *                              const opus_int16 *pmap, int N4)
 *
 * The post-rotation for the Good-Thomas transform.  Same arithmetic as
 * mdct_postrot_armv5e; what changes is that the spectrum arrives permuted,
 * X[k] at M*(k mod 15) + (k mod M), so each end is gathered through pmap,
 * which already holds that index as a byte offset.  The ascending half reads
 * the table forwards and the descending half backwards.
 *
 * Gathering makes yp0 and yp1 write-only, and that is what lets the twiddles
 * stay packed.  A kiss_twiddle_scalar is 16 bits, so t[k] and t[k+1] are one
 * aligned word; the mixed-radix kernel exploits that by holding all four
 * twiddle words -- both ends at once -- across a body that does two outputs
 * from each.  Here two of those registers are spent on the pmap pointers, so
 * instead the body does both ascending outputs, reloads the pair for the
 * descending end, and does both of those.  That reordering is legal only
 * because nothing is read back: in the in-place version the descending
 * output has to be read before the ascending one overwrites it, which forces
 * the two ends to interleave.
 *
 * No iteration counter either: the two pmap pointers walk towards each other
 * and the loop ends when they cross.  Four outputs an iteration and N4 a
 * multiple of four -- 60, 120, 240, 480 -- means that lands exactly.
 *
 * The scalar path below is the fallback for a trig table the packed loads
 * cannot address as words, or an N4 the unroll does not divide.  No CELT
 * mode reaches it; build with -DMDCT_ARMV5E_FORCE_SCALAR to route everything
 * through it and confirm it still agrees.
 * ------------------------------------------------------------------------ */
    .align  2
    .global mdct_postrot_pfa_armv5e
    .type   mdct_postrot_pfa_armv5e, %function
mdct_postrot_pfa_armv5e:
    push    {r4-r11, lr}
    ldr     r5, [sp, #36]           @ pmap
    ldr     r12, [sp, #40]          @ N4
    mov     r7, r12, lsl #1         @ byte offset from t[k] to t[N4+k]
    sub     r8, r12, #1
    add     r4, r3, r8, lsl #1      @ tb = &t[N4-1], walks down
    add     r6, r5, r8, lsl #1      @ pd = &pmap[N4-1], walks down

    /* The packed path needs the trig table word-addressable and N4 a
       multiple of four; the second gives the first its stride and lets the
       crossing test land exactly. */
    orr     r8, r3, r12
#ifdef MDCT_ARMV5E_FORCE_SCALAR
    orr     r8, r8, #1
#endif
    tst     r8, #3
    bne     .Lpost5_pfa_scalar

    cmp     r5, r6
    bhi     .Lpost5_pfa_done
.Lpost5_pfa_pairs:
    ldrsh   r12, [r5], #2           @ index of X[i]; the twiddle loads below
                                    @ stand between it and the address add
    ldr     r10, [r3]               @ t[i]    : t[i+1]
    ldr     r11, [r3, r7]           @ t[N4+i] : t[N4+i+1]
    add     r3, r3, #4
    PR5_PFA b, r5, 2, r1, 0, r2, 4      @ X[i]
    PR5_PFA t, r6, -2, r1, 8, r2, -4    @ X[i+1]

    add     lr, r4, r7
    ldr     r10, [r4, #-2]          @ t[N4-i-2] : t[N4-i-1]
    ldr     r11, [lr, #-2]          @ t[N2-i-2] : t[N2-i-1]
    sub     r4, r4, #4
    PR5_PFA t, r6, -2, r2, 0, r1, 4     @ X[N4-1-i]
    PR5_PFA b, none, 0, r2, -8, r1, 12  @ X[N4-2-i]

    add     r1, r1, #16
    sub     r2, r2, #16
    cmp     r5, r6
    blo     .Lpost5_pfa_pairs
    b       .Lpost5_pfa_done

/* One output per end per iteration, twiddles a halfword at a time.  The
   crossing test is bls rather than blo so an odd N4 still computes its
   middle pair, which is what the C loop to (N4+1)>>1 does. */
.Lpost5_pfa_scalar:
    cmp     r5, r6
    bhi     .Lpost5_pfa_done
.Lpost5_pfa_sloop:
    ldrsh   r11, [r3, r7]           @ t[N4+i]
    ldrsh   r10, [r3], #2           @ t[i]
    PR5_PFA_S b, r5, 2, r1, 0, r2, 4
    ldrsh   r11, [r4, r7]           @ t[N2-i-1]
    ldrsh   r10, [r4], #-2          @ t[N4-i-1]
    PR5_PFA_S b, r6, -2, r2, 0, r1, 4
    add     r1, r1, #8
    sub     r2, r2, #8
    cmp     r5, r6
    bls     .Lpost5_pfa_sloop
.Lpost5_pfa_done:
    pop     {r4-r11, pc}
    .size   mdct_postrot_pfa_armv5e, .-mdct_postrot_pfa_armv5e
#endif

    .section .note.GNU-stack,"",%progbits
