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

/** Cache coherency **/

/*
 * Cleans entire DCache
 * void clean_dcache(void);
 */
    .section   .icode, "ax", %progbits
    .align      2
    .global     clean_dcache
    .type       clean_dcache, %function
    .global     cpucache_flush      @ Alias
clean_dcache:
cpucache_flush:
    @ Index format: 31:26 = index, 5:4 = segment, remainder = SBZ
    mov     r1, #0x00000000         @
1:  @ clean_start                   @
    mcr     p15, 0, r1, c7, c10, 2  @ Clean entry by index
    add     r0, r1, #0x00000010     @
    mcr     p15, 0, r0, c7, c10, 2  @ Clean entry by index
    add     r0, r0, #0x00000010     @
    mcr     p15, 0, r0, c7, c10, 2  @ Clean entry by index
    add     r0, r0, #0x00000010     @
    mcr     p15, 0, r0, c7, c10, 2  @ Clean entry by index
    adds    r1, r1, #0x04000000     @ will wrap to zero at loop end
    bne     1b @ clean_start        @
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
    @ Index format: 31:26 = index, 5:4 = segment, remainder = SBZ
    mov     r1, #0x00000000         @
1:  @ inv_start                     @
    mcr     p15, 0, r1, c7, c14, 2  @ Clean and invalidate entry by index
    add     r0, r1, #0x00000010     @
    mcr     p15, 0, r0, c7, c14, 2  @ Clean and invalidate entry by index
    add     r0, r0, #0x00000010     @
    mcr     p15, 0, r0, c7, c14, 2  @ Clean and invalidate entry by index
    add     r0, r0, #0x00000010     @
    mcr     p15, 0, r0, c7, c14, 2  @ Clean and invalidate entry by index
    adds    r1, r1, #0x04000000     @ will wrap to zero at loop end
    bne     1b @ inv_start          @
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
    mov     r2, lr                  @ save lr to r2, call uses r0 and r1 only
    bl      invalidate_dcache       @ Clean and invalidate entire DCache
    mcr     p15, 0, r1, c7, c5, 0   @ Invalidate ICache (r1=0 from call)
    mov     pc, r2                  @
    .size   invalidate_idcache, .-invalidate_idcache
