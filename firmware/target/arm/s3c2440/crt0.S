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
 * Arm bootloader and startup code based on startup.s from the iPodLinux
 * loader
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

/* This branch is used to make sure that we know where the shutdown routine
 *  is located in flash (0x040A0020)
 */
    b   rom_shutdown

/* Add some strings to detect the bootloader in flash and give it a version
 * number.  (0x040A0028, 0x040A002C)
 */
.string "ROCKBOX\0"
.string "R 03.00\0"

/*
 * Function: word_copy
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
    subs    r2, r2, #0x04
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
.align      0x04 
.global     start
start:

    /* Get the execute address; R0 is used to store the address and it should
     * not be written to till the rest of the execution checks are done below.
     * This is done first thing since we have to check if the code was started
     * with the old rockbox bootloader that offset the image by 100 bytes.
     */
    ldr     r0, =0xffffff00
    and     r0, pc, r0

    /************************** DO NOT WRITE TO R0 ***************************/

    /* Check if the code is running from flash. If not skip all these checks */
    cmp     r0, #0xA0000
    bne     poweron

    /* Did an RTC event wake the player up? */ 

    mov     r2, #0x4A000000
    ldr     r1, [r2]
    ands    r1, r1, #0x40000000

    /* Woke up with the alarm - store a flag in GSTATUS3 */
    ldrne   r2, =0x560000b8
    movne   r1, #0x01
    strne   r1, [r2]
    bne     poweron

    /* Set GPG up to read power and menu status */
    ldr     r2, =0x56000050
    ldr     r1, [r2, #0x18]
    orr     r1, r1, #0x03
    str     r1, [r2, #0x18]

    /* Check if menu is held down */
    ldr     r1, [r2, #0x14]
    ands    r3, r1, #0x02
    bne     bootOF

    /* Check if power is held down  */
    ands    r3, r1, #0x01
    bne     poweron

    /* Set GPF up to read charger connection if power is not held down */
    ldr     r1, [r2, #0x08]
    orr     r1, r1, #0x10
    str     r1, [r2, #0x08]

    /* Check if charger is connected */
    ldr     r1, [r2, #0x04]
    ands    r1, r1, #0x10
    beq     poweron

bootOF:
    /* power is not down || menu is held || the charger is not connected */
    mov     pc, #0x70

poweron:
    /* enter supervisor mode, disable IRQ */
    msr     cpsr, #0xd3 

    /* Disable the watchdog */
    ldr     r2, =0x00000000
    mov     r1, #0x53000000
    str     r2, [r1]

    /* Mask all Interupts to be safe */
    ldr     r2, =0xFFFFFFFF
    mov     r1, #0x4A000000
    str     r2, [r1, #0x08]

    /* Submask too */
    ldr     r2, =0x00003FFF
    str     r2, [r1, #0x1C]

    /* Check if loaded by the old bootloader or by the OF.  This copy routine
     * cannot run/copy properly until the memory has been initialized, so the
     * copy routine later is still necessary.  The old bootloader/OF will
     * initialize the memory.
     */

    /* Calculate the length of the code needed to run/copy */
    ldr     r1, = _vectorstart
    ldr     r2, = _initdata_end
    sub     r2, r2, r1

    add     r3, r2, #0x30000000

    /* Is there enough space to copy without overwriting? */
    cmp     r0, r3

    /* There's enough space, skip copying */
    bgt     skipreset

    /* Is this code running from 0xA0000?  If so skip copy. */
    cmplt   r0, #0xA0000
    beq     skipreset

    /****************************** OK TO USE R0 *****************************/
    
    /* There's not enough space to copy without overwriting, copy to safe
     * spot and reset
     */
    mov     r1, #0x31000000     /* copy location */
    bl      word_copy

    mov     pc, #0x31000000

skipreset:  

    /* Initial Clock Setup */
    /* set Bus to Asynchronous mode (full speed) */    
    mov     r0, #0
    mrc     p15, 0, r0, c1, c0, 0
    ldr     r1,  =0xC0000000
    orr     r0, r0, r1 
    mcr     p15, 0, r0, c1, c0, 0

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

    /* Setup MISCCR */
    ldr     r2, =0x00613020
    mov     r1, #0x56000000
    str     r2, [r1, #0x80]

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
    
    /*
        0x56000000 0x1FFFCFF
                 4 0x1FFFEFF
        0X4800002C 0X0
        0X560000
    */

    /* GPACON */
    mov     r1, #0x56000000 
    ldr     r2, =0x00FFFFFF
    str     r2, [r1]

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
    mov     pc, r1           /* The code is located where we want it so jump */

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

    /* Should never get here, but let's restart in case (also needed for
     * linking)
     */
    b       vectors

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

/*
 * Function: rom_shutdown
 *  Variables:
 *      none
 */

.section    .init.text, "ax", %progbits
.align      0x04
.global     rom_shutdown
.type       rom_shutdown, %function
rom_shutdown:
    /* Turn off the MMU */
    mrc     p15, 0, r1, c1, c0, 0
    bic     r1, r1, #0x0001
    mcr     p15, 0, r1, c1, c0, 0

    /* Taken from 0x10010 */
    ldr     r2, =0x56000014 //GPBDAT     
    ldr     r1, =0x56000010 //GPBCON     
    ldr     r3, =0x00015450 
    ldr     r8, =0x56000024 //GPCDAT     
    ldr     r10, =0x56000020 //GPCCON    
    ldr     r5, =0xAAA054A8 
    ldr     r6, =0x56000024 //GPDDAT     
    ldr     r7, =0x56000030 //GPDCON     
    ldr     r12, =0x56000044 //GPEDAT    
    ldr     lr, =0x56000040 //GPECON     
    ldr     r0, =0x56000054 //GPFDAT     
    mov     r4, #0          
    str     r4, [r2]        
    str     r3, [r1]        
    ldr     r1, =0xAAA0AAA5 
    ldr     r2, =0xAA8002AA 
    mov     r3, #0x80       
    str     r3, [r8]        
    add     r3, r3, #0x980  
    str     r5, [r10]       
    mov     r5, #4          
    str     r4, [r6]        
    str     r1, [r7]        
    str     r4, [r12]       
    str     r2, [lr]        
    str     r4, [r0],#(0x56000064-0x56000054) //(GPGDAT - GPFDAT) 
    add     r12, r12, #(0x56000060-0x56000044)//(GPGCON - GPEDAT) 
    ldr     r1, =0x56000050 //GPFCON     
    ldr     r2, =0x01401002 
    mov    lr, #0xFFFFFFFF
    str     r3, [r1],#(0x56000074-0x56000050)// (GPHDAT - GPFCON) 
    str     r4, [r0],#(0x56000060-0x56000054)// (GPGCON - GPFDAT) 
//    str     r2, [r12]
    ldr     r2, =0x140A5    
    mov     r3, #0x81       
    str     r3, [r1]        
    ldr     r12, =0x4A000008 //INTMSK    

//    mov     r3, #0x200000   // disable EINT13
    ldr     r3, =0xFFFFFECF

    str     r2, [r0]        // GPFDAT=0x140A5
    ldr     r2, =0x56000088 //EXTINT0    
    ldr     r0, =0x4A000010 //INTPND 

    add     r1, r1, #(0x56000068 - 0x56000050) // (GPGUP - GPFCON) (0x18)
    str     lr, [r12]       /* INTMSK = 0xFFFFFFFF */
    str     r3, [r2],#(0x560000A4 - 0x56000088) // (EINTMASK - EXTINT0) disable EINT13 (0x1C)
//    mov    r3, #0xFFFFFECF
    mov    r3, #0xFFFFFEFF
    str     r5, [r1],#(0x56000074-0x56000058) //(GPHDAT - GPFUP) (0x1C) DCLKCON
    str     r3, [r2]

    ldr     r11, =0x56000060
    ldr     r6, =0x01401002
    str     r6, [r11]

//    add     r3, r3, #0x00000100 
    ldr     r3, =0xFFFFFFDF;
    str     r3, [r12]       // disable INT_TICK

    mov     r3, #0x4A000000     
    add     r2, r2, #(0x560000B0-0x56000088) //(GSTATUS1 - EXTINT0) //; 0x600 (0x28)
    str     lr, [r1]        
    str     lr, [r3]        
    mov     r3, #0x600      
    str     lr, [r0]        
    str     r3, [r2]        // GSTATUS1 = 0x600 /* what for ??? */

    ldr     r12, =0x56000080 //MISCCr    
    mov     r2, #0x58000000 //ADCCON     
    str     r5, [r2]        
    add     r2, r2, #(0x4D000000-0x58000000) //(LCDCON1 - ADCCON) // LCDCON1 (0xF5000000)
    ldr     r3, [r12]       
    ldr     r0, =0x48000024 // REFRESH   
    bic     r3, r3, #0x700000 
    bic     r3, r3, #0x3000
    orr     r3, r3, #0x600000
    orr     r3, r3, #0x3000 
    str     r3, [r12]       // MISCCR = (MISCCR & ~0x100000) | 0x603000;
    /* clear [20] BATTFLT_FUNC - BATT_FLT function On/Off.
     *            0, Battery fault function will be turned on.
     * set [21] BATTFLT_INTR - BATT_FLT Interrupt On/Off.
     *            1, Battery fault interrupt will be masked by hardware.
     * set [13] SEL_SUSPND1 - USB Port 1 Suspend mode
     *            1= suspend mode
     * set [12] SEL_SUSPND0 - USB Port 0 Suspend mode
     *            1= suspend mode
    */
    mov     r3, #0x4C000000 // LOCKTIME   
    str     r4, [r2]     
  
    str     lr, [r3]        
    ldr     r1, [r0]        
    ldr     lr, =0x4C00000C // CLKCON     
//    str     r1, [r11,#-0x28] 
    ldr     r2, [lr]        
//    str     r2, [r11,#-0x28] 
    ldr     r3, [r0]        
    orr     r3, r3, #0x00C00000
    /* REFRESH
     * [22] TREFMD - SDRAM Refresh Mode
     */
    str     r3, [r0]        
    ldr     r2, [r12]       

    ldr     r3, =0x00004018
    /* [3] SLEEP - Control SLEEP mode of S3C2440X.
     * [4] NAND - Control HCLK into NAND Flash Controller block.
     * [14] RTC - Control PCLK into RTC control block.
     */
    orr     r2, r2, #0x000E0000 
    /* [17] nEN_SCLK0 - SCLK0 output enable (1: SCLK 0 = 0)
     * [18] nEN_SCLK1 - SCLK1 output enable (1: SCLK 1 = 0)
     * [19] OFFREFRESH - Self refresh retain enable after wake-up from sleep
     */

    str     r2, [r12]       
    str     r3, [lr]
    
    1:
    b 1b
.ltorg  
.size       rom_shutdown, .-rom_shutdown

.section .text
/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

