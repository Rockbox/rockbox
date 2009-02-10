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
#include "config.h"
#include "cpu.h"

    .section .init.text,"ax",%progbits

    .global    start
start:
    /* Exception vectors */
    b       newstart
    b       undef_instr_handler
    b       software_int_handler
    b       prefetch_abort_handler
    b       data_abort_handler
    b       reserved_handler
    b       irq_handler
    b       fiq_handler
    .balign 0x40, 0x6B

/* Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */

/* Bootloader:
 * Initially this code is running at VA 0x8a000000 (PA 0x82000000).
 * The mapping stub is copied to IRAM (0x1fffc000), sets up the MMU and
 * jumps into the final VA remapping starting at 0x02000000 (32MB).
 *
 * Firmware:
 * This code will be running from VA 0x00000000 (PA 0x80000000) and perform
 * similar steps to the bootloader code.
 */
newstart:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

    adr     r2, remap_start    /* Load PC-relative labels (relocatable) */
    adr     r3, remap_end
    ldr     r5, =TTB_BASE_ADDR /* TTB pointer */
    ldr     r6, =IRAM_BASE_ADDR
    mov     r1, r6

1:  /* Copy remapping stub to IRAM */
    cmp     r3, r2
    ldrhi   r4, [r2], #4
    strhi   r4, [r1], #4
    bhi     1b

    mov     pc, r6

    /* Remapping stub. No absolute addresses may be used until after the
     * remapping is complete. */
remap_start:
    mrc     p15, 0, r3, c1, c0, 0  /* perform writeback if D cache is enabled */
    tst     r3, #(1 <<  2)         /* dcache? */
    tsteq   r3, #(1 << 12)         /* or icache? */
    mov     r0, #0
    mcrne   p15, 0, r0, c7, c10, 0 /* clean dcache */
    mcrne   p15, 0, r0, c7, c7, 0  /* invalidate I cache and D cache */
    mcr     p15, 0, r0, c8, c7, 0  /* invalidate TLBs */
    mcr     p15, 0, r0, c7, c10, 4 /* Drain the write buffer */
    
    mcr     p15, 0, r0, c13, c0, 0
    mcr     p15, 0, r0, c13, c0, 1

    /* Also setup the Peripheral Port Remap register inside the core */
    mov     r0, #0x40000000        /* start from AIPS 2GB region */
    add     r0, r0, #0x15
    mcr     p15, 0, r0, c15, c2, 4

    /*** L2 Cache setup/invalidation/disable ***/
    /* Disable L2 cache first */
    mov     r0, #L2CC_BASE_ADDR
    mov     r1, #0
    str     r1, [r0, #L2_CACHE_CTL_REG]

    /* Disble L1 caches and memory manager */
    bic     r3, r3, #(1 << 12)    /* L1 I-cache disabled */
    bic     r3, r3, #((1 << 2) |  /* L1 D-cache disabled */ \
                      (1 << 0))   /* MMU disabled */
    mcr     p15, 0, r3, c1, c0, 0
    
    /*
     * Configure L2 Cache:
     * - 128k size(16k way)
     * - 8-way associativity
     * - 0 ws TAG/VALID/DIRTY
     * - 4 ws DATA R/W
     */
    mov     r1, #0x00130000
    orr     r1, r1, #0x24
    str     r1, [r0, #L2_CACHE_AUX_CTL_REG]

    /* Invalidate L2 */
    mov     r1, #0x000000FF
    str     r1, [r0, #L2_CACHE_INV_WAY_REG]
1:
    /* Poll Invalidate By Way register */
    ldr     r1, [r0, #L2_CACHE_INV_WAY_REG]
    cmp     r1, #0
    bne     1b

    /*** End of L2 operations ***/

    /* TTB Initialisation */

    /* Set TTB base address */
    mcr     p15, 0, r5, c2, c0, 0

    /* Set all domains to manager status */
    mvn     r0, #0
    mcr     p15, 0, r0, c3, c0, 0

    /* Set page tables */

    /* Map each memory loc to itself, no cache */
                                   /* Physical address = 0x0 */
    mov     r1,      #(1 << 10)    /* superuser - r/w, user - no access */
    orr     r1, r1, #((0 <<  5)  | /* domain 0th */ \
                      (1 <<  4)  | /* should be "1" */ \
                      (1 <<  1))   /* Section signature */
    mov     r2, r5
    add     r3, r5, #TTB_SIZE   /* End position */
1:
    str     r1, [r2], #4
    add     r1, r1, #(1 << 20)  /* Next MB */
    cmp     r2, r3
    blo     1b

    bic     r1, r1, #0x0ff00000 /* Back up */

    /* Map 0x80000000 -> 0x0, cached */
    mov     r2, r5               /* TTB pointer */
    add     r3, r5, #64*4        /* End position */
    orr     r1, r1, #0x80000000  /* Physical address */
    orr     r1, r1, #((1 << 3) | /* cache flag */ \
                      (1 << 2))  /* buffer flag */
1:
    str     r1, [r2], #4
    add     r1, r1, #(1 << 20)
    cmp     r2, r3
    blo     1b

    /* Map device section 0x83f00000 to 0x03f00000 - buffered, not cached */
    bic     r1, r1, #0x0ff00000
    orr     r1, r1, #0x03f00000
    bic     r1, r1, #(1 << 3)
    add     r2, r5, #63*4
    str     r1, [r2]
    
    /* Enable MMU */
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0 /* Invalidate TLB */
    mcr     p15, 0, r0, c7, c7, 0 /* Invalidate icache and dcache */
    
    /* Auxilliary control register */
    mrc     p15, 0, r0, c1, c0, 1
    bic     r0, r0, #((1 << 6) | /* Restrict cache size OFF */ \
                      (1 << 5) | /* Enable block tranfer cache operations */ \
                      (1 << 4) | /* Clean+Invalidate cache operation ON */ \
                      (1 << 3))  /* Round-robin micro TLB replacement */
    orr     r0, r0, #((1 << 2) | /* Static branch prediction ON */ \
                      (1 << 1) | /* Dynamic branch prediction ON */ \
                      (1 << 0))  /* Return stack enabled */
    mcr     p15, 0, r0, c1, c0, 1

    /* Control register */
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #((1 << 29) | /* AF by AP disabled */ \
                      (1 << 28) | /* TEX remap disabled */ \
                      (1 << 23))  /* Sub AP bits enabled (compatible) */
    bic     r0, r0, #((1 << 21) | /* All performance features enabled */ \
                      (1 << 15))  /* Loads to PC set T bit */
    bic     r0, r0, #((1 << 13))  /* Low vectors */
    bic     r0, r0, #((1 <<  1))  /* Strict alignment disabled */
    orr     r0, r0, #((1 << 24) | /* Vectored interrupt ON */ \
                      (1 << 22))  /* Unaligned access support enabled */
    orr     r0, r0, #((1 << 14) | /* Round-robin replacement for I/D caches */ \
                      (1 << 12) | /* L1 I-cache enabled */ \
                      (1 << 11) | /* Program flow prediction enabled */ \
                      (1 <<  9) | /* ROM protection enabled */ \
                      (1 <<  8))  /* MMU protection enabled */
    orr     r0, r0, #((1 <<  2) | /* L1 D-cache enabled */ \
                      (1 <<  0))  /* MMU enabled */
    mcr     p15, 0, r0, c1, c0, 0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    ldr     pc, L_post_remap
L_post_remap:
    .word remap_end
remap_end:

#ifdef BOOTLOADER
    /* Copy bootloader exception handler code to address 0 */
    ldr     r2, =_vectorsstart
    ldr     r3, =_vectorsend
    ldr     r4, =_vectorscopy
1:
    cmp     r3, r2
    ldrhi   r5, [r4], #4
    strhi   r5, [r2], #4
    bhi     1b
#endif /* BOOTLOADER */

#ifndef BOOTLOADER
    /* Copy discardable SDMA code - loaded in the stack section
     * and so must be done first. Destination is the plugin buffer
     * which is safe when SDMA init takes place just after kernel
     * init. */
    ldr     r4, =_sdmacodecopy
    ldr     r3, =_sdmacodeend
    ldr     r2, =_sdmacodestart
1:
    cmp     r3, r2
    ldrhi   r5, [r4], #4
    strhi   r5, [r2], #4
    bhi     1b

    /* Make buffer coherent */
    ldr     r0, =_sdmacodestart
    sub     r1, r3, r0
    bl      clean_dcache_range
#endif /* BOOTLOADER */

    /* Initialise bss section to zero */
    ldr     r2, =_edata
    ldr     r3, =_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b
    
    /* Initialise the device bss section to zero */
    ldr     r2, =_devbssdata
    ldr     r3, =_devbssend
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b

    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =stackend
    ldr     r2, =stackbegin
    ldr     r3, =0xdeadbeef
1:
    cmp     sp, r2
    strhi   r3, [r2], #4
    bhi     1b
    
    /* Set up stack for IRQ mode */ 
    msr     cpsr_c, #0xd2
    ldr     sp, =irq_stack

    /* Set up stack for FIQ mode */ 
    msr     cpsr_c, #0xd1
    ldr     sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    msr     cpsr_c, #0xd7
    ldr     sp, =irq_stack
    msr     cpsr_c, #0xdb
    ldr     sp, =irq_stack

    /* Switch back to supervisor mode */
    msr     cpsr_c, #0xd3
    bl      main

#ifdef BOOTLOADER
    /* Exception vectors with absolute jumps for bootloader */
    .section .vectors,"aw"
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    ldr     pc, [pc, #24]
    .word   newstart
    .word   undef_instr_handler
    .word   software_int_handler
    .word   prefetch_abort_handler
    .word   data_abort_handler
    .word   reserved_handler
    .word   irq_handler
    .word   fiq_handler
#endif /* BOOTLOADER */

    .text

/* All illegal exceptions call into UIE with exception address as first
 * parameter. This is calculated differently depending on which exception
 * we're in. Second parameter is exception number, used for a string lookup
 * in UIE. */
undef_instr_handler:
    mov    r0, lr
    mov    r1, #0
    b      UIE

/* We run supervisor mode most of the time, and should never see a software
 * exception being thrown. Perhaps make it illegal and call UIE? */
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
