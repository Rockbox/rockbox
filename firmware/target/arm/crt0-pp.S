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

/* PortalPlayer bootloader and startup code based on startup.s from the iPodLinux 
 * loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */
#if CONFIG_CPU == PP5002
    .equ    PROC_ID,     0xc4000000
    .equ    CPU_ICLR,    0xcf001028
    .equ    CPU_CTRL,    0xcf004054
    .equ    COP_ICLR,    0xcf001038
    .equ    COP_CTRL,    0xcf004058
    .equ    CPU_STATUS,  0xcf004050
    .equ    COP_STATUS,  0xcf004050
    .equ    IIS_CONFIG,  0xc0002500
    .equ    SLEEP,       0x000000ca
    .equ    WAKE,        0x000000ce
    .equ    CPUSLEEPING, 0x00008000
    .equ    COPSLEEPING, 0x00004000
    .equ    CACHE_CTRL,  0xcf004024
    .equ    MMAP_LOG,    0xf000f000 /* MMAP0 */
    .equ    MMAP_PHYS,   0xf000f004
#if MEM > 32
    .equ    MMAP_MASK,   0x00003c00
#else
    .equ    MMAP_MASK,   0x00003e00
#endif
    .equ    MMAP_FLAGS,  0x00003f84
#else
    .equ    PROC_ID,     0x60000000
    .equ    CPU_ICLR,    0x60004028
    .equ    CPU_CTRL,    0x60007000
    .equ    CPU_STATUS,  0x60007000
    .equ    COP_ICLR,    0x60004038
    .equ    COP_CTRL,    0x60007004
    .equ    COP_STATUS,  0x60007004
    .equ    IIS_CONFIG,  0x70002800
    .equ    SLEEP,       0x80000000
    .equ    WAKE,        0x00000000
    .equ    CPUSLEEPING, 0x80000000
    .equ    COPSLEEPING, 0x80000000    
    .equ    CACHE_CTRL,  0x6000c000
    .equ    MMAP_LOG,    0xf000f000 /* MMAP0 */
    .equ    MMAP_PHYS,   0xf000f004
#if MEM > 32
    .equ    MMAP_MASK,   0x00003c00
#else
    .equ    MMAP_MASK,   0x00003e00
#endif
    .equ    MMAP_FLAGS,  0x00000f84
#endif

    msr    cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */
    b      pad_skip

.space 64*4 /* (more than enough) space for exception vectors and mi4 magic */

pad_skip:
    /* Find out which processor we are - r0 should be preserved for the
     * duration of the init to avoid constant reloading of the processor ID.
     * For each stage, CPU proceeds first, then COP. 
     */
    ldr    r0, =PROC_ID
    ldrb   r0, [r0]

    /* We need to remap memory from wherever SDRAM is mapped natively, to
       base address 0, so we can put our exception vectors there. We don't
       want to do this remapping while executing from SDRAM, so we copy the
       remapping code to IRAM, then execute from there. Hence, the following
       code is compiled for address 0, but is currently executing at either
       0x28000000 or 0x10000000, depending on chipset version. Do not use any
       absolute addresses until remapping has been done. */

    /* Cores are stepped though the init in turn: CPU then COP. The the remap
       stage is completed by each core in turn and then the COP waits for the
       CPU to finish initializing its kernel where the CPU will wake the COP
       and wait for the COP to finish. This ensures no threading activity
       starts until it is safe. */
    cmp    r0, #0x55
    
    /* mask all interrupt sources before setting anything up */
    ldreq  r2, =CPU_ICLR
    ldrne  r2, =COP_ICLR    
    mvn    r1, #0
    str    r1, [r2]

    /* put us (co-processor) to sleep and wait for CPU to remap */
    ldrne  r2, =COP_CTRL
    movne  r1, #SLEEP
    strne  r1, [r2]
    nop
    nop
    nop

    /* wait for co-processor to sleep then CPU can begin its remapping */
    ldreq  r2, =COP_STATUS
1:
    ldreq  r1, [r2]
    tsteq  r1, #COPSLEEPING
    beq    1b

    /* disable cache and local interrupt vectors - it is really not desireable
       to have them enabled here */
    ldr    r2, =CACHE_CTRL
    mov    r1, #0
    str    r1, [r2]

    mov    r2, #0x40000000
    ldr    r3, =remap_start
    ldr    r4, =remap_end

    and    r6, pc, #0xff000000 /* adjust for execute address */
    orr    r3, r3, r6
    orr    r4, r4, r6

    /* copy the code to 0x40000000 */
1:
    ldr    r5, [r3], #4
    str    r5, [r2], #4
    cmp    r3, r4
    blo    1b

    ldr    r4, =MMAP_FLAGS
    orr    r4, r4, r6      /* adjust for execute address */
    ldr    r3, =MMAP_PHYS
    ldr    r2, =MMAP_MASK  /* ldr is more flexible */
    ldr    r1, =MMAP_LOG
    mov    pc, #0x40000000 

remap_start:
    str    r2, [r1]
    str    r4, [r3]
    ldr    r1, L_post_remap
    mov    pc, r1
L_post_remap:
    .word remap_end
remap_end:

    cmp    r0, #0x55
    ldr    r4, =COP_CTRL
    /* Wakeup co-processor to let it do remappings */
    moveq  r3, #WAKE
    /* Sleep us (co-processor) and wait for CPU to do kernel initialization */
    movne  r3, #SLEEP
    str    r3, [r4]
    nop
    nop
    nop

    /* Jump to co-processor init */
    ldrne  pc, =cop_init

cpu_init:
    /* Wait for COP to go to sleep before proceeding */
    ldr    r4, =COP_STATUS
1:
    ldr    r3, [r4]
    tst    r3, #COPSLEEPING
    beq    1b

    /* Copy exception handler code to address 0 */
    ldr    r2, =_vectorsstart
    ldr    r3, =_vectorsend
    ldr    r4, =_vectorscopy
1:
    cmp    r3, r2
    ldrhi  r5, [r4], #4
    strhi  r5, [r2], #4
    bhi    1b

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

    /* Initialise bss section to zero */
    ldr    r2, =_edata
    ldr    r3, =_end
    mov    r4, #0
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b

    /* Load stack munge value */    
    ldr    r4, =0xdeadbeef

    /* Set up some stack and munge it with 0xdeadbeef */
    ldr    r2, =stackbegin
    ldr    sp, =stackend
1:
    cmp    sp, r2
    strhi  r4, [r2], #4
    bhi    1b

#if NUM_CORES > 1
    /* Set up idle stack and munge it with 0xdeadbeef */
    ldr    r2, =cpu_idlestackbegin
    ldr    r3, =cpu_idlestackend
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b
#endif

    /* Set up stack for IRQ mode */ 
    msr    cpsr_c, #0x92 /* IRQ disabled, FIQ enabled */
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    msr    cpsr_c, #0xd1 /* IRQ/FIQ disabled */
    ldr    sp, =fiq_stack
    /* We'll load the banked FIQ mode registers with useful values here.
       These values will be used in the FIQ handler in pcm-pp.c */
    ldr    r10, =IIS_CONFIG

    ldr    r11, =dma_play_data

    /* Let abort and undefined modes use IRQ stack */
    msr    cpsr_c, #0xd7 /* IRQ/FIQ disabled */
    ldr    sp, =irq_stack
    msr    cpsr_c, #0xdb /* IRQ/FIQ disabled */
    ldr    sp, =irq_stack

    /* Switch back to supervisor mode */
    msr    cpsr_c, #0xd3

    /* Delay waking the COP until thread initialization is complete unless dual-core
       support is not enabled in which case the cop_main function does not perform
       any kernel or thread initialization. It's just a trivial sleep loop. */
#if NUM_CORES == 1
    ldr    r4, =COP_CTRL
    mov    r3, #WAKE
    str    r3, [r4]
#endif

    bl     main
    /* main() should never return */

cop_init:
#if NUM_CORES > 1
    /* Wait for CPU to go to sleep at the end of its kernel init */
    ldr    r4, =CPU_STATUS
1:
    ldr    r3, [r4]
    tst    r3, #CPUSLEEPING
    beq    1b
#endif

    /* Set up idle stack for COP and munge it with 0xdeadbeef */
    ldr    sp, =cop_idlestackend
    ldr    r2, =cop_idlestackbegin
    ldr    r4, =0xdeadbeef
2:
    cmp    sp, r2
    strhi  r4, [r2], #4
    bhi    2b

    /* Set up stack for IRQ mode */
    msr    cpsr_c, #0x92 /* IRQ disabled, FIQ enabled */
    ldr    sp, =cop_irq_stack
    /* Set up stack for FIQ mode */
    msr    cpsr_c, #0xd1 /* IRQ/FIQ disabled */
    ldr    sp, =cop_fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    msr    cpsr_c, #0xd7 /* IRQ/FIQ disabled */
    ldr    sp, =cop_irq_stack
    msr    cpsr_c, #0xdb /* IRQ/FIQ disabled */
    ldr    sp, =cop_irq_stack

    /* Switch back to supervisor mode */
    msr    cpsr_c, #0xd3
    
    /* Run cop_main() in apps/main.c */
    bl     cop_main

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

irq_handler:
#ifndef STUB
    stmfd  sp!, {r0-r3, r12, lr}
    bl     irq
    ldmfd  sp!, {r0-r3, r12, lr}
#endif
    subs   pc, lr, #4

#ifdef STUB
UIE:
    b UIE
#endif

/* Align stacks to cache line boundary */
    .balign 32
    
/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of COP IRQ stack */
    .space 256*4
cop_irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

/* We'll need this soon - just reserve the symbol */
#if 0
/* 256 words of COP FIQ stack */
    .space 256*4
#endif
cop_fiq_stack:
