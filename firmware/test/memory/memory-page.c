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
#include <memory.h>

#define LESS -1
#define MORE +1

#ifdef TEST

struct memory_free_page free_page[MEMORY_TOTAL_PAGES];

static inline unsigned int get_offset (int order)
  {
    return (2 << order);
  }

// IA32 has no problem with shift operation
static inline unsigned int get_size (int order)
  {
    return (MEMORY_PAGE_MINIMAL_SIZE << order);
  }

// Arghhhh ! I cannot align 'free_page' on 512-byte boundary (max is 16-byte for Cygwin)
static inline struct memory_free_page *get_neighbour (struct memory_free_page *node,unsigned int size)
  {
    return ((struct memory_free_page *)((unsigned)free_page + (((unsigned)node - (unsigned)free_page) ^ size)));
  }

#else

extern struct memory_free_page free_page[MEMORY_TOTAL_PAGES] asm("dram");

static inline unsigned int get_offset (int order)
  {
    static unsigned short offset [MEMORY_TOTAL_ORDERS] =
      { 2,4,8,16,32,64,128,256,512,1024,2048,4096,8192 };
    return offset[order];
  }

// SH1 has very poor shift instructions (only <<1,>>1,<<2,>>2,<<8,>>8,<<16 and >>16).
// so we should use a lookup table to speedup. 
static inline unsigned int get_size (int order)
  {
    return (get_offset (order))<<8;
  }

static inline struct memory_free_page *get_neighbour (struct memory_free_page *node,unsigned int size)
  {
    return ((struct memory_free_page *)((unsigned)node ^ size));
  }

#endif

static char free_page_order[MEMORY_TOTAL_PAGES];
static struct memory_free_page *free_page_list[MEMORY_TOTAL_ORDERS];

static inline int get_order (struct memory_free_page *node)
  {
    return free_page_order[node -  free_page];
  }
static inline void set_order (struct memory_free_page *node,int order)
  {
    free_page_order[node -  free_page] = order;
  }

#if MEMORY_PAGE_USE_SPLAY_TREE

#  include <stdio.h>

static struct memory_free_page *splay_page (struct memory_free_page *root,struct memory_free_page *node)
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

static inline void insert_page (int order,struct memory_free_page *node)
  {
    struct memory_free_page *root = free_page_list[order];
    if (!root)
      {
        node->less = 
        node->more = 0;
      }
    else if (node < (root = splay_page (root,node)))
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
    free_page_list[order] = node;
    set_order (node,order);
    return;
  }

static inline struct memory_free_page *pop_page (int order,int want)
  {
    struct memory_free_page *root = free_page_list[order];
    if (root)
      {
        root = splay_page (root,free_page);
        free_page_list[order] = root->more;
        set_order (root,~want);
      }
    return root;
  }

static inline void remove_page (int order,struct memory_free_page *node)
  {
    struct memory_free_page *root = free_page_list[order];
    root = splay_page (root,node);
    if (root->less)
      {          
        node = splay_page (root->less,node);
        node->more = root->more;
      }
    else
      node = root->more;
    free_page_list[order] = node;
  }
 
#else

static inline void insert_page (int order,struct memory_free_page *node)
  {
    struct memory_free_page *head = free_page_list[order];
    node->less = 0;
    node->more = head;
    if (head)
      head->less = node;
    free_page_list[order] = node;
    set_order (node,order);
  }

static inline struct memory_free_page *pop_page (int order,int want)
  {
    struct memory_free_page *node = free_page_list[order];
    if (node)
      {
        free_page_list[order] = node->more;
        if (node->more)
          node->more->less = 0;
        set_order (node,~want);
      }
    return node;
  }

static inline void remove_page (int order,struct memory_free_page *node)
  {
    if (node->less)
      node->less->more = node->more;
    else
      free_page_list[order] = node->more;
    if (node->more)
      node->more->less = node->less;
  }

#endif

static inline void push_page (int order,struct memory_free_page *node)
  {
    node->less = 0;
    node->more = 0;
    free_page_list[order] = node;
    set_order (node,order);
  }

static struct memory_free_page *allocate_page (unsigned int size,int order)
  {
    struct memory_free_page *node;
    int min = order;
    while ((unsigned)order <= (MEMORY_TOTAL_ORDERS - 1))
      // order is valid ?
      {
        if (!(node = pop_page (order,min)))
          // no free page of this order ?
          {
            ++order; size <<= 1;
            continue;
          }
        while (order > min)
          // split our larger page in smaller pages
          {
            --order; size >>= 1;
            push_page (order,(struct memory_free_page *)((unsigned int)node + size));
          }
        return node;
      }
    return MEMORY_RETURN_FAILURE;
  }

static inline void release_page (struct memory_free_page *node,unsigned int size,int order)
  {
    struct memory_free_page *neighbour;
    while ((order <= (MEMORY_TOTAL_ORDERS - 1)) &&
           ((neighbour = get_neighbour (node,size)),
            (get_order (neighbour) == order)))
      // merge our released page with its contiguous page into a larger page
      {
        remove_page (order,neighbour);
        ++order; size <<= 1;
        if (neighbour < node)
          node = neighbour;
      }
    insert_page (order,node);
  }


/*****************************************************************************/
/*                             PUBLIC FUNCTIONS                              */
/*****************************************************************************/

void *memory_allocate_page (int order)
  {
    if (order < 0)
      return MEMORY_RETURN_FAILURE;
    return allocate_page (get_size (order),order);
  }

// release a page :
//   when called, 'address' MUST be a valid address pointing
//   to &dram[i], where i ranges from 0 to MEMORY_TOTAL_PAGES - 1.
//   FAILURE if block is already freed.
int memory_release_page (void *address)
  {
    struct memory_free_page *node = (struct memory_free_page *)address;
    int order = ~get_order (node);
    if (order < 0)
      return MEMORY_RETURN_FAILURE;
    release_page (node,get_size (order),order);
    return MEMORY_RETURN_SUCCESS;
  }

/* NOT VERY OPTIMIZED AT ALL BUT WE WILL DO IT WHEN PRIORITY COMES */
void memory_copy (void *target,void const *source,unsigned int count)
  {
    while (count--)
      *((char *)target)++ = *((char const *)source)++;
  }
  
/* NOT VERY OPTIMIZED AT ALL BUT WE WILL DO IT WHEN PRIORITY COMES */
void memory_set (void *target,int byte,unsigned int count)
  {
    while (count--)
      *((char *)target)++ = (char)byte;
  } 

void memory_setup (void)
  {
#if 0
    memory_set (free_page,0,MEMORY_TOTAL_BYTES);
    memory_set (free_page_list,0,MEMORY_TOTAL_ORDERS *sizeof (struct memory_free_page *));
#endif
    memory_set (free_page_order + 1,(MEMORY_TOTAL_ORDERS - 1),MEMORY_TOTAL_PAGES);
    free_page_order[0] = MEMORY_TOTAL_ORDERS - 1;
    free_page_list[MEMORY_TOTAL_ORDERS - 1] = free_page;
  }

#ifdef TEST
#  include <stdio.h>
#  include <stdlib.h>
#  if MEMORY_PAGE_USE_SPLAY_TREE

static void dump_splay_node (struct memory_free_page *node,int level)
  {
    if (!node)
      return;
    dump_splay_node (node->less,level+1);
    printf ("\n%*s[%d-%d]",level,"",(node - free_page),(node - free_page) + (1 << get_order (node)) - 1);
    dump_splay_node (node->more,level+1);    
  }

static void dump_splay_tree (struct memory_free_page *root)
  {
    dump_splay_node (root,2); fflush (stdout);
  }

#  endif

void memory_spy_page (void *address)
  {
    struct memory_free_page *node = (struct memory_free_page *)address;
    int order,used;
    if (node)
      {
        order = get_order (node);
        used = order < 0;
        if (used)
          order = ~order;
        printf("\n(%s,%2d,%7d)",(used ? "used" : "free"),order,get_size (order));
      }
  }

void memory_dump (int order)
  {
    struct memory_free_page *node = free_page_list[order];
    printf("\n(%s,%2d,%7d)",node ? "free" : "none",order,get_size (order));
#  if MEMORY_PAGE_USE_SPLAY_TREE
    dump_splay_tree (node);
#  else
    while (node)
      {
        printf("[%d-%d]",(node - free_page),(node - free_page) + (1<<order) - 1);
        node = node->more;
      }
#  endif

  }

void memory_check (int order)
  {
    struct memory_free_page *node[4096],*swap;
    unsigned int i = 0,j = 0;
    while (i <= 12)
      memory_dump (i++);
    i = 0;
    printf ("\nallocating...\n");
    while (order >= 0)
      {
        j = order;
        while ((swap = memory_allocate_page (j)))
          {
            node[i++] = swap;
            printf("[%d-%d]",(swap - free_page),(swap - free_page) + ((1 << j)-1));
            for (j += (rand () & 15); j > (unsigned int)order; j -= order);
          }
        --order;
      }
    node[i] = 0;
    while (j <= 12)
      memory_dump (j++);
    j = 0;
    printf ("\nreleasing...");
    --i;
    while (i > 0)
      {
        unsigned int k = 0;
        printf ("\n");
        swap = node[k++];
#if 0
        while (swap)
          {
            printf("[%d-%d]",(swap - free_page),(swap - free_page) + ((1 << ~get_order (swap))-1));
            swap = node[k++];
          }
#endif
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
      memory_dump (i++);
    printf("\n\n%s !",(get_order (free_page) == 12) ? "SUCCESS" : "FAILURE");
  }

#endif
