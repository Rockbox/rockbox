/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004-2005 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

    .section    .icode,"ax",@progbits

#if CONFIG_CPU == SH7034
    .align      2
    .global     _memcpy
    .global     ___memcpy_fwd_entry
    .type       _memcpy,@function

/* Copies <length> bytes of data in memory from <source> to <dest>
 * This version is optimized for speed
 *
 * arguments:
 *  r4 - destination address
 *  r5 - source address
 *  r6 - length
 *
 * return value:
 *  r0 - destination address (like ANSI version)
 *
 * register usage:
 *  r0 - data / scratch
 *  r1 - 2nd data / scratch
 *  r2 - scratch
 *  r3 - first long bound / adjusted end address (only if >= 11 bytes)
 *  r4 - current dest address
 *  r5 - current source address
 *  r6 - source end address
 *  r7 - stored dest start address
 *
 * The instruction order is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine also tries to utilize fast page mode.
 */

_memcpy:
    mov     r4,r7       /* store dest for returning */
___memcpy_fwd_entry:
    add     #-8,r4      /* offset for early increment (max. 2 longs) */
    mov     #11,r0
    cmp/hs  r0,r6       /* at least 11 bytes to copy? (ensures 2 aligned longs) */
    add     r5,r6       /* r6 = source_end */
    bf      .start_b2   /* no: jump directly to byte loop */
    
    mov     #3,r0
    neg     r5,r3
    and     r0,r3       /* r3 = (4 - align_offset) % 4 */
    tst     r3,r3       /* already aligned? */
    bt      .end_b1     /* yes: skip leading byte loop */
    
    add     r5,r3       /* r3 = first source long bound */

    /* leading byte loop: copies 0..3 bytes */
.loop_b1:
    mov.b   @r5+,r0     /* load byte & increment source addr */
    add     #1,r4       /* increment dest addr */
    mov.b   r0,@(7,r4)  /* store byte */
    cmp/hi  r5,r3       /* runs r5 up to first long bound */
    bt      .loop_b1
    /* now r5 is always at a long boundary */
    /* -> memory reading is done in longs for all dest alignments */

    /* selector for main copy loop */
.end_b1:
    mov     #3,r1
    and     r4,r1       /* r1 = dest alignment offset */
    mova    .jmptab,r0
    mov.b   @(r0,r1),r1 /* select appropriate main loop */
    add     r0,r1
    mov     r6,r3       /* move end address to r3 */
    jmp     @r1         /* and jump to it */
    add     #-7,r3      /* adjust end addr for main loops doing 2 longs/pass */
    
    /** main loops, copying 2 longs per pass to profit from fast page mode **/

    /* long aligned destination (fastest) */
    .align  2
.loop_do0:
    mov.l   @r5+,r1     /* load first long & increment source addr */
    add     #16,r4      /* increment dest addr & account for decrementing stores */
    mov.l   @r5+,r0     /* load second long & increment source addr */
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
    mov.l   r0,@-r4     /* store second long */
    mov.l   r1,@-r4     /* store first long; NOT ALIGNED - no speed loss here! */
    bt      .loop_do0
           
    add     #4,r3       /* readjust end address */
    cmp/hi  r5,r3       /* one long left? */
    bf      .start_b2   /* no, jump to trailing byte loop */
    
    mov.l   @r5+,r0     /* load last long & increment source addr */
    add     #4,r4       /* increment dest addr */
    bra     .start_b2   /* jump to trailing byte loop */
    mov.l   r0,@(4,r4)  /* store last long */

    /* word aligned destination (long + 2) */
    .align  2
.loop_do2:
    mov.l   @r5+,r1     /* load first long & increment source addr */
    add     #16,r4      /* increment dest addr */
    mov.l   @r5+,r0     /* load second long & increment source addr */
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
    mov.w   r0,@-r4     /* store low word of second long */
    xtrct   r1,r0       /* extract low word of first long & high word of second long */
    mov.l   r0,@-r4     /* and store as long */
    swap.w  r1,r0       /* get high word of first long */
    mov.w   r0,@-r4     /* and store it */
    bt      .loop_do2

    add     #4,r3       /* readjust end address */
    cmp/hi  r5,r3       /* one long left? */
    bf      .start_b2   /* no, jump to trailing byte loop */

    mov.l   @r5+,r0     /* load last long & increment source addr */
    add     #4,r4       /* increment dest addr */
    mov.w   r0,@(6,r4)  /* store low word */
    shlr16  r0          /* get high word */
    bra     .start_b2   /* jump to trailing byte loop */
    mov.w   r0,@(4,r4)  /* and store it */

    /* jumptable for loop selector */
    .align  2
.jmptab:
    .byte   .loop_do0 - .jmptab  /* placed in the middle because the SH1 */
    .byte   .loop_do1 - .jmptab  /* loads bytes sign-extended. Otherwise */
    .byte   .loop_do2 - .jmptab  /* the last loop would be out of reach */
    .byte   .loop_do3 - .jmptab  /* of the offset range. */

    /* byte aligned destination (long + 1) */
    .align  2
.loop_do1:
    mov.l   @r5+,r1     /* load first long & increment source addr */
    add     #16,r4      /* increment dest addr */
    mov.l   @r5+,r0     /* load second long & increment source addr */
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
    mov.b   r0,@-r4     /* store low byte of second long */
    shlr8   r0          /* get upper 3 bytes */
    mov     r1,r2       /* copy first long */
    shll16  r2          /* move low byte of first long all the way up, .. */
    shll8   r2
    or      r2,r0       /* ..combine with the 3 bytes of second long.. */
    mov.l   r0,@-r4     /* ..and store as long */
    shlr8   r1          /* get middle 2 bytes */
    mov.w   r1,@-r4     /* store as word */
    shlr16  r1          /* get upper byte */
    mov.b   r1,@-r4     /* and store */
    bt      .loop_do1

    add     #4,r3       /* readjust end address */
.last_do13:
    cmp/hi  r5,r3       /* one long left? */
    bf      .start_b2   /* no, jump to trailing byte loop */

    mov.l   @r5+,r0     /* load last long & increment source addr */
    add     #12,r4      /* increment dest addr */
    mov.b   r0,@-r4     /* store low byte */
    shlr8   r0          /* get middle 2 bytes */
    mov.w   r0,@-r4     /* store as word */
    shlr16  r0          /* get upper byte */
    mov.b   r0,@-r4     /* and store */
    bra     .start_b2   /* jump to trailing byte loop */
    add     #-4,r4      /* readjust destination */

    /* byte aligned destination (long + 3) */
    .align  2
.loop_do3:
    mov.l   @r5+,r1     /* load first long & increment source addr */
    add     #16,r4      /* increment dest addr */
    mov.l   @r5+,r0     /* load second long & increment source addr */
    mov     r1,r2       /* copy first long */
    mov.b   r0,@-r4     /* store low byte of second long */
    shlr8   r0          /* get middle 2 bytes */
    mov.w   r0,@-r4     /* store as word */
    shlr16  r0          /* get upper byte */
    shll8   r2          /* move lower 3 bytes of first long one up.. */
    or      r2,r0       /* ..combine with the 1 byte of second long.. */
    mov.l   r0,@-r4     /* ..and store as long */
    shlr16  r1          /* get upper byte of first long.. */
    shlr8   r1
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
    mov.b   r1,@-r4     /* ..and store */
    bt      .loop_do3

    bra     .last_do13  /* handle last longword: reuse routine for (long + 1) */
    add     #4,r3       /* readjust end address */

    /* trailing byte loop: copies 0..3 bytes (or all for < 11 in total) */
    .align  2
.loop_b2:
    mov.b   @r5+,r0     /* load byte & increment source addr */
    add     #1,r4       /* increment dest addr */
    mov.b   r0,@(7,r4)  /* store byte */
.start_b2:
    cmp/hi  r5,r6       /* runs r5 up to end address */
    bt      .loop_b2

    rts
    mov     r7,r0       /* return dest start address */
.end:
    .size   _memcpy,.end-_memcpy
#elif defined(CPU_COLDFIRE)
#define FULLSPEED /* use burst writing for word aligned destinations */
    .align  2
    .global memcpy
    .global __memcpy_fwd_entry
    .type   memcpy,@function

/* Copies <length> bytes of data in memory from <source> to <dest>
 * This version is optimized for speed
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
 *  %a2 - source end address (in line-copy loops)
 *  %d0 - data / scratch
 *  %d1 - source end address (byte and longword copy) / data / scratch
 *  %d2 - data / scratch
 *  %d3..%d7 - data
 *
 * For maximum speed this routine reads and writes whole lines using burst
 * move (movem.l) where possible. For byte aligned destinations (long+1 and
 * long+3) it writes longwords only. Same goes for word aligned destinations
 * if FULLSPEED is undefined.
 */
memcpy:
    move.l  (4,%sp),%a1     /* Destination */
    move.l  (8,%sp),%a0     /* Source */
    move.l  (12,%sp),%d1    /* Length */

__memcpy_fwd_entry:
    add.l   %a0,%d1         /* %d1 = source end */

    move.l  %a0,%d0
    addq.l  #7,%d0
    and.l   #0xFFFFFFFC,%d0 /* %d0 = first source long bound + 4 */
    cmp.l   %d0,%d1         /* at least one aligned longword to copy? */
    blo.w   .bytes2_start   /* no, jump directly to trailing byte loop */
    
    subq.l  #4,%d0          /* %d0 = first source long bound */
    cmp.l   %a0,%d0         /* any bytes to copy? */
    jls     .bytes1_end     /* no: skip byte loop */

    /* leading byte loop: copies 0..3 bytes */
.bytes1_loop:
    move.b  (%a0)+,(%a1)+   /* copy byte */
    cmp.l   %a0,%d0         /* runs %a0 up to first long bound */
    jhi     .bytes1_loop

.bytes1_end:
    moveq.l #31,%d0
    add.l   %a0,%d0
    and.l   #0xFFFFFFF0,%d0 /* %d0 = first source line bound + 16 */
    cmp.l   %d0,%d1         /* at least one aligned line to copy? */
    blo.w   .long_start     /* no: jump to longword copy loop */
    
    lea.l   (-28,%sp),%sp   /* free up some registers */
    movem.l %d2-%d7/%a2,(%sp)

    moveq.l #16,%d2
    sub.l   %d2,%d0         /* %d0 = first source line bound */
    move.l  %d1,%a2         /* %a2 = end address */
    lea.l   (-15,%a2),%a2   /* adjust end address for loops doing 16 bytes/ pass */
    move.l  %a1,%d1
    moveq.l #3,%d2          /* mask */
    and.l   %d2,%d1
    jmp.l   (2,%pc,%d1.l*4) /* switch (dest_addr & 3) */
    bra.w   .lines_do0_start
    bra.w   .lines_do1_start
    bra.w   .lines_do2_start
 /* bra.w   .lines_do3_start   implicit */

    /* byte aligned destination (long + 3): use line burst reads in main loop */
.lines_do3_start:
    moveq.l #24,%d1         /* shift count for shifting by 3 bytes */
    cmp.l   %a0,%d0         /* any leading longwords? */
    jhi     .lines_do3_head_start  /* yes: leading longword copy */

    movem.l (%a0),%d4-%d7   /* load first line */
    lea.l   (16,%a0),%a0
    move.l  %d4,%d2
    lsr.l   %d1,%d2         /* get high byte of first longword */
    move.b  %d2,(%a1)+      /* store byte */
    jra     .lines_do3_entry       /* jump into main loop */
    
.lines_do3_head_start:
    move.l  (%a0)+,%d7      /* load first longword */
    move.l  %d7,%d2
    lsr.l   %d1,%d2         /* get high byte */
    move.b  %d2,(%a1)+      /* store byte */
    jra     .lines_do3_head_entry  /* jump into leading longword loop */

.lines_do3_head_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    move.l  %d7,%d2
    lsr.l   %d1,%d2         /* get high byte */
    or.l    %d2,%d6         /* combine with old lower 3 bytes */
    move.l  %d6,(%a1)+      /* store longword */
.lines_do3_head_entry:
    lsl.l   #8,%d7          /* shift up lower 3 bytes */
    cmp.l   %a0,%d0         /* runs %a0 up to first line bound */
    jhi     .lines_do3_head_loop

.lines_do3_loop:
    move.l  %d7,%d3         /* move last longword of old line away */
    movem.l (%a0),%d4-%d7   /* load new line */
    lea.l   (16,%a0),%a0
    move.l  %d4,%d2
    lsr.l   %d1,%d2         /* get high byte of 1st longword */
    or.l    %d2,%d3         /* combine with old lower 3 bytes */
    move.l  %d3,(%a1)+      /* store longword */
.lines_do3_entry:
    lsl.l   #8,%d4          /* shift up lower 3 bytes */
    move.l  %d5,%d2
    lsr.l   %d1,%d2         /* get high byte of 2nd longword */
    or.l    %d2,%d4         /* combine with 1st lower 3 bytes */
    move.l  %d4,(%a1)+      /* store longword */
    lsl.l   #8,%d5          /* shift up lower 3 bytes */
    move.l  %d6,%d2
    lsr.l   %d1,%d2         /* get high byte of 3rd longword */
    or.l    %d2,%d5         /* combine with 2nd lower 3 bytes */
    move.l  %d5,(%a1)+      /* store longword */
    lsl.l   #8,%d6          /* shift up lower 3 bytes */
    move.l  %d7,%d2
    lsr.l   %d1,%d2         /* get high byte of 4th longword */
    or.l    %d2,%d6         /* combine with 3rd lower 3 bytes */
    move.l  %d6,(%a1)+      /* store longword */
    lsl.l   #8,%d7          /* shift up lower 3 bytes */
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_do3_loop

    lea.l   (12,%a2),%a2    /* readjust end address for doing longwords */
    cmp.l   %a0,%a2         /* any trailing longwords? */
    jls     .lines_do3_tail_end    /* no: just store last lower 3 bytes */

.lines_do3_tail_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    move.l  %d7,%d2
    lsr.l   %d1,%d2         /* get high byte */
    or.l    %d2,%d6         /* combine with old lower 3 bytes */
    move.l  %d6,(%a1)+      /* store longword */
    lsl.l   #8,%d7          /* shift up lower 3 bytes */
    cmp.l   %a0,%a2         /* runs %a0 up to last long bound */
    jhi     .lines_do3_tail_loop

.lines_do3_tail_end:
    swap    %d7             /* get high word */
    move.w  %d7,(%a1)+      /* store word */
    lsr.l   %d1,%d7         /* get moved-up low byte */
    move.b  %d7,(%a1)+      /* store byte */
    jra     .lines_end

    /* byte aligned destination (long + 1): use line burst reads in main loop */
.lines_do1_start:
    moveq.l #24,%d1         /* shift count for shifting by 3 bytes */
    cmp.l   %a0,%d0         /* any leading longwords? */
    jhi     .lines_do1_head_start  /* yes: leading longword copy */

    movem.l (%a0),%d4-%d7   /* load first line */
    lea.l   (16,%a0),%a0
    move.l  %d4,%d2         /* first longword, bytes 3210 */
    lsr.l   #8,%d2          /* first longword, bytes .321 */
    swap    %d2             /* first longword, bytes 21.3 */
    move.b  %d2,(%a1)+      /* store byte */
    swap    %d2             /* first longword, bytes .321 */
    move.w  %d2,(%a1)+      /* store word */
    jra     .lines_do1_entry

.lines_do1_head_start:
    move.l  (%a0)+,%d7      /* load first longword */
    move.l  %d7,%d2         /* first longword, bytes 3210 */
    lsr.l   #8,%d2          /* first longword, bytes .321 */
    swap    %d2             /* first longword, bytes 21.3 */
    move.b  %d2,(%a1)+      /* store byte */
    swap    %d2             /* first longword, bytes .321 */
    move.w  %d2,(%a1)+      /* store word */
    jra     .lines_do1_head_entry

.lines_do1_head_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    move.l  %d7,%d2
    lsr.l   #8,%d2          /* get upper 3 bytes */
    or.l    %d2,%d6         /* combine with old low byte */
    move.l  %d6,(%a1)+      /* store longword */
.lines_do1_head_entry:
    lsl.l   %d1,%d7         /* shift up low byte */
    cmp.l   %a0,%d0         /* runs %a0 up to first line bound */
    jhi     .lines_do1_head_loop

.lines_do1_loop:
    move.l  %d7,%d3         /* move last longword of old line away */
    movem.l (%a0),%d4-%d7   /* load new line */
    lea.l   (16,%a0),%a0
    move.l  %d4,%d2
    lsr.l   #8,%d2          /* get upper 3 bytes of 1st longword */
    or.l    %d2,%d3         /* combine with low byte of old longword */
    move.l  %d3,(%a1)+      /* store longword */
.lines_do1_entry:
    lsl.l   %d1,%d4         /* shift up low byte */
    move.l  %d5,%d2
    lsr.l   #8,%d2          /* get upper 3 bytes of 2nd longword */
    or.l    %d2,%d4         /* combine with low byte of 1st longword */
    move.l  %d4,(%a1)+      /* store longword */
    lsl.l   %d1,%d5         /* shift up low byte */
    move.l  %d6,%d2
    lsr.l   #8,%d2          /* get upper 3 bytes of 3rd longword */
    or.l    %d2,%d5         /* combine with low byte of 2nd longword */
    move.l  %d5,(%a1)+      /* store longword */
    lsl.l   %d1,%d6         /* shift up low byte */
    move.l  %d7,%d2
    lsr.l   #8,%d2          /* get upper 3 bytes of 4th longword */
    or.l    %d2,%d6         /* combine with low byte of 4th longword */
    move.l  %d6,(%a1)+      /* store longword */
    lsl.l   %d1,%d7         /* shift up low byte */
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_do1_loop
    
    lea.l   (12,%a2),%a2    /* readjust end address for doing longwords */
    cmp.l   %a0,%a2         /* any trailing longwords? */
    jls     .lines_do1_tail_end    /* no: just store last low byte */
    
.lines_do1_tail_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    move.l  %d7,%d2
    lsr.l   #8,%d2          /* get upper 3 bytes */
    or.l    %d2,%d6         /* combine with old low byte */
    move.l  %d6,(%a1)+      /* store longword */
    lsl.l   %d1,%d7         /* shift up low byte */
    cmp.l   %a0,%a2         /* runs %a0 up to last long bound */
    jhi     .lines_do1_tail_loop

.lines_do1_tail_end:
    lsr.l   %d1,%d7         /* get shifted-up low byte */
    move.b  %d7,(%a1)+      /* store byte */
    jra     .lines_end

    /* long aligned destination (line + 0/4/8/12): head */
.lines_do0_head_loop:
    move.l  (%a0)+,(%a1)+   /* copy longword */
.lines_do0_start:
    cmp.l   %a0,%d0         /* runs %a0 up to first line bound */
    jhi     .lines_do0_head_loop

.lines_do0_head_end:
    move.l  %a1,%d1
    lsr.l   #2,%d1
    moveq.l #3,%d0          /* mask */
    and.l   %d0,%d1
    moveq.l #16,%d0         /* address increment for one main loop pass */
    jmp.l   (2,%pc,%d1.l*2) /* switch ((dest_addr >> 2) & 3) */
    bra.b   .lines_lo0_start
    bra.b   .lines_lo4_start
    bra.b   .lines_lo8_start
 /* bra.b   .lines_lo12_start   implicit */

    /* long aligned destination (line + 12): use line bursts in the loop */
.lines_lo12_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    move.l  %d4,(%a1)+      /* store 1st longword */
    cmp.l   %a0,%a2         /* any full lines? */
    jls     .lines_lo12_end /* no: skip main loop */

.lines_lo12_loop:
    move.l  %d5,%d1         /* move last 3 longwords of old line away */
    move.l  %d6,%d2
    move.l  %d7,%d3
    movem.l (%a0),%d4-%d7   /* load new line */
    add.l   %d0,%a0
    movem.l %d1-%d4,(%a1)   /* store line (3 old + 1 new longwords) */
    add.l   %d0,%a1
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo12_loop

    /* long aligned destination (line + 0/4/8/12): tail */
.lines_lo12_end:
    move.l  %d5,(%a1)+      /* store 3rd last longword */
.lines_lo8_end:
    move.l  %d6,(%a1)+      /* store 2nd last longword */
.lines_lo4_end:
    move.l  %d7,(%a1)+      /* store last longword */
.lines_lo0_end:
    lea.l   (12,%a2),%a2    /* readjust end address for doing longwords */
    cmp.l   %a0,%a2         /* any trailing longwords? */
    jls     .lines_end      /* no: get outta here */

.lines_do0_tail_loop:
    move.l  (%a0)+,(%a1)+   /* copy longword */
    cmp.l   %a0,%a2         /* runs %a0 up to last long bound */
    jhi     .lines_do0_tail_loop

    jra     .lines_end 
    
    /* line aligned destination: use line bursts in the loop */
.lines_lo0_start:
.lines_lo0_loop:
    movem.l (%a0),%d4-%d7   /* load line */
    add.l   %d0,%a0
    movem.l %d4-%d7,(%a1)   /* store line */
    add.l   %d0,%a1
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo0_loop

    jra     .lines_lo0_end  /* handle trailing longwords */

    /* long aligned destination (line + 4): use line bursts in the loop */
.lines_lo4_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    move.l  %d4,(%a1)+      /* store 1st longword */
    move.l  %d5,(%a1)+      /* store 2nd longword */
    move.l  %d6,(%a1)+      /* store 3rd longword */
    cmp.l   %a0,%a2         /* any full lines? */
    jls     .lines_lo4_end  /* no: skip main loop */

.lines_lo4_loop:
    move.l  %d7,%d3         /* move last longword of old line away */
    movem.l (%a0),%d4-%d7   /* load new line */
    add.l   %d0,%a0
    movem.l %d3-%d6,(%a1)   /* store line (1 old + 3 new longwords) */
    add.l   %d0,%a1
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo4_loop

    jra     .lines_lo4_end  /* handle trailing longwords */

    /* long aligned destination (line + 8): use line bursts in the loop */
.lines_lo8_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    move.l  %d4,(%a1)+      /* store 1st longword */
    move.l  %d5,(%a1)+      /* store 2nd longword */
    cmp.l   %a0,%a2
    jls     .lines_lo8_end

.lines_lo8_loop:
    move.l  %d6,%d2         /* move last 2 longwords of old line away */
    move.l  %d7,%d3
    movem.l (%a0),%d4-%d7   /* load new line */
    add.l   %d0,%a0
    movem.l %d2-%d5,(%a1)   /* store line (2 old + 2 new longwords) */
    add.l   %d0,%a1
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo8_loop

    jra     .lines_lo8_end  /* handle trailing longwords */

#ifdef FULLSPEED

    /* word aligned destination (line + 2/6/10/14): head */
.lines_do2_start:
    cmp.l   %a0,%d0         /* any leading longwords? */
    jls     .lines_do2_selector    /* no: jump to mainloop selector */

    move.l  (%a0)+,%d7      /* load first longword */
    swap    %d7             /* swap words */
    move.w  %d7,(%a1)+      /* store high word */
    cmp.l   %a0,%d0         /* any more longword? */
    jls     .lines_do2_head_end    /* no: skip head loop */

.lines_do2_head_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    swap    %d7             /* swap words */
    move.w  %d7,%d6         /* combine high word with old low word */
    move.l  %d6,(%a1)+      /* store longword */
    cmp.l   %a0,%d0         /* runs %a0 up to first line bound */
    jhi     .lines_do2_head_loop

.lines_do2_head_end:
    swap    %d7             /* undo swap */
    move.w  %d7,(%a1)+      /* store word */

.lines_do2_selector:
    move.l  %a1,%d1
    lsr.l   #2,%d1
    moveq.l #3,%d0          /* mask */
    and.l   %d0,%d1
    moveq.l #16,%d0         /* address increment for one main loop pass */
    jmp.l   (2,%pc,%d1.l*4) /* switch ((dest_addr >> 2) & 3) */
    bra.w   .lines_lo2_start
    bra.w   .lines_lo6_start
    bra.w   .lines_lo10_start
 /* bra.w   .lines_lo14_start   implicit */

    /* word aligned destination (line + 14): use line bursts in the loop */
.lines_lo14_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,(%a1)+      /* store word */
    jra     .lines_lo14_entry      /* jump into main loop */

.lines_lo14_loop:
    move.l  %d4,%d0         /* move old line away */
    move.l  %d5,%d1
    move.l  %d6,%d2
    move.l  %d7,%d3
    movem.l (%a0),%d4-%d7   /* load new line */
    lea.l   (16,%a0),%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,%d3         /* combine 1st high word with old low word */
    movem.l %d0-%d3,(%a1)   /* store line */
    lea.l   (16,%a1),%a1
.lines_lo14_entry:
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3rd high word with 2nd low word */
    swap    %d7             /* swap words of 4th long */
    move.w  %d7,%d6         /* combine 4th high word with 3rd low word */
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo14_loop

    /* word aligned destination (line + 2/6/10/14): tail */
.lines_lo14_end:
    move.l  %d4,(%a1)+      /* store third last longword */
.lines_lo10_end:
    move.l  %d5,(%a1)+      /* store second last longword */
.lines_lo6_end:
    move.l  %d6,(%a1)+      /* store last longword */
.lines_lo2_end:
    lea.l   (12,%a2),%a2    /* readjust end address for doing longwords */
    cmp.l   %a0,%a2         /* any trailing longwords? */
    jls     .lines_do2_tail_end    /* no: skip tail loop */
    
.lines_do2_tail_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    swap    %d7             /* swap words */
    move.w  %d7,%d6         /* combine high word with old low word */
    move.l  %d6,(%a1)+      /* store longword */
    cmp.l   %a0,%a2         /* runs %a0 up to last long bound */
    jhi     .lines_do2_tail_loop

.lines_do2_tail_end:
    swap    %d7             /* undo swap */
    move.w  %d7,(%a1)+      /* store last word */
    jra     .lines_end  

    /* word aligned destination (line + 2): use line bursts in the loop */
.lines_lo2_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,(%a1)+      /* store high word */
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3nd high word with 2nd low word */
    swap    %d7             /* swap words of 4th long */
    move.w  %d7,%d6         /* combine 4th high word with 3rd low word */
    move.l  %d4,(%a1)+      /* store 1st longword */
    move.l  %d5,(%a1)+      /* store 2nd longword */
    move.l  %d6,(%a1)+      /* store 3rd longword */
    cmp.l   %a0,%a2         /* any full lines? */
    jls     .lines_lo2_end  /* no: skip main loop */

.lines_lo2_loop:
    move.l  %d7,%d3         /* move last longword of old line away */
    movem.l (%a0),%d4-%d7   /* load line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,%d3         /* combine 1st high word with old low word */
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3rd high word with 2nd low word */
    swap    %d7             /* swap words of 4th long */
    move.w  %d7,%d6         /* combine 4th high word with 3rd low word */
    movem.l %d3-%d6,(%a1)   /* store line */
    add.l   %d0,%a1
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo2_loop

    jra     .lines_lo2_end  /* handle trailing longwords */

    /* word aligned destination (line + 6): use line bursts in the loop */
.lines_lo6_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,(%a1)+      /* store high word */
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3rd high word with 2nd low word */
    move.l  %d4,(%a1)+      /* store 1st longword */
    move.l  %d5,(%a1)+      /* store 2nd longword */
    jra     .lines_lo6_entry       /* jump into main loop */

.lines_lo6_loop:
    move.l  %d6,%d2         /* move last 2 longwords of old line away */
    move.l  %d7,%d3
    movem.l (%a0),%d4-%d7   /* load line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,%d3         /* combine 1st high word with old low word */
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3rd high word with 2nd low word */
    movem.l %d2-%d5,(%a1)   /* store line */
    add.l   %d0,%a1
.lines_lo6_entry:
    swap    %d7             /* swap words of 4th long */
    move.w  %d7,%d6         /* combine 4th high word with 3rd low word */
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo6_loop

    jra     .lines_lo6_end  /* handle trailing longwords */

    /* word aligned destination (line + 10): use line bursts in the loop */
.lines_lo10_start:
    movem.l (%a0),%d4-%d7   /* load first line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,(%a1)+      /* store high word */
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    move.l  %d4,(%a1)+      /* store 1st longword */
    jra     .lines_lo10_entry      /* jump into main loop */

.lines_lo10_loop:
    move.l  %d5,%d1         /* move last 3 longwords of old line away */
    move.l  %d6,%d2
    move.l  %d7,%d3
    movem.l (%a0),%d4-%d7   /* load line */
    add.l   %d0,%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,%d3         /* combine 1st high word with old low word */
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    movem.l %d1-%d4,(%a1)   /* store line */
    add.l   %d0,%a1
.lines_lo10_entry:
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3rd high word with 2nd low word */
    swap    %d7             /* swap words of 4th long */
    move.w  %d7,%d6         /* combine 4th high word with 3rd low word */
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_lo10_loop

    jra     .lines_lo10_end /* handle trailing longwords */

#else /* !FULLSPEED */

    /* word aligned destination (long + 2): use line burst reads in the loop */
.lines_do2_start:
    cmp.l   %a0,%d0         /* any leading longwords? */
    jhi     .lines_do2_head_start  /* yes: leading longword copy */

    movem.l (%a0),%d4-%d7   /* load first line */
    lea.l   (16,%a0),%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,(%a1)+      /* store high word */
    jra     .lines_do2_entry       /* jump into main loop */

.lines_do2_head_start:
    move.l  (%a0)+,%d7      /* load first longword */
    swap    %d7             /* swap words */
    move.w  %d7,(%a1)+      /* store high word */
    cmp.l   %a0,%d0         /* any full longword? */
    jls     .lines_do2_loop /* no: skip head loop */

.lines_do2_head_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    swap    %d7             /* swap words */
    move.w  %d7,%d6         /* combine high word with old low word */
    move.l  %d6,(%a1)+      /* store longword */
    cmp.l   %a0,%d0         /* runs %a0 up to first line bound */
    jhi     .lines_do2_head_loop

.lines_do2_loop:
    move.l  %d7,%d3         /* move last longword of old line away */
    movem.l (%a0),%d4-%d7   /* load line */
    lea.l   (16,%a0),%a0
    swap    %d4             /* swap words of 1st long */
    move.w  %d4,%d3         /* combine 1st high word with old low word */
    move.l  %d3,(%a1)+      /* store 1st longword */
.lines_do2_entry:
    swap    %d5             /* swap words of 2nd long */
    move.w  %d5,%d4         /* combine 2nd high word with 1st low word */
    move.l  %d4,(%a1)+      /* store 2nd longword */
    swap    %d6             /* swap words of 3rd long */
    move.w  %d6,%d5         /* combine 3rd high word with 2nd low word */
    move.l  %d5,(%a1)+      /* store 3rd longword */
    swap    %d7             /* swap words of 4th long */
    move.w  %d7,%d6         /* combine 4th high word with 3rd low word */
    move.l  %d6,(%a1)+      /* store 4th longword */
    cmp.l   %a0,%a2         /* runs %a0 up to last line bound */
    jhi     .lines_do2_loop

.lines_do2_end:
    lea.l   (12,%a2),%a2    /* readjust end address for doing longwords */
    cmp.l   %a0,%a2         /* any trailing longwords? */
    jls     .lines_do2_tail_end    /* no: skip tail loop */
    
.lines_do2_tail_loop:
    move.l  %d7,%d6         /* move old longword away */
    move.l  (%a0)+,%d7      /* load new longword */
    swap    %d7             /* swap words */
    move.w  %d7,%d6         /* combine high word with old low word */
    move.l  %d6,(%a1)+      /* store longword */
    cmp.l   %a0,%a2         /* runs %a0 up to last long bound */
    jhi     .lines_do2_tail_loop

.lines_do2_tail_end:
    swap    %d7             /* undo swap */
    move.w  %d7,(%a1)+      /* store last word */
 /* jra     .lines_end    implicit */

#endif /* !FULLSPEED */

.lines_end:
    addq.l  #3,%a2          /* readjust end address */
    move.l  %a2,%d1         /* end address in %d1 again */
    movem.l (%sp),%d2-%d7/%a2      /* restore registers */
    lea.l   (28,%sp),%sp
    jra     .bytes2_start   /* jump to trailing byte loop */

.long_start:
    subq.l  #3,%d1          /* adjust end address for doing 4 bytes/ pass */

    /* longword copy loop - no lines */
.long_loop:
    move.l  (%a0)+,(%a1)+   /* copy longword (write can be unaligned) */
    cmp.l   %a0,%d1         /* runs %a0 up to last long bound */
    jhi     .long_loop

    addq.l  #3,%d1          /* readjust end address */
    cmp.l   %a0,%d1         /* any bytes left? */
    jls     .bytes2_end     /* no: skip trailing byte loop */

    /* trailing byte loop */
.bytes2_loop:
    move.b  (%a0)+,(%a1)+   /* copy byte */
.bytes2_start:
    cmp.l   %a0,%d1         /* runs %a0 up to end address */
    jhi     .bytes2_loop

.bytes2_end:
    move.l  (4,%sp),%d0     /* return destination */
    rts

.end:
    .size   memcpy,.end-memcpy
#endif
