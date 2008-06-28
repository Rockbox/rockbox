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

/*  The following code is based on code from the Linux kernel version 2.6.15.3,
 *  linux/arch/arm/lib/memset.S
 *
 *  Copyright (C) 1995-2000 Russell King
 */

        .global     memset16
        .type       memset16,%function
memset16:
        tst     r0, #2                  @ unaligned?
        cmpne   r2, #0
        strneh  r1, [r0], #2            @ store one halfword to align
        subne   r2, r2, #1

/*
 * we know that the pointer in r0 is aligned to a word boundary.
 */
        orr     r1, r1, r1, lsl #16
        mov     r3, r1
        cmp     r2, #8
        blt     4f
/*
 * We need an extra register for this loop - save the return address and
 * use the LR
 */
        str     lr, [sp, #-4]!
        mov     ip, r1
        mov     lr, r1

2:      subs    r2, r2, #32
        stmgeia r0!, {r1, r3, ip, lr}   @ 64 bytes at a time.
        stmgeia r0!, {r1, r3, ip, lr}
        stmgeia r0!, {r1, r3, ip, lr}
        stmgeia r0!, {r1, r3, ip, lr}
        bgt     2b
        ldmeqfd sp!, {pc}               @ Now <64 bytes to go.
/*
 * No need to correct the count; we're only testing bits from now on
 */
        tst     r2, #16
        stmneia r0!, {r1, r3, ip, lr}
        stmneia r0!, {r1, r3, ip, lr}
        tst     r2, #8
        stmneia r0!, {r1, r3, ip, lr}
        ldr     lr, [sp], #4

4:      tst     r2, #4
        stmneia r0!, {r1, r3}
        tst     r2, #2
        strne   r1, [r0], #4

        tst     r2, #1
        strneh  r1, [r0], #2
        bx      lr
.end:
        .size   memset16,.end-memset16
