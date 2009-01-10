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

/* PortalPlayer bootloader and startup code based on startup.s from the iPodLinux 
 * loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */
#if CONFIG_CPU == PP5002
    .equ    PROC_ID,     0xc4000000
    .equ    CPU_CTRL,    0xcf004054
    .equ    CPU_STATUS,  0xcf004050
    .equ    COP_CTRL,    0xcf004058
    .equ    COP_STATUS,  0xcf004050
    .equ    IIS_CONFIG,  0xc0002500
    .equ    SLEEP,       0xca
    .equ    WAKE,        0xce
    .equ    CPUSLEEPING, 0x8000
    .equ    COPSLEEPING, 0x4000
    .equ    CACHE_CTRL,  0xcf004024
    .equ    CACHE_ENAB,  0x2 /* Actually the CACHE_INIT flag */
#else
    .equ    PROC_ID,     0x60000000
    .equ    CPU_CTRL,    0x60007000
    .equ    CPU_STATUS,  0x60007000
    .equ    COP_CTRL,    0x60007004
    .equ    COP_STATUS,  0x60007004
    .equ    IIS_CONFIG,  0x70002800
    .equ    SLEEP,       0x80000000
    .equ    WAKE,        0x0
    .equ    CPUSLEEPING, 0x80000000
    .equ    COPSLEEPING, 0x80000000
    .equ    CACHE_CTRL,  0x6000c000
    .equ    CACHE_ENAB,  0x1
#endif

    msr    cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ */
#ifndef E200R_INSTALLER
/* 1 - Copy the bootloader to IRAM */
    /* get the high part of our execute address */
    bic    r0, pc, #0xff /* r4 = pc & 0xffffff00 */

    /* Copy bootloader to safe area - 0x40000000 (IRAM) */
    mov    r1, #0x40000000
    ldr    r2, =_dataend
1:
    cmp    r2, r1
    ldrhi  r3, [r0], #4
    strhi  r3, [r1], #4
    bhi    1b

#ifndef IPOD_ARCH
    /* For builds on targets with mi4 firmware, scramble writes data to 
       0xe0-0xeb, so jump past that. pad_skip must then exist at an
       address >= 0xec */
    b      pad_skip

.space 60*4 

pad_skip:
#endif /* IPOD_ARCH */


/* 2 - Jump both CPU and COP there */
    ldr    pc, =start_loc    /* jump to the relocated start_loc:  */
#endif /* E200R_INSTALLER */

start_loc:
    /* Find out which processor we are */
    ldr    r0, =PROC_ID
    ldrb   r0, [r0]
    cmp    r0, #0x55
    beq    cpu

cop:
    /* put us (co-processor) to sleep */
    ldr    r0, =COP_CTRL
    mov    r1, #SLEEP
    str    r1, [r0]
    nop
    nop
    
    /* Invalidate cache */
    mov    r0, #1
    bl     cache_op

    ldr    r0, =startup_loc
    ldr    pc, [r0]

cpu:
    /* Wait for COP to be sleeping */
    ldr    r0, =COP_STATUS
1:
    ldr    r1, [r0]
    tst    r1, #COPSLEEPING
    beq    1b
    
    /* Initialise bss section to zero */
    ldr    r0, =_edata
    ldr    r1, =_end
    mov    r2, #0
1:
    cmp    r1, r0
    strhi  r2, [r0], #4
    bhi    1b
       
    /* Set up some stack and munge it with 0xdeadbeef */
    ldr    sp, =stackend
    ldr    r0, =stackbegin
    ldr    r1, =0xdeadbeef
1:
    cmp    sp, r0
    strhi  r1, [r0], #4
    bhi    1b

    /* execute the loader - this will load an image to 0x10000000 */
    bl     main

    /* store actual startup location returned by main() */
    ldr    r1, =startup_loc
    str    r0, [r1]

    /* flush cache */    
    mov    r0, #0
    bl     cache_op

    /* Wake up the coprocessor before executing the firmware */
    ldr    r0, =COP_CTRL
    mov    r1, #WAKE
    str    r1, [r0]

#if defined(SANSA_C200) || defined(PHILIPS_HDD1630)
    /* Magic for loading the c200 OF */
    ldr    r0, =0xb00d10ad
    mov    r1, #0x700
    ldr    r2, =0xfff0
    mov    r3, #0x7
#endif

    ldr    r4, =startup_loc
    ldr    pc, [r4]

startup_loc:
    .word    0x0
    
#ifdef IPOD_ARCH
.align 8    /* starts at 0x100 */
.global boot_table
boot_table:
    /* here comes the boot table, don't move its offset - preceding
       code+data must stay <= 256 bytes */
    .space 400
#endif

cache_op:
    ldr    r2, =CACHE_CTRL
    ldr    r1, [r2]
    tst    r1, #CACHE_ENAB
    bxeq   lr
    cmp    r0, #0
#ifdef CPU_PP502x
    ldr    r0, =0xf000f044
    ldr    r1, [r0]
    orrne  r1, r1, #0x6
    orreq  r1, r1, #0x2
    str    r1, [r0]
1:
    ldr    r1, [r2]
    tst    r1, #0x8000
    bne    1b
#elif CONFIG_CPU == PP5002
    ldrne  r0, =0xf0004000
    ldreq  r0, =0xf000c000
    add    r1, r0, #0x2000
    mov    r2, #0
1:
    cmp    r1, r0
    strhi  r2, [r0], #16
    bhi    1b
#endif /* CPU type */
    bx     lr

