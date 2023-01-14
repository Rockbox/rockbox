/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2023 Aidan MacDonald
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
#ifndef _BUFLIB_MALLOC_H_
#define _BUFLIB_MALLOC_H_

#ifndef _BUFLIB_H_
# error "include buflib.h instead"
#endif

struct buflib_malloc_handle
{
    void *data;
    void *user;
    size_t size;
    unsigned int pin_count;
    struct buflib_callbacks *ops;
};

struct buflib_context
{
    struct buflib_malloc_handle *allocs;
    size_t num_allocs;

    void *buf;
    size_t bufsize;
};

#ifndef BUFLIB_DEBUG_GET_DATA
static inline void *buflib_get_data(struct buflib_context *ctx, int handle)
{
    return ctx->allocs[handle - 1].user;
}
#endif

static inline void *buflib_get_data_pinned(struct buflib_context *ctx, int handle)
{
    buflib_pin(ctx, handle);
    return buflib_get_data(ctx, handle);
}

void _buflib_malloc_put_data_pinned(struct buflib_context *ctx, void *data);
static inline void buflib_put_data_pinned(struct buflib_context *ctx, void *data)
{
    _buflib_malloc_put_data_pinned(ctx, data);
}

#endif /* _BUFLIB_MALLOC_H_ */
