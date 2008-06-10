/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Andree Buschmann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "mpc_config.h"

    .section .text, "ax", %progbits
    
/****************************************************************************
 * void mpc_decoder_windowing_D(...)
 * 
 * 2nd step within synthesis filter. Does the dewindowing.
 * 32=32x32 multiplies (OPTIMIZE_FOR_SPEED)
 * Uses pre-shifted V[] and D[] values.
 ****************************************************************************/
#if defined(OPTIMIZE_FOR_SPEED)
    .align  2
    .global mpc_decoder_windowing_D
    .type   mpc_decoder_windowing_D, %function
mpc_decoder_windowing_D:
    /* r0 = Data[] */
    /* r1 = V[] */
    /* r2 = D[] */
    /* lr = counter */
    
    stmfd   sp!, {r4-r12, lr}
    
    mov lr, #32
.loop32:
    ldmia r2!, { r3-r10 }   /* load first 8 window coefficients */
    ldr r11, [r1]           /*  0 */
    mul r12, r3, r11         
    ldr r11, [r1, #96*4]    /*  1 */
    mla r12, r4, r11, r12     
    ldr r11, [r1, #128*4]   /*  2 */
    mla r12, r5, r11, r12     
    ldr r11, [r1, #224*4]   /*  3 */
    mla r12, r6, r11, r12     
    ldr r11, [r1, #256*4]   /*  4 */
    mla r12, r7, r11, r12     
    ldr r11, [r1, #352*4]   /*  5 */
    mla r12, r8, r11, r12     
    ldr r11, [r1, #384*4]   /*  6 */
    mla r12, r9, r11, r12     
    ldr r11, [r1, #480*4]   /*  7 */
    mla r12, r10, r11, r12     
    ldmia r2!, { r3-r10 }   /* load last 8 window coefficients */
    ldr r11, [r1, #512*4]   /*  8 */
    mla r12, r3, r11, r12     
    ldr r11, [r1, #608*4]   /*  9 */
    mla r12, r4, r11, r12     
    ldr r11, [r1, #640*4]   /* 10 */
    mla r12, r5, r11, r12     
    ldr r11, [r1, #736*4]   /* 11 */
    mla r12, r6, r11, r12     
    ldr r11, [r1, #768*4]   /* 12 */
    mla r12, r7, r11, r12     
    ldr r11, [r1, #864*4]   /* 13 */
    mla r12, r8, r11, r12     
    ldr r11, [r1, #896*4]   /* 14 */
    mla r12, r9, r11, r12     
    ldr r11, [r1, #992*4]   /* 15 */
    mla r12, r10, r11, r12
    mov r12, r12, asr #1    /* post shift to compensate for pre-shifting */
    str r12, [r0], #4       /* store Data */
    add r1, r1, #4          /* V++ */
    
    subs lr, lr, #1
    bgt .loop32
    
    ldmfd   sp!, {r4-r12, pc}
.mpc_dewindowing_end:
    .size   mpc_decoder_windowing_D,.mpc_dewindowing_end-mpc_decoder_windowing_D
#else
/****************************************************************************
 * void mpc_decoder_windowing_D(...)
 * 
 * 2nd step within synthesis filter. Does the dewindowing.
 * 64=32x32 multiplies
 * Drops lo-part of 64bit multiply results and will therefor loose 1 bit
 * accuracy. The decoder output is binary identical as this imprecision is
 * far below the output's 16bit resolution.
 ****************************************************************************/
    .align  2
    .global mpc_decoder_windowing_D
    .type   mpc_decoder_windowing_D, %function
mpc_decoder_windowing_D:
    /* r0 = Data[] */
    /* r1 = V[] */
    /* r2 = D[] */
    /* lr = counter */
    
    stmfd   sp!, {r4-r12, lr}
    
    mov lr, #32
.loop32:
    ldmia r2!, { r3-r10 }   /* load first 8 window coefficients */
    ldr r11, [r1]           /*  0 */
    smull r11, r12, r3, r11  
    ldr r11, [r1, #96*4]    /*  1 */
    smlal r11, r12, r4, r11  
    ldr r11, [r1, #128*4]   /*  2 */
    smlal r11, r12, r5, r11  
    ldr r11, [r1, #224*4]   /*  3 */
    smlal r11, r12, r6, r11   
    ldr r11, [r1, #256*4]   /*  4 */
    smlal r11, r12, r7, r11  
    ldr r11, [r1, #352*4]   /*  5 */
    smlal r11, r12, r8, r11  
    ldr r11, [r1, #384*4]   /*  6 */
    smlal r11, r12, r9, r11  
    ldr r11, [r1, #480*4]   /*  7 */
    smlal r11, r12, r10, r11
    ldmia r2!, { r3-r10 }   /* load last 8 window coefficients */
    ldr r11, [r1, #512*4]   /*  8 */
    smlal r11, r12, r3, r11
    ldr r11, [r1, #608*4]   /*  9 */
    smlal r11, r12, r4, r11  
    ldr r11, [r1, #640*4]   /* 10 */
    smlal r11, r12, r5, r11  
    ldr r11, [r1, #736*4]   /* 11 */
    smlal r11, r12, r6, r11  
    ldr r11, [r1, #768*4]   /* 12 */
    smlal r11, r12, r7, r11  
    ldr r11, [r1, #864*4]   /* 13 */
    smlal r11, r12, r8, r11  
    ldr r11, [r1, #896*4]   /* 14 */
    smlal r11, r12, r9, r11  
    ldr r11, [r1, #992*4]   /* 15 */
    smlal r11, r12, r10, r11
    mov r4, r12, lsl #2     /* get result from hi-part, loose 2 bits */
    str r4, [r0], #4        /* store Data */
    add r1, r1, #4          /* V++ */
    
    subs lr, lr, #1
    bgt .loop32
    
    ldmfd   sp!, {r4-r12, pc}
.mpc_dewindowing_end:
    .size   mpc_decoder_windowing_D,.mpc_dewindowing_end-mpc_decoder_windowing_D
#endif
