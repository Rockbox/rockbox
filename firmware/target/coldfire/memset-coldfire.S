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

    .align      2
    .global     memset
    .type       memset,@function

/* Fills a memory region with specified byte value
 * This version is optimized for speed
 *
 * arguments:
 *  (4,%sp)  - start address
 *  (8,%sp)  - data
 *  (12,%sp) - length
 *
 * return value:
 *  %d0 - start address (like ANSI version)
 *
 * register usage:
 *  %d0 - data (spread to all 4 bytes when using long stores)
 *  %d1 - temporary / data (for burst transfer)
 *  %d2 - data (for burst transfer)
 *  %d3 - data (for burst transfer)
 *  %a0 - start address
 *  %a1 - current address (runs down from end to start)
 *
 * For maximum speed this routine uses both long stores and burst mode,
 * storing whole lines with movem.l. The routine fills memory from end
 * to start in order to ease returning the start address.
 */
memset:
    move.l  (4,%sp),%a0     /* start address */
    move.l  (8,%sp),%d0     /* data */
    move.l  (12,%sp),%a1    /* length */
    add.l   %a0,%a1         /* %a1 = end address */

    move.l  %a0,%d1
    addq.l  #7,%d1
    and.l   #0xFFFFFFFC,%d1 /* %d1 = first long bound + 4 */
    cmp.l   %d1,%a1         /* at least one aligned longword to fill? */
    blo.b   .no_longs       /* no, jump directly to byte loop */

    and.l   #0xFF,%d0       /* start: spread data to all 4 bytes */
    move.l  %d0,%d1
    lsl.l   #8,%d1
    or.l    %d1,%d0         /* data now in 2 lower bytes of %d0 */
    move.l  %d0,%d1
    swap    %d0
    or.l    %d1,%d0         /* data now in all 4 bytes of %d0 */

    move.l  %a1,%d1
    and.l   #0xFFFFFFFC,%d1 /* %d1 = last long bound */
    cmp.l   %d1,%a1         /* any bytes to set? */
    bls.b   .end_b1         /* no: skip byte loop */

    /* leading byte loop: sets 0..3 bytes */
.loop_b1:
    move.b  %d0,-(%a1)      /* store byte */
    cmp.l   %d1,%a1         /* runs %a1 down to last long bound */
    bhi.b   .loop_b1
    
.end_b1:
    moveq.l #31,%d1
    add.l   %a0,%d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = first line bound + 16 */
    cmp.l   %d1,%a1         /* at least one full line to fill? */
    blo.b   .no_lines       /* no, jump to longword loop */

    mov.l   %a1,%d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = last line bound */
    cmp.l   %d1,%a1         /* any longwords to set? */
    bls.b   .end_l1         /* no: skip longword loop */

    /* leading longword loop: sets 0..3 longwords */
.loop_l1:
    move.l  %d0,-(%a1)      /* store longword */
    cmp.l   %d1,%a1         /* runs %a1 down to last line bound */
    bhi.b   .loop_l1

.end_l1:
    move.l  %d2,-(%sp)      /* free some registers */
    move.l  %d3,-(%sp)

    move.l  %d0,%d1         /* spread data to 4 data registers */
    move.l  %d0,%d2
    move.l  %d0,%d3
    lea.l   (15,%a0),%a0    /* start address += 15, acct. for trl. data */
    
    /* main loop: set whole lines utilising burst mode */
.loop_line:
    lea.l   (-16,%a1),%a1   /* pre-decrement */
    movem.l %d0-%d3,(%a1)   /* store line */
    cmp.l   %a0,%a1         /* runs %a1 down to first line bound */
    bhi.b   .loop_line

    lea.l   (-15,%a0),%a0   /* correct start address */
    move.l  (%sp)+,%d3      /* restore registers */
    move.l  (%sp)+,%d2

    move.l  %a0,%d1         /* %d1 = start address ... */
    addq.l  #3,%d1          /* ... +3, account for possible trailing bytes */
    cmp.l   %d1,%a1         /* any longwords left */
    bhi.b   .loop_l2        /* yes: jump to longword loop */
    bra.b   .no_longs       /* no: skip loop */

.no_lines:
    move.l  %a0,%d1         /* %d1 = start address ... */
    addq.l  #3,%d1          /* ... +3, account for possible trailing bytes */

    /* trailing longword loop */
.loop_l2:
    move.l  %d0,-(%a1)      /* store longword */
    cmp.l   %d1,%a1         /* runs %a1 down to first long bound */
    bhi.b   .loop_l2

.no_longs:
    cmp.l   %a0,%a1         /* any bytes left? */
    bls.b   .end_b2         /* no: skip loop */

    /* trailing byte loop */
.loop_b2:
    move.b  %d0,-(%a1)      /* store byte */
    cmp.l   %a0,%a1         /* runs %a1 down to start address */
    bhi.b   .loop_b2

.end_b2:
    move.l  %a0,%d0         /* return start address */
    rts

.end:
    .size   memset,.end-memset
