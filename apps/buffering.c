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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "buffering.h"

#include "storage.h"
#include "system.h"
#include "thread.h"
#include "file.h"
#include "panic.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"
#include "settings.h"
#include "codecs.h"
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "screens.h"
#include "playlist.h"
#include "pcmbuf.h"
#include "appevents.h"
#include "metadata.h"
#ifdef HAVE_ALBUMART
#include "albumart.h"
#include "jpeg_load.h"
#include "bmp.h"
#include "playback.h"
#endif

#define GUARD_BUFSIZE   (32*1024)

/* Define LOGF_ENABLE to enable logf output in this file */
/* #define LOGF_ENABLE */
#include "logf.h"

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

/* amount of data to read in one read() call */
#define BUFFERING_DEFAULT_FILECHUNK      (1024*32)

#define BUF_HANDLE_MASK                  0x7FFFFFFF


/* assert(sizeof(struct memory_handle)%4==0) */
struct memory_handle {
    int id;                    /* A unique ID for the handle */
    enum data_type type;       /* Type of data buffered with this handle */
    int8_t pinned;             /* Count of references */
    int8_t signaled;           /* Stop any attempt at waiting to get the data */
    char path[MAX_PATH];       /* Path if data originated in a file */
    int fd;                    /* File descriptor to path (-1 if closed) */
    size_t data;               /* Start index of the handle's data buffer */
    volatile size_t ridx;      /* Read pointer, relative to the main buffer */
    size_t widx;               /* Write pointer, relative to the main buffer */
    size_t filesize;           /* File total length */
    size_t filerem;            /* Remaining bytes of file NOT in buffer */
    volatile size_t available; /* Available bytes to read from buffer */
    size_t offset;             /* Offset at which we started reading the file */
    struct memory_handle *next;
};
/* invariant: filesize == offset + available + filerem */


struct buf_message_data
{
    int handle_id;
    intptr_t data;
};

static char *buffer;
static char *guard_buffer;

static size_t buffer_len;

static volatile size_t buf_widx;  /* current writing position */
static volatile size_t buf_ridx;  /* current reading position */
/* buf_*idx are values relative to the buffer, not real pointers. */

/* Configuration */
static size_t conf_watermark = 0; /* Level to trigger filebuf fill */
static size_t high_watermark = 0; /* High watermark for rebuffer */

/* current memory handle in the linked list. NULL when the list is empty. */
static struct memory_handle *cur_handle;
/* first memory handle in the linked list. NULL when the list is empty. */
static struct memory_handle *first_handle;

static int num_handles;  /* number of handles in the list */

static int base_handle_id;

/* Main lock for adding / removing handles */
static struct mutex llist_mutex SHAREDBSS_ATTR;

/* Handle cache (makes find_handle faster).
   This is global so that move_handle and rm_handle can invalidate it. */
static struct memory_handle *cached_handle = NULL;

static struct data_counters
{
    size_t remaining;   /* Amount of data needing to be buffered */
    size_t wasted;      /* Amount of space available for freeing */
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



/* Ring buffer helper functions */

static inline uintptr_t ringbuf_offset(const void *ptr)
{
    return (uintptr_t)(ptr - (void*)buffer);
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
static inline uintptr_t ringbuf_sub(uintptr_t p, size_t v)
{
    uintptr_t res = p;
    if (p < v)
        res += buffer_len; /* wrap */
        
    return res - v;
}


/* How far value (v) plus buffer pointer (p1) will cross buffer pointer (p2) */
static inline ssize_t ringbuf_add_cross(uintptr_t p1, size_t v, uintptr_t p2)
{
    ssize_t res = p1 + v - p2;
    if (p1 >= p2) /* wrap if necessary */
        res -= buffer_len;

    return res;
}

/* Bytes available in the buffer */
#define BUF_USED ringbuf_sub(buf_widx, buf_ridx)

/* Real buffer watermark */
#define BUF_WATERMARK MIN(conf_watermark, high_watermark)

/*
LINKED LIST MANAGEMENT
======================

add_handle  : Add a handle to the list
rm_handle   : Remove a handle from the list
find_handle : Get a handle pointer from an ID
move_handle : Move a handle in the buffer (with or without its data)

These functions only handle the linked list structure. They don't touch the
contents of the struct memory_handle headers. They also change the buf_*idx
pointers when necessary and manage the handle IDs.

The first and current (== last) handle are kept track of.
A new handle is added at buf_widx and becomes the current one.
buf_widx always points to the current writing position for the current handle
buf_ridx always points to the location of the first handle.
buf_ridx == buf_widx means the buffer is empty.
*/


/* Add a new handle to the linked list and return it. It will have become the
   new current handle.
   data_size must contain the size of what will be in the handle.
   can_wrap tells us whether this type of data may wrap on buffer
   alloc_all tells us if we must immediately be able to allocate data_size
   returns a valid memory handle if all conditions for allocation are met.
           NULL if there memory_handle itself cannot be allocated or if the
           data_size cannot be allocated and alloc_all is set. */
static struct memory_handle *add_handle(size_t data_size, bool can_wrap,
                                        bool alloc_all)
{
    /* gives each handle a unique id */
    static int cur_handle_id = 0;
    size_t shift;
    size_t widx, new_widx;
    size_t len;
    ssize_t overlap;

    if (num_handles >= BUF_MAX_HANDLES)
        return NULL;

    widx = buf_widx;

    if (cur_handle && cur_handle->filerem > 0) {
        /* the current handle hasn't finished buffering. We can only add
           a new one if there is already enough free space to finish
           the buffering. */
        size_t req = cur_handle->filerem;
        if (ringbuf_add_cross(cur_handle->widx, req, buf_ridx) >= 0) {
            /* Not enough space to finish allocation */
            return NULL;
        } else {
            /* Allocate the remainder of the space for the current handle */
            widx = ringbuf_add(cur_handle->widx, cur_handle->filerem);
        }
    }

    /* align to 4 bytes up always leaving a gap */
    new_widx = ringbuf_add(widx, 4) & ~3;

    len = data_size + sizeof(struct memory_handle);

    /* First, will the handle wrap? */
    /* If the handle would wrap, move to the beginning of the buffer,
     * or if the data must not but would wrap, move it to the beginning */
    if (new_widx + sizeof(struct memory_handle) > buffer_len ||
                   (!can_wrap && new_widx + len > buffer_len)) {
        new_widx = 0;
    }

    /* How far we shifted the new_widx to align things, must be < buffer_len */
    shift = ringbuf_sub(new_widx, widx);

    /* How much space are we short in the actual ring buffer? */
    overlap = ringbuf_add_cross(widx, shift + len, buf_ridx);
    if (overlap >= 0 && (alloc_all || (size_t)overlap >= data_size)) {
        /* Not enough space for required allocations */
        return NULL;
    }

    /* There is enough space for the required data, advance the buf_widx and
     * initialize the struct */
    buf_widx = new_widx;

    struct memory_handle *new_handle =
        (struct memory_handle *)(&buffer[buf_widx]);

    /* Prevent buffering thread from looking at it */
    new_handle->filerem = 0;

    /* Handle can be moved by default */
    new_handle->pinned = 0;

    /* Handle data can be waited for by default */
    new_handle->signaled = 0;

    /* only advance the buffer write index of the size of the struct */
    buf_widx = ringbuf_add(buf_widx, sizeof(struct memory_handle));

    new_handle->id = cur_handle_id;
    /* Wrap signed int is safe and 0 doesn't happen */
    cur_handle_id = (cur_handle_id + 1) & BUF_HANDLE_MASK;
    new_handle->next = NULL;
    num_handles++;

    if (!first_handle)
        /* the new handle is the first one */
        first_handle = new_handle;

    if (cur_handle)
        cur_handle->next = new_handle;

    cur_handle = new_handle;

    return new_handle;
}

/* Delete a given memory handle from the linked list
   and return true for success. Nothing is actually erased from memory. */
static bool rm_handle(const struct memory_handle *h)
{
    if (h == NULL)
        return true;

    if (h == first_handle) {
        first_handle = h->next;
        if (h == cur_handle) {
            /* h was the first and last handle: the buffer is now empty */
            cur_handle = NULL;
            buf_ridx = buf_widx = 0;
        } else {
            /* update buf_ridx to point to the new first handle */
            buf_ridx = (size_t)ringbuf_offset(first_handle);
        }
    } else {
        struct memory_handle *m = first_handle;
        /* Find the previous handle */
        while (m && m->next != h) {
            m = m->next;
        }
        if (m && m->next == h) {
            m->next = h->next;
            if (h == cur_handle) {
                cur_handle = m;
                buf_widx = cur_handle->widx;
            }
        } else {
            /* If we don't find ourselves, this is a seriously incoherent
               state with a corrupted list and severe action is needed! */
            panicf("rm_handle fail: %d", h->id);
            return false;
        }
    }

    /* Invalidate the cache to prevent it from keeping the old location of h */
    if (h == cached_handle)
        cached_handle = NULL;

    num_handles--;
    return true;
}

/* Return a pointer to the memory handle of given ID.
   NULL if the handle wasn't found */
static struct memory_handle *find_handle(int handle_id)
{
    if (handle_id < 0 || !first_handle)
        return NULL;

    /* simple caching because most of the time the requested handle
    will either be the same as the last, or the one after the last */
    if (cached_handle) {
        if (cached_handle->id == handle_id) {
            return cached_handle;
        } else if (cached_handle->next &&
                   (cached_handle->next->id == handle_id)) {
            cached_handle = cached_handle->next;
            return cached_handle;
        }
    }

    struct memory_handle *m = first_handle;
    while (m && m->id != handle_id) {
        m = m->next;
    }
    /* This condition can only be reached with !m or m->id == handle_id */
    if (m)
        cached_handle = m;

    return m;
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
                        size_t data_size, bool can_wrap)
{
    struct memory_handle *dest;
    const struct memory_handle *src;
    size_t final_delta = *delta, size_to_move;
    uintptr_t oldpos, newpos;
    intptr_t overlap, overlap_old;

    if (h == NULL || (src = *h) == NULL)
        return false;

    size_to_move = sizeof(struct memory_handle) + data_size;

    /* Align to four bytes, down */
    final_delta &= ~3;
    if (final_delta < sizeof(struct memory_handle)) {
        /* It's not legal to move less than the size of the struct */
        return false;
    }

    oldpos = ringbuf_offset(src);
    newpos = ringbuf_add(oldpos, final_delta);
    overlap = ringbuf_add_cross(newpos, size_to_move, buffer_len);
    overlap_old = ringbuf_add_cross(oldpos, size_to_move, buffer_len);

    if (overlap > 0) {
        /* Some part of the struct + data would wrap, maybe ok */
        ssize_t correction = 0;
        /* If the overlap lands inside the memory_handle */
        if (!can_wrap) {
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
            /* Align correction to four bytes up */
            correction = (correction + 3) & ~3;
            if (final_delta < correction + sizeof(struct memory_handle)) {
                /* Delta cannot end up less than the size of the struct */
                return false;
            }
            newpos -= correction;
            overlap -= correction;/* Used below to know how to split the data */
            final_delta -= correction;
        }
    }

    dest = (struct memory_handle *)(&buffer[newpos]);

    if (src == first_handle) {
        first_handle = dest;
        buf_ridx = newpos;
    } else {
        struct memory_handle *m = first_handle;
        while (m && m->next != src) {
            m = m->next;
        }
        if (m && m->next == src) {
            m->next = dest;
        } else {
            return false;
        }
    }

    /* Update the cache to prevent it from keeping the old location of h */
    if (src == cached_handle)
        cached_handle = dest;

    /* the cur_handle pointer might need updating */
    if (src == cur_handle)
        cur_handle = dest;

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
        memmove(&buffer[final_delta], buffer, overlap_old);
        if (overlap <= 0)
            size_to_move -= overlap_old;
    }

    if (overlap > 0) {
        /* Move data that now wraps to the beginning */
        size_to_move -= overlap;
        memmove(buffer, SKIPBYTES(src, size_to_move),
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
buffer_is_low   : Returns true if the amount of useful data in the buffer is low
buffer_handle   : Buffer data for a handle
rebuffer_handle : Seek to a nonbuffered part of a handle by rebuffering the data
shrink_handle   : Free buffer space by moving a handle
fill_buffer     : Call buffer_handle for all handles that have data to buffer

These functions are used by the buffering thread to manage buffer space.
*/
static size_t handle_size_available(const struct memory_handle *h)
{
    /* Obtain proper distances from data start */
    size_t rd = ringbuf_sub(h->ridx, h->data);
    size_t wr = ringbuf_sub(h->widx, h->data);

    if (LIKELY(wr > rd))
        return wr - rd;

    return 0; /* ridx is ahead of or equal to widx at this time */
}

static void update_data_counters(struct data_counters *dc)
{
    size_t buffered = 0;
    size_t wasted = 0;
    size_t remaining = 0;
    size_t useful = 0;

    struct memory_handle *m;
    bool is_useful;

    if (dc == NULL)
        dc = &data_counters;

    mutex_lock(&llist_mutex);

    m = find_handle(base_handle_id);
    is_useful = m == NULL;

    m = first_handle;
    while (m) {
        buffered += m->available;
        /* wasted could come out larger than the buffer size if ridx's are
           overlapping data ahead of their handles' buffered data */
        wasted += ringbuf_sub(m->ridx, m->data);
        remaining += m->filerem;

        if (m->id == base_handle_id)
            is_useful = true;

        if (is_useful)
            useful += handle_size_available(m);

        m = m->next;
    }

    mutex_unlock(&llist_mutex);

    dc->buffered = buffered;
    dc->wasted = wasted;
    dc->remaining = remaining;
    dc->useful = useful;
}

static inline bool buffer_is_low(void)
{
    update_data_counters(NULL);
    return data_counters.useful < BUF_WATERMARK / 2;
}

/* Q_BUFFER_HANDLE event and buffer data for the given handle.
   Return whether or not the buffering should continue explicitly.  */
static bool buffer_handle(int handle_id, size_t to_buffer)
{
    logf("buffer_handle(%d, %lu)", handle_id, (unsigned long)to_buffer);
    struct memory_handle *h = find_handle(handle_id);
    bool stop = false;

    if (!h)
        return true;

    logf("  type: %d", (int)h->type);

    if (h->filerem == 0) {
        /* nothing left to buffer */
        return true;
    }

    if (h->fd < 0) { /* file closed, reopen */
        if (*h->path)
            h->fd = open(h->path, O_RDONLY);

        if (h->fd < 0)
        {
            /* could not open the file, truncate it where it is */
            h->filesize -= h->filerem;
            h->filerem = 0;
            return true;
        }

        if (h->offset)
            lseek(h->fd, h->offset, SEEK_SET);
    }

    trigger_cpu_boost();

    if (h->type == TYPE_ID3) {
        if (!get_metadata((struct mp3entry *)(buffer + h->data),
                          h->fd, h->path)) {
            /* metadata parsing failed: clear the buffer. */
            wipe_mp3entry((struct mp3entry *)(buffer + h->data));
        }
        close(h->fd);
        h->fd = -1;
        h->filerem = 0;
        h->available = sizeof(struct mp3entry);
        h->widx = ringbuf_add(h->widx, sizeof(struct mp3entry));
        send_event(BUFFER_EVENT_FINISHED, &handle_id);
        return true;
    }

    while (h->filerem > 0 && !stop)
    {
        /* max amount to copy */
        ssize_t copy_n = MIN( MIN(h->filerem, BUFFERING_DEFAULT_FILECHUNK),
                             buffer_len - h->widx);
        uintptr_t offset = h->next ? ringbuf_offset(h->next) : buf_ridx;
        ssize_t overlap = ringbuf_add_cross(h->widx, copy_n, offset) + 1;

        if (overlap > 0) {
            /* read only up to available space and stop if it would overwrite
               or be on top of the reading position or the next handle */
            stop = true;
            copy_n -= overlap;
        }

        if (copy_n <= 0)
            return false; /* no space for read */

        /* rc is the actual amount read */
        int rc = read(h->fd, &buffer[h->widx], copy_n);

        if (rc <= 0) {
            /* Some kind of filesystem error, maybe recoverable if not codec */
            if (h->type == TYPE_CODEC) {
                logf("Partial codec");
                break;
            }

            logf("File ended %ld bytes early\n", (long)h->filerem);
            h->filesize -= h->filerem;
            h->filerem = 0;
            break;
        }

        /* Advance buffer */
        h->widx = ringbuf_add(h->widx, rc);
        if (h == cur_handle)
            buf_widx = h->widx;
        h->available += rc;
        h->filerem -= rc;

        /* If this is a large file, see if we need to break or give the codec
         * more time */
        if (h->type == TYPE_PACKET_AUDIO &&
            pcmbuf_is_lowdata() && !buffer_is_low()) {
            sleep(1);
        } else {
            yield();
        }

        if (to_buffer == 0) {
            /* Normal buffering - check queue */
            if(!queue_empty(&buffering_queue))
                break;
        } else {
            if (to_buffer <= (size_t)rc)
                break; /* Done */
            to_buffer -= rc;
        }
    }

    if (h->filerem == 0) {
        /* finished buffering the file */
        close(h->fd);
        h->fd = -1;
        send_event(BUFFER_EVENT_FINISHED, &handle_id);
    }

    return !stop;
}

/* Close the specified handle id and free its allocation. */
static bool close_handle(int handle_id)
{
    bool retval = true;
    struct memory_handle *h;

    mutex_lock(&llist_mutex);
    h = find_handle(handle_id);

    /* If the handle is not found, it is closed */
    if (h) {
        if (h->fd >= 0) {
            close(h->fd);
            h->fd = -1;
        }

        /* rm_handle returns true unless the handle somehow persists after
           exit */
        retval = rm_handle(h);
    }

    mutex_unlock(&llist_mutex);
    return retval;
}

/* Free buffer space by moving the handle struct right before the useful
   part of its data buffer or by moving all the data. */
static void shrink_handle(struct memory_handle *h)
{
    if (!h)
        return;

    if (h->type == TYPE_PACKET_AUDIO) {
        /* only move the handle struct */
        /* data is pinned by default - if we start moving packet audio,
           the semantics will determine whether or not data is movable
           but the handle will remain movable in either case */
        size_t delta = ringbuf_sub(h->ridx, h->data);

        /* The value of delta might change for alignment reasons */
        if (!move_handle(&h, &delta, 0, true))
            return;

        h->data = ringbuf_add(h->data, delta);
        h->available -= delta;
        h->offset += delta;
    } else {
        /* metadata handle: we can move all of it */
        if (h->pinned || !h->next || h->filerem != 0)
            return; /* Pinned, last handle or not finished loading */

        uintptr_t handle_distance =
            ringbuf_sub(ringbuf_offset(h->next), h->data);
        size_t delta = handle_distance - h->available;

        /* The value of delta might change for alignment reasons */
        if (!move_handle(&h, &delta, h->available, h->type==TYPE_CODEC))
            return;

        size_t olddata = h->data;
        h->data = ringbuf_add(h->data, delta);
        h->ridx = ringbuf_add(h->ridx, delta);
        h->widx = ringbuf_add(h->widx, delta);

        if (h->type == TYPE_ID3 && h->filesize == sizeof(struct mp3entry)) {
            /* when moving an mp3entry we need to readjust its pointers. */
            adjust_mp3entry((struct mp3entry *)&buffer[h->data],
                            (void *)&buffer[h->data],
                            (const void *)&buffer[olddata]);
        } else if (h->type == TYPE_BITMAP) {
            /* adjust the bitmap's pointer */
            struct bitmap *bmp = (struct bitmap *)&buffer[h->data];
            bmp->data = &buffer[h->data + sizeof(struct bitmap)];
        }
    }
}

/* Fill the buffer by buffering as much data as possible for handles that still
   have data left to buffer
   Return whether or not to continue filling after this */
static bool fill_buffer(void)
{
    logf("fill_buffer()");
    struct memory_handle *m = first_handle;

    shrink_handle(m);

    while (queue_empty(&buffering_queue) && m) {
        if (m->filerem > 0) {
            if (!buffer_handle(m->id, 0)) {
                m = NULL;
                break;
            }
        }
        m = m->next;
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
                      struct bufopen_bitmap_data *data)
{
    int rc;
    struct bitmap *bmp = (struct bitmap *)&buffer[buf_widx];
    struct dim *dim = data->dim;
    struct mp3_albumart *aa = data->embedded_albumart;

    /* get the desired image size */
    bmp->width = dim->width, bmp->height = dim->height;
    /* FIXME: alignment may be needed for the data buffer. */
    bmp->data = &buffer[buf_widx + sizeof(struct bitmap)];
#ifndef HAVE_JPEG
    (void) path;
#endif
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    bmp->maskdata = NULL;
#endif

    int free = (int)MIN(buffer_len - BUF_USED, buffer_len - buf_widx)
                               - sizeof(struct bitmap);

#ifdef HAVE_JPEG
    if (aa != NULL) {
        lseek(fd, aa->pos, SEEK_SET);
        rc = clip_jpeg_fd(fd, aa->size, bmp, free, FORMAT_NATIVE|FORMAT_DITHER|
                         FORMAT_RESIZE|FORMAT_KEEP_ASPECT, NULL);
    }
    else if (strcmp(path + strlen(path) - 4, ".bmp"))
        rc = read_jpeg_fd(fd, bmp, free, FORMAT_NATIVE|FORMAT_DITHER|
                         FORMAT_RESIZE|FORMAT_KEEP_ASPECT, NULL);
    else
#endif
        rc = read_bmp_fd(fd, bmp, free, FORMAT_NATIVE|FORMAT_DITHER|
                         FORMAT_RESIZE|FORMAT_KEEP_ASPECT, NULL);
    return rc + (rc > 0 ? sizeof(struct bitmap) : 0);
}
#endif


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
int bufopen(const char *file, size_t offset, enum data_type type,
            void *user_data)
{
#ifndef HAVE_ALBUMART
    /* currently only used for aa loading */
    (void)user_data;
#endif
    int handle_id = ERR_BUFFER_FULL;

    /* No buffer refs until after the mutex_lock call! */

    if (type == TYPE_ID3) {
        /* ID3 case: allocate space, init the handle and return. */
        mutex_lock(&llist_mutex);

        struct memory_handle *h =
            add_handle(sizeof(struct mp3entry), false, true);

        if (h) {
            handle_id = h->id;
            h->fd = -1;
            h->filesize = sizeof(struct mp3entry);
            h->offset = 0;
            h->data = buf_widx;
            h->ridx = buf_widx;
            h->widx = buf_widx;
            h->available = 0;
            h->type = type;
            strlcpy(h->path, file, MAX_PATH);

            buf_widx = ringbuf_add(buf_widx, sizeof(struct mp3entry));

            h->filerem = sizeof(struct mp3entry);

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
    /* loading code from memory is not supported in application builds */
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
        /* if albumart is embedded, the complete file is not buffered,
         * but only the jpeg part; filesize() would be wrong */
        struct bufopen_bitmap_data *aa = (struct bufopen_bitmap_data*)user_data;
        if (aa->embedded_albumart)
            size = aa->embedded_albumart->size;
    }
#endif
    if (size == 0)
        size = filesize(fd);
    bool can_wrap = type==TYPE_PACKET_AUDIO || type==TYPE_CODEC;

    size_t adjusted_offset = offset;
    if (adjusted_offset > size)
        adjusted_offset = 0;

    /* Reserve extra space because alignment can move data forward */
    size_t padded_size = STORAGE_PAD(size-adjusted_offset);

    mutex_lock(&llist_mutex);

    struct memory_handle *h = add_handle(padded_size, can_wrap, false);
    if (!h) {
        DEBUGF("%s(): failed to add handle\n", __func__);
        mutex_unlock(&llist_mutex);
        close(fd);
        return ERR_BUFFER_FULL;
    }

    handle_id = h->id;
    strlcpy(h->path, file, MAX_PATH);
    h->offset = adjusted_offset;

#ifdef STORAGE_WANTS_ALIGN
    /* Don't bother to storage align bitmaps because they are not
     * loaded directly into the buffer.
     */
    if (type != TYPE_BITMAP) {
        /* Align to desired storage alignment */
        size_t alignment_pad = STORAGE_OVERLAP(adjusted_offset -
                                               (size_t)(&buffer[buf_widx]));
        buf_widx = ringbuf_add(buf_widx, alignment_pad);
    }
#endif /* STORAGE_WANTS_ALIGN */

    h->fd   = -1;
    h->data = buf_widx;
    h->ridx = buf_widx;
    h->widx = buf_widx;
    h->available = 0;
    h->type = type;

#ifdef HAVE_ALBUMART
    if (type == TYPE_BITMAP) {
        /* Bitmap file: we load the data instead of the file */
        int rc;
        rc = load_image(fd, file, (struct bufopen_bitmap_data*)user_data);
        if (rc <= 0) {
            rm_handle(h);
            handle_id = ERR_FILE_ERROR;
        } else {
            h->filesize = rc;
            h->available = rc;
            buf_widx = ringbuf_add(buf_widx, rc);
            h->widx = buf_widx;
        }
    }
    else
#endif
    {
        if (type == TYPE_CUESHEET)
            h->fd = fd;

        h->filesize = size;
        h->available = 0;
        h->widx = buf_widx;
        h->filerem = size - adjusted_offset;
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
    int handle_id;

    if (type == TYPE_UNKNOWN)
        return ERR_UNSUPPORTED_TYPE;

    handle_id = ERR_BUFFER_FULL;

    mutex_lock(&llist_mutex);

    struct memory_handle *h = add_handle(size, false, true);

    if (h) {
        handle_id = h->id;

        if (src) {
            if (type == TYPE_ID3 && size == sizeof(struct mp3entry)) {
                /* specially take care of struct mp3entry */
                copy_mp3entry((struct mp3entry *)&buffer[buf_widx],
                              (const struct mp3entry *)src);
            } else {
                memcpy(&buffer[buf_widx], src, size);
            }
        }
        
        h->fd = -1;
        *h->path = 0;
        h->filesize = size;
        h->offset = 0;
        h->ridx = buf_widx;
        h->data = buf_widx;
        buf_widx = ringbuf_add(buf_widx, size);
        h->widx = buf_widx;
        h->available = size;
        h->type = type;
    }

    mutex_unlock(&llist_mutex);

    logf("bufalloc: new hdl %d", handle_id);
    return handle_id;
}

/* Close the handle. Return true for success and false for failure */
bool bufclose(int handle_id)
{
    logf("bufclose(%d)", handle_id);
#if 0
    /* Don't interrupt the buffering thread if the handle is already
       stale */
    if (!find_handle(handle_id)) {
        logf("  handle already closed");
        return true;
    }
#endif
    LOGFQUEUE("buffering >| Q_CLOSE_HANDLE %d", handle_id);
    return queue_send(&buffering_queue, Q_CLOSE_HANDLE, handle_id);
}

/* Backend to bufseek and bufadvance. Call only in response to
   Q_REBUFFER_HANDLE! */
static void rebuffer_handle(int handle_id, size_t newpos)
{
    struct memory_handle *h = find_handle(handle_id);

    if (!h) {
        queue_reply(&buffering_queue, ERR_HANDLE_NOT_FOUND);
        return;
    }

    /* When seeking foward off of the buffer, if it is a short seek attempt to
       avoid rebuffering the whole track, just read enough to satisfy */
    if (newpos > h->offset &&
        newpos - h->offset < BUFFERING_DEFAULT_FILECHUNK) {

        size_t amount = newpos - h->offset;
        h->ridx = ringbuf_add(h->data, amount);

        if (buffer_handle(handle_id, amount + 1)) {
            size_t rd = ringbuf_sub(h->ridx, h->data); 
            size_t wr = ringbuf_sub(h->widx, h->data);
            if (wr >= rd) {
                /* It really did succeed */
                queue_reply(&buffering_queue, 0);
                buffer_handle(handle_id, 0); /* Ok, try the rest */
                return;
            }
        }
        /* Data collision or other file error - must reset */

        if (newpos > h->filesize)
            newpos = h->filesize; /* file truncation happened above */
    }

    /* Reset the handle to its new position */
    h->offset = newpos;

    size_t next = h->next ? ringbuf_offset(h->next) : buf_ridx;

#ifdef STORAGE_WANTS_ALIGN
    /* Strip alignment padding then redo */
    size_t new_index = ringbuf_add(ringbuf_offset(h), sizeof (*h));

    /* Align to desired storage alignment if space permits - handle could
       have been shrunken too close to the following one after a previous
       rebuffer. */
    size_t alignment_pad =
        STORAGE_OVERLAP(h->offset - (size_t)(&buffer[new_index]));

    if (ringbuf_add_cross(new_index, alignment_pad, next) >= 0)
        alignment_pad = 0; /* Forego storage alignment this time */

    new_index = ringbuf_add(new_index, alignment_pad);
#else
    /* Just clear the data buffer */
    size_t new_index = h->data;
#endif /* STORAGE_WANTS_ALIGN */

    h->ridx = h->widx = h->data = new_index;

    if (h == cur_handle)
        buf_widx = new_index;

    h->available = 0;
    h->filerem = h->filesize - h->offset;

    if (h->fd >= 0)
        lseek(h->fd, h->offset, SEEK_SET);

    if (h->next && ringbuf_sub(next, h->data) <= h->filesize - newpos) {
        /* There isn't enough space to rebuffer all of the track from its new
           offset, so we ask the user to free some */
        DEBUGF("%s(): space is needed\n", __func__);
        int hid = handle_id;
        send_event(BUFFER_EVENT_REBUFFER, &hid);
    }

    /* Now we do the rebuffer */
    queue_reply(&buffering_queue, 0);
    buffer_handle(handle_id, 0);
}

/* Backend to bufseek and bufadvance */
static int seek_handle(struct memory_handle *h, size_t newpos)
{
    if (newpos > h->filesize) {
        /* access beyond the end of the file */
        return ERR_INVALID_VALUE;
    }
    else if ((newpos < h->offset || h->offset + h->available <= newpos) &&
             (newpos < h->filesize || h->filerem > 0)) {
        /* access before or after buffered data and not to end of file or file
           is not buffered to the end-- a rebuffer is needed. */
        struct buf_message_data parm = { h->id, newpos };
        return queue_send(&buffering_queue, Q_REBUFFER_HANDLE,
                          (intptr_t)&parm);
    }
    else {
        h->ridx = ringbuf_add(h->data, newpos - h->offset);
    }

    return 0;
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

    return seek_handle(h, newpos);
}

/* Advance the reading index in a handle (relatively to its current position).
   Return 0 for success and for failure:
     ERR_HANDLE_NOT_FOUND if the handle wasn't found
     ERR_INVALID_VALUE if the new requested position was beyond the end of
     the file
 */
int bufadvance(int handle_id, off_t offset)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    size_t newpos = h->offset + ringbuf_sub(h->ridx, h->data) + offset;
    return seek_handle(h, newpos);
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
    return h->offset + ringbuf_sub(h->ridx, h->data);
}

/* Used by bufread and bufgetdata to prepare the buffer and retrieve the
 * actual amount of data available for reading.  This function explicitly
 * does not check the validity of the input handle.  It does do range checks
 * on size and returns a valid (and explicit) amount of data for reading */
static struct memory_handle *prep_bufdata(int handle_id, size_t *size,
                                          bool guardbuf_limit)
{
    struct memory_handle *h = find_handle(handle_id);
    size_t realsize;

    if (!h)
        return NULL;

    size_t avail = handle_size_available(h);

    if (avail == 0 && h->filerem == 0) {
        /* File is finished reading */
        *size = 0;
        return h;
    }

    realsize = *size;

    if (realsize == 0 || realsize > avail + h->filerem)
        realsize = avail + h->filerem;

    if (guardbuf_limit && h->type == TYPE_PACKET_AUDIO
            && realsize > GUARD_BUFSIZE) {
        logf("data request > guardbuf");
        /* If more than the size of the guardbuf is requested and this is a
         * bufgetdata, limit to guard_bufsize over the end of the buffer */
        realsize = MIN(realsize, buffer_len - h->ridx + GUARD_BUFSIZE);
        /* this ensures *size <= buffer_len - h->ridx + GUARD_BUFSIZE */
    }

    if (h->filerem > 0 && avail < realsize) {
        /* Data isn't ready. Request buffering */
        LOGFQUEUE("buffering >| Q_START_FILL %d",handle_id);
        queue_send(&buffering_queue, Q_START_FILL, handle_id);
        /* Wait for the data to be ready */
        do
        {
            sleep(0);
            /* it is not safe for a non-buffering thread to sleep while
             * holding a handle */
            h = find_handle(handle_id);
            if (!h || h->signaled != 0)
                return NULL;
            avail = handle_size_available(h);
        }
        while (h->filerem > 0 && avail < realsize);
    }

    *size = MIN(realsize, avail);
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
    const struct memory_handle *h;
    size_t adjusted_size = size;

    h = prep_bufdata(handle_id, &adjusted_size, false);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->ridx + adjusted_size > buffer_len) {
        /* the data wraps around the end of the buffer */
        size_t read = buffer_len - h->ridx;
        memcpy(dest, &buffer[h->ridx], read);
        memcpy(dest+read, buffer, adjusted_size - read);
    } else {
        memcpy(dest, &buffer[h->ridx], adjusted_size);
    }

    return adjusted_size;
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
    const struct memory_handle *h;
    size_t adjusted_size = size;

    h = prep_bufdata(handle_id, &adjusted_size, true);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->ridx + adjusted_size > buffer_len) {
        /* the data wraps around the end of the buffer :
           use the guard buffer to provide the requested amount of data. */
        size_t copy_n = h->ridx + adjusted_size - buffer_len;
        /* prep_bufdata ensures
           adjusted_size <= buffer_len - h->ridx + GUARD_BUFSIZE,
           so copy_n <= GUARD_BUFSIZE */
        memcpy(guard_buffer, (const unsigned char *)buffer, copy_n);
    }

    if (data)
        *data = &buffer[h->ridx];

    return adjusted_size;
}

ssize_t bufgettail(int handle_id, size_t size, void **data)
{
    size_t tidx;

    const struct memory_handle *h;

    h = find_handle(handle_id);

    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->filerem)
        return ERR_HANDLE_NOT_DONE;

    /* We don't support tail requests of > guardbuf_size, for simplicity */
    if (size > GUARD_BUFSIZE)
        return ERR_INVALID_VALUE;

    tidx = ringbuf_sub(h->widx, size);

    if (tidx + size > buffer_len) {
        size_t copy_n = tidx + size - buffer_len;
        memcpy(guard_buffer, (const unsigned char *)buffer, copy_n);
    }

    *data = &buffer[tidx];
    return size;
}

ssize_t bufcuttail(int handle_id, size_t size)
{
    struct memory_handle *h;
    size_t adjusted_size = size;

    h = find_handle(handle_id);

    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->filerem)
        return ERR_HANDLE_NOT_DONE;

    if (h->available < adjusted_size)
        adjusted_size = h->available;

    h->available -= adjusted_size;
    h->filesize -= adjusted_size;
    h->widx = ringbuf_sub(h->widx, adjusted_size);
    if (h == cur_handle)
        buf_widx = h->widx;

    return adjusted_size;
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

ssize_t buf_handle_offset(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->offset;
}

void buf_set_base_handle(int handle_id)
{
    mutex_lock(&llist_mutex);
    base_handle_id = handle_id;
    mutex_unlock(&llist_mutex);
}

enum data_type buf_handle_data_type(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return TYPE_UNKNOWN;
    return h->type;
}

ssize_t buf_handle_remaining(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->filerem;
}

bool buf_is_handle(int handle_id)
{
    return find_handle(handle_id) != NULL;
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

/* Return the amount of buffer space used */
size_t buf_used(void)
{
    return BUF_USED;
}

void buf_set_watermark(size_t bytes)
{
    conf_watermark = bytes;
}

size_t buf_get_watermark(void)
{
    return BUF_WATERMARK;
}

#ifdef HAVE_IO_PRIORITY
void buf_back_off_storage(bool back_off)
{
    int priority = back_off ?
        IO_PRIORITY_BACKGROUND : IO_PRIORITY_IMMEDIATE;
    thread_set_io_priority(buffering_thread_id, priority);
}
#endif

/** -- buffer thread helpers -- **/
static void shrink_buffer_inner(struct memory_handle *h)
{
    if (h == NULL)
        return;

    shrink_buffer_inner(h->next);

    shrink_handle(h);
}

static void shrink_buffer(void)
{
    logf("shrink_buffer()");
    shrink_buffer_inner(first_handle);
}

static void NORETURN_ATTR buffering_thread(void)
{
    bool filling = false;
    struct queue_event ev;
    struct buf_message_data *parm;

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
                parm = (struct buf_message_data *)ev.data;
                LOGFQUEUE("buffering < Q_REBUFFER_HANDLE %d %ld",
                          parm->handle_id, parm->data);
                rebuffer_handle(parm->handle_id, parm->data);
                break;

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
#if 0
        /* TODO: This needs to be fixed to use the idle callback, disable it
         * for simplicity until its done right */
#if MEMORYSIZE > 8
        /* If the disk is spinning, take advantage by filling the buffer */
        else if (storage_disk_is_active()) {
            if (num_handles > 0 && data_counters.useful <= high_watermark)
                send_event(BUFFER_EVENT_BUFFER_LOW, 0);

            if (data_counters.remaining > 0 && BUF_USED <= high_watermark) {
                /* This is a new fill, shrink the buffer up first */
                if (!filling)
                    shrink_buffer();
                filling = fill_buffer();
                update_data_counters(NULL);
            }
        }
#endif
#endif

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

void buffering_init(void)
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
    while (num_handles != 0)
        bufclose(first_handle->id);

    buffer = buf;
    buffer_len = buflen;
    guard_buffer = buf + buflen;

    buf_widx = 0;
    buf_ridx = 0;

    first_handle = NULL;
    cur_handle = NULL;
    cached_handle = NULL;
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
    update_data_counters(&dc);
    dbgdata->num_handles = num_handles;
    dbgdata->data_rem = dc.remaining;
    dbgdata->wasted_space = dc.wasted;
    dbgdata->buffered_data = dc.buffered;
    dbgdata->useful_data = dc.useful;
    dbgdata->watermark = BUF_WATERMARK;
}
