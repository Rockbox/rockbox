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
#include "jz4760b.h"

#include "mips_lib.S"

/* This code expects to be run either in K0 or K1, but preferrably in K0 because relocations address
 * is in K0 so running from K1 will incur a relocation penalty. Thus we also assumes that the
 * cache is initialized, which is alsways the case because it is done with the IPL and SPL */
.section .init.text
.set mips32
.set noreorder
.set noat
.global _start
_start:
    save_cur_pc k0
    /* disable all interrupts */
    disable_interrupts
enable_big_pages_mode:
    /* we need to enable some undocumented feature to use big pages and standard JTLB */
    li      t0, 0xa9000000
    mtc0    t0, $5, 4 /* big page mode register */
init_mmu:
    /* init MMU (see jz4760b.h for details) */
    mmu_init
map_dram:
    /* map dram as cacheable using 16M pages */
    map_address DRAM_ORIG, PHYSICAL_DRAM_ADDR, DRAM_SIZE, K_CacheAttrCN, (16 * 1024 * 1024)
relocate:
    /* relocate binary, note that addresses are in K0 */
    copy_code   k0, _relocstart, _relocend
copy_iram:
    /* copy iram content to its correct location
     * NOTE: it works because _iram{copy,end} are in k0, not in k2*/
    la      t0, _iramcopy
    copy_code   t0, _iramstart, _iramend
    /* jump to real start address */
    la      t0, real_start
    jr      t0
    nop
real_start:
bss_clear:
    /* clear BSS */
    clear_mem   _edata, _end
setup_stack:
    /* munge the stack */
    la      sp, stackend
    la      t0, stackbegin
    li      t2, 0xdeadbeef
1:
    addiu   t0, 4
    bne     t0, sp, 1b
    sw      t2, -4(t0)
call_main:
    /* return */
    j       main
    nop

.section .text.hwstub
.global die_hwstub
die_hwstub:
.incbin "/home/pamaury/project/rockbox/myrockbox/utils/hwstub/stub/jz4760b/build/hwstub.bin"

/*
 * 0x0   - Simple TLB refill handler
 * 0x100 - Cache error handler
 * 0x180 - Exception/Interrupt handler
 * 0x200 - Special Exception Interrupt handler (when IV is set in CP0_CAUSE)
 */
.section .vectors.tlb_refill, "ax", %progbits
    j      tlb_refill_handler
    nop

.section .vectors.cache_error, "ax", %progbits
    j      real_exception_handler
    nop

.section .vectors.exception, "ax", %progbits
    j      real_exception_handler
    nop

.section .vectors.interrupt, "ax", %progbits
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
