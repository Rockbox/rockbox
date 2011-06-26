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
 * Note: r12 is a scratch register and can be used without restorage.
 ****************************************************************************/

/* To be called as first block to call smull for initial filling of the result
 * registers lr/r9 and r12/r8. */
#define DEWIN_8_SAMPLES_MUL_ASM \
    ldmia r2!, {r4, r5};            /* load win[0..1] */ \
    ldmia r1!, {r6, r7};            /* load in [0..1] */ \
    smull lr , r9, r4, r6;          /* s1 = win[0] * in[0] */ \
    smull r12, r8, r5, r7;          /* s2 = win[1] * in[1] */ \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */ \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */ \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */

/* Called after first block. Will always multiply-add to the result registers 
 * lr/r9 and r12/r8. */
#define DEWIN_8_SAMPLES_MLA_ASM \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */ \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */ \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */ \
    ldmia r2!, {r4, r5};            /* load win[i..i+1] */ \
    ldmia r1!, {r6, r7};            /* load in [i..i+1] */ \
    smlal lr , r9, r4, r6;          /* s1 = win[i  ] * in[i  ] */ \
    smlal r12, r8, r5, r7;          /* s2 = win[i+1] * in[i+1] */

    .align  2
    .global atrac3_iqmf_dewindowing
    .type   atrac3_iqmf_dewindowing, %function
    
atrac3_iqmf_dewindowing:
    /* r0 = dest */
    /* r1 = input samples */
    /* r2 = window coefficients */
    /* r3 = counter */
    stmfd   sp!, {r4-r9, lr}        /* save non-scratch registers */
    
.iqmf_dewindow_outer_loop:          /* outer loop 0...counter-1 */
    
    DEWIN_8_SAMPLES_MUL_ASM         /*  0.. 7, use "MUL" macro here! */
    DEWIN_8_SAMPLES_MLA_ASM         /*  8..15 */
    DEWIN_8_SAMPLES_MLA_ASM         /* 16..23 */
    DEWIN_8_SAMPLES_MLA_ASM         /* 24..31 */
    DEWIN_8_SAMPLES_MLA_ASM         /* 32..39 */
    DEWIN_8_SAMPLES_MLA_ASM         /* 40..47 */

    mov   lr , lr , lsr #31
    orr   r9, lr , r9, lsl #1       /* s1 = low>>31 || hi<<1 */
    mov   r12, r12, lsr #31
    orr   r8, r12, r8, lsl #1       /* s2 = low>>31 || hi<<1 */

    stmia r0!, {r8, r9}             /* store result out[0]=s2, out[1]=s1 */
    sub   r1, r1, #184              /* roll back 64 entries = 184 bytes */
    sub   r2, r2, #192              /* roll back 48 entries = 192 bytes = win[0] */

    subs r3, r3, #1                 /* outer loop -= 1 */
    bgt .iqmf_dewindow_outer_loop
    
    ldmpc   regs=r4-r9              /* restore registers */
    
.atrac3_iqmf_dewindowing_end:
    .size   atrac3_iqmf_dewindowing,.atrac3_iqmf_dewindowing_end-atrac3_iqmf_dewindowing
