/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2009 by Jorge Pinto
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

#include "at91sam9260.h"

#define DRAMORIG        AT91C_EBI_SDRAM_32BIT
#define DRAMSIZE        (MEMORYSIZE * 0x100000)
#define IRAM0ORIG       AT91C_IRAM_1
#define IRAM0SIZE       AT91C_IRAM_1_SIZE
#define TOP_OF_MEMORY   (IRAM0ORIG + IRAM0SIZE)
#define ABT_STACK_SIZE  8*3*4
#define IRQ_STACK_SIZE  8*3*4

#define ARM_MODE_ABT     0x17
#define ARM_MODE_FIQ     0x11
#define ARM_MODE_IRQ     0x12
#define ARM_MODE_SVC     0x13
#define ARM_MODE_UND     0x1b
#define ARM_MODE_SYS     0x1f

#define I_BIT            0x80
#define F_BIT            0x40


/* Application startup entry point */
.globl reset_handler

.align 4

.section .vectors
.arm


/* Exception vectors (should be a branch to be detected as a valid code
 * by the rom) */
_exception_vectors:
reset_vector:
    ldr      pc, =reset_handler
undef_vector:
    b        undef_vector    /* Undefined Instruction */
swi_vector:
    b        swi_vector      /* Software Interrupt */
pabt_vector:
    ldr      pc, =pabt_handler     /* Prefetch Abort */
dabt_vector:
    ldr      pc, =dabt_handler     /* Data Abort */
rsvd_vector:
    b        rsvd_vector     /* reserved */
irq_vector:
    b        irq_handler     /* IRQ : read the AIC */
fiq_vector:
/*-----------------------------------------------------------------------------
 *- Function             : fiq_handler
 *- Treatments           : FIQ (Fast Interrupt) Interrupt Handler.
 *- Called Functions     :
 *---------------------------------------------------------------------------*/
fiq_handler:
pabt_handler:
dabt_handler:
    b        fiq_handler

/*-----------------------------------------------------------------------------
 *- Function             : irq_handler
 *- Treatments           : IRQ Controller Interrupt Handler.
 *- Called Functions     : AIC_IVR[interrupt]
 *---------------------------------------------------------------------------*/
irq_handler:
/*- Manage Exception Entry */
/*- Adjust and save LR_irq in IRQ stack - (LR - Link Register) */
    sub      lr, lr, #4
    stmfd    sp!, {lr}
/*- Save r0 and SPSR (Saved Program Status Register) in IRQ stack */
    mrs      r14, SPSR
    stmfd    sp!, {r0,r14}

/*- Write in the IVR (Interrupt Vector Register) to support Protect Mode */
/*- No effect in Normal Mode */
/*- De-assert the NIRQ and clear the source in Protect Mode */
    ldr      r14, =AT91C_BASE_AIC
    ldr      r0 , [r14, #AIC_IVR]
    str      r14, [r14, #AIC_IVR]

/*- Enable Interrupt and Switch in Supervisor Mode */
    msr      CPSR_c, #ARM_MODE_SVC

/*- Save scratch/used registers and LR in User Stack */
    stmfd    sp!, {r1-r3, r12, r14}

/*- Branch to the routine pointed by the AIC_IVR */
    mov      r14, pc
    bx       r0

/*- Restore scratch/used registers and LR from User Stack */
    ldmia    sp!, {r1-r3, r12, r14}

/*- Disable Interrupt and switch back in IRQ mode */
    msr      CPSR_c, #ARM_MODE_IRQ | I_BIT

/*- Mark the End of Interrupt on the AIC */
    ldr      r14, =AT91C_BASE_AIC
    str      r14, [r14, #AIC_EOICR]

/*- Restore SPSR_irq and r0 from IRQ stack */
    ldmia    sp!, {r0,r14}
    msr      SPSR_cxsf, r14

/*- Restore adjusted LR_irq from IRQ stack directly in the PC */
    ldmia    sp!, {pc}^

/*-----------------------------------------------------------------------------
 *- Function             : reset_handler
 *- Treatments           : Reset Interrupt Handler.
 *- Called Functions     : lowlevel_init
 *                         main
 *---------------------------------------------------------------------------*/
.section .text
reset_handler:
/*-----------------------------------------------------------------------------
 *- Low level Init is performed in a C function: lowlevel_init
 *- Init Stack Pointer to a valid memory area before calling lowlevel_init
 *
 * Put Stack Pointer on end of IRAM 1 and branches to lowlevel_init function.
 *---------------------------------------------------------------------------*/
/*- Temporary stack in internal RAM for Low Level Init execution */
_low_level_init:
    ldr      r2, =_litteral_pool_lowlevel_init /* Load r2 with the address of
    * _litteral_pool_lowlevel_init
    */
    ldmia    r2, {r0, r1} /* r0 = lowlevel_init and r1 = TOP_OF_MEMORY */
    mov      sp, r1 /* sp (Stack Pointer) = TOP_OF_MEMORY (end of IRAM 1) */
    mov      lr, pc /* lr (Link register) = pc (Program Counter) */
    bx       r0 /* Branch on C function (interworking) -- branch to
    * lowlevel_init
    */

/*-----------------------------------------------------------------------------
 *- Setup the stack for each mode
 *
 * Executes this code after returning from lowlevel_init fucntion.
 * Configures Abort Mode Stack + Interrupt Mode Stack + Supervisor Mode Stack,
 * reserves 3 blocks, one for each mode and they start at end of IRAM 1.
 *---------------------------------------------------------------------------*/
_stack_init:
    ldr      r2, =_litteral_pool_stack_init
    ldmia    r2, {r0, r1, r2} /* r0 = TOP_OF_MEMORY (end of IRAM 1);
    * r1 = ABT_STACK_SIZE;
    * r2 = IRQ_STACK_SIZE
    */

    /*- Set up Interrupt Mode and set IRQ Mode Stack */
    msr      CPSR_c, #ARM_MODE_IRQ | I_BIT | F_BIT /* Enter in Mode Interrupt
    * and disable IRQ (Interrupt) and FIQ (Fast Interrupt)
    */

    mov      sp, r0 /* sp (Interrupt Mode Stack Pointer) =
    * TOP_OF_MEMORY (end of IRAM 1)
    */

    /*- Set up Abort Mode and set ABT Mode Stack */
    msr      CPSR_c, #ARM_MODE_ABT | I_BIT | F_BIT /* Enter in Mode Abort
    * and disable IRQ (Interrupt) and FIQ (Fast Interrupt)
    */

    mov      sp, r0 /* sp (ABT Mode Stack Pointer) = TOP_OF_MEMORY
    * (end of IRAM 1)
    */

    /*- Set up Abort Mode and set ABT Mode Stack */
    msr      CPSR_c, #ARM_MODE_UND | I_BIT | F_BIT /* Enter in Mode Undefined
    * and disable IRQ (Interrupt) and FIQ (Fast Interrupt)
    */

    mov      sp, r0 /* sp (ABT Mode Stack Pointer) = TOP_OF_MEMORY
    * (end of IRAM 1)
    */

    /*- Set up Supervisor Mode and set Supervisor Mode Stack */
    msr      CPSR_c, #ARM_MODE_SVC | I_BIT | F_BIT
    mov      sp, r0

    sub      r0, r0, r2 /* Put on r0 the new address for next
    * Stack (Sys Mode)
    */

    /*- Set up Sys Mode and set Sys Mode Stack */
    msr      CPSR_c, #ARM_MODE_SYS | F_BIT /* Enter in Sys Mode
    * and enable IRQ (Interrupt) and disable FIQ (Fast Interrupt)
    */
    mov      sp, r0

/*-----------------------------------------------------------------------------
 *- Segments initialization
 *---------------------------------------------------------------------------*/
/* Copy the vectors section code from the LMA address to the LVA address */
_init_vectors:
    ldr      r1, =_litteral_pool_vectors /* Load r1 with the address of
    * _litteral_pool_vectors
    */
    ldmia   r1, {r2, r3, r4} /* r2 = DRAMORIG; r3 = _start_vectors_section;
    * r4 = _end_vectors_section;
    */
1:
    cmp     r3, r4 /* Compare r3 with r4 (r3 - r4) */
    ldrcc   r5, [r2], #4 /* if (_start_vectors_section < _end_vectors_section)
    * { r5 = [r2]; r2 = r2 + 4; }
    */
    strcc   r5, [r3], #4 /* if (_start_vectors_section < _end_vectors_section)
    * { [r3] = r5; r3 = r3 + 4; }
    */
    bcc     1b /* loop while (_start_vectors_section < _end_vectors_section) */

/* Clear the bss segment */
_init_bss:
    ldr      r2, =_litteral_pool_bss
    ldmia    r2, {r3, r4} /* r3 = _start_bss_section; r4 = _end_bss_section */
    mov      r2, #0 /* r2 = 0 */
1:
    cmp      r3, r4 /* Compare --> (_start_bss_section - _end_bss_section) */
    strcc    r2, [r3], #4  /* if (_start_bss_section < _end_bss_section) {
    * [r3] = 0; r3 = r3 + 4; }
    */
    bcc      1b /* loop while _start_bss_section < _end_bss_section */

/* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =stackend
    mov     r3, sp
    ldr     r2, =stackbegin
    ldr     r4, =0xdeadbeef

stackmunge:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     stackmunge

/*-----------------------------------------------------------------------------
 *- Branch to the main
 *---------------------------------------------------------------------------*/
_branch_main:
    ldr      r0, =main
    mov      lr, pc
    bx       r0

/*-----------------------------------------------------------------------------
 *- Litteral pools
 *---------------------------------------------------------------------------*/
_litteral_pool_lowlevel_init:
    .word    lowlevel_init
    .word    TOP_OF_MEMORY             /* Default SVC stack after power up */

_litteral_pool_stack_init:
    .word    TOP_OF_MEMORY             /* Top of the stack */
    .word    ABT_STACK_SIZE            /* ABT stack size */
    .word    IRQ_STACK_SIZE            /* IRQ stack size */

_litteral_pool_bss:
    .word    _start_bss_section
    .word    _end_bss_section

_litteral_pool_vectors:
#if defined(BOOTLOADER)
    .word   (DRAMORIG + DRAMSIZE - 0x100000) /* LMA address of vectors are at
    * end of DRAM minus 1MByte
    */
#else
    .word   DRAMORIG /* LMA address of vectors are at DRAMORIG */
#endif
    .word   _start_vectors_section
    .word   _end_vectors_section
