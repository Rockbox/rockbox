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

/* Arm bootloader and startup code based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */
    msr    cpsr, #0xd3 /* enter supervisor mode, disable IRQ */

#if !defined(BOOTLOADER)
    /* Copy exception handler code to address 0 */
    ldr    r2, =_vectorsstart
    ldr    r3, =_vectorsend
    ldr    r4, =_vectorscopy
1:
    cmp    r3, r2
    ldrhi  r5, [r4], #4
    strhi  r5, [r2], #4
    bhi    1b
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
#endif /* !BOOTLOADER, !STUB */

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
/* Code for ARM bootloader targets other than iPod go here */

    /* Proper initialization pulled from 0x5070 */

    /* BWSCON
     *      Reserved 0
     * Bank 0:
     *      Bus width 01 (16 bit)
     * Bank 1:
     *      Buswidth 00 (8 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 2:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 3:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Use UB/LB 1
     * Bank 4:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Use UB/LB 1
     * Bank 5: 
     *      Buswidth 00 (8 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 6:
     *      Buswidth 10 (32 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     * Bank 7:
     *      Buswidth 00 (8 bit)
     *      Disable wait 0
     *      Not using UB/LB 0
     */
    ldr     r2,=0x01055102
    mov     r1, #0x48000000
    str     r2, [r1]

    /* BANKCON0 
     *      Pagemode: normal (1 data) 00
     *      Pagemode access cycle: 2 clocks 00
     *      Address hold: 2 clocks 10
     *      Chip selection hold time: 1 clock 10
     *      Access cycle: 8 clocks 101
     *      Chip select setup time: 1 clock 01
     *      Address setup time: 0 clock 00
     */
    ldr     r2,=0x00000D60
    str     r2, [r1, #4]


    /* BANKCON1 
     *      Pagemode: normal (1 data) 00
     *      Pagemode access cycle: 2 clocks 00
     *      Address hold: 0 clocks 00
     *      Chip selection hold time: 0 clock 00
     *      Access cycle: 1 clocks 000
     *      Chip select setup time: 0 clocks 00
     *      Address setup time: 0 clocks 00
     */
    ldr     r2,=0x00000000
    str     r2, [r1, #8]

    /* BANKCON2 
     *      Pagemode: normal (1 data) 00
     *      Pagemode access cycle: 2 clocks 00
     *      Address hold: 2 clocks 10
     *      Chip selection hold time: 2 clocks 10
     *      Access cycle: 14 clocks 111
     *      Chip select setup time: 4 clocks 11
     *      Address setup time: 0 clocks 00
     */
    ldr     r2,=0x00001FA0
    str     r2, [r1, #0xC]

    /* BANKCON3 */
    ldr     r2,=0x00001D80
    str     r2, [r1, #0x10]
    /* BANKCON4 */
    str     r2, [r1, #0x14]

    /* BANKCON5 */
    ldr     r2,=0x00000000
    str     r2, [r1, #0x18]

    /* BANKCON6/7
     *      SCAN:   9 bit       01
     *      Trcd:   3 clocks    01
     *      Tcah:   0 clock     00
     *      Tcoh:   0 clock     00
     *      Tacc:   1 clock     000
     *      Tcos:   0 clock     00
     *      Tacs:   0 clock     00
     *      MT:     Sync DRAM   11
     */
    ldr     r2,=0x00018005
    str     r2, [r1, #0x1C]
    /* BANKCON7 */
    str     r2, [r1, #0x20]

    /* REFRESH */
    ldr     r2,=0x00980501
    str     r2, [r1, #0x24]

    /* BANKSIZE
     *      BK76MAP: 32M/32M    000
     *      Reserved: 0         0 (was 1)
     *      SCLK_EN: always     1 (was 0)
     *      SCKE_EN: disable    0
     *      Reserved: 0         0
     *      BURST_EN: enabled   1
     */
    ldr     r2,=0x00000090
    str     r2, [r1, #0x28]

    /* MRSRB6 */
    ldr     r2,=0x00000030
    str     r2, [r1, #0x2C]
    /* MRSRB7 */
    str     r2, [r1, #0x30]
    
#if 0
    /* This next part I am not sure of the purpose */
    
    /* GPACON */
    mov    r2,#0x01FFFCFF
    str    r2,=0x56000000
    
    /* GPADAT */
    mov    r2,#0x01FFFEFF
    str    r2,=0x56000004

    /* MRSRB6 */
    mov    r2,#0x00000000
    str    r2,=0x4800002C

    /* GPADAT */
    ldr     r2,=0x01FFFFFF
    mov     r1, #0x56000000 
    str     r2, [r1, #4]

    /* MRSRB6 */
    mov    r2,#0x00000030
    str    r2,=0x4800002C

    /* GPACON */
    mov    r2,#0x01FFFFFF
    str    r2,=0x56000000

    /* End of the unknown */
#endif

    /* get the high part of our execute address */
    ldr    r2, =0xffffff00
    and    r4, pc, r2

    /* Copy bootloader to safe area - 0x31000000 */
    mov    r5, #0x30000000
    add    r5, r5, #0x1000000
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
     bl main

#else /* BOOTLOADER */

     /* Set up stack for IRQ mode */
    msr    cpsr_c, #0xd2
    ldr    sp, =irq_stack
    /* Set up stack for FIQ mode */ 
    msr    cpsr_c, #0xd1
    ldr    sp, =fiq_stack

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

data_abort_handler:
    sub    r0, lr, #8 
    mov    r1, #2
    b      UIE

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
