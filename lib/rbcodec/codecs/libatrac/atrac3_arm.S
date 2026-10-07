/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: 
 *
 * Copyright (C) 2009 by Andree Buschmann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

    .syntax unified
    .section .text, "ax", %progbits

/****************************************************************************
 * void atrac3_iqmf_matrixing(int32_t *dest, 
 *                            int32_t *inlo, 
 *                            int32_t *inhi, 
 *                            unsigned int count);
 * 
 * Matrixing step within iqmf of atrac3 synthesis. Reference implementation:
 *
 * for(i=0; i<counter; i+=2){
 *      dest[2*i+0] = inlo[i  ] + inhi[i  ];
 *      dest[2*i+1] = inlo[i  ] - inhi[i  ];
 *      dest[2*i+2] = inlo[i+1] + inhi[i+1];
 *      dest[2*i+3] = inlo[i+1] - inhi[i+1];
 * }
 * Note: r12 is a scratch register and can be used without restorage.
 ****************************************************************************/
    .align  2
    .global atrac3_iqmf_matrixing
    .type   atrac3_iqmf_matrixing, %function

atrac3_iqmf_matrixing:
    /* r0 = dest */
    /* r1 = inlo */
    /* r2 = inhi */
    /* r3 = counter */
    stmfd   sp!, {r4-r9, lr}       /* save non-scratch registers */
    
.iqmf_matrixing_loop:
    ldmia r1!, { r4, r6, r8, r12}   /* load inlo[0...3] */
    ldmia r2!, { r5, r7, r9, lr }   /* load inhi[0...3] */
    add   r4, r4, r5                /* r4  = inlo[0] + inhi[0] */
    sub   r5, r4, r5, asl #1        /* r5  = inlo[0] - inhi[0] */
    add   r6, r6, r7                /* r6  = inlo[1] + inhi[1] */
    sub   r7, r6, r7, asl #1        /* r7  = inlo[1] - inhi[1] */
    add   r8, r8, r9                /* r8  = inlo[2] + inhi[2] */
    sub   r9, r8, r9, asl #1        /* r9  = inlo[2] - inhi[2] */
    add   r12, r12, lr              /* r12 = inlo[3] + inhi[3] */
    sub   lr , r12, lr, asl #1      /* lr  = inlo[3] - inhi[3] */
    stmia r0!, {r4-r9, r12, lr}     /* store results to dest */
    subs r3, r3, #4                 /* counter -= 4 */
    bgt .iqmf_matrixing_loop
    
    ldmpc   regs=r4-r9              /* restore registers */

.atrac3_iqmf_matrixing_end:
    .size   atrac3_iqmf_matrixing,.atrac3_iqmf_matrixing_end-atrac3_iqmf_matrixing


/* ATRAC3_OUT_SHIFT of atrac3.h */
#define OUT_SHIFT 11

/****************************************************************************
 * void atrac3_iqmf_matrixing_out(int32_t *dest,
 *                                int32_t *inlo,
 *                                int32_t *inhi,
 *                                unsigned int count);
 *
 * The matrixing step of the last iqmf stage: as atrac3_iqmf_matrixing, with
 * the results scaled up by OUT_SHIFT bits, which then is the scale of the
 * decoder's output. Not used on ARMv4, where the multiplications of the
 * dewindowing would take longer for it: see atrac3_iqmf_dewindowing_out.
 ****************************************************************************/
#if ARM_ARCH >= 5
    .align  2
    .global atrac3_iqmf_matrixing_out
    .type   atrac3_iqmf_matrixing_out, %function

atrac3_iqmf_matrixing_out:
    /* r0 = dest */
    /* r1 = inlo */
    /* r2 = inhi */
    /* r3 = counter */
    stmfd   sp!, {r4-r9, lr}       /* save non-scratch registers */

.iqmf_matrixing_out_loop:
    ldmia r1!, { r4, r6, r8, r12}   /* load inlo[0...3] */
    ldmia r2!, { r5, r7, r9, lr }   /* load inhi[0...3] */
    add   r4, r4, r5                /* r4  = inlo[0] + inhi[0] */
    mov   r4, r4, asl #OUT_SHIFT
    sub   r5, r4, r5, asl #OUT_SHIFT+1 /* r5  = inlo[0] - inhi[0] */
    add   r6, r6, r7                /* r6  = inlo[1] + inhi[1] */
    mov   r6, r6, asl #OUT_SHIFT
    sub   r7, r6, r7, asl #OUT_SHIFT+1 /* r7  = inlo[1] - inhi[1] */
    add   r8, r8, r9                /* r8  = inlo[2] + inhi[2] */
    mov   r8, r8, asl #OUT_SHIFT
    sub   r9, r8, r9, asl #OUT_SHIFT+1 /* r9  = inlo[2] - inhi[2] */
    add   r12, r12, lr              /* r12 = inlo[3] + inhi[3] */
    mov   r12, r12, asl #OUT_SHIFT
    sub   lr , r12, lr, asl #OUT_SHIFT+1 /* lr  = inlo[3] - inhi[3] */
    stmia r0!, {r4-r9, r12, lr}     /* store results to dest */
    subs r3, r3, #4                 /* counter -= 4 */
    bgt .iqmf_matrixing_out_loop

    ldmpc   regs=r4-r9              /* restore registers */

.atrac3_iqmf_matrixing_out_end:
    .size   atrac3_iqmf_matrixing_out,.atrac3_iqmf_matrixing_out_end-atrac3_iqmf_matrixing_out
#endif /* ARM_ARCH >= 5 */

    
/****************************************************************************
 * atrac3_iqmf_dewindowing(int32_t *out,
 *                         int32_t *in,
 *                         int32_t *win,
 *                         unsigned int nIn);
 * 
 * Dewindowing step within iqmf of atrac3 synthesis. Reference implementation:
 *
 * for (j = nIn; j != 0; j--) {
 *          s1 = fixmul32(in[0], win[0]);
 *          s2 = fixmul32(in[1], win[1]);
 *          for (i = 2; i < 48; i += 2) {
 *              s1 += fixmul32(in[i  ], win[i  ]);
 *              s2 += fixmul32(in[i+1], win[i+1]);
 *          }
 *          out[0] = s2 << 1;
 *          out[1] = s1 << 1;
 *          in += 2;
 *          out += 2;
 *      }
 *
 * The window is symmetric, win[i] = win[47-i], so only its first half is
 * read: win[2k] and win[2k+1] serve in[2k] and in[2k+1] from the front of
 * the 48 inputs and, swapped, in[46-2k] and in[47-2k] from the back. That
 * is 24 coefficient loads for an output pair where there were 48, and
 * they can be loaded four at a time. The sums are kept in 64 bits, so the
 * order of the additions does not change the result.
 *
 * Note: r12 is a scratch register and can be used without restorage.
 ****************************************************************************/

/* Eight taps: coefficients win[2k..2k+3], two input pairs from the front
 * (r1) and two from the back (r3). s1 is lr/r9 and s2 is r12/r8. The
 * samples are the last operand of the multiplies, as they are the smaller
 * one and that is what the multiplier's early termination goes by. */
#define DEWIN_8_SAMPLES_ASM(first1, first2) \
    ldmia r2!, {r4-r7};             /* load win[2k..2k+3] */ \
    ldmia r1!, {r10, r11};          /* load in[2k], in[2k+1] */ \
    first1 lr , r9, r4, r10;        /* s1 += win[2k  ] * in[2k  ] */ \
    first2 r12, r8, r5, r11;        /* s2 += win[2k+1] * in[2k+1] */ \
    ldmdb r3!, {r10, r11};          /* load in[46-2k], in[47-2k] */ \
    smlal lr , r9, r5, r10;         /* s1 += win[2k+1] * in[46-2k] */ \
    smlal r12, r8, r4, r11;         /* s2 += win[2k  ] * in[47-2k] */ \
    ldmia r1!, {r10, r11};          /* load in[2k+2], in[2k+3] */ \
    smlal lr , r9, r6, r10;         /* s1 += win[2k+2] * in[2k+2] */ \
    smlal r12, r8, r7, r11;         /* s2 += win[2k+3] * in[2k+3] */ \
    ldmdb r3!, {r10, r11};          /* load in[44-2k], in[45-2k] */ \
    smlal lr , r9, r7, r10;         /* s1 += win[2k+3] * in[44-2k] */ \
    smlal r12, r8, r6, r11;         /* s2 += win[2k+2] * in[45-2k] */

/* atrac3_iqmf_dewindowing is the function above. atrac3_iqmf_dewindowing_out
 * is the same for the last iqmf stage, with the result scaled up by OUT_SHIFT
 * bits, which then is the scale of the decoder's output: the bits are taken
 * from the 64-bit sums, that have them. */
.macro DEWINDOWING name, shift
    .align  2
    .global \name
    .type   \name, %function

\name:
    /* r0 = dest */
    /* r1 = input samples */
    /* r2 = window coefficients */
    /* r3 = counter */
    stmfd   sp!, {r4-r11, lr}       /* save non-scratch registers */
    add     r3, r0, r3, lsl #3      /* end of dest: two words a count */
    stmfd   sp!, {r3}
    add     r3, r1, #192            /* r3 = end of the 48 inputs */
    
1:                                  /* outer loop 0...counter-1 */
    
    DEWIN_8_SAMPLES_ASM(smull, smull)   /* in[ 0.. 3], in[44..47] */
    DEWIN_8_SAMPLES_ASM(smlal, smlal)   /* in[ 4.. 7], in[40..43] */
    DEWIN_8_SAMPLES_ASM(smlal, smlal)   /* in[ 8..11], in[36..39] */
    DEWIN_8_SAMPLES_ASM(smlal, smlal)   /* in[12..15], in[32..35] */
    DEWIN_8_SAMPLES_ASM(smlal, smlal)   /* in[16..19], in[28..31] */
    DEWIN_8_SAMPLES_ASM(smlal, smlal)   /* in[20..23], in[24..27] */

    mov   lr , lr , lsr #31-\shift
    orr   r9, lr , r9, lsl #1+\shift /* s1 = low>>31 || hi<<1, << shift */
    mov   r12, r12, lsr #31-\shift
    orr   r8, r12, r8, lsl #1+\shift /* s2 = low>>31 || hi<<1, << shift */

    stmia r0!, {r8, r9}             /* store result out[0]=s2, out[1]=s1 */
    ldr   r4, [sp]                  /* end of dest */
    sub   r1, r1, #88               /* back 24 entries, on 2: in += 2 */
    add   r3, r3, #104              /* on 24 entries, and 2 more */
    sub   r2, r2, #96               /* roll back 24 entries = win[0] */

    cmp   r0, r4
    bne   1b
    
    add     sp, sp, #4
    ldmpc   regs=r4-r11             /* restore registers */
    
    .size   \name, .-\name
.endm

    DEWINDOWING atrac3_iqmf_dewindowing, 0
    DEWINDOWING atrac3_iqmf_dewindowing_out, OUT_SHIFT
