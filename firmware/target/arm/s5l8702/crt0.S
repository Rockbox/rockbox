/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: crt0.S 18776 2008-10-11 18:32:17Z gevaerts $
 *
 * Copyright (C) 2008 by Marcoen Hirschberg
 * Copyright (C) 2008 by Denes Balatoni
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
#define ASM
#include "config.h"
#include "cpu.h"

    .section .intvect,"ax",%progbits
    .global    start
    .global    _newstart
    /* Exception vectors */
start:
    b _newstart
    ldr pc, =undef_instr_handler
    ldr pc, =software_int_handler
    ldr pc, =prefetch_abort_handler
    ldr pc, =data_abort_handler
    ldr pc, =reserved_handler
    ldr pc, =irq_handler
    ldr pc, =fiq_handler
    .ltorg
_newstart:
#if !defined(BOOTLOADER)
    ldr pc, =newstart2 // we do not want to execute from 0x0 as iram will be mapped there
    .section .init.text,"ax",%progbits
newstart2:
#endif
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

#ifdef BOOTLOADER
    /* Relocate ourself to IRAM1 - we have been loaded to IRAM0 */
    ldr     r0, =_dfuloadaddr
    ldr     r1, =_movestart
    ldr     r2, =_moveend
1:
    ldmia   r0!, {r3-r10}
    stmia   r1!, {r3-r10}
    cmp     r1, r2
    blt     1b

    ldr     pc, =start_loc    /* jump to the relocated start_loc: */

    .section .init.text,"ax",%progbits
    .global  start_loc
start_loc:
#endif

    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #0x1000
    bic     r0, r0, #0x5
    mcr     p15, 0, r0, c1, c0, 0   /* disable caches and protection unit */

.cleancache:
    mrc     p15, 0, r15, c7, c10, 3 /* test and clean dcache */
    bne     .cleancache
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4  /* drain write buffer */
    mcr     p15, 0, r0, c7, c5, 0   /* invalidate icache */

    /* reset VIC controller */
    ldr     r1, =0x38e00000
    add     r2, r1, #0x00001000
    add     r3, r1, #0x00002000
    sub     r4, r0, #1
    str     r4, [r1,#0x14]
    str     r4, [r2,#0x14]
    str     r4, [r1,#0xf00]
    str     r4, [r2,#0xf00]
    str     r4, [r3,#0x08]
    str     r4, [r3,#0x0c]
    str     r0, [r1,#0x14]
    str     r0, [r2,#0x14]

#if defined(BOOTLOADER)
    /* SPI speed is limited when icache is not active. Not worth
     * activating dcache, it is almost useless on pre-init stage
     * and the TLB needs 16Kb in detriment of the bootloader.
     */
    mrc     p15, 0, r0, c1, c0, 0
    orr     r0, r0, #1<<12           /* enable icache */
    mcr     p15, 0, r0, c1, c0, 0
#else
    bl      memory_init

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

    /* Copy icode and data to ram */
    ldr     r2, =_iramstart
    ldr     r3, =_iramend
    ldr     r4, =_iramcopy
1:
    cmp     r3, r2
    ldrhi   r1, [r4], #4
    strhi   r1, [r2], #4
    bhi     1b

    /* Initialise ibss section to zero */
    ldr     r2, =_iedata
    ldr     r3, =_iend
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b
#endif

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

    b       main

#ifdef BOOTLOADER
    /* Initialise bss section to zero */
    .global bss_init
    .type   bss_init, %function

bss_init:
    stmfd   sp!, {r4-r9,lr}

    ldr     r0, =_edata
    ldr     r1, =_end
    mov     r2, #0
    mov     r3, #0
    mov     r4, #0
    mov     r5, #0
    mov     r6, #0
    mov     r7, #0
    mov     r8, #0
    mov     r9, #0
    b       2f
    .align  5   /* cache line size */
1:
    stmia   r0!, {r2-r9}
2:
    cmp     r0, r1
    blt     1b

    ldmpc   regs=r4-r9
    .ltorg
#endif
