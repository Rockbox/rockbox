/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 by Marcin Bukat
 * Copyright (C) 2017 by Amaury Pouly
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

/** The goal of this file is to serve as library for target-specific code, to avoid duplicating
 * standard code but keeping enough flexibility since ech platform is slightly different. This is
 * achieved by creating macros that can be called from crt0.S to insert code. This file tries to
 * make as little assumption about the MIPS32 release as possible, except for the standard defines
 * in config.h
 *
 * IMPORTANT NOTE: all those macros must only use temporary registers, all others are preserved */

#include "config.h"
#include "mips.h"
#include "cpu.h"

#ifndef MIPS32_RELEASE
#error You need to define MIPS32_RELEASE
#endif

/* wait N cycles, needed to clear hazard on pre-MIPS32r2 platforms */
.macro wait_cycles cycles
    .rept \cycles
        ssnop /* be of the safe side, if we ever have a superscalar processor */
    .endr
.endm

/* clear execution hazard */
.macro clear_hazard
#if MIPS32_RELEASE <2 
    wait_cycles MIPS32_HAZARD_CYCLES
#endif
    ehb /* ehb is nop on old architectures */
.endm


/* disable interrupts */
.macro disable_interrupts
#if MIPS32_RELEASE >= 2
    di
#else
    mfc0    t0, C0_STATUS
    srl     t0, t0, 1
    sll     t0, t0, 1
    mtc0    t0, C0_STATUS
    clear_hazard
#endif
.endm

/* on processor that do not support extract, implement it as a macro */
.macro ext  dest, src, pos, width
    .if \pos + \width != 32
        sll     \dest, \src, (32 - \pos - \width)
    .endif
    srl     \dest, \dest, (32 - \width)
.endm

/* initialize the MMU to a know state
 * WARNING: on some CPUs like XBurst, initializing the TLB in the reverse order (entry 31 to 0)
 * cause a TLB shutdown... */
.macro mmu_init
    mfc0    t0, C0_Config1
    ext     t3, t0, 25, 6      # extract number of TLB entries

    mtc0    zero, C0_EntryLo0
    mtc0    zero, C0_EntryLo1
    mtc0    zero, C0_PageMask
    mtc0    zero, C0_Wired
    li      t2, 0x80000000
    li      t1, 0

1:
    mtc0    t1, C0_Index       # store TLB index
    mtc0    t2, C0_EntryHi
    clear_hazard               # clear hazard
    tlbwi                      # write TLB entry
    addiu   t2, t2, 0x2000     # 4k * 2 per entry
    bne     t1, t3, 1b
    addiu   t1, t1, 1
.endm

/* map some range of addresses using wired entries of the MMU, this macro assumes all parameters
 * are constants, this allows to unroll the loop; it also assumes virtual and physical addresses are
 * page aligned */
.macro map_address  virtual, physical, length, cache_flags, pagesize
    mfc0    t0, C0_Wired       # get wired entry index
    li      t1, (((\pagesize << 1) - 1) & M_PageMaskMask)
    mtc0    t1, C0_PageMask
    li      t1, (\virtual & M_EntryHiVPN2)  # entryhi
    li      t2, (\pagesize << 1)            # entryhi's increment (2 * pagesize)
    pfn = \physical >> (S_EntryHiVPN2 - 1)
    ent_low = \cache_flags << S_EntryLoC | M_EntryLoV | M_EntryLoD | M_EntryLoG
    li      t3, (pfn << S_EntryLoPFN | ent_low) # entrylo
    li      t4, (\pagesize >> (S_EntryHiVPN2 - 1)) << S_EntryLoPFN  # entrylo's increment (pagesize)

    /* each entry cover 2*pagesize */
    pagecount = (\length + \pagesize - 1) / \pagesize
    .rept ((pagecount + 1) / 2)
        mtc0    t0, C0_Index
        addiu   t0, t0, 1
        mtc0    t1, C0_EntryHi      # write high entry
        addu    t1, t1, t2          # increment PFN by pagesize * 2
        mtc0    t3, C0_EntryLo0     # write low entry0
        .if pagecount == 1
            li      t3, M_EntryLoG      # clear second entry if we need an odd number of pages
                                        # but make sure to keep G set to keep the JTLB entry global
        .else
            addu    t3, t3, t4          # increment PFN by pagesize
        .endif
        mtc0    t3, C0_EntryLo1     # write low entry1
        addu    t3, t3, t4          # increment PFN by pagesize
        clear_hazard
        tlbwi                       # write mapping
        pagecount = (pagecount - 2)
    .endr

    mtc0    t0, C0_Wired        # update number of wired entries
    clear_hazard
.endm

/* save current address into a register */
.macro save_cur_pc  reg
    bltzal  zero, 1f      /* ra = PC + 8, branch not taken */
    nop
1:
    addiu   \reg, ra, -8          /* calc real load address account for branch delay slot */
.endm

/* invalidate icache */
.macro invalidate_icache
    /* commit dcache and invalidate icache */
    la      t0, 0x80000000   /* an idx op should use an unmappable address */
    ori     t1, t0, CACHE_SIZE   /* cache size */
1:
    cache   ICIndexInv, 0(t0) /* invalidate icache index */
    bne     t0, t1, 1b
    addiu   t0, t0, CACHEALIGN_SIZE  /* bytes per cache line */
.endm

/* writeback and discard icache */
.macro writeback_dcache
    /* commit dcache and invalidate icache */
    la      t0, 0x80000000   /* an idx op should use an unmappable address */
    ori     t1, t0, CACHE_SIZE   /* cache size */
1:
    cache   DCIndexWBInv, 0(t0) /* invalidate and write-back dcache index */
    bne     t0, t1, 1b
    addiu   t0, t0, CACHEALIGN_SIZE /* bytes per cache line */
.endm

/* move a binary blob to another place, nop if binary is already at the right place,
 * this macro expects cur_addr to be a register and wanted_addr(_end) to be symbols, wanted_add
 * must be cache aligned */
.macro copy_mem cur_addr, wanted_addr, wanted_addr_end
    move    a0, \cur_addr
    la      a1, \wanted_addr
    la      a2, \wanted_addr_end
    beq     a0, a1, 3f  /* no relocation needed */
    nop
    /* if wanted < cur, do a forward copy */
    sltu    t0, a1, a0
    bnez    t0, 2f
    /* backward copy, adjust a0 to point at the end */
    addu    a0, a2
    subu    a0, a1
1:
    copy_cache_line a0, a2, -CACHEALIGN_SIZE
    subu    a2, CACHEALIGN_SIZE
    slt     t0, a1, a2
    bnez    t0, 1b
    subu    a0, CACHEALIGN_SIZE
    b       3f
    /* forward copy */
2:
    copy_cache_line a0, a1, 0
    addiu   a1, CACHEALIGN_SIZE
    slt     t0, a1, a2
    bnez    t0, 2b
    addiu   a0, CACHEALIGN_SIZE
3:
.endm

/* copy an entire cache line from one address to another, which should be held in registers.
 * this macros uses all temporary registers as intermediates */
.macro copy_cache_line from, to, offset
#if CACHEALIGN_SIZE != 16 && CACHEALIGN_SIZE != 32
#error the code below does not support this cache line size
#endif
#if CACHEALIGN_SIZE >= 16
    lw      t0, (\offset + 0)(\from)
    lw      t1, (\offset + 4)(\from)
    lw      t2, (\offset + 8)(\from)
    lw      t3, (\offset + 12)(\from)
#endif
#if CACHEALIGN_SIZE >= 32
    lw      t4, (\offset + 16)(\from)
    lw      t5, (\offset + 20)(\from)
    lw      t6, (\offset + 24)(\from)
    lw      t7, (\offset + 28)(\from)
#endif
#if CACHEALIGN_SIZE >= 16
    sw      t0, (\offset + 0)(\to)
    sw      t1, (\offset + 4)(\to)
    sw      t2, (\offset + 8)(\to)
    sw      t3, (\offset + 12)(\to)
#endif
#if CACHEALIGN_SIZE >= 32
    sw      t4, (\offset + 16)(\to)
    sw      t5, (\offset + 20)(\to)
    sw      t6, (\offset + 24)(\to)
    sw      t7, (\offset + 28)(\to)
#endif
.endm

/* clear a range of memory
 * this macro expects start/end_addr to be symbols */
.macro clear_mem start_addr, end_addr
    la      a0, \start_addr
    la      a1, \end_addr
    beq     a0, a1, 2f  /* no relocation needed */
    nop
    /* clear 4 bytes at a time */
1:
    sw      zero, (a0)
    bne     a0, a1, 1f
    addiu   a0, 4
2:
.endm
