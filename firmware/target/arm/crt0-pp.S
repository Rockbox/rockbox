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
    .equ    PROC_ID,  0xc4000000
    .equ    COP_CTRL, 0xcf004058
    .equ    COP_STATUS, 0xcf004050
    .equ    IIS_CONFIG, 0xc0002500
    .equ    SLEEP,    0xca
    .equ    WAKE,     0xce
    .equ    SLEEPING, 0x4000
#else
    .equ    PROC_ID,  0x60000000
    .equ    COP_CTRL, 0x60007004
    .equ    COP_STATUS, 0x60007004
    .equ    IIS_CONFIG, 0x70002800
    .equ    SLEEP,    0x80000000
    .equ    WAKE,     0x0
    .equ    SLEEPING, 0x80000000
#endif

    msr    cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ */

#ifndef BOOTLOADER
    b      pad_skip
.space 50*4 /* (more than enough) space for exception vectors */
pad_skip:
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
    mov    r1, #0x3a00
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
    ldr    r0, =PROC_ID
    ldr    r0, [r0]
    and    r0, r0, #0xff
    cmp    r0, #0x55
    beq    1f
    
    /* put us (co-processor) to sleep */
    ldr    r4, =COP_CTRL
    mov    r3, #SLEEP
    str    r3, [r4]

    ldr    pc, =cop_init

1:

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
    /* TODO: the high part of the address is probably dependent on CONFIG_CPU.
       Since we tend to use ifdefs for each chipset target
       anyway, we might as well just hardcode it here.
     */
       
    /* get the high part of our execute address */
    ldr    r0, =0xff000000
    and    r8, pc, r0             @ r8 is used later

    /* Find out which processor we are */
    mov    r0, #PROC_ID
    ldr    r0, [r0]
    and    r0, r0, #0xff
    cmp    r0, #0x55
    beq    1f

    /* put us (co-processor) to sleep */
    ldr    r4, =COP_CTRL
    mov    r3, #SLEEP
    str    r3, [r4]
    ldr    pc, =cop_wake_start

cop_wake_start:
    /* jump the COP to startup */
    ldr    r0, =startup_loc
    ldr    pc, [r0]

1:
    
    /* get the high part of our execute address */
    ldr    r2, =0xffffff00
    and    r4, pc, r2

    /* Copy bootloader to safe area - 0x40000000 */
    mov    r5, #0x40000000
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

    /* execute the loader - this will load an image to 0x10000000 */
    bl     main

    /* Wake up the coprocessor before executing the firmware */

    /* save the startup address for the COP */
    ldr    r1, =startup_loc
    str    r0, [r1]

    /* make sure COP is sleeping */
    ldr    r4, =COP_STATUS
1:
    ldr    r3, [r4]
    ands   r3, r3, #SLEEPING
    beq    1b

    /* wake up COP */
    ldr    r4, =COP_CTRL
    mov    r3, #WAKE
    str    r3, [r4]

    /* jump to start location */
    mov    pc, r0

startup_loc:
    .word    0x0

.align 8    /* starts at 0x100 */
.global boot_table
boot_table:
    /* here comes the boot table, don't move its offset */
    .space 400

#else /* BOOTLOADER */
    
    /* Set up stack for IRQ mode */ 
    msr    cpsr_c, #0xd2
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    msr    cpsr_c, #0xd1
    ldr    sp, =fiq_stack
    /* We'll load the banked FIQ mode registers with useful values here.
       These values will be used in the FIQ handler in pcm_playback.c */
    ldr    r12, =IIS_CONFIG

    ldr    r11, =p

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

cop_init:
    ldr    sp, =cop_stackend
    mov    r3, sp
    ldr    r2, =cop_stackbegin
    ldr    r4, =0xdeadbeef
2:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    2b

    ldr    sp, =cop_stackend
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

fiq_handler:
    @ Branch straight to FIQ handler in pcm_playback.c. This also handles the
    @ the correct return sequence.
    ldr    pc, =fiq

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

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

#endif /* BOOTLOADER */ 
