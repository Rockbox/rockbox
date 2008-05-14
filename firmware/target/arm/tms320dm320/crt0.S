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
 * Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
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
    .equ    INTC_IRQ0,    0x00030508
    .equ    INTC_IRQ1,    0x0003050A
    .equ    INTC_IRQ2,    0x0003050C
    .equ    INTC_FIQ0,    0x00030500
    .equ    INTC_FIQ1,    0x00030502
    .equ    INTC_FIQ2,    0x00030504
    .equ    INTC_EINT0,   0x00030528
    .equ    INTC_EINT1,   0x0003052A
    .equ    INTC_EINT2,   0x0003052C
    .equ    INTC_FISEL0,  0x00030520
    .equ    INTC_FISEL1,  0x00030522
    .equ    INTC_FISEL2,  0x00030524
    .equ    INTC_MASK,    0xFFFFFFFF

    msr    cpsr, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

    /* Copy exception handler code to address 0 */
    ldr    r2, =_vectorsstart
    ldr    r3, =_vectorsend
    ldr    r4, =_vectorscopy
1:
    cmp    r3, r2
    ldrhi  r5, [r4], #4
    strhi  r5, [r2], #4
    bhi    1b

    /* Disable data and instruction cache, high vectors (at 0xffff0000 instead of 0x00000000) */
    mrc    p15, 0, r0, c1, c0, 0
    /* clear bits 13, 9:8 (--VI --RS) */
    bic    r0, r0, #0x00003300
    /* clear bits 7, 2:0 (B--- -C-M) */
    bic    r0, r0, #0x00000085
    /* make sure bit 2 (A) Align is set */
    orr    r0, r0, #0x00000002
    mcr    p15, 0, r0, c1, c0, 0

#if 0
    /* mask interrupts */
    ldr    r1, =INTC_MASK
    ldr    r2, =INTC_IRQ0
    strh   r1, [r2]
    ldr    r2, =INTC_IRQ1
    strh   r1, [r2]
    ldr    r2, =INTC_IRQ2
    strh   r1, [r2]
    ldr    r2, =INTC_FIQ0
    strh   r1, [r2]
    ldr    r2, =INTC_FIQ1
    strh   r1, [r2]
    ldr    r2, =INTC_FIQ2
    strh   r1, [r2]

    mov    r1, #0
    ldr    r2, =INTC_EINT0
    strh   r1, [r2]
    ldr    r2, =INTC_EINT1
    strh   r1, [r2]
    ldr    r2, =INTC_EINT2
    strh   r1, [r2]
    ldr    r2, =INTC_FISEL0
    strh   r1, [r2]
    ldr    r2, =INTC_FISEL1
    strh   r1, [r2]
    ldr    r2, =INTC_FISEL2
    strh   r1, [r2]
#endif

#if !defined(BOOTLOADER) && !defined(STUB)
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
#endif /* !BOOTLOADER,!STUB */

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
    ldr    r3, =stackend
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

    /* Let abort and undefined modes use IRQ stack */
    msr    cpsr_c, #0xd7 /* IRQ/FIQ disabled */
    ldr    sp, =irq_stack
    msr    cpsr_c, #0xdb /* IRQ/FIQ disabled */
    ldr    sp, =irq_stack

    /* Switch to supervisor mode (no IRQ) */
    msr    cpsr_c, #0xd3
    ldr    sp, =stackend
    
#if defined(BOOTLOADER) && !defined(CREATIVE_ZVx)
    /* get the high part of our execute address */
    ldr    r2, =0xffffff00
    and    r4, pc, r2

    /* Copy bootloader to safe area - 0x01900000 */
    mov    r5, #0x01900000
    ldr    r6, = _dataend
    sub    r0, r6, r5       /* length of loader */
    add    r0, r4, r0     /* r0 points to start of loader */
1:
    cmp    r5, r6
    ldrcc  r2, [r4], #4
    strcc  r2, [r5], #4
    bcc    1b

    ldr    pc, =start_loc    /* jump to the relocated start_loc:  */

#endif

start_loc:
     bl main
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

#if defined(STUB) || defined(BOOTLOADER)
UIE:
    b UIE
#endif

/* Align stacks to cache line boundary */
    .balign 16

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:
