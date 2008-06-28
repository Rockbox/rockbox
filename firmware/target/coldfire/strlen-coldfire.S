/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id $
 *
 * Copyright (C) 2007 Nils Wallménius
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

/* size_t strlen(const char *str) */

    .section    .text,"ax",@progbits
    .align  2
    .globl  strlen
    .type   strlen, @function

strlen:
    move.l 4(%sp),%a0    /* %a0 = *str */
    move.l %a0,%d0       /* %d0 = start address */

    1:
    tst.b (%a0)+         /* test if %a0 == 0 and increment */
    bne.b 1b             /* if the test was false repeat */

    sub.l %d0,%a0        /* how many times did we repeat? */
    move.l %a0,%d0
    subq.l #1,%d0        /* %d0 is 1 too large due to the last increment */
    rts
    .size   strlen, .-strlen

