/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Karl Kurbjun
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

#define CACHE_NONE  0
#define CACHE_ALL   0x0C
#define BUFFERED    0x04

#define LONG_VECTORS 1

/******************************************************************************
 * vectors:                                                                   *
 *    This is the ARM vector table                                            *
 * Long call exception handlers are used for simplicity between flash         *
 * bootloader and SDRAM main-application.  These need to be copied to address *
 * 0x0 at start.                                                              *
 ******************************************************************************/
.section .vectors,"ax"
.code 32

.global _vectors
@entry:
_vectors:

#if defined(SHORT_VECTORS) /* Use relative branch vectors (64 MB limit) */
   b        _start                  /* Reset Vector            */
   b        _undefined_instruction  /* Undefined instruction   */
   b        _software_interrupt     /* Software Vector         */
   b        _prefetch_abort         /* Prefetch Abort          */
   b        _data_abort             /* Data Abort              */
   b        _dead_loop              /* Reserved/Unused         */
   b        irq_handler             /* IRQ vector              */
   b        fiq_handler             /* FIQ vector              */
#else
#if defined(LONG_VECTORS)
   /* Load the PC with the word values stored below */
   ldr      pc, [pc, #0x18]   /* Reset */
   ldr      pc, [pc, #0x18]   /* Undefined instruction   */
   ldr      pc, [pc, #0x18]   /* Software interrupt      */
   ldr      pc, [pc, #0x18]   /* Prefetch Abort          */
   ldr      pc, [pc, #0x18]   /* Data Abort              */
   ldr      pc, [pc, #0x18]   /* Reserved/Unused         */
   ldr      pc, [pc, #0x18]   /* IRQ                     */
   ldr      pc, [pc, #0x18]   /* FIQ                     */
   
   /* Addresses of the handlers */
   .word    _start
   .word    _undefined_instruction
   .word    _software_interrupt
   .word    _prefetch_abort
   .word    _data_abort
   .word    _dead_loop
   .word    irq_handler
   .word    fiq_handler
#else
    #error Vector type undefined
#endif
#endif

/******************************************************************************
 * _start:                                                                    *
 *    This is the main entry point to the program                             *
 ******************************************************************************/
.section    .init, "ax"
.code       32
.align      0x04
.global     _start
_start:
    /* Go into supervisor state with IRQ's disabled.
     * This register is described in section "A2.5 Program status registers" 
     * of the "ARM Architecture Reference Manual".
     */
    msr     cpsr, #0xd3
    
    /* Disable all the fancy stuff */
    mov     r0, #0
    mcr     p15, 0, r0, c1, c0, 0

    /* Disable data and instruction cache, high vectors (at 0xffff0000 instead
     * of 0x00000000)
     */
    mrc    p15, 0, r0, c1, c0, 0
    /* clear bits 13, 9:8 (--VI --RS) */
    bic    r0, r0, #0x00003300
    /* clear bits 7, 2:0 (B--- -C-M) */
    bic    r0, r0, #0x00000085
    /* make sure bit 2 (A) Align is set */
    orr    r0, r0, #0x00000002
    mcr    p15, 0, r0, c1, c0, 0

    /* Add a few cycles of delay before continuing due to system requirements */
    mov     r0, #0x20
    bl      _delay_cycles

#if defined(BOOTLOADER)
    bl      _init_board
#endif

    /* Copy exception handler code to address 0 */
    ldr     r0, =_vectorscopy
    ldr     r1, =_vectorsstart
    ldr     r2, =_vectorsend
    bl      _copy_section

    /* Add some delay time to make sure JTAG can be accessed cleanly */
    mov     r0, #0x100000
    bl      _delay_cycles

#if defined(BOOTLOADER)
    /* Copy the DRAM */
    ldr     r0, =_dramcopy
    ldr     r1, =_dramstart
    ldr     r2, =_dramend
    bl      _copy_section
#endif

    /* Zero out the IBSS */
    mov     r0, #0
    ldr     r1, =_ibss_start
    ldr     r2, =_ibss_end
    bl      _init_section

    /* Copy the IRAM */
    ldr     r0, =_iramcopy
    ldr     r1, =_iramstart
    ldr     r2, =_iramend
    bl      _copy_section

    /* Zero out the BSS */
    mov     r0, #0
    ldr     r1, =_bss_start
    ldr     r2, =_bss_end
    bl      _init_section

    /* Initialize fiq stack */
    ldr     r0, =0xDEADBEEF
    ldr     r1, =_fiq_stack_end     /* Stack counts backwards, so end is first*/
    ldr     r2, =_fiq_stack_start
    bl      _init_section

    msr     cpsr_c, #0xd1           /* Go into fiq state */
    ldr     sp, =_fiq_stack_start   /* set the fiq stack pointer */

    /* Initialize irq stack */
    ldr     r0, =0xDEADBEEF         /* Can be taken out; left for clarity */
    ldr     r1, =_irq_stack_end     /* Stack counts backwards, so end is first*/
    ldr     r2, =_irq_stack_start
    bl      _init_section

    msr     cpsr_c, #0xd2           /* Go into irq state */
    ldr     sp, =_irq_stack_start   /* set the irq stack pointer */

    /* This should not be needed, but set the stack location for abort and
    * undefined to at least a known stack location (IRQ)
    */
    msr     cpsr_c, #0xd7           /* Go into abort state */
    ldr     sp, =_irq_stack_start   /* set the stack pointer */

    msr     cpsr_c, #0xdb           /* Go into undefined state */
    ldr     sp, =_irq_stack_start   /* set the stack pointer */

    /* Initialize program stack */
    msr     cpsr_c, #0xd3           /* Go into supervisor state */
    ldr     r0, =0xDEADBEEF         /* Can be taken out; left for clarity */
    ldr     r1, =_pro_stack_end     /* Stack counts backwards, so end is first*/
    ldr     r2, =_pro_stack_start
    bl      _init_section

    ldr     sp, =_pro_stack_start   /* set the supervisor stack pointer */

    /* MMU initialization */ 
    bl      ttb_init

    /* Make sure everything is mapped on itself */
    ldr     r0, =0x0
    ldr     r1, =0x0
    ldr     r2, =0x1000
    mov     r3, #CACHE_NONE
    bl      map_section

    /* Enable caching for FLASH */
    ldr     r0, =_flash_start
    ldr     r1, =_flash_start
    ldr     r2, =_flash_sizem
    mov     r3, #CACHE_ALL
    bl      map_section

    /* Enable caching for RAM */
    ldr     r0, =_sdram_start
    ldr     r1, =_sdram_start
    ldr     r2, =_sdram_sizem
    mov     r3, #CACHE_ALL
    bl      map_section

    bl      enable_mmu  

    /* Initial setup is complete, go into main */
    ldr      pc, =main

    /* If main returns go into an infinite loop */
    b        _dead_loop

/* Constants go here (from _start - .ltorg): */
.ltorg

/******************************************************************************
 * _init_section:                                                             *
 *    This function initializes a section with the 32-bit value specified.    *
 ******************************************************************************/
.section    .init, "ax"
.code       32
.align      0x04
.global     _init_section
.type       _init_section, %function
/* r0 = init value
 * r1 = start location
 * r2 = end location
 */

/* This function will not run if end is less than or equal to start */
_init_section:
   cmp      r2, r1
   strhi    r0, [r1], #4            /* store and increment start location */
   bhi      _init_section
   bx       lr
.ltorg  
.size       _init_section, .-_init_section

/******************************************************************************
 * _copy_section:                                                             *
 *    This function copies a section to a new location                        *
 ******************************************************************************/
.section    .init, "ax"
.code       32
.align      0x04
.global     _copy_section
.type       _copy_section, %function
/* r0 = source address
 * r1 = destination start address
 * r2 = destination end address
 *
 * r3 is a scratch register
 */

_copy_section:
   cmp      r2, r1
   ldrhi    r3, [r0], #4
   strhi    r3, [r1], #4
   bhi      _copy_section
   bx lr
.ltorg  
.size       _copy_section, .-_copy_section

/******************************************************************************
 * _delay_cycles:                                                             *
 *    This function delays for the specified number of cycles                 *
 ******************************************************************************/
.section    .init, "ax"
.code       32
.align      0x04
.global     _delay_cycles
.type       _delay_cycles, %function
/* r0 = number of cycles to delay */

/* If r0 is zero it will be the maximum length delay */
_delay_cycles:
    subs    r0, r0, #1
    bne     _delay_cycles
    bx      lr
.ltorg  
.size       _delay_cycles, .-_delay_cycles

/******************************************************************************
 *  Unused exception vectors.  These call the UIE function.                   *
 *  Arguements are:                                                           *
 *      r0: PC of exception                                                   *
 *      r1: Exception number.                                                 *
 *  Exception numbers are as defined:                                         *
 *      0: Undefined Instruction                                              *
 *      1: Prefetch Abort                                                     *
 *      2: Data Abort                                                         *
 *  The exceptions return operations are documented in section A2.6 of the    *
 *  ARM Architecture Reference Manual.                                        *
 ******************************************************************************/

/* A2.6.3: Undefined Instruction Exception - LR=PC of next instruction */
_undefined_instruction:
    sub    r0, lr, #4
    mov    r1, #0
    bl     UIE

/* A2.6.4: Software Interrupt exception - These should not happen in Rockbox,
 * but for now leave this as a placeholder and continue with the program.
 * LR=PC of next instruction.
 */
_software_interrupt:
    mov    pc, lr

/* A2.6.5 Prefetch Abort - This is also the BKPT instruction since this is a
 * v5 target.  Pass it on to UIE since it is not currently used.
 */
_prefetch_abort:
    sub    r0, lr, #4
    mov    r1, #1
    bl     UIE

/* A2.6.6 Data Abort - There was a memory abort, can return after fixing cause
 * with the LR address.
 */
_data_abort:
    sub    r0, lr, #8 
    mov    r1, #2
    bl     UIE

/******************************************************************************
 * _dead_loop: Something really unexpected happened (like a reserved          *
 * exception).  Just hang.                                                    *
 ******************************************************************************/
_dead_loop:
   b        _dead_loop

.ltorg

