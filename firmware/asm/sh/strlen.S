/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Jens Arnold
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
    .global     _strlen
    .type       _strlen,@function

/* Works out the length of a string
 * This version is optimized for speed
 *
 * arguments:
 *  r4 - start address
 *
 * return value:
 *  r0 - string length
 *
 * register usage:
 *  r0 - current address
 *  r1 - current value (byte/long)
 *  r2 - mask for alignment / zero (for cmp/str)
 *  r4 - start address
 *
 */

_strlen:
    mov     r4,r0       /* r0 = start address */
    tst     #3,r0       /* long aligned? */
    bt      .start_l    /* yes, jump directly to the longword loop */

    /* not long aligned: check the first 3 bytes */
    mov.b   @r0+,r1     /* fetch first byte */
    tst     r1,r1       /* byte == 0 ? */
    bt      .hitzero    /* yes, string end found */
    mov.b   @r0+,r1     /* fetch second byte */
    mov     #3,r2       /* prepare mask: r2 = 0..00000011b */
    tst     r1,r1       /* byte == 0 ? */
    bt      .hitzero    /* yes, string end found */
    mov.b   @r0+,r1     /* fetch third byte */
    not     r2,r2       /* prepare mask: r2 = 1..11111100b */
    tst     r1,r1       /* byte == 0 ? */
    bt      .hitzero    /* yes, string end found */

    /* not yet found, fall through into longword loop */
    and     r2,r0       /* align down to long bound */

    /* main loop: check longwords */
.start_l:
    mov     #0,r2       /* zero longword for cmp/str */
.loop_l:
    mov.l   @r0+,r1     /* fetch long word */
    cmp/str r1,r2       /* any zero byte within? */
    bf      .loop_l     /* no, loop */
    add     #-4,r0      /* set address back to start of this longword */

    /* the last longword contains the string end: figure out the byte */
    mov.b   @r0+,r1     /* fetch first byte */    
    tst     r1,r1       /* byte == 0 ? */
    bt      .hitzero    /* yes, string end found */
    mov.b   @r0+,r1     /* fetch second byte */
    tst     r1,r1       /* byte == 0 ? */
    bt      .hitzero    /* yes, string end found */
    mov.b   @r0+,r1     /* fetch third byte */
    tst     r1,r1       /* byte == 0 ? */
    bt      .hitzero    /* yes, string end found */
    rts                 /* must be the fourth byte */
    sub     r4,r0       /* len = string_end - string_start */

.hitzero:
    add     #-1,r0      /* undo address increment */
    rts
    sub     r4,r0       /* len = string_end - string_start */

.end:
    .size   _strlen,.end-_strlen

