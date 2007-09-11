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
    .equ    COP_CTRL,   0xcf004058
    .equ    COP_STATUS, 0xcf004050
    .equ    IIS_CONFIG, 0xc0002500
    .equ    SLEEP,      0xca
    .equ    WAKE,       0xce
    .equ    SLEEPING,   0x4000
#else
    .equ    PROC_ID,    0x60000000
    .equ    COP_CTRL,   0x60007004
    .equ    COP_STATUS, 0x60007004
    .equ    IIS_CONFIG, 0x70002800
    .equ    SLEEP,      0x80000000
    .equ    WAKE,       0x0
    .equ    SLEEPING,   0x80000000
    .equ    CACHE_CTRL, 0x6000c000
#endif

    msr    cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ */
#ifndef E200R_INSTALLER
/* 1 - Copy the bootloader to IRAM */
    /* get the high part of our execute address */
    ldr    r7, =0xffffff00
    and    r4, pc, r7

    /* Copy bootloader to safe area - 0x40000000 (IRAM) */
    mov    r5, #0x40000000
    ldr    r6, = _dataend
1:
    cmp    r5, r6
    ldrcc  r2, [r4], #4
    strcc  r2, [r5], #4
    bcc    1b

#ifndef IPOD_ARCH
    /* For builds on targets with mi4 firmware, scramble writes data to 
       0xe0-0xeb, so jump past that.*/
    b      pad_skip

.space 60*4 

pad_skip:
#endif


/* 2 - Jump both CPU and COP there */
    ldr    pc, =start_loc    /* jump to the relocated start_loc:  */
#endif /* E200R_INSTALLER */

start_loc:

    /* Find out which processor we are */
    ldr    r0, =PROC_ID
    ldr    r0, [r0]
    and    r0, r0, #0xff
    cmp    r0, #0x55
    beq    cpu
    
    /* put us (co-processor) to sleep */
    ldr    r4, =COP_CTRL
    mov    r3, #SLEEP
    str    r3, [r4]
    ldr    pc, =cop_wake_start

cop_wake_start:
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

    ldr    r0, =startup_loc
    ldr    pc, [r0]

cpu:
    /* Wait for COP to be sleeping */
    ldr    r4, =COP_STATUS
1:
    ldr    r3, [r4]
    ands   r3, r3, #SLEEPING
    beq    1b
    
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

    /* execute the loader - this will load an image to 0x10000000 */
    bl     main

    ldr    r1, =startup_loc
    str    r0, [r1]

#if CONFIG_CPU != PP5002
    /* Flush cache */
    ldr    r3, =0xf000f044
    ldr    r4, [r3]
    orr    r4, r4, #0x2
    str    r4, [r3]

    ldr    r3, =CACHE_CTRL
1:
    ldr    r4, [r3]
    tst    r4, #0x8000
    bne    1b
#endif

    /* Wake up the coprocessor before executing the firmware */
    ldr    r4, =COP_CTRL
    mov    r3, #WAKE
    str    r3, [r4]

    mov    pc, r0

startup_loc:
    .word    0x0
    
#ifdef IPOD_ARCH
.align 8    /* starts at 0x100 */
.global boot_table
boot_table:
    /* here comes the boot table, don't move its offset */
    .space 400
#endif
