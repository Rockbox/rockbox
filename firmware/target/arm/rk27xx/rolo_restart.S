/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2008 by Michael Sevakis
 * Copyright (C) 2012 by Marcin Bukat
 *
 * RoLo firmware restart code for rk27xx
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
#include "cpu.h"

/****************************************************************************
 * void rolo_restart(const unsigned char* source, unsigned char* dest,
 *                   int length);
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     rolo_restart
rolo_restart:
    /* Disable cache */
    ldr     r4, =0xefff0000
    ldrh    r5, [r4]
    strh    r5, [r4]

    /* Ungate all clocks */
    ldr     r4, =0x1801c018
    eor     r5, r5
    str     r5, [r4]

    adr     r4, restart_copy_start
    adr     r5, restart_copy_end
    ldr     r6, =0x18200000
    mov     r7, r6

    /* Copy stub to IRAM */
1:
    ldr     r8, [r4], #4
    str     r8, [r7], #4
    cmp     r5, r4
    bhi     1b

    /* Branch to stub */
    bx      r6

restart_copy_start:
    /* Trivial copy of firmware to final location
     * this version assumes that src and dst are
     * word aligned
     */
    mov     r4, r1
1:
    subs    r2, r2, #4
    ldr     r7, [r0], #4
    str     r7, [r4], #4
    bge     1b

    /* Branch to destination */
    bx      r1
restart_copy_end:
    .size   rolo_restart,.-rolo_restart
