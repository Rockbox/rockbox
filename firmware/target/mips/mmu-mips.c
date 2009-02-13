/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include "mipsregs.h"
#include "system.h"
#include "mmu-mips.h"

#define BARRIER                            \
    __asm__ __volatile__(                  \
    "    .set    noreorder          \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    .set    reorder            \n");

#define DEFAULT_PAGE_SHIFT       PL_4K
#define DEFAULT_PAGE_MASK        PM_4K
#define UNIQUE_ENTRYHI(idx, ps)  (A_K0BASE + ((idx) << (ps + 1)))
#define ASID_MASK                M_EntryHiASID
#define VPN2_SHIFT               S_EntryHiVPN2
#define PFN_SHIFT                S_EntryLoPFN
#define PFN_MASK                 0xffffff
static void local_flush_tlb_all(void)
{
    unsigned long old_ctx;
    int entry;
    unsigned int old_irq = disable_irq_save();
    
    /* Save old context and create impossible VPN2 value */
    old_ctx = read_c0_entryhi();
    write_c0_entrylo0(0);
    write_c0_entrylo1(0);
    BARRIER;

    /* Blast 'em all away. */
    for(entry = 0; entry < 32; entry++)
    {
        /* Make sure all entries differ. */
        write_c0_entryhi(UNIQUE_ENTRYHI(entry, DEFAULT_PAGE_SHIFT));
        write_c0_index(entry);
        BARRIER;
        tlb_write_indexed();
    }
    BARRIER;
    write_c0_entryhi(old_ctx);
    
    restore_irq(old_irq);
}

static void add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
                            unsigned long entryhi,  unsigned long pagemask)
{
    unsigned long wired;
    unsigned long old_pagemask;
    unsigned long old_ctx;
    unsigned int  old_irq = disable_irq_save();
    
    old_ctx = read_c0_entryhi() & ASID_MASK;
    old_pagemask = read_c0_pagemask();
    wired = read_c0_wired();
    write_c0_wired(wired + 1);
    write_c0_index(wired);
    BARRIER;
    write_c0_pagemask(pagemask);
    write_c0_entryhi(entryhi);
    write_c0_entrylo0(entrylo0);
    write_c0_entrylo1(entrylo1);
    BARRIER;
    tlb_write_indexed();
    BARRIER;

    write_c0_entryhi(old_ctx);
    BARRIER;
    write_c0_pagemask(old_pagemask);
    local_flush_tlb_all();
    restore_irq(old_irq);
}

void map_address(unsigned long virtual, unsigned long physical,
                 unsigned long length, unsigned int cache_flags)
{
    unsigned long entry0  = (physical & PFN_MASK) << PFN_SHIFT;
    unsigned long entry1  = ((physical+length) & PFN_MASK) << PFN_SHIFT;
    unsigned long entryhi = virtual & ~VPN2_SHIFT;
    
    entry0 |= (M_EntryLoG | M_EntryLoV | (cache_flags << S_EntryLoC) );
    entry1 |= (M_EntryLoG | M_EntryLoV | (cache_flags << S_EntryLoC) );
    
    add_wired_entry(entry0, entry1, entryhi, DEFAULT_PAGE_MASK);
}

void mmu_init(void)
{
    write_c0_pagemask(DEFAULT_PAGE_MASK);
    write_c0_wired(0);
    write_c0_framemask(0);
    
    local_flush_tlb_all();
/*
    map_address(0x80000000, 0x80000000, 0x4000, K_CacheAttrC);
    map_address(0x80004000, 0x80004000, MEM * 0x100000, K_CacheAttrC);
*/
}

#define SYNC_WB() __asm__ __volatile__ ("sync")

#define __CACHE_OP(op, addr)                 \
    __asm__ __volatile__(                    \
    "    .set    noreorder        \n"        \
    "    .set    mips32\n\t       \n"        \
    "    cache   %0, %1           \n"        \
    "    .set    mips0            \n"        \
    "    .set    reorder          \n"        \
    :                                        \
    : "i" (op), "m" (*(unsigned char *)(addr)))

void __flush_dcache_line(unsigned long addr)
{
    __CACHE_OP(DCHitWBInv, addr);
    SYNC_WB();
}

void __icache_invalidate_all(void)
{
    unsigned int i;

    asm volatile (".set   noreorder  \n"
                  ".set   mips32     \n"
                  "mtc0   $0, $28    \n" /* TagLo */
                  "mtc0   $0, $29    \n" /* TagHi */
                  ".set   mips0      \n"
                  ".set   reorder    \n"
                  );
    for(i=A_K0BASE; i<A_K0BASE+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        __CACHE_OP(ICIndexStTag, i);

    /* invalidate btb */
    asm volatile (
        ".set mips32        \n"
        "mfc0 %0, $16, 7    \n"
        "nop                \n"
        "ori  %0, 2         \n"
        "mtc0 %0, $16, 7    \n"
        ".set mips0         \n"
        :
        : "r" (i));
}

void cpucache_invalidate(void)
{
    __icache_invalidate_all();
}

void __dcache_invalidate_all(void)
{
    unsigned int i;

    asm volatile (".set   noreorder  \n"
                  ".set   mips32     \n"
                  "mtc0   $0, $28    \n"
                  "mtc0   $0, $29    \n"
                  ".set   mips0      \n"
                  ".set   reorder    \n"
                  );
    for (i=A_K0BASE; i<A_K0BASE+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        __CACHE_OP(DCIndexStTag, i);
}

void __dcache_writeback_all(void)
{
    unsigned int i;
    for(i=A_K0BASE; i<A_K0BASE+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        __CACHE_OP(DCIndexWBInv, i);
    
    SYNC_WB();
}

void dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
    unsigned long end, a;

    if (size >= CACHE_SIZE)
        __dcache_writeback_all();
    else
    {
        unsigned long dc_lsize = CACHE_LINE_SIZE;
        
        a = addr & ~(dc_lsize - 1);
        end = (addr + size - 1) & ~(dc_lsize - 1);
        for(; a < end; a += dc_lsize)
            __flush_dcache_line(a);
    }
}
