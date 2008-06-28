/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Karl Kurbjun
 *
 * Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *  Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 *  Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
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

/* Exception Handlers */
.section .vectors,"ax",%progbits
.code 32

.global vectors
vectors:
    b   start
    b   undef_instr_handler
    b   software_int_handler
    b   prefetch_abort_handler
    b   data_abort_handler
    b   reserved_handler
    b   irq_handler
    b   fiq_handler

/*
 * Function: code_copy
 *  Variables:
 *      r0 = from
 *      r1 = to
 *      r2 = length
 */

.section    .init.text, "ax", %progbits
.align      0x04
.global     word_copy
.type       word_copy, %function
word_copy: 
    sub     r2, r2, #0x04
    cmp     r2, #0
    ldrge   r3, [r0], #4
    strge   r3, [r1], #4
    bgt     word_copy
    bx lr
.ltorg  
.size       word_copy, .-word_copy

/*
 * Entry: start
 *  Variables:
 *      none
 */

.section    .init.text,"ax",%progbits
.code       32
.align      0x04 /* Align */
.global     start
start:
    msr     cpsr, #0xd3 /* enter supervisor mode, disable IRQ */

    /* Disable the watchdog */
    ldr     r2, =0x00000000
    mov     r1, #0x53000000
    str     r2, [r1]

    /* Mask all Interupts to be safe */
    ldr     r2, =0xFFFFFFFF
    mov     r1, #0x4A000000
    str     r2, [r1]

    /* Submask too */
    ldr     r2, =0x00003FFF
    str     r2, [r1, #0x1C]

    /* Check if loaded by the old bootloader or by the OF
     * Be careful with code size above this as well.
     */

    /* Get the execute address (cannot be past 0x100 for this to work */
    ldr     r0, =0xffffff00
    and     r0, pc, r0

    /* Calculate the length of the code needed to run/copy */
    ldr     r1, = _vectorstart
    ldr     r2, = _initdata_end
    sub     r2, r2, r1

    add     r3, r2, #0x30000000

    /* Is there enough space to copy without overwriting? */
    cmp     r0, r3

    /* There's enough space, skip copying */
    bgt     skipreset

    /* Is this code running from 0x0?  If so skip copy. */
    cmplt   r0, #0
    beq     skipreset
    
    /* There's not enough space to copy without overwriting, copy to safe spot
     * and reset
     */
    mov     r1, #0x31000000     /* copy location */
    bl      word_copy

    mov     pc, #0x31000000

    skipreset:  

    /* Initial Clock Setup */
    mov     r2, #0x7
    mov     r1, #0x4C000000
    str     r2, [r1, #0x14]

    mov     r2, #0x0
    str     r2, [r1, #0x18]

    ldr     r2, =0xFFFFFFFF
    str     r2, [r1]

    ldr     r2, =0x0003C042
    str     r2, [r1, #0x08]

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    ldr     r2, =0x000C9042
    str     r2, [r1, #0x04]

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    /* If we want to disable extraneous clocks, uncomment, but it can 
     *  freeze the device 
     */
#if 0
    ldr     r2, =0x6030
    mov     r1, #0x4C000000
    str     r2, [r1, #0x0C]
#endif

    /* set Bus to Asynchronous mode (full speed) */    
    mov     r0, #0
    mrc     p15, 0, r0, c1, c0, 0
    ldr     r1,  =0xC0000000
    orr     r0, r0, r1 
    mcr     p15, 0, r0, c1, c0, 0

    /* Setup MISCCR */
    ldr     r2, =0x00613020
    mov     r1, #0x56000000
    str     r2, [r1, #0x80]

    /* Setup some unknown outputs in GPB and GPH */
    ldr     r2, [r1, #0x10] 
    mov     r3, #0x05
    orr     r2, r3, r2
    str     r2, [r1, #0x10]

    ldr     r2, [r1, #0x14]
    mov     r3, #0x03
    orr     r2, r3, r2
    str     r2, [r1, #0x14]

    ldr     r2, [r1, #0x70] 
    mov     r3, #0x05
    orr     r2, r3, r2
    str     r2, [r1, #0x70]

    ldr     r2, [r1, #0x74]
    mov     r3, #0x03
    orr     r2, r3, r2
    str     r2, [r1, #0x74]

    /* Memory setup (taken from 0x5070) */

    /* BWSCON
     *      Reserved 0
     * Bank 0:
     *      Bus width 01 (16 bit)
     * Bank 1:
     *      Buswidth 00 (8 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 2:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 3:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Use UB/LB 1
     * Bank 4:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Use UB/LB 1
     * Bank 5: 
     *      Buswidth 00 (8 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 6:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 7:
     *      Buswidth 00 (8 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     */
    ldr     r2, =0x01055102
    mov     r1, #0x48000000
    str     r2, [r1]

    /* BANKCON0 
     *      Pagemode: normal (1 data) 00
     *      Pagemode access cycle: 2 clocks 00
     *      Address hold: 2 clocks 10
     *      Chip selection hold time: 1 clock 10
     *      Access cycle: 8 clocks 101
     *      Chip select setup time: 1 clock 01
     *      Address setup time: 0 clock 00
     */
    ldr     r2, =0x00000D60
    str     r2, [r1, #0x04]


    /* BANKCON1 
     *      Pagemode: normal (1 data) 00
     *      Pagemode access cycle: 2 clocks 00
     *      Address hold: 0 clocks 00
     *      Chip selection hold time: 0 clock 00
     *      Access cycle: 1 clocks 000
     *      Chip select setup time: 0 clocks 00
     *      Address setup time: 0 clocks 00
     */
    ldr     r2, =0x00000000
    str     r2, [r1, #0x08]

    /* BANKCON2 
     *      Pagemode: normal (1 data) 00
     *      Pagemode access cycle: 2 clocks 00
     *      Address hold: 2 clocks 10
     *      Chip selection hold time: 2 clocks 10
     *      Access cycle: 14 clocks 111
     *      Chip select setup time: 4 clocks 11
     *      Address setup time: 0 clocks 00
     */
    ldr     r2, =0x00001FA0
    str     r2, [r1, #0xC]

    /* BANKCON3 */
    ldr     r2, =0x00001D80
    str     r2, [r1, #0x10]
    /* BANKCON4 */
    str     r2, [r1, #0x14]

    /* BANKCON5 */
    ldr     r2, =0x00000000
    str     r2, [r1, #0x18]

    /* BANKCON6/7
     *      SCAN:   9 bit       01
     *      Trcd:   3 clocks    01
     *      Tcah:   0 clock     00
     *      Tcoh:   0 clock     00
     *      Tacc:   1 clock     000
     *      Tcos:   0 clock     00
     *      Tacs:   0 clock     00
     *      MT:     Sync DRAM   11
     */
    ldr     r2, =0x00018005
    str     r2, [r1, #0x1C]
    /* BANKCON7 */
    str     r2, [r1, #0x20]

    /* REFRESH */
    ldr     r2, =0x00980501
    str     r2, [r1, #0x24]

    /* BANKSIZE
     *      BK76MAP: 32M/32M    000
     *      Reserved: 0         0 (was 1)
     *      SCLK_EN: always     1 (was 0)
     *      SCKE_EN: disable    0
     *      Reserved: 0         0
     *      BURST_EN: enabled   1
     */
    ldr     r2, =0x00000090
    str     r2, [r1, #0x28]

    /* MRSRB6 */
    ldr     r2, =0x00000030
    str     r2, [r1, #0x2C]
    /* MRSRB7 */
    str     r2, [r1, #0x30]
    
#if 0
    /* GPACON */
    mov     r1, #0x56000000
    ldr     r2, =0x01FFFCFF /* 0x01FFFCFF */
    str     r2, [r1]
    
    /* GPADAT */
    ldr     r2, =0x01FFFEFF
    str     r2, [r1, #0x04]

    /* MRSRB6 */
    mov     r1, #0x48000000
    mov     r2, #0x00000000
    str     r2, [r1, #0x2C]

    /* GPADAT */
    mov     r1, #0x56000000
    ldr     r2, =0x01FFFFFF     
    str     r2, [r1, #0x04]

    /* MRSRB6 */
    mov     r1, #0x48000000
    mov     r2, #0x00000030
    str     r2, [r1, #0x2C]

    /* GPACON */
    mov     r1, #0x56000000 
    mov     r2, #0x01FFFFFF
    str     r2, [r1]

    /* End of the unknown */
#endif

    /* The builds have two potential load addresses, one being from flash, 
     * and the other from some "unknown" location right now the assumption
     * is that the code is not at 0x3000000.
     */   
    /* get the high part of our execute address (where am I) */
    ldr     r0, =0xfffff000
    and     r0, pc, r0

    /* Copy code to 0x30000000 */
    ldr     r2, = _vectorstart
    ldr     r3, = _initdata_end

    sub     r2, r3, r2           /* length of loader */
    
    ldr     r1, =0x30000000     /* copy location */   
    
    bl      word_copy

    ldr     r1, =donecopy
    ldr     r2, =0x30000000
    add     r1, r1, r2
    mov     pc, r1           /* The code is located where we want it-jump*/

donecopy: 

    /* Setup the MMU, start by disabling */

    mrc     p15, 0, r0, c1, c0, 0  
    bic     r0, r0, #0x41      /* disable mmu and dcache */
    bic     r0, r0, #0x1000   /* disable icache */
    mcr     p15, 0, r0, c1, c0, 0

    bl      ttb_init
    
    ldr     r0, =0x0
    ldr     r1, =0x0
    ldr     r2, =0x1000
    mov     r3, #0
    bl      map_section

    ldr     r0, =0x30000000
    ldr     r1, =0x0
    mov     r2, #32
    mov     r3, #12
    bl      map_section

    ldr     r0, =0x31FD6800     /* FRAME */
    mov     r1, r0
    mov     r2, #1
    mov     r3, #4
    bl      map_section        
    
    bl      enable_mmu   

    /* Initialise bss section to zero */
    ldr     r2, =_edata
    ldr     r3, =_end
    mov     r4, #0
bsszero:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     bsszero
    
    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =stackend
    mov     r3, sp
    ldr     r2, =stackbegin
    ldr     r4, =0xdeadbeef
stackmunge:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     stackmunge

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
    /* Switch to supervisor mode */
    msr     cpsr_c, #0xd3
    ldr     sp, =stackend

    /* Start the main function */
    ldr     pc, =main

    /* Should never get here, but let's restart in case */
//    b       vectors

/* All illegal exceptions call into UIE with exception address as first
   parameter. This is calculated differently depending on which exception
   we're in. Second parameter is exception number, used for a string lookup
   in UIE.
 */
undef_instr_handler:
    mov    r0, lr
    mov    r1, #0
    b      UIE

/* We run supervisor mode most of the time, and should never see a software
   exception being thrown. Perhaps make it illegal and call UIE?
 */
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

#if defined(BOOTLOADER)
fiq_handler:
    b UIE
#endif

UIE:
    b UIE

.section .text
/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

