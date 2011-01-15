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
 * Copyright (C) 2010 by Michael Sevakis
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

/* PortalPlayer bootloader and startup code based on startup.s from the iPodLinux 
 * loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */
    .equ    PROC_ID,     0x60000000
    .equ    CPU_IDIS,    0x60004028
    .equ    CPU_CTRL,    0x60007000
    .equ    CPU_STATUS,  0x60007000
    .equ    COP_IDIS,    0x60004038
    .equ    COP_CTRL,    0x60007004
    .equ    COP_STATUS,  0x60007004
    .equ    CPU_SLEEPING,0x80000000
    .equ    COP_SLEEPING,0x80000000
    .equ    SLEEP,       0x80000000
    .equ    WAKE,        0x00000000
    .equ    MMAP_LOG,    0xf000f000 /* MMAP0 */
    .equ    MMAP_PHYS,   0xf000f004
    .equ    INT_VECT_TBL,0x6000f100
    .equ    CACHE_CTRL,  0x6000c000
    .equ    CACHE_ENAB,  0x1
    .equ    CACHE_OP_COMMIT_DISCARD, 0x1
    .equ    CACHE_OP_COMMIT        , 0x0
#if MEM > 32
    .equ    MMAP_MASK,   0x00003c00
#else
    .equ    MMAP_MASK,   0x00003e00
#endif
    .equ    MMAP_FLAGS,  0x00000f84

/*
 * Entry point
 */
    .section .init.text,"ax",%progbits
    .global  start
start:
    b       newstart

#ifdef IPOD_ARCH
.align 8    /* starts at 0x100 */
.global boot_table
boot_table:
    /* here comes the boot table, don't move its offset - preceding
       code+data must stay <= 256 bytes */
    .space  400
#else /* !IPOD_ARCH */
    /* (more than enough) space for exception vectors and mi4 magic */
    .space  68*4
#endif /* IPOD_ARCH */

newstart:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */
    adr     r4, start     /* cache initial load address */

    /* Copy startup stub to IRAM since we need to both move the bootloader's
     * location, which could overlap itself, and setup the memory mapper. */
    adr     r0, start_stub_begin
    mov     r1, #0x40000000
    adr     r2, start_stub_end
1:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
    cmp     r0, r2
    blo     1b
    mov     pc, #0x40000000

start_stub_begin:
    ldr     r0, =PROC_ID
    ldrb    r0, [r0]
    cmp     r0, #0x55
    beq     cpu

cop:
    mov     r0, #CACHE_OP_COMMIT_DISCARD
    bl      cache_operation

    ldr     r1, =COP_CTRL
    mov     r0, #SLEEP

    /* sleep us (co-processor) while bootloader is copied */
    str     r0, [r1]
    nop
    nop
    nop

    /* branch to final physical load address */
    ldr     r2, =1f
    and     r4, r4, #0xfc000000
    add     pc, r2, r4
1:
    /* wait for bootloader to finish */
    str     r0, [r1]
    nop
    nop
    nop

    /* branch to the address returned by main() */
    adr     r0, startup_loc
    ldr     pc, [r0]

cpu:
    /* wait for COP to sleep */
    ldr     r1, =COP_STATUS
1:
    ldr     r0, [r1]
    tst     r0, #COP_SLEEPING
    beq     1b

    mov     r0, #CACHE_OP_COMMIT_DISCARD
    bl      cache_operation

    /* move bootloader to the correct load address if needed */
    ldr     r1, =_loadaddress
    cmp     r4, r1
    ldrne   r2, =_loadaddressend
    movne   r0, r4
    sublo   r3, r2, r1 /* size */
    addlo   r0, r0, r3 /* initial load end addr */
1: /* lower to higher move - copy up */
    cmphi   r2, r1
    ldrhi   r3, [r0], #4
    strhi   r3, [r1], #4
    bhi     1b
1: /* higher to lower move - copy down */
    cmplo   r1, r2
    ldrlo   r3, [r0, #-4]!
    strlo   r3, [r2, #-4]!
    blo     1b

    mov     r0, #CACHE_OP_COMMIT
    bl      cache_operation

    and     r4, r4, #0xfc000000

    ldr     r0, =MMAP_FLAGS
    orr     r0, r0, r4 /* adjust for execute address */
    ldr     r1, =MMAP_MASK
    ldr     r2, =MMAP_LOG
    ldr     r3, =MMAP_PHYS
    str     r1, [r2] /* MMAP_LOG = MMAP_MASK */
    str     r0, [r3] /* MMAP_PHYS = MMAP_FLAGS | SDRAM base addr */

    /* wake the COP to jump it to the correct place */
    ldr     r1, =COP_CTRL
    mov     r0, #WAKE
    str     r0, [r1]

    /* wait for COP to halt then loading may proceed */
    ldr     r1, =COP_STATUS
1:
    ldr     r0, [r1]
    tst     r0, #COP_SLEEPING
    beq     1b

    ldr     r0, =start_stub_end
    add     pc, r0, r4

cache_operation: /* (bool commit_discard) */
    ldr     r2, =CACHE_CTRL
    ldr     r1, [r2]
    tst     r1, #CACHE_ENAB
    bxeq    lr
    cmp     r0, #CACHE_OP_COMMIT
    ldr     r0, =0xf000f044
    ldr     r1, [r0]
    orrne   r1, r1, #0x6
    orreq   r1, r1, #0x2
    str     r1, [r0]
1:
    ldr     r1, [r2]
    tst     r1, #0x8000
    bne     1b
    bx      lr
    .ltorg /* constants used in stub come with us to IRAM */
start_stub_end:
    /* now executing from final physical address */

    /* copy the vector addresses to the table */
    ldr     r0, =INT_VECT_TBL
    adr     r1, vectorsstart
    adr     r2, vectorsend
1:
    cmp     r2, r1
    ldrhi   r3, [r1], #4
    strhi   r3, [r0], #4
    bhi     1b

    /* Copy the IRAM */
    ldr     r0, =_iramcopy
    ldr     r1, =_iramstart
    ldr     r2, =_iramend
1:
    cmp     r2, r1
    ldrhi   r3, [r0], #4
    strhi   r3, [r1], #4
    bhi     1b

    mov     r0, #0

    /* Zero out IBSS */
    ldr     r1, =_iedata
    ldr     r2, =_iend
1:
    cmp     r2, r1
    strhi   r0, [r1], #4
    bhi     1b

    /* Initialise bss/ncbss sections to zero */
    ldr     r1, =_edata
    ldr     r2, =_end
1:
    cmp     r2, r1
    strhi   r0, [r1], #4
    bhi     1b

    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     r0, =0xdeadbeef
    ldr     r1, =stackbegin
    ldr     sp, =stackend
1:
    cmp     sp, r1
    strhi   r0, [r1], #4
    bhi     1b

    /* Set up stack for IRQ mode */ 
    msr     cpsr_c, #0xd2 /* IRQ/FIQ disabled */
    ldr     sp, =irq_stack
    /* Let abort and undefined modes use IRQ stack */
    msr     cpsr_c, #0xd7 /* IRQ/FIQ disabled */
    ldr     sp, =irq_stack
    msr     cpsr_c, #0xdb /* IRQ/FIQ disabled */
    ldr     sp, =irq_stack

    /* Switch back to supervisor mode */
    msr     cpsr_c, #0xd3

    /* execute the loader - this will load an image to 0x10000000 */
    ldr     r0, =main
    mov     lr, pc
    bx      r0

    /* store actual startup location returned by main() */
    ldr     r1, =startup_loc
    str     r0, [r1]

    /* write back anything loaded + startup_loc */
    mov     r0, #CACHE_OP_COMMIT
    bl      cache_operation

    mov     r0, #0

    /* disable memory mapper */
    ldr     r1, =MMAP_LOG
    ldr     r2, =MMAP_PHYS
    str     r0, [r1]
    str     r0, [r2]

    /* bring COP back to life */
    ldr     r1, =COP_CTRL
    mov     r0, #WAKE
    str     r0, [r1]

    /* after this point, r0-r3 are reserved for OF magic */

#if defined(SANSA_C200) || defined(PHILIPS_HDD1630)
    /* Magic for loading the c200 OF */
    ldr     r0, =0xb00d10ad
    mov     r1, #0x700
    ldr     r2, =0xfff0
    mov     r3, #0x7
#endif

#if defined(PHILIPS_HDD6330)
    /* Magic for loading the HDD6XX0 OF */
    ldr     r0, =0xb00d10ad
    mov     r1, #0x800
    ldr     r2, =0xfff0
    mov     r3, #0x7
#endif

    /* branch to the address returned by main() */
    adr     r4, startup_loc
    ldr     pc, [r4]

startup_loc:
    .word   0x00000000

/* exception handlers: will be copied to local vector table */
vectorsstart:
    .word   newstart 
    .word   undef_instr_handler
    .word   software_int_handler
    .word   prefetch_abort_handler
    .word   data_abort_handler
    .word   reserved_handler
    .word   irq_handler
    .word   fiq_handler
vectorsend:

    .text

/* All illegal exceptions call into UIE with exception address as first
   parameter. This is calculated differently depending on which exception
   we're in. Second parameter is exception number, used for a string lookup
   in UIE.
 */
undef_instr_handler:
    sub     r0, lr, #4
    mov     r1, #0
    b       UIE

/* We run supervisor mode most of the time, and should never see a software
   exception being thrown. Perhaps make it illegal and call UIE?
 */
software_int_handler:
reserved_handler:
    movs    pc, lr
prefetch_abort_handler:
    sub     r0, lr, #4
    mov     r1, #1
    b       UIE

data_abort_handler:
    sub     r0, lr, #8 
    mov     r1, #2
    b       UIE

/* should never happen in the bootloader */
fiq_handler:
    subs    pc, lr, #4

/* 256 words of IRQ stack */
    .section .bss
    .balign 16
    .space  256*4
irq_stack:
