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
    b          newstart
    .space 4*16


/* Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */

newstart:
    msr    cpsr, #0xd3 /* enter supervisor mode, disable IRQ */

#if !defined(BOOTLOADER)
#if !defined(DEBUG)
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
    mov    r0,#0xd2
    msr    cpsr, r0
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    mov    r0,#0xd1
    msr    cpsr, r0
    ldr    sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    mov    r0,#0xd7
    msr    cpsr, r0
    ldr    sp, =irq_stack
    mov    r0,#0xdb
    msr    cpsr, r0
    ldr    sp, =irq_stack
    /* Switch to supervisor mode */
    mov    r0,#0xd3
    msr    cpsr, r0
    ldr    sp, =stackend
 
    /*remap memory as well as exception vectors*/
    /*for now this will be done in bootloader, especially
      if usb will be needed within the bootloader to load the 
      main firmware file. Interrupts will be needed for this
      (whether they be swi or irq)*/

    /* TTB Initialisation */
    ldr r3, =(0x80000000+64*1024*1024-TTB_SIZE)
    mov  r1, #0
    ldr r2, =(0x80000000+64*1024*1024)
ttbloop:
    str r1, [r3], #4
    cmp r3, r2
    bne ttbloop

    /* Set TTB base address */
    ldr r3, =(0x80000000+64*1024*1024-TTB_SIZE)
    mcr 15, 0 ,r3, cr2, cr0, 0
    /* Set all domains to manager status */
    mvn r3, #0
    mcr 15, 0, r3, cr3, cr0, 0

    /* Set page tables */

    /* Map each memory loc to itself, no cache */
    mov r1, #0 /* Physical address */
    ldr r3, =(0x80000000+64*1024*1024-TTB_SIZE) /* TTB pointer */
    ldr r4, =(0x80000000+64*1024*1024-TTB_SIZE+0x4000) /* End position */
maploop1:
    mov r2, r1
    orr r2, r2, #(1<<10)    /* superuser - r/w, user - no access */
    //orr r2, r2, #(0<<5)   /* domain 0th */
    orr r2, r2, #(1<<4)     /* should be "1" */
    orr r2, r2, #(1<<1)     /* Section signature */
    str r2, [r3], #4
    add r1, r1, #(1<<20)
    cmp r3, r4
    bne maploop1

    /* Map 0x80000000 -> 0x0, cached */
    mov r1, #0x80000000 /* Physical address */
    ldr r3, =(0x80000000+64*1024*1024-TTB_SIZE) /* TTB pointer */
    ldr r4, =(0x80000000+64*1024*1024-TTB_SIZE+256) /* End position */
maploop2:
    mov r2, r1
    orr r2, r2, #(1<<10)    /* superuser - r/w, user - no access */
    //orr r2, r2, #(0<<5)   /* domain 0th */
    orr r2, r2, #(1<<4)     /* should be "1" */
    orr r2, r2, #(1<<3)     /* cache flags */
    orr r2, r2, #(1<<2)     /* more cache stuff */
    orr r2, r2, #(1<<1)     /* Section signature */
    str r2, [r3], #4
    add r1, r1, #(1<<20)
    cmp r3, r4
    bne maploop2

    /* Enable MMU */
    mrc 15, 0, r3, cr1, cr0, 0
    tst r3, #0x4
    bleq clean_dcache
    tst r3, #0x1000
    bleq clean_dcache
    mov r0, #0

    mcr 15, 0, r0, cr8, cr7, 0 /* Invalidate TLB */
    mcr 15, 0, r0, cr7, cr7, 0 /* Invalidate icache and dcache */
    mrc 15, 0, r0, cr1, cr0, 0
    orr r0, r0, #1        /* enable mmu bit */
    orr r0, r0, #(1<<2)  /* enable dcache */
    orr r0, r0, #(1<<12) /* enable icache */
    mcr 15, 0, r0, cr1, cr0, 0
    nop
    nop
    nop
    nop

    mov r0,#0
    ldr r1,=_vectorstart
    mov r2,#0

lp: ldr r3,[r1]
    add r1,r1,#4
    str r3,[r0]
    add r0,r0,#4
    add r2,r2,#1
    cmp r2,#16
    bne lp
    bl main

.section .vectors,"aw"
_vectorstart:
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
    .word  irqz
    .word  fiqz

    .text
    .global irq
    .global fiq
    .global UIE

undef_instr_handler:
    mov    r0, lr
    mov    r1, #0
    b      UIE

software_int_handler:
reserved_handler:
    bl     irq_handler
    movs   pc, lr

prefetch_abort_handler:
    sub    r0, lr, #4
    mov    r1, #1
    b      UIE

data_abort_handler:
    sub    r0, lr, #8 
    mov    r1, #2
    b      UIE

/*not working....if we get here, let someone
know....*/
irqz: bl irq_handler
fiqz: bl fiq_handler

UIE:
    b UIE

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:


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
_vectorstart:
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

    .global irq
    .global fiq
    .global UIE

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

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

#endif /* BOOTLOADER */
