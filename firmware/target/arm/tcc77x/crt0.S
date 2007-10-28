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
    /* Add -DTCCBOOT to EXTRA_DEFINES in the bootloader Makefile to
       enable building the bootloader to be appended to the end of the
       original firmware, dual-booting based on a key-press.

       The following two values are filled in by mktccboot.
     */
    .word  0              /* Saved entrypoint of original firmware*/
    .word  0              /* Location in RAM of the start of our bootloader */
#else
    ldr    pc, =start_loc    /* jump to the main entry point  */

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
#ifdef TCCBOOT
#ifdef LOGIK_DAX
    mov    r0, #0x80000000
    ldr    r0, [r0, #0x300]     /* Read GPIO A */
    tst    r0, #0x2
    ldrne  pc, [pc, #-28]     /* Jump to original firmware if HOLD button not pressed */
#else
    #error No bootup key detection implemented for this target
#endif

    /* Copy bootloader to safe area - 0x21000000 (DRAM) */
    /* TODO: Adjust this for other targets - DRAM + DRAMSIZE - 0x100000 */
    ldr    r0, [pc, #-28]
    mov    r1, #0x20000000
    add    r1, r1, #0x100000
    ldr    r2, =_dataend
1:
    cmp    r2, r1
    ldrhi  r3, [r0], #4
    strhi  r3, [r1], #4
    bhi    1b

    ldr    pc, =copied_start   /* jump to the relocated start_loc:  */

copied_start:
#endif
#else
    /* We don't use interrupts in the bootloader */

    /* Set up stack for IRQ mode */ 
    mov    r0,#0xd2
    msr    cpsr, r0
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    mov    r0,#0xd1
    msr    cpsr, r0
    ldr    sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    mov    r0,#0xd7
    msr    cpsr, r0
    ldr    sp, =irq_stack
    mov    r0,#0xdb
    msr    cpsr, r0
    ldr    sp, =irq_stack
#endif

    /* Switch to supervisor mode */
    mov    r0,#0xd3
    msr    cpsr, r0
    ldr    sp, =stackend
    
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

#ifndef BOOTLOADER
    /* We don't use interrupts in the bootloader */

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

#endif
