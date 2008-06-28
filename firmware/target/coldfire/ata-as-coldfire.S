/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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

    .equ    .ata_port, 0x20000020

    .align  2
    .global copy_read_sectors
    .type   copy_read_sectors,@function
    
/* Read a number of words from the ATA data port
 *
 * Utilises line bursts, assumes there is at least one full line to copy.
 *
 * Arguments:
 *   (4,%sp) - buffer address
 *   (8,%sp) - word count
 *
 * Register usage:
 *   %a0 - current address
 *   %a1 - end address
 *   %a2 - ata port
 *   %d0 - scratch
 *   %d1 - shift count
 *   %d2-%d6 - read buffers
 */

copy_read_sectors:
    lea.l   (-24, %sp), %sp
    movem.l %d2-%d6/%a2, (%sp)
    movem.l (28, %sp), %a0-%a1
    add.l   %a1, %a1
    add.l   %a0, %a1
    lea.l   .ata_port, %a2

    move.l  %a0, %d0
    btst.l  #0, %d0         /* 16-bit aligned? */
    jeq     .r_aligned      /* yes, do word copy */

    /* not 16-bit aligned */
    subq.l  #1, %a1         /* last byte is done unconditionally */
    moveq.l #24, %d1        /* preload shift count */

    move.w  (%a2), %d2      /* load initial word */
    move.l  %d2, %d3
    lsr.l   #8, %d3
    move.b  %d3, (%a0)+     /* write high byte of it, aligns dest addr */

    btst.l  #1, %d0         /* longword aligned? (testing old d0 value!) */
    bne.b   .r_end_u_w1     /* yes, skip leading word handling */

    swap    %d2             /* move initial word up */
    move.w  (%a2), %d2      /* combine with second word */
    move.l  %d2, %d3
    lsr.l   #8, %d3
    move.w  %d3, (%a0)+     /* write bytes 2 and 3 as word */

.r_end_u_w1:
    moveq.l #12, %d0
    add.l   %a0, %d0
    and.l   #0xFFFFFFF0,%d0 /* d0 == first line bound */
    cmp.l   %a0, %d0        /* any leading longwords? */
    bls.b   .r_end_u_l1     /* no: skip loop */
        
.r_loop_u_l1:
    move.w  (%a2), %d3      /* load first word */
    swap    %d3             /* move to upper 16 bit */
    move.w  (%a2), %d3      /* load second word */
    move.l  %d3, %d4
    lsl.l   %d1, %d2
    lsr.l   #8, %d3
    or.l    %d3, %d2        /* combine old low byte with new top 3 bytes */
    move.l  %d2, (%a0)+     /* store as long */
    move.l  %d4, %d2
    cmp.l   %a0, %d0        /* run up to first line bound */
    bhi.b   .r_loop_u_l1

.r_end_u_l1:
    lea.l   (-14, %a1), %a1 /* adjust end addr. to 16 bytes/pass */

.r_loop_u_line:
    move.w  (%a2), %d3      /* load 1st word */
    swap    %d3             /* move to upper 16 bit */
    move.w  (%a2), %d3      /* load 2nd word */
    move.l  %d3, %d0
    lsl.l   %d1, %d2
    lsr.l   #8, %d0
    or.l    %d0, %d2        /* combine old low byte with new top 3 bytes */
    move.w  (%a2), %d4      /* load 3rd word */
    swap    %d4             /* move to upper 16 bit */
    move.w  (%a2), %d4      /* load 4th word */
    move.l  %d4, %d0
    lsl.l   %d1, %d3
    lsr.l   #8, %d0
    or.l    %d0, %d3        /* combine old low byte with new top 3 bytes */
    move.w  (%a2), %d5      /* load 5th word */
    swap    %d5             /* move to upper 16 bit */
    move.w  (%a2), %d5      /* load 6th word */
    move.l  %d5, %d0
    lsl.l   %d1, %d4
    lsr.l   #8, %d0
    or.l    %d0, %d4        /* combine old low byte with new top 3 bytes */
    move.w  (%a2), %d6      /* load 7th word */
    swap    %d6             /* move to upper 16 bit */
    move.w  (%a2), %d6      /* load 8th word */
    move.l  %d6, %d0
    lsl.l   %d1, %d5
    lsr.l   #8, %d0
    or.l    %d0, %d5        /* combine old low byte with new top 3 bytes */
    movem.l %d2-%d5, (%a0)  /* store line */
    lea.l   (16, %a0), %a0
    move.l  %d6, %d2
    cmp.l   %a0, %a1        /* run up to last line bound */
    bhi.b   .r_loop_u_line

    lea.l   (12, %a1), %a1  /* readjust for longword loop */
    cmp.l   %a0, %a1        /* any trailing longwords? */
    bls.b   .r_end_u_l2     /* no: skip loop */

.r_loop_u_l2:
    move.w  (%a2), %d3      /* load first word */
    swap    %d3             /* move to upper 16 bit */
    move.w  (%a2), %d3      /* load second word */
    move.l  %d3, %d4
    lsl.l   %d1, %d2
    lsr.l   #8, %d3
    or.l    %d3, %d2        /* combine old low byte with new top 3 bytes */
    move.l  %d2, (%a0)+     /* store as long */
    move.l  %d4, %d2
    cmp.l   %a0, %a1        /* run up to last long bound */
    bhi.b   .r_loop_u_l2
        
.r_end_u_l2:
    addq.l  #2, %a1         /* back to final end address */
    cmp.l   %a0, %a1        /* one word left? */
    bls.b   .r_end_u_w2

    swap    %d2             /* move old word to upper 16 bits */
    move.w  (%a2), %d2      /* load final word */
    move.l  %d2, %d3
    lsr.l   #8, %d3
    move.w  %d3, (%a0)+     /* write bytes 2 and 3 as word */

.r_end_u_w2:
    move.b  %d2, (%a0)+     /* store final byte */
    bra.b   .r_exit

    /* 16-bit aligned */
.r_aligned:
    btst.l  #1, %d0         /* longword aligned? */
    beq.b   .r_end_a_w1     /* yes, skip leading word handling */

    move.w  (%a2), (%a0)+   /* copy initial word */

.r_end_a_w1:
    moveq.l #12, %d0
    add.l   %a0, %d0
    and.l   #0xFFFFFFF0,%d0 /* d0 == first line bound */
    cmp.l   %a0, %d0        /* any leading longwords? */
    bls.b   .r_end_a_l1     /* no: skip loop */

.r_loop_a_l1:
    move.w  (%a2), %d1      /* load first word */
    swap    %d1             /* move it to upper 16 bits */
    move.w  (%a2), %d1      /* load second word */
    move.l  %d1, (%a0)+     /* store as long */
    cmp.l   %a0, %d0        /* run up to first line bound */
    bhi.b   .r_loop_a_l1

.r_end_a_l1:
    lea.l   (-14, %a1), %a1 /* adjust end addr. to 16 bytes/pass */

.r_loop_a_line:
    move.w  (%a2), %d0      /* load 1st word */
    swap    %d0             /* move it to upper 16 bits */
    move.w  (%a2), %d0      /* load 2nd word */
    move.w  (%a2), %d1      /* load 3rd word */
    swap    %d1             /* move it to upper 16 bits */
    move.w  (%a2), %d1      /* load 4th word */
    move.w  (%a2), %d2      /* load 5th word */
    swap    %d2             /* move it to upper 16 bits */
    move.w  (%a2), %d2      /* load 6th word */
    move.w  (%a2), %d3      /* load 7th word */
    swap    %d3             /* move it to upper 16 bits */
    move.w  (%a2), %d3      /* load 8th word */
    movem.l %d0-%d3, (%a0)  /* store line */
    lea.l   (16, %a0), %a0
    cmp.l   %a0, %a1        /* run up to last line bound */
    bhi.b   .r_loop_a_line

    lea.l   (12, %a1), %a1  /* readjust for longword loop */
    cmp.l   %a0, %a1        /* any trailing longwords? */
    bls.b   .r_end_a_l2     /* no: skip loop */

.r_loop_a_l2:
    move.w  (%a2), %d1      /* read first word */
    swap    %d1             /* move it to upper 16 bits */
    move.w  (%a2), %d1      /* read second word */
    move.l  %d1, (%a0)+     /* store as long */
    cmp.l   %a0, %a1        /* run up to last long bound */
    bhi.b   .r_loop_a_l2
        
.r_end_a_l2:
    addq.l  #2, %a1         /* back to final end address */
    cmp.l   %a0, %a1        /* one word left? */
    bls.b   .r_end_a_w2

    move.w  (%a2), (%a0)+   /* copy final word */

.r_end_a_w2:

.r_exit:
    movem.l (%sp), %d2-%d6/%a2
    lea.l   (24, %sp), %sp
    rts

.r_end:
    .size   copy_read_sectors,.r_end-copy_read_sectors

    .align  2
    .global copy_write_sectors
    .type   copy_write_sectors,@function
    
/* Write a number of words to the ATA data port
 *
 * Utilises line bursts, assumes there is at least one full line to copy.
 *
 * Arguments:
 *   (4,%sp) - buffer address
 *   (8,%sp) - word count
 *
 * Register usage:
 *   %a0 - current address
 *   %a1 - end address
 *   %a2 - ata port
 *   %d0 - scratch
 *   %d1 - shift count
 *   %d2-%d6 - read buffers
 */

copy_write_sectors:
    lea.l   (-24, %sp), %sp
    movem.l %d2-%d6/%a2, (%sp)
    movem.l (28, %sp), %a0-%a1
    add.l   %a1, %a1
    add.l   %a0, %a1
    lea.l   .ata_port, %a2

    move.l  %a0, %d0
    btst.l  #0, %d0         /* 16-bit aligned? */
    jeq     .w_aligned      /* yes, do word copy */

    /* not 16-bit aligned */
    subq.l  #1, %a1         /* last byte is done unconditionally */
    moveq.l #24, %d1        /* preload shift count */

    move.b  (%a0)+, %d2

    btst.l  #1, %d0         /* longword aligned? (testing old d0 value!) */
    bne.b   .w_end_u_w1     /* yes, skip leading word handling */

    swap    %d2
    move.w  (%a0)+, %d2
    move.l  %d2, %d3
    lsr.l   #8, %d3
    move.w  %d3, (%a2)
        
.w_end_u_w1:
    moveq.l #12, %d0
    add.l   %a0, %d0
    and.l   #0xFFFFFFF0,%d0 /* d0 == first line bound */
    cmp.l   %a0, %d0        /* any leading longwords? */
    bls.b   .w_end_u_l1     /* no: skip loop */
        
.w_loop_u_l1:
    move.l  (%a0)+, %d3
    move.l  %d3, %d4
    lsl.l   %d1, %d2
    lsr.l   #8, %d3
    or.l    %d3, %d2
    swap    %d2
    move.w  %d2, (%a2)
    swap    %d2
    move.w  %d2, (%a2)
    move.l  %d4, %d2
    cmp.l   %a0, %d0        /* run up to first line bound */
    bhi.b   .w_loop_u_l1

.w_end_u_l1:
    lea.l   (-14, %a1), %a1 /* adjust end addr. to 16 bytes/pass */

.w_loop_u_line:
    movem.l (%a0), %d3-%d6
    lea.l   (16, %a0), %a0
    move.l  %d3, %d0
    lsl.l   %d1, %d2
    lsr.l   #8, %d0
    or.l    %d0, %d2
    swap    %d2
    move.w  %d2, (%a2)
    swap    %d2
    move.w  %d2, (%a2)
    move.l  %d4, %d0
    lsl.l   %d1, %d3
    lsr.l   #8, %d0
    or.l    %d0, %d3
    swap    %d3
    move.w  %d3, (%a2)
    swap    %d3
    move.w  %d3, (%a2)
    move.l  %d5, %d0
    lsl.l   %d1, %d4
    lsr.l   #8, %d0
    or.l    %d0, %d4
    swap    %d4
    move.w  %d4, (%a2)
    swap    %d4
    move.w  %d4, (%a2)
    move.l  %d6, %d0
    lsl.l   %d1, %d5
    lsr.l   #8, %d0
    or.l    %d0, %d5
    swap    %d5
    move.w  %d5, (%a2)
    swap    %d5
    move.w  %d5, (%a2)
    move.l  %d6, %d2
    cmp.l   %a0, %a1        /* run up to last line bound */
    bhi.b   .w_loop_u_line

    lea.l   (12, %a1), %a1  /* readjust for longword loop */
    cmp.l   %a0, %a1        /* any trailing longwords? */
    bls.b   .w_end_u_l2     /* no: skip loop */

.w_loop_u_l2:
    move.l  (%a0)+, %d3
    move.l  %d3, %d4
    lsl.l   %d1, %d2
    lsr.l   #8, %d3
    or.l    %d3, %d2
    swap    %d2
    move.w  %d2, (%a2)
    swap    %d2
    move.w  %d2, (%a2)
    move.l  %d4, %d2
    cmp.l   %a0, %a1        /* run up to first line bound */
    bhi.b   .w_loop_u_l2

.w_end_u_l2:
    addq.l  #2, %a1         /* back to final end address */
    cmp.l   %a0, %a1        /* one word left? */
    bls.b   .w_end_u_w2

    swap    %d2
    move.w  (%a0)+, %d2
    move.l  %d2, %d3
    lsr.l   #8, %d3
    move.w  %d3, (%a2)

.w_end_u_w2:
    lsl.l   #8, %d2
    move.b  (%a0)+, %d2
    move.w  %d2, (%a2)
    bra.b    .w_exit

    /* 16-bit aligned */
.w_aligned:
    btst.l  #1, %d0
    beq.b   .w_end_a_w1

    move.w  (%a0)+, (%a2)   /* copy initial word */

.w_end_a_w1:
    moveq.l #12, %d0
    add.l   %a0, %d0
    and.l   #0xFFFFFFF0,%d0 /* d0 == first line bound */
    cmp.l   %a0, %d0        /* any leading longwords? */
    bls.b   .w_end_a_l1     /* no: skip loop */

.w_loop_a_l1:
    move.l  (%a0)+, %d1
    swap    %d1
    move.w  %d1, (%a2)
    swap    %d1
    move.w  %d1, (%a2)
    cmp.l   %a0, %d0        /* run up to first line bound */
    bhi.b   .w_loop_a_l1

.w_end_a_l1:
    lea.l   (-14, %a1), %a1 /* adjust end addr. to 16 bytes/pass */

.w_loop_a_line:
    movem.l (%a0), %d0-%d3
    lea.l   (16, %a0), %a0
    swap    %d0
    move.w  %d0, (%a2)
    swap    %d0
    move.w  %d0, (%a2)
    swap    %d1
    move.w  %d1, (%a2)
    swap    %d1
    move.w  %d1, (%a2)
    swap    %d2
    move.w  %d2, (%a2)
    swap    %d2
    move.w  %d2, (%a2)
    swap    %d3
    move.w  %d3, (%a2)
    swap    %d3
    move.w  %d3, (%a2)
    cmp.l   %a0, %a1        /* run up to last line bound */
    bhi.b   .w_loop_a_line
        
    lea.l   (12, %a1), %a1  /* readjust for longword loop */
    cmp.l   %a0, %a1        /* any trailing longwords? */
    bls.b   .w_end_a_l2     /* no: skip loop */

.w_loop_a_l2:
    move.l  (%a0)+, %d1
    swap    %d1
    move.w  %d1, (%a2)
    swap    %d1
    move.w  %d1, (%a2)
    cmp.l   %a0, %a1        /* run up to first line bound */
    bhi.b   .w_loop_a_l2

.w_end_a_l2:
    addq.l  #2, %a1         /* back to final end address */
    cmp.l   %a0, %a1        /* one word left? */
    bls.b   .w_end_a_w2

    move.w  (%a0)+, (%a2)   /* copy final word */

.w_end_a_w2:

.w_exit:
    movem.l (%sp), %d2-%d6/%a2
    lea.l   (24, %sp), %sp
    rts

.w_end:
    .size   copy_write_sectors,.w_end-copy_write_sectors
