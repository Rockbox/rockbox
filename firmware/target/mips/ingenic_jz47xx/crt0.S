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
    
    .set mips3

    .extern main

    .global    _start
#ifdef BOOTLOADER
    .section .init.text,"ax",%progbits
#else
    .section .resetvectors,"ax",%progbits
#endif
    .set    noreorder
    .set    noat
    
#ifdef BOOTLOADER
    .word 0 /* HACK */
    .word 0 /* HACK */
#endif
_start:
    la      ra, _start
    //----------------------------------------------------
    // init cp0 registers.
    //----------------------------------------------------
    mtc0    zero, C0_WATCHLO
    mtc0    zero, C0_WATCHHI

    li      t0, (M_StatusBEV  | M_StatusIM7 | M_StatusIM6 \
                | M_StatusIM5 | M_StatusIM4 | M_StatusIM3 \
                | M_StatusIM2 | M_StatusERL)
             //  BEV = Enable Boot Exception Vectors
             //  IMx = Interrupt mask
             //  ERL = Denotes error level
    mtc0    t0, C0_STATUS

    li      t1, M_CauseIV
    mtc0    t1, C0_CAUSE

    //----------------------------------------------------
    // init caches, assumes a 4way*128set*32byte i/d cache
    //----------------------------------------------------
    li      t0, 3            // enable cache for kseg0 accesses
    mtc0    t0, C0_CONFIG    // CONFIG reg
    la      t0, 0x80000000   // an idx op should use a unmappable address
    ori     t1, t0, 0x4000   // 16kB cache
    mtc0    zero, C0_TAGLO   // TAGLO reg
    mtc0    zero, C0_TAGHI   // TAGHI reg

_init_cache_loop:    
    cache   0x8, 0(t0)       // index store icache tag
    cache   0x9, 0(t0)       // index store dcache tag
    bne     t0, t1, _init_cache_loop
    addiu   t0, t0, 0x20     // 32 bytes per cache line
    nop

    //----------------------------------------------------
    // Invalidate BTB
    //----------------------------------------------------
    mfc0    t0, C0_CONFIG
    nop
    ori     t0, 2
    mtc0    t0, C0_CONFIG
    nop

    //----------------------------------------------------
    // setup stack, jump to C code
    //----------------------------------------------------
    la      sp, stackend
    la      t0, stackbegin
    li      t1, 0xDEADBEEF
    
_init_stack_loop:    
    sw      t1, 0(t0)
    bne     t0, sp, _init_stack_loop
    addiu   t0, t0, 4

    la      t0, main
    jr      t0
    nop

    
#ifndef BOOTLOADER
    .section .vectors,"ax",%progbits
#endif
    .extern exception_handler
    .global except_common_entry
    .type   except_common_entry,@function
except_common_entry:
    la     k0, exception_handler
    jr     k0
    nop
    nop
    nop

    .extern _int
    .extern _exception
    .global exception_handler
    .type   exception_handler,@function
    .set noreorder
exception_handler:


    addiu   sp, -0x80      # Add Immediate Unsigned
    sw      ra, 0(sp)      # Store Word
    sw      fp, 4(sp)      # Store Word
    sw      gp, 8(sp)      # Store Word
    sw      t9, 0xC(sp)    # Store Word
    sw      t8, 0x10(sp)   # Store Word
    sw      s7, 0x14(sp)   # Store Word
    sw      s6, 0x18(sp)   # Store Word
    sw      s5, 0x1C(sp)   # Store Word
    sw      s4, 0x20(sp)   # Store Word
    sw      s3, 0x24(sp)   # Store Word
    sw      s2, 0x28(sp)   # Store Word
    sw      s1, 0x2C(sp)   # Store Word
    sw      s0, 0x30(sp)   # Store Word
    sw      t7, 0x34(sp)   # Store Word
    sw      t6, 0x38(sp)   # Store Word
    sw      t5, 0x3C(sp)   # Store Word
    sw      t4, 0x40(sp)   # Store Word
    sw      t3, 0x44(sp)   # Store Word
    sw      t2, 0x48(sp)   # Store Word
    sw      t1, 0x4C(sp)   # Store Word
    sw      t0, 0x50(sp)   # Store Word
    sw      a3, 0x54(sp)   # Store Word
    sw      a2, 0x58(sp)   # Store Word
    sw      a1, 0x5C(sp)   # Store Word
    sw      a0, 0x60(sp)   # Store Word
    sw      v1, 0x64(sp)   # Store Word
    sw      v0, 0x68(sp)   # Store Word
    sw      $1, 0x6C(sp)   # Store Word
    mflo    t0             # Move F LO
    nop
    sw      t0, 0x70(sp)   # Store Word
    mfhi    t0             # Move F HI
    nop
    sw      t0, 0x74(sp)   # Store Word
    mfc0    t0, C0_STATUS  # Status register
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sw      t0, 0x78(sp)   # Store Word
    mfc0    t0, C0_EPC     # Exception Program Counter
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sw      t0, 0x7C(sp)   # Store Word
    li      k1, 0x7C       # Load Immediate
    mfc0    k0, C0_CAUSE   # C0_CAUSE of last exception
    and     k0, k1         # AND
    beq     zero, k0, _int # Branch on Equal
    nop
    la    k0, _exception
    jr    k0
    nop
    
    .global _int
    .type    _int,@function
_int:
    jal     intr_handler   # Jump And Link
    nop
    lw      ra, 0(sp)      # Load Word
    lw      fp, 4(sp)      # Load Word
    sw      gp, 8(sp)      # Store Word
    lw      t9, 0xC(sp)    # Load Word
    lw      t8, 0x10(sp)   # Load Word
    lw      s7, 0x14(sp)   # Load Word
    lw      s6, 0x18(sp)   # Load Word
    lw      s5, 0x1C(sp)   # Load Word
    lw      s4, 0x20(sp)   # Load Word
    lw      s3, 0x24(sp)   # Load Word
    lw      s2, 0x28(sp)   # Load Word
    lw      s1, 0x2C(sp)   # Load Word
    lw      s0, 0x30(sp)   # Load Word
    lw      t7, 0x34(sp)   # Load Word
    lw      t6, 0x38(sp)   # Load Word
    lw      t5, 0x3C(sp)   # Load Word
    lw      t4, 0x40(sp)   # Load Word
    lw      t3, 0x44(sp)   # Load Word
    lw      t2, 0x48(sp)   # Load Word
    lw      t1, 0x4C(sp)   # Load Word
    lw      t0, 0x50(sp)   # Load Word
    lw      a3, 0x54(sp)   # Load Word
    lw      a2, 0x58(sp)   # Load Word
    lw      a1, 0x5C(sp)   # Load Word
    lw      a0, 0x60(sp)   # Load Word
    lw      v1, 0x64(sp)   # Load Word
    lw      v0, 0x68(sp)   # Load Word
    lw      v1, 0x6C(sp)   # Load Word
    lw      k0, 0x70(sp)   # Load Word
    mtlo    k0             # Move To LO
    nop
    lw      k0, 0x74(sp)   # Load Word
    mthi    k0             # Move To HI
    nop
    lw      k0, 0x78(sp)   # Load Word
    nop
    mtc0    k0, C0_STATUS  # Status register
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    lw      k0, 0x7C(sp)   # Load Word
    nop
    mtc0    k0, C0_EPC     # Exception Program Counter
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    sll     zero, 1        # Shift Left Logical
    addiu   sp, 0x80       # Add Immediate Unsigned
    eret                   # Exception Return
    nop
    
    .extern _except_handler
    .global _exception
    .type   _exception,@function
_exception:
    move    a0, sp
    mfc0    a1, C0_CAUSE       # C0_CAUSE of last exception
    mfc0    a2, C0_EPC         # Exception Program Counter
    la      k0, except_handler # Load Address
    jr      k0                 # Jump Register
    nop

    .set reorder
