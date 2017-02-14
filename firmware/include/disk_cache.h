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
#ifndef DISK_CACHE_H
#define DISK_CACHE_H

#include "storage.h"
#include "searchtree.h"
#include "mv.h"

struct bpb;

struct iobuf_handle
{
    void *h; /* buffer reference for fast path lookup and release */
};

static inline void iobuf_init(struct iobuf_handle *iobp)
{
    iobp->h = NULL; /* just set as invalid */
}

typedef int (*iocache_wb_cb_t)(struct bpb *bpb,
                               unsigned long start,
                               int count,
                               const void *buf);

struct iocache_bpb
{
    struct searchtree index;   /* buffer index for searching */
    struct ll_head    wbq;     /* buffers awaiting writeback */
    iocache_wb_cb_t   wbcb;
};

void * iobuf_cache(struct bpb *bpb,
                   unsigned long num,
                   struct iobuf_handle *iobp,
                   unsigned int priority,
                   bool fill);

void iobuf_release(struct iobuf_handle *iobp);

void iobuf_release_dirty(struct iobuf_handle *iobp);

void iobuf_release_dirty_cb(struct iobuf_handle *iobp);

long iocache_readwrite(struct bpb *bpb,
                       unsigned long start,
                       int count,
                       void *buf,
                       bool write);

#define iocache_writeback(bpb, start, count, buf) \
    storage_write_sectors(IF_MD((bpb)->drive,) (start), (count), (buf))

void iocache_mount_volume(struct bpb *bpb, iocache_wb_cb_t wbcb);

void iocache_flush_volume(struct bpb *bpb);

void iocache_discard_volume(struct bpb *bpb);


/** These synchronize and can be called by anyone **/

/* expropriate a buffer from the cache of DC_CACHE_BUFSIZE bytes */
void * iocache_get_buffer(void);

/* return buffer to the cache by buffer */
void iocache_release_buffer(void *buf);

/** Boot init **/

/* one-time init at startup */
void iocache_init(void) INIT_ATTR;

#endif /* DISK_CACHE_H */
