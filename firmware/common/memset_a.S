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

    .align      2
#if CONFIG_CPU == SH7034        
    .global     _memset
    .type       _memset,@function

/* Fills a memory region with specified byte value
 * This version is optimized for speed
 *
 * arguments:
 *  r4 - start address
 *  r5 - data
 *  r6 - length
 *
 * return value:
 *  r0 - start address (like ANSI version)
 *
 * register usage:
 *  r0 - temporary
 *  r1 - bit mask for rounding to long bounds
 *  r2 - last / first long bound (only if >= 12 bytes)
 *  r4 - start address
 *  r5 - data (spread to all 4 bytes if >= 12 bytes)
 *  r6 - current address (runs down from end to start)
 *
 * The instruction order below is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine fills memory from end to start in
 * order to utilize the auto-decrementing store instructions.
 */

_memset:
    add     r4,r6       /* r6 = end_address */

    mov     r6,r0
    add     #-12,r0     /* r0 = r6 - 12; don't go below 12 here! */
    cmp/hs  r4,r0       /* >= 12 bytes to fill? */
    bf      .start_b2   /* no, jump directly to byte loop */

    extu.b  r5,r5       /* start: spread data to all 4 bytes */
    swap.b  r5,r0
    or      r0,r5       /* data now in 2 lower bytes of r5 */
    swap.w  r5,r0
    or      r0,r5       /* data now in all 4 bytes of r5 */

    mov     #-4,r1      /* r1 = 0xFFFFFFFC */

    mov     r6,r2
    bra     .start_b1
    and     r1,r2       /* r2 = last long bound */

    /* leading byte loop: sets 0..3 bytes */
.loop_b1:
    mov.b   r5,@-r6     /* store byte */
.start_b1:
    cmp/hi  r2,r6       /* runs r6 down to last long bound */
    bt      .loop_b1

    mov     r4,r2
    add     #11,r2      /* combined for rounding and offset */
    and     r1,r2       /* r2 = first long bound + 8 */

    /* main loop: set 2 longs per pass */
.loop2_l:
    mov.l   r5,@-r6     /* store first long */
    cmp/hi  r2,r6       /* runs r6 down to first or second long bound */
    mov.l   r5,@-r6     /* store second long */
    bt      .loop2_l

    add     #-8,r2      /* correct offset */
    cmp/hi  r2,r6       /* 1 long left? */
    bf      .start_b2   /* no, jump to trailing byte loop */

    bra     .start_b2   /* jump to trailing byte loop */
    mov.l   r5,@-r6     /* store last long */

    /* trailing byte loop */
    .align  2
.loop_b2:
    mov.b   r5,@-r6     /* store byte */
.start_b2:
    cmp/hi  r4,r6       /* runs r6 down to the start address */
    bt      .loop_b2

    rts
    mov     r4,r0       /* return start address */

.end:
    .size   _memset,.end-_memset
#elif CONFIG_CPU == MCF5249
    .global     memset
    .type       memset,@function

/* Fills a memory region with specified byte value
 * This version is not optimized at all
 */
memset:
        move.l  (4,%sp),%a0     /* Start address */
        move.l  (8,%sp),%d0    /* Value */
        move.l  (12,%sp),%d1    /* Length */
        lea.l   (%d1,%a0),%a1 /* a1 = a0+d1 */

        bra.b   .byteloopend
        
.byteloop:
        move.b  %d0,(%a0)+
.byteloopend:
        cmp.l   %a0,%a1
        bne.b   .byteloop
        
        rts
#endif
