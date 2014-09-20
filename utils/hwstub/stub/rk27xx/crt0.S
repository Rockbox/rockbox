/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2008 by Marcoen Hirschberg
 * Copyright (C) 2008 by Denes Balatoni
 * Copyright (C) 2010 by Marcin Bukat
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

    .extern INT_UDC

    .global start
    .global entry_point

    /* Exception vectors */
    .section .intvect,"ax",%progbits
    ldr     pc, =start
    ldr     pc, =start
    ldr     pc, =start
    ldr     pc, =start
    ldr     pc, =data_abort_handler
    ldr     pc, =start
    ldr     pc, =irq_handler
    ldr     pc, =start
    .ltorg

    .section .text,"ax",%progbits
start:
    msr     cpsr_c, #0xd3      /* enter supervisor mode, disable IRQ/FIQ */

    sub     r4, pc, #12         /* copy running address, accomodate
                                 * for prefetch (-8) and msr instr (-4)
                                 */

    ldr     r0, =0xefff0000    /* cache controler base address */
    ldrh    r1, [r0]
    strh    r1, [r0]           /* global cache disable */

    ldr     r2, =_relocstart
    ldr     r3, =_relocend

    cmp     r2, r4
    beq     entry_point        /* skip copying if we are in place already */
1:
    cmp     r3, r2
    ldrhi   r1, [r4], #4
    strhi   r1, [r2], #4
    bhi     1b

entry_point_jmp:
    ldr     pc, =entry_point

entry_point:
    mov     r0, #0x18000000
    add     r0, r0, #0x1c000

    /* setup ARM core freq = 200MHz
     * AHB bus freq (HCLK) = 100MHz
     * APB bus freq (PCLK) = 50MHz
     * Note: it seems there is no way to run AHB bus at ARM freq
     * bit2 in DIVCON1 must have different meaning to what datasheet
     * states. It influences SDRAM read speed but does not change
     * APB freq
     */
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    bic     r1, r1, #0x1f
    orr     r1, r1, #9       /* ((1<<3)|(1<<0)) ARM slow mode, HCLK:PCLK = 2:1 */
    str     r1, [r0,#0x14]

    ldr     r1,=0x1850310    /* ((1<<24)|(1<<23)|(5<<16)|(49<<4)) */
    str     r1, [r0,#0x08]

    ldr     r2,=0x40000
1:
    ldr     r1, [r0,#0x2c]   /* SCU_STATUS */
    tst     r1, #1           /* ARM pll lock */
    bne     1f
    subs    r2, r2, #1
    bne     1b
1:
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    bic     r1, #1           /* leave ARM slow mode */
    str     r1, [r0,#0x14]

    /* remap iram to 0x00000000 */
    ldr     r1,=0xdeadbeef
    str     r1, [r0, #4]

    /* Copy interrupt vectors to iram */
    ldr     r2, =_intvectstart
    ldr     r3, =_intvectend
    ldr     r4, =_intvectcopy
1:
    cmp     r3, r2
    ldrhi   r1, [r4], #4
    strhi   r1, [r2], #4
    bhi     1b

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
    ldr     sp, =_irqstackend

    /* Set up stack for FIQ mode */
    msr     cpsr_c, #0xd1
    ldr     sp, =_fiqstackend

    /* Let svc, abort and undefined modes use irq stack */
    msr     cpsr_c, #0xd3
    ldr     sp, =_irqstackend
    msr     cpsr_c, #0xd7
    ldr     sp, =_irqstackend
    msr     cpsr_c, #0xdb
    ldr     sp, =_irqstackend

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

    /* Jump to C code */
    b      main

/* copy from rockbox code - context save may be excessive but who cares */
irq_handler:
    stmfd sp!, {r0-r5, ip, lr}   /* store context */
    ldr   r4, =0x18080000        /* INTC base */
    ldr   r5, [r4, #0x104]       /* INTC_ISR */
    and   r5, r5, #0x1f          /* irq_no = INTC_ISR & 0x1f */
    cmp   r5, #0x10              /* UDC irq */
    bleq INT_UDC                /* handle it */
    mov   r3, #1
    lsl   r5, r3, r5             /* clear interrupt */
    str   r5, [r4, #0x118]       /* INTC_ICCR = (1<<irq_no) */
    ldmfd sp!, {r0-r5, ip, lr}   /* restore context */
    subs  pc, lr, #4
