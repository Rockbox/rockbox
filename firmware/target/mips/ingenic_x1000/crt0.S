/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

    .text
    .extern main
    .global _start

    .set push
    .set mips32
    .set noreorder
    .set noat

    .section .init.text

_start:
    /* Clear data watchpoint */
    mtc0    zero, C0_WATCHLO
    mtc0    zero, C0_WATCHHI

    /* Set BEV, ERL, mask interrupts */
    li      v0, 0x40fc04
    mtc0    v0, C0_Status

    /* Set Cause_IV to 1 (use special interrupt vector) */
    li      v0, M_CauseIV
    mtc0    v0, C0_Cause

    /* Set CPU_MODE and BUS_MODE to 1 in CPM_OPCR (Ingenic does this) */
    lui     v0, 0xb000
    lw      v1, 0x24(v0)
    ori     v1, v1, 0x22
    sw      v1, 0x24(v0)

    /* Enable kseg0 cacheability */
    li      v0, 3
    mtc0    v0, C0_Config
    nop

    /* According to ingenic: "enable idx-store-data cache insn" */
    li      v0, 0x20000000
    mtc0    v0, C0_ErrCtl

    /* Cache init */
    li      v0, 0x80000000
    ori     v1, v0, 0x4000
    mtc0    zero, C0_TAGLO
    mtc0    zero, C0_TAGHI
_cache_loop:
    cache   ICIndexStTag, 0(v0)
    cache   DCIndexStTag, 0(v0)
    addiu   v0, v0, 32
    bne     v0, v1, _cache_loop
    nop

    /* Invalidate BTB */
    mfc0    v0, C0_Config, 7
    nop
    ori     v0, v0, 2
    mtc0    v0, C0_Config, 7
    nop

#ifndef BOOTLOADER_SPL
    /* Copy IRAM from BSS to low memory. */
    la      t0, _iramcopy
    la      t1, _iramstart
    la      t2, _iramend
_iram_loop:
    lw      t3, 0(t0)
    addiu   t1, 4
    addiu   t0, 4
    bne     t1, t2, _iram_loop
    sw      t3, -4(t1)
#endif

    /* Clear the BSS segment (needed to zero-initialize C static values) */
    la      t0, _bssbegin
    la      t1, _bssend
    beq     t0, t1, _bss_done
_bss_loop:
    addiu   t0, 4
    bne     t0, t1, _bss_loop
    sw      zero, -4(t0)
_bss_done:

#ifndef BOOTLOADER_SPL
    /* Set stack pointer and clear the stack */
    la      sp, stackend
    la      t0, stackbegin
    li      t1, 0xDEADBEEF
_stack_loop:
    addiu   t0, 4
    bne     t0, sp, _stack_loop
    sw      t1, -4(t0)

    /* Clear the IRQ stack */
    la      k0, _irqstackend
    la      t0, _irqstackbegin
_irqstack_loop:
    addiu   t0, 4
    bne     t0, k0, _irqstack_loop
    sw      t1, -4(t0)
#endif

    /* Jump to C code */
    j       main
    nop

#ifndef BOOTLOADER_SPL
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
#endif

    .set pop
