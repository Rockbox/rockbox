/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

/*
 * init.S
 *
 * Initialization code for JzRISC.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "config.h"
#include "mips.h"

    .text

    .extern system_main
    .extern main
    .global _start

    .section .init.text
    .set    push
    .set    mips32
    .set    noreorder
    .set    noat

#ifdef BOOTLOADER
#ifndef XDUOO_X3
    /* These will get filled in by scramble */
    .word 0 /* Empty    */
    .word 0 /* Filesize */

    /* Relocate bootloader */
    la     t0, (_loadaddress-0xE00000)
    la     t1, _loadaddress
    la     t2, _bootend
_relocate_loop:
    lw     t3, 0(t0)
    addiu  t1, 4
    addiu  t0, 4
    bne    t1, t2, _relocate_loop
    sw     t3, -4(t1)
#endif
#endif

_start:
    la      ra, _start
    /*
      ----------------------------------------------------
       Init CP0 registers.
      ----------------------------------------------------
    */
    mtc0    zero, C0_WATCHLO
    mtc0    zero, C0_WATCHHI

    li      t0, (M_StatusBEV  | M_StatusIM7 | M_StatusIM6 \
                | M_StatusIM5 | M_StatusIM4 | M_StatusIM3 \
                | M_StatusIM2 | M_StatusERL)
             /*
                 BEV = Enable Boot Exception Vectors
                 IMx = Interrupt mask
                 ERL = Denotes error level
             */
    mtc0    t0, C0_STATUS

    li      t0, M_CauseIV
    mtc0    t0, C0_CAUSE

    /*
      ----------------------------------------------------
       Init caches, assumes a 4way*128set*32byte I/D cache
      ----------------------------------------------------
    */
    li      t0, 3            # enable cache for kseg0 accesses
    mtc0    t0, C0_CONFIG    # CONFIG reg
    la      t0, 0x80000000   # an idx op should use an unmappable address
    ori     t1, t0, 0x4000   # 16kB cache
    mtc0    zero, C0_TAGLO   # TAGLO reg
    mtc0    zero, C0_TAGHI   # TAGHI reg

_cache_loop:
    cache   0x8, 0(t0)       # index store icache tag
    cache   0x9, 0(t0)       # index store dcache tag
    addiu   t0, t0, 0x20     # 32 bytes per cache line
    bne     t0, t1, _cache_loop
    ssnop

    /*
      ----------------------------------------------------
       Invalidate BTB
      ----------------------------------------------------
    */
    mfc0    t0, C0_CONFIG
    ssnop
    ori     t0, 2
    mtc0    t0, C0_CONFIG
    ssnop

    /*
      ----------------------------------------------------
       Copy IRAM section
        * copy IRAM first before BSS gets cleared, as both
          have the same address
      ----------------------------------------------------
    */
    la     t0, _iramcopy
    la     t1, _iramstart
    la     t2, _iramend
_iram_loop:
    lw     t3, 0(t0)
    addiu  t1, 4
    addiu  t0, 4
    bne    t1, t2, _iram_loop
    sw     t3, -4(t1)

    /*
      ----------------------------------------------------
       Clear BSS section
      ----------------------------------------------------
    */
    la     t0, _edata
    la     t1, _end
_bss_loop:
    addiu  t0, 4
    bne    t0, t1, _bss_loop
    sw     zero, -4(t0)

    /*
      ----------------------------------------------------
       Set up stack
      ----------------------------------------------------
    */
    la      sp, stackend
    la      t0, stackbegin
    li      t2, 0xDEADBEEF

_stack_loop:
    addiu   t0, 4
    bne     t0, sp, _stack_loop
    sw      t2, -4(t0)

    /*
      ----------------------------------------------------
       Set up interrupt stack
      ----------------------------------------------------
    */
    la     k0, _irqstackend
    la     t0, _irqstackbegin

_irq_stack_loop:
    addiu  t0, 4
    bne    t0, k0, _irq_stack_loop
    sw     t2, -4(t0)

    /*
      ----------------------------------------------------
       Jump to C code
      ----------------------------------------------------
    */
    jal     system_main  /* Init clocks etc first */
    ssnop
    j       main
    move    ra, zero     /* init backtrace root */

    .set pop
