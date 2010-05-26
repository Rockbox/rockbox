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
#include "cpu.h"

/* Used by ARMv4 & ARMv5 CPUs with cp15 register and MMU */

#if CONFIG_CPU == TCC7801 || CONFIG_CPU == AT91SAM9260
/* MMU present but unused */
#define HAVE_TEST_AND_CLEAN_CACHE

#elif CONFIG_CPU == DM320 || CONFIG_CPU == AS3525v2
#define USE_MMU
#define HAVE_TEST_AND_CLEAN_CACHE

#elif CONFIG_CPU == AS3525
#define USE_MMU
#define CACHE_SIZE 8

#elif CONFIG_CPU == S3C2440
#define USE_MMU
#define CACHE_SIZE 16

#elif CONFIG_CPU == S5L8701
/* MMU not present */
#define CACHE_SIZE 4

#else
#error Cache settings unknown for this CPU !

#endif /* CPU specific configuration */

@ Index format: 31:26 = index, N:5 = segment, remainder = SBZ
@ assume 64-way set associative separate I/D caches
@ CACHE_SIZE = N (kB) = N*2^10 B
@ number of lines = N*2^(10-CACHEALIGN_BITS)
@ Index bits = 6
@ Segment loops = N*2^(10-CACHEALIGN_BITS-6) = N*2^(4-CACHEALIGN_BITS)
@ Segment loops = N/2^(CACHEALIGN_BITS - 4)
@ Segment loops = N/(1<<(CACHEALIGN_BITS - 4))

#ifdef CACHE_SIZE

#if CACHEALIGN_BITS == 4
#define INDEX_STEPS CACHE_SIZE
#elif CACHEALIGN_BITS == 5
#define INDEX_STEPS (CACHE_SIZE/2)
#endif /* CACHEALIGN_BITS */

@ assume 64-way set associative separate I/D caches (log2(64) == 6)
@ Index format: 31:26 = index, M:N = segment, remainder = SBZ
@ Segment bits = log2(cache size in bytes / cache line size in byte) - Index bits (== 6)
@ N = CACHEALIGN_BITS

#endif /* CACHE_SIZE */


#ifdef USE_MMU

/** MMU setup **/

/*
 * void ttb_init(void);
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     ttb_init
    .type       ttb_init, %function
ttb_init:
    ldr     r0, =TTB_BASE_ADDR      @
    mvn     r1, #0                  @
    mcr     p15, 0, r0, c2, c0, 0   @ Set the TTB base address
    mcr     p15, 0, r1, c3, c0, 0   @ Set all domains to manager status
    bx      lr                      @
    .size   ttb_init, .-ttb_init

/*
 * void map_section(unsigned int pa, unsigned int va, int mb, int flags);
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     map_section
    .type       map_section, %function
map_section:
    @ align to 1MB
    @ pa &= (-1 << 20);
    mov     r0, r0, lsr #20
    mov     r0, r0, lsl #20

    @ pa |= (flags | 0x412);
    @ bit breakdown:
    @  10:  superuser - r/w, user - no access
    @  4:   should be "1"
    @  3,2: Cache flags (flags (r3))
    @  1:   Section signature
    orr     r0, r0, r3
    orr     r0, r0, #0x410
    orr     r0, r0, #0x2

    @ unsigned int* ttbPtr = TTB_BASE + (va >> 20);
    @ sections are 1MB size
    mov     r1, r1, lsr #20
    ldr     r3, =TTB_BASE_ADDR
    add     r1, r3, r1, lsl #0x2

    @ Add MB to pa, flags are already present in pa, but addition
    @ should not effect them
    @
    @ for( ; mb>0; mb--, pa += (1 << 20))
    @ {
    @     *(ttbPtr++) = pa;
    @ }
    cmp    r2, #0
    bxle   lr
    mov    r3, #0x0
1:  @ loop
    str    r0, [r1], #4
    add    r0, r0, #0x100000
    add    r3, r3, #0x1
    cmp    r2, r3
    bne    1b @ loop
    bx     lr
    .size   map_section, .-map_section

/*
 * void enable_mmu(void);
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     enable_mmu
    .type       enable_mmu, %function
enable_mmu:
    mov     r0, #0                  @
    mcr     p15, 0, r0, c8, c7, 0   @ invalidate TLB
    mcr     p15, 0, r0, c7, c7,0    @ invalidate both i and dcache
    mrc     p15, 0, r0, c1, c0, 0   @
    orr     r0, r0, #1              @ enable mmu bit, i and dcache
    orr     r0, r0, #1<<2           @ enable dcache
    orr     r0, r0, #1<<12          @ enable icache
    mcr     p15, 0, r0, c1, c0, 0   @
    nop                             @
    nop                             @
    nop                             @
    nop                             @
    bx      lr                      @
    .size   enable_mmu, .-enable_mmu
    .ltorg

#endif  /* USE_MMU */



/** Cache coherency **/

/*
 * Invalidate DCache for this range
 * will do write back
 * void invalidate_dcache_range(const void *base, unsigned int size);
 */
    .section   .text, "ax", %progbits
    .align      2
    .global     invalidate_dcache_range
    .type       invalidate_dcache_range, %function
    @ MVA format: 31:5 = Modified virtual address, 4:0 = SBZ
invalidate_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    bxls    lr                      @
    bic     r0, r0, #31             @ Align start to cache line (down)
1:  @ inv_start                     @
    mcr     p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    add     r0, r0, #32             @
    cmp     r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    bhi     1b @ inv_start          @
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 4  @ Drain write buffer
    bx      lr                      @
    .size   invalidate_dcache_range, .-invalidate_dcache_range

/*
 * clean DCache for this range
 * forces DCache writeback for the specified range
 * void clean_dcache_range(const void *base, unsigned int size);
 */
    .section   .text, "ax", %progbits
    .align      2
    .global     clean_dcache_range
    .type       clean_dcache_range, %function
    @ MVA format: 31:5 = Modified virtual address, 4:0 = SBZ
clean_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    bxls    lr                      @
    bic     r0, r0, #31             @ Align start to cache line (down)
1:  @ clean_start                   @
    mcr     p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    add     r0, r0, #32             @
    cmp     r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c10, 1  @ Clean line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    bhi     1b @clean_start         @
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 4  @ Drain write buffer
    bx      lr                      @
    .size   clean_dcache_range, .-clean_dcache_range

/*
 * Dump DCache for this range
 * will *NOT* do write back except for buffer edges not on a line boundary
 * void dump_dcache_range(const void *base, unsigned int size);
 */
    .section   .text, "ax", %progbits
    .align      2
    .global     dump_dcache_range
    .type       dump_dcache_range, %function
    @ MVA format: 31:5 = Modified virtual address, 4:0 = SBZ
 dump_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    bxls    lr                      @
    tst     r0, #31                 @ Check first line for bits set
    bicne   r0, r0, #31             @ Clear low five bits (down)
    mcrne   p15, 0, r0, c7, c14, 1  @ Clean and invalidate line by MVA
                                    @ if not cache aligned
    addne   r0, r0, #32             @ Move to the next cache line
                                    @
    tst     r1, #31                 @ Check last line for bits set
    bicne   r1, r1, #31             @ Clear low five bits (down)
    mcrne   p15, 0, r1, c7, c14, 1  @ Clean and invalidate line by MVA
                                    @ if not cache aligned
    cmp     r1, r0                  @ end <= start now?
1:  @ dump_start                    @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    mcrhi   p15, 0, r0, c7, c6, 1   @ Invalidate line by MVA
    addhi   r0, r0, #32             @
    cmphi   r1, r0                  @
    bhi     1b @ dump_start         @
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 4  @ Drain write buffer
    bx      lr                      @
    .size   dump_dcache_range, .-dump_dcache_range

/*
 * Cleans entire DCache
 * void clean_dcache(void);
 */
    .section   .text, "ax", %progbits
    .align      2
    .global     clean_dcache
    .type       clean_dcache, %function
    .global     cpucache_flush      @ Alias
clean_dcache:
cpucache_flush:
#ifdef HAVE_TEST_AND_CLEAN_CACHE
    mrc     p15, 0, r15, c7, c10, 3 @ test and clean dcache
    bne     clean_dcache
    mov     r1, #0
#else
    mov     r1, #0x00000000         @
1:  @ clean_start                   @
    mcr     p15, 0, r1, c7, c10, 2  @ Clean entry by index
    add     r0, r1, #(1<<CACHEALIGN_BITS)
    mcr     p15, 0, r0, c7, c10, 2  @ Clean entry by index
.rept INDEX_STEPS - 2 /* 2 steps already executed */
    add     r0, r0, #(1<<CACHEALIGN_BITS)
    mcr     p15, 0, r0, c7, c10, 2  @ Clean entry by index
.endr
    adds    r1, r1, #0x04000000     @ will wrap to zero at loop end
    bne     1b @ clean_start        @
#endif /* HAVE_TEST_AND_CLEAN_CACHE */
    mcr     p15, 0, r1, c7, c10, 4  @ Drain write buffer
    bx      lr                      @
    .size   clean_dcache, .-clean_dcache

/*
 * Invalidate entire DCache
 * will do writeback
 * void invalidate_dcache(void);
 */
    .section   .icode, "ax", %progbits
    .align      2
    .global     invalidate_dcache
    .type       invalidate_dcache, %function
invalidate_dcache:
#ifdef HAVE_TEST_AND_CLEAN_CACHE
    mrc     p15, 0, r15, c7, c14, 3 @ test, clean and invalidate dcache
    bne     invalidate_dcache
    mov     r1, #0
#else
    mov     r1, #0x00000000         @
1:  @ inv_start                     @
    mcr     p15, 0, r1, c7, c14, 2  @ Clean and invalidate entry by index
    add     r0, r1, #(1<<CACHEALIGN_BITS)
    mcr     p15, 0, r0, c7, c14, 2  @ Clean and invalidate entry by index
.rept INDEX_STEPS - 2 /* 2 steps already executed */
    add     r0, r0, #(1<<CACHEALIGN_BITS)
    mcr     p15, 0, r0, c7, c14, 2  @ Clean and invalidate entry by index
.endr
    adds    r1, r1, #0x04000000     @ will wrap to zero at loop end
    bne     1b @ inv_start          @
#endif /* HAVE_TEST_AND_CLEAN_CACHE */
    mcr     p15, 0, r1, c7, c10, 4  @ Drain write buffer
    bx      lr                      @
    .size   invalidate_dcache, .-invalidate_dcache

/*
 * Invalidate entire ICache and DCache
 * will do writeback
 * void invalidate_idcache(void);
 */
    .section   .icode, "ax", %progbits
    .align      2
    .global     invalidate_idcache
    .type       invalidate_idcache, %function
    .global     cpucache_invalidate @ Alias
invalidate_idcache:
cpucache_invalidate:
    mov     r2, lr                  @ save lr to r1, call uses r0 only
    bl      invalidate_dcache       @ Clean and invalidate entire DCache
    mcr     p15, 0, r1, c7, c5, 0   @ Invalidate ICache (r1=0 from call)
    mov     pc, r2                  @
    .size   invalidate_idcache, .-invalidate_idcache
