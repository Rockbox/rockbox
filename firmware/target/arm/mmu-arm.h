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

#define CACHE_ALL (1 << 3 | 1 << 2 )
#define CACHE_NONE 0
#define BUFFERED (1 << 2)

void ttb_init(void);
void enable_mmu(void);
void map_section(unsigned int pa, unsigned int va, int mb, int cache_flags);

/* Invalidate DCache for this range  */
/* Will do write back */
void invalidate_dcache_range(const void *base, unsigned int size);

/* clean DCache for this range  */
/* forces DCache writeback for the specified range */
void clean_dcache_range(const void *base, unsigned int size);

/* Dump DCache for this range  */
/* Will *NOT* do write back */
void dump_dcache_range(const void *base, unsigned int size);

/* Cleans entire DCache */
void clean_dcache(void);

void memory_init(void);
