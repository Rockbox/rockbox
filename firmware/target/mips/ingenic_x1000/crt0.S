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

    /* Exception entry points */
    .section .vectors.1, "ax", %progbits
    j       tlb_refill_handler
    nop

    .section .vectors.2, "ax", %progbits
    j       real_exception_handler
    nop

    .section .vectors.3, "ax", %progbits
    j       real_exception_handler
    nop

    .section .vectors.4, "ax", %progbits
    j       real_exception_handler
    nop

    .section .vectors, "ax", %progbits
real_exception_handler:
    move    k0, sp
    la      sp, _irqstackend
    addiu   sp, -0x84
    sw      k0, 0x80(sp)
    sw      ra, 0x00(sp)
    sw      fp, 0x04(sp)
    sw      gp, 0x08(sp)
    sw      t9, 0x0c(sp)
    sw      t8, 0x10(sp)
    sw      s7, 0x14(sp)
    sw      s6, 0x18(sp)
    sw      s5, 0x1c(sp)
    sw      s4, 0x20(sp)
    sw      s3, 0x24(sp)
    sw      s2, 0x28(sp)
    sw      s1, 0x2c(sp)
    sw      s0, 0x30(sp)
    sw      t7, 0x34(sp)
    sw      t6, 0x38(sp)
    sw      t5, 0x3c(sp)
    sw      t4, 0x40(sp)
    sw      t3, 0x44(sp)
    sw      t2, 0x48(sp)
    sw      t1, 0x4c(sp)
    sw      t0, 0x50(sp)
    sw      a3, 0x54(sp)
    sw      a2, 0x58(sp)
    sw      a1, 0x5c(sp)
    sw      a0, 0x60(sp)
    sw      v1, 0x64(sp)
    sw      v0, 0x68(sp)
    sw      $1, 0x6c(sp)
    mflo    k0
    nop
    sw      k0, 0x70(sp)
    mfhi    k0
    nop
    sw      k0, 0x74(sp)
    mfc0    k0, C0_STATUS
    nop
    nop
    nop
    sw      k0, 0x78(sp)
    mfc0    k0, C0_EPC
    nop
    nop
    nop
    sw      k0, 0x7c(sp)

    li      k1, M_CauseExcCode
    mfc0    a0, C0_CAUSE
    and     k0, a0, k1
    bnez    k0, _exception
    nop
    jal     intr_handler
    nop
    j       _exception_return

_exception:
    mfc0    a1, C0_EPC
    nop
    nop
    nop
    jal     exception_handler
    move    a2, sp

_exception_return:
    lw      ra, 0x00(sp)
    lw      fp, 0x04(sp)
    lw      gp, 0x08(sp)
    lw      t9, 0x0c(sp)
    lw      t8, 0x10(sp)
    lw      s7, 0x14(sp)
    lw      s6, 0x18(sp)
    lw      s5, 0x1c(sp)
    lw      s4, 0x20(sp)
    lw      s3, 0x24(sp)
    lw      s2, 0x28(sp)
    lw      s1, 0x2c(sp)
    lw      s0, 0x30(sp)
    lw      t7, 0x34(sp)
    lw      t6, 0x38(sp)
    lw      t5, 0x3c(sp)
    lw      t4, 0x40(sp)
    lw      t3, 0x44(sp)
    lw      t2, 0x48(sp)
    lw      t1, 0x4c(sp)
    lw      t0, 0x50(sp)
    lw      a3, 0x54(sp)
    lw      a2, 0x58(sp)
    lw      a1, 0x5c(sp)
    lw      a0, 0x60(sp)
    lw      v1, 0x64(sp)
    lw      v0, 0x68(sp)
    lw      $1, 0x6c(sp)
    lw      k0, 0x70(sp)
    mtlo    k0
    nop
    lw      k0, 0x74(sp)
    mthi    k0
    nop
    lw      k0, 0x78(sp)
    mtc0    k0, C0_STATUS
    nop
    nop
    nop
    lw      k0, 0x7c(sp)
    mtc0    k0, C0_EPC
    nop
    nop
    nop
    lw      sp, 0x80(sp)
    eret
    nop

    .set pop
