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

/* TTB routines not used */

/** Cache coherency **/

/*
 * Invalidate DCache for this range
 * will do write back
 * void invalidate_dcache_range(const void *base, unsigned int size)
 */
    .section   .text, "ax", %progbits
    .align      2
    .global     invalidate_dcache_range
    .type       invalidate_dcache_range, %function
    @ MVA format: 31:5 = Modified virtual address, 4:0 = Ignored
invalidate_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    subhi   r1, r1, #1              @ round it down
    movhi   r2, #0                  @
    mcrrhi  p15, 0, r1, r0, c14     @ Clean and invalidate DCache range
    mcrhi   p15, 0, r2, c7, c10, 4  @ Data synchronization barrier
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
    @ MVA format: 31:5 = Modified virtual address, 4:0 = Ignored
clean_dcache_range:
    add     r1, r0, r1              @ size -> end
    cmp     r1, r0                  @ end <= start?
    subhi   r1, r1, #1              @ round it down
    movhi   r2, #0                  @
    mcrrhi  p15, 0, r1, r0, c12     @ Clean DCache range
    mcrhi   p15, 0, r2, c7, c10, 4  @ Data synchronization barrier
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
    @ MVA format (mcr):  31:5 = Modified virtual address, 4:0 = SBZ
    @ MVA format (mcrr): 31:5 = Modified virtual address, 4:0 = Ignored
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
    sub     r1, r1, #32             @ Move to the previous cache line
    cmp     r1, r0                  @ end < start now?
    mcrrhs  p15, 0, r1, r0, c6      @ Invalidate DCache range
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
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
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c10, 0  @ Clean entire DCache
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    bx      lr                      @
    .size   clean_dcache, .-clean_dcache

/*
 * Invalidate entire DCache
 * will do writeback
 * void invalidate_dcache(void);
 */
    .section   .text, "ax", %progbits
    .align      2
    .global     invalidate_dcache
    .type       invalidate_dcache, %function
invalidate_dcache:
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c14, 0  @ Clean and invalidate entire DCache
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
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
    mov     r0, #0                  @
    mcr     p15, 0, r0, c7, c14, 0  @ Clean and invalidate entire DCache
    mcr     p15, 0, r0, c7, c5, 0   @ Invalidate entire ICache
                                    @ Also flushes the branch target cache
    mcr     p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    mcr     p15, 0, r0, c7, c5, 4   @ Flush prefetch buffer (IMB)
    bx      lr                      @
    .size   invalidate_idcache, .-invalidate_idcache
