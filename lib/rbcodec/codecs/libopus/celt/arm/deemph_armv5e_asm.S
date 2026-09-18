/* ARMv5E stereo de-emphasis for the CELT decoder output.
 *
 * Copyright (c) 2007-2008 CSIRO
 * Copyright (c) 2007-2010 Xiph.Org Foundation
 * Copyright (c) 2008 Gregory Maxwell
 * Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * celt/celt_decoder.c are met.
 *
 * Why this exists
 * ---------------
 * deemphasis_stereo_simple is the decoder's last stage: a one-pole filter
 * per channel and the conversion to 16-bit PCM.  The compiled loop feeds
 * each channel's ldr straight into an add, a stall apiece, shifts both
 * filter states left once per sample, and clamps to 16 bits with four
 * instructions and a register holding -32768.
 *
 * Here the state is kept as the smulwb result and shifted inside the add
 * that consumes it, "add r8, r8, r4, lsl #1", which removes two shifts per
 * sample.  The incoming state from mem[] is a full value, so the first
 * sample adds it unshifted and joins the loop after the adds; nothing is
 * assumed about its low bit.  The clamp is the three-instruction form
 *
 *     mov   lr, y, asr #15
 *     teq   lr, y, asr #31       @ in range iff bits 31..15 agree
 *     eorne y, r7, y, asr #31    @ else 0x7FFF, or 0xFFFF8000 for negatives
 *
 * whose low halfword is exactly the clamped value strh stores.
 *
 * Bit-exact: smulwb followed by a shift is MULT16_32_Q15 on this
 * architecture, the rounding shift is PSHR32(x, SIG_SHIFT), and additions
 * wrap exactly as the C does.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "deemph_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2

    .macro  PCM16 src, tmp
    add     \tmp, \src, #0x800      @ PSHR32(x, 12)
    mov     \tmp, \tmp, asr #12
    mov     lr, \tmp, asr #15
    teq     lr, \tmp, asr #31
    eorne   \tmp, r7, \tmp, asr #31
    strh    \tmp, [r2], #2
    .endm

/* ------------------------------------------------------------------------
 * void deemph_stereo_armv5e(celt_sig *x0, celt_sig *x1, opus_val16 *pcm,
 *                           int N, int coef0, celt_sig *mem)
 *
 * r0 x0, r1 x1, r2 pcm, r3 and r6 the count, r4 and r5 the two filter
 * states, r7 32767, r8 and r9 the two channel sums, r10 coef0, r11 and lr
 * scratch, ip mem.
 * ------------------------------------------------------------------------ */
    .global deemph_stereo_armv5e
    .type   deemph_stereo_armv5e, %function
deemph_stereo_armv5e:
    push    {r4-r11, lr}
    ldr     r10, [sp, #36]          @ coef0; smulwb reads its low halfword
    ldr     ip, [sp, #40]           @ mem
    mov     r6, r3
    ldmia   ip, {r4, r5}            @ m0, m1, full values
    mov     r7, #0x7F00
    orr     r7, r7, #0xFF
    cmp     r6, #0
    ble     .Lde5_store
    ldr     r8, [r0], #4
    ldr     r9, [r1], #4
    add     r8, r8, r4              @ first sample: the state as stored
    add     r9, r9, r5
    b       .Lde5_body
.Lde5_loop:
    ldr     r8, [r0], #4
    ldr     r9, [r1], #4
    add     r8, r8, r4, lsl #1      @ tmp0 = x0[j] + m0
    add     r9, r9, r5, lsl #1      @ tmp1 = x1[j] + m1
.Lde5_body:
    smulwb  r4, r8, r10             @ m0 = this << 1
    smulwb  r5, r9, r10             @ m1 = this << 1
    PCM16   r8, r11                 @ pcm[2j]
    PCM16   r9, r11                 @ pcm[2j+1]
    subs    r6, r6, #1
    bne     .Lde5_loop
    mov     r4, r4, lsl #1
    mov     r5, r5, lsl #1
.Lde5_store:
    stmia   ip, {r4, r5}            @ mem[0], mem[1]
    pop     {r4-r11, pc}
    .size   deemph_stereo_armv5e, .-deemph_stereo_armv5e

    .section .note.GNU-stack,"",%progbits
