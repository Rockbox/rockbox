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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */

#include "config.h"
#include "cpu.h"

    .section .init.text,"ax",%progbits

    .extern irq
    .extern fiq
    .extern UIE
    .extern main

    .global    start

/* Telechips firmware files start with a 32-byte header, as part of the code. */

start:
#ifdef TCCBOOT
    /* Add -DTCCBOOT to EXTRA_DEFINES in the bootloader Makefile to
       enable building the bootloader to be appended to the end of the
       original firmware, dual-booting based on a key-press.

       The following two values are filled in by mktccboot.
     */
    .word  0              /* Saved entrypoint of original firmware*/
    .word  0              /* Location in RAM of the start of our bootloader */
#else
//    ldr    pc, =start_loc    /* jump to the main entry point  */
    b       start_loc

    .word  0xffff0601        /* Unknown magic */
    .word  0x3a726556        /* "Ver:" */
    .word  0x31373030        /* "0071" */
    .word  0                 /* First CRC32 */
    .word  0                 /* Unknown - always 0 */
    .word  0                 /* Second CRC32 */
    .word  0                 /* length of firmware file */

#ifdef LOGIK_DAX
    /* Some original firmwares have 0x40 bytes of zeroes here - we
       don't know why, but err on the side of caution and include it
       here. */
    .space 0x40
#endif
#endif

start_loc:

#ifdef BOOTLOADER

/*
  If we are appended to the OF (i.e. dual-booting), do a simple GPIO
  button check, and branch to the OF's entry point (saved by mktccboot)
  if not active 
*/

#ifdef TCCBOOT
    mov    r0, #0x80000000
#if defined(LOGIK_DAX)
    ldr    r0, [r0, #0x300]     /* Hold button is GPIO A, pin 0x2 */
    tst    r0, #0x2
#elif defined(SANSA_M200)
    ldr    r0, [r0, #0x310]     /* Hold button is GPIO B, pin 0x200 */
    tst    r0, #0x200
#elif defined(IAUDIO_7)
    ldr    r0, [r0, #0x300]     /* Hold button is !GPIO A, pin 0x2 */
    tst  r0, #0x2
#else
    #error No bootup key detection implemented for this target
#endif

    ldrne  pc, [pc, #-28]     /* Jump to OF if HOLD button not pressed */
#endif /* TCCBOOT */

/* We are now definitely executing the bootloader, so we relocate to the
   linked address (see boot.lds) - 1MB from the end of DRAM.
*/

#ifdef TCCBOOT
    ldr    r0, [pc, #-28]     /* mktccboot fills in the load address */
#else
    mov    r0, #0x20000000    /* Otherwise, load address is the start of DRAM */
#endif
    mov    r1, #0x20000000    /* Destination: 1MB from end of DRAM */
    add    r1, r1, #((MEM - 1) * 0x100000)

    ldr    r2, =_dataend
1:
    cmp    r2, r1
    ldrhi  r3, [r0], #4
    strhi  r3, [r1], #4
    bhi    1b

    ldr    pc, =copied_start   /* jump to the relocated start_loc:  */
copied_start:
#endif /* BOOTLOADER */

    /* Set up stack for IRQ mode */ 
    mov    r0,#0xd2
    msr    cpsr, r0
    ldr    sp, =irq_stack

    /* Set up stack for FIQ mode */ 
    mov    r0,#0xd1
    msr    cpsr, r0
    ldr    sp, =fiq_stack

#ifndef BOOTLOADER
    /* Load the banked FIQ mode registers with useful values here.
       These values will be used in the FIQ handler in pcm-telechips.c */
    .equ   DADO_BASE,  0x80000020

    ldr    r10, =DADO_BASE
    ldr    r11, =dma_play_data
#endif

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
    
    /* Copy exception handler code to address 0 */
    mov    r2, #0x0
    ldr    r3, =vectors_start
    ldr    r4, =vectors_end
1:
    cmp    r4, r3
    ldrhi  r5, [r3], #4
    strhi  r5, [r2], #4
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
    
    bl     main
    /* main() should never return */

/* Exception handlers. Will be copied to address 0 after memory remapping */
vectors_start:
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
vectors_end:

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

irq_handler:
    stmfd  sp!, {r0-r3, r12, lr}
    bl     irq
    ldmfd  sp!, {r0-r3, r12, lr}
    subs   pc, lr, #4

/* Align stacks to cache line boundary */
    .balign 16
    
/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

