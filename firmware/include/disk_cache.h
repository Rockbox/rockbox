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

/* This needs enough for all file handles to have a buffer in the worst case
 * plus at least one reserved exclusively for the cache client and a couple
 * for other file system code. The buffers are put to use by the cache if not
 * taken for another purpose (meaning nothing is wasted sitting fallow).
 *
 * One map per volume is maintained in order to avoid collisions between
 * volumes that would slow cache probing. DC_MAP_NUM_ENTRIES is the number
 * for each map per volume. The buffers themselves are shared.
 */
#if MEMORYSIZE < 8
#define DC_NUM_ENTRIES      32
#define DC_MAP_NUM_ENTRIES  128
#elif MEMORYSIZE <= 32
#define DC_NUM_ENTRIES      48
#define DC_MAP_NUM_ENTRIES  128
#else /* MEMORYSIZE > 32 */
#define DC_NUM_ENTRIES      64
#define DC_MAP_NUM_ENTRIES  256
#endif /* MEMORYSIZE */

/* this _could_ be larger than a sector if that would ever be useful */
#define DC_CACHE_BUFSIZE    SECTOR_SIZE

#include "mutex.h"
#include "mv.h"

static inline void dc_lock_cache(void)
{
    extern struct mutex disk_cache_mutex;
    mutex_lock(&disk_cache_mutex);
}

static inline void dc_unlock_cache(void)
{
    extern struct mutex disk_cache_mutex;
    mutex_unlock(&disk_cache_mutex);
}

void * dc_cache_probe(IF_MV(int volume,) unsigned long secnum,
                      unsigned int *flags);
void dc_dirty_buf(void *buf);
void dc_discard_buf(void *buf);
void dc_commit_all(IF_MV_NONVOID(int volume));
void dc_discard_all(IF_MV_NONVOID(int volume));

void dc_init(void) INIT_ATTR;

/* in addition to filling, writeback is implemented by the client */
extern void dc_writeback_callback(IF_MV(int volume, ) unsigned long sector,
                                  void *buf);


/** These synchronize and can be called by anyone **/

/* expropriate a buffer from the cache of DC_CACHE_BUFSIZE bytes */
void * dc_get_buffer(void);
/* return buffer to the cache by buffer */
void dc_release_buffer(void *buf);

#endif /* DISK_CACHE_H */
