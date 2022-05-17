/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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
#include "mips.h"
#include "bootdata.h"

    .text
    .extern main
    .extern system_early_init
    .extern _loadaddress
    .global _start

    .set push
    .set mips32
    .set noreorder
    .set noat

    .section .init.text

_start:
    b       _realstart
    nop

    /* Header entries are 4-byte string labels (not null terminated!) followed
     * by 4-byte values. Header should begin in the first 128 bytes and should
     * be no more than 256 bytes in length. */
_header:
    .ascii  "BEGINHDR"      /* beginning of header */
    .ascii  "LOAD"
    .word   _loadaddress
    .ascii  "ENDH"          /* end of header structure */

#ifndef BOOTLOADER
    /* Multiboot support header; this is not part of the above header. */
    put_boot_data_here
#endif

_realstart:
    /* Copy IRAM from BSS to low memory. */
    la      a0, _iramcopy
    la      a1, _iramstart
    la      a2, _iramend
    bal     _copy
    nop

    /* Copy TCSM from BSS */
    la      a0, _tcsmcopy
    la      a1, _tcsmstart
    la      a2, _tcsmend
    bal     _copy
    nop

    /* Clear the BSS segment (needed to zero-initialize C static values) */
    la      a0, _bssbegin
    la      a1, _bssend
    bal     _clear
    move    a2, $0

    /* Set stack pointer and clear the stack */
    la      sp, stackend
    la      a0, stackbegin
    li      a2, 0xDEADBEEF
    bal     _clear
    move    a1, sp

    /* Clear the IRQ stack */
    la      k0, _irqstackend
    la      a0, _irqstackbegin
    bal     _clear
    move    a1, k0

    /* Write back D-cache and invalidate I-cache  */
    li      v0, 0x80000000
    ori     v1, v0, (0x4000 - 32)
    mtc0    zero, C0_TAGLO
    mtc0    zero, C0_TAGHI
1:
    cache   DCIndexWBInv, 0(v0)
    cache   ICIndexStTag, 0(v0)
    bne     v0, v1, 1b
    addiu   v0, v0, 32

    /* Invalidate BTB */
    mfc0    v0, C0_Config, 7
    nop
    ori     v0, v0, 2
    mtc0    v0, C0_Config, 7
    nop

    /* Jump to C code */
    jal     system_early_init
    nop
    j       main
    move    ra, zero     /* init backtrace root */

    /* copy(void* src, void* dst, void* dst_end) */
_copy:
    beq     a1, a2, 1f
    addiu   a1, 4
    lw      t0, 0(a0)
    addiu   a0, 4
    b       _copy
    sw      t0, -4(a1)
1:
    jr      ra
    nop

    /* clear(void* dst, void* dst_end, int value) */
_clear:
    beq     a0, a1, 1f
    addiu   a0, 4
    b       _clear
    sw      a2, -4(a0)
1:
    jr      ra
    nop

    .set pop
