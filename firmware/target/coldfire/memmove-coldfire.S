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
#include "config.h"

    .section    .icode,"ax",@progbits

#define FULLSPEED /* use burst writing for word aligned destinations */
    .align  2
    .global memmove
    .type   memmove,@function

/* Moves <length> bytes of data in memory from <source> to <dest>
 * Regions may overlap.
 * This version is optimized for speed, and needs the corresponding memcpy
 * implementation for the forward copy branch.
 *
 * arguments:
 *  (4,%sp)  - destination address
 *  (8,%sp)  - source address
 *  (12,%sp) - length
 *
 * return value:
 *  %d0 - destination address (like ANSI version)
 *
 * register usage:
 *  %a0 - current source address
 *  %a1 - current dest address
 *  %a2 - source start address (in line-copy loops)
 *  %d0 - source start address (byte and longword copy) / data / scratch
 *  %d1 - data / scratch
 *  %d2 - data / scratch
 *  %d3..%d7 - data
 *
 * For maximum speed this routine reads and writes whole lines using burst
 * move (movem.l) where possible. For byte aligned destinations (long-1 and
 * long-3) it writes longwords only. Same goes for word aligned destinations
 * if FULLSPEED is undefined.
 */
memmove:
    move.l  (4,%sp),%a1     /* Destination */
    move.l  (8,%sp),%a0     /* Source */
    move.l  (12,%sp),%d1    /* Length */

    cmp.l   %a0,%a1
    bhi.b   .backward       /* dest > src -> backward copy */
    jmp     __memcpy_fwd_entry

.backward:
    move.l  %a0,%d0         /* %d0 = source start */
    add.l   %d1,%a0         /* %a0 = source end */
    add.l   %d1,%a1         /* %a1 = destination end */

    move.l  %a0,%d1
    and.l   #0xFFFFFFFC,%d1 /* %d1 = last source long bound */
    subq.l  #4,%d1
    cmp.l   %d0,%d1         /* at least one aligned longword to copy? */
    blo.w   .bytes2r_start

    addq.l  #4,%d1          /* %d1 = last source long bound */
    cmp.l   %d1,%a0         /* any bytes to copy */
    jls     .bytes1r_end    /* no: skip byte loop */
    
    /* leading byte loop: copies 0..3 bytes */
.bytes1r_loop:
    move.b  -(%a0),-(%a1)   /* copy byte */
    cmp.l   %d1,%a0         /* runs %a0 down to last long bound */
    jhi     .bytes1r_loop

.bytes1r_end:
    moveq.l #-16,%d1
    add.l   %a0,%d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = last source line bound - 16 */
    cmp.l   %d0,%d1         /* at least one aligned line to copy? */
    blo.w   .longr_start    /* no: jump to longword copy loop */
    
    lea.l   (-28,%sp),%sp   /* free up some registers */
    movem.l %d2-%d7/%a2,(%sp)

    moveq.l #16,%d2
    add.l   %d2,%d1         /* %d1 = last source line bound */
    move.l  %d0,%a2         /* %a2 = start address */
    lea.l   (15,%a2),%a2    /* adjust start address for loops doing 16 bytes/pass */
    move.l  %a1,%d0
    moveq.l #3,%d2          /* mask */
    and.l   %d2,%d0
    jmp.l   (2,%pc,%d0.l*4) /* switch (dest_addr & 3) */
    bra.w   .lines_do0r_start
    bra.w   .lines_do1r_start
    bra.w   .lines_do2r_start
 /* bra.w   .lines_do3r_start   implicit */

    /* byte aligned destination (long - 1): use line burst reads in main loop */
.lines_do3r_start:
    moveq.l #24,%d0         /* shift count for shifting by 3 bytes */
    cmp.l   %d1,%a0         /* any leading longwords? */
    jhi     .lines_do3r_head_start  /* yes: leading longword copy */
    
    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d3-%d6   /* load initial line */
    move.l  %d6,%d2         /* last longword, bytes 3210 */
    move.b  %d2,-(%a1)      /* store byte */
    lsr.l   #8,%d2          /* last longword, bytes .321 */
    move.w  %d2,-(%a1)      /* store word */
    jra     .lines_do3r_entry
    
.lines_do3r_head_start:
    move.l  -(%a0),%d3      /* load initial longword */
    move.l  %d3,%d2         /* bytes 3210 */
    move.b  %d2,-(%a1)      /* store byte */
    lsr.l   #8,%d2          /* bytes .321 */
    move.w  %d2,-(%a1)      /* store word */
    jra     .lines_do3r_head_entry
    
.lines_do3r_head_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.l  %d3,%d2     
    lsl.l   #8,%d2          /* get bytes 210. */
    or.l    %d2,%d4         /* combine with old high byte */
    move.l  %d4,-(%a1)      /* store longword */
.lines_do3r_head_entry:
    lsr.l   %d0,%d3         /* shift down high byte */
    cmp.l   %d1,%a0         /* run %a0 down to last line bound */
    jhi     .lines_do3r_head_loop

.lines_do3r_loop:
    move.l  %d3,%d7         /* move first longword of last line away */
    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d3-%d6   /* load new line */
    move.l  %d6,%d2
    lsl.l   #8,%d2          /* get bytes 210. of 4th longword */
    or.l    %d2,%d7         /* combine with high byte of old longword */
    move.l  %d7,-(%a1)      /* store longword */
.lines_do3r_entry:
    lsr.l   %d0,%d6         /* shift down high byte */
    move.l  %d5,%d2
    lsl.l   #8,%d2          /* get bytes 210. of 3rd longword */
    or.l    %d2,%d6         /* combine with high byte of 4th longword */
    move.l  %d6,-(%a1)      /* store longword */
    lsr.l   %d0,%d5         /* shift down high byte */
    move.l  %d4,%d2
    lsl.l   #8,%d2          /* get bytes 210. of 2nd longword */
    or.l    %d2,%d5         /* combine with high byte or 3rd longword */
    move.l  %d5,-(%a1)      /* store longword */
    lsr.l   %d0,%d4         /* shift down high byte */
    move.l  %d3,%d2
    lsl.l   #8,%d2          /* get bytes 210. of 1st longword */
    or.l    %d2,%d4         /* combine with high byte of 2nd longword */
    move.l  %d4,-(%a1)      /* store longword */
    lsr.l   %d0,%d3         /* shift down high byte */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_do3r_loop

    lea.l   (-12,%a2),%a2   /* readjust start address for doing longwords */
    cmp.l   %a2,%a0         /* any trailing longwords? */
    jls     .lines_do3r_tail_end    /* no: just store last high byte */

.lines_do3r_tail_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.l  %d3,%d2   
    lsl.l   #8,%d2          /* get bytes 210. */
    or.l    %d2,%d4         /* combine with old high byte */
    move.l  %d4,-(%a1)      /* store longword */
    lsr.l   %d0,%d3         /* shift down high byte */
    cmp.l   %a2,%a0         /* run %a0 down to first long bound */
    jhi     .lines_do3r_tail_loop
    
.lines_do3r_tail_end:
    move.b  %d3,-(%a1)      /* store shifted-down high byte */
    jra     .linesr_end
    
    /* byte aligned destination (long - 3): use line burst reads in main loop */
.lines_do1r_start:
    moveq.l #24,%d0         /* shift count for shifting by 3 bytes */
    cmp.l   %d1,%a0         /* any leading longwords? */
    jhi     .lines_do1r_head_start  /* yes: leading longword copy */

    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d3-%d6   /* load initial line */
    move.b  %d6,-(%a1)      /* store low byte of last longword */
    jra     .lines_do1r_entry
    
.lines_do1r_head_start:
    move.l  -(%a0),%d3      /* load initial longword */
    move.b  %d3,-(%a1)      /* store low byte */
    jra     .lines_do1r_head_entry

.lines_do1r_head_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.l  %d3,%d2
    lsl.l   %d0,%d2         /* get low byte */
    or.l    %d2,%d4         /* combine with old bytes .321 */
    move.l  %d4,-(%a1)      /* store longword */
.lines_do1r_head_entry:
    lsr.l   #8,%d3          /* get bytes .321 */
    cmp.l   %d1,%a0         /* run %a0 down to last line bound */
    jhi     .lines_do1r_head_loop

.lines_do1r_loop:
    move.l  %d3,%d7         /* move first longword of old line away */
    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d3-%d6   /* load new line */
    move.l  %d6,%d2
    lsl.l   %d0,%d2         /* get low byte of 4th longword */
    or.l    %d2,%d7         /* combine with bytes .321 of old longword */
    move.l  %d7,-(%a1)      /* store longword */
.lines_do1r_entry:
    lsr.l   #8,%d6          /* get bytes .321 */
    move.l  %d5,%d2
    lsl.l   %d0,%d2         /* get low byte of 3rd longword */
    or.l    %d2,%d6         /* combine with bytes .321 of 4th longword */
    move.l  %d6,-(%a1)      /* store longword */
    lsr.l   #8,%d5          /* get bytes .321 */
    move.l  %d4,%d2
    lsl.l   %d0,%d2         /* get low byte of 2nd longword */
    or.l    %d2,%d5         /* combine with bytes .321 of 3rd longword */
    move.l  %d5,-(%a1)      /* store longword */
    lsr.l   #8,%d4          /* get bytes .321 */
    move.l  %d3,%d2
    lsl.l   %d0,%d2         /* get low byte of 1st longword */
    or.l    %d2,%d4         /* combine with bytes .321 of 2nd longword */
    move.l  %d4,-(%a1)      /* store longword */
    lsr.l   #8,%d3          /* get bytes .321 */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_do1r_loop

    lea.l   (-12,%a2),%a2   /* readjust start address for doing longwords */
    cmp.l   %a2,%a0         /* any trailing longwords? */
    jls     .lines_do1r_tail_end    /* no: just store last high byte */

.lines_do1r_tail_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.l  %d3,%d2
    lsl.l   %d0,%d2         /* get low byte */
    or.l    %d2,%d4         /* combine with old bytes .321 */
    move.l  %d4,-(%a1)      /* store longword */
    lsr.l   #8,%d3          /* get bytes .321 */
    cmp.l   %a2,%a0         /* run %a0 down to first long bound */
    jhi     .lines_do1r_tail_loop
    
.lines_do1r_tail_end:
    move.w  %d3,-(%a1)      /* store word 21 */
    swap    %d3
    move.b  %d3,-(%a1)      /* store byte 3 */
    jra     .linesr_end

    /* long aligned destination (line - 0/4/8/12): head */
.lines_do0r_head_loop:
    move.l  -(%a0),-(%a1)   /* copy longword */
.lines_do0r_start:
    cmp.l   %d1,%a0         /* run %a0 down to last line bound */
    jhi     .lines_do0r_head_loop

.lines_do0r_head_end:
    move.l  %a1,%d1
    lsr.l   #2,%d1
    moveq.l #3,%d0          /* mask */
    and.l   %d0,%d1
    moveq.l #16,%d0         /* address decrement for one main loop pass */
    jmp.l   (2,%pc,%d1.l*2) /* switch ((dest_addr >> 2) & 3) */
    bra.b   .lines_lo0r_start
    bra.b   .lines_lo4r_start
    bra.b   .lines_lo8r_start
 /* bra.b   .lines_lo12r_start   implicit */

    /* long aligned destination (line - 4): use line bursts in the loop */
.lines_lo12r_start:
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load initial line */
    move.l  %d4,-(%a1)      /* store 4th longword */
    move.l  %d3,-(%a1)      /* store 3rd longword */
    move.l  %d2,-(%a1)      /* store 2nd longword */
    cmp.l   %a2,%a0         /* any full lines? */
    jls     .lines_lo12r_end   /* no: skip main loop */
    
.lines_lo12r_loop:
    move.l  %d1,%d5         /* move first longword of old line away */
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load new line */
    sub.l   %d0,%a1
    movem.l %d2-%d5,(%a1)   /* store line (1 old + 3 new longwords */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo12r_loop

    jra     .lines_lo12r_end   /* handle trailing longwords */

    /* line aligned destination: use line bursts in the loop */
.lines_lo0r_start:
.lines_lo0r_loop:
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load line */
    sub.l   %d0,%a1
    movem.l %d1-%d4,(%a1)   /* store line */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo0r_loop

    jra     .lines_lo0r_end /* handle trailing longwords */

    /* long aligned destination (line - 8): use line bursts in the loop */
.lines_lo8r_start:
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load initial line */
    move.l  %d4,-(%a1)      /* store 4th longword */
    move.l  %d3,-(%a1)      /* store 3rd longword */
    cmp.l   %a2,%a0         /* any full lines? */
    jls     .lines_lo8r_end /* no: skip main loop */
    
.lines_lo8r_loop:
    move.l  %d2,%d6         /* move first 2 longwords of old line away */
    move.l  %d1,%d5         
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load new line */
    sub.l   %d0,%a1
    movem.l %d3-%d6,(%a1)   /* store line (2 old + 2 new longwords */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo8r_loop

    jra     .lines_lo8r_end /* handle trailing longwords */

    /* long aligned destination (line - 12): use line bursts in the loop */
.lines_lo4r_start:
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load initial line */
    move.l  %d4,-(%a1)      /* store 4th longword */
    cmp.l   %a2,%a0         /* any full lines? */
    jls     .lines_lo4r_end /* no: skip main loop */
    
.lines_lo4r_loop:
    move.l  %d3,%d7         /* move first 3 longwords of old line away */
    move.l  %d2,%d6         
    move.l  %d1,%d5         
    sub.l   %d0,%a0
    movem.l (%a0),%d1-%d4   /* load new line */
    sub.l   %d0,%a1
    movem.l %d4-%d7,(%a1)   /* store line (3 old + 1 new longwords */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo4r_loop

    /* long aligned destination (line - 0/4/8/12): tail */
.lines_lo4r_end:
    move.l  %d3,-(%a1)      /* store 3rd last longword */
.lines_lo8r_end:
    move.l  %d2,-(%a1)      /* store 2nd last longword */
.lines_lo12r_end:
    move.l  %d1,-(%a1)      /* store last longword */
.lines_lo0r_end:
    lea.l   (-12,%a2),%a2   /* readjust end address for doing longwords */
    cmp.l   %a2,%a0         /* any trailing longwords? */
    jls     .linesr_end     /* no: get outta here */

.lines_do0r_tail_loop:
    move.l  -(%a0),-(%a1)   /* copy longword */
    cmp.l   %a2,%a0         /* run %a0 down to first long bound */
    jhi     .lines_do0r_tail_loop

    jra     .linesr_end
    
#ifdef FULLSPEED
    /* word aligned destination (line - 2/6/10/14): head */
.lines_do2r_start:
    cmp.l   %d1,%a0         /* any leading longwords? */
    jls     .lines_do2r_selector    /* no: jump to mainloop selector */
    
    move.l  -(%a0),%d3      /* load initial longword */
    move.w  %d3,-(%a1)      /* store low word */
    cmp.l   %d1,%a0         /* any more longwords? */
    jls     .lines_do2r_head_end    /* no: skip head loop */
    
.lines_do2r_head_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.w  %d3,%d4         /* combine low word with old high word */
    swap    %d4             /* swap words */
    move.l  %d4,-(%a1)      /* store longword */
    cmp.l   %d1,%a0         /* run %a0 down to last line bound */
    jhi     .lines_do2r_head_loop
    
.lines_do2r_head_end:
    swap    %d3             /* get high word */
    move.w  %d3,-(%a1)      /* and store it */
    
.lines_do2r_selector:
    move.l  %a1,%d1
    lsr.l   #2,%d1
    moveq.l #3,%d0          /* mask */
    and.l   %d0,%d1
    moveq.l #16,%d7         /* address decrement for one main loop pass */
    jmp.l   (2,%pc,%d1.l*4) /* switch ((dest_addr >> 2) & 3) */
    bra.w   .lines_lo2r_start
    bra.w   .lines_lo6r_start
    bra.w   .lines_lo10r_start
 /* bra.w   .lines_lo14r_start   implicit */

    /* word aligned destination (line - 2): use line bursts in the loop */
.lines_lo14r_start:
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load initial line */
    move.w  %d3,-(%a1)      /* store last low word */
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    move.w  %d1,%d2         /* combine 2nd low word with 3rd high word */
    swap    %d2             /* swap words of 2nd long */
    move.w  %d0,%d1         /* combine 1st low word with 2nd high word */
    swap    %d1             /* swap words of 1st long */
    move.l  %d3,-(%a1)      /* store 3rd longword */
    move.l  %d2,-(%a1)      /* store 2nd longword */
    move.l  %d1,-(%a1)      /* store 1st longword */
    cmp.l   %a2,%a0         /* any full lines? */
    jls     .lines_lo14r_end   /* no: skip main loop */

.lines_lo14r_loop:
    move.l  %d0,%d4         /* move first longword of old line away */
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load line */
    move.w  %d3,%d4         /* combine 4th low word with old high word */
    swap    %d4             /* swap words of 4th long */
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    move.w  %d1,%d2         /* combine 2nd low word with 3rd high word */
    swap    %d2             /* swap words of 2nd long */
    move.w  %d0,%d1         /* combine 1st low word with 2nd high word */
    swap    %d1             /* swap words of 1st long */
    sub.l   %d7,%a1
    movem.l %d1-%d4,(%a1)   /* store line */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo14r_loop
    
    jra     .lines_lo14r_end   /* handle trailing longwords */

    /* word aligned destination (line - 6): use line bursts in the loop */
.lines_lo10r_start:
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load initial line */
    move.w  %d3,-(%a1)      /* store last low word */
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    move.w  %d1,%d2         /* combine 2nd low word with 3rd high word */
    swap    %d2             /* swap words of 2nd long */
    move.l  %d3,-(%a1)      /* store 3rd longword */
    move.l  %d2,-(%a1)      /* store 2nd longword */
    jra     .lines_lo10r_entry /* jump into main loop */
    
.lines_lo10r_loop:
    move.l  %d0,%d4         /* move first 2 longwords of old line away */
    move.l  %d1,%d5
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load line */
    move.w  %d3,%d4         /* combine 4th low word with old high word */
    swap    %d4             /* swap words of 4th long */
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    move.w  %d1,%d2         /* combine 2nd low word with 3rd high word */
    swap    %d2             /* swap words of 2nd long */
    sub.l   %d7,%a1
    movem.l %d2-%d5,(%a1)   /* store line */
.lines_lo10r_entry:
    move.w  %d0,%d1         /* combine 1st low word with 2nd high word */
    swap    %d1             /* swap words of 1st long */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo10r_loop
    
    jra     .lines_lo10r_end   /* handle trailing longwords */

    /* word aligned destination (line - 10): use line bursts in the loop */
.lines_lo6r_start:
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load initial line */
    move.w  %d3,-(%a1)      /* store last low word */
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    move.l  %d3,-(%a1)      /* store 3rd longword */
    jra     .lines_lo6r_entry  /* jump into main loop */
    
.lines_lo6r_loop:
    move.l  %d0,%d4         /* move first 3 longwords of old line away */
    move.l  %d1,%d5
    move.l  %d2,%d6
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load line */
    move.w  %d3,%d4         /* combine 4th low word with old high word */
    swap    %d4             /* swap words of 4th long */
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    sub.l   %d7,%a1
    movem.l %d3-%d6,(%a1)   /* store line */
.lines_lo6r_entry:
    move.w  %d1,%d2         /* combine 2nd low word with 3rd high word */
    swap    %d2             /* swap words of 2nd long */
    move.w  %d0,%d1         /* combine 1st low word with 2nd high word */
    swap    %d1             /* swap words of 1st long */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo6r_loop
    
    jra     .lines_lo6r_end /* handle trailing longwords */

    /* word aligned destination (line - 14): use line bursts in the loop */
.lines_lo2r_start:
    sub.l   %d7,%a0
    movem.l (%a0),%d0-%d3   /* load initial line */
    move.w  %d3,-(%a1)      /* store last low word */
    jra     .lines_lo2r_entry  /* jump into main loop */
    
.lines_lo2r_loop:
    move.l  %d0,%d4         /* move old line away */
    move.l  %d1,%d5
    move.l  %d2,%d6
    move.l  %d3,%d7
    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d0-%d3   /* load line */
    move.w  %d3,%d4         /* combine 4th low word with old high word */
    swap    %d4             /* swap words of 4th long */
    lea.l   (-16,%a1),%a1
    movem.l %d4-%d7,(%a1)   /* store line */
.lines_lo2r_entry:
    move.w  %d2,%d3         /* combine 3rd low word with 4th high word */
    swap    %d3             /* swap words of 3rd long */
    move.w  %d1,%d2         /* combine 2nd low word with 3rd high word */
    swap    %d2             /* swap words of 2nd long */
    move.w  %d0,%d1         /* combine 1st low word with 2nd high word */
    swap    %d1             /* swap words of 1st long */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_lo2r_loop
    
    /* word aligned destination (line - 2/6/10/14): tail */
.lines_lo2r_end:
    move.l  %d3,-(%a1)      /* store third last longword */
.lines_lo6r_end:
    move.l  %d2,-(%a1)      /* store second last longword */
.lines_lo10r_end:
    move.l  %d1,-(%a1)      /* store last longword */
.lines_lo14r_end:
    lea.l   (-12,%a2),%a2   /* readjust start address for doing longwords */
    cmp.l   %a2,%a0         /* any trailing longwords? */
    jls     .lines_do2r_tail_end    /* no: skip tail loop */

.lines_do2r_tail_loop:
    move.l  %d0,%d1         /* move old longword away */
    move.l  -(%a0),%d0      /* load new longword */
    move.w  %d0,%d1         /* combine low word with old high word */
    swap    %d1             /* swap words */
    move.l  %d1,-(%a1)      /* store longword */
    cmp.l   %a2,%a0         /* run %a0 down to first long bound */
    jhi     .lines_do2r_tail_loop
    
.lines_do2r_tail_end:
    swap    %d0             /* get final high word */
    move.w  %d0,-(%a1)      /* store it */
 /* jra     .linesr_end    implicit */

#else /* !FULLSPEED */

    /* word aligned destination (long - 2): use line burst reads in the loop */
.lines_do2r_start:
    cmp.l   %d1,%a0         /* any leading longwords? */
    jhi     .lines_do2r_head_start  /* yes: leading longword copy */

    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d3-%d6   /* load initial line */
    move.w  %d6,-(%a1)      /* store last low word */
    jra     .lines_do2r_entry  /* jump into main loop */
    
.lines_do2r_head_start:
    move.l  -(%a0),%d3      /* load initial longword */
    move.w  %d3,-(%a1)      /* store low word */
    cmp.l   %d1,%a0         /* any full longword? */
    jls     .lines_do2r_loop   /* no: skip head loop */
    
.lines_do2r_head_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.w  %d3,%d4         /* combine low word with old high word */
    swap    %d4             /* swap words */
    move.l  %d4,-(%a1)      /* store longword */
    cmp.l   %d1,%a0         /* run %a0 down to last line bound */
    jhi     .lines_do2r_head_loop
    
.lines_do2r_loop:
    move.l  %d3,%d7         /* move first longword of old line away */
    lea.l   (-16,%a0),%a0
    movem.l (%a0),%d3-%d6   /* load line */
    move.w  %d6,%d7         /* combine 4th low word with old high word */
    swap    %d7             /* swap words of 4th long */
    move.l  %d7,-(%a1)      /* store 4th longword */
.lines_do2r_entry:
    move.w  %d5,%d6         /* combine 3rd low word with 4th high word */
    swap    %d6             /* swap words of 3rd long */
    move.l  %d6,-(%a1)      /* store 3rd longword */
    move.w  %d4,%d5         /* combine 2nd low word with 3rd high word */
    swap    %d5             /* swap words of 2nd long */
    move.l  %d5,-(%a1)      /* store 2nd longword */
    move.w  %d3,%d4         /* combine 1st low word with 2nd high word */
    swap    %d4             /* swap words of 1st long */
    move.l  %d4,-(%a1)      /* store 1st longword */
    cmp.l   %a2,%a0         /* run %a0 down to first line bound */
    jhi     .lines_do2r_loop

.lines_do2r_end:
    lea.l   (-12,%a2),%a2   /* readjust start address for doing longwords */
    cmp.l   %a2,%a0         /* any trailing longwords? */
    jls     .lines_do2r_tail_end   /* no: skip tail loop */
    
.lines_do2r_tail_loop:
    move.l  %d3,%d4         /* move old longword away */
    move.l  -(%a0),%d3      /* load new longword */
    move.w  %d3,%d4         /* combine low word with old high word */
    swap    %d4             /* swap words */
    move.l  %d4,-(%a1)      /* store longword */
    cmp.l   %a2,%a0         /* run %a0 down to first long bound */
    jhi     .lines_do2r_tail_loop

.lines_do2r_tail_end:
    swap    %d3             /* get final high word */
    move.w  %d3,-(%a1)      /* store it */
 /* jra     .linesr_end    implicit */

#endif /* !FULLSPEED */

.linesr_end:
    subq.l  #3,%a2          /* readjust end address */
    move.l  %a2,%d0         /* start address in %d0 again */
    movem.l (%sp),%d2-%d7/%a2      /* restore registers */
    lea.l   (28,%sp),%sp
    jra     .bytes2r_start  /* jump to trailing byte loop */

.longr_start:
    addq.l  #3,%d0          /* adjust start address for doing 4 bytes/ pass */

    /* longword copy loop - no lines */
.longr_loop:
    move.l  -(%a0),-(%a1)   /* copy longword (write can be unaligned) */
    cmp.l   %d0,%a0         /* runs %a0 down to first long bound */
    jhi     .longr_loop

    subq.l  #3,%d0          /* readjust start address */
    cmp.l   %d0,%a0         /* any bytes left? */
    jls     .bytes2r_end    /* no: skip trailing byte loop */

    /* trailing byte loop */
.bytes2r_loop:
    move.b  -(%a0),-(%a1)   /* copy byte */
.bytes2r_start:
    cmp.l   %d0,%a0         /* runs %a0 down to start address */
    jhi     .bytes2r_loop

.bytes2r_end:
    rts                     /* returns start address */

.end:
    .size   memmove,.end-memmove
