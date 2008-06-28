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
 *  r1 - start address +11 for main loop
 *  r4 - start address
 *  r5 - data (spread to all 4 bytes when using long stores)
 *  r6 - current address (runs down from end to start)
 *
 * The instruction order below is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine fills memory from end to start in
 * order to utilize the auto-decrementing store instructions.
 */

_memset:
    neg     r4,r0
    and     #3,r0       /* r0 = (4 - align_offset) % 4 */
    add     #4,r0
    cmp/hs  r0,r6       /* at least one aligned longword to fill? */
    add     r4,r6       /* r6 = end_address */
    bf      .no_longs   /* no, jump directly to byte loop */

    extu.b  r5,r5       /* start: spread data to all 4 bytes */
    swap.b  r5,r0
    or      r0,r5       /* data now in 2 lower bytes of r5 */
    swap.w  r5,r0
    or      r0,r5       /* data now in all 4 bytes of r5 */
    
    mov     r6,r0
    tst     #3,r0       /* r0 already long aligned? */
    bt      .end_b1     /* yes: skip loop */

    /* leading byte loop: sets 0..3 bytes */
.loop_b1:
    mov.b   r5,@-r0     /* store byte */
    tst     #3,r0       /* r0 long aligned? */
    bf      .loop_b1    /* runs r0 down until long aligned */
    
    mov     r0,r6       /* r6 = last long bound */
    nop                 /* keep alignment */

.end_b1:
    mov     r4,r1       /* r1 = start_address... */
    add     #11,r1      /* ... + 11, combined for rounding and offset */
    xor     r1,r0
    tst     #4,r0       /* bit 2 tells whether an even or odd number of */
    bf      .loop_odd   /* longwords to set */

    /* main loop: set 2 longs per pass */
.loop_2l:
    mov.l   r5,@-r6     /* store first long */
.loop_odd:
    cmp/hi  r1,r6       /* runs r6 down to first long bound */
    mov.l   r5,@-r6     /* store second long */
    bt      .loop_2l

.no_longs:
    cmp/hi  r4,r6       /* any bytes left? */
    bf      .end_b2     /* no: skip loop */

    /* trailing byte loop */
.loop_b2:
    mov.b   r5,@-r6     /* store byte */
    cmp/hi  r4,r6       /* runs r6 down to the start address */
    bt      .loop_b2

.end_b2:
    rts
    mov     r4,r0       /* return start address */

.end:
    .size   _memset,.end-_memset
