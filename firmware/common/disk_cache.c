/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#define __FILESYS_STRUCTS_HDR
#include "config.h"
#include "debug.h"
#include "system.h"
#include "file_internal.h"
#include "linked_list.h"
#include <panic.h>
#include <kernel.h>

/* this helps but file code should have higher compiler optimization which
 * really speeds it up */
#define PROBEFUNC_ATTR      HOT_ATTR
#define SCHED_YIELD_PERIOD  (HZ/20)

/* iocache: LRU cache with randomized binary search tree
 *
 * Does not use locks unless storage access is required. This allows threads
 * to obtain cache hits even if another is blocked by I/O. "Locks" are put in
 * place for the sake of informing the compiler not to move loads and stores
 * and where more appropriate locks would be needed for different scheduling.
 *
 * Buffer indexing is done with a binary search tree
 */

enum iobuf_flags /* flags for each cache entry */
{
    IOBUF_VALID  = 0x01, /* cache probe was a hit, buffer is valid */
    IOBUF_DIRTY  = 0x02, /* entry is dirty in need of writeback */
    IOBUF_CB     = 0x04, /* call the bpb callback for writeback instead */
    IOBUF_BUF    = 0x08, /* entry is being used as a general buffer */
    IOBUF_ERR    = 0x10, /* error occured while processing */
};

struct iocache_buf_desc
{
    unsigned char flags;        /* IOBUF_* flags */
    unsigned char refcount;     /* how many haves zis buffer? */
    unsigned char priority;
    void          *buf;         /* entry's buffer */ 
#ifdef HAVE_MULTIVOLUME
    struct bpb    *bpb;         /* bpb of buffer's volume */
    struct bpb    *bpbwb;       /* wb pending bpb */
#endif
    unsigned long num[0];       /* device buffer number */
                                /* don't want to store sector in two places */
};

struct iocache_entry
{
    struct lldc_node        node;    /* LRU list node (first!) */
    struct iocache_buf_desc bd;      /* buffer data */
    struct searchtree_node  idxnode; /* indexing node (after bd) */
};

struct iocache_bdwb
{
    struct iocache_buf_desc bd;      /* buffer doing writeback */
    unsigned long           num;     /* buffer number (after bd) */
};

struct iocache_wb
{
    struct ll_node          node;    /* queue node (first!) */
    struct iocache_buf_desc *bdp;    /* buffer replacing this */
    struct iocache_bdwb     bdwb;    /* writeback details */
};

static struct iocache
{
    char buffer[IOC_NUM_ENTRIES][IOCACHE_BUFSIZE] MEM_ALIGN_ATTR;
    struct iocache_entry entry[IOC_NUM_ENTRIES];
#ifndef HAVE_MULTIVOLUME
    struct bpb           *bpb;
#endif
    struct lldc_head     lru;
} ioc MEM_ALIGN_ATTR;

static struct mutex iocache_io_mutex SHAREDBSS_ATTR;

#ifdef HAVE_MULTIVOLUME
#define BD_BPB(bd)  ((bd)->bpb)
#else
#define BD_BPB(bd)  (ioc.bpb)
#endif

/* convert between buffer and index */
#define IOC_IDX_BUF(index)  ((void *)ioc.buffer[(index)])
#define IOC_BUF_IDX(buf)    \
    ((unsigned int)((char (*)[IOCACHE_BUFSIZE])(buf) - ioc.buffer))

/* convert between entry and index */
#define IOC_IDX_ENT(index)  (&ioc.entry[(index)])
#define IOC_ENT_IDX(ent)    ((ent) - ioc.entry)

#define IDXNODE_ENT(node)   container_of(node, struct iocache_entry, idxnode)

/* for now, these are merely a compiler fence - names for visibility */
#define __iocache_lock_init()   do {} while (0)
#define __iocache_lock()        asm volatile("" : : : "memory")
#define __iocache_unlock()      asm volatile("" : : : "memory")

static void buffer_write_back(struct iocache_buf_desc *bd);

/* we may not yield very much if there are a lot of hits and the disk may
 * wrongly spin down as well */
static long next_yield IBSS_ATTR;
static FORCE_INLINE void scheduled_yield_str(struct bpb *bpb)
{
    __vol_active(bpb);
    next_yield = current_tick + SCHED_YIELD_PERIOD;
}

static PROBEFUNC_ATTR void scheduled_yield_yield(void)
{
    yield();
    next_yield = current_tick + SCHED_YIELD_PERIOD;
}

static FORCE_INLINE void scheduled_yield_nostr(struct bpb *bpb)
{
    __vol_active(bpb);
    if (!TIME_BEFORE(current_tick, next_yield))
        scheduled_yield_yield();
}

static void iocache_io_lock_init(void)
{
    mutex_init(&iocache_io_mutex);
}

static FORCE_INLINE void iocache_lock_io(void)
{
    mutex_lock(&iocache_io_mutex);
}

static FORCE_INLINE void iocache_unlock_io(void)
{
    mutex_unlock(&iocache_io_mutex);
}

/** LRU list **/
static void lru_init(void)
{
    lldc_init(&ioc.lru);
}

static void lru_insert(struct lldc_node *node)
{
    lldc_insert_last(&ioc.lru, node);
}

static FORCE_INLINE struct iocache_entry *
    entry_lru_next(struct iocache_entry *ent)
{
    return (struct iocache_entry *)ent->node.next;
}

static FORCE_INLINE struct iocache_entry *
    lru_node_entry(struct lldc_node *node)
{
    return (struct iocache_entry *)node;
}

/* make entry MRU by moving it to the list tail */
static PROBEFUNC_ATTR
void lru_touch(struct lldc_node *node)
{
    struct lldc_head *list = &ioc.lru;
    struct lldc_node *lru = list->head;

    if (node == lru->prev)  /* already MRU */
        ; /**/
    else if (node == lru)   /* is the LRU? just rotate list */
        list->head = lru->next;
    else                    /* somewhere else; move it */
    {
        lldc_remove(list, node);
        lldc_insert_last(list, node);
    }
}

/* find an entry that may be evicted or an invalid one; a query of a certain
 * priority always return the lru of a cached buffer of equal of lesser
 * priority; priorities decay as higher priority buffers are passed by; the
 * global lru is always the victim if nothing else qualifies. */
static PROBEFUNC_ATTR
    struct iocache_entry * lru_select_victim_ent(unsigned int priority)
{
    struct iocache_entry *lru = NULL;

    /* TODO: there must be a better way to perform this */
    struct iocache_entry *ent0 = lru_node_entry(ioc.lru.head);
    if (ent0)
    {
        struct iocache_entry *ent = ent0;

        do
        {
            if (LIKELY(ent->bd.refcount == 0))
            {
                if (priority >= ent->bd.priority)
                {
                    lru = ent;
                    break;
                }

                if (!lru)
                    lru = ent;      /* actually-available lru */

                ent->bd.priority--; /* decay */
            }
            /* else can't touch in-use buffer */
        }
        while ((ent = entry_lru_next(ent)) != ent0);
    }

    if (lru)
        lru->bd.priority = priority;

    return lru;
}

/* grab an unreferenced buffer for general use */
static struct iocache_entry * lru_grab_unreferenced_ent(void)
{
    struct iocache_entry *ent = lru_select_victim_ent(0);
    if (ent && ent != entry_lru_next(ent))
    {
        lldc_remove(&ioc.lru, &ent->node);
        return ent;
    }

    return NULL;
}

/* return entry to the cache list and set it LRU */
static void lru_return_ent(struct iocache_entry *ent)
{
    lldc_insert_first(&ioc.lru, &ent->node);
}

/** writeback queue **/
static void wbq_init(struct bpb *bpb)
{
    ll_init(&bpb->ioc.wbq);
}

static FORCE_INLINE void wbq_add(struct bpb *bpb,
                                 struct iocache_wb *wb)
{
    ll_insert_last(&bpb->ioc.wbq, &wb->node);
}

static FORCE_INLINE struct iocache_wb * wbq_first(struct bpb *bpb)
{
    return (struct iocache_wb *)bpb->ioc.wbq.head;
}

static FORCE_INLINE struct iocache_wb * wbq_get_next(struct ll_head *wbq)
{
    struct iocache_wb *wb = (struct iocache_wb *)wbq->head;
    ll_remove_next(wbq, NULL);
    return wb;
}

static PROBEFUNC_ATTR void wbq_addbuf(struct iocache_entry *ent,
                                      struct iocache_wb *wb)
{
    struct iocache_buf_desc *bd = &ent->bd;
    struct bpb *bpb = BD_BPB(bd);
#ifdef HAVE_MULTIVOLUME
    bd->bpbwb = bd->bpb;
#endif
    wb->bdp  = bd;
    wb->bdwb = *(struct iocache_bdwb *)bd;
    wbq_add(bpb, wb);
}

/* commit all the buffers in the writeback queue */
static PROBEFUNC_ATTR void wbq_commit(struct bpb *bpb)
{
    /* io lock and cache lock must be held */
    struct iocache_wb *wbp;
    struct ll_head *wbq = &bpb->ioc.wbq;
    while ((wbp = wbq_get_next(wbq)))
    {
        /* TODO: Assess whether it's worthwhile to grab the buffer back
           before writeback if it matches the one being loaded and it's
           in the queue. */
        struct iocache_buf_desc *bd = &wbp->bdwb.bd;
        buffer_write_back(bd);
#ifdef HAVE_MULTIVOLUME
        bd->bpbwb = NULL;
#endif
    }
}

/* discard all writeback queue buffers */
static void wbq_discard_all(struct bpb *bpb)
{
    while (wbq_get_next(&bpb->ioc.wbq));
}

/** search tree index **/
static inline void index_init(struct bpb *bpb)
{
    searchtree_init(&bpb->ioc.index);
}

static FORCE_INLINE struct searchtree_node * index_first(struct bpb *bpb)
{
    return searchtree_min(&bpb->ioc.index);
}

static FORCE_INLINE struct searchtree_node * index_next(struct searchtree_node *node)
{
    return searchtree_successor_of(node);
}

static FORCE_INLINE unsigned long index_num(const struct searchtree_node *node)
{
    return searchtree_key_of(node);
}

static FORCE_INLINE
    struct searchtree_node * index_search(struct bpb *bpb,
                                          unsigned long num,
                                          struct searchtree_node **parentp)
{
    return searchtree_search(&bpb->ioc.index, num, parentp);
}

static FORCE_INLINE void index_insert_ent(struct bpb *bpb,
                                          struct iocache_entry *ent,
                                          unsigned long num,
                                          struct searchtree_node *parent)
{
    searchtree_insert(&bpb->ioc.index, &ent->idxnode, num, parent);
}

static FORCE_INLINE void index_remove_ent_adj(struct bpb *bpb,
                                              struct iocache_entry *ent,
                                              unsigned long num,
                                              struct searchtree_node **parentp)
{
    searchtree_remove(&bpb->ioc.index, &ent->idxnode, num, parentp);
}

static FORCE_INLINE void index_remove_ent(struct bpb *bpb,
                                          struct iocache_entry *ent)
{
    searchtree_remove(&bpb->ioc.index, &ent->idxnode, 0, NULL);
}

static FORCE_INLINE bool index_check_range(struct bpb *bpb,
                                           unsigned long *numminp,
                                           unsigned long *nummaxp)
{
    return searchtree_check_range(&bpb->ioc.index, numminp, nummaxp);
}

static FORCE_INLINE
    struct searchtree_node * index_range_start(struct bpb *bpb,
                                               unsigned long nummin,
                                               unsigned long nummax)
{
    return searchtree_range_start(&bpb->ioc.index, nummin, nummax);
}

static FORCE_INLINE unsigned char buffer_ref(struct iocache_buf_desc *bd)
{
    return ++bd->refcount;
}

static FORCE_INLINE unsigned char buffer_rel(struct iocache_buf_desc *bd)
{
    return --bd->refcount;
}

static void buffer_discard(struct iocache_buf_desc *bd)
{
    bd->flags    = 0;
    bd->refcount = 0;
    bd->priority = 0;
}

static PROBEFUNC_ATTR void buffer_write_back(struct iocache_buf_desc *bd)
{
    unsigned int flags = bd->flags;
    unsigned long num = bd->num[0];
    void *buf = bd->buf;
    struct bpb *bpb = BD_BPB(bd);

    bd->flags &= ~(IOBUF_DIRTY|IOBUF_CB);

    __iocache_unlock();

    int rc = (flags & IOBUF_CB) ?
            bpb->ioc.wbcb(bpb, num, 1, buf) :
            storage_write_sectors(IF_MD(bpb->drive,) num, 1, buf);

    if (rc < 0)
    {
        /* check for disk present - if not, ignore it, else panuck */
        if (storage_read_sectors(IF_MD(bpb->drive,) 0, 1, buf) == 0)
            panicf("E:sector wrtbk:n=%lu:rc=%d", num, rc);
    }

    __iocache_lock();
}

/* fill the buffer from storage */
static PROBEFUNC_ATTR int buffer_fill(struct iocache_buf_desc *bd)
{
    __iocache_unlock();

    int rc = storage_read_sectors(IF_MD(bd->bpb->drive,) bd->num[0],
                                  1, bd->buf);
    if (rc < 0)
    {
        DEBUGF("%s:Error reading sector %lu (rc=%d)\n",
               __func__, bd->num[0], rc);
    }

    __iocache_lock();
    return rc;
}

/* search the cache for the specified key and return it if it exists; if it
 * doesn't exist, find a victim, set it up for eviction and initialize it.
 * return info for buffer writeback if any victim was valid and dirty */
static PROBEFUNC_ATTR
    struct iocache_entry * probe_alloc(struct bpb *bpb,
                                       unsigned long num,
                                       unsigned int priority,
                                       struct iocache_wb *wb)
{
    struct searchtree_node *parent;

    /* see if it's cached already */
    struct searchtree_node *node;
    if ((node = index_search(bpb, num, &parent)))
         return IDXNODE_ENT(node);

    /* if cache is full, something has to be evicted; if cache is filling,
       this will return an unused entry */
    struct iocache_entry *ent;
    if (!(ent = lru_select_victim_ent(priority)))
        return NULL;

    struct iocache_buf_desc *bd = &ent->bd;
    if (LIKELY(bd->flags & IOBUF_VALID))
    {
        /* victim entry is valid - this will almost always be the case after
           the cache fills */
        if (bd->flags & IOBUF_DIRTY)
            wbq_addbuf(ent, wb);

        /* don't adjust parent if it's a volume switch; nothing was removed
           from the destination structure */
        struct bpb *bdbpb = BD_BPB(bd);
        index_remove_ent_adj(bdbpb, ent, num,
                             IF_MV( bdbpb != bpb ? NULL : ) &parent);
    }

    index_insert_ent(bpb, ent, num, parent);

    bd->flags = 0;
    /* bd->num[0] is shared with ent->idxnode.key */
#ifdef HAVE_MULTIVOLUME
    bd->bpb   = bpb;
#endif
    return ent;
}

/* obtain a buffer from the cache and fetch data if necessary */
PROBEFUNC_ATTR void * iobuf_cache(struct bpb *bpb,
                                  unsigned long num,
                                  struct iobuf_handle *iobp,
                                  unsigned int priority,
                                  bool fill)
{
    __iocache_lock();
    struct iocache_entry *ent = iobp->h;
    struct iocache_buf_desc *bd = &ent->bd;

    if (LIKELY(ent && (bd->flags & IOBUF_VALID) &&
        num == bd->num[0] IF_MV( && bpb == bd->bpb )))
    {
        /* hit by fast path */
        if (priority > bd->priority)
            bd->priority = priority;

        buffer_ref(bd);
        __iocache_unlock();
        scheduled_yield_nostr(bpb);
        return bd->buf;
    }

    struct iocache_wb wb;
    while (UNLIKELY(!(ent = probe_alloc(bpb, num, priority, &wb))))
    {
        __iocache_unlock();
        yield();
        __iocache_lock();
    }

    bd = &ent->bd;
    buffer_ref(bd);
    iobp->h = ent;

    if (bd->flags & IOBUF_VALID)
    {
        /* hit by probe */
        if (priority > bd->priority)
            bd->priority = priority;

        __iocache_unlock();
        scheduled_yield_nostr(bpb);
        return bd->buf;
    }

    __iocache_unlock();
    iocache_lock_io();
    __iocache_lock();

    /* do the write thing */

    /* first, flush us in case what we want is queued; victim may
       reside here as well */
    if (wbq_first(bpb))
        wbq_commit(bpb);

#ifdef HAVE_MULTIVOLUME
    /* may have to flush victim on a different volume */
    struct bpb *bpbwb = bd->bpbwb;
    if (bpbwb && bpbwb != bpb && wbq_first(bpbwb))
        wbq_commit(bpbwb);
#endif

    void *buf = bd->buf;
    unsigned int flags = bd->flags;

    /* fill it */
    if (flags == 0)
    {
        flags = IOBUF_VALID;

        if (UNLIKELY(fill && buffer_fill(bd) < 0))
        {
            index_remove_ent(bpb, ent);
            bd->priority = 0;
            flags = IOBUF_ERR;
        }

        bd->flags = flags;
    }

    if (UNLIKELY(!(flags & IOBUF_VALID)))
    {
        /* ensure no hits or being chosen again until unreferenced */
        if (buffer_rel(bd) == 0)
            bd->flags = 0;

        iobp->h = NULL;
        buf = NULL;
    }

    __iocache_unlock();
    iocache_unlock_io();

    scheduled_yield_str(bpb);
    return buf;
}

/* release buffer and mark it MRU */
static FORCE_INLINE void release_iobuf_handle(struct iobuf_handle *iobp,
                                              unsigned int addflags)
{
    struct iocache_entry *ent = iobp->h;
#ifdef DEBUG
    struct iocache_buf_desc *bd = &ent->bd;
    if (!ent)
    {
        DEBUGF("%s:invalid handle\n", __func__);
        return;
    }

    /* this is referenced; it shouldn't change */
    if (!(bd->flags & IOBUF_VALID))
    {
        DEBUGF("%s:invalid entry!\n", __func__);
        return;
    }

    /* must have referenced it first */
    if (!bd->refcount)
    {
        DEBUGF("%s:no reference\n", __func__);
        return;
    }
#endif /* DEBUG */

    __iocache_lock();

    if (addflags)
    {
        if ((addflags & IOBUF_CB) && !BD_BPB(&ent->bd)->ioc.wbcb)
            addflags &= ~IOBUF_CB;

        ent->bd.flags |= addflags;
    }

    lru_touch(&ent->node);
    buffer_rel(&ent->bd);

    __iocache_unlock();
}

/* release buffer reference without added dirt */
PROBEFUNC_ATTR void iobuf_release(struct iobuf_handle *iobp)
{
    release_iobuf_handle(iobp, 0);
}

/* release buffer and mark it dirty */
PROBEFUNC_ATTR void iobuf_release_dirty(struct iobuf_handle *iobp)
{
    release_iobuf_handle(iobp, IOBUF_DIRTY);
}

PROBEFUNC_ATTR void iobuf_release_dirty_cb(struct iobuf_handle *iobp)
{
    release_iobuf_handle(iobp, IOBUF_DIRTY|IOBUF_CB);
}

/* read and write whole buffers */
long iocache_readwrite(struct bpb *bpb,
                       unsigned long start,
                       int count,
                       void *buf,
                       bool write)
{
    /* This function knows that these blocks belong to a single file and that
       it is synchronized from above thereby blocking any acquisition by another
       thread of buffers that belong to said file. They get a reference count
       so that evictions don't happen behind our back. */

    if (count <= 0)
        return 0;

    int rc = 0;

    /* limit amount per loop so as not to blow the stack top */
    struct iocache_buf_desc *descs[16+1]; /* +1 for NULL-term */
    struct iocache_buf_desc **curbd = descs;
    struct iocache_buf_desc *bd = NULL;
    bool used_storage = false;

    while (1)
    {
        if (!bd)
        {
            unsigned long nummin = start, nummax = start + count - 1;
            if (UNLIKELY(index_check_range(bpb, &nummin, &nummax)))
            {
                /* acquire a sorted list of cached buffers that fall in the
                   transfer range; writes must write over them and read must
                   read from them in order that data remain coherent */
                iocache_lock_io();
                __iocache_lock();

                /* enforce io ordering */
                if (wbq_first(bpb))
                    wbq_commit(bpb);

                struct searchtree_node *node =
                    index_range_start(bpb, nummin, nummax);

                curbd = descs;

                while (node && index_num(node) <= nummax &&
                       (size_t)(curbd - descs) < ARRAYLEN(descs)-1)
                {
                    bd = &IDXNODE_ENT(node)->bd;
                    *curbd++ = bd;
                    buffer_ref(bd);
                    node = index_next(node);
                }

                __iocache_unlock();
                iocache_unlock_io();

                /* NULL-terminate and rewind */
                *curbd = NULL;
                curbd = descs;
                bd = *curbd;
            }
            /* else storage bulk transfer will finish the rest */
        }

        if (bd && bd->num[0] == start)
        {
            /* do a contiguous run of cached buffers */
            do
            {
                void *dst, *src;
                if (write)
                {
                    /* can set flag here since it's referenced and any
                       others interested in it are excluded */
                    bd->flags |= IOBUF_DIRTY;
                    dst = bd->buf, src = buf;
                }
                else
                {
                    dst = buf, src = bd->buf;
                }

                memcpy(dst, src, IOCACHE_BUFSIZE);

                __iocache_lock();
                buffer_rel(bd);
                __iocache_unlock();

                if (--count <= 0)
                    goto done;

                start++;
                buf += IOCACHE_BUFSIZE;
            }
            while ((bd = *++curbd) && bd->num[0] == start);

            /* there may be more cached */
            if (!bd)
                continue;
        }

        /* do any gap in the cached buffers or finish the remainder */
        int cnt = bd ? bd->num[0] - start : (unsigned long)count;
        rc = write ? storage_write_sectors(IF_MD(bpb->drive,) start, cnt, buf) :
                     storage_read_sectors(IF_MD(bpb->drive,) start, cnt, buf);
        if (rc < 0)
        {
            /* failed; release everything */
            __iocache_lock();
            for (; bd; bd = *++curbd)
                buffer_rel(bd);
            __iocache_unlock();
            goto done;
        }

        used_storage = true;

        if ((count -= cnt) <= 0)
            goto done;

        start += cnt;
        buf += cnt*IOCACHE_BUFSIZE;
    }

done:
    if (used_storage)
        scheduled_yield_str(bpb);
    else
        scheduled_yield_nostr(bpb);

    return rc;
}

/* initialize everything for the volume - call only when mounting */
void iocache_mount_volume(struct bpb *bpb, iocache_wb_cb_t wbcb)
{
#ifndef HAVE_MULTIVOLUME
    ioc.bpb = bpb;
#endif
    bpb->ioc.wbcb = wbcb;
    wbq_init(bpb);
    index_init(bpb);
}

/* commit all dirty cache entries to storage for a specified volume */
void iocache_flush_volume(struct bpb *bpb)
{
    DEBUGF("%s:volume=%d\n", __func__, IF_MV_VOL(bpb->volume));

    iocache_lock_io();
    __iocache_lock();

    wbq_commit(bpb);

    for (struct searchtree_node *node = index_first(bpb);
         node; node = index_next(node))
    {
        /* if in use, pass it up so that we're coherent; buffer could
           even be under construction */
        struct iocache_buf_desc *bd = &IDXNODE_ENT(node)->bd;
        if (!bd->refcount && (bd->flags & IOBUF_DIRTY))
        {
            buffer_ref(bd);
            buffer_write_back(bd);
            buffer_rel(bd);
        }
    }

    __iocache_unlock();
    iocache_unlock_io();
}

/* discard all cache entries from the specified volume */
void iocache_discard_volume(struct bpb *bpb)
{
    DEBUGF("%s:volume=%d\n", __func__, IF_MV_VOL(bpb->volume));

    /* this should only be done when things are dead on the volume */
    iocache_lock_io();
    __iocache_lock();

    /* shouldn't happen since we must be in the big lock */
    wbq_discard_all(bpb);

    struct searchtree_node *node;
    while ((node = index_first(bpb)))
    {
        struct iocache_entry *ent = IDXNODE_ENT(node);
        if (ent->bd.refcount)
        {
            /* shouldn't happen since we must be in the big lock */
            DEBUGF("%s:discarding referenced entry:%lu\n",
                   __func__, ent->bd.num[0]);
        }

        buffer_discard(&ent->bd);
        index_remove_ent(bpb, ent);
    }

    __iocache_unlock();
    iocache_unlock_io();
}

/* expropriate a buffer from the cache */
void * iocache_get_buffer(void)
{
    __iocache_lock();

    struct iocache_entry *ent;
    while (UNLIKELY(!(ent = lru_grab_unreferenced_ent())))
    {
        __iocache_unlock();
        yield();
        __iocache_lock();
    }

    struct iocache_buf_desc *bd = &ent->bd;

    if (bd->flags & IOBUF_VALID)
    {
        struct bpb *bpb = BD_BPB(bd);

        index_remove_ent(bpb, ent);

        if (bd->flags & IOBUF_DIRTY)
        {
            /* must first write back this buffer */
            struct iocache_wb wb;
            wbq_addbuf(ent, &wb);

            /* finally, ensure it won't hit on any path */
            bd->flags = 0;

            __iocache_unlock();
            iocache_lock_io();
            __iocache_lock();

            wbq_commit(bpb);
            bd->flags = IOBUF_BUF;

            __iocache_unlock();
            iocache_unlock_io();
            return bd->buf;
       }
    }

    bd->flags = IOBUF_BUF;
    __iocache_unlock();

    return bd->buf;
}

/* return buffer to the cache by buffer */
void iocache_release_buffer(void *buf)
{
    unsigned int index = IOC_BUF_IDX(buf);
    if (index >= IOC_NUM_ENTRIES)
        return;

    struct iocache_entry *ent = IOC_IDX_ENT(index);

    __iocache_lock();

    if (ent->bd.flags == IOBUF_BUF)
    {
        ent->bd.flags = 0;
        lru_return_ent(ent);
    }

    __iocache_unlock();
}

/* one-time init at startup */
void iocache_init(void)
{
    __iocache_lock_init();
    iocache_io_lock_init();
    lru_init();

    for (unsigned int i = 0; i < IOC_NUM_ENTRIES; i++)
    {
        struct iocache_entry *ent = IOC_IDX_ENT(i);
        ent->bd.buf = IOC_IDX_BUF(i);
        lru_insert(&ent->node);
    }
}
