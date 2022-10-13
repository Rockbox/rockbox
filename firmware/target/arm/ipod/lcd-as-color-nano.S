/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010-2011 by Andree Buschmann
 *
 * Generic asm helper function used by YUV blitting.
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
#include "cpu.h"

/**************************************************************************** 
 * #define FORCE_FIFO_WAIT
 *
 * This is not needed in YUV blitting when the LCD IF is fast enough. In this
 * case YUV-to-RGB conversion per pixel needs longer than the transfer of a 
 * pixel via the LCD IF.
 ****************************************************************************/

#include "config.h"

/* Set FIFO wait for both iPod Color and iPod nano1G until we know for which
 * devices we can switch this off. */
#define FORCE_FIFO_WAIT

    .section .icode, "ax", %progbits
    
/****************************************************************************
 * extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
 *                                    const unsigned LCD_BASE,
 *                                    int width,
 *                                    int stride);
 *
 *   Conversion from Motion JPEG and MPEG Y'PbPr to RGB is:
 *   |R|   |1.164  0.000  1.596| |Y' -  16|
 *   |G| = |1.164 -0.391 -0.813| |Pb - 128|
 *   |B|   |1.164  2.018  0.000| |Pr - 128|
 *
 *   Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
 *
 * Converts two lines from YUV to RGB565 and writes to LCD at once. First loop
 * loads Cb/Cr, calculates the chroma offset and saves them to buffer. Within
 * the second loop these chroma offset are reloaded from buffer. Within each 
 * loop two pixels are calculated and written to LCD. 
 */
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, %function
lcd_write_yuv420_lines:
                                      /* r0 = src = yuv_src */
                                      /* r1 = dst = LCD_BASE */
                                      /* r2 = width */
                                      /* r3 = stride */                
    stmfd       sp!, { r4-r10, lr }   /* save non-scratch */
    ldmia       r0, { r9, r10, r12 }  /* r9 = yuv_src[0] = Y'_p */
                                      /* r10 = yuv_src[1] = Cb_p */
                                      /* r12 = yuv_src[2] = Cr_p */
    add         r3, r9, r3            /* r3 = &ysrc[stride] */
    add         r4, r2, r2, asr #1    /* chroma buffer lenght = width/2 *3 */
    mov         r4, r4, asl #2        /*   use words for str/ldm possibility */
    add         r4, r4, #19           /*   plus room for 4 additional words, */
    bic         r4, r4, #3            /*   rounded up to multiples of 4 byte */
    sub         sp, sp, r4            /*   and allocate on stack */
    stmia       sp, {r1-r4}           /* LCD_BASE, width, &ysrc[stride], stack_alloc */

    mov         r7, r2                /* r7 = loop count */
    add         r8, sp, #16           /* chroma buffer */
    add         lr, r1, #0x100        /* LCD data port = LCD2_BASE + 0x100 */

    /* 1st loop start */
10:                                   /* loop start */

    ldrb        r0, [r10], #1         /* r0 = *usrc++ = *Cb_p++ */
    ldrb        r1, [r12], #1         /* r1 = *vsrc++ = *Cr_p++ */

    sub         r0, r0, #128          /* r0 = Cb-128 */
    sub         r1, r1, #128          /* r1 = Cr-128 */

    add         r2, r1, r1, asl #1    /* r2 = Cr*51 + Cb*24 */
    add         r2, r2, r2, asl #4   
    add         r2, r2, r0, asl #3   
    add         r2, r2, r0, asl #4   

    add         r4, r1, r1, asl #2    /* r1 = Cr*101 */
    add         r4, r4, r1, asl #5
    add         r1, r4, r1, asl #6

    add         r1, r1, #256          /* r1 = rv = (r1 + 256) >> 9 */
    mov         r1, r1, asr #9
    rsb         r2, r2, #128          /* r2 = guv = (-r2 + 128) >> 8 */
    mov         r2, r2, asr #8       
    add         r0, r0, #2            /* r0 = bu = (Cb*128 + 256) >> 9 */
    mov         r0, r0, asr #2       
    stmia       r8!, {r0-r2}          /* store r0, r1 and r2 to chroma buffer */

    /* 1st loop, first pixel */
    ldrb        r5, [r9], #1          /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r3, r5, r5, asl #2
    add         r5, r3, r5, asl #5

    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r3, r2, r5, asr #7    /* r3 = g = (Y >> 8) + guv */
    add         r4, r0, r5, asr #8    /* r4 = b = (Y >> 9) + bu */

    orr         r5, r6, r4            /* check if clamping is needed... */
    orr         r5, r5, r3, asr #1    /* ...at all */
    cmp         r5, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r3, #63               /* clamp g */
    mvnhi       r3, r3, asr #31
    andhi       r3, r3, #63
    cmp         r4, #31               /* clamp b */
    mvnhi       r4, r4, asr #31         
    andhi       r4, r4, #31          
15:                                   /* no clamp */

    /* calculate pixel_1 and save to r4 for later pixel packing */
    orr         r4, r4, r3, lsl #5    /* pixel_1 = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11   /* r4 = pixel_1 */

    /* 1st loop, second pixel */
    ldrb        r5, [r9], #1          /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r3, r5, r5, asl #2
    add         r5, r3, r5, asl #5

    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r3, r2, r5, asr #7    /* r3 = g = (Y >> 8) + guv */
    add         r5, r0, r5, asr #8    /* r5 = b = (Y >> 9) + bu */   

    orr         r0, r6, r5            /* check if clamping is needed... */
    orr         r0, r0, r3, asr #1    /* ...at all */
    cmp         r0, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r3, #63               /* clamp g */
    mvnhi       r3, r3, asr #31
    andhi       r3, r3, #63
    cmp         r5, #31               /* clamp b */
    mvnhi       r5, r5, asr #31         
    andhi       r5, r5, #31          
15:                                   /* no clamp */

    /* calculate pixel_2 and pack with pixel_1 before writing */
    orr         r5, r5, r3, lsl #5    /* pixel_2 = r<<11 | g<<5 | b */
    orr         r5, r5, r6, lsl #11   /* r5 = pixel_2 */
#ifdef FORCE_FIFO_WAIT
    /* wait for FIFO half full */
.fifo_wait1:
    ldr         r3, [lr, #-0xE0]      /* while !(LCD2_BLOCK_CTRL & 0x1000000); */
    tst         r3, #0x1000000
    beq         .fifo_wait1
#endif

    mov         r3, r4, lsl #8        /* swap pixel_1 */
    and         r3, r3, #0xff00
    add         r4, r3, r4, lsr #8
    
    orr         r4, r4, r5, lsl #24   /* swap pixel_2 and pack with pixel_1 */
    mov         r5, r5, lsr #8
    orr         r4, r4, r5, lsl #16

    str         r4, [lr]              /* write pixel_1 and pixel_2 */

    subs        r7, r7, #2            /* check for loop end */
    bgt         10b                   /* back to beginning  */
    /* 1st loop end */

    /* Reload several registers for pointer rewinding for next loop */
    add         r8, sp, #16           /* chroma buffer */
    ldmia       sp, { r1, r7, r9}     /* r1  = LCD_BASE */
                                      /* r7  = loop count */
                                      /* r9 = &ysrc[stride] */   

    /* 2nd loop start */
20:                                   /* loop start */
    /* restore r0 (bu), r1 (rv) and r2 (guv) from chroma buffer */
    ldmia       r8!, {r0-r2}

    /* 2nd loop, first pixel */
    ldrb        r5, [r9], #1          /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r3, r5, r5, asl #2
    add         r5, r3, r5, asl #5

    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r3, r2, r5, asr #7    /* r3 = g = (Y >> 8) + guv */
    add         r4, r0, r5, asr #8    /* r4 = b = (Y >> 9) + bu */

    orr         r5, r6, r4            /* check if clamping is needed... */
    orr         r5, r5, r3, asr #1    /* ...at all */
    cmp         r5, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r3, #63               /* clamp g */
    mvnhi       r3, r3, asr #31
    andhi       r3, r3, #63
    cmp         r4, #31               /* clamp b */
    mvnhi       r4, r4, asr #31         
    andhi       r4, r4, #31          
15:                                   /* no clamp */
    /* calculate pixel_1 and save to r4 for later pixel packing */
    orr         r4, r4, r3, lsl #5    /* pixel_1 = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11   /* r4 = pixel_1 */

    /* 2nd loop, second pixel */
    ldrb        r5, [r9], #1          /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r3, r5, r5, asl #2
    add         r5, r3, r5, asl #5

    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r3, r2, r5, asr #7    /* r3 = g = (Y >> 8) + guv */
    add         r5, r0, r5, asr #8    /* r5 = b = (Y >> 9) + bu */

    orr         r0, r6, r5            /* check if clamping is needed... */
    orr         r0, r0, r3, asr #1    /* ...at all */
    cmp         r0, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r3, #63               /* clamp g */
    mvnhi       r3, r3, asr #31
    andhi       r3, r3, #63
    cmp         r5, #31               /* clamp b */
    mvnhi       r5, r5, asr #31         
    andhi       r5, r5, #31          
15:                                   /* no clamp */

    /* calculate pixel_2 and pack with pixel_1 before writing */
    orr         r5, r5, r3, lsl #5    /* pixel_2 = r<<11 | g<<5 | b */
    orr         r5, r5, r6, lsl #11   /* r5 = pixel_2 */
#ifdef FORCE_FIFO_WAIT
    /* wait for FIFO half full */
.fifo_wait2:
    ldr         r3, [lr, #-0xE0]      /* while !(LCD2_BLOCK_CTRL & 0x1000000); */
    tst         r3, #0x1000000
    beq         .fifo_wait2
#endif

    mov         r3, r4, lsl #8        /* swap pixel_1 */
    and         r3, r3, #0xff00
    add         r4, r3, r4, lsr #8
    
    orr         r4, r4, r5, lsl #24   /* swap pixel_2 and pack with pixel_1 */
    mov         r5, r5, lsr #8
    orr         r4, r4, r5, lsl #16
    
    str         r4, [lr]              /* write pixel_1 and pixel_2 */

    subs        r7, r7, #2            /* check for loop end */
    bgt         20b                   /* back to beginning  */
    /* 2nd loop end */

    ldr         r3, [sp, #12]
    add         sp, sp, r3            /* deallocate buffer */
    ldmpc       regs=r4-r10           /* restore registers */

    .ltorg
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines
