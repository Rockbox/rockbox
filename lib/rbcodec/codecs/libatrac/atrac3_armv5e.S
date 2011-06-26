/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: 
 *
 * Copyright (C) 2010 by Michael Giacomelli
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
 * atrac3_iqmf_dewindowing_armv5e(int32_t *out,
 *                         int32_t *in,
 *                         int32_t *win,
 *                         unsigned int nIn);
 * 
 * Dewindowing step within iqmf of atrac3 synthesis using 16 bit filter 
 * coefficients and armv5e packed multiply instructions.  Uses 2.5 cycles
 * per filter coefficient (ideal).  Benchmarked 3.54 per coefficient (Clip+).
 * 
 * Reference implementation:
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
    .align  2
    .global atrac3_iqmf_dewindowing_armv5e
    .type   atrac3_iqmf_dewindowing_armv5e, %function
    
atrac3_iqmf_dewindowing_armv5e:
    /* r0 = dest */
    /* r1 = input samples */
    /* r2 = window coefficients */
    /* r3 = counter */
    stmfd   sp!, {r4-r11, lr}       /* save non-scratch registers */
    
.iqmf_dewindow_outer_loop:          /* outer loop 0...counter-1 */
    /*  0.. 7 */
    ldmia r2!, {r4, r5, r8, r9}     /* load win[0..7] */
    ldmia r1!, {r6, r7, r10, r11}   /* load in[0..3] to avoid stall on arm11 */
    smulwb lr, r6, r4               /* s1 = in[0] * win[0] */   
    smulwt r12, r7, r4              /* s2 = in[1] * win[1] */               
    smlawb lr, r10, r5, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r5, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
                    
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */
    smlawb lr, r6, r8, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r8, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
    smlawb lr, r10, r9, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11, r9, r12        /* s2 += in[i+1] * win[i+1] >> 16 */
    
    /*  8..15 */
    ldmia r2!, {r4, r5, r8, r9}     /* load win[8..15] */
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */ 
    smlawb lr, r6, r4, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r4, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r5, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r5, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
                    
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */
    smlawb lr, r6, r8, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r8, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r9, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r9, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
    
    /* 16..23 */
    ldmia r2!, {r4, r5, r8, r9}     /* load win[16..23] */
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */ 
    smlawb lr, r6, r4, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r4, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r5, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r5, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
                    
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */
    smlawb lr, r6, r8, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r8, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r9, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r9, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
    
    /* 24..31 */
    ldmia r2!, {r4, r5, r8, r9}     /* load win[24..31] */
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */ 
    smlawb lr, r6, r4, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r4, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r5, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r5, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
                    
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */
    smlawb lr, r6, r8, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r8, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r9, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r9, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
    
    /* 32..39 */
    ldmia r2!, {r4, r5, r8, r9}     /* load win[32..39] */
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */ 
    smlawb lr, r6, r4, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r4, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r5, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r5, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
                    
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */
    smlawb lr, r6, r8, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r8, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r9, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r9, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
    
    /* 40..47 */
    ldmia r2!, {r4, r5, r8, r9}     /* load win[40..47] */
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */ 
    smlawb lr, r6, r4, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r4, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r5, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r5, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
                    
    ldmia r1!, {r6, r7, r10, r11}   /* load in[i...i+3] */
    smlawb lr, r6, r8, lr           /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r7, r8, r12         /* s2 += in[i+1] * win[i+1] >> 16 */        
    smlawb lr, r10, r9, lr          /* s1 += in[i  ] * win[i  ] >> 16 */
    smlawt r12, r11,r9, r12         /* s2 += in[i+1] * win[i+1] >> 16 */
    
        
    mov   lr , lr , lsl #1                  
    mov   r12, r12, lsl #1                   

    stmia r0!, {r12, lr}            /* store result out[0]=s2, out[1]=s1 */
    sub   r1, r1, #184              /* roll back 64 entries = 184 bytes */
    sub   r2, r2, #96               /* roll back 48 entries * 2 bytes = 96 bytes = win[0] */

    subs r3, r3, #1                 /* outer loop -= 1 */
    bgt .iqmf_dewindow_outer_loop
    
    ldmpc   regs=r4-r11             /* restore registers */
    
.atrac3_iqmf_dewindowing_armv5e_end:
    .size   atrac3_iqmf_dewindowing_armv5e,.atrac3_iqmf_dewindowing_armv5e_end-atrac3_iqmf_dewindowing_armv5e
