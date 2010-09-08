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

/*  This file MUST be included in your system-target.h file if you want arm
 *  cache coherence functions to be called (I.E. during codec load, etc).   
 */

#ifndef MMU_ARM_H
#define MMU_ARM_H

#define CACHE_ALL   0x0C
#define CACHE_NONE  0
#define BUFFERED    0x04

void memory_init(void);
void ttb_init(void);
void enable_mmu(void);
void map_section(unsigned int pa, unsigned int va, int mb, int flags);

/* Note for the function names
 *
 * ARM refers to the cache coherency functions as (in the CPU manuals):
 *  clean (write-back)
 *  clean and invalidate (write-back and removing the line from cache)
 *  invalidate (removing from cache without write-back)
 *
 * The deprecated functions below don't follow the above (which is why
 * they're deprecated).
 *
 * This names have been proven to cause confusion, therefore we use:
 *  commit
 *  commit and discard
 *  discard
 */

/* Commits entire DCache */
void commit_dcache(void);
/* deprecated alias */
void clean_dcache(void);

/* Commit and discard entire DCache, will do writeback */
void commit_discard_dcache(void);
/* deprecated alias */
void invalidate_dcache(void);

/* Write DCache back to RAM for the given range and remove cache lines
 * from DCache afterwards */
void commit_discard_dcache_range(const void *base, unsigned int size);
/* deprecated alias */
void invalidate_dcache_range(const void *base, unsigned int size);

/* Write DCache back to RAM for the given range */
void commit_dcache_range(const void *base, unsigned int size);
/* deprecated alias */
void clean_dcache_range(const void *base, unsigned int size);

/*
 * Remove cache lines for the given range from DCache
 * will *NOT* do write back except for buffer edges not on a line boundary
 */
void discard_dcache_range(const void *base, unsigned int size);
/* deprecated alias */
void dump_dcache_range(const void *base, unsigned int size);

/* Discards the entire ICache, and commit+discards the entire DCache */
void commit_discard_idcache(void);
/* deprecated alias */
void invalidate_idcache(void);

#define HAVE_CPUCACHE_COMMIT_DISCARD
#define HAVE_CPUCACHE_COMMIT
/* deprecated alias */
#define HAVE_CPUCACHE_INVALIDATE
#define HAVE_CPUCACHE_FLUSH

#endif /* MMU_ARM_H */
