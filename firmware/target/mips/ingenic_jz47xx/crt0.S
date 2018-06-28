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
    nop

    /*
      ----------------------------------------------------
       Invalidate BTB
      ----------------------------------------------------
    */
    mfc0    t0, C0_CONFIG
    nop
    ori     t0, 2
    mtc0    t0, C0_CONFIG
    nop
    
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
       Jump to C code
      ----------------------------------------------------
    */
    jal     system_main  /* Init clocks etc first */
    nop
    j       main
    nop

    
    /*
     * 0x0   - Simple TLB refill handler
     * 0x100 - Cache error handler
     * 0x180 - Exception/Interrupt handler
     * 0x200 - Special Exception Interrupt handler (when IV is set in CP0_CAUSE)
     */
    
    .section .vectors.1, "ax", %progbits
    j      tlb_refill_handler
    nop
    
    .section .vectors.2, "ax", %progbits
    j      real_exception_handler
    nop
    
    .section .vectors.3, "ax", %progbits
    j      real_exception_handler
    nop
    
    .section .vectors.4, "ax", %progbits
    j      real_exception_handler
    nop

    .section .vectors, "ax", %progbits
real_exception_handler:
    addiu   sp, -0x80
    sw      ra,    0(sp)
    sw      fp,    4(sp)
    sw      gp,    8(sp)
    sw      t9,  0xC(sp)
    sw      t8, 0x10(sp)
    sw      s7, 0x14(sp)
    sw      s6, 0x18(sp)
    sw      s5, 0x1C(sp)
    sw      s4, 0x20(sp)
    sw      s3, 0x24(sp)
    sw      s2, 0x28(sp)
    sw      s1, 0x2C(sp)
    sw      s0, 0x30(sp)
    sw      t7, 0x34(sp)
    sw      t6, 0x38(sp)
    sw      t5, 0x3C(sp)
    sw      t4, 0x40(sp)
    sw      t3, 0x44(sp)
    sw      t2, 0x48(sp)
    sw      t1, 0x4C(sp)
    sw      t0, 0x50(sp)
    sw      a3, 0x54(sp)
    sw      a2, 0x58(sp)
    sw      a1, 0x5C(sp)
    sw      a0, 0x60(sp)
    sw      v1, 0x64(sp)
    sw      v0, 0x68(sp)
    sw      $1, 0x6C(sp)
    mflo    k0
    nop
    sw      k0, 0x70(sp)
    mfhi    k0
    nop
    sw      k0, 0x74(sp)
    mfc0    k0, C0_STATUS
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sw      k0, 0x78(sp)
    mfc0    k0, C0_EPC
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sw      k0, 0x7C(sp)
    
    li      k1, M_CauseExcCode
    mfc0    k0, C0_CAUSE
    and     k0, k1
    beq     zero, k0, _int
    nop
    j       _exception
    nop
    
_int:
    jal     intr_handler
    nop
    j       _exception_return
    
_exception:
    move    a0, sp
    mfc0    a1, C0_CAUSE
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    mfc0    a2, C0_EPC
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    jal     exception_handler
    nop

_exception_return:
    lw      ra,    0(sp)
    lw      fp,    4(sp)
    lw      gp,    8(sp)
    lw      t9,  0xC(sp)
    lw      t8, 0x10(sp)
    lw      s7, 0x14(sp)
    lw      s6, 0x18(sp)
    lw      s5, 0x1C(sp)
    lw      s4, 0x20(sp)
    lw      s3, 0x24(sp)
    lw      s2, 0x28(sp)
    lw      s1, 0x2C(sp)
    lw      s0, 0x30(sp)
    lw      t7, 0x34(sp)
    lw      t6, 0x38(sp)
    lw      t5, 0x3C(sp)
    lw      t4, 0x40(sp)
    lw      t3, 0x44(sp)
    lw      t2, 0x48(sp)
    lw      t1, 0x4C(sp)
    lw      t0, 0x50(sp)
    lw      a3, 0x54(sp)
    lw      a2, 0x58(sp)
    lw      a1, 0x5C(sp)
    lw      a0, 0x60(sp)
    lw      v1, 0x64(sp)
    lw      v0, 0x68(sp)
    lw      $1, 0x6C(sp)
    lw      k0, 0x70(sp)
    mtlo    k0
    nop
    lw      k0, 0x74(sp)
    mthi    k0
    nop
    lw      k0, 0x78(sp)
    mtc0    k0, C0_STATUS
    nop
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    lw      k0, 0x7C(sp)
    mtc0    k0, C0_EPC
    nop
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    sll     zero, 1
    addiu   sp, 0x80
    eret
    nop
    .set reorder
    .set at
