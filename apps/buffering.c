/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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
#include "config.h"
#include <string.h>
#include "strlcpy.h"
#include "system.h"
#include "storage.h"
#include "thread.h"
#include "kernel.h"
#include "panic.h"
#include "debug.h"
#include "file.h"
#include "appevents.h"
#include "metadata.h"
#include "bmp.h"
#ifdef HAVE_ALBUMART
#include "albumart.h"
#include "jpeg_load.h"
#include "playback.h"
#endif
#include "buffering.h"
#include "linked_list.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/* #define LOGF_ENABLE */
#include "logf.h"

#define BUF_MAX_HANDLES 384

/* macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define BUFFERING_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/* #define BUFFERING_LOGQUEUES_SYS_TIMEOUT */
#endif

#ifdef BUFFERING_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#ifdef BUFFERING_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif

#define GUARD_BUFSIZE   (32*1024)

/* amount of data to read in one read() call */
#define BUFFERING_DEFAULT_FILECHUNK      (1024*32)

enum handle_flags
{
    H_CANWRAP   = 0x1,   /* Handle data may wrap in buffer */
    H_ALLOCALL  = 0x2,   /* All data must be allocated up front */
    H_FIXEDDATA = 0x4,   /* Data is fixed in position */
};

struct memory_handle {
    struct lld_node hnode;  /* Handle list node (first!) */
    struct lld_node mrunode;/* MRU list node (second!) */
    size_t  size;           /* Size of this structure + its auxilliary data */
    int     id;             /* A unique ID for the handle */
    enum data_type type;    /* Type of data buffered with this handle */
    uint8_t flags;          /* Handle property flags */
    int8_t  pinned;         /* Count of pinnings */
    int8_t  signaled;       /* Stop any attempt at waiting to get the data */
    int     fd;             /* File descriptor to path (-1 if closed) */
    size_t  data;           /* Start index of the handle's data buffer */
    size_t  ridx;           /* Read pointer, relative to the main buffer */
    size_t  widx;           /* Write pointer, relative to the main buffer */
    off_t   filesize;       /* File total length (possibly trimmed at tail) */
    off_t   start;          /* Offset at which we started reading the file */
    off_t   pos;            /* Read position in file */
    off_t volatile end;     /* Offset at which we stopped reading the file */
    char    path[];         /* Path if data originated in a file */
};

/* Minimum allowed handle movement */
#define MIN_MOVE_DELTA      sizeof(struct memory_handle)

struct buf_message_data
{
    int handle_id;
    intptr_t data;
};

static char *buffer;
static char *guard_buffer;

static size_t buffer_len;

/* Configuration */
static size_t conf_watermark = 0; /* Level to trigger filebuf fill */
static size_t high_watermark = 0; /* High watermark for rebuffer */

static struct lld_head handle_list; /* buffer-order handle list */
static struct lld_head mru_cache;   /* MRU-ordered list of handles */
static int num_handles;             /* number of handles in the lists */
static int base_handle_id;

/* Main lock for adding / removing handles */
static struct mutex llist_mutex SHAREDBSS_ATTR;

#define HLIST_HANDLE(node) \
    ({ struct lld_node *__node = (node); \
       (struct memory_handle *)__node; })

#define HLIST_FIRST \
    HLIST_HANDLE(handle_list.head)

#define HLIST_LAST \
    HLIST_HANDLE(handle_list.tail)

#define HLIST_PREV(h) \
    HLIST_HANDLE((h)->hnode.prev)

#define HLIST_NEXT(h) \
    HLIST_HANDLE((h)->hnode.next)

#define MRU_HANDLE(m) \
    container_of((m), struct memory_handle, mrunode)

static struct data_counters
{
    size_t remaining;   /* Amount of data needing to be buffered */
    size_t buffered;    /* Amount of data currently in the buffer */
    size_t useful;      /* Amount of data still useful to the user */
} data_counters;


/* Messages available to communicate with the buffering thread */
enum
{
    Q_BUFFER_HANDLE = 1, /* Request buffering of a handle, this should not be
                            used in a low buffer situation. */
    Q_REBUFFER_HANDLE,   /* Request reset and rebuffering of a handle at a new
                            file starting position. */
    Q_CLOSE_HANDLE,      /* Request closing a handle */

    /* Configuration: */
    Q_START_FILL,        /* Request that the buffering thread initiate a buffer
                            fill at its earliest convenience */
    Q_HANDLE_ADDED,      /* Inform the buffering thread that a handle was added,
                            (which means the disk is spinning) */
};

/* Buffering thread */
static void buffering_thread(void);
static long buffering_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)];
static const char buffering_thread_name[] = "buffering";
static unsigned int buffering_thread_id = 0;
static struct event_queue buffering_queue SHAREDBSS_ATTR;
static struct queue_sender_list buffering_queue_sender_list SHAREDBSS_ATTR;

static void close_fd(int *fd_p)
{
    int fd = *fd_p;
    if (fd >= 0) {
        close(fd);
        *fd_p = -1;
    }
}

/* Ring buffer helper functions */
static inline void * ringbuf_ptr(uintptr_t p)
{
    return buffer + p;
}

static inline uintptr_t ringbuf_offset(const void *ptr)
{
    return (uintptr_t)(ptr - (void *)buffer);
}

/* Buffer pointer (p) plus value (v), wrapped if necessary */
static inline uintptr_t ringbuf_add(uintptr_t p, size_t v)
{
    uintptr_t res = p + v;
    if (res >= buffer_len)
        res -= buffer_len; /* wrap if necssary */
    return res;
}

/* Buffer pointer (p) minus value (v), wrapped if necessary */
/* Interprets p == v as empty */
static inline uintptr_t ringbuf_sub_empty(uintptr_t p, size_t v)
{
    uintptr_t res = p;
    if (p < v)
        res += buffer_len; /* wrap */

    return res - v;
}

/* Buffer pointer (p) minus value (v), wrapped if necessary */
/* Interprets p == v as full */
static inline uintptr_t ringbuf_sub_full(uintptr_t p, size_t v)
{
    uintptr_t res = p;
    if (p <= v)
        res += buffer_len; /* wrap */

    return res - v;
}

/* How far value (v) plus buffer pointer (p1) will cross buffer pointer (p2) */
/* Interprets p1 == p2 as empty */
static inline ssize_t ringbuf_add_cross_empty(uintptr_t p1, size_t v,
                                              uintptr_t p2)
{
    ssize_t res = p1 + v - p2;
    if (p1 >= p2) /* wrap if necessary */
        res -= buffer_len;

    return res;
}

/* How far value (v) plus buffer pointer (p1) will cross buffer pointer (p2) */
/* Interprets p1 == p2 as full */
static inline ssize_t ringbuf_add_cross_full(uintptr_t p1, size_t v,
                                             uintptr_t p2)
{
    ssize_t res = p1 + v - p2;
    if (p1 > p2) /* wrap if necessary */
        res -= buffer_len;

    return res;
}

/* Real buffer watermark */
#define BUF_WATERMARK MIN(conf_watermark, high_watermark)

static size_t bytes_used(void)
{
    struct memory_handle *first = HLIST_FIRST;
    if (!first) {
        return 0;
    }

    return ringbuf_sub_full(HLIST_LAST->widx, ringbuf_offset(first));
}

/*
LINKED LIST MANAGEMENT
======================

add_handle    : Create a new handle
link_handle   : Add a handle to the list
unlink_handle : Remove a handle from the list
find_handle   : Get a handle pointer from an ID
move_handle   : Move a handle in the buffer (with or without its data)

These functions only handle the linked list structure. They don't touch the
contents of the struct memory_handle headers.

Doubly-linked list, not circular.
New handles are added at the tail.

num_handles = N
       NULL <- h0 <-> h1 <-> h2 -> ... <- hN-1 -> NULL
head=> --------^                          ^
tail=> -----------------------------------+

MRU cache is similar except new handles are added at the head and the most-
recently-accessed handle is always moved to the head (if not already there).

*/

static int next_handle_id(void)
{
    static int cur_handle_id = 0;

    int next_hid = cur_handle_id + 1;
    if (next_hid == INT_MAX)
        cur_handle_id = 0; /* next would overflow; reset the counter */
    else
        cur_handle_id = next_hid;

    return next_hid;
}

/* Adds the handle to the linked list */
static void link_handle(struct memory_handle *h)
{
    lld_insert_last(&handle_list, &h->hnode);
    lld_insert_first(&mru_cache, &h->mrunode);
    num_handles++;
}

/* Delete a given memory handle from the linked list */
static void unlink_handle(struct memory_handle *h)
{
    lld_remove(&handle_list, &h->hnode);
    lld_remove(&mru_cache, &h->mrunode);
    num_handles--;
}

/* Adjusts handle list pointers _before_ it's actually moved */
static void adjust_handle_node(struct lld_head *list,
                               struct lld_node *srcnode,
                               struct lld_node *destnode)
{
    if (srcnode->prev) {
        srcnode->prev->next = destnode;
    } else {
        list->head = destnode;
    }

    if (srcnode->next) {
        srcnode->next->prev = destnode;
    } else {
        list->tail = destnode;
    }
}

/* Add a new handle to the linked list and return it. It will have become the
   new current handle.
   flags contains information on how this may be allocated
   data_size must contain the size of what will be in the handle.
   widx_out points to variable to receive first available byte of data area
   returns a valid memory handle if all conditions for allocation are met.
           NULL if there memory_handle itself cannot be allocated or if the
           data_size cannot be allocated and alloc_all is set. */
static struct memory_handle *
add_handle(unsigned int flags, size_t data_size, const char *path,
           size_t *data_out)
{
    /* Gives each handle a unique id */
    if (num_handles >= BUF_MAX_HANDLES)
        return NULL;

    size_t ridx = 0, widx = 0;
    off_t cur_total = 0;

    struct memory_handle *first = HLIST_FIRST;
    if (first) {
        /* Buffer is not empty */
        struct memory_handle *last = HLIST_LAST;
        ridx = ringbuf_offset(first);
        widx = last->data;
        cur_total = last->filesize - last->start;
    }

    if (cur_total > 0) {
        /* the current handle hasn't finished buffering. We can only add
           a new one if there is already enough free space to finish
           the buffering. */
        if (ringbuf_add_cross_full(widx, cur_total, ridx) > 0) {
            /* Not enough space to finish allocation */
            return NULL;
        } else {
            /* Apply all the needed reserve */
            widx = ringbuf_add(widx, cur_total);
        }
    }

    /* Align to align size up */
    size_t pathsize = path ? strlen(path) + 1 : 0;
    size_t adjust = ALIGN_UP(widx, alignof(struct memory_handle)) - widx;
    size_t index = ringbuf_add(widx, adjust);
    size_t handlesize = ALIGN_UP(sizeof(struct memory_handle) + pathsize,
                                 alignof(struct memory_handle));
    size_t len = handlesize + data_size;

    /* First, will the handle wrap? */
    /* If the handle would wrap, move to the beginning of the buffer,
     * or if the data must not but would wrap, move it to the beginning */
    if (index + handlesize > buffer_len ||
        (!(flags & H_CANWRAP) && index + len > buffer_len)) {
        index = 0;
    }

    /* How far we shifted index to align things, must be < buffer_len */
    size_t shift = ringbuf_sub_empty(index, widx);

    /* How much space are we short in the actual ring buffer? */
    ssize_t overlap = first ?
        ringbuf_add_cross_full(widx, shift + len, ridx) :
        ringbuf_add_cross_empty(widx, shift + len, ridx);

    if (overlap > 0 &&
        ((flags & H_ALLOCALL) || (size_t)overlap > data_size)) {
        /* Not enough space for required allocations */
        return NULL;
    }

    /* There is enough space for the required data, initialize the struct */
    struct memory_handle *h = ringbuf_ptr(index);

    h->size     = handlesize;
    h->id       = next_handle_id();
    h->flags    = flags;
    h->pinned   = 0; /* Can be moved */
    h->signaled = 0; /* Data can be waited for */

    /* Save the provided path */
    memcpy(h->path, path, pathsize);

    /* Return the start of the data area */
    *data_out = ringbuf_add(index, handlesize);

    return h;
}

/* Return a pointer to the memory handle of given ID.
   NULL if the handle wasn't found */
static struct memory_handle * find_handle(int handle_id)
{
    struct memory_handle *h = NULL;
    struct lld_node *mru = mru_cache.head;
    struct lld_node *m = mru;

    while (m && MRU_HANDLE(m)->id != handle_id) {
        m = m->next;
    }

    if (m) {
        if (m != mru) {
            lld_remove(&mru_cache, m);
            lld_insert_first(&mru_cache, m);
        }

        h = MRU_HANDLE(m);
    }

    return h;
}

/* Move a memory handle and data_size of its data delta bytes along the buffer.
   delta maximum bytes available to move the handle.  If the move is performed
         it is set to the actual distance moved.
   data_size is the amount of data to move along with the struct.
   returns true if the move is successful and false if the handle is NULL,
           the  move would be less than the size of a memory_handle after
           correcting for wraps or if the handle is not found in the linked
           list for adjustment.  This function has no side effects if false
           is returned. */
static bool move_handle(struct memory_handle **h, size_t *delta,
                        size_t data_size)
{
    struct memory_handle *src;

    if (h == NULL || (src = *h) == NULL)
        return false;

    size_t size_to_move = src->size + data_size;

    /* Align to align size down */
    size_t final_delta = *delta;
    final_delta = ALIGN_DOWN(final_delta, alignof(struct memory_handle));
    if (final_delta < MIN_MOVE_DELTA) {
        /* It's not legal to move less than MIN_MOVE_DELTA */
        return false;
    }

    uintptr_t oldpos = ringbuf_offset(src);
    uintptr_t newpos = ringbuf_add(oldpos, final_delta);
    intptr_t overlap = ringbuf_add_cross_full(newpos, size_to_move, buffer_len);
    intptr_t overlap_old = ringbuf_add_cross_full(oldpos, size_to_move, buffer_len);

    if (overlap > 0) {
        /* Some part of the struct + data would wrap, maybe ok */
        ssize_t correction = 0;
        /* If the overlap lands inside the memory_handle */
        if (!(src->flags & H_CANWRAP)) {
            /* Otherwise the overlap falls in the data area and must all be
             * backed out.  This may become conditional if ever we move
             * data that is allowed to wrap (ie audio) */
            correction = overlap;
        } else if ((uintptr_t)overlap > data_size) {
            /* Correct the position and real delta to prevent the struct from
             * wrapping, this guarantees an aligned delta if the struct size is
             * aligned and the buffer is aligned */
            correction = overlap - data_size;
        }
        if (correction) {
            /* Align correction to align size up */
            correction = ALIGN_UP(correction, alignof(struct memory_handle));
            if (final_delta < correction + MIN_MOVE_DELTA) {
                /* Delta cannot end up less than MIN_MOVE_DELTA */
                return false;
            }
            newpos -= correction;
            overlap -= correction;/* Used below to know how to split the data */
            final_delta -= correction;
        }
    }

    struct memory_handle *dest = ringbuf_ptr(newpos);

    /* Adjust list pointers */
    adjust_handle_node(&handle_list, &src->hnode, &dest->hnode);
    adjust_handle_node(&mru_cache, &src->mrunode, &dest->mrunode);

    /* x = handle(s) following this one...
     * ...if last handle, unmoveable if metadata, only shrinkable if audio.
     * In other words, no legal move can be made that would have the src head
     * and dest tail of the data overlap itself. These facts reduce the
     * problem to four essential permutations.
     *
     * movement: always "clockwise" >>>>
     *
     * (src nowrap, dest nowrap)
     * |0123  x |
     * |  0123x | etc...
     * move: "0123"
     *
     * (src nowrap, dest wrap)
     * |  x0123 |
     * |23x   01|
     * move: "23", "01"
     *
     * (src wrap, dest nowrap)
     * |23   x01|
     * | 0123x  |
     * move: "23", "01"
     *
     * (src wrap, dest wrap)
     * |23 x  01|
     * |123x   0|
     * move: "23", "1", "0"
     */
    if (overlap_old > 0) {
        /* Move over already wrapped data by the final delta */
        memmove(ringbuf_ptr(final_delta), ringbuf_ptr(0), overlap_old);
        if (overlap <= 0)
            size_to_move -= overlap_old;
    }

    if (overlap > 0) {
        /* Move data that now wraps to the beginning */
        size_to_move -= overlap;
        memmove(ringbuf_ptr(0), SKIPBYTES(src, size_to_move),
                overlap_old > 0 ? final_delta : (size_t)overlap);
    }

    /* Move leading fragment containing handle struct */
    memmove(dest, src, size_to_move);

    /* Update the caller with the new location of h and the distance moved */
    *h = dest;
    *delta = final_delta;
    return true;
}


/*
BUFFER SPACE MANAGEMENT
=======================

update_data_counters: Updates the values in data_counters
buffer_handle   : Buffer data for a handle
rebuffer_handle : Seek to a nonbuffered part of a handle by rebuffering the data
shrink_handle   : Free buffer space by moving a handle
fill_buffer     : Call buffer_handle for all handles that have data to buffer

These functions are used by the buffering thread to manage buffer space.
*/

static int update_data_counters(struct data_counters *dc)
{
    size_t buffered  = 0;
    size_t remaining = 0;
    size_t useful    = 0;

    if (dc == NULL)
        dc = &data_counters;

    mutex_lock(&llist_mutex);

    int num = num_handles;
    struct memory_handle *m = find_handle(base_handle_id);
    bool is_useful = m == NULL;

    for (m = HLIST_FIRST; m; m = HLIST_NEXT(m))
    {
        off_t pos = m->pos;
        off_t end = m->end;

        buffered  += end - m->start;
        remaining += m->filesize - end;

        if (m->id == base_handle_id)
            is_useful = true;

        if (is_useful)
            useful += end - pos;
    }

    mutex_unlock(&llist_mutex);

    dc->buffered  = buffered;
    dc->remaining = remaining;
    dc->useful    = useful;

    return num;
}

/* Q_BUFFER_HANDLE event and buffer data for the given handle.
   Return whether or not the buffering should continue explicitly.  */
static bool buffer_handle(int handle_id, size_t to_buffer)
{
    logf("buffer_handle(%d, %lu)", handle_id, (unsigned long)to_buffer);
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return true;

    logf("  type: %d", (int)h->type);

    if (h->end >= h->filesize) {
        /* nothing left to buffer */
        return true;
    }

    if (h->fd < 0) { /* file closed, reopen */
        if (h->path[0] != '\0')
            h->fd = open(h->path, O_RDONLY);

        if (h->fd < 0) {
            /* could not open the file, truncate it where it is */
            h->filesize = h->end;
            return true;
        }

        if (h->start)
            lseek(h->fd, h->start, SEEK_SET);
    }

    trigger_cpu_boost();

    if (h->type == TYPE_ID3) {
        if (!get_metadata(ringbuf_ptr(h->data), h->fd, h->path)) {
            /* metadata parsing failed: clear the buffer. */
            wipe_mp3entry(ringbuf_ptr(h->data));
        }
        close_fd(&h->fd);
        h->widx = ringbuf_add(h->data, h->filesize);
        h->end  = h->filesize;
        send_event(BUFFER_EVENT_FINISHED, &handle_id);
        return true;
    }

    bool stop = false;
    while (h->end < h->filesize && !stop)
    {
        /* max amount to copy */
        size_t widx = h->widx;
        ssize_t copy_n = h->filesize - h->end;
        copy_n = MIN(copy_n, BUFFERING_DEFAULT_FILECHUNK);
        copy_n = MIN(copy_n, (off_t)(buffer_len - widx));

        mutex_lock(&llist_mutex);

        /* read only up to available space and stop if it would overwrite
           the next handle; stop one byte early to avoid empty/full alias
           (or else do more complicated arithmetic to differentiate) */
        size_t next = ringbuf_offset(HLIST_NEXT(h) ?: HLIST_FIRST);
        ssize_t overlap = ringbuf_add_cross_full(widx, copy_n, next);

        mutex_unlock(&llist_mutex);

        if (overlap > 0) {
            stop = true;
            copy_n -= overlap;
        }

        if (copy_n <= 0)
            return false; /* no space for read */

        /* rc is the actual amount read */
        ssize_t rc = read(h->fd, ringbuf_ptr(widx), copy_n);

        if (rc <= 0) {
            /* Some kind of filesystem error, maybe recoverable if not codec */
            if (h->type == TYPE_CODEC) {
                logf("Partial codec");
                break;
            }

            logf("File ended %lu bytes early\n",
                 (unsigned long)(h->filesize - h->end));
            h->filesize = h->end;
            break;
        }

        /* Advance buffer and make data available to users */
        h->widx = ringbuf_add(widx, rc);
        h->end += rc;

        yield();

        if (to_buffer == 0) {
            /* Normal buffering - check queue */
            if (!queue_empty(&buffering_queue))
                break;
        } else {
            if (to_buffer <= (size_t)rc)
                break; /* Done */
            to_buffer -= rc;
        }
    }

    if (h->end >= h->filesize) {
        /* finished buffering the file */
        close_fd(&h->fd);
        send_event(BUFFER_EVENT_FINISHED, &handle_id);
    }

    return !stop;
}

/* Close the specified handle id and free its allocation. */
/* Q_CLOSE_HANDLE */
static bool close_handle(int handle_id)
{
    mutex_lock(&llist_mutex);
    struct memory_handle *h = find_handle(handle_id);

    /* If the handle is not found, it is closed */
    if (h) {
        close_fd(&h->fd);
        unlink_handle(h);
    }

    mutex_unlock(&llist_mutex);
    return true;
}

/* Free buffer space by moving the handle struct right before the useful
   part of its data buffer or by moving all the data. */
static struct memory_handle * shrink_handle(struct memory_handle *h)
{
    if (!h)
        return NULL;

    if (h->type == TYPE_PACKET_AUDIO) {
        /* only move the handle struct */
        /* data is pinned by default - if we start moving packet audio,
           the semantics will determine whether or not data is movable
           but the handle will remain movable in either case */
        size_t delta = ringbuf_sub_empty(h->ridx, h->data);

        /* The value of delta might change for alignment reasons */
        if (!move_handle(&h, &delta, 0))
            return h;

        h->data = ringbuf_add(h->data, delta);
        h->start += delta;
    } else {
        /* metadata handle: we can move all of it */
        if (h->pinned || !HLIST_NEXT(h))
            return h; /* Pinned, last handle */

        size_t data_size = h->filesize - h->start;
        uintptr_t handle_distance =
            ringbuf_sub_empty(ringbuf_offset(HLIST_NEXT(h)), h->data);
        size_t delta = handle_distance - data_size;

        /* The value of delta might change for alignment reasons */
        if (!move_handle(&h, &delta, data_size))
            return h;

        size_t olddata = h->data;
        h->data = ringbuf_add(h->data, delta);
        h->ridx = ringbuf_add(h->ridx, delta);
        h->widx = ringbuf_add(h->widx, delta);

        switch (h->type)
        {
            case TYPE_ID3:
                if (h->filesize != sizeof(struct mp3entry))
                    break;
                /* when moving an mp3entry we need to readjust its pointers */
                adjust_mp3entry(ringbuf_ptr(h->data), ringbuf_ptr(h->data),
                                ringbuf_ptr(olddata));
                break;

            case TYPE_BITMAP:
                /* adjust the bitmap's pointer */
                ((struct bitmap *)ringbuf_ptr(h->data))->data =
                    ringbuf_ptr(h->data + sizeof(struct bitmap));
                break;

            default:
                break;
        }
    }

    return h;
}

/* Fill the buffer by buffering as much data as possible for handles that still
   have data left to buffer
   Return whether or not to continue filling after this */
static bool fill_buffer(void)
{
    logf("fill_buffer()");
    mutex_lock(&llist_mutex);

    struct memory_handle *m = shrink_handle(HLIST_FIRST);

    mutex_unlock(&llist_mutex);

    while (queue_empty(&buffering_queue) && m) {
        if (m->end < m->filesize && !buffer_handle(m->id, 0)) {
            m = NULL;
            break;
        }
        m = HLIST_NEXT(m);
    }

    if (m) {
        return true;
    } else {
        /* only spin the disk down if the filling wasn't interrupted by an
           event arriving in the queue. */
        storage_sleep();
        return false;
    }
}

#ifdef HAVE_ALBUMART
/* Given a file descriptor to a bitmap file, write the bitmap data to the
   buffer, with a struct bitmap and the actual data immediately following.
   Return value is the total size (struct + data). */
static int load_image(int fd, const char *path,
                      struct bufopen_bitmap_data *data,
                      size_t bufidx, size_t max_size)
{
    (void)path;
    int rc;
    struct bitmap *bmp = ringbuf_ptr(bufidx);
    struct dim *dim = data->dim;
    struct mp3_albumart *aa = data->embedded_albumart;

    /* get the desired image size */
    bmp->width = dim->width, bmp->height = dim->height;
    /* FIXME: alignment may be needed for the data buffer. */
    bmp->data = ringbuf_ptr(bufidx + sizeof(struct bitmap));

#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    bmp->maskdata = NULL;
#endif
    const int format = FORMAT_NATIVE | FORMAT_DITHER |
                       FORMAT_RESIZE | FORMAT_KEEP_ASPECT;
#ifdef HAVE_JPEG
    if (aa != NULL) {
        lseek(fd, aa->pos, SEEK_SET);
        rc = clip_jpeg_fd(fd, aa->size, bmp, (int)max_size, format, NULL);
    }
    else if (strcmp(path + strlen(path) - 4, ".bmp"))
        rc = read_jpeg_fd(fd, bmp, (int)max_size, format, NULL);
    else
#endif
        rc = read_bmp_fd(fd, bmp, (int)max_size, format, NULL);

    return rc + (rc > 0 ? sizeof(struct bitmap) : 0);
}
#endif /* HAVE_ALBUMART */


/*
MAIN BUFFERING API CALLS
========================

bufopen     : Request the opening of a new handle for a file
bufalloc    : Open a new handle for data other than a file.
bufclose    : Close an open handle
bufseek     : Set the read pointer in a handle
bufadvance  : Move the read pointer in a handle
bufread     : Copy data from a handle into a given buffer
bufgetdata  : Give a pointer to the handle's data

These functions are exported, to allow interaction with the buffer.
They take care of the content of the structs, and rely on the linked list
management functions for all the actual handle management work.
*/


/* Reserve space in the buffer for a file.
   filename: name of the file to open
   offset: offset at which to start buffering the file, useful when the first
           offset bytes of the file aren't needed.
   type: one of the data types supported (audio, image, cuesheet, others
   user_data: user data passed possibly passed in subcalls specific to a
              data_type (only used for image (albumart) buffering so far )
   return value: <0 if the file cannot be opened, or one file already
   queued to be opened, otherwise the handle for the file in the buffer
*/
int bufopen(const char *file, off_t offset, enum data_type type,
            void *user_data)
{
    int handle_id = ERR_BUFFER_FULL;
    size_t data;
    struct memory_handle *h;

    /* No buffer refs until after the mutex_lock call! */

    if (type == TYPE_ID3) {
        /* ID3 case: allocate space, init the handle and return. */
        mutex_lock(&llist_mutex);

        h = add_handle(H_ALLOCALL, sizeof(struct mp3entry), file, &data);

        if (h) {
            handle_id = h->id;

            h->type     = type;
            h->fd       = -1;
            h->data     = data;
            h->ridx     = data;
            h->widx     = data;
            h->filesize = sizeof(struct mp3entry);
            h->start    = 0;
            h->pos      = 0;
            h->end      = 0;

            link_handle(h);

            /* Inform the buffering thread that we added a handle */
            LOGFQUEUE("buffering > Q_HANDLE_ADDED %d", handle_id);
            queue_post(&buffering_queue, Q_HANDLE_ADDED, handle_id);
        }

        mutex_unlock(&llist_mutex);
        return handle_id;
    }
    else if (type == TYPE_UNKNOWN)
        return ERR_UNSUPPORTED_TYPE;
#ifdef APPLICATION
    /* Loading code from memory is not supported in application builds */
    else if (type == TYPE_CODEC)
        return ERR_UNSUPPORTED_TYPE;
#endif
    /* Other cases: there is a little more work. */
    int fd = open(file, O_RDONLY);
    if (fd < 0)
        return ERR_FILE_ERROR;

    size_t size = 0;
#ifdef HAVE_ALBUMART
    if (type == TYPE_BITMAP) {
        /* Bitmaps are resized to the requested dimensions when loaded,
         * so the file size should not be used as it may be too large
         * or too small */
        struct bufopen_bitmap_data *aa = user_data;
        size = BM_SIZE(aa->dim->width, aa->dim->height, FORMAT_NATIVE, false);
        size += sizeof(struct bitmap);

#ifdef HAVE_JPEG
        /* JPEG loading requires extra memory
         * TODO: don't add unncessary overhead for .bmp images! */
        size += JPEG_DECODE_OVERHEAD;
#endif
    }
#endif

    if (size == 0)
        size = filesize(fd);

    unsigned int hflags = 0;
    if (type == TYPE_PACKET_AUDIO || type == TYPE_CODEC)
        hflags |= H_CANWRAP;
    /* Bitmaps need their space allocated up front */
    if (type == TYPE_BITMAP)
        hflags |= H_ALLOCALL;

    size_t adjusted_offset = offset;
    if (adjusted_offset > size)
        adjusted_offset = 0;

    /* Reserve extra space because alignment can move data forward */
    size_t padded_size = STORAGE_PAD(size - adjusted_offset);

    mutex_lock(&llist_mutex);

    h = add_handle(hflags, padded_size, file, &data);
    if (!h) {
        DEBUGF("%s(): failed to add handle\n", __func__);
        mutex_unlock(&llist_mutex);
        close(fd);

        /*warn playback.c if it is trying to buffer too large of an image*/
        if(type == TYPE_BITMAP && padded_size >= buffer_len - 64*1024)
        {
            return ERR_BITMAP_TOO_LARGE;
        }
        return ERR_BUFFER_FULL;

    }

    handle_id = h->id;

    h->type = type;
    h->fd   = -1;

#ifdef STORAGE_WANTS_ALIGN
    /* Don't bother to storage align bitmaps because they are not
     * loaded directly into the buffer.
     */
    if (type != TYPE_BITMAP) {
        /* Align to desired storage alignment */
        size_t alignment_pad = STORAGE_OVERLAP((uintptr_t)adjusted_offset -
                                               (uintptr_t)ringbuf_ptr(data));
        data = ringbuf_add(data, alignment_pad);
    }
#endif /* STORAGE_WANTS_ALIGN */

    h->data  = data;
    h->ridx  = data;
    h->start = adjusted_offset;
    h->pos   = adjusted_offset;

#ifdef HAVE_ALBUMART
    if (type == TYPE_BITMAP) {
        /* Bitmap file: we load the data instead of the file */
        int rc = load_image(fd, file, user_data, data, padded_size);
        if (rc <= 0) {
            handle_id = ERR_FILE_ERROR;
        } else {
            data = ringbuf_add(data, rc);
            size = rc;
            adjusted_offset = rc;
        }
    }
    else
#endif
    if (type == TYPE_CUESHEET) {
        h->fd = fd;
    }

    if (handle_id >= 0) {
        h->widx     = data;
        h->filesize = size;
        h->end      = adjusted_offset;
        link_handle(h);
    }

    mutex_unlock(&llist_mutex);

    if (type == TYPE_CUESHEET) {
        /* Immediately start buffering those */
        LOGFQUEUE("buffering >| Q_BUFFER_HANDLE %d", handle_id);
        queue_send(&buffering_queue, Q_BUFFER_HANDLE, handle_id);
    } else {
        /* Other types will get buffered in the course of normal operations */
        close(fd);

        if (handle_id >= 0) {
            /* Inform the buffering thread that we added a handle */
            LOGFQUEUE("buffering > Q_HANDLE_ADDED %d", handle_id);
            queue_post(&buffering_queue, Q_HANDLE_ADDED, handle_id);
        }
    }

    logf("bufopen: new hdl %d", handle_id);
    return handle_id;

    /* Currently only used for aa loading */
    (void)user_data;
}

/* Open a new handle from data that needs to be copied from memory.
   src is the source buffer from which to copy data. It can be NULL to simply
   reserve buffer space.
   size is the requested size. The call will only be successful if the
   requested amount of data can entirely fit in the buffer without wrapping.
   Return value is the handle id for success or <0 for failure.
*/
int bufalloc(const void *src, size_t size, enum data_type type)
{
    if (type == TYPE_UNKNOWN)
        return ERR_UNSUPPORTED_TYPE;

    int handle_id = ERR_BUFFER_FULL;

    mutex_lock(&llist_mutex);

    size_t data;
    struct memory_handle *h = add_handle(H_ALLOCALL, size, NULL, &data);

    if (h) {
        handle_id = h->id;

        if (src) {
            if (type == TYPE_ID3 && size == sizeof(struct mp3entry)) {
                /* specially take care of struct mp3entry */
                copy_mp3entry(ringbuf_ptr(data), src);
            } else {
                memcpy(ringbuf_ptr(data), src, size);
            }
        }

        h->type      = type;
        h->fd        = -1;
        h->data      = data;
        h->ridx      = data;
        h->widx      = ringbuf_add(data, size);
        h->filesize  = size;
        h->start     = 0;
        h->pos       = 0;
        h->end       = size;

        link_handle(h);
    }

    mutex_unlock(&llist_mutex);

    logf("bufalloc: new hdl %d", handle_id);
    return handle_id;
}

/* Close the handle. Return true for success and false for failure */
bool bufclose(int handle_id)
{
    logf("bufclose(%d)", handle_id);

    if (handle_id <= 0) {
        return true;
    }

    LOGFQUEUE("buffering >| Q_CLOSE_HANDLE %d", handle_id);
    return queue_send(&buffering_queue, Q_CLOSE_HANDLE, handle_id);
}

/* Backend to bufseek and bufadvance. Call only in response to
   Q_REBUFFER_HANDLE! */
static void rebuffer_handle(int handle_id, off_t newpos)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h) {
        queue_reply(&buffering_queue, ERR_HANDLE_NOT_FOUND);
        return;
    }

    /* Check that we still need to do this since the request could have
       possibly been met by this time */
    if (newpos >= h->start && newpos <= h->end) {
        h->ridx = ringbuf_add(h->data, newpos - h->start);
        h->pos  = newpos;
        queue_reply(&buffering_queue, 0);
        return;
    }

    /* When seeking foward off of the buffer, if it is a short seek attempt to
       avoid rebuffering the whole track, just read enough to satisfy */
    off_t amount = newpos - h->pos;

    if (amount > 0 && amount <= BUFFERING_DEFAULT_FILECHUNK) {
        h->ridx = ringbuf_add(h->data, newpos - h->start);
        h->pos  = newpos;

        if (buffer_handle(handle_id, amount + 1) && h->end >= h->pos) {
            /* It really did succeed */
            queue_reply(&buffering_queue, 0);
            buffer_handle(handle_id, 0); /* Ok, try the rest */
            return;
        }
        /* Data collision or other file error - must reset */

        if (newpos > h->filesize)
            newpos = h->filesize; /* file truncation happened above */
    }

    mutex_lock(&llist_mutex);

    size_t next = ringbuf_offset(HLIST_NEXT(h) ?: HLIST_FIRST);

#ifdef STORAGE_WANTS_ALIGN
    /* Strip alignment padding then redo */
    size_t new_index = ringbuf_add(ringbuf_offset(h), h->size);

    /* Align to desired storage alignment if space permits - handle could
       have been shrunken too close to the following one after a previous
       rebuffer. */
    size_t alignment_pad = STORAGE_OVERLAP((uintptr_t)newpos -
                                           (uintptr_t)ringbuf_ptr(new_index));

    if (ringbuf_add_cross_full(new_index, alignment_pad, next) > 0)
        alignment_pad = 0; /* Forego storage alignment this time */

    new_index = ringbuf_add(new_index, alignment_pad);
#else
    /* Just clear the data buffer */
    size_t new_index = h->data;
#endif /* STORAGE_WANTS_ALIGN */

    /* Reset the handle to its new position */
    h->ridx = h->widx = h->data = new_index;
    h->start = h->pos = h->end = newpos;

    if (h->fd >= 0)
        lseek(h->fd, newpos, SEEK_SET);

    off_t filerem = h->filesize - newpos;
    bool send = HLIST_NEXT(h) &&
                ringbuf_add_cross_full(new_index, filerem, next) > 0;

    mutex_unlock(&llist_mutex);

    if (send) {
        /* There isn't enough space to rebuffer all of the track from its new
           offset, so we ask the user to free some */
        DEBUGF("%s(): space is needed\n", __func__);
        send_event(BUFFER_EVENT_REBUFFER, &(int){ handle_id });
    }

    /* Now we do the rebuffer */
    queue_reply(&buffering_queue, 0);
    buffer_handle(handle_id, 0);
}

/* Backend to bufseek and bufadvance */
static int seek_handle(struct memory_handle *h, off_t newpos)
{
    if ((newpos < h->start || newpos >= h->end) &&
        (newpos < h->filesize || h->end < h->filesize)) {
        /* access before or after buffered data and not to end of file or file
           is not buffered to the end-- a rebuffer is needed. */
        return queue_send(&buffering_queue, Q_REBUFFER_HANDLE,
                    (intptr_t)&(struct buf_message_data){ h->id, newpos });
    }
    else {
        h->ridx = ringbuf_add(h->data, newpos - h->start);
        h->pos  = newpos;
        return 0;
    }
}

/* Set reading index in handle (relatively to the start of the file).
   Access before the available data will trigger a rebuffer.
   Return 0 for success and for failure:
     ERR_HANDLE_NOT_FOUND if the handle wasn't found
     ERR_INVALID_VALUE if the new requested position was beyond the end of
     the file
*/
int bufseek(int handle_id, size_t newpos)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (newpos > (size_t)h->filesize)
        return ERR_INVALID_VALUE;

    return seek_handle(h, newpos);
}

/* Advance the reading index in a handle (relatively to its current position).
   Return 0 for success and for failure:
     ERR_HANDLE_NOT_FOUND if the handle wasn't found
     ERR_INVALID_VALUE if the new requested position was before the beginning
     or beyond the end of the file
 */
int bufadvance(int handle_id, off_t offset)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    off_t pos = h->pos;

    if ((offset < 0 && offset < -pos) ||
        (offset >= 0 && offset > h->filesize - pos))
        return ERR_INVALID_VALUE;

    return seek_handle(h, pos + offset);
}

/* Get the read position from the start of the file
   Returns the offset from byte 0 of the file and for failure:
     ERR_HANDLE_NOT_FOUND if the handle wasn't found
 */
off_t bufftell(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    return h->pos;
}

/* Used by bufread and bufgetdata to prepare the buffer and retrieve the
 * actual amount of data available for reading. It does range checks on
 * size and returns a valid (and explicit) amount of data for reading */
static struct memory_handle *prep_bufdata(int handle_id, size_t *size,
                                          bool guardbuf_limit)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return NULL;

    if (h->pos >= h->filesize) {
        /* File is finished reading */
        *size = 0;
        return h;
    }

    off_t realsize = *size;
    off_t filerem = h->filesize - h->pos;

    if (realsize <= 0 || realsize > filerem)
        realsize = filerem; /* clip to eof */

    if (guardbuf_limit && realsize > GUARD_BUFSIZE) {
        logf("data request > guardbuf");
        /* If more than the size of the guardbuf is requested and this is a
         * bufgetdata, limit to guard_bufsize over the end of the buffer */
        realsize = MIN((size_t)realsize, buffer_len - h->ridx + GUARD_BUFSIZE);
        /* this ensures *size <= buffer_len - h->ridx + GUARD_BUFSIZE */
    }

    off_t end = h->end;
    off_t wait_end = h->pos + realsize;

    if (end < wait_end && end < h->filesize) {
        /* Wait for the data to be ready */
        unsigned int request = 1;

        do
        {
            if (--request == 0) {
                request = 100;
                /* Data (still) isn't ready; ping buffering thread */
                LOGFQUEUE("buffering >| Q_START_FILL %d",handle_id);
                queue_send(&buffering_queue, Q_START_FILL, handle_id);
            }

            sleep(0);
            /* it is not safe for a non-buffering thread to sleep while
             * holding a handle */
            h = find_handle(handle_id);
            if (!h)
                return NULL;

            if (h->signaled != 0)
                return NULL; /* Wait must be abandoned */

            end = h->end;
        }
        while (end < wait_end && end < h->filesize);

        filerem = h->filesize - h->pos;
        if (realsize > filerem)
            realsize = filerem;
    }

    *size = realsize;
    return h;
}


/* Note: It is safe for the thread responsible for handling the rebuffer
 * cleanup request to call bufread or bufgetdata only when the data will
 * be available-- not if it could be blocked waiting for it in prep_bufdata.
 * It should be apparent that if said thread is being forced to wait for
 * buffering but has not yet responded to the cleanup request, the space
 * can never be cleared to allow further reading of the file because it is
 * not listening to callbacks any longer. */

/* Copy data from the given handle to the dest buffer.
   Return the number of bytes copied or < 0 for failure (handle not found).
   The caller is blocked until the requested amount of data is available.
*/
ssize_t bufread(int handle_id, size_t size, void *dest)
{
    const struct memory_handle *h =
        prep_bufdata(handle_id, &size, false);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->ridx + size > buffer_len) {
        /* the data wraps around the end of the buffer */
        size_t read = buffer_len - h->ridx;
        memcpy(dest, ringbuf_ptr(h->ridx), read);
        memcpy(dest + read, ringbuf_ptr(0), size - read);
    } else {
        memcpy(dest, ringbuf_ptr(h->ridx), size);
    }

    return size;
}

/* Update the "data" pointer to make the handle's data available to the caller.
   Return the length of the available linear data or < 0 for failure (handle
   not found).
   The caller is blocked until the requested amount of data is available.
   size is the amount of linear data requested. it can be 0 to get as
   much as possible.
   The guard buffer may be used to provide the requested size. This means it's
   unsafe to request more than the size of the guard buffer.
*/
ssize_t bufgetdata(int handle_id, size_t size, void **data)
{
    struct memory_handle *h =
        prep_bufdata(handle_id, &size, true);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->ridx + size > buffer_len) {
        /* the data wraps around the end of the buffer :
           use the guard buffer to provide the requested amount of data. */
        size_t copy_n = h->ridx + size - buffer_len;
        /* prep_bufdata ensures
           adjusted_size <= buffer_len - h->ridx + GUARD_BUFSIZE,
           so copy_n <= GUARD_BUFSIZE */
        memcpy(guard_buffer, ringbuf_ptr(0), copy_n);
    }

    if (data)
        *data = ringbuf_ptr(h->ridx);

    return size;
}

/*
SECONDARY EXPORTED FUNCTIONS
============================

buf_handle_offset
buf_set_base_handle
buf_handle_data_type
buf_is_handle
buf_pin_handle
buf_signal_handle
buf_length
buf_used
buf_set_watermark
buf_get_watermark

These functions are exported, to allow interaction with the buffer.
They take care of the content of the structs, and rely on the linked list
management functions for all the actual handle management work.
*/
bool buf_is_handle(int handle_id)
{
    return find_handle(handle_id) != NULL;
}

int buf_handle_data_type(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->type;
}

off_t buf_filesize(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->filesize;
}

off_t buf_handle_offset(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->start;
}

off_t buf_handle_remaining(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->filesize - h->end;
}

bool buf_pin_handle(int handle_id, bool pin)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return false;

    if (pin) {
        h->pinned++;
    } else if (h->pinned > 0) {
        h->pinned--;
    }

    return true;
}

bool buf_signal_handle(int handle_id, bool signal)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return false;

    h->signaled = signal ? 1 : 0;
    return true;
}

/* Return the size of the ringbuffer */
size_t buf_length(void)
{
    return buffer_len;
}

/* Set the handle from which useful data is counted */
void buf_set_base_handle(int handle_id)
{
    mutex_lock(&llist_mutex);
    base_handle_id = handle_id;
    mutex_unlock(&llist_mutex);
}

/* Return the amount of buffer space used */
size_t buf_used(void)
{
    mutex_lock(&llist_mutex);
    size_t used = bytes_used();
    mutex_unlock(&llist_mutex);
    return used;
}

void buf_set_watermark(size_t bytes)
{
    conf_watermark = bytes;
}

size_t buf_get_watermark(void)
{
    return BUF_WATERMARK;
}

/** -- buffer thread helpers -- **/
static void shrink_buffer(void)
{
    logf("shrink_buffer()");

    mutex_lock(&llist_mutex);

    for (struct memory_handle *h = HLIST_LAST; h; h = HLIST_PREV(h)) {
        h = shrink_handle(h);
    }

    mutex_unlock(&llist_mutex);
}

static void NORETURN_ATTR buffering_thread(void)
{
    bool filling = false;
    struct queue_event ev;

    while (true)
    {
        if (num_handles > 0) {
            if (!filling) {
                cancel_cpu_boost();
            }
            queue_wait_w_tmo(&buffering_queue, &ev, filling ? 1 : HZ/2);
        } else {
            filling = false;
            cancel_cpu_boost();
            queue_wait(&buffering_queue, &ev);
        }

        switch (ev.id)
        {
            case Q_START_FILL:
                LOGFQUEUE("buffering < Q_START_FILL %d", (int)ev.data);
                shrink_buffer();
                queue_reply(&buffering_queue, 1);
                if (buffer_handle((int)ev.data, 0)) {
                    filling = true;
                }
                else if (num_handles > 0 && conf_watermark > 0) {
                    update_data_counters(NULL);
                    if (data_counters.useful >= BUF_WATERMARK) {
                        send_event(BUFFER_EVENT_BUFFER_LOW, NULL);
                    }
                }
                break;

            case Q_BUFFER_HANDLE:
                LOGFQUEUE("buffering < Q_BUFFER_HANDLE %d", (int)ev.data);
                queue_reply(&buffering_queue, 1);
                buffer_handle((int)ev.data, 0);
                break;

            case Q_REBUFFER_HANDLE:
            {
                struct buf_message_data *parm =
                    (struct buf_message_data *)ev.data;
                LOGFQUEUE("buffering < Q_REBUFFER_HANDLE %d %ld",
                          parm->handle_id, parm->data);
                rebuffer_handle(parm->handle_id, parm->data);
                break;
                }

            case Q_CLOSE_HANDLE:
                LOGFQUEUE("buffering < Q_CLOSE_HANDLE %d", (int)ev.data);
                queue_reply(&buffering_queue, close_handle((int)ev.data));
                break;

            case Q_HANDLE_ADDED:
                LOGFQUEUE("buffering < Q_HANDLE_ADDED %d", (int)ev.data);
                /* A handle was added: the disk is spinning, so we can fill */
                filling = true;
                break;

            case SYS_TIMEOUT:
                LOGFQUEUE_SYS_TIMEOUT("buffering < SYS_TIMEOUT");
                break;
        }

        if (num_handles == 0 || !queue_empty(&buffering_queue))
            continue;

        update_data_counters(NULL);

        if (filling) {
            filling = data_counters.remaining > 0 ? fill_buffer() : false;
        } else if (ev.id == SYS_TIMEOUT) {
            if (data_counters.useful < BUF_WATERMARK) {
                /* The buffer is low and we're idle, just watching the levels
                   - call the callbacks to get new data */
                send_event(BUFFER_EVENT_BUFFER_LOW, NULL);

                /* Continue anything else we haven't finished - it might
                   get booted off or stop early because the receiver hasn't
                   had a chance to clear anything yet */
                if (data_counters.remaining > 0) {
                    shrink_buffer();
                    filling = fill_buffer();
                }
            }
        }
    }
}

void INIT_ATTR buffering_init(void)
{
    mutex_init(&llist_mutex);

    /* Thread should absolutely not respond to USB because if it waits first,
       then it cannot properly service the handles and leaks will happen -
       this is a worker thread and shouldn't need to care about any system
       notifications.
                                      ***
       Whoever is using buffering should be responsible enough to clear all
       the handles at the right time. */
    queue_init(&buffering_queue, false);
    buffering_thread_id = create_thread( buffering_thread, buffering_stack,
            sizeof(buffering_stack), CREATE_THREAD_FROZEN,
            buffering_thread_name IF_PRIO(, PRIORITY_BUFFERING)
            IF_COP(, CPU));

    queue_enable_queue_send(&buffering_queue, &buffering_queue_sender_list,
                            buffering_thread_id);
}

/* Initialise the buffering subsystem */
bool buffering_reset(char *buf, size_t buflen)
{
    /* Wraps of storage-aligned data must also be storage aligned,
       thus buf and buflen must be a aligned to an integer multiple of
       the storage alignment */

    if (buf) {
        buflen -= MIN(buflen, GUARD_BUFSIZE);

        STORAGE_ALIGN_BUFFER(buf, buflen);

        if (!buf || !buflen)
            return false;
    } else {
        buflen = 0;
    }

    send_event(BUFFER_EVENT_BUFFER_RESET, NULL);

    /* If handles weren't closed above, just do it */
    struct memory_handle *h;
    while ((h = HLIST_FIRST)) {
        bufclose(h->id);
    }

    buffer = buf;
    buffer_len = buflen;
    guard_buffer = buf + buflen;

    lld_init(&handle_list);
    lld_init(&mru_cache);

    num_handles = 0;
    base_handle_id = -1;

    /* Set the high watermark as 75% full...or 25% empty :)
       This is the greatest fullness that will trigger low-buffer events
       no matter what the setting because high-bitrate files can have
       ludicrous margins that even exceed the buffer size - most common
       with a huge anti-skip buffer but even without that setting,
       staying constantly active in buffering is pointless */
    high_watermark = 3*buflen / 4;

    thread_thaw(buffering_thread_id);

    return true;
}

void buffering_get_debugdata(struct buffering_debug *dbgdata)
{
    struct data_counters dc;
    dbgdata->num_handles = update_data_counters(&dc);
    dbgdata->data_rem = dc.remaining;
    dbgdata->buffered_data = dc.buffered;
    dbgdata->useful_data = dc.useful;
    dbgdata->watermark = BUF_WATERMARK;
}
