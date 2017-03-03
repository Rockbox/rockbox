/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-as-video.S 26756 2010-06-11 04:41:36Z funman $
 *
 * Copyright (C) 2010 by Andree Buschmann
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

/* Version history:
 *
 *   SVN:
 *    - initial SVN version.
 *
 *   ARMv4:
 *    - use all available registers to calculate four pixels within each
 *      loop iteration.
 *    - avoid LDR interlocks.
 *
 *   ARMv5TE:
 *    - use ARMv5TE+ 1-cycle multiply-accumulate instructions.
 *
 *   ARMv5TE_WST:
 *    - use data tables (256 bytes) for RBG565 saturation.
 *
 * All versions are based on current SVN algorithm (round->scale->add)
 * using the same coefficients, so output results are identical.
 *
 * TODO?: SVN coefficients are a very nice approximation for operations
 * with shift+add instructions. When 16x16+32 MLA instructions are used,
 * NBR and COEF_N could probably be adjusted to slighly increase accuracy.
 */
#define VERSION_SVN             0
#define VERSION_ARMV4           1
#define VERSION_ARMV5TE         2
#define VERSION_ARMV5TE_WST     3

#define YUV2RGB_VERSION     VERSION_ARMV5TE_WST


#define ASM
#include "config.h"
#include "cpu.h"

#if (YUV2RGB_VERSION == VERSION_SVN)
    .section .icode, "ax", %progbits


/****************************************************************************
 * extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
 *                                    uint16_t* out,
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
                                      /* r1 = dst = out */
                                      /* r2 = width */
                                      /* r3 = stride */
    stmfd       sp!, { r4-r10, lr }   /* save non-scratch */
    ldmia       r0, { r9, r10, r12 }  /* r9 = yuv_src[0] = Y'_p */
                                      /* r10 = yuv_src[1] = Cb_p */
                                      /* r12 = yuv_src[2] = Cr_p */
    add         r3, r9, r3            /* r3 = &ysrc[stride] */
    add         r4, r2, r2, asr #1    /* chroma buffer lenght = width/2 *3 */
    mov         r4, r4, asl #2        /*   use words for str/ldm possibility */
    add         r4, r4, #15           /*   plus room for 3 additional words, */
    bic         r4, r4, #3            /*   rounded up to multiples of 4 byte */
    sub         sp, sp, r4            /*   and allocate on stack */
    stmia       sp, {r2-r4}           /* width, &ysrc[stride], stack_alloc */

    mov         r7, r2                /* r7 = loop count */
    add         r8, sp, #12           /* chroma buffer */
    mov         lr, r1                /* RGB565 data destination buffer */

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
    orr         r4, r4, r5, lsl #16
    str         r4, [lr], #4          /* write pixel_1 and pixel_2 */

    subs        r7, r7, #2            /* check for loop end */
    bgt         10b                   /* back to beginning  */
    /* 1st loop end */

    /* Reload several registers for pointer rewinding for next loop */
    add         r8, sp, #12           /* chroma buffer */
    ldmia       sp, {r7, r9}          /* r7  = loop count */
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
    orr         r4, r4, r5, lsl #16
    str         r4, [lr], #4          /* write pixel_1 and pixel_2 */

    subs        r7, r7, #2            /* check for loop end */
    bgt         20b                   /* back to beginning  */
    /* 2nd loop end */

    ldr         r3, [sp, #8]
    add         sp, sp, r3            /* deallocate buffer */
    ldmpc       regs=r4-r10           /* restore registers */

    .ltorg
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines


#elif (YUV2RGB_VERSION == VERSION_ARMV4)
/****************************************************************************
 * extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
 *                                    uint16_t* out,
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
 * Converts two lines from YUV420 to RGB565, within each iteration four
 * pixels (2 per line) are calculated and written to destination buffer.
 */
    .section .icode, "ax", %progbits

    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, %function

lcd_write_yuv420_lines:
                                        /* r0 = src = yuv_src */
                                        /* r1 = dst = out */
                                        /* r2 = width */
                                        /* r3 = stride */
    stmfd       sp!, {r4-r11,lr}        /* save non-scratch */
    ldmia       r0, {r10-r12}           /* r10 = yuv_src[0] = Y'_p */
                                        /* r11 = yuv_src[1] = Cb_p */
                                        /* r12 = yuv_src[2] = Cr_p */
    mov         r9, r2, lsl #1          /* r9 = 2*width (loop count) */
    str         r9, [sp, #-4]!          /* [--sp] = 2*width (constant) */
    add         r8, r10, r3             /* r8 = Y'_p + stride = Y'stride_p */
    mov         lr, r1                  /* RGB565 data destination buffer */

10:                                     /* loop start */
    ldrb        r0, [r11], #1           /* r0 = *Cb_p++ */
    ldrb        r1, [r12], #1           /* r1 = *Cr_p++ */
    ldrb        r3, [r8], #1            /* r3 = Y'3 */
    ldrb        r4, [r8], #1            /* r4 = Y'4 */

    sub         r0, r0, #128            /* r0 = Cb-128 */
    sub         r1, r1, #128            /* r1 = Cr-128 */

    add         r2, r1, r1, asl #1      /* r2 = Cr*51 + Cb*24 */
    add         r2, r2, r2, asl #4
    add         r2, r2, r0, asl #3
    add         r2, r2, r0, asl #4

    add         r5, r1, r1, asl #2      /* r1 = Cr*101 */
    add         r5, r5, r1, asl #5
    add         r1, r5, r1, asl #6

    add         r1, r1, #256            /* r1 = rv = (r1 + 256) >> 9 */
    mov         r1, r1, asr #9
    rsb         r2, r2, #128            /* r2 = guv = (-r2 + 128) >> 8 */
    mov         r2, r2, asr #8
    add         r0, r0, #2              /* r0 = bu = (Cb*128 + 256) >> 9 */
    mov         r0, r0, asr #2

    /* pixel_3 */
    sub         r3, r3, #16             /* r3 = (Y'-16) * (74/2) */
    add         r7, r3, r3, asl #2
    add         r3, r7, r3, asl #5

    add         r6, r1, r3, asr #8      /* r6 = r = (Y >> 9) + rv */
    add         r7, r2, r3, asr #7      /* r7 = g = (Y >> 8) + guv */
    add         r5, r0, r3, asr #8      /* r5 = b = (Y >> 9) + bu */

    orr         r3, r6, r5              /* check if clamping is needed... */
    orr         r3, r3, r7, asr #1      /* ...at all */
    cmp         r3, #31
    bls         15f                     /* no clamp */
    cmp         r6, #31                 /* clamp r */
    mvnhi       r6, r6, asr #31
    andhi       r6, r6, #31
    cmp         r7, #63                 /* clamp g */
    mvnhi       r7, r7, asr #31
    andhi       r7, r7, #63
    cmp         r5, #31                 /* clamp b */
    mvnhi       r5, r5, asr #31
    andhi       r5, r5, #31
15:                                     /* no clamp */

    /* calculate pixel_3 and save to r5 for later pixel packing */
    orr         r5, r5, r7, lsl #5      /* pixel_3 = r<<11 | g<<5 | b */
    orr         r5, r5, r6, lsl #11     /* r5 = pixel_3 */

    /* pixel_4 */
    sub         r4, r4, #16             /* r4 = (Y'-16) * (74/2) */
    add         r7, r4, r4, asl #2
    add         r4, r7, r4, asl #5

    add         r6, r1, r4, asr #8      /* r6 = r = (Y >> 9) + rv */
    add         r7, r2, r4, asr #7      /* r7 = g = (Y >> 8) + guv */
    add         r4, r0, r4, asr #8      /* r4 = b = (Y >> 9) + bu */

    orr         r3, r6, r4              /* check if clamping is needed... */
    orr         r3, r3, r7, asr #1      /* ...at all */
    cmp         r3, #31
    bls         15f                     /* no clamp */
    cmp         r6, #31                 /* clamp r */
    mvnhi       r6, r6, asr #31
    andhi       r6, r6, #31
    cmp         r7, #63                 /* clamp g */
    mvnhi       r7, r7, asr #31
    andhi       r7, r7, #63
    cmp         r4, #31                 /* clamp b */
    mvnhi       r4, r4, asr #31
    andhi       r4, r4, #31
15:                                     /* no clamp */

    /* calculate pixel_4 and pack with pixel_3 before writing */
    orr         r4, r4, r7, lsl #5      /* pixel_4 = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11     /* r4 = pixel_4 */
    orr         r5, r5, r4, lsl #16     /* r5 = pixel_4<<16 | pixel_3 */

    ldr         r7, [sp]                /* r7 = 2*width */
    ldrb        r3, [r10], #1           /* r3 = Y'1 */
    ldrb        r4, [r10], #1           /* r4 = Y'2 */

    str         r5, [lr, r7]            /* write pixel_3 and pixel_4 */

    /* pixel_1 */
    sub         r3, r3, #16             /* r3 = (Y'-16) * (74/2) */
    add         r7, r3, r3, asl #2
    add         r3, r7, r3, asl #5

    add         r6, r1, r3, asr #8      /* r6 = r = (Y >> 9) + rv */
    add         r7, r2, r3, asr #7      /* r7 = g = (Y >> 8) + guv */
    add         r5, r0, r3, asr #8      /* r5 = b = (Y >> 9) + bu */

    orr         r3, r6, r5              /* check if clamping is needed... */
    orr         r3, r3, r7, asr #1      /* ...at all */
    cmp         r3, #31
    bls         15f                     /* no clamp */
    cmp         r6, #31                 /* clamp r */
    mvnhi       r6, r6, asr #31
    andhi       r6, r6, #31
    cmp         r7, #63                 /* clamp g */
    mvnhi       r7, r7, asr #31
    andhi       r7, r7, #63
    cmp         r5, #31                 /* clamp b */
    mvnhi       r5, r5, asr #31
    andhi       r5, r5, #31
15:                                     /* no clamp */

    /* calculate pixel_1 and save to r5 for later pixel packing */
    orr         r5, r5, r7, lsl #5      /* pixel_1 = r<<11 | g<<5 | b */
    orr         r5, r5, r6, lsl #11     /* r5 = pixel_1 */

    /* pixel_2 */
    sub         r4, r4, #16             /* r4 = (Y'-16) * (74/2) */
    add         r7, r4, r4, asl #2
    add         r4, r7, r4, asl #5

    add         r6, r1, r4, asr #8      /* r6 = r = (Y >> 9) + rv */
    add         r7, r2, r4, asr #7      /* r7 = g = (Y >> 8) + guv */
    add         r4, r0, r4, asr #8      /* r4 = b = (Y >> 9) + bu */

    orr         r3, r6, r4              /* check if clamping is needed... */
    orr         r3, r3, r7, asr #1      /* ...at all */
    cmp         r3, #31
    bls         15f                     /* no clamp */
    cmp         r6, #31                 /* clamp r */
    mvnhi       r6, r6, asr #31
    andhi       r6, r6, #31
    cmp         r7, #63                 /* clamp g */
    mvnhi       r7, r7, asr #31
    andhi       r7, r7, #63
    cmp         r4, #31                 /* clamp b */
    mvnhi       r4, r4, asr #31
    andhi       r4, r4, #31
15:                                     /* no clamp */

    /* calculate pixel_2 and pack with pixel_1 before writing */
    orr         r4, r4, r7, lsl #5      /* pixel_2 = r<<11 | g<<5 | b */
    orr         r4, r4, r6, lsl #11     /* r4 = pixel_2 */
    orr         r5, r5, r4, lsl #16     /* r5 = pixel_2<<16 | pixel_1 */

    str         r5, [lr], #4            /* write pixel_1 and pixel_2 */

    subs        r9, r9, #4              /* check for loop end */
    bgt         10b                     /* back to beginning  */

    /* loop end */
    add         sp, sp, #4              /* deallocate stack */
    ldmpc       regs=r4-r11             /* restore registers */

    .ltorg
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines


#elif (YUV2RGB_VERSION == VERSION_ARMV5TE)
/****************************************************************************
 * How do I encode Y'CBCR components from R'G'B' in [0, +1]? (see ColorFAQ)
 *   |R|   |0.00456621  0           0.00625893| |Y' -  16|
 *   |G| = |0.00456621 -0.00153632 -0.00318811| |Pb - 128|
 *   |B|   |0.00456621  0.00791071  0         | |Pr - 128|
 *
 * Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
 */
#define NBR         14 /* 14-bit resolution (SVN) */
#define COEF_C0     74
#define COEF_C1     101
#define COEF_C2     -24
#define COEF_C3     -51
#define COEF_C4     128
#define C4_IS_POW2

/* constant for rounding a NBR number before down-scaling it to RS bits */
#define ROUND(RS)       (1 << (NBR - RS - 1))

/* packed 16-bit coefficients */
#define COEF_C4_C1      ((COEF_C4 << 16) | (COEF_C1 & 0xffff))
#define COEF_2C3_2C2    ((COEF_C3 << 17) | ((COEF_C2 << 1) & 0xffff))
/* 32-bit MLA constants */
#define CONST_MLA_Y     (-16 * COEF_C0)

/****************************************************************************
 * extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
 *                                    uint16_t* out,
 *                                    int width,
 *                                    int stride);
 *
 * Converts two lines from YUV420 to RGB565, within each iteration four
 * pixels (2 per line) are calculated and written to destination buffer.
 *
 * - use ARMv5TE+ 1-cycle multiply+accumulator instructions.
 */
    .section .icode, "ax", %progbits

    .align  2
    .global lcd_write_yuv420_lines
    .type   lcd_write_yuv420_lines, %function

lcd_write_yuv420_lines:
                                            @ r0 = src = yuv_src
                                            @ r1 = out = dst_p
                                            @ r2 = width
                                            @ r3 = stride
    stmfd       sp!, {r4-r11,lr}            @ save non-scratch
    ldmia       r0, {r10-r12}               @ r10 = yuv_src[0] = Y'_p
                                            @ r11 = yuv_src[1] = Cb_p
                                            @ r12 = yuv_src[2] = Cr_p
    adr         r0, const_data              @ load constants
    ldmia       r0, {r5-r8}                 @ r5 = COEF_C4_C1
                                            @ r6 = COEF_2C3_2C2
                                            @ r7 = COEF_C0
                                            @ r8 = CONST_MLA_Y
    sub         r4, r12, r11                @ r4 = Cr_p-Cb_p
    mov         r9, r2, asl #1              @ r9 = 2*width
    stmfd       sp!, {r4-r6,r9}             @ SP -> Cr_p-Cb_p
                                            @       COEF_C4_C1
                                            @       COEF_2C3_2C2
                                            @       2*width
    add         r12, r10, r3                @ r12 = Y'_p + stride = Y'stride_p
    mov         lr, r1                      @ RGB565 data destination buffer
    orr         r9, r7, r2, lsl #15         @ loop_count = width/2;
                                            @ r9 = loop_count<<16 | COEF_C0
    sub         r9, r9, #0x10000            @ loop_count--

10:                                         @ loop_start

    @ register usage:
    @  r8  = CONST_MLA_Y
    @  r9  = loop count<<16 | COEF_C0
    @  r10 = Y'_p
    @  r11 = Cb_p
    @  r12 = Y'stride_p
    @  lr  = dst_p
    @  free: r0-r7

    ldmia       sp, {r2-r4}                 @ r2 = Cr_p-Cb_p
                                            @ r3 = COEF_C4_C1
                                            @ r4 = COEF_2C3_2C2
    mov         r5, #ROUND(5)               @ r5 = round constant

    ldrb        r6, [r12], #1               @ r6 = Y'3
    ldrb        r7, [r12], #1               @ r7 = Y'4

    ldrb        r1, [r11, r2]               @ r1 = Cr = *Cr_p++
    ldrb        r0, [r11], #1               @ r0 = Cb = *Cb_p++

    /* calculate Y3 and Y4 */
    smlabb      r6, r6, r9, r8              @ r6 = Y3 = C0*Y'3 - C0*16
    smlabb      r7, r7, r9, r8              @ r7 = Y4 = C0*Y'4 - C0*16

    /* calculate rv, guv, bu */
    sub         r1, r1, #128                @ r1 = Cr" = Cr-128
    sub         r0, r0, #128                @ r0 = Cb" = Cb-128

    smlabt      r2, r1, r4, r5              @ r2 = guv" = Cr"*(2*C2) +
    smlabb      r2, r0, r4, r2              @             Cb"*(2*C3) + round
    smlabb      r1, r1, r3, r5              @ r1 = rv"  = Cr"*C1 + round
    #ifdef C4_IS_POW2
    add         r0, r5, r0, asl #NBR-7      @ r0 = bu"  = Cb"*C4 + round
    #else
    smlabt      r0, r0, r3, r5              @ r0 = bu"  = Cb"*C4 + round
    #endif

    /* scale rv",guv",bu" */
    mov         r2, r2, asr #NBR-5          @ r2 = guv = guv" >> scale
    mov         r1, r1, asr #NBR-5          @ r1 = rv = rv" >> scale
    mov         r0, r0, asr #NBR-5          @ r0 = bu = bu" >> scale

    @ register usage:
    @  r8-r12,lr: pointers, counters
    @  r0,r1,r2 = bu,rv,guv (rounded and scaled to RGB565)
    @  r6,r7 = Y'3,Y'4
    @  free: r3-r5

    /* pixel_3 */
    add         r5, r1, r6, asr #NBR-5      @ r5 = r = (Y3 >> scale) + rv
    add         r4, r2, r6, asr #NBR-6      @ r4 = g = (Y3 >> scale) + guv
    add         r3, r0, r6, asr #NBR-5      @ r3 = b = (Y3 >> scale) + bu

    orr         r6, r5, r3                  @ check if clamping is needed...
    orr         r6, r6, r4, asr #1          @ ...at all
    cmp         r6, #31
    bls         15f                         @ no clamp
    cmp         r5, #31                     @ clamp r
    mvnhi       r5, r5, asr #31
    andhi       r5, r5, #31
    cmp         r4, #63                     @ clamp g
    mvnhi       r4, r4, asr #31
    andhi       r4, r4, #63
    cmp         r3, #31                     @ clamp b
    mvnhi       r3, r3, asr #31
    andhi       r3, r3, #31
15:                                         @ no clamp

    /* calculate pixel_3 and save to r3 for later pixel packing */
    orr         r3, r3, r4, lsl #5          @ r3 = pixel_3 = r<<11 | g<<5 | b
    orr         r3, r3, r5, lsl #11

    /* pixel_4 */
    add         r5, r1, r7, asr #NBR-5      @ r5 = r = (Y4 >> scale) + rv
    add         r4, r2, r7, asr #NBR-6      @ r4 = g = (Y4 >> scale) + guv
    add         r7, r0, r7, asr #NBR-5      @ r7 = b = (Y4 >> scale) + bu

    orr         r6, r5, r7                  @ check if clamping is needed...
    orr         r6, r6, r4, asr #1          @ ...at all
    cmp         r6, #31
    bls         15f                         @ no clamp
    cmp         r5, #31                     @ clamp r
    mvnhi       r5, r5, asr #31
    andhi       r5, r5, #31
    cmp         r4, #63                     @ clamp g
    mvnhi       r4, r4, asr #31
    andhi       r4, r4, #63
    cmp         r7, #31                     @ clamp b
    mvnhi       r7, r7, asr #31
    andhi       r7, r7, #31
15:                                         @ no clamp

    /* calculate pixel_4 and pack with pixel_3 before writing */
    orr         r7, r7, r4, lsl #5          @ r7 = pixel_4 = r<<11 | g<<5 | b
    orr         r7, r7, r5, lsl #11
    orr         r3, r3, r7, lsl #16         @ r3 = pixel_4<<16 | pixel_3

    /* avoid interlocks when writing pixel_3 and pixel_4 */
    ldr         r5, [sp, #12]               @ r5 = 2*width

    ldrb        r6, [r10], #1               @ r6 = Y'1
    ldrb        r7, [r10], #1               @ r7 = Y'2

    /* write pixel_3 and pixel_4 */
    str         r3, [lr, r5]                @ [dst_p + 2*width] = r3

    @ register usage:
    @  r8-r12,lr: pointers, counters
    @  r0,r1,r2 = bu,rv,guv (rounded and scaled to RGB565)
    @  r6,r7 = Y'1,Y'2
    @  free: r3-r5

    /* calculate Y1 and Y2 */
    smlabb      r6, r6, r9, r8              @ r6 = Y1 = C0*Y'1 - C0*16
    smlabb      r7, r7, r9, r8              @ r7 = Y2 = C0*Y'2 - C0*16

    /* pixel_1 */
    add         r5, r1, r6, asr #NBR-5      @ r5 = r = (Y1 >> scale) + rv
    add         r4, r2, r6, asr #NBR-6      @ r4 = g = (Y1 >> scale) + guv
    add         r3, r0, r6, asr #NBR-5      @ r3 = b = (Y1 >> scale) + bu

    orr         r6, r5, r3                  @ check if clamping is needed...
    orr         r6, r6, r4, asr #1          @ ...at all
    cmp         r6, #31
    bls         15f                         @ no clamp
    cmp         r5, #31                     @ clamp r
    mvnhi       r5, r5, asr #31
    andhi       r5, r5, #31
    cmp         r4, #63                     @ clamp g
    mvnhi       r4, r4, asr #31
    andhi       r4, r4, #63
    cmp         r3, #31                     @ clamp b
    mvnhi       r3, r3, asr #31
    andhi       r3, r3, #31
15:                                         @ no clamp

    /* calculate pixel_1 and save to r3 for later pixel packing */
    orr         r3, r3, r4, lsl #5          @ r3 = pixel_1 = r<<11 | g<<5 | b
    orr         r3, r3, r5, lsl #11

    /* pixel_2 */
    add         r5, r1, r7, asr #NBR-5      @ r5 = r = (Y2 >> scale) + rv
    add         r4, r2, r7, asr #NBR-6      @ r4 = g = (Y2 >> scale) + guv
    add         r7, r0, r7, asr #NBR-5      @ r7 = b = (Y2 >> scale) + bu

    orr         r6, r5, r7                  @ check if clamping is needed...
    orr         r6, r6, r4, asr #1          @ ...at all
    cmp         r6, #31
    bls         15f                         @ no clamp
    cmp         r5, #31                     @ clamp r
    mvnhi       r5, r5, asr #31
    andhi       r5, r5, #31
    cmp         r4, #63                     @ clamp g
    mvnhi       r4, r4, asr #31
    andhi       r4, r4, #63
    cmp         r7, #31                     @ clamp b
    mvnhi       r7, r7, asr #31
    andhi       r7, r7, #31
15:                                         @ no clamp

    /* calculate pixel_2 and pack with pixel_1 before writing */
    orr         r7, r7, r4, lsl #5          @ r7 = pixel_2 = r<<11 | g<<5 | b
    orr         r7, r7, r5, lsl #11
    orr         r3, r3, r7, lsl #16         @ r3 = pixel_2 << 16 | pixel_1

    str         r3, [lr], #4                @ write pixel_1 and pixel_2

    /* check for loop end */
    subs        r9, r9, #0x10000            @ loop_count--
    bge         10b                         @ back to beginning

    /* bye */
    add         sp, sp, #16
    ldmpc       regs=r4-r11                 @ restore registers

    .ltorg
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines

/* data */
    .align 2
const_data:
    .word       COEF_C4_C1
    .word       COEF_2C3_2C2
    .word       COEF_C0
    .word       CONST_MLA_Y

    .size   const_data, .-const_data


#else   /* YUV2RGB_VERSION == VERSION_ARMV5TE_WST */
/****************************************************************************
 * How do I encode Y'CBCR components from R'G'B' in [0, +1]? (see ColorFAQ)
 *   |R|   |0.00456621  0           0.00625893| |Y' -  16|
 *   |G| = |0.00456621 -0.00153632 -0.00318811| |Pb - 128|
 *   |B|   |0.00456621  0.00791071  0         | |Pr - 128|
 *
 * Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
 */
#define NBR         14 /* 14-bit resolution (SVN) */
#define COEF_C0     74
#define COEF_C1     101
#define COEF_C2     -24
#define COEF_C3     -51
#define COEF_C4     128
#define C4_IS_POW2

/* packed 16-bit coefficients */
#define COEF_C4_C1      ((COEF_C4 << 16) | (COEF_C1 & 0xffff))
#define COEF_C3_C2      ((COEF_C3 << 16) | (COEF_C2 & 0xffff))

/* constant for rounding an NBR number before down-scaling it to RS bits */
#define ROUND(RS)       (1 << (NBR - RS - 1))

/* 32-bit MLA constants */
#define CONST_MLA_Y     (-16 * COEF_C0)
#define CONST_MLA_RV    ((-128 * COEF_C1) + ROUND(5))
#define CONST_MLA_BU    ((-128 * COEF_C4) + ROUND(5))
/* trick to save the register needed for table_sat6 reference:
   add table_sat6-table_sat5 offset (conveniently scaled) to guv MLA */
#define CONST_MLA_GUV   (-128 * (COEF_C2 + COEF_C3) + ROUND(6) + \
                                ((table_sat6 - table_sat5) << (NBR - 6)))

/****************************************************************************
 * extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
 *                                    uint16_t* out,
 *                                    int width,
 *                                    int stride);
 *
 * Converts two lines from YUV420 to RGB565, within each iteration four
 * pixels (2 per line) are calculated and written to destination buffer.
 *
 * - use ARMv5TE+ 1-cycle multiply+accumulator instructions.
 * - use data tables (256 bytes) for RBG565 saturation.
 */
    .section .icode, "ax", %progbits

    .align  2
    .global lcd_write_yuv420_lines
    .type   lcd_write_yuv420_lines, %function

lcd_write_yuv420_lines:
                                            @ r0 = src = yuv_src
                                            @ r1 = out = dst1_p
                                            @ r2 = width
                                            @ r3 = stride
    stmfd       sp!, {r4-r11,lr}            @ save non-scratch
    ldmia       r0, {r10-r12}               @ r10 = yuv_src[0] = Y'_p
                                            @ r11 = yuv_src[1] = Cb_p
                                            @ r12 = yuv_src[2] = Cr_p
    /* prepare data and fill stack */
    adr         r0, const_data              @ load constants
    ldmia       r0, {r4-r9,lr}              @ r4 = COEF_C0
                                            @ r5 = CONST_MLA_GUV
                                            @ r6 = COEF_C3_C2
                                            @ r7 = CONST_MLA_BU
                                            @ r8 = COEF_C4_C1
                                            @ r9 = CONST_MLA_RV
                                            @ lr = table_sat5
    sub         r0, r12, r11                @ r0 = Cr_p-Cb_p
                #define STACK_SZ 28
    stmfd       sp!, {r0,r5-r9,lr}          @ SP -> Cr_p-Cb_p
                                            @       CONST_MLA_GUV
                                            @       COEF_C3_C2
                                            @       CONST_MLA_BU
                                            @       COEF_C4_C1
                                            @       CONST_MLA_RV
                                            @       table_sat5
    mov         r8, r4, lsl #4              @
    rsb         r8, #0                      @ r8 = -16*COEF_C0 = CONST_MLA_Y
    mov         lr, r1                      @ RGB565 data destination buffer
    add         r9, lr, r2, asl #1          @ r9 = out + 2*width = dst2_p
    add         r12, r3, r10                @ r12 = Y'_p + stride
    orr         r7, r4, r2, lsl #15         @ loop_count = width/2;
                                            @ r7 = loop_count<<16 | COEF_C0
    sub         r7, r7, #0x10000            @ loop_count--

    /* align loop code to minimize occupied lines, execution
       time per loop is optimized ~10% on ARM926EJ-S */
    .align CACHEALIGN_BITS
loop_start:

    @ register usage:
    @  r7 = loop count<<16 | COEF_C0
    @  r8 = CONST_MLA_Y
    @  r9 = dst2_p
    @  r10 = Y'_p
    @  r11 = Cb_p
    @  r12 = Y'stride_p
    @  lr = dst1_p
    @  free: r0-r6

    /* load constants from stack */
    ldmia       sp, {r1-r3,r6}              @ r1 = Cr_p-Cb_p
                                            @ r2 = CONST_MLA_GUV
                                            @ r3 = COEF_C3_C2
                                            @ r6 = CONST_MLA_BU

    /* read Cr", Cb" */
    ldrb        r1, [r11, r1]               @ r1 = Cr = *Cr_p++
    ldrb        r0, [r11], #1               @ r0 = Cb = *Cb_p++

    /* load more constants (avoids r1 interlock) */
    ldrd        r4, [sp, #16]               @ r4 = COEF_C4_C1
                                            @ r5 = CONST_MLA_RV

    /* calculate rv", guv", bu" */
    smlabt      r2, r1, r3, r2              @ r2 = guv" = Cr*C2 + Cb*C3
    smlabb      r2, r0, r3, r2              @                  + CONST_MLA_GUV
    smlabb      r1, r1, r4, r5              @ r1 = rv" = Cr*C1 + CONST_MLA_RV
    #ifdef C4_IS_POW2
    add         r0, r6, r0, asl #NBR-7      @ r0 = bu" = Cb*C4 + CONST_MLA_BU
    #else
    smlabt      r0, r0, r4, r6              @ r0 = bu" = Cb*C4 + CONST_MLA_BU
    #endif

    ldr         r4, [sp, #STACK_SZ-4]       @ r4 = table_sat5

    /* read Y'1 and Y'2 */
    ldrb        r5, [r10], #1               @ r5 = Y'1 = *Y'_p++
    ldrb        r6, [r10], #1               @ r6 = Y'2 = *Y'_p++

    /* scale rv",guv",bu", adding sat5_p here saves instructions later */
    add         r1, r4, r1, asr #NBR-5      @ r1 = rv'  = sat5_p + rv">>scale
    add         r2, r4, r2, asr #NBR-6      @ r2 = guv' = sat5_p + guv">>scale
    add         r0, r4, r0, asr #NBR-5      @ r0 = bu'  = sat5_p + bu">>scale

    @ register usage:
    @  r7-r12,lr: pointers, counters, tables
    @  r0,r1,r2 = (bu,rv,guv) rounded and RGB565 scaled
    @  r5,r6 = Y'1,Y'2
    @  free: r3,r4

    /* calculate Y1 and Y2 */
    smlabb      r5, r5, r7, r8              @ r5 = Y1 = C0*Y'1 - 16*C0
    smlabb      r6, r6, r7, r8              @ r6 = Y2 = C0*Y'2 - 16*C0

    /* pixel_1 */
    ldrb        r3, [r0, r5, asr #NBR-5]    @ r3 = b = sat5[Y1>>scale + bu']
    ldrb        r4, [r2, r5, asr #NBR-6]    @ r4 = g = sat6[Y1>>scale + guv']
    ldrb        r5, [r1, r5, asr #NBR-5]    @ r5 = r = sat5[Y1>>scale + rv']

    /* calculate pixel_1 */
    orr         r3, r3, r4, lsl #5          @ r3 = pixel_1 = g<<5 | b

    /* pixel_2 (avoid r5 interlock) */
    ldrb        r4, [r0, r6, asr #NBR-5]    @ r4 = b = sat5[Y2>>scale + bu']

    /* calculate pixel_1 and save to r3 for later pixel packing */
    orr         r3, r3, r5, lsl #11         @ r3 = pixel_1 = r<<11 | g<<5 | b

    /* pixel_2 */
    ldrb        r5, [r2, r6, asr #NBR-6]    @ r5 = g = sat6[Y2>>scale + guv']
    ldrb        r6, [r1, r6, asr #NBR-5]    @ r6 = r = sat5[Y2>>scale + rv']

    /* calculate pixel_2 and pack with pixel_1 before writing */
    orr         r3, r3, r4, lsl #16         @ r3 = pixel_2<<16 | pixel_1
    orr         r3, r3, r5, lsl #21
    orr         r3, r3, r6, lsl #27

    /* read Y'3 and Y'4 */
    ldrb        r5, [r12], #1               @ r5 = Y'3 = *Y'stride_p++
    ldrb        r6, [r12], #1               @ r6 = Y'4 = *Y'stride_p++

    /* write pixel_1 and pixel_2 */
    str         r3, [lr], #4                @ *dst2_p++ = r3

    @ register usage:
    @  r7-r12,lr: pointers, counters, tables
    @  r0,r1,r2 = (bu,rv,guv) rounded and RGB565 scaled
    @  r5,r6 = Y'3,Y'4
    @  free: r3,r4

    /* calculate Y3 and Y4 */
    smlabb      r5, r5, r7, r8              @ r5 = Y3 = C0*Y'3 - 16*C0
    smlabb      r6, r6, r7, r8              @ r6 = Y4 = C0*Y'4 - 16*C0

    /* pixel_3 */
    ldrb        r3, [r0, r5, asr #NBR-5]    @ r3 = b = sat5[Y3>>scale + bu']
    ldrb        r4, [r2, r5, asr #NBR-6]    @ r4 = g = sat6[Y3>>scale + guv']
    ldrb        r5, [r1, r5, asr #NBR-5]    @ r5 = r = sat5[Y3>>scale + rv']

    /* calculate pixel_3 */
    orr         r3, r3, r4, lsl #5          @ r3 = pixel_3 = g<<5 | b

    /* pixel_4 (avoid r5 interlock) */
    ldrb        r4, [r0, r6, asr #NBR-5]    @ r4 = b = sat5[Y4>>scale + bu']

    /* calculate pixel_3 and save to r3 for later pixel packing */
    orr         r3, r3, r5, lsl #11         @ r3 = pixel_3 = r<<11 | g<<5 | b

    /* pixel_4 */
    ldrb        r5, [r2, r6, asr #NBR-6]    @ r5 = g = sat6[Y4>>scale + guv']
    ldrb        r6, [r1, r6, asr #NBR-5]    @ r6 = r = sat5[Y4>>scale + rv']

    /* calculate pixel_4 and pack with pixel_3 before writing */
    orr         r3, r3, r4, lsl #16         @ r3 = pixel_4 << 16 | pixel_3
    orr         r3, r3, r5, lsl #21
    orr         r3, r3, r6, lsl #27

    /* write pixel_3 and pixel_4 */
    str         r3, [r9], #4                @ *dst1_p++ = r3

    /* check for loop end */
    subs        r7, r7, #0x10000            @ loop_count--
    bge         loop_start                  @ back to beginning

    /* bye */
    add         sp, sp, #STACK_SZ           @ deallocate stack
    ldmpc       regs=r4-r11                 @ restore registers

    .ltorg
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines

/* data */
    .align 2
const_data:
    .word       COEF_C0
    .word       CONST_MLA_GUV
    .word       COEF_C3_C2
    .word       CONST_MLA_BU
    .word       COEF_C4_C1
    .word       CONST_MLA_RV
    .word       table_sat5

    .size   const_data, .-const_data

/* saturation tables */
    /*.section .data*/
    /* aligned to cache line size to minimize cache usage */
    .align CACHEALIGN_BITS

saturation_tables:
    /* 5-bit saturation table [-36..0..+67], size=104 */
    /* table_sat5[-36..-1] */
    .byte                                                        0,  0,  0,  0
    .byte        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    .byte        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  table_sat5:
    /* table_sat5[0..67] */
    .byte        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    .byte       16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    .byte       31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31
    .byte       31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31
    .byte       31, 31, 31, 31

    /* 6-bit saturation table [-44..0..+107], size=152 */
    /* table_sat6[-44..-1] */
    .byte                        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    .byte        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    .byte        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  table_sat6:
    /* table_sat6[0..107] */
    .byte        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    .byte       16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    .byte       32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
    .byte       48, 49, 50, 51, 52, 53 ,54, 55, 56, 57, 58, 59, 60, 61, 62, 63
    .byte       63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63
    .byte       63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63
    .byte       63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63

    .size   saturation_tables, .-saturation_tables
#endif  /* YUV2RGB_VERSION */
