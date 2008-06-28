/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Thom Johansen
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

    .section    .icode,"ax",%progbits

    .align      2

/*  The following code is based on code found in Linux kernel version 2.6.15.3
 *  linux/arch/arm/lib/memset.S
 *
 *  Copyright (C) 1995-2000 Russell King
 */

/* This code will align a pointer for memset, if needed */
1:      cmp     r2, #4                  @ 1 do we have enough
        blt     5f                      @ 1 bytes to align with?
        cmp     r3, #2                  @ 1
        strgtb  r1, [r0, #-1]!          @ 1
        strgeb  r1, [r0, #-1]!          @ 1
        strb    r1, [r0, #-1]!          @ 1
        sub     r2, r2, r3              @ 1 r2 = r2 - r3
        b 2f

        .global     memset
        .type       memset,%function
memset:
        add     r0, r0, r2              @ we'll write backwards in memory
        ands    r3, r0, #3              @ 1 unaligned?
        bne     1b                      @ 1
2:
/*
 * we know that the pointer in r0 is aligned to a word boundary.
 */
        orr     r1, r1, r1, lsl #8
        orr     r1, r1, r1, lsl #16
        mov     r3, r1
        cmp     r2, #16
        blt     5f
/*
 * We need an extra register for this loop - save the return address and
 * use the LR
 */
        str     lr, [sp, #-4]!
        mov     ip, r1
        mov     lr, r1

3:      subs    r2, r2, #64
        stmgedb r0!, {r1, r3, ip, lr}   @ 64 bytes at a time.
        stmgedb r0!, {r1, r3, ip, lr}
        stmgedb r0!, {r1, r3, ip, lr}
        stmgedb r0!, {r1, r3, ip, lr}
        bgt     3b
        ldmeqfd sp!, {pc}               @ Now <64 bytes to go.
/*
 * No need to correct the count; we're only testing bits from now on
 */
        tst     r2, #32
        stmnedb r0!, {r1, r3, ip, lr}
        stmnedb r0!, {r1, r3, ip, lr}
        tst     r2, #16
        stmnedb r0!, {r1, r3, ip, lr}
        ldr     lr, [sp], #4

5:      tst     r2, #8
        stmnedb r0!, {r1, r3}
        tst     r2, #4
        strne   r1, [r0, #-4]!
/*
 * When we get here, we've got less than 4 bytes to zero.  We
 * may have an unaligned pointer as well.
 */
6:      tst     r2, #2
        strneb  r1, [r0, #-1]!
        strneb  r1, [r0, #-1]!
        tst     r2, #1
        strneb  r1, [r0, #-1]!
        bx      lr
.end:
        .size   memset,.end-memset
