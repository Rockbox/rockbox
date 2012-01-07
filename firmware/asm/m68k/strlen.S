/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Nils Wallménius
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

    .section .text,"ax",@progbits
    .align   2
    .globl   strlen
    .type    strlen, @function

strlen:
    move.l   4(%sp), %a0    /* %a0 = *str */
    move.l   %a0, %a1       /* %a1 = start address */
    move.l   %a0, %d0
    andi.l   #3, %d0        /* %d0 = %a0 & 3 */
    beq.b    1f             /* already aligned */
    jmp.l    (-2,%pc,%d0.l*4)
    tst.b    (%a0)+
    beq.b    .done
    tst.b    (%a0)+
    beq.b    .done
    tst.b    (%a0)+
    beq.b    .done

    1:
    move.l   (%a0)+, %d0    /* load %d0 increment %a0 */
    /* use trick to test the whole word for null bytes */
    move.l   %d0, %d1
    subi.l   #0x01010101, %d1
    not.l    %d0
    and.l    %d1, %d0
    andi.l   #0x80808080, %d0
    beq.b    1b             /* if the test was false repeat */

 /* ok, so the last word contained a 0 byte, test individual bytes */
    subq.l   #4, %a0
    tst.b    (%a0)+
    beq.b    .done
    tst.b    (%a0)+
    beq.b    .done
    tst.b    (%a0)+
    beq.b    .done
 /* last byte must be 0 so we don't need to load it, so we don't increment a0
    so we jump past the subq instr */
    .word    0x51fa         /* trapf.w, shadow next instr */

.done:
    subq.l   #1, %a0        /* %a0 is 1 too large due to the last increment */
    sub.l    %a1, %a0       /* how many times did we repeat? */
    move.l   %a0, %d0       /* return value in %d0 */
    rts
    .size    strlen, .-strlen

