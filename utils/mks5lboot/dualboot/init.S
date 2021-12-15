/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright © 2009 Michael Sparmann
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

    .section .init.text, "ax", %progbits
    .global _start

_start:
    mov     r0, #0xD7
    msr     CPSR_c, r0         @ Abort mode, IRQs/FIQs disabled
    ldr     sp, =_exception_stack

    mov     r0, #0xDB
    msr     CPSR_c, r0         @ Undefined Instruction mode, IRQs/FIQs disabled
    ldr     sp, =_exception_stack

    mov     r0, #0xD3
    msr     CPSR_c, r0         @ Supervisor mode, IRQs/FIQs disabled
    ldr     sp, =_supervisor_stack

    MOV R0, #0x00050000
    ORR R0, #0x00000078
    MCR p15, 0, R0, c1, c0, 0  @ Get rid of some CPU "features" likely to cause trouble

    bl      main
    .ltorg
