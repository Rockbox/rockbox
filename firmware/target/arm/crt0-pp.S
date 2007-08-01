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
    .equ    PROC_ID,    0xc4000000
    .equ    CPU_ICLR,   0xcf001028
    .equ    COP_ICLR,   0xcf001038
    .equ    COP_CTRL,   0xcf004058
    .equ    COP_STATUS, 0xcf004050
    .equ    IIS_CONFIG, 0xc0002500
    .equ    SLEEP,      0x000000ca
    .equ    WAKE,       0x000000ce
    .equ    SLEEPING,   0x00004000
#else
    .equ    PROC_ID,    0x60000000
    .equ    CPU_ICLR,   0x60004028
    .equ    COP_ICLR,   0x60004038
    .equ    COP_CTRL,   0x60007004
    .equ    COP_STATUS, 0x60007004
    .equ    IIS_CONFIG, 0x70002800
    .equ    SLEEP,      0x80000000
    .equ    WAKE,       0x00000000
    .equ    SLEEPING,   0x80000000
    .equ    CACHE_CTRL, 0x6000c000
#endif

    msr    cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */
    b      pad_skip

.space 60*4 /* (more than enough) space for exception vectors and mi4 magic */

pad_skip:
#ifdef SANSA_E200
    /* On the Sansa, copying the vectors fails if the cache is initialised */
    ldr    r1, =CACHE_CTRL
    mov    r2, #0x0
    str    r2, [r1]
#endif
    /* We need to remap memory from wherever SDRAM is mapped natively, to
       base address 0, so we can put our exception vectors there. We don't
       want to do this remapping while executing from SDRAM, so we copy the
       remapping code to IRAM, then execute from there. Hence, the following
       code is compiled for address 0, but is currently executing at either
       0x28000000 or 0x10000000, depending on chipset version. Do not use any
       absolute addresses until remapping has been done. */
    ldr    r1, =0x40000000
    ldr    r2, =remap_start
    ldr    r3, =remap_end

    and    r5, pc, #0xff000000 /* adjust for execute address */
    orr    r2, r2, r5
    orr    r3, r3, r5

    /* copy the code to 0x40000000 */
1:
    ldr    r4, [r2], #4
    str    r4, [r1], #4
    cmp    r2, r3
    ble    1b

    ldr    r3, =0x3f84     /* r3 and r1 values here are magic, don't touch */
    orr    r3, r3, r5      /* adjust for execute address */
    ldr    r2, =0xf000f014
#if MEM > 32
    mov    r1, #0x7400     /* r1 appears to indicate how much memory (not in
                              bytes) is remapped */
#else
    mov    r1, #0x3a00
#endif
    ldr    r0, =0xf000f010
    mov    pc, #0x40000000 

remap_start:
    str    r1, [r0]
    str    r3, [r2]
    ldr    r0, L_post_remap
    mov    pc, r0
L_post_remap: .word remap_end
remap_end:

    /* After doing the remapping, send the COP to sleep.
       On wakeup it will go to cop_init */

    /* Find out which processor we are */
    ldr    r0, =PROC_ID
    ldrb   r0, [r0]
    cmp    r0, #0x55

    /* Mask all interrupt sources before setting up modes */
    ldreq  r0, =CPU_ICLR
    ldrne  r0, =COP_ICLR    
    mvn    r1, #1
    str    r1, [r0]    
    
    /* put us (co-processor) to sleep */
    ldrne  r4, =COP_CTRL
    movne  r3, #SLEEP
    strne  r3, [r4]

    ldrne  pc, =cop_init

cpu_init:
    /* Wait for COP to be sleeping */
    ldr    r4, =COP_STATUS
1:
    ldr    r3, [r4]
    tst    r3, #SLEEPING
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
    
    /* Set up some stack and munge it with 0xdeadbeef */
    ldr    sp, =stackend
    mov    r3, sp
    ldr    r2, =stackbegin
    ldr    r4, =0xdeadbeef
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b
    
    /* Set up stack for IRQ mode */ 
    msr    cpsr_c, #0x92 /* IRQ disabled, FIQ enabled */
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    msr    cpsr_c, #0xd1 /* IRQ/FIQ disabled */
    ldr    sp, =fiq_stack
    /* We'll load the banked FIQ mode registers with useful values here.
       These values will be used in the FIQ handler in pcm_playback.c */
    ldr    r12, =IIS_CONFIG

    ldr    r11, =p

    /* Let abort and undefined modes use IRQ stack */
    msr    cpsr_c, #0xd7 /* IRQ/FIQ disabled */
    ldr    sp, =irq_stack
    msr    cpsr_c, #0xdb /* IRQ/FIQ disabled */
    ldr    sp, =irq_stack
    /* Switch to supervisor mode */
    msr    cpsr_c, #0xd3
    ldr    sp, =stackend
    bl     main
    /* main() should never return */

cop_init:
#if CONFIG_CPU != PP5002
    /* COP: Invalidate cache */
    ldr    r0, =0xf000f044
    ldr    r1, [r0]
    orr    r1, r1, #0x6
    str    r1, [r0]

    ldr    r0, =CACHE_CTRL
1:
    ldr    r1, [r0]
    tst    r1, #0x8000
    bne    1b
#endif

    /* Setup stack for COP */
    ldr    sp, =cop_stackend
    mov    r3, sp
    ldr    r2, =cop_stackbegin
    ldr    r4, =0xdeadbeef
2:
    cmp    r3, r2
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

    ldr    sp, =cop_stackend
    
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
    .word  0 /* fiq handler set in pcm driver */

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
