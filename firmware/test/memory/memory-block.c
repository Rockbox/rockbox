/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __LIBRARY_MEMORY_C__
#  error "This header file must be included ONLY from memory.c."
#endif
#ifndef __LIBRARY_MEMORY_BLOCK_H__
#  define __LIBRARY_MEMORY_BLOCK_H__

static struct memory_cache *free_block_cache[MEMORY_PAGE_MINIMAL_ORDER - 2];

///////////////////////////////////////////////////////////////////////////////
// MEMORY BLOCK :
/////////////////
//
//  - memory_allocate_block : allocate a power-of-2-sized block (or a page)
//  - memory_release_block : release a power-of-2-sized block (or a page)
//

static inline void *allocate_small_block (int order)
  {
    struct memory_cache *cache = free_block_cache[order - 2];
    do
      {
        if (cache)
          return memory_cache_allocate (cache);
      }
    while ((free_block_cache[order] = cache = memory_create_cache (size,0,0)));
    return MEMORY_RETURN_FAILURE;
  }

void *memory_allocate_block (int order)
  {
    if (order < 2)
      order = 2;
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
      return allocate_small_block (order);
    if (order < MEMORY_PAGE_MAXIMAL_ORDER)
      return memory_allocate_page (order);
    return MEMORY_RETURN_FAILURE;
  }

static inline int release_block (int order,void *address)
  {
    struct memory_cache *cache = free_block_cache[order - 2];
    if (cache)
      return memory_cache_release (cache,address);
    return MEMORY_RETURN_FAILURE;
  }

int memory_release_block (int order,void *address)
  {
    if (order < 2)
      order = 2;
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
      return release_block (order);
    if (order < MEMORY_PAGE_MAXIMAL_ORDER)
      return memory_release_page (address);
    return MEMORY_RETURN_FAILURE;
  }

#endif
