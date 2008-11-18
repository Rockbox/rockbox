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
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]
    ldr     pc, [pc, #28]

#if CONFIG_CPU==S5L8700
    .word 0x43554644 /* DFUC */
#else
    .word 0xdeadbeef /* to keep the same PC offsets */
#endif

.word newstart
.word undef_instr_handler
.word software_int_handler
.word prefetch_abort_handler
.word data_abort_handler
.word reserved_handler
.word irq_handler
.word fiq_handler

_vectorsend:

    .text

newstart:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

#if CONFIG_CPU==AS3525 && !defined(BOOTLOADER)

    /* relocate vectors */
    mov     r1, #0              @ destination
    ldr     r2, =_vectorscopy   @ source
    ldr     r3, =_vectorsend    @ end

1:  ldr     r0, [r2], #4
    str     r0, [r1], #4
    cmp     r1, r3
    bne     1b

    /* Zero out IBSS */
    ldr     r2, =_iedata
    ldr     r3, =_iend
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b

    /* Copy the IRAM */
    ldr     r2, =_iramcopy
    ldr     r3, =_iramstart
    ldr     r4, =_iramend
1:
    cmp     r4, r3
    ldrhi   r5, [r2], #4
    strhi   r5, [r3], #4
    bhi     1b
#endif

    /* Initialise bss section to zero */
    ldr     r2, =_edata
    ldr     r3, =_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b
    
    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =stackend
    ldr     r2, =stackbegin
    ldr     r3, =0xdeadbeef
1:
    cmp     sp, r2
    strhi   r3, [r2], #4
    bhi     1b
    
    /* Set up stack for IRQ mode */ 
    msr     cpsr_c, #0xd2
    ldr     sp, =irq_stack

    /* Set up stack for FIQ mode */ 
    msr     cpsr_c, #0xd1
    ldr     sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    msr     cpsr_c, #0xd7
    ldr     sp, =irq_stack
    msr     cpsr_c, #0xdb
    ldr     sp, =irq_stack

    /* Switch back to supervisor mode */
    msr     cpsr_c, #0xd3

    bl      main


/* All illegal exceptions call into UIE with exception address as first
 * parameter. This is calculated differently depending on which exception
 * we're in. Second parameter is exception number, used for a string lookup
 * in UIE. */
undef_instr_handler:
    mov    r0, lr
    mov    r1, #0
    b      UIE

/* We run supervisor mode most of the time, and should never see a software
 * exception being thrown. Perhaps make it illegal and call UIE? */
software_int_handler:
reserved_handler:
    movs   pc, lr

prefetch_abort_handler:
    sub    r0, lr, #4
    mov    r1, #1
    b      UIE

data_abort_handler:
    sub    r0, lr, #8 
    mov    r1, #2
    b      UIE

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

end:
