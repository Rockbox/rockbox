/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "cpucache-armv7m.h"
#include "cortex-m/cache.h"

/*
 * The target must define DCACHE_WAYS and DCACHE_SETS to the number
 * of sets/ways in the L1 data cache. DCACHE_LINESIZE should be set
 * to the writeback granule size.
 *
 * If you have multiple levels of caches (L2 or higher) then you'll
 * probably need to change things here to handle it properly.
 */

#define DCACHE_WAYSHIFT \
    (DCACHE_WAYS <= 1 ? 0 : __builtin_clzl(DCACHE_WAYS - 1))

static inline void full_dcache_op(volatile uint32_t *cache_reg)
{
    arm_dsb();

    for (uint32_t way = 0; way < DCACHE_WAYS; ++way)
    {
        uint32_t arg = way << DCACHE_WAYSHIFT;

        for (uint32_t set = 0; set < DCACHE_SETS; ++set)
        {
            *cache_reg = arg;
            arg += DCACHE_LINESIZE;
        }
    }

    arm_dsb();
}

static inline void range_dcache_op(const void *base, unsigned int size,
                                   volatile uint32_t *cache_reg)
{
    arm_dsb();

    uint32_t addr = (uint32_t)base;
    uint32_t endaddr = addr + size;

    while (addr < endaddr)
    {
        *cache_reg = addr;
        addr += DCACHE_LINESIZE;
    }

    arm_dsb();
}

void commit_dcache(void)
{
    full_dcache_op(&REG_CACHE_DCCSW);
}

void commit_discard_dcache(void)
{
    full_dcache_op(&REG_CACHE_DCCISW);
}

void commit_discard_idcache(void)
{
    full_dcache_op(&REG_CACHE_DCCISW);

    REG_CACHE_ICIALLU = 0;

    arm_isb();
}

void __discard_idcache(void)
{
    full_dcache_op(&REG_CACHE_DCISW);

    REG_CACHE_ICIALLU = 0;

    arm_isb();
}

void commit_discard_dcache_range(const void *base, unsigned int size)
{
    range_dcache_op(base, size, &REG_CACHE_DCCIMVAC);
}

void commit_dcache_range(const void *base, unsigned int size)
{
    range_dcache_op(base, size, &REG_CACHE_DCCMVAC);
}

void discard_dcache_range(const void *base, unsigned int size)
{
    range_dcache_op(base, size, &REG_CACHE_DCIMVAC);
}
