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
 * Copyright (C) 2016 by Amaury Pouly
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

/* Boot overview:
 *
 * We assume the cache is already configured and flushed. We start
 * with some basic mips init and then relocate the image in memory if necessary.
 * We then put the iram image at the right place, flush all caches, clear bss,
 * munge the stack and jump to C code.
 */

.section .init.text
.set mips32
.set noreorder
.set noat
.global _start
_start:
    bltzal zero, load_addr      /* ra = PC + 8, branch not taken */
    nop
load_addr:
    addiu   v0, ra, -8          /* calc real load address account for branch delay slot */

    /* unmask all hardware interrupt source, but disable interrupts globally */
    li      t0, (0xfc << S_StatusIM)
    mtc0    t0, C0_STATUS

    /* use the special interrupt vector */
    li      t0, M_CauseIV
    mtc0    t0, C0_CAUSE

    /* relocate if necessary */
    la      t0, _relocstart
    la      t1, _relocend
    beq     t0, v0, real_start  /* no relocation needed */
    nop
reloc_loop:
    lw     t3, 0(v0)            /* v0 is source */
    addiu  v0, 4
    addiu  t0, 4
    bne    t0, t1, reloc_loop
    sw     t3, -4(t0)

    /* commit dcache and invalidate icache */
    jal     flush_caches
    nop

    /* jump to real start address */
    la      t0, real_start
    jr      t0
    nop

real_start:
    /* setup memory
     * NOTE: since the final memory layout is not available, we need a different
     * stack address that is valid */
    la      sp, crt0_tmpstack_phys
    jal     memory_init
    nop
    j       hwstub
    nop

    /* copy iram image to the right place */
    la     t0, _iramcopy
    la     t1, _iramstart
    la     t2, _iramend
iram_loop:
    lw     t3, 0(t0)
    addiu  t1, 4
    addiu  t0, 4
    bne    t1, t2, iram_loop
    sw     t3, -4(t1)

    /* clear bss */
    la     t0, _edata
    la     t1, _end
bss_loop:
    addiu  t0, 4
    bne    t0, t1, bss_loop
    sw     zero, -4(t0)

    /* munge the stack */
    la      sp, stackend
    la      t0, stackbegin
    li      t2, 0xdeadbeef
stack_loop:
    addiu   t0, 4
    bne     t0, sp, stack_loop
    sw      t2, -4(t0)

    /* commit dcache and invalidate icache */
    jal     flush_caches
    nop

    j       hwstub
    nop
    /* jump to C code */
    j       main
    nop

/* commit and discard dcache, discard icache */
flush_caches:
    la      t0, 0x80000000   /* an idx op should use an unmappable address */
    ori     t1, t0, 0x4000   /* 16kB cache */
dcache_loop:
    cache   DCIndexWBInv, 0(t0) /* invalidate and write-back dcache index */
    addiu   t0, t0, 0x20     /* 32 bytes per cache line */
    bne     t0, t1, dcache_loop
    nop
    la      t0, 0x80000000   /* an idx op should use an unmappable address */
    ori     t1, t0, 0x4000   /* 16kB cache */
icache_loop:
    cache   ICIndexInv, 0(t0) /* invalidate icache index */
    addiu   t0, t0, 0x20     /* 32 bytes per cache line */
    bne     t0, t1, icache_loop
    nop
    /* return */
    jr      ra
    nop

hwstub:
    .incbin "/home/pamaury/project/rockbox/myrockbox/utils/hwstub/stub/jz4760b/build/hwstub.bin"

die_blink:
    /* setup pin */
    la      a0, 0xb0010400
    li      a1, 2
    sw      a1, 0x48(a0) /* clear function (gpio or interrupt) */
    sw      a1, 0x58(a0) /* clear select (gpio) */
    sw      a1, 0x64(a0) /* set direction (out) */
    sw      a1, 0x34(a0) /* set pull (disable) */
    /* turn backlight on and off */
    la      a0, 0xb0010414
    li      a1, 2
.blink_loop:
    sw      a1, (a0)
    la      v0, 10000000
.wait:
    bnez    v0, .wait
    subu    v0, 1
    sw      a1, 4(a0)
    la      v0, 10000000
.wait2:
    bnez    v0, .wait2
    subu    v0, 1
    j       .blink_loop
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
