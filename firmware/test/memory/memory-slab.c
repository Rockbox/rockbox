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
#ifndef __LIBRARY_MEMORY_C__
#  error "This header file must be included ONLY from memory.c."
#endif
#ifndef __LIBRARY_MEMORY_PAGE_H__
#  define __LIBRARY_MEMORY_PAGE_H__

struct memory_free_block
  {
    struct memory_free_block
      *link;
  };

///////////////////////////////////////////////////////////////////////////////
// MEMORY SLAB :
////////////////
//
//

struct memory_slab
  {
    struct memory_slab
      *less,*more; 
    unsigned int // left == number of free blocks left
      left;
    struct memory_free_block
      *free; 
  };

static inline struct memory_slab *push_slab (struct memory_slab *head,struct memory_slab *node)
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

static inline struct memory_slab *pop_slab (struct memory_slab *head,struct memory_slab *node)
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

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// MEMORY CACHE :
/////////////////
//
//

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

static struct memory_cache *cache_tree;

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

//
/////////////////////
// PUBLIC FUNCTIONS :
/////////////////////
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
            cache_tree = move_cache (cache_tree,cache,+1);
            return slab;
          }
      }
    return MEMORY_RETURN_FAILURE;
  }

static int shrink_cache (struct memory_cache *cache,int all,int move)
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
              cache_tree = move_cache (cache_tree,cache,-slabs);
            return MEMORY_RETURN_SUCCESS;
          }
      }
    return MEMORY_RETURN_FAILURE;
  }

int memory_shrink_cache (struct memory_cache *cache,int all)
  {
    return shrink_cache (cache,all,1 /* move cache in cache_tree */);
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

    cache_tree = insert_cache (cache_tree,cache);

    return cache;
  }

int memory_destroy_cache (struct memory_cache *cache)
  {
    /* FIX ME : this function shouldn't be called if there are still used blocks */
    if (cache && !cache->free && !cache->used)
      {
        cache_tree = remove_cache (cache_tree,cache);
        if (shrink_cache (cache,1 /* release all free slabs */,0 /* don't move in cache_tree */))
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
                cache_tree = move_cache (cache_tree,cache,-1);
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
    ((struct memory_free_block *)address)->link = slab->free;
    slab->free = (struct memory_free_block *)address;
    if (slab->left++ == 0)
      move_slab (&cache->used,&cache->free);
    else if (slab->left == cache->blocks_per_slab)
      {
        move_slab (&cache->free,&cache->reap);
        cache_tree = move_cache (cache_tree,cache,+1);
      }
    return MEMORY_RETURN_SUCCESS;
  }

//
///////////////////////////////////////////////////////////////////////////////

#endif
