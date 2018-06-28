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

void map_address(unsigned long virtual, unsigned long physical,
                 unsigned long length, unsigned int cache_flags);
void mmu_init(void);

#define HAVE_CPUCACHE_INVALIDATE
//#define HAVE_CPUCACHE_FLUSH

void __idcache_invalidate_all(void);
void __icache_invalidate_all(void);
void __dcache_invalidate_all(void);
void __dcache_writeback_all(void);

void dma_cache_wback_inv(unsigned long addr, unsigned long size);

#define commit_discard_idcache   __idcache_invalidate_all
#define commit_discard_icache    __icache_invalidate_all
#define commit_discard_dcache    __dcache_invalidate_all
#define commit_dcache            __dcache_writeback_all

#endif /* __MMU_MIPS_INCLUDE_H */
