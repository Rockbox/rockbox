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
#include "config.h"
#include "cpu.h"

    .section .intvect,"ax",%progbits
    .global    _start
    .global    _newstart
    /* Exception vectors */
_start:
    b _newstart
    ldr pc, =undef_instr_handler
    ldr pc, =software_int_handler
    ldr pc, =prefetch_abort_handler
    ldr pc, =data_abort_handler
    ldr pc, =reserved_handler
    ldr pc, =irq_handler
    ldr pc, =fiq_handler
#if CONFIG_CPU==S5L8700
    .word 0x43554644 /* DFUC */
#endif
    .ltorg
_newstart:
    ldr pc, =newstart2 // we do not want to execute from 0x0 as iram will be mapped there
    .section .init.text,"ax",%progbits
newstart2:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

    mov r1, #0x80
    mrc 15, 0, r0, c1, c0, 0
    orr r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // set bigendian
			 
    ldr r1, =0x3c800000 // disable watchdog
    mov r0, #0xa5
    str r0, [r1]

    mov r0, #0
    ldr r1, =0x39c00008
    str r0, [r1] // mask all interrupts
    ldr r1, =0x39c00020
    str r0, [r1] // mask all external interrupts
    mvn r0, #0
    mov r1, #0x39c00000
    str r0, [r1] // irq priority
    ldr r1, =0x39c00010
    str r0, [r1] // clear pending interrupts
    ldr r1, =0x39c0001c
    str r0, [r1] // clear pending external interrupts
									    
//    ldr r1, =0x3cf00000
//    ldr r0, [r1]
//    mvn r2, #0x30
//    and r0, r0, r2
//    mov r2, #0x10
//    orr r0, r0, r2
//    str r0, [r1]
//    ldr r1, =0x3cf00004
//    ldr r0, [r1]
//    mov r2, #4
//    orr r0, r0, r2
//    str r0, [r1] // switch backlight on

    ldr r1, =0x3c500000 // CLKCON
    ldr r0, =0x00800080
    str r0, [r1]
    ldr r1, =0x3c500024 // PLLCON
    mov r0, #0
    str r0, [r1]
    ldr r1, =0x3c500004 // PLL0PMS
    ldr r0, =0x1ad200
    str r0, [r1]
    ldr r1, =0x3c500014 // PLL0LCNT
    ldr r0, =8100
    str r0, [r1]
    ldr r1, =0x3c500024 // PLLCON
    mov r0, #1
    str r0, [r1]
    ldr r1, =0x3c500020 // PLLLOCK
1:
    ldr r0, [r1]
    tst r0, #1
    beq 1b
    ldr r1, =0x3c50003c // CLKCON2
    mov r0, #0x80
    str r0, [r1]
    ldr r1, =0x3c500000 // CLKCON
    ldr r0, =0x20803180
    str r0, [r1] // FCLK_CPU = 200MHz, HCLK = 100MHz, PCLK = 50MHz, other clocks off

    ldr r2, =0xc0000078
    mrc 15, 0, r0, c1, c0, 0
    mvn r1, #0xc0000000
    and r0, r0, r1
    orr r0, r0, r2
    mcr 15, 0, r0, c1, c0, 0 // asynchronous clocking mode
    nop
    nop					        
    nop
    nop

//    ldr r0, =0x10100000
//    ldr r1, =0x38200034
//    str r0, [r1] // SRAM0/1 data width 16 bit
//    ldr r0, =0x00220922
//    ldr r7, =0x38200038
//    str r0, [r7] // SRAM0/1 clocks
//    ldr r0, =0x00220922
//    ldr r9, =0x3820003c
//    str r0, [r9] // SRAM2/3 clocks
//    nop
//    nop
//    nop
//    nop

    ldr r1, =0x3c500000
    mov r0, #0  // 0x0
    str r0, [r1, #40] // enable clock for all peripherals
    mov r0, #0  // 0x0
    str r0, [r1, #44] // do not enter any power saving mode

    mov r1, #0x1
    mrc 15, 0, r0, c1, c0, 0
    bic r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // disable protection unit

    mov r1, #0x4
    mrc 15, 0, r0, c1, c0, 0
    bic r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // dcache disable

    mov r1, #0x1000
    mrc 15, 0, r0, c1, c0, 0
    bic r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // icache disable

    mov r1, #0
1:
    mov r0, #0
2:
    orr r2, r1, r0
    mcr 15, 0, r2, c7, c14, 2 // clean and flush dcache single entry
    add r0, r0, #0x10
    cmp r0, #0x40
    bne 2b
    add r1, r1, #0x4000000
    cmp r1, #0x0
    bne 1b
    nop
    nop
    mov r0, #0
    mcr 15, 0, r0, c7, c10, 4 // clean and flush whole dcache
									    
    mov r0, #0
    mcr 15, 0, r0, c7, c5, 0 // flush icache
    
    mov r0, #0
    mcr 15, 0, r0, c7, c6, 0 // flush dcache
			       
    mov r0, #0x3f
    mcr 15, 0, r0, c6, c0, 1
    mov r0, #0x2f
    mcr 15, 0, r0, c6, c1, 1
    ldr r0, =0x0800002f
    mcr 15, 0, r0, c6, c2, 1
    ldr r0, =0x22000023
    mcr 15, 0, r0, c6, c3, 1
    ldr r0, =0x24000027
    mcr 15, 0, r0, c6, c4, 1
    mov r0, #0x3f
    mcr 15, 0, r0, c6, c0, 0
    mov r0, #0x2f
    mcr 15, 0, r0, c6, c1, 0
    ldr r0, =0x0800002f
    mcr 15, 0, r0, c6, c2, 0
    ldr r0, =0x22000023
    mcr 15, 0, r0, c6, c3, 0
    ldr r0, =0x24000029
    mcr 15, 0, r0, c6, c4, 0
    mov r0, #0x1e
    mcr 15, 0, r0, c2, c0, 1
    mov r0, #0x1e
    mcr 15, 0, r0, c2, c0, 0
    mov r0, #0x1e
    mcr 15, 0, r0, c3, c0, 0
    ldr r0, =0x0000ffff
    mcr 15, 0, r0, c5, c0, 1
    ldr r0, =0x0000ffff
    mcr 15, 0, r0, c5, c0, 0 // set up protection and caching
				    					
    mov r1, #0x4
    mrc 15, 0, r0, c1, c0, 0
    orr r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // dcache enable
    
    mov r1, #0x1000
    mrc 15, 0, r0, c1, c0, 0
    orr r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // icache enable
						   
    mov r1, #0x1
    mrc 15, 0, r0, c1, c0, 0
    orr r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // enable protection unit

    
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
    ldr     r2, =_datastart
    ldr     r3, =_dataend
    ldr     r4, =_datacopy
1:
    cmp     r3, r2
    ldrhi   r1, [r4], #4
    strhi   r1, [r2], #4
    bhi     1b
    
    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =_stackend
    ldr     r2, =_stackbegin
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

// if we did not switch remap on, device
// would crash when MENU is pressed,
// as that button is connected to BOOT_MODE pin
    ldr r1, =0x38200000
    ldr r0, [r1]
    mvn r2, #0x10000
    and r0, r0, r2
    mov r2, #0x1
    orr r0, r0, r2
    str r0, [r1] // remap iram to address 0x0

    bl      main

    .text
/*    .global UIE*/

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
