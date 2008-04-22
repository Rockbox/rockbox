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

void __attribute__((naked)) ttb_init(void) {
    asm volatile
    (
        "mcr p15, 0, %[ttbB], c2, c0, 0 \n" /* Set the TTB base address */
        "mcr p15, 0, %[ffff], c3, c0, 0 \n" /* Set all domains to manager status */
        "bx  lr                         \n"
        : 
        :   [ttbB] "r" (TTB_BASE), 
            [ffff] "r" (0xFFFFFFFF)
    );
}

void __attribute__((naked)) map_section(unsigned int pa, unsigned int va, int mb, int flags) {
#if 0 /* This code needs to be fixed and the C needs to be replaced to ensure that stack is not used */
    asm volatile
    (
        /* pa &= (-1 << 20);   // align to 1MB */
        "mov  r0, r0, lsr #20   \n"
        "mov  r0, r0, lsl #20   \n"

        /* pa |= (flags | 0x412);
         * bit breakdown:
         *  10:     superuser - r/w, user - no access
         *  4:      should be "1"
         *  3,2:    Cache flags (flags (r3))
         *  1:      Section signature 
         */

        "orr  r0, r0, r3        \n"
        "orr  r0, r0, #0x410    \n"
        "orr  r0, r0, #0x2      \n"
        :
        :
    );

    register int *ttb_base asm ("r3") = TTB_BASE;   /* force in r3 */

    asm volatile
    (
        /* unsigned int* ttbPtr = TTB_BASE + (va >> 20);
         * sections are 1MB size 
         */

        "mov  r1, r1, lsr #20           \n"
        "add  r1, %[ttbB], r1, lsl #0x2 \n" 

        /* Add MB to pa, flags are already present in pa, but addition 
         *  should not effect them
         *
         * #define MB (1 << 20)
         * for( ; mb>0; mb--, pa += MB)
         * {
         *     *(ttbPtr++) = pa;
         * }
         * #undef MB
         */

        "cmp  r2, #0                    \n"
        "bxle lr                        \n"
        "loop:                          \n"
        "str  r0, [r1], #4              \n"
        "add  r0, r0, #0x100000         \n"
        "sub  r2, r2, #0x01             \n"
        "bne  loop                      \n"
        "bx   lr                        \n"
        : 
        :   [ttbB] "r" (ttb_base)   /* This /HAS/ to be in r3 */
    );
    (void) pa;
    (void) va;
    (void) mb;
    (void) flags;
#else
    pa &= (-1 << 20);
    pa |= (flags | 0x412);
    unsigned int* ttbPtr = TTB_BASE + (va >> 20);

#define MB (1 << 20)
    for( ; mb>0; mb--, pa += MB)
    {
        *(ttbPtr++) = pa;
    }
#undef MB
#endif
}

void __attribute__((naked)) enable_mmu(void) {
    asm volatile(
        "mov r0, #0                 \n"
        "mcr p15, 0, r0, c8, c7, 0  \n" /* invalidate TLB */
        "mcr p15, 0, r0, c7, c7,0   \n" /* invalidate both icache and dcache */
        "mrc p15, 0, r0, c1, c0, 0  \n"
        "orr r0, r0, #1             \n" /* enable mmu bit, icache and dcache */
        "orr r0, r0, #1<<2          \n" /* enable dcache */
        "orr r0, r0, #1<<12         \n" /* enable icache */
        "mcr p15, 0, r0, c1, c0, 0  \n"
        "nop                        \n" 
        "nop                        \n"
        "nop                        \n"
        "nop                        \n"
        "bx  lr                     \n"
        : 
        : 
        : "r0"
    );
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
        : : "r" (addr), "r" (end)
    );
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
    unsigned int index, addr, low;

    for(index = 0; index <= 63; index++) 
    {
        for(low = 0;low <= 7; low++)
        {
            addr = (index << 26) | (low << 5);
            asm volatile
            (
                "mcr p15, 0, %[addr], c7, c10, 2 \n" /* Clean this entry by index */
                : 
                : [addr] "r" (addr)
            );
        }
    }
}
#endif

