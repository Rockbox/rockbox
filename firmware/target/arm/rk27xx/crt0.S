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
    ldr pc, =newstart2
    .section .init.text,"ax",%progbits
newstart2:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

    mov     r0, #0x18000000
    add     r0, r0, #0x1c000

    /* setup ARM core freq = 200MHz */
    /* AHB bus freq (HCLK) = 100MHz */
    /* APB bus freq (PCLK) = 50MHz  */
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    orr     r1, #9           /* ARM slow mode, HCLK:PCLK = 2:1 */
    str     r1, [r0,#0x14]

    ldr     r1,=0x01970c70   /* (1<<24) | (1<<23) | (23<<16) | (199<<4) */
    str     r1, [r0,#0x08]

    ldr     r2,=0x40000
1:
    ldr     r1, [r0,#0x2c]   /* SCU_STATUS */
    tst     r1, #1           /* ARM pll lock */
    bne     1f
    subs    r2, #1
    bne     1b
1:
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    bic     r1, #5           /* leave ARM slow mode, ARMclk:HCLK = 2:1 */
    str     r1, [r0,#0x14]
  
#if defined(BOOTLOADER)
    /* remap iram to 0x00000000 */
    ldr     r1,=0xdeadbeef
    str     r1, [r0, #4]
#endif

#if 0
    /* setup caches */
    ldr     r0, =0xefff0000    /* cache controler base address */
    ldrh    r1, [r0]
    strh    r1, [r0]          /* global cache disable */

    /* setup uncached regions */
    mov     r1, #0x18000000
    orr     r1, r1, #0xfe
    str     r1, [r0,#0x10]     /* MemMapA BUS0IP, 32MB */
    str     r1, [r0,#0x14]     /* MemMapB BUS0IP, 32MB */
    mov     r1, #0x30000000
    orr     r1, r1, #0xfe
    str     r1, [r0,#0x18]     /* MemMapC DSPMEM, 32MB */
    mov     r1, #0xee000000    /* 0xefff0000 & 0xfe000000 */
    orr     r1, r1, #0xfe
    str     r1, [r0,#0x1c]     /* MemMapD cache controller, 32MB */

    mov     r1, #2             /* invalidate way opcode */
    str     r1, [r0,#4]        /* invalidate way0 */
1:
    ldr     r2, [r0,#4]
    tst     r2, #3
    bne     1b                 /* wait for invalidate to complete */

    orr     r1, r1, #0x80000000
    str     r1, [r0,#4]        /* invalidate way1 */
1:
    ldr     r2, [r0,#4]
    tst     r2, #3
    bne     1b                 /* wait for invalidate to complete */

    ldr     r1, [r0]
    orr     r1, r1, #0x80000000
    str     r1, [r0]           /* global cache enable */
#endif

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

#ifndef BOOTLOADER
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
    ldr     sp, =_irqstackend

    /* Set up stack for FIQ mode */ 
    msr     cpsr_c, #0xd1
    ldr     sp, =_fiqstackend

    /* Let abort and undefined modes use IRQ stack */
    msr     cpsr_c, #0xd7
    ldr     sp, =_irqstackend
    msr     cpsr_c, #0xdb
    ldr     sp, =_irqstackend

    /* Switch back to supervisor mode */
    msr     cpsr_c, #0xd3

    bl      main

    .text
/*    .global UIE*/

/* All illegal exceptions call into UIE with exception address as first
 * parameter. This is calculated differently depending on which exception
 * we're in. Second parameter is exception number, used for a string lookup
 * in UIE. */
undef_instr_handler:
    sub    r0, lr, #4
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
