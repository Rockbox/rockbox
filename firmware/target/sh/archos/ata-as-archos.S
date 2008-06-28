/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004-2006 by Jens Arnold
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

    .section    .icode,"ax",@progbits

    .align  2
    .global _copy_read_sectors
    .type   _copy_read_sectors,@function
    
/* Read a number of words from the ATA data port
 *
 * Assumes wordcount to be a multiple of 4
 *
 * Arguments:
 *   r4 - buffer address
 *   r5 - word count
 *
 * Register usage:
 *   r0 - scratch
 *   r1/r2 - read buffers
 *   r3 - mask (if unaligned)
 *   r4 - current address
 *   r5 - end address
 *   r6 - ata port
 */

_copy_read_sectors:
    add     r5, r5          /* words -> bytes */
    add     r4, r5          /* bytes -> end address */
    add     #-12, r5        /* adjust for offsets */
    mov.l   .ata_data, r6

    mov     r4, r0
    tst     #1, r0          /* 16-bit aligned ? */
    bt      .r_aligned      /* yes, do word copy */

    /* not 16-bit aligned */
    mov     #-1, r3         /* prepare a bit mask for high byte */
    shll8   r3              /* r3 = 0xFFFFFF00 */
    
    mov.w   @r6, r2         /* read first word (1st round) */
    mov.b   r2, @r4         /* store low byte of first word */
    bra     .r_start_b      /* jump into loop after next instr. */
    add     #-5, r4         /* adjust for dest. offsets; now even */

    .align  2
.r_loop_b:                  /* main loop: copy 4 words in a row */
    mov.w   @r6, r2         /* read first word (2+ round) */
    and     r3, r1          /* get high byte of fourth word (2+ round) */
    extu.b  r2, r0          /* get low byte of first word (2+ round) */
    or      r1, r0          /* combine with high byte of fourth word */
    mov.w   r0, @(4, r4)    /* store at buf[4] */
    nop                     /* maintain alignment */
.r_start_b:
    mov.w   @r6, r1         /* read second word */
    and     r3, r2          /* get high byte of first word */
    extu.b  r1, r0          /* get low byte of second word */
    or      r2, r0          /* combine with high byte of first word */
    mov.w   r0, @(6, r4)    /* store at buf[6] */
    add     #8, r4          /* buf += 8 */
    mov.w   @r6, r2         /* read third word */
    and     r3, r1          /* get high byte of second word */
    extu.b  r2, r0          /* get low byte of third word */
    or      r1, r0          /* combine with high byte of second word */
    mov.w   r0, @r4         /* store at buf[0] */
    cmp/hi  r4, r5          /* check for end */
    mov.w   @r6, r1         /* read fourth word */
    and     r3, r2          /* get high byte of third word */
    extu.b  r1, r0          /* get low byte of fourth word */
    or      r2, r0          /* combine with high byte of third word */
    mov.w   r0, @(2, r4)    /* store at buf[2] */
    bt      .r_loop_b
    /* 24 instructions for 4 copies, takes 30 clock cycles (4 wait) */
    /* avg. 7.5 cycles per word */

    swap.b  r1, r0          /* get high byte of last word */
    rts
    mov.b   r0, @(4, r4)    /* and store it */

    /* 16-bit aligned, loop(read and store word) */
.r_aligned:
    mov.w   @r6, r2         /* read first word (1st round) */
    bra     .r_start_w      /* jump into loop after next instr. */
    add     #-6, r4         /* adjust for destination offsets */

    .align  2
.r_loop_w:                  /* main loop: copy 4 words in a row */
    mov.w   @r6, r2         /* read first word (2+ round) */
    swap.b  r1, r0          /* swap fourth word (2+ round) */
    mov.w   r0, @(4, r4)    /* store fourth word (2+ round) */
    nop                     /* maintain alignment */
.r_start_w:
    mov.w   @r6, r1         /* read second word */
    swap.b  r2, r0          /* swap first word */
    mov.w   r0, @(6, r4)    /* store first word in buf[6] */
    add     #8, r4          /* buf += 8 */
    mov.w   @r6, r2         /* read third word */
    swap.b  r1, r0          /* swap second word */
    mov.w   r0, @r4         /* store second word in buf[0] */
    cmp/hi  r4, r5          /* check for end */
    mov.w   @r6, r1         /* read fourth word */
    swap.b  r2, r0          /* swap third word */
    mov.w   r0, @(2, r4)    /* store third word */
    bt      .r_loop_w
    /* 16 instructions for 4 copies, takes 22 clock cycles (4 wait) */
    /* avg. 5.5 cycles per word */

    swap.b  r1, r0          /* swap fourth word (last round) */
    rts
    mov.w   r0, @(4, r4)    /* and store it */

.r_end:
    .size   _copy_read_sectors,.r_end-_copy_read_sectors

    .align  2
    .global _copy_write_sectors
    .type   _copy_write_sectors,@function
    
/* Write a number of words to the ATA data port
 *
 * Assumes wordcount to be a multiple of 2.
 * Writing is not unrolled as much as reading, for several reasons:
 *
 * - a similar instruction sequence is faster for writing than for reading
 *   because the auto-incrementing load instructions can be used
 * - writing profits from warp mode
 *
 * Both of these add up to have writing faster than the more unrolled reading.
 *
 * Arguments:
 *   r4 - buffer address
 *   r5 - word count
 *
 * Register usage:
 *   r0/r1 - scratch
 *   r2/r3 - write buffers
 *   r4 - current address
 *   r5 - end address
 *   r6 - mask (if unaligned)
 *   r7 - ata port
 */

_copy_write_sectors:
    add     r5, r5          /* words -> bytes */
    add     r4, r5          /* bytes -> end address */
    add     #-4, r5         /* adjust for offsets */
    mov.l   .ata_data, r7

    mov     r4, r0
    tst     #1, r0          /* 16-bit aligned ? */
    bt      .w_aligned      /* yes, do word copy */

    /* not 16-bit aligned */
    mov     #-1, r6         /* prepare a bit mask for high byte */
    shll8   r6              /* r6 = 0xFFFFFF00 */

    mov.b   @r4+, r2        /* load (initial old second) first byte */
    mov.w   @r4+, r3        /* load (initial) first word */
    bra     .w_start_b
    extu.b  r2, r0          /* extend unsigned */

    .align  2
.w_loop_b:                  /* main loop: copy 2 words in a row */
    mov.w   @r4+, r3        /* load first word (2+ round) */
    extu.b  r2, r0          /* put away low byte of second word (2+ round) */
    and     r6, r2          /* get high byte of second word (2+ round) */
    or      r1, r2          /* combine with low byte of old first word */
    mov.w   r2, @r7         /* write that */
.w_start_b:
    cmp/hi  r4, r5          /* check for end */
    mov.w   @r4+, r2        /* load second word */
    extu.b  r3, r1          /* put away low byte of first word */
    and     r6, r3          /* get high byte of first word */
    or      r0, r3          /* combine with high byte of old second word */
    mov.w   r3, @r7         /* write that */
    bt      .w_loop_b
    /* 12 instructions for 2 copies, takes 14 clock cycles */
    /* avg. 7 cycles per word */

    /* the loop "overreads" 1 byte past the buffer end, however, the last */
    /* byte is not written to disk */
    and     r6, r2          /* get high byte of last word */
    or      r1, r2          /* combine with low byte of old first word */
    rts
    mov.w   r2, @r7         /* write last word */

    /* 16-bit aligned, loop(load and write word) */
.w_aligned:
    bra     .w_start_w      /* jump into loop after next instr. */
    mov.w   @r4+, r2        /* load first word (1st round) */

    .align  2
.w_loop_w:                  /* main loop: copy 2 words in a row */
    mov.w   @r4+, r2        /* load first word (2+ round) */
    swap.b  r1, r0          /* swap second word (2+ round) */
    mov.w   r0, @r7         /* write second word (2+ round) */
.w_start_w:
    cmp/hi  r4, r5          /* check for end */
    mov.w   @r4+, r1        /* load second word */
    swap.b  r2, r0          /* swap first word */
    mov.w   r0, @r7         /* write first word */
    bt      .w_loop_w
    /* 8 instructions for 2 copies, takes 10 clock cycles */
    /* avg. 5 cycles per word */

    swap.b  r1, r0          /* swap second word (last round) */
    rts
    mov.w   r0, @r7         /* and write it */

.w_end:
    .size   _copy_write_sectors,.w_end-_copy_write_sectors

    .align  2
.ata_data:
    .long   0x06104100      /* ATA data port */
