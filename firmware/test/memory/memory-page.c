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
#include <memory.h>
#include "memory-page.h"
#if 0 
#include "memory-slab.h"
#endif

#ifdef TEST

// IA32 has no problem with shift operation
static inline unsigned int __memory_get_size (int order)
  {
    return (MEMORY_PAGE_MINIMAL_SIZE << order);
  }

// Arghhhh ! I cannot align 'free_page' on 512-byte boundary (max is 16-byte for Cygwin)
static inline struct memory_free_page *__memory_get_neighbour (struct memory_free_page *node,unsigned int size)
  {
    return ((struct memory_free_page *)((unsigned)__memory_free_page + (((unsigned)node - (unsigned)__memory_free_page) ^ size)));
  }

#else

// SH1 has very poor shift instructions (only <<1,>>1,<<2,>>2,<<8,>>8,<<16 and >>16).
// so we should use a lookup table to speedup. 
static inline unsigned int __memory_get_size (int order)
  {
    static unsigned short size [MEMORY_TOTAL_ORDERS] =
      {
           1<<MEMORY_PAGE_MINIMAL_ORDER,
           2<<MEMORY_PAGE_MINIMAL_ORDER,
           4<<MEMORY_PAGE_MINIMAL_ORDER,
           8<<MEMORY_PAGE_MINIMAL_ORDER,
          16<<MEMORY_PAGE_MINIMAL_ORDER,
          32<<MEMORY_PAGE_MINIMAL_ORDER,
          64<<MEMORY_PAGE_MINIMAL_ORDER,
         128<<MEMORY_PAGE_MINIMAL_ORDER,
         256<<MEMORY_PAGE_MINIMAL_ORDER,
         512<<MEMORY_PAGE_MINIMAL_ORDER,
        1024<<MEMORY_PAGE_MINIMAL_ORDER,
        2048<<MEMORY_PAGE_MINIMAL_ORDER,
        4096<<MEMORY_PAGE_MINIMAL_ORDER
      };
    return size[order];
  }

static inline struct memory_free_page *__memory_get_neighbour (struct memory_free_page *node,unsigned int size)
  {
    return ((struct memory_free_page *)((unsigned)node ^ size));
  }

#endif

static inline int __memory_get_order (struct memory_free_page *node)
  {
    return __memory_free_page_order[node -  __memory_free_page];
  }
static inline void __memory_set_order (struct memory_free_page *node,int order)
  {
    __memory_free_page_order[node -  __memory_free_page] = order;
  }

#if MEMORY_PAGE_USE_SPLAY_TREE

static struct memory_free_page *__memory_splay_page (struct memory_free_page *root,struct memory_free_page *node)
  {
    struct memory_free_page *down;
    struct memory_free_page *less;
    struct memory_free_page *more;
    struct memory_free_page  head;
    head.less =
    head.more = 0;
    less =
    more = &head;
    while (1)
      {
        if (node < root)
          {
            if ((down = root->less))
              {
                if (node < down)
                  {
                    root->less = down->more;
                    down->more = root;
                    root = down;
                    if (!root->less)
                      break;
                  }
                more->less = root;
                more       = root;
                root       = root->less;
                continue;
              }
            break;
          }
        if (root < node)
          {
            if ((down = root->more))
              {
                if (root < node)
                  {
                    root->more = down->less;
                    down->less = root;
                    root = down;
                    if (!root->more)
                      break;
                  }
                less->more = root;
                less       = root;
                root       = root->more;
                continue;
              }
          }
        break;
      }
    less->more = root->less;
    more->less = root->more;
    root->less = head.more;
    root->more = head.less;
    return root;
  }

static inline void __memory_insert_page (int order,struct memory_free_page *node)
  {
    struct memory_free_page *root = __memory_free_page_bin[order];
    if (!root)
      {
        node->less = 
        node->more = 0;
      }
    else if (node < (root = __memory_splay_page (root,node)))
      {
        node->less = root->less;
        node->more = root;
        root->less = 0;
      }
    else if (node > root)
      {
        node->less = root;
        node->more = root->more;
        node->more = 0;
      }
    __memory_free_page_bin[order] = node;
    __memory_set_order (node,order);
    return;
  }

static inline struct memory_free_page *__memory_pop_page (int order,int want)
  {
    struct memory_free_page *root = __memory_free_page_bin[order];
    if (root)
      {
        root = __memory_splay_page (root,__memory_free_page);
        __memory_free_page_bin[order] = root->more;
        __memory_set_order (root,~want);
      }
    return root;
  }

static inline void __memory_remove_page (int order,struct memory_free_page *node)
  {
    struct memory_free_page *root = __memory_free_page_bin[order];
    root = __memory_splay_page (root,node);
    if (root->less)
      {        
        node = __memory_splay_page (root->less,node);
        node->more = root->more;
      }
    else
      node = root->more;
    __memory_free_page_bin[order] = node;
  }
 
#else

static inline void __memory_insert_page (int order,struct memory_free_page *node)
  {
    struct memory_free_page *head = __memory_free_page_bin[order];
    node->less = 0;
    node->more = head;
    if (head)
      head->less = node;
    __memory_free_page_bin[order] = node;
    __memory_set_order (node,order);
  }

static inline struct memory_free_page *pop_page (int order,int want)
  {
    struct memory_free_page *node = __memory_free_page_bin[order];
    if (node)
      {
        __memory_free_page_bin[order] = node->more;
        if (node->more)
          node->more->less = 0;
        __memory_set_order (node,~want);
      }
    return node;
  }

static inline void __memory_remove_page (int order,struct memory_free_page *node)
  {
    if (node->less)
      node->less->more = node->more;
    else
      __memory_free_page_bin[order] = node->more;
    if (node->more)
      node->more->less = node->less;
  }

#endif

static inline void __memory_push_page (int order,struct memory_free_page *node)
  {
    node->less = 0;
    node->more = 0;
    __memory_free_page_bin[order] = node;
    __memory_set_order (node,order);
  }

static struct memory_free_page *__memory_allocate_page (unsigned int size,int order)
  {
    struct memory_free_page *node;
    int min = order;
    while ((unsigned)order <= (MEMORY_TOTAL_ORDERS - 1))
      // order is valid ?
      {
        if (!(node = __memory_pop_page (order,min)))
          // no free page of this order ?
          {
            ++order; size <<= 1;
            continue;
          }
        while (order > min)
          // split our larger page in smaller pages
          {
            --order; size >>= 1;
            __memory_push_page (order,(struct memory_free_page *)((unsigned int)node + size));
          }
        return node;
      }
    return MEMORY_RETURN_FAILURE;
  }

static inline void __memory_release_page (struct memory_free_page *node,unsigned int size,int order)
  {
    struct memory_free_page *neighbour;
    while ((order <= (MEMORY_TOTAL_ORDERS - 1)) &&
         ((neighbour = __memory_get_neighbour (node,size)),
         (__memory_get_order (neighbour) == order)))
      // merge our released page with its contiguous page into a larger page
      {
        __memory_remove_page (order,neighbour);
        ++order; size <<= 1;
        if (neighbour < node)
          node = neighbour;
      }
    __memory_insert_page (order,node);
  }


/*****************************************************************************/
/*                             PUBLIC FUNCTIONS                            */
/*****************************************************************************/

void *memory_allocate_page (int order)
  {
    if (order < 0)
      return MEMORY_RETURN_FAILURE;
    return __memory_allocate_page (__memory_get_size (order),order);
  }

// release a page :
//   when called, 'address' MUST be a valid address pointing
//   to &dram[i], where i ranges from 0 to MEMORY_TOTAL_PAGES - 1.
//   FAILURE if block is already freed.
int memory_release_page (void *address)
  {
    struct memory_free_page *node = (struct memory_free_page *)address;
    int order = ~__memory_get_order (node);
    if (order < 0)
      return MEMORY_RETURN_FAILURE;
    __memory_release_page (node,__memory_get_size (order),order);
    return MEMORY_RETURN_SUCCESS;
  }


#ifdef TEST
#  include <stdio.h>
#  include <stdlib.h>
#  if MEMORY_PAGE_USE_SPLAY_TREE

void __memory_dump_splay_node (struct memory_free_page *node,int level)
  {
    if (!node)
      return;
    __memory_dump_splay_node (node->less,level+1);
    printf ("\n%*s[%d-%d]",level,"",(node - __memory_free_page),(node - __memory_free_page) + (1 << __memory_get_order (node)) - 1);
    __memory_dump_splay_node (node->more,level+1);    
  }

void __memory_dump_splay_tree (struct memory_free_page *root)
  {
    __memory_dump_splay_node (root,2); fflush (stdout);
  }

#  endif

void __memory_spy_page (void *address)
  {
    struct memory_free_page *node = (struct memory_free_page *)address;
    int order,used;
    if (node)
      {
        order = __memory_get_order (node);
        used = order < 0;
        if (used)
          order = ~order;
        printf("\n(%s,%2d,%7d)",(used ? "used" : "free"),order,__memory_get_size (order));
      }
  }

void __memory_dump (int order)
  {
    struct memory_free_page *node = __memory_free_page_bin[order];
    printf("\n(%s,%2d,%7d)",node ? "free" : "none",order,__memory_get_size (order));
#  if MEMORY_PAGE_USE_SPLAY_TREE
    __memory_dump_splay_tree (node);
#  else
    while (node)
      {
        printf("[%d-%d]",(node - __memory_free_page),(node - __memory_free_page) + (1<<order) - 1);
        node = node->more;
      }
#  endif

  }

void __memory_check (int order)
  {
    struct memory_free_page *node[4096],*swap;
    unsigned int i = 0,j = 0;
    while (i <= 12)
      __memory_dump (i++);
    i = 0;
    printf ("\nallocating...\n");
    while (order >= 0)
      {
        j = order;
        while ((swap = memory_allocate_page (j)))
          {
            node[i++] = swap;
            printf("[%d-%d]",(swap - __memory_free_page),(swap - __memory_free_page) + ((1 << j)-1));
            for (j += (rand () & 15); j > (unsigned int)order; j -= order);
          }
        --order;
      }
    node[i] = 0;
    while (j <= 12)
      __memory_dump (j++);
    j = 0;
    printf ("\nreleasing...");
    --i;
    while (i > 0)
      {
        unsigned int k = 0;
#  if 0
        printf ("\n");
#  endif
        swap = node[k++];
#  if 0
        while (swap)
          {
            printf("[%d-%d]",(swap - __memory_free_page),(swap - __memory_free_page) + ((1 << ~__memory_get_order (swap))-1));
            swap = node[k++];
          }
#  endif
        for (j += 1 + (rand () & 15); j >= i; j -= i);
        swap = node[j];
        node[j] = node[i];
        memory_release_page (swap);
        node[i] = 0;
        --i;
      }
    memory_release_page (node[0]);
    i = 0;
    while (i <= 12)
      __memory_dump (i++);
    printf("\n\n%s !",(__memory_get_order (__memory_free_page) == 12) ? "SUCCESS" : "FAILURE");
  }

#endif
