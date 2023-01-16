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

/*
 * Malloc backed buflib. This is intended for debugging rather than for
 * serious use - the buffer passed to the context is wasted, and memory
 * is acquired from malloc() instead. The main point is to make ASAN more
 * effective by isolating buflib allocations from each other.
 *
 * Currently this is a bare-minimum implementation, it doesn't even run
 * buflib callbacks since it never moves anything. It could later be
 * extended with stress-testing options, for example by randomly moving
 * allocations around.
 */

#include "buflib.h"
#include "panic.h"
#include <stdlib.h>

static struct buflib_malloc_handle *get_free_handle(struct buflib_context *ctx)
{
    struct buflib_malloc_handle *h;
    for (size_t i = 0; i < ctx->num_allocs; ++i)
    {
        h = &ctx->allocs[i];
        if (h->size == 0)
            return h;
    }

    ctx->num_allocs++;
    ctx->allocs = realloc(ctx->allocs, ctx->num_allocs * sizeof(*ctx->allocs));
    if (!ctx->allocs)
        panicf("buflib %p handle OOM", ctx);

    h = &ctx->allocs[ctx->num_allocs - 1];
    h->size = 0;
    return h;
}

static int get_handle_num(struct buflib_context *ctx,
                          struct buflib_malloc_handle *handle)
{
    return (handle - ctx->allocs) + 1;
}

static struct buflib_malloc_handle *get_handle(struct buflib_context *ctx,
                                               int handle)
{
    return &ctx->allocs[handle - 1];
}

struct buflib_callbacks buflib_ops_locked = {
    .move_callback = NULL,
    .shrink_callback = NULL,
    .sync_callback = NULL,
};

void buflib_init(struct buflib_context *ctx, void *buf, size_t size)
{
    ctx->allocs = NULL;
    ctx->num_allocs = 0;
    ctx->buf = buf;
    ctx->bufsize = size;
}

size_t buflib_available(struct buflib_context *ctx)
{
    return ctx->bufsize;
}

size_t buflib_allocatable(struct buflib_context *ctx)
{
    return ctx->bufsize;
}

bool buflib_context_relocate(struct buflib_context *ctx, void *buf)
{
    ctx->buf = buf;
    return true;
}

int buflib_alloc(struct buflib_context *ctx, size_t size)
{
    return buflib_alloc_ex(ctx, size, NULL);
}

int buflib_alloc_ex(struct buflib_context *ctx, size_t size,
                    struct buflib_callbacks *ops)
{
    struct buflib_malloc_handle *handle = get_free_handle(ctx);

    handle->data = malloc(size);
    handle->user = handle->data;
    handle->size = size;
    handle->pin_count = 0;
    handle->ops = ops;

    if (!handle->data)
        panicf("buflib %p data OOM", ctx);

    return get_handle_num(ctx, handle);
}

int buflib_alloc_maximum(struct buflib_context* ctx,
                         size_t *size, struct buflib_callbacks *ops)
{
    *size = ctx->bufsize;

    return buflib_alloc_ex(ctx, *size, ops);
}

bool buflib_shrink(struct buflib_context *ctx, int handle,
                   void *newstart, size_t new_size)
{
    struct buflib_malloc_handle *h = get_handle(ctx, handle);
    if (newstart < h->user || new_size > h->size - (newstart - h->user))
        return false;

    /* XXX: this might be allowed, but what would be the point... */
    if (new_size == 0)
    {
        panicf("weird shrink");
        return false;
    }

    /* due to buflib semantics we must not realloc */
    h->user = newstart;
    h->size = new_size;
    return true;
}

void buflib_pin(struct buflib_context *ctx, int handle)
{
    struct buflib_malloc_handle *h = get_handle(ctx, handle);

    h->pin_count++;
}

void buflib_unpin(struct buflib_context *ctx, int handle)
{
    struct buflib_malloc_handle *h = get_handle(ctx, handle);

    h->pin_count--;
}

unsigned buflib_pin_count(struct buflib_context *ctx, int handle)
{
    struct buflib_malloc_handle *h = get_handle(ctx, handle);

    return h->pin_count;
}

void _buflib_malloc_put_data_pinned(struct buflib_context *ctx, void *data)
{
    for (size_t i = 0; i < ctx->num_allocs; ++i)
    {
        if (ctx->allocs[i].user == data)
        {
            ctx->allocs[i].pin_count--;
            break;
        }
    }
}

int buflib_free(struct buflib_context *ctx, int handle)
{
    if (handle <= 0)
        return 0;

    struct buflib_malloc_handle *h = get_handle(ctx, handle);

    free(h->data);
    h->size = 0;

    return 0;
}

#ifdef BUFLIB_DEBUG_GET_DATA
void *buflib_get_data(struct buflib_context *ctx, int handle)
{
    /* kind of silly since it's better for ASAN to catch this but... */
    if (handle <= 0 || handle > ctx->num_allocs)
        panicf("buflib %p: invalid handle %d", ctx, handle);

    struct buflib_malloc_handle *h = get_handle(ctx, handle);
    if (h->user == NULL)
        panicf("buflib %p: handle %d use after free", ctx, handle);

    return h->user;
}
#endif

void *buflib_buffer_out(struct buflib_context *ctx, size_t *size)
{
    if (*size == 0)
        *size = ctx->bufsize;

    void *ret = ctx->buf;

    ctx->buf += *size;
    return ret;
}

void buflib_buffer_in(struct buflib_context *ctx, int size)
{
    ctx->buf -= size;
}

#ifdef BUFLIB_DEBUG_PRINT
int buflib_get_num_blocks(struct buflib_context *ctx)
{
    return ctx->num_allocs;
}

bool buflib_print_block_at(struct buflib_context *ctx, int block_num,
                           char *buf, size_t bufsize)
{
    if (block_num >= ctx->num_allocs)
    {
        if (bufsize > 0)
            *buf = '\0';
        return false;
    }

    struct buflib_malloc_handle *handle = &ctx->allocs[block_num];
    if (handle->data)
    {
        snprintf(buf, bufsize, "%03d addr:%8p length:%zu",
                 block_num, handle->data, handle->size);
    }
    else
    {
        snprintf(buf, bufsize, "%03d (unallocated)", block_num);
    }

    return true;
}
#endif

#ifdef BUFLIB_DEBUG_CHECK_VALID
void buflib_check_valid(struct buflib_context *ctx)
{
    (void)ctx;
}
#endif
