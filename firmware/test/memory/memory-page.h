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
#ifndef __LIBRARY_MEMORY_PAGE_H__
#define __LIBRARY_MEMORY_PAGE_H__

struct memory_free_page
  {
    struct memory_free_page
      *less,*more;
    char
      reserved[MEMORY_PAGE_MINIMAL_SIZE - 2*sizeof (struct memory_free_page *)];
  };

#define LESS -1
#define MORE +1

#ifdef TEST

struct memory_free_page __memory_free_page[MEMORY_TOTAL_PAGES];

#else

extern struct memory_free_page __memory_free_page[MEMORY_TOTAL_PAGES] asm("dram");

#endif

char __memory_free_page_order[MEMORY_TOTAL_PAGES];
struct memory_free_page *__memory_free_page_bin[MEMORY_TOTAL_ORDERS];

#ifdef TEST
#  if MEMORY_PAGE_USE_SPLAY_TREE

void __memory_dump_splay_node (struct memory_free_page *node,int level);
void __memory_dump_splay_tree (struct memory_free_page *root);

#  endif

void __memory_spy_page (void *address);
void __memory_dump (int order);
void __memory_check (int order);

#endif
#endif