/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"


#if CONFIG_CPU == PP5002
    /* Causes ATA retries on iPod G3 probably related to improper controller
     * setup. Needs investigation. */
    .section    .icode,"ax",%progbits
    .equ    .ata_port, 0xc00031e0
#elif defined CPU_PP502x
    /* Verified working on (PP5020, PP5022) targets */
    .section    .icode,"ax",%progbits
    .equ    .ata_port, 0xc30001e0
#elif CONFIG_CPU == S3C2440
    /* Untested */
    .text
    .equ    .ata_port, 0x18000000
#elif defined(CREATIVE_ZVM)
    .text
    .equ    .ata_port, 0x50FEE000
#endif

    .align  2
    .global copy_read_sectors
    .type   copy_read_sectors,%function
    
/* Read a number of words from the ATA data port
 *
 * Optimised for speed; assumes wordcount >= 10
 *
 * Arguments:
 *   r0 - buffer address
 *   r1 - word count
 *
 * Register usage:
 *   r0 - current address
 *   r1 - word count
 *   r2 - ata port
 *   r3..r6, lr - read buffers
 */

copy_read_sectors:
    stmfd   sp!, {r4, r5, r6, lr}
    ldr     r2, =.ata_port
    tst     r0, #1              /* 16 bit aligned? */
    beq     .r_aligned

    /* not 16-bit aligned */
    sub     r1, r1, #1          /* one halfword is handled unconditionally */
    ldrh    r3, [r2]            /* read first halfword */
    strb    r3, [r0], #1        /* store low byte */
    mov     r3, r3, lsr #8

    tst     r0, #2              /* 32 bit aligned? */
    beq     .r_noword_u
    ldrh    r4, [r2]            /* read second halfword */
    orr     r3, r3, r4, lsl #8  /* combine with old byte */
    strh    r3, [r0], #2        /* store */
    mov     r3, r4, lsr #8
    sub     r1, r1, #1          /* another halfword taken */
.r_noword_u:

    sub     r1, r1, #8          /* adjust for zero-check and doing 8 halfwords/loop */
.r_loop_u:
    ldrh    r4, [r2]            /* Read 8 halfwords and combine them into */
    orr     r3, r3, r4, lsl #8  /*  4 words so that they're properly aligned */
    ldrh    r4, [r2]            /*  in memory. Bottom byte of first word is */
    orr     r3, r3, r4, lsl #24 /*  the top byte from the last round. Write */
    mov     r4, r4, lsr #8      /*  all 4 words at once. */
    ldrh    r5, [r2]
    orr     r4, r4, r5, lsl #8
    ldrh    r5, [r2]
    orr     r4, r4, r5, lsl #24
    mov     r5, r5, lsr #8
    ldrh    r6, [r2]
    orr     r5, r5, r6, lsl #8
    ldrh    r6, [r2]
    orr     r5, r5, r6, lsl #24
    mov     r6, r6, lsr #8
    ldrh    lr, [r2]
    orr     r6, r6, lr, lsl #8
    ldrh    lr, [r2]
    orr     r6, r6, lr, lsl #24
    stmia   r0!, {r3, r4, r5, r6}
    mov     r3, lr, lsr #8
    subs    r1, r1, #8          /* 8 or more halfwords left? */
    bge     .r_loop_u

    /* No need to adjust the count, only checking bits from now on. */
    tst     r1, #4              /* 4 or more halfwords left? */
    beq     .r_end4_u
    ldrh    r4, [r2]
    orr     r3, r3, r4, lsl #8
    ldrh    r4, [r2]
    orr     r3, r3, r4, lsl #24
    mov     r4, r4, lsr #8
    ldrh    r5, [r2]
    orr     r4, r4, r5, lsl #8
    ldrh    r5, [r2]
    orr     r4, r4, r5, lsl #24
    stmia   r0!, {r3, r4}
    mov     r3, r5, lsr #8
.r_end4_u:

    tst     r1, #2              /* 2 or more halfwords left? */
    beq     .r_end2_u
    ldrh    r4, [r2]
    orr     r3, r3, r4, lsl #8
    ldrh    r4, [r2]
    orr     r3, r3, r4, lsl #24
    str     r3, [r0], #4
    mov     r3, r4, lsr #8
.r_end2_u:

    tst     r1, #1              /* one halfword left? */
    ldrneh  r4, [r2]
    orrne   r3, r3, r4, lsl #8
    strneh  r3, [r0], #2
    movne   r3, r4, lsr #8

    strb    r3, [r0], #1        /* store final byte */

    ldmfd   sp!, {r4, r5, r6, pc}

    /* 16-bit aligned */
.r_aligned:
    tst     r0, #2              /* 32 bit aligned? */
    ldrneh  r3, [r2]            /* no: read first halfword */
    strneh  r3, [r0], #2        /*     store */
    subne   r1, r1, #1          /*     one halfword taken */

    sub     r1, r1, #8          /* adjust for zero-check and doing 8 halfwords/loop */
.r_loop_a:
    ldrh    r3, [r2]            /* Read 8 halfwords and combine each pair */
    ldrh    r4, [r2]            /*  into a word, then store all at once. */
    orr     r3, r3, r4, lsl #16
    ldrh    r4, [r2]
    ldrh    r5, [r2]
    orr     r4, r4, r5, lsl #16
    ldrh    r5, [r2]
    ldrh    r6, [r2]
    orr     r5, r5, r6, lsl #16
    ldrh    r6, [r2]
    ldrh    lr, [r2]
    orr     r6, r6, lr, lsl #16
    stmia   r0!, {r3, r4, r5, r6}
    subs    r1, r1, #8          /* 8 or more halfwords left? */
    bge     .r_loop_a
    
    /* No need to adjust the count, only checking bits from now on. */
    tst     r1, #4              /* 4 or more halfwords left? */
    beq     .r_end4_a
    ldrh    r3, [r2]
    ldrh    r4, [r2]
    orr     r3, r3, r4, lsl #16
    ldrh    r4, [r2]
    ldrh    r5, [r2]
    orr     r4, r4, r5, lsl #16
    stmia   r0!, {r3, r4}
.r_end4_a:

    tst     r1, #2              /* 2 or more halfwords left? */
    ldrneh  r3, [r2]
    ldrneh  r4, [r2]
    orrne   r3, r3, r4, lsl #16
    strne   r3, [r0], #4

    tst     r1, #1              /* one halfword left? */
    ldrneh  r3, [r2]
    strneh  r3, [r0], #2

    ldmfd   sp!, {r4, r5, r6, pc}

.r_end:
    .size   copy_read_sectors,.r_end-copy_read_sectors

    .align  2
    .global copy_write_sectors
    .type   copy_write_sectors,%function
    
/* Write a number of words to the ATA data port
 *
 * Optimised for speed; assumes wordcount >= 10
 *
 * Arguments:
 *   r0 - buffer address
 *   r1 - word count
 *
 * Register usage:
 *   r0 - current address
 *   r1 - word count
 *   r2 - ata port
 *   r3..r6, lr - read buffers
 */

copy_write_sectors:
    stmfd   sp!, {r4, r5, r6, lr}
    ldr     r2, =.ata_port
    tst     r0, #1              /* 16 bit aligned? */
    beq     .w_aligned
    
    /* not 16-bit aligned */
    sub     r1, r1, #1          /* one halfword is done unconditionally */
    ldrb    r3, [r0], #1        /* load 1st byte, now halfword aligned. */

    tst     r0, #2              /* 32 bit aligned? */
    beq     .w_noword_u
    ldrh    r4, [r0], #2        /* load a halfword */
    orr     r3, r3, r4, lsl #8  /* combine with old byte */
    strh    r3, [r2]            /* write halfword */
    mov     r3, r4, lsr #8
    sub     r1, r1, #1          /* another halfword taken */
.w_noword_u:

    sub     r1, r1, #8          /* adjust for zero-check and doing 8 halfwords/loop */
.w_loop_u:
    ldmia   r0!, {r4, r5, r6, lr}
    orr     r3, r3, r4, lsl #8  /* Load 4 words at once and decompose them */
    strh    r3, [r2]            /*  into 8 halfwords in a way that the words */
    mov     r3, r3, lsr #16     /*  are shifted by 8 bits, putting the high */
    strh    r3, [r2]            /*  byte of one word into the low byte of */
    mov     r4, r4, lsr #24     /*  the next. High byte of last word becomes */
    orr     r4, r4, r5, lsl #8  /*  low byte of next round. */
    strh    r4, [r2]
    mov     r4, r4, lsr #16
    strh    r4, [r2]
    mov     r5, r5, lsr #24
    orr     r5, r5, r6, lsl #8
    strh    r5, [r2]
    mov     r5, r5, lsr #16
    strh    r5, [r2]
    mov     r6, r6, lsr #24
    orr     r6, r6, lr, lsl #8
    strh    r6, [r2]
    mov     r6, r6, lsr #16
    strh    r6, [r2]
    mov     r3, lr, lsr #24
    subs    r1, r1, #8          /* 8 or more halfwords left? */
    bge     .w_loop_u
    
    /* No need to adjust the count, only checking bits from now on. */
    tst     r1, #4              /* 4 or more halfwords left? */
    beq     .w_end4_u
    ldmia   r0!, {r4, r5}
    orr     r3, r3, r4, lsl #8
    strh    r3, [r2]
    mov     r3, r3, lsr #16
    strh    r3, [r2]
    mov     r4, r4, lsr #24
    orr     r4, r4, r5, lsl #8
    strh    r4, [r2]
    mov     r4, r4, lsr #16
    strh    r4, [r2]
    mov     r3, r5, lsr #24
.w_end4_u:

    tst     r1, #2              /* 2 or more halfwords left? */
    beq     .w_end2_u
    ldr     r4, [r0], #4
    orr     r3, r3, r4, lsl #8
    strh    r3, [r2]
    mov     r3, r3, lsr #16
    strh    r3, [r2]
    mov     r3, r4, lsr #24
.w_end2_u:

    tst     r1, #1              /* one halfword left? */
    ldrneh  r4, [r0], #2
    orrne   r3, r3, r4, lsl #8
    strneh  r3, [r2]
    movne   r3, r3, lsr #16
    
    ldrb    r4, [r0], #1        /* load final byte */
    orr     r3, r3, r4, lsl #8
    strh    r3, [r2]            /* write final halfword */

    ldmfd   sp!, {r4, r5, r6, pc}

    /* 16-bit aligned */
.w_aligned:
    tst     r0, #2              /* 32 bit aligned? */
    ldrneh  r3, [r0], #2        /* no: load first halfword */
    strneh  r3, [r2]            /*     write */
    subne   r1, r1, #1          /*     one halfword taken */

    sub     r1, r1, #8          /* adjust for zero-check and doing 8 halfwords/loop */
.w_loop_a:
    ldmia   r0!, {r3, r4, r5, r6}
    strh    r3, [r2]            /* Load 4 words and decompose them into */
    mov     r3, r3, lsr #16     /*  2 halfwords each, and write those. */
    strh    r3, [r2]
    strh    r4, [r2]
    mov     r4, r4, lsr #16
    strh    r4, [r2]
    strh    r5, [r2]
    mov     r5, r5, lsr #16
    strh    r5, [r2]
    strh    r6, [r2]
    mov     r6, r6, lsr #16
    strh    r6, [r2]
    subs    r1, r1, #8          /* 8 or more halfwords left? */
    bge     .w_loop_a

    /* No need to adjust the count, only checking bits from now on. */
    tst     r1, #4              /* 4 or more halfwords left? */
    beq     .w_end4_a
    ldmia   r0!, {r3, r4}
    strh    r3, [r2]
    mov     r3, r3, lsr #16
    strh    r3, [r2]
    strh    r4, [r2]
    mov     r4, r4, lsr #16
    strh    r4, [r2]
.w_end4_a:

    tst     r1, #2              /* 2 or more halfwords left? */
    ldrne   r3, [r0], #4
    strneh  r3, [r2]
    movne   r3, r3, lsr #16
    strneh  r3, [r2]

    tst     r1, #1              /* one halfword left? */
    ldrneh  r3, [r0], #2
    strneh  r3, [r2]

    ldmfd   sp!, {r4, r5, r6, pc}

.w_end:
    .size   copy_write_sectors,.w_end-copy_write_sectors
