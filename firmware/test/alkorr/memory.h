/*
//////////////////////////////////////////////////////////////////////////////////
//                __________               __   ___.                            //
// Open           \______   \ ____   ____ |  | _\_ |__   _______  ___           //
//  Source         |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /           //
//   Jukebox       |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <            //
//    Software     |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \           //              
//                        \/            \/     \/    \/            \/           //
//////////////////////////////////////////////////////////////////////////////////
//
// $Id$
//
/////////////////////////////////////
// Copyright (C) 2002 by Alan Korr //
/////////////////////////////////////
//
// All files in this archive are subject to the GNU General Public License.
// See the file COPYING in the source tree root for full license agreement.
//
// This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
// either express or implied.
//                                                                          
//////////////////////////////////////////////////////////////////////////////////
*/

#ifndef __MEMORY_H__
#define __MEMORY_H__

enum
{
    MEMORY_RETURN_SUCCESS = 1,
    MEMORY_RETURN_FAILURE = 0
};

/*
/////////////////////////////////////////////////////////////////////
// MEMORY PAGE //
/////////////////
//
//
*/

#define MEMORY_PAGE_MINIMAL_ORDER ( 9) /* 512 bytes by default */
#define MEMORY_PAGE_MAXIMAL_ORDER (21) /* 2 Mbytes by default */
#define MEMORY_PAGE_MINIMAL_SIZE  (1 << MEMORY_PAGE_MINIMAL_ORDER)
#define MEMORY_PAGE_MAXIMAL_SIZE  (1 << MEMORY_PAGE_MAXIMAL_ORDER)

#define MEMORY_TOTAL_PAGES (MEMORY_PAGE_MAXIMAL_SIZE / MEMORY_PAGE_MINIMAL_SIZE)
#define MEMORY_TOTAL_ORDERS (1 + MEMORY_PAGE_MAXIMAL_ORDER - MEMORY_PAGE_MINIMAL_ORDER)

extern void *memory_page_allocate (int order);
extern int memory_page_release (void *address);
extern void memory_page_release_range (unsigned int start,unsigned int end);

/*
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// MEMORY CACHE //
//////////////////
//
//
*/

struct memory_cache;

extern struct memory_cache *memory_cache_create (unsigned int size,int align);
extern int memory_cache_destroy (struct memory_cache *cache);
extern void *memory_cache_allocate (struct memory_cache *cache);
extern int memory_cache_release (struct memory_cache *cache,void *address);

/*
//
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// MEMORY BLOCK //
//////////////////
//
//
*/

extern void *memory_block_allocate (int order);
extern int memory_block_release (int order,void *address);

/*
//
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// MEMORY //
////////////
//
//
*/

#define MEMORY_TOTAL_BYTES (MEMORY_PAGE_MAXIMAL_SIZE)

extern void memory_copy (void *target,void const *source,unsigned int count);
extern void memory_set (void *target,int byte,unsigned int count);

extern void memory_setup (void);

/*
//
//
/////////////////////////////////////////////////////////////////////
*/

#endif
