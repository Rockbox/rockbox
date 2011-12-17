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
