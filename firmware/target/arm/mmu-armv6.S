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

#if CONFIG_CPU == S5L8720
#define USE_MMU
#endif


#ifdef USE_MMU

/** MMU setup **/

/*
 * void ttb_init(void);
 */
    .section    .text.ttb_init, "ax", %progbits
    .align      2
    .global     ttb_init
    .type       ttb_init, %function
ttb_init:
    ldr     r0, =TTB_BASE_ADDR      @
    mvn     r1, #0                  @
    // TODO: only set the first 3 bits to 0
    // mcr     p15, 0, r1, c2, c0, 2   @ Set TTB backwards compatiblity
    mcr     p15, 0, r0, c2, c0, 0   @ Set the TTB base address
    mcr     p15, 0, r1, c3, c0, 0   @ Set all domains to manager status
    bx      lr                      @
    .size   ttb_init, .-ttb_init

/*
 * void map_section(unsigned int pa, unsigned int va, int mb, int flags);
 */
    .section    .text.map_section, "ax", %progbits
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
    .section    .text.enable_mmu, "ax", %progbits
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

#if 0 /* unused */

/*
 * Write DCache back to RAM for the given range and remove cache lines
 * from DCache afterwards
 * void commit_discard_dcache_range(const void *base, unsigned int size);
 */
    .section   .text.commit_discard_dcache_range, "ax", %progbits
    .align      2
    .global     commit_discard_dcache_range
    .type       commit_discard_dcache_range, %function

    @ MVA format: 31:5 = Modified virtual address, 4:0 = SBZ
commit_discard_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    subhi   r1, r1, #1              @ round it down
    movhi   r2, #0                  @
    mcrrhi  p15, 0, r1, r0, c14     @ Clean and invalidate DCache range
    mcrhi   p15, 0, r2, c7, c10, 4  @ Data synchronization barrier
    bx      lr                      @
    .size   commit_discard_dcache_range, .-commit_discard_dcache_range

#endif /* unused function */

/*
 * Write DCache back to RAM for the given range
 * void commit_dcache_range(const void *base, unsigned int size);
 */
    .section   .text.commit_dcache_range, "ax", %progbits
    .align      2
    .global     commit_dcache_range
    .type       commit_dcache_range, %function

    @ MVA format: 31:5 = Modified virtual address, 4:0 = SBZ
commit_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    subhi   r1, r1, #1              @ round it down
    movhi   r2, #0                  @
    mcrrhi  p15, 0, r1, r0, c12     @ Clean DCache range
    mcrhi   p15, 0, r2, c7, c10, 4  @ Data synchronization barrier
    bx      lr                      @
    .size   commit_dcache_range, .-commit_dcache_range

/*
 * Remove cache lines for the given range from DCache
 * will *NOT* do write back except for buffer edges not on a line boundary
 * void discard_dcache_range(const void *base, unsigned int size);
 */
    .section   .text.discard_dcache_range, "ax", %progbits
    .align      2
    .global     discard_dcache_range
    .type       discard_dcache_range, %function

    @ MVA format: 31:5 = Modified virtual address, 4:0 = SBZ
discard_dcache_range:
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
    sub     r1, r1, #32             @ Move to the previous cache line
    cmp     r1, r0                  @ end < start now?
    mcrrhs  p15, 0, r1, r0, c6      @ Invalidate DCache range
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    bx      lr                      @
    .size   discard_dcache_range, .-discard_dcache_range


/*
 * Write entire DCache back to RAM
 * void commit_dcache(void);
 */
    .section   .text.commit_dcache, "ax", %progbits
    .align      2
    .global     commit_dcache
    .type       commit_dcache, %function

commit_dcache:
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 0  @ Clean entire DCache
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    bx      lr                      @
    .size   commit_dcache, .-commit_dcache

/*
 * Clean and invalidate entire DCache, will do writeback
 * void commit_discard_dcache(void);
 */
    .section   .icode.commit_discard_dcache, "ax", %progbits
    .align      2
    .global     commit_discard_dcache
    .type       commit_discard_dcache, %function

commit_discard_dcache:
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c14, 0  @ Clean and invalidate entire DCache
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    bx      lr                      @
    .size   commit_discard_dcache, .-commit_discard_dcache


/*
 * Discards the entire ICache, and commit+discards the entire DCache
 * void commit_discard_idcache(void);
 */
    .section   .icode.commit_discard_idcache, "ax", %progbits
    .align      2
    .global     commit_discard_idcache
    .type       commit_discard_idcache, %function

commit_discard_idcache:
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c14, 0  @ Clean and invalidate entire DCache
    mcr     p15, 0, r0, c7, c5, 0   @ Invalidate entire ICache
                                    @ Also flushes the branch target cache
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    mcr     p15, 0, r0, c7, c5, 4   @ Flush prefetch buffer (IMB)
    bx      lr                      @
    .size   commit_discard_idcache, .-commit_discard_idcache
