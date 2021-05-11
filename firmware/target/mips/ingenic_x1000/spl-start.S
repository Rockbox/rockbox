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
    .extern spl_main
    .global _spl_start

    .set push
    .set mips32
    .set noreorder
    .set noat

    .section .init.spl

_spl_start:
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

    /* Clear the BSS segment (needed to zero-initialize C static values) */
    la      t0, _bssbegin
    la      t1, _bssend
    beq     t0, t1, _bss_done
_bss_loop:
    addiu   t0, 4
    bne     t0, t1, _bss_loop
    sw      zero, -4(t0)
_bss_done:

    /* Jump to C code */
    j       spl_main
    nop

    .set pop
