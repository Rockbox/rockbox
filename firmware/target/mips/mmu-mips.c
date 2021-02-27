/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
 * Copyright (C) 2015 by Marcin Bukat
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

#if CONFIG_CPU == JZ4732 || CONFIG_CPU == JZ4760B || CONFIG_CPU == X1000
/* XBurst core has 32 JTLB entries */
#define NR_TLB_ENTRIES  32
#else
#error please define NR_TLB_ENTRIES
#endif

#define BARRIER                            \
    __asm__ __volatile__(                  \
    "    .set    push               \n"    \
    "    .set    noreorder          \n"    \
    "    ssnop                      \n"    \
    "    ssnop                      \n"    \
    "    ssnop                      \n"    \
    "    ssnop                      \n"    \
    "    ssnop                      \n"    \
    "    ssnop                      \n"    \
    "    .set    pop                \n");

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

    /* blast all entries except the wired one */
    for(entry = read_c0_wired(); entry < NR_TLB_ENTRIES; entry++)
    {
        /* Make sure all entries differ and are in unmapped space, making them
         * impossible to match */
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
}

/* Target specific operations:
 * - invalidate BTB (Branch Table Buffer)
 * - sync barrier after cache operations */
#if CONFIG_CPU == JZ4732 || CONFIG_CPU == JZ4760B || CONFIG_CPU == X1000
#define INVALIDATE_BTB()                     \
do {                                         \
        register unsigned long tmp;          \
        __asm__ __volatile__(                \
        "    .set push            \n"        \
        "    .set noreorder       \n"        \
        "    .set mips32          \n"        \
        "    mfc0 %0, $16, 7      \n"        \
        "    ssnop                \n"        \
        "    ori  %0, 2           \n"        \
        "    mtc0 %0, $16, 7      \n"        \
        "    ssnop                \n"        \
        "    .set pop             \n"        \
        : "=&r"(tmp));                       \
    } while (0)

#define SYNC_WB() __asm__ __volatile__ ("sync":::"memory")
#else /* !JZ4732 */
#define INVALIDATE_BTB() do { } while(0)
#define SYNC_WB() do { } while(0)
#endif /* CONFIG_CPU */

#define __CACHE_OP(op, addr)                 \
    __asm__ __volatile__(                    \
    "    .set    push\n\t         \n"        \
    "    .set    noreorder        \n"        \
    "    .set    mips32\n\t       \n"        \
    "    cache   %0, %1           \n"        \
    "    .set    pop              \n"        \
    :                                        \
    : "i" (op), "m"(*(unsigned char *)(addr)))

/* rockbox cache api */

/* Writeback whole D-cache
 * Alias to commit_discard_dcache() as there is no index type
 * variant of writeback-only operation
 */
void commit_dcache(void) __attribute__((alias("commit_discard_dcache")));

/* Writeback whole D-cache and invalidate D-cache lines */
void commit_discard_dcache(void)
{
    register unsigned int i;

    /* Use index type operation and iterate whole cache */
    for (i=A_K0BASE; i<A_K0BASE+CACHE_SIZE; i+=CACHEALIGN_SIZE)
        __CACHE_OP(DCIndexWBInv, i);

    SYNC_WB();
}

/* Writeback lines of D-cache corresponding to address range and
 * invalidate those D-cache lines
 */
void commit_discard_dcache_range(const void *base, unsigned int size)
{
    char *ptr = CACHEALIGN_DOWN((char*)base);
    char *end = CACHEALIGN_UP((char*)base + size);

    for(; ptr != end; ptr += CACHEALIGN_SIZE)
        __CACHE_OP(DCHitWBInv, ptr);

    SYNC_WB();
}

/* Writeback lines of D-cache corresponding to address range
 */
void commit_dcache_range(const void *base, unsigned int size)
{
    char *ptr = CACHEALIGN_DOWN((char*)base);
    char *end = CACHEALIGN_UP((char*)base + size);

    for(; ptr != end; ptr += CACHEALIGN_SIZE)
        __CACHE_OP(DCHitWB, ptr);

    SYNC_WB();
}

/* Invalidate D-cache lines corresponding to address range
 * WITHOUT writeback
 */
void discard_dcache_range(const void *base, unsigned int size)
{
    char *ptr = CACHEALIGN_DOWN((char*)base);
    char *end = CACHEALIGN_UP((char*)base + size);

    /* If the start of the buffer is unaligned, write
       back that cacheline and shrink up the region
       to discard. */
    if (base != ptr) {
        __CACHE_OP(DCHitWBInv, ptr);
        ptr += CACHEALIGN_SIZE;
    }

    /* If the end of the buffer is unaligned, write back that
       cacheline and shrink down the region to discard. */
    if (ptr != end && (end !=((char*)base + size))) {
        end -= CACHEALIGN_SIZE;
        __CACHE_OP(DCHitWBInv, end);
    }

    /* Finally, discard whatever is left */
    for(; ptr != end; ptr += CACHEALIGN_SIZE)
        __CACHE_OP(DCHitInv, ptr);

    SYNC_WB();
}

/* Invalidate whole I-cache */
static void discard_icache(void)
{
    register unsigned int i;

    asm volatile (".set   push       \n"
                  ".set   noreorder  \n"
                  ".set   mips32     \n"
                  "mtc0   $0, $28    \n" /* TagLo */
                  "mtc0   $0, $29    \n" /* TagHi */
                  ".set   pop        \n"
                  );
    /* Use index type operation and iterate whole cache */
    for (i=A_K0BASE; i<A_K0BASE+CACHE_SIZE; i+=CACHEALIGN_SIZE)
        __CACHE_OP(ICIndexStTag, i);

    INVALIDATE_BTB();
}

/* Invalidate the entire I-cache
 * and writeback + invalidate the entire D-cache
 */
void commit_discard_idcache(void)
{
    commit_discard_dcache();
    discard_icache();
}
