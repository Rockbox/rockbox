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

    .global    start

/* Telechips firmware files start with a 32-byte header, as part of the code. */

start:
#ifdef TCCBOOT
#ifdef BOOTLOADER
    /* Add -DTCCBOOT to EXTRA_DEFINES in the bootloader Makefile to
       enable building the bootloader to be appended to the end of the
       original firmware, dual-booting based on a key-press.

       The following two values are filled in by mktccboot.
     */
    .word  0              /* Saved entrypoint of original firmware*/
    .word  0              /* Location in RAM of the start of our bootloader */
#endif
#else
    ldr    pc, =start_loc    /* jump to the main entry point  */

    .word  0xffff0601        /* Unknown magic */
    .word  0x3a726556        /* "Ver:" */
    .word  0x31373030        /* "0071" */
    .word  0                 /* First CRC32 */
    .word  0                 /* Unknown - always 0 */
    .word  0                 /* Second CRC32 */
    .word  0                 /* length of firmware file */

#ifdef COWON_D2
    /* Some original firmwares have 0x40 bytes of zeroes here - we
       don't know why, but err on the side of caution and include it
       here. */
    .space 0x40
#endif
#endif

start_loc:

#ifdef BOOTLOADER
#ifdef TCCBOOT
#ifdef COWON_D2
    ldr    r0, =0xf005a000
    ldr    r0, [r0, #0x20] /* Read GPIO A */
    tst    r0, #0x8
    ldreq  pc, [pc, #-28]  /* Jump to original firmware if HOLD pressed */
    ldr    r0, =0xf005a000
    ldr    r0, [r0, #0x60] /* Read GPIO C */
    tst    r0, #0x4000000
    ldreq  pc, [pc, #-44]  /* Jump to original firmware if usb connected */
#else
    #error No bootup key detection implemented for this target
#endif

    /* Copy bootloader to safe area - 0x21F00000 (end of DRAM) */
    /* TODO: Adjust this for other targets - DRAM + DRAMSIZE - 0x100000 */
    ldr    r0, [pc, #-44]
    mov    r1, #0x22000000
    sub    r1, r1, #0x100000
    ldr    r2, =_dataend
1:
    cmp    r2, r1
    ldrhi  r3, [r0], #4
    strhi  r3, [r1], #4
    bhi    1b

    ldr    pc, =copied_start   /* jump to the relocated start_loc:  */

copied_start:
#endif /* TCCBOOT */
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
       These values will be used in the FIQ handler in pcm-tcc780x.c */
    .equ   DADO_BASE,  0xF0059020
    
    ldr    r10, =DADO_BASE
    ldr    r11, =dma_play_data
#endif

    /* Let svc, abort and undefined modes use irq stack */
    msr    cpsr, #0xd3
    ldr    sp, =irq_stack
    msr    cpsr, #0xd7
    ldr    sp, =irq_stack
    msr    cpsr, #0xdb
    ldr    sp, =irq_stack

    /* Switch to sys mode */
    msr    cpsr, #0xdf
    ldr    sp, =stackend

    /* Enable MMU & caches. At present this is just doing what the OF does.
       Ensure TCMs are enabled before copying the exception vectors to 0x0. */

    mov     r1, #0xf7000000  /* Virtual MMU Table base */

    ldr     r0, =0x1fe0c     /* Region 0: 0x00000000-0xffffffff (4Gb) */
    str     r0, [r1]         /* AP: 3 EN: 1 DO: 0 CACHE_ALL */

    ldr     r0, =0x2801ae24  /* Region 1: 0x28000000-0x2fffffff (128Mb) */
    str     r0, [r1,#4]      /* AP: 3 EN: 1 DO: 1 BUFFERED */

    ldr     r0, =0x13e44     /* Region 2: 0x00000000-0x000fffff (1Mb) */
    str     r0, [r1,#8]      /* AP: 3 EN: 1 DO: 2 BUFFERED */

    ldr     r0, =0x4001ce60  /* Region 3: 0x40000000-0x5fffffff (512Mb) */
    str     r0, [r1,#0xc]    /* AP: 3 EN: 1 DO: 3 CACHE_NONE */

    ldr     r0, =0x6001be80  /* Region 4: 0x60000000-0x6fffffff (256Mb) */
    str     r0, [r1,#0x10]   /* AP: 3 EN: 1 DO: 4 CACHE_NONE */

    ldr     r0, =0x3801aea4  /* Region 5: 0x38000000-0x3fffffff (128Mb) */
    str     r0, [r1,#0x14]   /* AP: 3 EN: 1 DO: 5 BUFFERED */

    ldr     r0, =0x8001eec0  /* Region 6: 0x80000000-0xffffffff (2Gb) */
    str     r0, [r1,#0x18]   /* AP: 3 EN: 1 DO: 6 CACHE_NONE */

    ldr     r0, =0x1001aeec  /* Region 7: 0x10000000-0x17ffffff (128Mb) */
    str     r0, [r1,#0x1c]   /* AP: 3 EN: 1 DO: 7 CACHE_ALL */

    add     r1, r1, #0x8000
    mcr     p15, 0, r1, c2, c0, 0  /* Set TTBR = TABBASE (Virtual TLB) */

    ldr     r0, =0x55555555
    mcr     p15, 0, r0, c3, c0, 0  /* Domain access d0-d15 = 'client' */
    
    ldr     r0, =0xa0000011
    mcr     p15, 0, r0, c9, c1, 0  /* Data TCM: 0xA0000000-0xA00001fff (8Kb) */
    mov     r0, #0xd
    mcr     p15, 0, r0, c9, c1, 1  /* Instr. TCM: 0x00000000-0x00000fff (4Kb) */

    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0  /* Invalidate Icache */
    ldr     r2, =0x5507d
    mcr     p15, 0, r2, c1, c0, 0  /* Enable MMU, I & D caches */
    mcr     p15, 0, r0, c7, c6, 0  /* Invalidate Dcache */
    mcr     p15, 0, r1, c8, c7, 0  /* Invalidate TLB */

#ifndef STUB
    
    /* Copy exception handler code to address 0 */
    mov    r2, #0x0
    ldr    r3, =vectors_start
    ldr    r4, =vectors_end
1:
    cmp    r4, r3
    ldrhi  r5, [r3], #4
    strhi  r5, [r2], #4
    bhi    1b

#ifndef BOOTLOADER
    /* Copy the IRAM (SRAM) */
    ldr    r2, =_iramcopy
    ldr    r3, =_iramstart
    ldr    r4, =_iramend
1:
    cmp    r4, r3
    ldrhi  r5, [r2], #4
    strhi  r5, [r3], #4
    bhi    1b
    
    /* Zero out IBSS */
    ldr    r2, =_iedata
    ldr    r3, =_iend
    mov    r4, #0
1:
    cmp    r3, r2
    strhi  r4, [r2], #4
    bhi    1b

    /* Copy the ITCM */
    ldr    r2, =_itcmcopy
    ldr    r3, =_itcmstart
    ldr    r4, =_itcmend
1:
    cmp    r4, r3
    ldrhi  r5, [r2], #4
    strhi  r5, [r3], #4
    bhi    1b

    /* Copy the DTCM */
    ldr    r2, =_dtcmcopy
    ldr    r3, =_dtcmstart
    ldr    r4, =_dtcmend
1:
    cmp    r4, r3
    ldrhi  r5, [r2], #4
    strhi  r5, [r3], #4
    bhi    1b
#endif /* !BOOTLOADER */
#endif /* !STUB */

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

#ifdef BOOTLOADER
fiq_handler:
    subs    pc, lr, #4
#endif
    
#if defined(STUB)
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
