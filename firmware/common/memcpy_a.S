/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
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
 *  r0 - data / temporary
 *  r1 - bit mask for rounding to long bounds / 2nd data
 *  r2 - first long bound (only if >= 12 bytes)
 *  r3 - last long bound (-4) (only if >= 12 bytes)
 *  r4 - current dest address
 *  r5 - current source address
 *  r6 - source end address
 *  r7 - stored dest start address
 *
 * The instruction order below is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine also tries to utilize fast page mode.
 */

_memcpy:
    add     r5,r6       /* r6 = source_end */
    mov     r4,r7       /* store for returning */
    add     #-8,r4      /* adjust for early increments (max. 2 longs) */

    mov     r6,r0
    add     #-12,r0     /* r0 = r6 - 12; don't go below 12 here! */
    cmp/hs  r5,r0       /* >= 12 bytes to copy? */
    bf      .start_b2   /* no, jump into byte loop */

    mov     #-4,r1      /* r1 = 0xFFFFFFFC */

    mov     r5,r2
    add     #3,r2
    and     r1,r2       /* r2 = first source long bound */
    mov     r6,r3
    add     #-4,r3      /* end offset for copying 2 longs per pass */
    bra     .start_b1   /* jump into leading byte loop */
    and     r1,r3       /* r3 = last source long bound - 4 */

    /* leading byte loop: copies 0..3 bytes */
    .align  2
.loop_b1:
    mov.b   @r5+,r0     /* load byte & increment source addr */
    add     #1,r4       /* increment dest addr */
    mov.b   r0,@(7,r4)  /* store byte */
.start_b1:
    cmp/hi  r5,r2       /* runs r5 up to first long bound */
    bt      .loop_b1
    /* now r5 is always at a long boundary */
    /* -> memory reading is done in longs for all dest alignments */

    /* selector for main copy loop */
    mov     r4,r0
    tst     #3,r0       /* dest now also at long bound? */
    bt      .loop2_l    /* yes, do long copy */
    tst     #1,r0       /* dest now at least at word bound? */
    bt      .start4_w   /* yes, do word copy */

    /* main loop for byte aligned destination (fast) */
    /* copies 1 long per pass */
    add     #4,r3       /* reset end offset */
    add     #-1,r4      /* adjust to word alignment for word write+ */

.loop4_b:
    mov.l   @r5+,r0     /* load a long & increment source addr */
    add     #4,r4       /* increment dest addr */
    mov.b   r0,@(8,r4)  /* store low byte */
    shlr8   r0          /* get middle 2 bytes */
    mov.w   r0,@(6,r4)  /* store as word+ */
    shlr16  r0          /* get upper byte */
    mov.b   r0,@(5,r4)  /* and store */
    cmp/hi  r5,r3       /* runs r5 up to last long bound */
    bt      .loop4_b

    bra     .start_b2   /* jump to trailing byte loop */
    add     #1,r4       /* readjust */

    /* main loop for word aligned destination (faster) */
    /* copies 2 longs per pass, utilizing fast page mode */
.start4_w:
    add     #-2,r4      /* adjust to long alignment for long write+ */

.loop4_w:
    mov.l   @r5+,r1     /* load first long & increment source addr */
    add     #8,r4       /* increment dest addr */
    mov.l   @r5+,r0     /* load second long & increment source addr */
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
    mov.w   r0,@(8,r4)  /* store low word of second long */
    xtrct   r1,r0       /* extract low word of first long & high word of second long */
    mov.l   r0,@(4,r4)  /* and store as long+ */
    swap.w  r1,r0       /* get high word of first long */
    mov.w   r0,@(2,r4)  /* and store it */
    bt      .loop4_w

    add     #2,r4       /* readjust destination */
    add     #4,r3       /* reset end offset */
    cmp/hi  r5,r3       /* one long left? */
    bf      .start_b2   /* no, jump to trailing byte loop */

    mov.l   @r5+,r0     /* load last long & increment source addr */
    add     #4,r4       /* increment dest addr */
    mov.w   r0,@(6,r4)  /* store low word */
    shlr16  r0          /* get high word */
    bra     .start_b2   /* jump to trailing byte loop */
    mov.w   r0,@(4,r4)  /* and store it */

    /* main loop for long aligned destination (fastest) */
    /* copies 2 longs per pass, utilizing fast page mode */
.loop2_l:
    mov.l   @r5+,r1     /* load first long & increment source addr */
    add     #8,r4       /* increment dest addr */
    mov.l   @r5+,r0     /* load second long & increment source addr */
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
    mov.l   r1,@r4      /* store first long */
    mov.l   r0,@(4,r4)  /* store second long; NOT ALIGNED - no speed loss here! */
    bt      .loop2_l

    add     #4,r3       /* reset end offset */
    cmp/hi  r5,r3       /* one long left? */
    bf      .start_b2   /* no, jump to trailing byte loop */

    mov.l   @r5+,r0     /* load last long & increment source addr */
    add     #4,r4       /* increment dest addr */
    bra     .start_b2   /* jump to trailing byte loop */
    mov.l   r0,@(4,r4)  /* store last long */

    /* trailing byte loop: copies 0..3 bytes (or all for < 12 in total) */
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
#elif CONFIG_CPU == MCF5249
    .align      2
    .global     memcpy
    .type       memcpy,@function

/* Copies <length> bytes of data in memory from <source> to <dest>
 * This version is not optimized at all
 */
memcpy:
        move.l  (4,%sp),%a1    /* Destination */
        move.l  (8,%sp),%a0     /* Source */
        move.l  (12,%sp),%d1    /* Length */

        cmp.l   #0,%d1
        bra.b   .byteloopend

.byteloop:
        move.b  (%a0)+,(%a1)+
        subq.l  #1,%d1
.byteloopend:
        bne.b   .byteloop

        rts
#endif
