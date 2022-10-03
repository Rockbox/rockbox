/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Andree Buschmann
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

    .section .icode, "ax", %progbits

/****************************************************************************
 * void lcd_write_data(const fb_data *addr, 
 *                     int pixelcount);
 * 
 * Writes pixelcount pixels from src-pointer (lcd_framebuffer) to BCM dataport.
 * Use the sequence 2:2:2:2 (2 = read/write 2 regs) for best performance.
 */
    .align  2
    .global lcd_write_data
    .type   lcd_write_data, %function
                                      /* r0 = addr, must be aligned */
lcd_write_data:                       /* r1 = pixel count, must be even */
    stmfd   sp!, {r4, lr}
    mov     lr, #0x30000000           /* LCD data port */

    subs    r1, r1, #16
.loop16:
    ldmgeia r0!, {r2-r3}
    stmgeia lr,  {r2-r3}
    ldmgeia r0!, {r2-r3}
    stmgeia lr,  {r2-r3}
    ldmgeia r0!, {r2-r3}
    stmgeia lr,  {r2-r3}
    ldmgeia r0!, {r2-r3}
    stmgeia lr,  {r2-r3}
    subges  r1, r1, #16
    bge     .loop16

    /* no need to correct the count, we're just checking bits from now */
    tst     r1, #8
    ldmneia r0!, {r2-r4, r12}
    stmneia lr,  {r2-r4, r12}
    tst     r1, #4
    ldmneia r0!, {r2-r3}
    stmneia lr,  {r2-r3}
    tst     r1, #2
    ldrne   r3, [r0], #4
    strne   r3, [lr]

    ldmpc   regs=r4
