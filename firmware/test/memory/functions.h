/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef __LIBRARY_MEMORY_H__
#  error "This header file must be included ONLY from memory.h."
#endif
#  ifndef __LIBRARY_MEMORY_FUNCTIONS_H__
#  define __LIBRARY_MEMORY_FUNCTIONS_H__

/////////////////////////////////////////////////////////////////////
// MEMORY :
///////////

extern void memory_copy (void *target,void const *source,unsigned int count);
extern void memory_set (void *target,int byte,unsigned int count);

/////////////////////////////////////////////////////////////////////
// MEMORY PAGE :
////////////////
//
//  - memory_allocate_page : allocate a page
//  - memory_release_page : release a page
//

extern int memory_release_page (void *address);
extern void *memory_allocate_page (int order);
extern void memory_setup (void);

//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// MEMORY SLAB :
////////////////
//
//  - memory_grow_cache : allocate a new slab for a cache 
//  - memory_shrink_cache : release free slabs from a cache 
//  - memory_create_cache : create a new cache of size-fixed blocks 
//  - memory_destroy_cache : destroy the cache and release all the slabs  
//  - memory_cache_allocate : allocate a block from the cache
//  - memory_cache_release : release a block in the cache
//

extern struct memory_slab *memory_grow_cache (struct memory_cache *cache);
extern int memory_shrink_cache (struct memory_cache *cache,int all);
extern struct memory_cache *memory_create_cache (unsigned int size,int align,int flags);
extern int memory_destroy_cache (struct memory_cache *cache);
extern void *memory_cache_allocate (struct memory_cache *cache);
extern int memory_cache_release (struct memory_cache *cache,void *address);

//
/////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// MEMORY BLOCK :
/////////////////
//
//  - memory_allocate_small_block : allocate a small block (no page)
//  - memory_release_small_block : release a small block (no page)
//  - memory_allocate_block : allocate a block (or a page)
//  - memory_release_block : release a block (or a page)
//

extern void *memory_allocate_small_block (int order);
extern int memory_release_small_block (int order,void *address);
extern void *memory_allocate_block (unsigned int size);
extern int memory_release_block (void *address);

//
/////////////////////////////////////////////////////////////////////



#  ifdef TEST
void memory_spy_page (void *address);
void memory_dump (int order);
void memory_check (int order);
#  endif
#endif
