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

#include "memory.h"

#define MEMORY_PAGE_USE_SPLAY_TREE

/*
/////////////////////////////////////////////////////////////////////
// MEMORY PAGE //
/////////////////
//
//
*/

struct __memory_free_page
{
    struct __memory_free_page *less,*more;
    char reserved[MEMORY_PAGE_MINIMAL_SIZE - 2*sizeof (struct memory_free_page *)];
};

#define LESS -1
#define MORE +1

extern struct __memory_free_page __memory_free_page[MEMORY_TOTAL_PAGES] asm("dram");

char __memory_free_page_order[MEMORY_TOTAL_PAGES];
struct __memory_free_page *__memory_free_page_bin[MEMORY_TOTAL_ORDERS];

static inline unsigned int __memory_get_size (int order)
/*
    SH1 has very poor shift instructions (only <<1,>>1,<<2,>>2,<<8,
    >>8,<<16 and >>16), so we should use a lookup table to speedup.
*/ 
{
    return
        (
            (unsigned short [MEMORY_TOTAL_ORDERS])
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
                }
        )[order];
}

static inline struct __memory_free_page *__memory_get_neighbour (struct __memory_free_page *node,unsigned int size)
{
    return ((struct __memory_free_page *)((unsigned)node ^ size));
}

static inline int __memory_get_order (struct __memory_free_page *node)
{
    return __memory_free_page_order[node -  __memory_free_page];
}

static inline void __memory_set_order (struct __memory_free_page *node,int order)
{
    __memory_free_page_order[node -  __memory_free_page] = order;
}

#ifdef MEMORY_PAGE_USE_SPLAY_TREE

static struct __memory_free_page *__memory_splay_page (struct __memory_free_page *root,struct __memory_free_page *node)
{
    struct __memory_free_page *down;
    struct __memory_free_page *less;
    struct __memory_free_page *more;
    struct __memory_free_page *head[2];
    ((struct __memory_free_page *)head)->less =
    ((struct __memory_free_page *)head)->more = 0;
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
                            more             = root;
                            root             = root->less;
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
                            less             = root;
                            root             = root->more;
                            continue;
                        }
                }
            break;
        }
    less->more = root->less;
    more->less = root->more;
    root->less = ((struct __memory_free_page *)head)->more;
    root->more = ((struct __memory_free_page *)head)->less;
    return root;
}

static inline void __memory_insert_page (int order,struct __memory_free_page *node)
{
    struct __memory_free_page *root = __memory_free_page_bin[order];
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

static inline struct __memory_free_page *__memory_pop_page (int order,int want)
{
    struct __memory_free_page *root = __memory_free_page_bin[order];
    if (root)
        {
            root = __memory_splay_page (root,__memory_free_page);
            __memory_free_page_bin[order] = root->more;
            __memory_set_order (root,~want);
        }
    return root;
}

static inline void __memory_remove_page (int order,struct __memory_free_page *node)
{
    struct __memory_free_page *root = __memory_free_page_bin[order];
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

static inline void __memory_insert_page (int order,struct __memory_free_page *node)
{
    struct __memory_free_page *head = __memory_free_page_bin[order];
    node->less = 0;
    node->more = head;
    if (head)
        head->less = node;
    __memory_free_page_bin[order] = node;
    __memory_set_order (node,order);
}

static inline struct __memory_free_page *pop_page (int order,int want)
{
    struct __memory_free_page *node = __memory_free_page_bin[order];
    if (node)
        {
            __memory_free_page_bin[order] = node->more;
            if (node->more)
                node->more->less = 0;
            __memory_set_order (node,~want);
        }
    return node;
}

static inline void __memory_remove_page (int order,struct __memory_free_page *node)
{
    if (node->less)
        node->less->more = node->more;
    else
        __memory_free_page_bin[order] = node->more;
    if (node->more)
        node->more->less = node->less;
}

#endif

static inline void __memory_push_page (int order,struct __memory_free_page *node)
{
    node->less = 0;
    node->more = 0;
    __memory_free_page_bin[order] = node;
    __memory_set_order (node,order);
}

static struct __memory_free_page *__memory_allocate_page (unsigned int size,int order)
{
    struct __memory_free_page *node;
    int min = order;
    while ((unsigned)order <= (MEMORY_TOTAL_ORDERS - 1))
        /* order is valid ? */
        {
            if (!(node = __memory_pop_page (order,min)))
                /* no free page of this order ? */
                {
                    ++order; size <<= 1;
                    continue;
                }
            while (order > min)
                /* split our larger page in smaller pages */
                {
                    --order; size >>= 1;
                    __memory_push_page (order,(struct __memory_free_page *)((unsigned int)node + size));
                }
            return node;
        }
    return MEMORY_RETURN_FAILURE;
}

static inline void __memory_release_page (struct __memory_free_page *node,unsigned int size,int order)
{
    struct __memory_free_page *neighbour;
    while ((order <= (MEMORY_TOTAL_ORDERS - 1)) &&
           ((neighbour = __memory_get_neighbour (node,size)),
           (__memory_get_order (neighbour) == order)))
        /* merge our released page with its contiguous page into a larger page */
        {
            __memory_remove_page (order,neighbour);
            ++order; size <<= 1;
            if (neighbour < node)
                node = neighbour;
        }
    __memory_insert_page (order,node);
}

void *memory_page_allocate (int order)
{
    if ((unsigned)order < MEMORY_TOTAL_ORDERS)
        return MEMORY_RETURN_FAILURE;
    return __memory_allocate_page (__memory_get_size (order),order);
}

int memory_page_release (void *address)
{
    struct __memory_free_page *node = (struct __memory_free_page *)address;
    int order = ~__memory_get_order (node);
    if ((unsigned)order < MEMORY_TOTAL_ORDERS)
        return MEMORY_RETURN_FAILURE;
    __memory_release_page (node,__memory_get_size (order),order);
    return MEMORY_RETURN_SUCCESS;
}

void memory_page_release_range (unsigned int start,unsigned int end)
{
    start = ((start + MEMORY_PAGE_MINIMAL_SIZE - 1) & -MEMORY_PAGE_MINIMAL_SIZE;
    end     = ((end                                                                 ) & -MEMORY_PAGE_MINIMAL_SIZE;
    /* release pages between _start_ and _end_ (each must be 512 bytes long) */
    for (; start < end; start += MEMORY_PAGE_MINIMAL_SIZE)
        memory_page_release (start);
}

static inline void __memory_page_setup (void)
{
#if 0
    memory_set (__memory_free_page_bin,0,MEMORY_TOTAL_ORDERS *sizeof (struct memory_free_page *));
#endif
    /* all pages are marked as used (no free page) */ 
    memory_set (__memory_free_page_order,~0,MEMORY_TOTAL_PAGES);
}

/*
//
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// MEMORY CACHE //
//////////////////
//
//
*/

#if 0


#define MEMORY_MAX_PAGE_ORDER_PER_SLAB (5)
#define MEMORY_MAX_PAGE_SIZE_PER_SLAB (MEMORY_PAGE_MINIMAL_SIZE << MEMORY_MAX_PAGE_ORDER_PER_SLAB)
#define MEMORY_MIN_BLOCKS_PER_SLAB (4)
 
struct __memory_free_block
{
    struct __memory_free_block *link;
};

struct __memory_slab
{
    struct __memory_slab *less,*more; 
    unsigned int free_blocks_left;
    struct __memory_free_block *free_block_list; 
    };

#define WITH_NO_FREE_BLOCK       0
#define WITH_SOME_FREE_BLOCKS  1
#define WITH_ONLY_FREE_BLOCKS  2

struct memory_cache
{
    struct memory_cache  *less;
    struct memory_cache  *more;
    struct memory_cache  *prev;
    struct memory_cache  *next; 
    struct __memory_slab *slab_list[3];
    unsigned int          page_size;
    unsigned int          free_slabs_left;
    unsigned short        size;
    unsigned short        original_size;
    int                   page_order;
    unsigned int          blocks_per_slab;
    struct __memory_slab  cache_slab; /* only used for __memory_cache_cache ! */
};

static inline struct __memory_slab *__memory_push_slab (struct __memory_slab *head,struct __memory_slab *node)
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

static inline struct __memory_slab *__memory_pop_slab (struct __memory_slab *head,struct __memory_slab *node)
{
    if (head)
        head->more = node->more;
    return node->more;
}

static inline struct __memory_slab *__memory_move_slab (struct memory_cache *cache,int from,int to)
{
    struct __memory_slab *head = cache->slab_list[from];
    cache->slab_list[from] = head->more;
    if (head->more)
        head->more->less = 0;
    head->more = cache->slab_list[to];
    if (head->more)
        head->more->prev = head;
    cache->slab_list[to] = head;
    return head;
}

static struct memory_cache *__memory_cache_tree;
static struct memory_cache *__memory_cache_cache;

static inline int __memory_get_order (unsigned size)
{
    int order = 0;
    size = (size + MEMORY_PAGE_MINIMAL_SIZE - 1) & -MEMORY_PAGE_MINIMAL_SIZE;
    while (size > MEMORY_PAGE_MINIMAL_SIZE)
        {
            ++order; size <<= 1;
        }
    return order;
}

static inline struct __memory_slab *__memory_get_slab (struct memory_cache *cache,void *address)
{
    return (struct __memory_slab *)((((unsigned)address + cache->page_size) & -cache->page_size) - sizeof (struct __memory_slab));
}    

static struct memory_cache *__memory_splay_cache (struct memory_cache *root,unsigned int left)
{
    struct memory_cache *down;
    struct memory_cache *less;
    struct memory_cache *more;
    struct memory_cache *head[2];
    ((struct memory_cache *)head->less =
    ((struct memory_cache *)head->more = 0;
    less =
    more = &head;
    while (1)
        {
            if (left < root->free_slabs_left)
                {
                    if ((down = root->less))
                        {
                            if (left < down->free_slabs_left)
                                {
                                    root->less = down->more;
                                    down->more = root;
                                    root = down;
                                    if (!root->less)
                                        break;
                                }
                            more->less = root;
                            more             = root;
                            root             = root->less;
                            continue;
                        }
                    break;
                }
            if (root->free_slabs_left < left)
                {
                    if ((down = root->more))
                        {
                            if (root->free_slabs_left < left)
                                {
                                    root->more = down->less;
                                    down->less = root;
                                    root = down;
                                    if (!root->more)
                                        break;
                                }
                            less->more = root;
                            less             = root;
                            root             = root->more;
                            continue;
                        }
                }
            break;
        }
    less->more = root->less;
    more->less = root->more;
    root->less = ((struct memory_cache *)head->more;
    root->more = ((struct memory_cache *)head->less;
    return root;
}

static inline struct memory_cache *__memory_insert_cache (struct memory_cache *root,struct memory_cache *node)
{
    node->less = 
    node->more =
    node->prev = 0;
    node->next = 0;
    if (root)
        {
            if (node->free_slabs_left == ((root = __memory_splay_cache (root,node))->free_slabs_left))
                {
                    node->less = root.less;
                    node->more = root.more;
                    node->next = root;
                    root->prev = node;
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

static inline struct memory_cache *__memory_remove_cache (struct memory_cache *root,struct memory_cache *node)
{
    if (root)
        {
            if (node->prev)
                {
                    node->prev->next = node->next;
                    if (node->next)
                        node->next->prev = node->prev;
                    return node->prev;
                }
            root = __memory_splay_cache (root,node);
            if (root->less)
                {                
                    node = __memory_splay_page (root->less,node);
                    node->more = root->more;
                }
            else
                node = root->more;
        }
    return node;
}

static inline struct memory_cache *__memory_move_cache (struct memory_cache *root,struct memory_cache *node,int delta)
{
    if ((root = __memory_remove_cache (root,node)))
        {
            node->free_slabs_left += delta;
            root = __memory_insert_cache (root,node);
        }
    return root;
}

static struct __memory_slab *__memory_grow_cache (struct memory_cache *cache)
{
    struct __memory_slab *slab;
    unsigned int page;
    if (cache)
        {
            page = (unsigned int)memory_allocate_page (cache->page_order);
            if (page)
                {
                    struct __memory_free_block *block,**link;
                    slab = (struct __memory_slab *)(page + cache->page_size - sizeof (struct __memory_slab));
                    slab->free_blocks_left = 0;
                    link = &slab->free_block_list;
                    for ((unsigned int)block = page;
                         (unsigned int)block + cache->size < (unsigned int)slab;
                         (unsigned int)block += cache->size)
                        {
                            *link = block;
                            link = &block->link;
                            ++slab->free_blocks_left;
                        }
                    *link = 0;
                    cache->blocks_per_slab = slab->free_blocks_left;
                    cache->slab_list[WITH_ONLY_FREE_BLOCKS] =
                        __memory_push_slab (cache->slab_list[WITH_ONLY_FREE_BLOCKS],slab);
                    __memory_cache_tree = __memory_move_cache (__memory_cache_tree,cache,+1);
                    return slab;
                }
        }
    return MEMORY_RETURN_FAILURE;
}

static int __memory_shrink_cache (struct memory_cache *cache,int all,int move)
{
    struct __memory_slab *slab;
    unsigned int slabs = 0;
    if (cache)
        {
            while ((slab = cache->slab_list[WITH_ONLY_FREE_BLOCKS]))
                {
                    ++slabs;
                    cache->slab_list[WITH_ONLY_FREE_BLOCKS] =
                        __memory_pop_slab (cache->slab_list[WITH_ONLY_FREE_BLOCKS],slab);
                    memory_release_page ((void *)((unsigned int)slab & -cache->page_size));
                    if (all)
                        continue;
                    if (move)
                        __memory_cache_tree = __memory_move_cache (__memory_cache_tree,cache,-slabs);
                    return MEMORY_RETURN_SUCCESS;
                }
        }
    return MEMORY_RETURN_FAILURE;
}

struct memory_cache *memory_cache_create (unsigned int size,int align)
{
    struct memory_cache *cache;
    unsigned int waste = 0,blocks_per_page;
    int page_order;
    unsigned int page_size;
    unsigned int original_size = size;

    size = (align > 4) ? ((size + align - 1) & -align) : ((size + sizeof (int) - 1) & -sizeof (int));

    if ((size >= MEMORY_MAX_PAGE_SIZE_PER_SLAB) ||
        (!(cache = memory_cache_allocate (__memory_cache_cache)))
        return MEMORY_RETURN_FAILURE;

    cache->free_slabs_left  = 0;

    cache->slab_list[  WITH_NO_FREE_BLOCK ] =
    cache->slab_list[WITH_SOME_FREE_BLOCKS] =
    cache->slab_list[WITH_ONLY_FREE_BLOCKS] = 0;

    cache->original_size = original_size;
    cache->size                  = size;

    page_size = 0;
    page_order = MEMORY_PAGE_MINIMAL_SIZE;

    for (;; ++order,(page_size <<= 1))
        {
            if (page_order >= MEMORY_MAX_PAGE_ORDER_PER_SLAB)
                break;

            waste = page_size;
            waste -= sizeof (struct __memory_slab);
                
            blocks_per_slab = waste / size;
            waste -= block_per_slab * size;
                
            if (blocks_per_slab < MEMORY_MIN_BLOCKS_PER_SLAB)
                {
                    ++page_order; page_size <<= 1;
                    continue;
                }       

            /* below 5% of lost space is correct */
            if ((waste << 16) / page_size) < 3276)
                break;
            ++page_order; page_size <<= 1;
        }

    cache->page_size = page_size;
    cache->page_order = page_order;

    cache_tree = __memory_insert_cache (cache_tree,cache);

    return cache;
}

int memory_cache_destroy (struct memory_cache *cache)
{
    /* this function shouldn't be called if there are still used blocks */
    if (cache && !cache->slab_list[WITH_NO_FREE_BLOCK] && !cache->slab_list[WITH_SOME_FREE_BLOCKS])
        {
            __memory_cache_tree = __memory_remove_cache (__memory_cache_tree,cache);
            if (__memory_shrink_cache (cache,1 /* release all free slabs */,0 /* don't move in cache_tree */))
                return memory_cache_release (__memory_cache_cache,cache);
        }
    return MEMORY_RETURN_FAILURE;
}


void *memory_cache_allocate (struct memory_cache *cache)
{
    if (cache) 
        {
            do
                {
                    struct __memory_slab *slab;
                    if ((slab = cache->slab_list[WITH_SOME_FREE_BLOCKS]))
                        {
                            if (slab->free_blocks_left > 0)
                                {
ok:                                 struct __memory_free_block *block = slab->free_block_list;
                                    slab->free_block_list = block->link;
                                    if (--slab->free_blocks_left == 0)
                                        __memory_move_slab (WITH_SOME_FREE_BLOCKS,WITH_NO_FREE_BLOCK);
                                    return block;
                                }
                        }
                    if (cache->slab_list[WITH_FULL_FREE_BLOCKS])
                        {
                            slab = __memory_move_slab (WITH_ONLY_FREE_BLOCKS,WITH_SOME_FREE_BLOCKS);
                            __memory_cache_tree = __memory_move_cache (__memory_cache_tree,cache,-1);
                            goto ok;
                        }
                }
            while (__memory_grow_cache (cache));
        }
    return MEMORY_RETURN_FAILURE;
}

int memory_cache_release (struct memory_cache *cache,void *address)
    {
        struct __memory_slab *slab = __memory_get_slab (cache,address);
        ((struct __memory_free_block *)address)->link = slab->free_block_list;
        slab->free_block_list = (struct __memory_free_block *)address;
        if (slab->free_blocks_left++ == 0)
            __memory_move_slab (WITH_NO_FREE_BLOCK,WITH_SOME_FREE_BLOCKS);
        else if (slab->free_blocks_left == cache->blocks_per_slab)
            {
                __memory_move_slab (WITH_SOME_FREE_BLOCKS,WITH_ONLY_FREE_BLOCKS);
                __memory_cache_tree = __memory_move_cache (__memory_cache_tree,cache,+1);
            }
        return MEMORY_RETURN_SUCCESS;
    }

static inline void __memory_cache_setup (void)
{
    struct memory_cache *cache;
    struct __memory_slab *slab;
    struct __memory_free_block *block,**link;

    cache = (struct memory_cache *)__memory_free_page;
    cache->original_size = sizeof (*cache);
    cache->size                  = sizeof (*cache);
    cache->page_size         = MEMORY_PAGE_MINIMAL_SIZE;
    cache->page_order    = MEMORY_PAGE_MINIMAL_ORDER;
    cache->free_slabs_left = 0; 

    slab = __memory_get_slab (cache,(void *)cache);

    cache->slab_list[  WITH_NO_FREE_BLOCK ] =
    cache->slab_list[WITH_SOME_FREE_BLOCKS] =
    cache->slab_list[WITH_ONLY_FREE_BLOCKS] = 0;

    link = &slab->free_block_list;
    for ((unsigned int)block = (unsigned int)cache;
         (unsigned int)block + sizeof (*cache) < (unsigned int)slab;
         (unsigned int)block += sizeof (*cache))
        {
            *link = block;
            link = &block->link;
            ++slab->free_blocks_left;
        }
    *link = 0;
    cache->blocks_per_slab = slab->free_blocks_left + 1;
    cache->slab_list[WITH_SOME_FREE_BLOCKS] =
        __memory_push_slab (cache->slab_list[WITH_SOME_FREE_BLOCKS],slab);

    __memory_cache_tree = __memory_insert_cache (__memory_cache_tree,cache);
}

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

static struct memory_cache *__memory_free_block_cache[MEMORY_PAGE_MINIMAL_ORDER - 2];

static inline void *__memory_allocate_block (int order)
{
    struct memory_cache *cache = __memory_free_block_cache[order - 2];
    do
        {
            if (cache)
            return memory_cache_allocate (cache);
        }
    while ((__memory_free_block_cache[order] = cache = memory_create_cache (size,0,0)));
    return MEMORY_RETURN_FAILURE;
}

static inline int __memory_release_block (int order,void *address)
{
    struct memory_cache *cache = __memory_free_block_cache[order - 2];
    if (cache)
        return memory_cache_release (cache,address);
    return MEMORY_RETURN_FAILURE;
}

void *memory_block_allocate (int order)
{
    if (order < 2) /* minimal size is 4 bytes */
        order = 2;
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
        return __memory_allocate_block (order);
    return MEMORY_RETURN_FAILURE;
}

int memory_block_release (int order,void *address)
{
    if (order < 2) /* minimal size is 4 bytes */
        order = 2;
    if (order < MEMORY_PAGE_MINIMAL_ORDER)
        return __memory_release_block (order,address);
    return MEMORY_RETURN_FAILURE;
}

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

#endif

#if 0
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
#endif

void memory_setup (void)
{
    __memory_page_setup ();
#if 0
    __memory_cache_setup ();
#endif
}
