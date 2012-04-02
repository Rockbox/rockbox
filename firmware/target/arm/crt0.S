/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Marcoen Hirschberg
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

    .section .init.text,"ax",%progbits

    .global    start
start:
    /* Exception vectors */

    /*
     * reset vector *MUST* use relative-addressing only
     * the MMU might not be enabled yet, and the PC might point to
     * a memory region not present in the linked binary
     */

    b   newstart
    b   undef_instr_handler
    b   software_int_handler
    b   prefetch_abort_handler
    b   data_abort_handler
    b   reserved_handler
    b   irq_handler
    b   fiq_handler

_vectorsend:

    .text

newstart:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

#if CONFIG_CPU == AS3525 || CONFIG_CPU == AS3525v2
    bl      memory_init
#endif

#ifdef USE_IRAM
    /* Zero out IBSS */
    ldr     r2, =_iedata
    ldr     r3, =_iend
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b

    /* Copy the IRAM */
    /* must be done before bss is zeroed */
    ldr     r2, =_iramcopy
    ldr     r3, =_iramstart
    ldr     r4, =_iramend
1:
    cmp     r4, r3
    ldrhi   r5, [r2], #4
    strhi   r5, [r3], #4
    bhi     1b
#endif

#ifdef HAVE_INIT_ATTR
    /* copy init data to codec buffer */
    /* must be done before bss is zeroed */
    ldr    r2, =_initcopy
    ldr    r3, =_initstart
    ldr    r4, =_initend
1:
    cmp     r4, r3
    ldrhi   r5, [r2], #4
    strhi   r5, [r3], #4
    bhi     1b

    mov     r2, #0
    mcr     p15, 0, r2, c7, c5, 0   @ Invalidate ICache
#endif

    /* Initialise bss section to zero */
    ldr     r2, =_edata
    ldr     r3, =_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b

    /* Set up stack for IRQ mode */
    msr     cpsr_c, #0xd2
    ldr     sp, =irq_stack

    msr     cpsr_c, #0xd3
#if CONFIG_CPU == AS3525 || CONFIG_CPU == AS3525v2
    /* Let abort and undefined modes use irq stack */
    /* svc stack is for interrupt processing */
    ldr     sp, =svc_stack
#else
    /* Let svc, abort and undefined modes use irq stack */
    ldr     sp, =irq_stack

    /* Set up stack for FIQ mode */
    msr     cpsr_c, #0xd1
    ldr     sp, =fiq_stack
#endif
    msr     cpsr_c, #0xd7
    ldr     sp, =irq_stack
    msr     cpsr_c, #0xdb
    ldr     sp, =irq_stack

    /* Switch to sys mode */
    msr     cpsr_c, #0xdf

    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =stackend
    ldr     r2, =stackbegin
    ldr     r3, =0xdeadbeef
1:
    cmp     sp, r2
    strhi   r3, [r2], #4
    bhi     1b

    ldr     ip, =main       @ make sure we are using the virtual address
    bx      ip

/* Cache-align interrupt stacks */
.balign    32

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ/SVC stack */
    .space 256*4
fiq_stack:
svc_stack:

end: