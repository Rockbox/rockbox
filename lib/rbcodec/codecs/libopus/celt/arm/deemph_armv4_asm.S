/* ARMv4 stereo de-emphasis for the CELT decoder output.
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
 * per channel and the conversion to 16-bit PCM.  On the ARM7TDMI the two
 * smull dominate and cannot be avoided, but the compiled loop also shifts
 * both filter states left once per sample and clamps to 16 bits with four
 * instructions and a register holding -32768.
 *
 * Here the state is kept as the high word of the smull and shifted inside
 * the add that consumes it, "add r8, r8, r4, lsl #1", and the clamp is the
 * three-instruction asr / teq / eorne form.  The incoming state from mem[]
 * is a full value, so the first sample adds it unshifted.  See
 * deemph_armv5e_asm.S for the clamp.
 *
 * Bit-exact: the high word of smull(tmp, coef0<<16) shifted left one is
 * exactly MULT16_32_Q15_armv4, low bit dropped included.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "deemph_armv4_asm.S must be assembled in ARM mode"
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

    .macro  PCM16 src, tmp
    add     \tmp, \src, #0x800      @ PSHR32(x, 12)
    mov     \tmp, \tmp, asr #12
    mov     lr, \tmp, asr #15
    teq     lr, \tmp, asr #31
    eorne   \tmp, r7, \tmp, asr #31
    strh    \tmp, [r2], #2
    .endm

/* ------------------------------------------------------------------------
 * void deemph_stereo_armv4(celt_sig *x0, celt_sig *x1, opus_val16 *pcm,
 *                          int N, int coef0, celt_sig *mem)
 *
 * r0 x0, r1 x1, r2 pcm, r6 the count, r4 and r5 the two filter states,
 * r7 32767, r8 and r9 the two channel sums, r10 coef0<<16, r11 and lr
 * scratch, ip mem.  smull needs RdLo, RdHi and Rm distinct: lr, r4/r5, r8/r9.
 * ------------------------------------------------------------------------ */
    .global deemph_stereo_armv4
    .type   deemph_stereo_armv4, %function
deemph_stereo_armv4:
    push    {r4-r11, lr}
    ldr     r10, [sp, #36]          @ coef0
    ldr     ip, [sp, #40]           @ mem
    mov     r10, r10, lsl #16       @ into the top half of Rs, as the C does
    mov     r6, r3
    ldmia   ip, {r4, r5}            @ m0, m1, full values
    mov     r7, #0x7F00
    orr     r7, r7, #0xFF
    cmp     r6, #0
    ble     .Lde4_store
    ldr     r8, [r0], #4
    ldr     r9, [r1], #4
    add     r8, r8, r4              @ first sample: the state as stored
    add     r9, r9, r5
    b       .Lde4_body
.Lde4_loop:
    ldr     r8, [r0], #4
    ldr     r9, [r1], #4
    add     r8, r8, r4, lsl #1      @ tmp0 = x0[j] + m0
    add     r9, r9, r5, lsl #1      @ tmp1 = x1[j] + m1
.Lde4_body:
    smull   lr, r4, r8, r10         @ m0 = r4 << 1
    smull   lr, r5, r9, r10         @ m1 = r5 << 1
    PCM16   r8, r11                 @ pcm[2j]
    PCM16   r9, r11                 @ pcm[2j+1]
    subs    r6, r6, #1
    bne     .Lde4_loop
    mov     r4, r4, lsl #1
    mov     r5, r5, lsl #1
.Lde4_store:
    stmia   ip, {r4, r5}            @ mem[0], mem[1]
    pop     {r4-r11, pc}
    .size   deemph_stereo_armv4, .-deemph_stereo_armv4

    .section .note.GNU-stack,"",%progbits
