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
#if 0

#include <memory.h>

static struct memory_cache *free_block_cache[MEMORY_PAGE_MINIMAL_SIZE - ];
static struct memory_cache *cache_list;

static inline int get_order (unsigned size)
  {
    int order = 0;
    size = (size + sizeof(struct memory_free_block) - 1) & - sizeof(struct memory_free_block);
    while (size > 0)
      {
        ++order; size <<= 1;
      }
    return order;
  }

static inline struct memory_slab *get_slab (struct memory_cache *cache,void *address)
  {
#ifdef TEST
    return (struct memory_slab *)((((unsigned)address + cache->page_size) & -cache->page_size) - sizeof (struct memory_slab));
#else
    return (struct memory_slab *)((free_page + (((unsigned)address - free_page + cache->page_size) & -cache->page_size) - sizeof (struct memory_slab)));
#endif
  }    

static struct memory_cache *splay_cache (struct memory_cache *root,unsigned int left)
  {
    struct memory_cache *down;
    struct memory_cache *less;
    struct memory_cache *more;
    struct memory_cache  head;
    head.less =
    head.more = 0;
    less =
    more = &head;
    while (1)
      {
        if (left < root->left)
          {
            if ((down = root->less))
              {
                if (left < down->left)
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
        if (root->left < left)
          {
            if ((down = root->more))
              {
                if (root->left < left)
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

static inline struct memory_cache *insert_cache (struct memory_cache *root,struct memory_cache *node)
  {
    node->less = 
    node->more =
    node->same = 0;
    if (root)
      {
        if (node->left == ((root = splay_cache (root,node))->left))
          {
            node->less = root.less;
            node->more = root.more;
            node->same = root;
            root->less = node;
          }
        else if (node < root)
          {
            node->less = root->less;
            node->more = root;
            root->less = 0;
          }
        else
          {
            node->less = root;
            node->more = root->more;
            node->more = 0;
          }
      }
    return node;
  }

static inline struct memory_cache *remove_cache (struct memory_cache *root,struct memory_cache *node)
  {
    if (root)
      {
        root = splay_cache (root,node);
        if (root != node)
          {
            node->less->same = node->same;
            if (node->same)
              node->same->less = node->less;
            return root;
          }
        if (root->less)
          {          
            node = splay_page (root->less,node);
            node->more = root->more;
          }
        else
          node = root->more;
      }
    return root;
  }

static inline struct memory_cache *move_cache (struct memory_cache *root,struct memory_cache *node,int delta)
  {
    if ((root = remove_cache (root,node)))
      {
        node->left += delta;
        root = insert_cache (root,node);
      }
    return root;
  }

static inline struct memory_slab *push_slab (struct memory_cache *head,struct memory_cache *node)
  {
    node->less = head;
    if (head)
      {
        node->more = head->more;
        head->more = node;
      }
    else
      node->more = 0;
    return node;
  }

static inline struct memory_slab *pop_slab (struct memory_cache *head,struct memory_cache *node)
  {
    if (head)
      head->more = node->more;
    return node->more;
  }

static inline struct memory_slab *move_slab (struct memory_slab **from,struct memory_slab **to)
  {
    struct memory_slab *head = *from;
    *from = (*from)->more;
    if (*from)
      (*from)->less = head->less;
    head->less = 0;
    head->more = (*to);
    if (*to)
      (*to)->prev = head;
    *to = head;
    return head;
  }


/*****************************************************************************/
/*                             PUBLIC FUNCTIONS                              */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// MEMORY CACHE :
/////////////////
//
//  - memory_grow_cache : allocate a new slab for a cache 
//  - memory_shrink_cache : release free slabs from a cache 
//  - memory_create_cache : create a new cache of size-fixed blocks 
//  - memory_destroy_cache : destroy the cache and release all the slabs  
//  - memory_cache_allocate : allocate a block from the cache
//  - memory_cache_release : release a block in the cache
//

struct memory_slab *memory_grow_cache (struct memory_cache *cache)
  {
    struct memory_slab *slab;
    unsigned int page;
    if (cache)
      {
        page = (unsigned int)memory_allocate_page (cache->page_order);
        if (page)
          {
            struct memory_free_block *block,**link;
            slab = (struct memory_slab *)(page + cache->page_size - sizeof (struct memory_slab));
            slab->free = 0;
            slab->left = 0;
            link = &slab->free;
            for ((unsigned int)block = page;
                 (unsigned int)block + cache->size < (unsigned int)slab;
                 (unsigned int)block += cache->size)
              {
                *link = block;
                link = &block->link;
                ++slab->free;
              }
            *link = 0;
            cache->blocks_per_slab = slab->free;
            cache->reap = push_slab (cache->reap,slab);
            cache_list = move_cache (cache_list,cache,+1);
            return slab;
          }
      }
    return MEMORY_RETURN_FAILURE;
  }

static int memory_shrink_cache (struct memory_cache *cache,int all,int move)
  {
    struct memory_slab *slab;
    unsigned int slabs = 0;
    if (cache)
      {
        while ((slab = cache->reap))
          {
            ++slabs;
            cache->reap = pop_slab (cache->reap,slab);
            memory_release_page ((void *)slab);
            if (all)
              continue;
            if (move)
              cache_list = move_cache (cache_list,cache,-slabs);
            return MEMORY_RETURN_SUCCESS;
          }
      }
    return MEMORY_RETURN_FAILURE;
  }

int memory_shrink_cache (struct memory_cache *cache,int all)
  {
    return shrink_cache (cache,all,1 /* move cache in cache_list */);
  }

struct memory_cache *memory_create_cache (unsigned int size,int align,int flags)
  {
    struct memory_cache *cache;
    unsigned int waste = 0,blocks_per_page;
    int page_order;
    unsigned int page_size;
    unsigned int original_size = size;

    // Align size on 'align' bytes ('align' should equal 1<<n)
    // if 'align' is inferior to 4, 32-bit word alignment is done by default.
    size = (align > 4) ? ((size + align - 1) & -align) : ((size + sizeof (int) - 1) & -sizeof (int));
    if (!(cache = memory_cache_allocate (&cache_cache))
      return MEMORY_RETURN_FAILURE;

    cache->flags =
    cache->left  = 0;

    cache->used =
    cache->free =
    cache->reap = 0;

    cache->original_size = original_size;
    cache->size          = size;

    page_size = 0;
    page_order = MEMORY_PAGE_MINIMAL_SIZE;;

    // Trying to determine what is the best number of pages per slab
    for (;; ++order,(page_size <<= 1))
      {
        if (page_order >= MEMORY_MAXIMUM_PAGE_ORDER_PER_SLAB)
          {
            memory_cache_release (&cache_cache,cache);
            return MEMORY_RETURN_FAILURE;
          }

        waste = page_size;
        waste -= sizeof (struct memory_slab);
        
        blocks_per_slab = waste / size;
        waste -= block_per_slab * size;
        
        if (blocks_per_slab < MEMORY_MINIMUM_BLOCKS_PER_SLAB)
          {
            ++page_order; page_size <<= 1;
	          continue;
          }     

        // below 3% of lost space is correct
        if ((waste << 16) / page_size) < 1967)
	        break;
        ++page_order; page_size <<= 1;
      }

    cache->page_size = page_size;
    cache->page_order = page_order;

    cache_list = insert_cache (cache_list,cache);

    return cache;
  }

int memory_destroy_cache (struct memory_cache *cache)
  {
    if (cache)
      {
        cache_list = remove_cache (cache_list,cache);
        if (shrink_cache (cache,1 /* release all free slabs */,0 /* don't move in cache_list */))
          return memory_cache_release (&cache_cache,cache);
      }
    return MEMORY_RETURN_FAILURE;
  }

void *memory_cache_allocate (struct memory_cache *cache)
  {
    if (cache) 
      {
        do
          {
            struct memory_slab *slab;
            if ((slab = cache->free))
              {
                if (slab->left > 0)
                  {
ok:                 struct memory_free_block *block = slab->free;
                    slab->free = block->link;
                    if (--slab->left == 0)
                      move_slab (&cache->free,&cache->used);
                    return block;
                  }
              }
            if (cache->reap)
              {
                slab = move_slab (&cache->reap,&cache->free);
                cache_list = move_cache (cache_list,cache,-1);
                goto ok;
              }
          }
        while (grow_cache (cache));
      }
    return MEMORY_RETURN_FAILURE;
  }

int memory_cache_release (struct memory_cache *cache,void *address)
  {
    struct memory_slab *slab = get_slab (cache,address);
    slab->free = (struct memory_free_block *)address;
    if (slab->left++ == 0)
      move_slab (&cache->used,&cache->free);
    else if (slab->left == cache->elements_per_slab)
      {
        move_slab (&cache->free,&cache->reap);
        cache_list = move_cache (cache_list,cache,+1);
      }
    return MEMORY_RETURN_SUCCESS;
  }


///////////////////////////////////////////////////////////////////////////////
// MEMORY BLOCK :
/////////////////
//
//  - memory_allocate_small_block : allocate a small block (no page)
//  - memory_release_small_block : release a small block (no page)
//  - memory_allocate_block : allocate a block (or a page)
//  - memory_release_block : release a block (or a page)
//

static inline void *allocate_small_block (int order)
  {
    struct memory_cache *cache = free_block_cache[order];
    do
      {
        if (cache)
          return memory_cache_allocate (cache);
      }
    while ((free_block_cache[order] = cache = memory_create_cache (size,0,0)));
    return MEMORY_RETURN_FAILURE;
  }

void *memory_allocate_small_block (int order)
  {
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
      return allocate_small_block (order)
    return MEMORY_RETURN_FAILURE;
  }

static inline int release_small_block (int order,void *address)
  {
    struct memory_cache *cache = free_block_cache[order];
    if (cache)
      return memory_cache_release (cache,address);
    return MEMORY_RETURN_FAILURE;
  }

int memory_release_small_block (int order,void *address)
  {
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
      return memory_release_small_block (order,address);
    return memory_release_page (address);
  }

void *memory_allocate_block (unsigned int size)
  {
    size += sizeof (int *);
    int order = get_order (size);
    if (size < MEMORY_PAGE_MINIMAL_SIZE)
      {
        int *block = (int *)allocate_block (order);
        *block = order;
        return block;
      }
    if (size < MEMORY_PAGE_MAXIMAL_SIZE)
      return memory_allocate_page (order);
    return MEMORY_RETURN_FAILURE;
  }

int memory_release_block (void *address)
  {
    int order = *((int *)address);
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
      return release_block (order);
    if (order < MEMORY_PAGE_MAXIMAL_ORDER)
      return memory_release_page (address);
    return MEMORY_RETURN_FAILURE;
  }

#endif