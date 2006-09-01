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

/* Most of the code from this file has now been moved into the target trees */

#include "config.h"
#include "cpu.h"

    .section .init.text,"ax",@progbits

    .global    start
start:

#if CONFIG_CPU == TCC730
    /* Platform:     Gmini 120/SP */
    ;; disable all interrupts
    clrsr fe
    clrsr ie
    clrsr te
    ld a14, #0x3F0000

    ld r5, 0xA5
    ldb @[a14 + 6], r5    ; disable watchdog

    ld a11, #(_datacopy)    ; where the data section is in the flash
    ld a8, #(_datastart)    ; destination

    ;; copy data section from flash to ram.
    ld a9, #_datasize
    ld r6, e9
    cmp eq, r6, #0
    brf .data_copy_loop
    cmp eq, r9, #0
    brt .data_copy_end
.data_copy_loop:
    ldc r2, @a11
    ldw @[a8 + 0], r2
    add a11, #0x2
    add a8, #0x2
    sub r9, #0x2
    sbc r6, #0
    cmp ugt, r6, #0
    brt .data_copy_loop
    cmp ugt, r9, #0
    brt .data_copy_loop
.data_copy_end:

    ;; zero out bss
    ld r2, #0
    ld a8, #(_bssstart)    ; destination
    ld a9, #_bsssize
    ld r6, e9

    cmp eq, r6, #0
    brf .bss_init_loop
    cmp eq, r9, #0
    brt .bss_init_end
.bss_init_loop:
    ldw @[a8 + 0], r2
    add a8, #0x2
    sub r9, #0x2
    sbc r6, #0
    cmp ugt, r6, #0
    brt .bss_init_loop
    cmp ugt, r9, #0
    brt .bss_init_loop
.bss_init_end:

    ;; set stack pointer
    ld a15, _stackend

    ;; go!
    jsr _main

    ;; soft reset
    ld a10, #0
    ldc r10, @a10
    jmp a10


    .section .vectors, "ax"
irq_handler:

    push r0, r1
    push r2, r3
    push r4, r5
    push r6, r7
    push a8, a9
    push a10, a11
    push a12, a13
    push a14
    ld a13, #0x3f0000
    ldb r0, @[a13 + 0x26]
    add r0, r0
    ld a10, #_interrupt_vector
    ldw a13, @[a10 + r0]
    jsr a13
    pop a14
    pop a13, a12
    pop a11, a10
    pop a9, a8
    pop r7, r6
    pop r5, r4
    pop r3, r2
    pop r1, r0
    ret_irq
#endif
