/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* This is a memory allocator designed to provide reasonable management of free
* space and fast access to allocated data. More than one allocator can be used
* at a time by initializing multiple contexts.
*
* Copyright (C) 2009 Andrew Mahone
* Copyright (C) 2011 Thomas Martitz
* 
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

#include <stdlib.h> /* for abs() */
#include <stdio.h> /* for snprintf() */
#include "buflib.h"
#include "string-extra.h" /* strlcpy() */
#include "debug.h"
#include "system.h" /* for ALIGN_*() */

/* This implements the buflib API with a malloc() backend. This avoids
 * move and shrink operations on platforms where malloc() is available.
 */

#define B_ALIGN_DOWN(x) \
    ALIGN_DOWN(x, sizeof(union buflib_data))

#define B_ALIGN_UP(x) \
    ALIGN_UP(x, sizeof(union buflib_data))

#ifdef DEBUG
    #include <stdio.h>
    #include "panic.h"
    #define BDEBUGF DEBUGF
#else
    #define BDEBUGF(...) do { } while(0)
#endif

struct buflib_buffer_inout_data {
    char *ptr;
    size_t len;
};

#define HANDLE_TABLE_QUANTUM 64

static
union buflib_data* buflib_data_new(int n)
{
    union buflib_data* ret = calloc(n, sizeof(union buflib_data));
    if (!ret) panicf("buflib: OOM\n");
    return ret;
}

/* Initialize buffer manager */
void
buflib_init(struct buflib_context *ctx, void *buf, size_t size)
{
    (void)buf;
    memset(ctx, 0, sizeof(*ctx));

    /* use buf_start to remember the remaining size */
    ctx->buf_start = (union buflib_data*)((uintptr_t)size);

    /* like in normal buflib, the handle table grows backwards
     * this allows a) to re-use some code and b) the handle-to-pointer
     * conversion (which is a static inline) is identical */
    ctx->alloc_end = buflib_data_new(HANDLE_TABLE_QUANTUM);
    ctx->handle_table = ctx->alloc_end + HANDLE_TABLE_QUANTUM;
    ctx->last_handle  = ctx->alloc_end + HANDLE_TABLE_QUANTUM;
    ctx->first_free_handle = ctx->last_handle - 1;

    /* always ignored */
    ctx->compact = true;

    /* setup a special handle to support buflib_buffer_in/_out */
    buflib_alloc(ctx, sizeof(struct buflib_buffer_inout_data));

    BDEBUGF("buflib initialized with %lu.%2lu kiB",
            (unsigned long)size / 1024, ((unsigned long)size%1000)/10);
}

/* unit is entries, not bytes */
static size_t handle_table_size(struct buflib_context *ctx)
{
    return ctx->handle_table - ctx->alloc_end;
}

static
void handle_table_grow(struct buflib_context *ctx)
{
    size_t old_size = handle_table_size(ctx);
    size_t new_size = old_size + HANDLE_TABLE_QUANTUM;

    union buflib_data *old_table = ctx->alloc_end, *free_handle;

    ctx->alloc_end = buflib_data_new(new_size);
    /* copy old table over to the new one. as it grows backwards the
     * destination is the upper part */
    memcpy(ctx->alloc_end + HANDLE_TABLE_QUANTUM, old_table, old_size*sizeof(union buflib_data));
    /* as this is only called when the old table is full it's ok to set
     * the pointers to the new place */
    ctx->handle_table = ctx->alloc_end + new_size;
    ctx->last_handle  = ctx->handle_table - old_size;
    for(free_handle = ctx->handle_table-1; free_handle >= ctx->alloc_end; free_handle--)
    {
        if (!free_handle->alloc)
        {
            ctx->first_free_handle = free_handle;
            break;
        }
    }
    free(old_table);
}

/* Allocate a new handle, returning 0 on failure */
static
union buflib_data* handle_alloc(struct buflib_context *ctx)
{
    union buflib_data *handle;
    /* first_free_handle is a lower bound on free handles, work through the
     * table from there until a handle containing NULL is found, or the end
     * of the table is reached.
     */
    for (handle = ctx->first_free_handle; handle >= ctx->last_handle; handle--)
        if (!handle->alloc)
            break;
    /* If the search went past the end of the table, it means we need to extend
     * the table to get a new handle.
     */
    if (handle < ctx->last_handle)
    {
        if (handle >= ctx->alloc_end)
            ctx->last_handle--;
        else
        {   /* grow and restart */
            handle_table_grow(ctx);
            return handle_alloc(ctx);
        }
    }
    handle->val = -1;
    return handle;
}

/* Free one handle, shrinking the handle table if it's the last one */
static inline
void handle_free(struct buflib_context *ctx, union buflib_data *handle)
{
    handle->alloc = 0;
    /* Update free handle lower bound if this handle has a lower index than the
     * old one.
     */
    if (handle > ctx->first_free_handle)
        ctx->first_free_handle = handle;
    if (handle == ctx->last_handle)
        ctx->last_handle++;
    else
        ctx->compact = false;
}

/* Get the start block of an allocation */
static union buflib_data* handle_to_block(struct buflib_context* ctx, int handle)
{
    union buflib_data* name_field =
                (union buflib_data*)buflib_get_name(ctx, handle);

    return name_field - 3;
}


/* to support buflib_buffer_out/in APIs it is required to return a buffer
 * that is stable with subsequent calls (i.e. subsequent calls only shift
 * the returned buffer pointer). For this we allocate a buffer in the first
 * call that is re-used in subsequent calls. The size is limited to the buffer
 * size the context was initialized with, and can be retrieved with *size == 0.
 *
 * In order to reuse the buffer the pointer and buffer position have to be
 * remembered. This is stored in a "normal" buflib allocation, however its
 * handle is fixed at 1.
 */

/* Shift buffered items up by size bytes, or as many as possible if size == 0.
 * Set size to the number of bytes freed.
 */
void*
buflib_buffer_out(struct buflib_context *ctx, size_t *size)
{
    size_t siz = *size, buf_pos;
    int handle = 1; /* special handle setup in buflib_init() */
    struct buflib_buffer_inout_data *data = buflib_get_data(ctx, handle);
    if (!data->ptr)
    {   /* first call: allocate buffer */
        data->len = (uintptr_t)ctx->buf_start;
        data->ptr = malloc(data->len);
    }
    if (!data->len)
        return NULL;

    /* calculate the buffer postion before substracting siz */
    buf_pos = (uintptr_t)ctx->buf_start - data->len;
    siz = siz ? MIN(siz, data->len) : data->len;
    data->len -= siz;
    *size = siz;

    return &data->ptr[buf_pos];
}

/* Shift buffered items down by size bytes */
void
buflib_buffer_in(struct buflib_context *ctx, int size)
{
    int handle = 1; /* special handle setup in buflib_init() */
    struct buflib_buffer_inout_data *data = buflib_get_data(ctx, handle);
    
    data->len += size;
    if (data->len >= (uintptr_t)ctx->buf_start)
    {
        /* all of the buffer was returned back, time to free */
        free(data->ptr);
        data->ptr = NULL;
        data->len = 0;
    }
}

/* Allocate a buffer of size bytes, returning a handle for it */
int
buflib_alloc(struct buflib_context *ctx, size_t size)
{
    return buflib_alloc_ex(ctx, size, "<anonymous>", NULL);
}

/* Allocate a buffer of size bytes, returning a handle for it.
 *
 * The additional name parameter gives the allocation a human-readable name,
 * the ops parameter points to caller-implemented callbacks for moving and
 * shrinking. NULL for default callbacks (which do nothing but don't
 * prevent moving or shrinking)
 */

int
buflib_alloc_ex(struct buflib_context *ctx, size_t size, const char *name,
                struct buflib_callbacks *ops)
{
    (void)ops;
    union buflib_data *handle, *block;
    size_t name_len = name ? B_ALIGN_UP(strlen(name)+1) : 0;
    size += name_len;
    size = B_ALIGN_UP(size) / sizeof(union buflib_data);
   /* add 4 objects for alloc len, pointer to handle table entry,
    * name length and pointer to callbacks */
    size += 4;

    handle = handle_alloc(ctx);
    if (!handle)
        return -1;

    block = buflib_data_new(size);
    /* Set up the allocated block, by marking the size allocated, and storing
     * a pointer to the handle.
     */
    union buflib_data *name_len_slot;
    block->val = size;
    block[1].handle = handle;
    block[2].ops = ops;
    strcpy(block[3].name, name);
    name_len_slot = (union buflib_data*)B_ALIGN_UP(block[3].name + name_len);
    name_len_slot->val = 1 + name_len/sizeof(union buflib_data);
    handle->alloc = (char*)(name_len_slot + 1);

    /* Return the handle index as a positive integer. */
    return ctx->handle_table - handle;
}

/* Free the buffer associated with handle_num. */
int
buflib_free(struct buflib_context *ctx, int handle_num)
{
    union buflib_data *handle = ctx->handle_table - handle_num,
                      *freed_block = handle_to_block(ctx, handle_num);
    handle_free(ctx, handle);
    free(freed_block);

    return 0; /* unconditionally */
}

/* Return the maximum allocatable memory in bytes */
size_t
buflib_available(struct buflib_context* ctx)
{
    return (uintptr_t)ctx->buf_start;
}

size_t
buflib_allocatable(struct buflib_context* ctx)
{
    return (uintptr_t)ctx->buf_start;
}

/*
 * Allocate all available (as returned by buflib_available()) memory and return
 * a handle to it
 */
int
buflib_alloc_maximum(struct buflib_context* ctx, const char* name, size_t *size, struct buflib_callbacks *ops)
{
    *size = buflib_available(ctx);
    if (*size <= 0) /* OOM */
        return -1;

    return buflib_alloc_ex(ctx, *size, name, ops);
}

/* Shrink the allocation indicated by the handle according to new_start and
 * new_size. Grow is not possible, therefore new_start and new_start + new_size
 * must be within the original allocation
 */
bool
buflib_shrink(struct buflib_context* ctx, int handle, void** new_start, size_t new_size)
{
    /* to keep things simple, we simply make a new allocation and copy things
     * over. This is also the only way to free memory the memory between
     * the header and the new new_start (if any). We need to make sure
     * that the old handle is still valid, i.e. the entry in the handle
     * table stays the same */
    void *data, *new_data;
    union buflib_data *block, *new_block;
    struct buflib_callbacks *ops;
    int new_handle;
    bool retval = true;

    block = handle_to_block(ctx, handle);
    ops = block[2].ops;
    new_handle = buflib_alloc_ex(ctx, new_size, block[3].name, ops);
    if (new_handle < 0)
        return false;

    /* disable IRQs to make accessing the buffer from interrupt context safe. */
    /* protect the move callback, as a cached global pointer might be updated
     * in it. and protect "tmp->alloc = new_start" for buflib_get_data() */
    /* call the callback before moving */
    if (ops && ops->sync_callback)
        ops->sync_callback(handle, true);
    else
        disable_irq();

    /* copy data over */
    data = buflib_get_data(ctx, handle);
    new_data = buflib_get_data(ctx, new_handle);

    /* call user's move_callback since a new allocation changes pointers */
    if (ops && ops->move_callback(handle, data, new_data) == BUFLIB_CB_CANNOT_MOVE)
    {
        /* cannot move. undo and abort */
        buflib_free(ctx, new_handle);
        retval = false;
        *new_start = data;
    }
    else
    {
        /* success. replace alloc with new one */
        memcpy(new_data, data, new_size);
        /* fix header */
        new_block = handle_to_block(ctx, new_handle);
        /* replace the handle table entry to keep the old handle valid */
        handle_free(ctx, new_block[1].handle);
        new_block[1].handle = block[1].handle;
        /* free old alloc. cannot call buflib_free() as it would
         * free the handle table entry as well (which is still needed) */
        free(block);
        /* at last, fix handle table */
        new_block[1].handle->alloc = new_data;

        *new_start = new_data;
    }
    if (ops && ops->sync_callback)
        ops->sync_callback(handle, false);
    else
        enable_irq();

    return retval;
}

const char* buflib_get_name(struct buflib_context *ctx, int handle)
{
    union buflib_data *data = ALIGN_DOWN(buflib_get_data(ctx, handle), sizeof (*data));
    size_t len = data[-1].val;
    if (len <= 1)
        return NULL;
    return data[-len].name;
}

#ifdef BUFLIB_DEBUG_BLOCKS
void buflib_print_allocs(struct buflib_context *ctx,
                                        void (*print)(int, const char*))
{
    union buflib_data *this, *end = ctx->handle_table;
    char buf[128];
    for(this = end - 1; this >= ctx->last_handle; this--)
    {
        if (!this->alloc) continue;

        int handle_num;
        const char *name;
        union buflib_data *block_start, *alloc_start;
        intptr_t alloc_len;

        handle_num = end - this;
        alloc_start = buflib_get_data(ctx, handle_num);
        name = buflib_get_name(ctx, handle_num);
        block_start = (union buflib_data*)name - 3;
        alloc_len = block_start->val * sizeof(union buflib_data);

        snprintf(buf, sizeof(buf),
                "%s(%d):\t%p\n"
                "   \t%p\n"
                "   \t%ld\n",
                name?:"(null)", handle_num, block_start, alloc_start, alloc_len);
        /* handle_num is 1-based */
        print(handle_num - 1, buf);
    }
}

void buflib_print_blocks(struct buflib_context *ctx,
                                        void (*print)(int, const char*))
{
    char buf[128];
    int i = 0;
    for(union buflib_data* this = ctx->buf_start;
                           this < ctx->alloc_end;
                           this += abs(this->val))
    {
        snprintf(buf, sizeof(buf), "%8p: val: %4ld (%s)",
                                this, this->val,
                                this->val > 0? this[3].name:"<unallocated>");
        print(i++, buf);
    }
}
#endif

#ifdef BUFLIB_DEBUG_BLOCK_SINGLE
int buflib_get_num_blocks(struct buflib_context *ctx)
{
    return ctx->handle_table - ctx->last_handle;
}

void buflib_print_block_at(struct buflib_context *ctx, int block_num,
                                char* buf, size_t bufsize)
{
    /* handles are 1-based, block_num is 0 based */
    int handle = block_num + 1;
    union buflib_data* this;
    if (!buflib_get_data(ctx, handle))
    {
        snprintf(buf, bufsize, "(null): val: %4ld <unallocated>", (long)-1);
    }
    else
    {
        this = handle_to_block(ctx, block_num + 1);
        snprintf(buf, bufsize, "%8p: val: %4ld (%s)",
                            this, (long)this->val,
                            this->val > 0? this[3].name:"<unallocated>");
    }
}

#endif
