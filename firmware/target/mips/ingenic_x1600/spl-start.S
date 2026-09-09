/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

/* Flash SPL and USB stage1 have separate link addresses. Both execute from
 * locked cache, so an index cache-init sweep would invalidate running
 * code. The USB entry must also preserve the live BootROM state. */

#include "config.h"
#include "x1600.h"
#include "mips.h"

    .set push
    .set mips32
    .set noreorder
    .set noat

/* Flash SPL entry (SFC / MSC boot) */

    .text
    .extern spl_main
    .global _spl_start
    .section .startup.spl, "ax", %progbits

_spl_start:
    /* Clear data watchpoints; the BootROM may have left them set */
    mtc0    zero, C0_WATCHLO
    mtc0    zero, C0_WATCHHI

    /* Invalidate the branch target buffer (Config7 bit 1). The vendor SPL
     * does this first, before touching Status. */
    mfc0    v0, C0_Config, 7
    nop
    ori     v0, v0, 2
    mtc0    v0, C0_Config, 7
    nop

    /* Status = BEV | ERL, interrupts masked: byte-for-byte what both the
     * vendor SPL and the X1000 SPL use */
    li      v0, 0x40fc04
    mtc0    v0, C0_Status

    /* Cause_IV = 1 (use the special interrupt vector) */
    li      v0, M_CauseIV
    mtc0    v0, C0_Cause

    /* The first 4 KiB of cache address space is ours once the BootROM has
     * handed over; grow the stack down from the top of it */
    li      sp, X1600_SPL_LOAD_ADDR
    addiu   sp, sp, -32

    /* Zero the BSS so C statics behave */
    la      t0, _bssbegin
    la      t1, _bssend
    beq     t0, t1, _bss_done
    nop
_bss_loop:
    addiu   t0, 4
    bne     t0, t1, _bss_loop
    sw      zero, -4(t0)
_bss_done:

    /* Into C. spl_main() does not return. */
    j       spl_main
    nop

/* USB stage1 must preserve all BootROM registers, including o32
 * caller-saved registers. The BootROM command loop does not follow the
 * o32 calling convention. */

    .extern usb_stage1_main
    .global _usb_stage1_start
    .section .startup.usbstage1, "ax", %progbits

_usb_stage1_start:
    /* Use a separate stack outside the live BootROM window at
     * 0x80000000-0x80009000. */
    li      k0, X1600_USB_STAGE1_STACK
    addiu   k0, k0, -128

    sw      ra, 124(k0)         /* SAVE RA FIRST */
    sw      sp, 120(k0)
    sw      fp, 116(k0)
    sw      gp, 112(k0)
    sw      s7, 108(k0)
    sw      s6, 104(k0)
    sw      s5, 100(k0)
    sw      s4,  96(k0)
    sw      s3,  92(k0)
    sw      s2,  88(k0)
    sw      s1,  84(k0)
    sw      s0,  80(k0)
    sw      t9,  76(k0)
    sw      t8,  72(k0)
    sw      t7,  68(k0)
    sw      t6,  64(k0)
    sw      t5,  60(k0)
    sw      t4,  56(k0)
    sw      t3,  52(k0)
    sw      t2,  48(k0)
    sw      t1,  44(k0)
    sw      t0,  40(k0)
    sw      a3,  36(k0)
    sw      a2,  32(k0)
    sw      a1,  28(k0)
    sw      a0,  24(k0)
    sw      v1,  20(k0)
    sw      v0,  16(k0)
    sw      AT,  12(k0)
    /* k1 is scratch for us; k0 holds the save area for the whole call */

    move    sp, k0
    addiu   sp, sp, -32

    jal     usb_stage1_main
    nop

    li      k0, X1600_USB_STAGE1_STACK
    addiu   k0, k0, -128

    lw      AT,  12(k0)
    lw      v0,  16(k0)
    lw      v1,  20(k0)
    lw      a0,  24(k0)
    lw      a1,  28(k0)
    lw      a2,  32(k0)
    lw      a3,  36(k0)
    lw      t0,  40(k0)
    lw      t1,  44(k0)
    lw      t2,  48(k0)
    lw      t3,  52(k0)
    lw      t4,  56(k0)
    lw      t5,  60(k0)
    lw      t6,  64(k0)
    lw      t7,  68(k0)
    lw      t8,  72(k0)
    lw      t9,  76(k0)
    lw      s0,  80(k0)
    lw      s1,  84(k0)
    lw      s2,  88(k0)
    lw      s3,  92(k0)
    lw      s4,  96(k0)
    lw      s5, 100(k0)
    lw      s6, 104(k0)
    lw      s7, 108(k0)
    lw      gp, 112(k0)
    lw      fp, 116(k0)
    lw      sp, 120(k0)
    lw      ra, 124(k0)

    jr      ra
    nop

    .set pop
