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
#ifndef __LIBRARY_MEMORY_H__
# error "This header file must be included ONLY from memory.h."
#endif
#ifndef __LIBRARY_MEMORY_TYPES_H__
# define __LIBRARY_MEMORY_TYPES_H__

struct memory_free_page
  {
    struct memory_free_page
      *less,*more;
    char
      reserved[MEMORY_PAGE_MINIMAL_SIZE - 2*sizeof (struct memory_free_page *)];
  };
struct memory_free_block
  {
    struct memory_free_block
      *link;
  };

struct memory_cache
  {
    struct memory_cache
      *less,*more,*same; 
    unsigned int
      left; // number of free slabs
    struct memory_slab
      *used;
    struct memory_slab
      *free;
    struct memory_slab
      *reap;
    unsigned int
      size,original_size;
    unsigned int
      page_size;
    unsigned int
      blocks_per_slab; 
    int
      page_order;
    unsigned int
      flags;
  };

struct memory_slab
  {
    struct memory_slab
      *less,*more; 
    unsigned int // left == number of free blocks left
      left;
    struct memory_free_block
      *free; 
  };

#endif