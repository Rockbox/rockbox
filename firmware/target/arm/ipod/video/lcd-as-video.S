/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Andree Buschmann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

    .section .icode, "ax", %progbits

    .align  2
    .global lcd_write_data
    .type   lcd_write_data, %function
                                      /* r0 = addr */
lcd_write_data:                       /* r1 = pixel count */
    stmfd   sp!, {r4-r6}
    mov     r2, #0x30000000           /* LCD data port */

    tst     r0, #2                    /* first pixel unaligned? */
    ldrneh  r3, [r0], #2
    strneh  r3, [r2]
    subne   r1, r1, #1
    
    subs    r1, r1, #16
.loop16:
    ldmgeia r0!, {r3-r6}
    stmgeia r2, {r3-r6}
    ldmgeia r0!, {r3-r6}
    stmgeia r2, {r3-r6}
    subges  r1, r1, #16
    bge     .loop16

    /* no need to correct the count, we're just checking bits from now */
    tst     r1, #8
    ldmneia r0!, {r3-r6}
    stmneia r2, {r3-r6}
    tst     r1, #4
    ldmneia r0!, {r3-r4}
    stmneia r2, {r3-r4}
    tst     r1, #2
    ldrne   r3, [r0], #4
    strne   r3, [r2]
    tst     r1, #1
    ldrneh  r3, [r0]
    strneh  r3, [r2]

    ldmfd   sp!, {r4-r6}
    bx      lr

/****************************************************************************
 * void lcd_write_yuv_420_lines(unsigned char const * const src[3],
 *                              int width,
 *                              int stride);
 *
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
 */
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, %function
lcd_write_yuv420_lines:
                                      /* r0 = yuv_src */
                                      /* r1 = width */
                                      /* r2 = stride */
    stmfd       sp!, { r4-r12 }       /* save non-scratch */
    ldmia       r0, { r10, r11, r12 } /* r10 = yuv_src[0] = Y'_p */
                                      /* r11 = yuv_src[1] = Cb_p */
                                      /* r12 = yuv_src[2] = Cr_p */
    add         r2, r10, r2           /* r2 = &ysrc[stride] */
    add         r3, r1, r1, asl #1    /* number of bytes for chroma buffer */
    add         r3, r3, #15           /*   plus room for 3 additional words, */
    bic         r3, r3, #3            /*   rounded up to multiples of 4 byte */
    sub         sp, sp, r3            /*   and allocate on stack */
    stmia       sp, {r1, r2, r3}      /* width, &ysrc[stride], stack_alloc */

    mov         r7, r1                /* r7 = loop count */
    add         r8, sp, #12           /* chroma buffer */
    mov         r9, #0x30000000       /* LCD data port */

    /* 1st loop start */
10:                                   /* loop start */

    ldrb        r0, [r11], #1         /* r0 = *usrc++ = *Cb_p++ */
    ldrb        r1, [r12], #1         /* r1 = *vsrc++ = *Cr_p++ */

    sub         r0, r0, #128          /* r0 = Cb-128 */
    sub         r1, r1, #128          /* r1 = Cr-128 */

    add         r3, r1, r1, asl #1    /* r3 = Cr*51 + Cb*24 */
    add         r3, r3, r3, asl #4   
    add         r3, r3, r0, asl #3   
    add         r3, r3, r0, asl #4   

    add         r4, r1, r1, asl #2    /* r1 = Cr*101 */
    add         r4, r4, r1, asl #5
    add         r1, r4, r1, asl #6

    add         r1, r1, #256          /* r1 = rv = (r1 + 256) >> 9 */
    mov         r1, r1, asr #9
    strb        r1, [r8], #1          /* store r1 to chroma_buf */
    rsb         r3, r3, #128          /* r3 = guv = (-r3 + 128) >> 8 */
    mov         r3, r3, asr #8       
    strb        r3, [r8], #1          /* store r3 to chroma_buf */
    add         r0, r0, #2            /* r0 = bu = (Cb*128 + 256) >> 9 */
    mov         r0, r0, asr #2       
    strb        r0, [r8], #1          /* store r0 to chroma_buf */
    
    /* 1st loop, first pixel */
    ldrb        r5, [r10], #1         /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r2, r5, r5, asl #2
    add         r5, r2, r5, asl #5
    
    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r2, r3, r5, asr #7    /* r2 = g = (Y >> 8) + guv */
    add         r4, r0, r5, asr #8    /* r4 = b = (Y >> 9) + bu */
    
    orr         r5, r6, r4            /* check if clamping is needed... */
    orr         r5, r5, r2, asr #1    /* ...at all */
    cmp         r5, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r2, #63               /* clamp g */
    mvnhi       r2, r2, asr #31
    andhi       r2, r2, #63
    cmp         r4, #31               /* clamp b */
    mvnhi       r4, r4, asr #31         
    andhi       r4, r4, #31          
15:                                   /* no clamp */
   
    orr         r4, r4, r2, lsl #5    /* pixel = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11  
    strh        r4, [r9]              /* write pixel */

    /* 1st loop, second pixel */
    ldrb        r5, [r10], #1         /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r2, r5, r5, asl #2
    add         r5, r2, r5, asl #5
    
    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r2, r3, r5, asr #7    /* r2 = g = (Y >> 8) + guv */
    add         r4, r0, r5, asr #8    /* r4 = b = (Y >> 9) + bu */
    
    orr         r5, r6, r4            /* check if clamping is needed... */
    orr         r5, r5, r2, asr #1    /* ...at all */
    cmp         r5, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r2, #63               /* clamp g */
    mvnhi       r2, r2, asr #31
    andhi       r2, r2, #63
    cmp         r4, #31               /* clamp b */
    mvnhi       r4, r4, asr #31         
    andhi       r4, r4, #31          
15:                                   /* no clamp */
   
    orr         r4, r4, r2, lsl #5    /* pixel = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11  
    strh        r4, [r9]              /* write pixel */
    
    subs        r7, r7, #2            /* check for loop end */
    bgt         10b                   /* back to beginning  */
    /* 1st loop end */
    
    add         r8, sp, #12           /* chroma buffer */
    ldmia       sp, { r7, r10 }       /* r7 = loop count */
                                      /* r10 = &ysrc[stride] */

    /* 2nd loop start */
20:                                   /* loop start */

    /* restore r1, r3 and r0 from chroma buffer */
    ldrsb       r1, [r8], #1
    ldrsb       r3, [r8], #1     
    ldrsb       r0, [r8], #1
    
    /* 2nd loop, first pixel */
    ldrb        r5, [r10], #1     /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r2, r5, r5, asl #2
    add         r5, r2, r5, asl #5
    
    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r2, r3, r5, asr #7    /* r2 = g = (Y >> 8) + guv */
    add         r4, r0, r5, asr #8    /* r4 = b = (Y >> 9) + bu */
    
    orr         r5, r6, r4            /* check if clamping is needed... */
    orr         r5, r5, r2, asr #1    /* ...at all */
    cmp         r5, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r2, #63               /* clamp g */
    mvnhi       r2, r2, asr #31
    andhi       r2, r2, #63
    cmp         r4, #31               /* clamp b */
    mvnhi       r4, r4, asr #31         
    andhi       r4, r4, #31          
15:                                   /* no clamp */
   
    orr         r4, r4, r2, lsl #5    /* pixel = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11  
    strh        r4, [r9]              /* write pixel */
    
    /* 2nd loop, second pixel */
    ldrb        r5, [r10], #1     /* r5 = *ysrc++ = *Y'_p++ */
    sub         r5, r5, #16           /* r5 = (Y'-16) * 74 */
    add         r2, r5, r5, asl #2
    add         r5, r2, r5, asl #5
    
    add         r6, r1, r5, asr #8    /* r6 = r = (Y >> 9) + rv */
    add         r2, r3, r5, asr #7    /* r2 = g = (Y >> 8) + guv */
    add         r4, r0, r5, asr #8    /* r4 = b = (Y >> 9) + bu */
    
    orr         r5, r6, r4            /* check if clamping is needed... */
    orr         r5, r5, r2, asr #1    /* ...at all */
    cmp         r5, #31                 
    bls         15f                   /* -> no clamp */
    cmp         r6, #31               /* clamp r */
    mvnhi       r6, r6, asr #31         
    andhi       r6, r6, #31             
    cmp         r2, #63               /* clamp g */
    mvnhi       r2, r2, asr #31
    andhi       r2, r2, #63
    cmp         r4, #31               /* clamp b */
    mvnhi       r4, r4, asr #31         
    andhi       r4, r4, #31          
15:                                   /* no clamp */
   
    orr         r4, r4, r2, lsl #5    /* pixel = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11  
    strh        r4, [r9]              /* write pixel */
    
    subs        r7, r7, #2            /* check for loop end */
    bgt         20b                   /* back to beginning  */
    /* 2nd loop end */

    ldr         r3, [sp, #8]
    add         sp, sp, r3            /* deallocate buffer */
    ldmfd       sp!, { r4-r12 }       /* restore registers */
    bx          lr

    .ltorg
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines
