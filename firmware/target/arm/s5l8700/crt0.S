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

/* Meizu M3 SDRAM settings */
#ifdef MEIZU_M3
#define SDR_DSS_SEL_B   1
#define SDR_DSS_SEL_O   1
#define SDR_DSS_SEL_C   1
#define SDR_TIMING      0x6A491D
#define SDR_CONFIG      0x900
#define SDR_MRS         0x37
#define SDR_EMRS        0x4000
#endif

/* Meizu M6SP SDRAM settings */
#ifdef MEIZU_M6SP
#define SDR_DSS_SEL_B   5
#define SDR_DSS_SEL_O   2
#define SDR_DSS_SEL_C   2
#define SDR_TIMING      0x6A4965
#define SDR_CONFIG      0x700
#define SDR_MRS         0x33
#define SDR_EMRS        0x4033
#endif

    .section .intvect,"ax",%progbits
    .global    start
    .global    _newstart
    /* Exception vectors */
start:
#if CONFIG_CPU==S5L8701 && defined(BOOTLOADER)
    b newstart2
#else
    b _newstart
#endif
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

#ifdef ROCKBOX_BIG_ENDIAN
    mov r1, #0x80
    mrc 15, 0, r0, c1, c0, 0
    orr r0, r0, r1
    mcr 15, 0, r0, c1, c0, 0 // set bigendian
#endif
             
    ldr r1, =0x3c800000 // disable watchdog
    mov r0, #0xa5
    str r0, [r1]

    mov r0, #0
    mov r1, #0x39c00000
    str r0, [r1,#0x08] // mask all interrupts
    str r0, [r1,#0x20] // mask all external interrupts
    mvn r0, #0
    str r0, [r1,#0x1c] // clear pending external interrupts
    str r0, [r1] // irq priority
    str r0, [r1,#0x10] // clear pending interrupts

//    ldr r1, =0x3cf00000
//    ldr r0, [r1]
//    mvn r2, #0x30
//    and r0, r0, r2
//    mov r2, #0x10
//    orr r0, r0, r2
//    str r0, [r1]
//    ldr r0, [r1,#0x04]
//    mov r2, #4
//    orr r0, r0, r2
//    str r0, [r1,#0x04] // switch backlight on

#if CONFIG_CPU==S5L8701
    ldr r1, =0x38200000
    ldr r2, [r1]
    orr r2, r2, #1
    bic r2, r2, #0x10000
    str r2, [r1] // remap iram to address 0x0

#ifdef BOOTLOADER
    /* Relocate ourself to IRAM - we have been loaded to DRAM */
    mov    r0, #0x08000000   /* source (DRAM) */
    mov    r1, #0x22000000   /* dest (IRAM) */
    ldr    r2, =_dataend
1:
    cmp    r2, r1
    ldrhi  r3, [r0], #4
    strhi  r3, [r1], #4
    bhi    1b

    ldr    pc, =start_loc    /* jump to the relocated start_loc:  */
start_loc:
#endif /* BOOTLOADER */
#endif /* CONFIG_CPU==S5L8701 */

#if !(CONFIG_CPU==S5L8701 && defined(BOOTLOADER))
    ldr r1, =0x3c500000 
    ldr r0, =0x00800080
    str r0, [r1]        // CLKCON
    mov r0, #0
    str r0, [r1,#0x24]  // PLLCON
#ifdef IPOD_NANO2G
    ldr r0, =0x21200    // pdiv=2, mdiv=0x12 sdiv=0
#else
    ldr r0, =0x1ad200   // pdiv=0x1a, mdiv=0xd2 sdiv=0
#endif
    str r0, [r1,#0x04]  // PLL0PMS
    ldr r0, =8100
    str r0, [r1,#0x14]  // PLL0LCNT
    mov r0, #1
    str r0, [r1,#0x24]  // PLLCON
1:
    ldr r0, [r1,#0x20]  // PLLLOCK
    tst r0, #1
    beq 1b
    mov r0, #0x280
    str r0, [r1,#0x3c]  // CLKCON2
    ldr r0, =0x20803180 // FCLK_CPU = 200MHz, HCLK = 100MHz, PCLK = 50MHz, other clocks off
    str r0, [r1]        // CLKCON

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
#endif

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

/* The following two sections of code (i.e. Nano2G and Meizus) should
   be unified at some point. */
#ifdef IPOD_NANO2G

    ldr r1, =0x3c500000 
    ldr r0, =0xffdff7ff
    str r0, [r1,#0x28]  // PWRCON
    ldr r0, =0xffffef7e
    str r0, [r1,#0x40]  // PWRCONEXT

    mrc 15, 0, r0, c1, c0, 0
    bic r0, r0, #0x1000
    bic r0, r0, #0x5
    mcr 15, 0, r0, c1, c0, 0 // disable caches and protection unit

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
    mcr 15, 0, r0, c7, c5, 0 // flush icache
    mcr 15, 0, r0, c7, c6, 0 // flush dcache

    mov r0, #0x3f
    mcr 15, 0, r0, c6, c0, 1 // CS0: 4GB at offset 0 - everything
    mcr 15, 0, r0, c6, c0, 0 // DS0: 4GB at offset 0 - everything
#ifdef IPOD_NANO2G
    mov r0, #0x31 // FIXME: calculate that from MEMORYSIZE
#else
    mov r0, #0x2f // FIXME: calculate that from MEMORYSIZE
#endif
    mcr 15, 0, r0, c6, c1, 1 // CS1: SRAM/SDRAM mirror
    mcr 15, 0, r0, c6, c1, 0 // DS1: SRAM/SDRAM mirror
    add r0, r0, #0x08000000
    mcr 15, 0, r0, c6, c2, 1 // CS2: SDRAM
    mcr 15, 0, r0, c6, c2, 0 // DS2: SDRAM
    ldr r0, =0x22000023
    mcr 15, 0, r0, c6, c3, 1 // CS3: SRAM
    mcr 15, 0, r0, c6, c3, 0 // DS3: SRAM
    ldr r0, =0x24000027
    mcr 15, 0, r0, c6, c4, 1 // CS4: NOR flash
    mcr 15, 0, r0, c6, c4, 0 // DS4: NOR flash
    mov r0, #0
    mcr 15, 0, r0, c6, c5, 1 // CS5: unused
    mcr 15, 0, r0, c6, c5, 0 // DS5: unused
    mcr 15, 0, r0, c6, c6, 1 // CS6: unused
    mcr 15, 0, r0, c6, c6, 0 // DS6: unused
    mcr 15, 0, r0, c6, c7, 1 // CS7: unused
    mcr 15, 0, r0, c6, c7, 0 // DS7: unused
    mov r0, #0x1e
    mcr 15, 0, r0, c2, c0, 1 // CS1-4: cacheable
    mcr 15, 0, r0, c2, c0, 0 // DS1-4: cacheable
    mcr 15, 0, r0, c3, c0, 0 // DS1-4: write cacheable
    ldr r0, =0x000003ff
    mcr 15, 0, r0, c5, c0, 1 // CS0-4: full access
    mcr 15, 0, r0, c5, c0, 0 // DS0-4: full access

    mrc 15, 0, r0, c1, c0, 0
    orr r0, r0, #0x5
    orr r0, r0, #0x1000
    mcr 15, 0, r0, c1, c0, 0 // re-enable protection unit and caches

#else

    ldr r1, =0x3c500000
    mov r0, #0  // 0x0
    str r0, [r1, #40] // enable clock for all peripherals
    mov r0, #0  // 0x0
    str r0, [r1, #44] // do not enter any power saving mode

#if defined(MEIZU_M6SP) || defined(MEIZU_M3)
    /* setup SDRAM for Meizu M6SP */
    ldr r1, =0x38200000
    // configure SDR drive strength and pad settings
    mov r0, #SDR_DSS_SEL_B
    str r0, [r1, #0x4C] // MIU_DSS_SEL_B
    mov r0, #SDR_DSS_SEL_O
    str r0, [r1, #0x50] // MIU_DSS_SEL_O
    mov r0, #SDR_DSS_SEL_C
    str r0, [r1, #0x54] // MIU_DSS_SEL_C
    mov r0, #2
    str r0, [r1, #0x60] // SSTL2_PAD_ON
    // select SDR mode
    ldr r0, [r1, #0x40]
    mov r2, #0xFFFDFFFF
    and r0, r0, r2
    orr r0, r0, #1
    str r0, [r1, #0x40] // MIUORG
    // set controller configuration
    mov r0, #SDR_CONFIG
    str r0, [r1]        // MIUCON
    // set SDRAM timing
    ldr r0, =SDR_TIMING
    str r0, [r1, #0x10] // MIUSDPARA
    // set refresh rate
    mov r0, #0x1080
    str r0, [r1, #0x08] // MIUAREF
    // initialise SDRAM 
    mov r0, #0x003
    str r0, [r1, #0x04] // MIUCOM = nop
    ldr r0, =0x203
    str r0, [r1, #0x04] // MIUCOM = precharge all banks
    nop
    nop
    nop
    ldr r0, =0x303
    str r0, [r1, #0x04] // MIUCOM = auto-refresh
    nop
    nop
    nop
    nop
    str r0, [r1, #0x04] // MIUCOM = auto-refresh
    nop
    nop
    nop
    nop
    str r0, [r1, #0x04] // MIUCOM = auto-refresh
    nop
    nop
    nop
    nop
    // set mode register
    mov r0, #SDR_MRS
    str r0, [r1, #0x0C] // MIUMRS
    ldr r0, =0x103
    str r0, [r1, #0x04] // MIUCOM = mode register set
    ldr r0, =SDR_EMRS
    str r0, [r1, #0x0C] // MIUMRS
    ldr r0, =0x103
    str r0, [r1, #0x04] // MIUCOM = mode register set
#endif /* MEIZU_M6SP */

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
    ldr r0, =0x08000031
    mcr 15, 0, r0, c6, c2, 1
    ldr r0, =0x22000023
    mcr 15, 0, r0, c6, c3, 1
    ldr r0, =0x24000027
    mcr 15, 0, r0, c6, c4, 1
    mov r0, #0x3f
    mcr 15, 0, r0, c6, c0, 0
    mov r0, #0x2f
    mcr 15, 0, r0, c6, c1, 0
    ldr r0, =0x08000031
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
#endif

#if CONFIG_CPU==S5L8700 || !defined(BOOTLOADER)
    /* Copy interrupt vectors to iram */
    ldr     r2, =_intvectstart
    ldr     r3, =_intvectend
    ldr     r4, =_intvectcopy
1:
    cmp     r3, r2
    ldrhi   r1, [r4], #4
    strhi   r1, [r2], #4
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

#if CONFIG_CPU==S5L8700 && defined(BOOTLOADER)
    /* Copy icode and data to ram */
    ldr     r2, =_datastart
    ldr     r3, =_dataend
    ldr     r4, =_datacopy
1:
    cmp     r3, r2
    ldrhi   r1, [r4], #4
    strhi   r1, [r2], #4
    bhi     1b
#endif

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

// if we did not switch remap on, device
// would crash when MENU is pressed,
// as that button is connected to BOOT_MODE pin
#if CONFIG_CPU==S5L8700
    ldr r1, =0x38200000
    ldr r0, [r1]
    mvn r2, #0x10000
    and r0, r0, r2
    mov r2, #0x1
    orr r0, r0, r2
    str r0, [r1] // remap iram to address 0x0
#endif

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
