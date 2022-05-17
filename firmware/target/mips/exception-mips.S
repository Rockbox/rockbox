/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "mips.h"

    .text
    .set push
    .set mips32

    .section .vectors.1, "ax", %progbits
    la      k1, tlb_refill_handler
    j       _exception_enter

    .section .vectors.2, "ax", %progbits
    la      k1, cache_error_handler
    j       _exception_enter

    .section .vectors.3, "ax", %progbits
    li      k1, M_CauseExcCode
    mfc0    k0, C0_CAUSE
    and     k0, k1
    beq     zero, k0, 1f
    la      k1, exception_handler
    j       _exception_enter
1:
    la      k1, intr_handler
    j       _exception_enter

    .section .vectors.4, "ax", %progbits
    /* same as above */
    li      k1, M_CauseExcCode
    mfc0    k0, C0_CAUSE
    and     k0, k1
    beq     zero, k0, 1f
    nop
    la      k1, exception_handler
    j       _exception_enter
1:
    la      k1, intr_handler
    j       _exception_enter

    .set push
    .set noat
    .set noreorder
    .section .vectors, "ax", %progbits

_exception_enter:
    la      k0, _irqstackend
    addiu   k0, -0x84

    sw      $1,  0x00(k0)
    sw      $2,  0x04(k0)
    sw      $3,  0x08(k0)
    sw      $4,  0x0c(k0)
    sw      $5,  0x10(k0)
    sw      $6,  0x14(k0)
    sw      $7,  0x18(k0)
    sw      $8,  0x1c(k0)
    sw      $9,  0x20(k0)
    sw      $10, 0x24(k0)
    sw      $11, 0x28(k0)
    sw      $12, 0x2c(k0)
    sw      $13, 0x30(k0)
    sw      $14, 0x34(k0)
    sw      $15, 0x38(k0)
    sw      $16, 0x3c(k0)
    sw      $17, 0x40(k0)
    sw      $18, 0x44(k0)
    sw      $19, 0x48(k0)
    sw      $20, 0x4c(k0)
    sw      $21, 0x50(k0)
    sw      $22, 0x54(k0)
    sw      $23, 0x58(k0)
    sw      $24, 0x5c(k0)
    sw      $25, 0x60(k0)
    sw      $28, 0x64(k0)
    sw      $29, 0x68(k0)
    sw      $30, 0x6c(k0)
    sw      $31, 0x70(k0)

    mflo    t0
    ssnop
    sw      t0,  0x74(k0)

    mfhi    t0
    ssnop
    sw      t0,  0x78(k0)

    mfc0    t0, C0_STATUS
    ssnop
    ssnop
    ssnop
    sw      t0,  0x7c(k0)

    mfc0    a1, C0_EPC
    ssnop
    ssnop
    ssnop
    sw      a1,  0x80(k0)

    move    sp, k0

    jalr    k1
    move    a0, sp

_exception_return:
    /* restore all GPRs except sp */
    lw      $1,  0x00(sp)
    lw      $2,  0x04(sp)
    lw      $3,  0x08(sp)
    lw      $4,  0x0c(sp)
    lw      $5,  0x10(sp)
    lw      $6,  0x14(sp)
    lw      $7,  0x18(sp)
    lw      $8,  0x1c(sp)
    lw      $9,  0x20(sp)
    lw      $10, 0x24(sp)
    lw      $11, 0x28(sp)
    lw      $12, 0x2c(sp)
    lw      $13, 0x30(sp)
    lw      $14, 0x34(sp)
    lw      $15, 0x38(sp)
    lw      $16, 0x3c(sp)
    lw      $17, 0x40(sp)
    lw      $18, 0x44(sp)
    lw      $19, 0x48(sp)
    lw      $20, 0x4c(sp)
    lw      $21, 0x50(sp)
    lw      $22, 0x54(sp)
    lw      $23, 0x58(sp)
    lw      $24, 0x5c(sp)
    lw      $25, 0x60(sp)
    lw      $28, 0x64(sp)
    lw      $30, 0x6c(sp)
    lw      $31, 0x70(sp)

    lw      k0,  0x74(sp)
    mtlo    k0
    ssnop

    lw      k0,  0x78(sp)
    mthi    k0
    ssnop

    lw      k0,  0x7c(sp)
    mtc0    k0, C0_STATUS
    ssnop
    ssnop
    ssnop

    lw      k0,  0x80(sp)
    mtc0    k0, C0_EPC
    ssnop
    ssnop
    ssnop

    /* restore stack pointer last */
    lw      sp,  0x68(sp)
    eret
    ssnop

    .set pop
    .set pop
