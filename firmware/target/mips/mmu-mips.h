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

#ifndef __MMU_MIPS_INCLUDE_H
#define __MMU_MIPS_INCLUDE_H

#include "system-target.h"

/* By default the cache management functions go in .icode so they can be
 * called safely eg. by the bootloader or RoLo, which need to flush the
 * cache before jumping to the loaded binary.
 */
#ifndef MIPS_CACHEFUNC_ATTR
# define MIPS_CACHEFUNC_ATTR  __attribute__((section(".icode")))
#endif

void map_address(unsigned long virtual, unsigned long physical,
                 unsigned long length, unsigned int cache_flags);
void mmu_init(void);

/* Commits entire DCache */
void commit_dcache(void) MIPS_CACHEFUNC_ATTR;
/* Commit and discard entire DCache, will do writeback */
void commit_discard_dcache(void) MIPS_CACHEFUNC_ATTR;

/* Write DCache back to RAM for the given range and remove cache lines
 * from DCache afterwards */
void commit_discard_dcache_range(const void *base, unsigned int size) MIPS_CACHEFUNC_ATTR;

/* Write DCache back to RAM for the given range */
void commit_dcache_range(const void *base, unsigned int size) MIPS_CACHEFUNC_ATTR;

/*
 * Remove cache lines for the given range from DCache
 * will *NOT* do write back except for buffer edges not on a line boundary
 */
void discard_dcache_range(const void *base, unsigned int size) MIPS_CACHEFUNC_ATTR;

/* Discards the entire ICache, and commit+discards the entire DCache */
void commit_discard_idcache(void) MIPS_CACHEFUNC_ATTR;

#endif /* __MMU_MIPS_INCLUDE_H */
