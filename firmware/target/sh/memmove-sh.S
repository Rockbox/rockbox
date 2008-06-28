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

    .align      2
    .global     _memmove
    .type       _memmove,@function

/* Moves <length> bytes of data in memory from <source> to <dest>
 * Regions may overlap.
 * This version is optimized for speed, and needs the corresponding memcpy
 * implementation for the forward copy branch.
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
 *  r3 - last long bound / adjusted start address (only if >= 11 bytes)
 *  r4 - current dest address
 *  r5 - source start address
 *  r6 - current source address
 *
 * The instruction order is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine also tries to utilize fast page mode.
 */

_memmove:
    cmp/hi  r4,r5       /* source > destination */
    bf     .backward    /* no: backward copy */
    mov.l  .memcpy_fwd,r0
    jmp     @r0
    mov     r4,r7       /* store dest for returning */

    .align  2
.memcpy_fwd:
    .long   ___memcpy_fwd_entry

.backward:
    add     r6,r4       /* r4 = destination end */
    mov     #11,r0
    cmp/hs  r0,r6       /* at least 11 bytes to copy? (ensures 2 aligned longs) */
    add     #-8,r5      /* adjust for late decrement (max. 2 longs) */
    add     r5,r6       /* r6 = source end - 8 */
    bf      .start_b2r  /* no: jump directly to byte loop */
    
    mov     #-4,r3      /* r3 = 0xfffffffc */
    and     r6,r3       /* r3 = last source long bound */
    cmp/hi  r3,r6       /* already aligned? */
    bf      .end_b1r    /* yes: skip leading byte loop */
    
.loop_b1r:
    mov.b   @(7,r6),r0  /* load byte */
    add     #-1,r6      /* decrement source addr */
    mov.b   r0,@-r4     /* store byte */
    cmp/hi  r3,r6       /* runs r6 down to last long bound */
    bt      .loop_b1r
    
.end_b1r:
    mov     #3,r1
    and     r4,r1       /* r1 = dest alignment offset */
    mova    .jmptab_r,r0
    mov.b   @(r0,r1),r1 /* select appropriate main loop.. */
    add     r0,r1
    mov     r5,r3       /* copy start adress to r3 */
    jmp     @r1         /* ..and jump to it */
    add     #7,r3       /* adjust end addr for main loops doing 2 longs/pass */

    /** main loops, copying 2 longs per pass to profit from fast page mode **/

    /* long aligned destination (fastest) */
    .align  2
.loop_do0r:
    mov.l   @r6,r1      /* load first long */
    add     #-8,r6      /* decrement source addr */
    mov.l   @(12,r6),r0 /* load second long */
    cmp/hi  r3,r6       /* runs r6 down to first or second long bound */
    mov.l   r0,@-r4     /* store second long */
    mov.l   r1,@-r4     /* store first long; NOT ALIGNED - no speed loss here! */
    bt      .loop_do0r
    
    add     #-4,r3      /* readjust end address */
    cmp/hi  r3,r6       /* first long left? */
    bf      .start_b2r  /* no, jump to trailing byte loop */
    
    mov.l   @(4,r6),r0  /* load first long */
    add     #-4,r6      /* decrement source addr */
    bra     .start_b2r  /* jump to trailing byte loop */
    mov.l   r0,@-r4     /* store first long */
    
    /* word aligned destination (long + 2) */
    .align  2
.loop_do2r:
    mov.l   @r6,r1      /* load first long */
    add     #-8,r6      /* decrement source addr */
    mov.l   @(12,r6),r0 /* load second long */
    cmp/hi  r3,r6       /* runs r6 down to first or second long bound */
    mov.w   r0,@-r4     /* store low word of second long */
    xtrct   r1,r0       /* extract low word of first long & high word of second long */
    mov.l   r0,@-r4     /* and store as long */
    shlr16  r1          /* get high word of first long */
    mov.w   r1,@-r4     /* and store it */
    bt      .loop_do2r

    add     #-4,r3      /* readjust end address */
    cmp/hi  r3,r6       /* first long left? */
    bf      .start_b2r  /* no, jump to trailing byte loop */

    mov.l   @(4,r6),r0  /* load first long & decrement source addr */
    add     #-4,r6      /* decrement source addr */
    mov.w   r0,@-r4     /* store low word */
    shlr16  r0          /* get high word */
    bra     .start_b2r  /* jump to trailing byte loop */
    mov.w   r0,@-r4     /* and store it */

    /* jumptable for loop selector */
    .align  2
.jmptab_r:
    .byte   .loop_do0r - .jmptab_r  /* placed in the middle because the SH1 */
    .byte   .loop_do1r - .jmptab_r  /* loads bytes sign-extended. Otherwise */
    .byte   .loop_do2r - .jmptab_r  /* the last loop would be out of reach */
    .byte   .loop_do3r - .jmptab_r  /* of the offset range. */
    
    /* byte aligned destination (long + 1) */
    .align  2
.loop_do1r:
    mov.l   @r6,r1      /* load first long */
    add     #-8,r6      /* decrement source addr */
    mov.l   @(12,r6),r0 /* load second long */
    cmp/hi  r3,r6       /* runs r6 down to first or second long bound */
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
    bt      .loop_do1r
    
    add     #-4,r3      /* readjust end address */
.last_do13r:
    cmp/hi  r3,r6       /* first long left? */
    bf      .start_b2r  /* no, jump to trailing byte loop */

    nop                 /* alignment */
    mov.l   @(4,r6),r0  /* load first long */
    add     #-4,r6      /* decrement source addr */
    mov.b   r0,@-r4     /* store low byte */
    shlr8   r0          /* get middle 2 bytes */
    mov.w   r0,@-r4     /* store as word */
    shlr16  r0          /* get upper byte */
    bra     .start_b2r  /* jump to trailing byte loop */
    mov.b   r0,@-r4     /* and store */

    /* byte aligned destination (long + 3) */
    .align  2
.loop_do3r:
    mov.l   @r6,r1      /* load first long */
    add     #-8,r6      /* decrement source addr */
    mov.l   @(12,r6),r0 /* load second long */
    mov     r1,r2       /* copy first long */
    mov.b   r0,@-r4     /* store low byte of second long */
    shlr8   r0          /* get middle 2 bytes */
    mov.w   r0,@-r4     /* store as word */
    shlr16  r0          /* get upper byte */
    shll8   r2          /* move lower 3 bytes of first long one up.. */
    or      r2,r0       /* ..combine with the 1 byte of second long.. */
    mov.l   r0,@-r4     /* ..and store as long */
    shlr16  r1          /* get upper byte of first long */
    shlr8   r1
    cmp/hi  r3,r6       /* runs r6 down to first or second long bound */
    mov.b   r1,@-r4     /* ..and store */
    bt      .loop_do3r

    bra     .last_do13r /* handle first longword: reuse routine for (long + 1) */
    add     #-4,r3      /* readjust end address */

    /* trailing byte loop: copies 0..3 bytes (or all for < 11 in total) */
    .align  2
.loop_b2r:
    mov.b   @(7,r6),r0  /* load byte */
    add     #-1,r6      /* decrement source addr */
    mov.b   r0,@-r4     /* store byte */
.start_b2r:
    cmp/hi  r5,r6       /* runs r6 down to start address */
    bt      .loop_b2r

    rts
    mov     r4,r0       /* return dest start address */
.end:
    .size   _memmove,.end-_memmove
