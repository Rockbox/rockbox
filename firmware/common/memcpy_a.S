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
 * The instruction order below is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine also tries to utilize fast page mode.
 */

_memcpy:
    mov     r4,r7       /* store dest for returning */
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
    mov     r1,r2       /* copy first long */
    mov.b   r0,@-r4     /* store low byte of second long */
    shlr8   r0          /* get upper 3 bytes */
    shll16  r2          /* move low byte of first long all the way up, .. */
    shll8   r2
    or      r2,r0       /* ..combine with the 3 bytes of second long.. */
    cmp/hi  r5,r3       /* runs r5 up to last or second last long bound */
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
