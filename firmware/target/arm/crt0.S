/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/* Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */

#if CONFIG_CPU == IMX31L
    mrs r0, cpsr
    bic r0, r0, #0x100
    add r0,r0,#0xD3
    msr cpsr_c, r0
#else
    msr    cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ */
#endif  

#ifndef BOOTLOADER

#ifndef DEBUG
    /* Copy exception handler code to address 0 */
    ldr    r2, =_vectorsstart
    ldr    r3, =_vectorsend
    ldr    r4, =_vectorscopy
1:
    cmp    r3, r2
    ldrhi  r5, [r4], #4
    strhi  r5, [r2], #4
    bhi    1b
#else
    ldr    r1, =vectors
    ldr    r0, =irq_handler
    str    r0, [r1, #24]
    ldr    r0, =fiq_handler
    str    r0, [r1, #28]
#endif
    
#ifndef STUB
    /* Zero out IBSS */
    ldr    r2, =_iedata
    ldr    r3, =_iend
    mov    r4, #0
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b

    /* Copy the IRAM */
    ldr    r2, =_iramcopy
    ldr    r3, =_iramstart
    ldr    r4, =_iramend
1:
    cmp    r4, r3
    ldrhi  r5, [r2], #4
    strhi  r5, [r3], #4
    bhi    1b
#endif /* !STUB */
#endif /* !BOOTLOADER */

    /* Initialise bss section to zero */
    ldr    r2, =_edata
    ldr    r3, =_end
    mov    r4, #0
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b
    
    /* Set up some stack and munge it with 0xdeadbeef */
    ldr    sp, =stackend
    mov    r3, sp
    ldr    r2, =stackbegin
    ldr    r4, =0xdeadbeef
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b

#ifdef BOOTLOADER
/* Code for ARM bootloader targets other than iPod go here */

#if CONFIG_CPU == S3C2440
    /* get the high part of our execute address */
    ldr    r2, =0xffffff00
    and    r4, pc, r2

    /* Copy bootloader to safe area - 0x31000000 */
    mov    r5, #0x30000000
    add    r5, r5, #0x1000000
    ldr    r6, = _dataend
    sub    r0, r6, r5       /* length of loader */
    add    r0, r4, r0     /* r0 points to start of loader */
1:
    cmp    r5, r6
    ldrcc  r2, [r4], #4
    strcc  r2, [r5], #4
    bcc    1b

    ldr    pc, =start_loc    /* jump to the relocated start_loc:  */
    
start_loc:
     bl main

#elif CONFIG_CPU == IMX31L
    mov r0, #0
    mcr 15, 0, r0, c7, c7, 0        /* invalidate I cache and D cache */
    mcr 15, 0, r0, c8, c7, 0        /* invalidate TLBs */
    mcr 15, 0, r0, c7, c10, 4        /* Drain the write buffer */

    /* Also setup the Peripheral Port Remap register inside the core */
    ldr r0, =0x40000015        /* start from AIPS 2GB region */
    mcr p15, 0, r0, c15, c2, 4

    /*** L2 Cache setup/invalidation/disable ***/
    /* Disable L2 cache first */
    ldr r0, =L2CC_BASE_ADDR
    ldr r2, [r0, #L2_CACHE_CTL_REG]
    bic r2, r2, #0x1
    str r2, [r0, #L2_CACHE_CTL_REG]


    /*
     * Configure L2 Cache:
     * - 128k size(16k way)
     * - 8-way associativity
     * - 0 ws TAG/VALID/DIRTY
     * - 4 ws DATA R/W
     */
    ldr r1, [r0, #L2_CACHE_AUX_CTL_REG]
    and r1, r1, #0xFE000000
    ldr r2, =0x00030024
    orr r1, r1, r2
    str r1, [r0, #L2_CACHE_AUX_CTL_REG]

    /* Invalidate L2 */
    ldr r1, =0x000000FF
    str r1, [r0, #L2_CACHE_INV_WAY_REG]
L2_loop:
    /* Poll Invalidate By Way register */
    ldr r2, [r0, #L2_CACHE_INV_WAY_REG]
    cmp r2, #0
    bne L2_loop
    /*** End of L2 operations ***/
	
	/* Set up stack for IRQ mode */ 
    mrs    r0,cpsr
    orr    r0,r0,#0xd2
    msr    cpsr_c,r0
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    mrs    r0,cpsr
    orr    r0,r0,#0xd1
    msr    cpsr_c,r0
    ldr    sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    mrs    r0,cpsr
    orr    r0,r0,#0xd7
    msr    cpsr_c,r0
    ldr    sp, =irq_stack
    mrs    r0,cpsr
    orr    r0,r0,#0xdb
    msr    cpsr_c,r0
    ldr    sp, =irq_stack
    /* Switch to supervisor mode */
    mrs    r0,cpsr
    orr    r0,r0,#0xdb
    msr    cpsr_c,r0
    ldr    sp, =stackend
	
    bl main
    
    /* 256 words of IRQ stack */
    .space 256*4
	irq_stack:

	/* 256 words of FIQ stack */
    .space 256*4
	fiq_stack:

#endif

#else /* BOOTLOADER */
    
    /* Set up stack for IRQ mode */ 
    msr    cpsr_c, #0xd2
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    msr    cpsr_c, #0xd1
    ldr    sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    msr    cpsr_c, #0xd7
    ldr    sp, =irq_stack
    msr    cpsr_c, #0xdb
    ldr    sp, =irq_stack
    /* Switch to supervisor mode */
    msr    cpsr_c, #0xd3
    ldr    sp, =stackend
    bl     main
    /* main() should never return */
   
/* Exception handlers. Will be copied to address 0 after memory remapping */
    .section .vectors,"aw"
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]
    ldr    pc, [pc, #24]

    /* Exception vectors */
    .global vectors
vectors:
    .word  start 
    .word  undef_instr_handler
    .word  software_int_handler
    .word  prefetch_abort_handler
    .word  data_abort_handler
    .word  reserved_handler
    .word  irq_handler
    .word  fiq_handler

    .text

#ifndef STUB
    .global irq
    .global fiq
    .global UIE
#endif

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

#ifdef STUB
UIE:
    b UIE
#endif

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

#endif /* BOOTLOADER */ 
