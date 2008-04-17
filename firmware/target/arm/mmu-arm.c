/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006,2007 by Greg White
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "mmu-arm.h"
#include "panic.h"

#define SECTION_ADDRESS_MASK (-1 << 20)
#define MB (1 << 20)

void ttb_init(void) {
    unsigned int* ttbPtr;

    /* must be 16Kb (0x4000) aligned - clear out the TTB */
    for (ttbPtr=TTB_BASE; ttbPtr<(TTB_SIZE+TTB_BASE); ttbPtr++)
    {
        *ttbPtr = 0;
    }

    /* Set the TTB base address */
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r" (TTB_BASE));

    /* Set all domains to manager status */
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r" (0xFFFFFFFF));
}

void map_section(unsigned int pa, unsigned int va, int mb, int cache_flags) {
    unsigned int* ttbPtr;
    int i;
    int section_no;

    section_no = va >> 20; /* sections are 1Mb size */
    ttbPtr = TTB_BASE + section_no;
    pa &= SECTION_ADDRESS_MASK; /* align to 1Mb */
    for(i=0; i<mb; i++, pa += MB) {
        *(ttbPtr + i) =
            pa |
            1 << 10 |    /* superuser - r/w, user - no access */
            0 << 5 |    /* domain 0th */
            1 << 4 |    /* should be "1" */
            cache_flags |
            1 << 1;    /* Section signature */
    }
}

void enable_mmu(void) {
    int regread;

    asm volatile(
        "MRC p15, 0, %r0, c1, c0, 0\n"  /* Read reg1, control register */
    : /* outputs */
        "=r"(regread)
    : /* inputs */
    : /* clobbers */
        "r0"
    );

    if ( !(regread & 0x04) || !(regread & 0x00001000) ) /* Was the ICache or DCache Enabled? */
        clean_dcache(); /* If so we need to clean the DCache before invalidating below */

    asm volatile("mov r0, #0\n"
        "mcr p15, 0, r0, c8, c7, 0\n" /* invalidate TLB */

        "mcr p15, 0, r0, c7, c7,0\n" /* invalidate both icache and dcache */

        "mrc p15, 0, r0, c1, c0, 0\n"
        "orr r0, r0, #1<<0\n" /* enable mmu bit, icache and dcache */
        "orr r0, r0, #1<<2\n" /* enable dcache */
        "orr r0, r0, #1<<12\n" /* enable icache */
        "mcr p15, 0, r0, c1, c0, 0" : : : "r0");
    asm volatile("nop \n nop \n nop \n nop");
}

#if CONFIG_CPU == IMX31L
void __attribute__((naked)) invalidate_dcache_range(const void *base, unsigned int size)
{
    asm volatile(
        "add    r1, r1, r0             \n"
        "mov    r2, #0                 \n"
        "mcrr   p15, 0, r1, r0, c14    \n" /* Clean and invalidate dcache range */
        "mcr    p15, 0, r2, c7, c10, 4 \n" /* Data synchronization barrier */
        "bx     lr                     \n"
    );
    (void)base; (void)size;
}
#else
/* Invalidate DCache for this range  */
/* Will do write back */
void invalidate_dcache_range(const void *base, unsigned int size) {
    unsigned int addr = (((int) base) & ~31);    /* Align start to cache line*/
    unsigned int end = ((addr+size) & ~31)+64;  /* Align end to cache line, pad */
    asm volatile(
"inv_start: \n"
    "mcr p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "add %0, %0, #32 \n"
    "cmp %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "mcrne p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "addne %0, %0, #32 \n"
    "cmpne %0, %1 \n"
    "bne inv_start \n"
    "mov %0, #0\n"
    "mcr p15,0,%0,c7,c10,4\n"    /* Drain write buffer */
        : : "r" (addr), "r" (end));
}
#endif


#if CONFIG_CPU == IMX31L
void __attribute__((naked)) clean_dcache_range(const void *base, unsigned int size)
{
    asm volatile(
        "add    r1, r1, r0             \n"
        "mov    r2, #0                 \n"
        "mcrr   p15, 0, r1, r0, c12    \n" /* Clean dcache range */
        "mcr    p15, 0, r2, c7, c10, 4 \n" /* Data synchronization barrier */
        "bx     lr                     \n"
    );
    (void)base; (void)size;
}
#else
/* clean DCache for this range  */
/* forces DCache writeback for the specified range */
void clean_dcache_range(const void *base, unsigned int size) {
    unsigned int addr = (int) base;
    unsigned int end = addr+size+32;
    asm      volatile(
    "bic %0, %0, #31 \n"
"clean_start: \n"
    "mcr p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "add %0, %0, #32 \n"
    "cmp %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "mcrlo p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "addlo %0, %0, #32 \n"
    "cmplo %0, %1 \n"
    "blo clean_start \n"
    "mov %0, #0\n"
    "mcr p15,0,%0,c7,c10,4 \n"    /* Drain write buffer */
        : : "r" (addr), "r" (end));
}
#endif

#if CONFIG_CPU == IMX31L
void __attribute__((naked)) dump_dcache_range(const void *base, unsigned int size)
{
    asm volatile(
        "add    r1, r1, r0         \n"
        "mcrr   p15, 0, r1, r0, c6 \n"
        "bx     lr                 \n"
    );
    (void)base; (void)size;
}
#else
/* Dump DCache for this range  */
/* Will *NOT* do write back */
void dump_dcache_range(const void *base, unsigned int size) {
    unsigned int addr = (int) base;
    unsigned int end = addr+size;
    asm volatile(
    "tst %0, #31 \n"                    /* Check to see if low five bits are set */
    "bic %0, %0, #31 \n"                /* Clear them */
    "mcrne p15, 0, %0, c7, c14, 1 \n"     /* Clean and invalidate this line, if those bits were set */
    "add %0, %0, #32 \n"                /* Move to the next cache line */
    "tst %1, #31 \n"                    /* Check last line for bits set */
    "bic %1, %1, #31 \n"                /* Clear those bits */
    "mcrne p15, 0, %1, c7, c14, 1 \n"     /* Clean and invalidate this line, if not cache aligned */
"dump_start: \n"
    "mcr p15, 0, %0, c7, c6, 1 \n"         /* Invalidate this line */
    "add %0, %0, #32 \n"                /* Next cache line */
    "cmp %0, %1 \n"
    "bne dump_start \n"
"dump_end: \n"
    "mcr p15,0,%0,c7,c10,4 \n"    /* Drain write buffer */
        : : "r" (addr), "r" (end));
}
#endif

#if CONFIG_CPU == IMX31L
void __attribute__((naked)) clean_dcache(void)
{
    asm volatile (
        /* Clean entire data cache */
        "mov    r0, #0                 \n"
        "mcr    p15, 0, r0, c7, c10, 0 \n"
        "bx     lr                     \n"
    );
}
#else
/* Cleans entire DCache */
void clean_dcache(void)
{
    unsigned int index, addr;

    for(index = 0; index <= 63; index++) {
        addr = (0 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (1 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (2 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (3 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (4 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (5 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (6 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
        addr = (7 << 5) | (index << 26);
        asm volatile(
            "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
            : : "r" (addr));
    }
}
#endif

