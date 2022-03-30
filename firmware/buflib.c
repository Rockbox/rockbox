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

#include <stdarg.h>
#include <stdlib.h> /* for abs() */
#include <stdio.h> /* for snprintf() */
#include <stddef.h> /* for ptrdiff_t */
#include "buflib.h"
#include "string-extra.h" /* strlcpy() */
#include "debug.h"
#include "panic.h"
#include "crc32.h"
#include "system.h" /* for ALIGN_*() */

/* The main goal of this design is fast fetching of the pointer for a handle.
 * For that reason, the handles are stored in a table at the end of the buffer
 * with a fixed address, so that returning the pointer for a handle is a simple
 * table lookup. To reduce the frequency with which allocated blocks will need
 * to be moved to free space, allocations grow up in address from the start of
 * the buffer. The buffer is treated as an array of union buflib_data. Blocks
 * start with a length marker, which is included in their length. Free blocks
 * are marked by negative length. Allocated blocks have a positiv length marker,
 * and additional metadata following that: It follows a pointer
 * (union buflib_data*) to the corresponding handle table entry. so that it can
 * be quickly found and updated during compaction. After that follows
 * the pointer to the struct buflib_callbacks associated with this allocation
 * (may be NULL). That pointer follows a variable length character array
 * containing the nul-terminated string identifier of the allocation. After this
 * array there's a length marker for the length of the character array including
 * this length marker (counted in n*sizeof(union buflib_data)), which allows
 * to find the start of the character array (and therefore the start of the
 * entire block) when only the handle or payload start is known.
 *
 * UPDATE BUFLIB_ALLOC_OVERHEAD (buflib.h) WHEN THE METADATA CHANGES!
 *
 * Example:
 * |<- alloc block #1 ->|<- unalloc block ->|<- alloc block #2      ->|<-handle table->|
 * |L|H|C|cccc|L2|crc|XXXXXX|-L|YYYYYYYYYYYYYYYY|L|H|C|cc|L2|crc|XXXXXXXXXXXXX|AAA|
 *
 * L - length marker (negative if block unallocated)
 * H - handle table entry pointer
 * C - pointer to struct buflib_callbacks
 * c - variable sized string identifier
 * L2 - length of the metadata
 * crc - crc32 protecting buflib metadata integrity
 * X - actual payload
 * Y - unallocated space
 * 
 * A - pointer to start of payload (first X) in the handle table (may be null)
 *
 * The blocks can be walked by jumping the abs() of the L length marker, i.e.
 * union buflib_data* L;
 * for(L = start; L < end; L += abs(L->val)) { .... }
 *
 * 
 * The allocator functions are passed a context struct so that two allocators
 * can be run, for example, one per core may be used, with convenience wrappers
 * for the single-allocator case that use a predefined context.
 */

#define B_ALIGN_DOWN(x) \
    ALIGN_DOWN(x, sizeof(union buflib_data))

#define B_ALIGN_UP(x) \
    ALIGN_UP(x, sizeof(union buflib_data))

#ifdef DEBUG
    #include <stdio.h>
    #define BDEBUGF DEBUGF
#else
    #define BDEBUGF(...) do { } while(0)
#endif

#define BPANICF panicf

/* Available paranoia checks */
#define PARANOIA_CHECK_LENGTH       (1 << 0)
#define PARANOIA_CHECK_HANDLE       (1 << 1)
#define PARANOIA_CHECK_BLOCK_HANDLE (1 << 2)
/* Bitmask of enabled paranoia checks */
#define BUFLIB_PARANOIA 0

/* Forward indices, used to index a block start pointer as block[fidx_XXX] */
enum {
    fidx_LEN,       /* length of the block, must come first */
    fidx_HANDLE,    /* pointer to entry in the handle table */
    fidx_OPS,       /* pointer to an ops struct */
    fidx_NAME,      /* name, optional and variable length, must come last */
};

/* Backward indices, used to index a block end pointer as block[-bidx_XXX] */
enum {
    bidx_USER,      /* dummy to get below fields to be 1-based */
    bidx_CRC,       /* CRC, protects all metadata behind it */
    bidx_BSIZE,     /* total size of the block header */
};

/* Number of fields in the block header, excluding the name, which is
 * accounted for using the BSIZE field. Note that bidx_USER is not an
 * actual field so it is not included in the count. */
#define BUFLIB_NUM_FIELDS   5

struct buflib_callbacks buflib_ops_locked = {
    .move_callback = NULL,
    .shrink_callback = NULL,
    .sync_callback = NULL,
};

#define IS_MOVABLE(a) (!a[fidx_OPS].ops || a[fidx_OPS].ops->move_callback)

static union buflib_data* find_first_free(struct buflib_context *ctx);
static union buflib_data* find_block_before(struct buflib_context *ctx,
                                            union buflib_data* block,
                                            bool is_free);

/* Check the length of a block to ensure it does not go beyond the end
 * of the allocated area. The block can be either allocated or free.
 *
 * This verifies that it is safe to iterate to the next block in a loop.
 */
static void check_block_length(struct buflib_context *ctx,
                               union buflib_data *block);

/* Check a handle table entry to ensure the user pointer is within the
 * bounds of the allocated area and there is enough room for a minimum
 * size block header.
 *
 * This verifies that it is safe to convert the entry's pointer to a
 * block end pointer and dereference fields at the block end.
 */
static void check_handle(struct buflib_context *ctx,
                         union buflib_data *h_entry);

/* Check a block's handle pointer to ensure it is within the handle
 * table, and that the user pointer is pointing within the block.
 *
 * This verifies that it is safe to dereference the entry, in addition
 * to all checks performed by check_handle(). It also ensures that the
 * pointer in the handle table points within the block, as determined
 * by the length field at the start of the block.
 */
static void check_block_handle(struct buflib_context *ctx,
                               union buflib_data *block);

/* Initialize buffer manager */
void
buflib_init(struct buflib_context *ctx, void *buf, size_t size)
{
    union buflib_data *bd_buf = buf;
    BDEBUGF("buflib initialized with %lu.%02lu kiB\n",
            (unsigned long)size / 1024, ((unsigned long)size%1000)/10);

    /* Align on sizeof(buflib_data), to prevent unaligned access */
    ALIGN_BUFFER(bd_buf, size, sizeof(union buflib_data));
    size /= sizeof(union buflib_data);
    /* The handle table is initialized with no entries */
    ctx->handle_table = bd_buf + size;
    ctx->last_handle = bd_buf + size;
    ctx->first_free_handle = bd_buf + size - 1;
    ctx->buf_start = bd_buf;
    /* A marker is needed for the end of allocated data, to make sure that it
     * does not collide with the handle table, and to detect end-of-buffer.
     */
    ctx->alloc_end = bd_buf;
    ctx->compact = true;

    if (size == 0)
    {
        BPANICF("buflib_init error (CTX:%p, %zd bytes):\n", ctx,
            (ctx->handle_table - ctx->buf_start) * sizeof(union buflib_data));
    }
}

bool buflib_context_relocate(struct buflib_context *ctx, void *buf)
{
    union buflib_data *handle, *bd_buf = buf;
    ptrdiff_t diff = bd_buf - ctx->buf_start;

    /* cannot continue if the buffer is not aligned, since we would need
     * to reduce the size of the buffer for aligning */
    if (!IS_ALIGNED((uintptr_t)buf, sizeof(union buflib_data)))
        return false;

    /* relocate the handle table entries  */
    for (handle = ctx->last_handle; handle < ctx->handle_table; handle++)
    {
        if (handle->alloc)
            handle->alloc += diff * sizeof(union buflib_data);
    }
    /* relocate the pointers in the context */
    ctx->handle_table       += diff;
    ctx->last_handle        += diff;
    ctx->first_free_handle  += diff;
    ctx->buf_start          += diff;
    ctx->alloc_end          += diff;

    return true;
}

static void buflib_panic(struct buflib_context *ctx, const char *message, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, message);
    vsnprintf(buf, sizeof(buf), message, ap);
    va_end(ap);

    BPANICF("buflib error (CTX:%p, %zd bytes):\n%s", ctx,
        (ctx->handle_table - ctx->buf_start) * sizeof(union buflib_data), buf);
}

/* Allocate a new handle, returning 0 on failure */
static inline
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
        {
            /* We know the table is full, so update first_free_handle */
            ctx->first_free_handle = ctx->last_handle - 1;
            return NULL;
        }
    }

    /* We know there are no free handles between the old first_free_handle
     * and the found handle, therefore we can update first_free_handle */
    ctx->first_free_handle = handle - 1;

    /* We need to set the table entry to a non-NULL value to ensure that
     * compactions triggered by an allocation do not compact the handle
     * table and delete this handle. */
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
static inline
union buflib_data* handle_to_block(struct buflib_context* ctx, int handle)
{
    void *ptr = buflib_get_data(ctx, handle);

    /* this is a valid case for shrinking if handle
     * was freed by the shrink callback */
    if (!ptr)
        return NULL;

    union buflib_data *data = ALIGN_DOWN(ptr, sizeof(*data));
    return data - data[-bidx_BSIZE].val;
}

/* Get the block end pointer from a handle table entry */
static union buflib_data*
h_entry_to_block_end(struct buflib_context *ctx, union buflib_data *h_entry)
{
    check_handle(ctx, h_entry);

    void *alloc = h_entry->alloc;
    union buflib_data *data = ALIGN_DOWN(alloc, sizeof(*data));
    return data;
}

/* Shrink the handle table, returning true if its size was reduced, false if
 * not
 */
static inline bool handle_table_shrink(struct buflib_context *ctx)
{
    union buflib_data *handle;
    union buflib_data *old_last = ctx->last_handle;

    for (handle = ctx->last_handle; handle != ctx->handle_table; ++handle)
        if (handle->alloc)
            break;

    if (handle > ctx->first_free_handle)
        ctx->first_free_handle = handle - 1;

    ctx->last_handle = handle;
    return handle != old_last;
}

static uint32_t calc_block_crc(union buflib_data *block,
                               union buflib_data *block_end)
{
    union buflib_data *crc_slot = &block_end[-bidx_CRC];
    const size_t size = (crc_slot - block) * sizeof(*block);
    return crc_32(block, size, 0xffffffff);
}

/* If shift is non-zero, it represents the number of places to move
 * blocks in memory. Calculate the new address for this block,
 * update its entry in the handle table, and then move its contents.
 *
 * Returns false if moving was unsucessful
 * (NULL callback or BUFLIB_CB_CANNOT_MOVE was returned)
 */
static bool
move_block(struct buflib_context* ctx, union buflib_data* block, int shift)
{
    char* new_start;
    union buflib_data *new_block;

    check_block_handle(ctx, block);
    union buflib_data *h_entry = block[fidx_HANDLE].handle;
    union buflib_data *block_end = h_entry_to_block_end(ctx, h_entry);

    uint32_t crc = calc_block_crc(block, block_end);
    if (crc != block_end[-bidx_CRC].crc)
        buflib_panic(ctx, "buflib metadata corrupted, crc: 0x%08x, expected: 0x%08x",
               (unsigned int)crc, (unsigned int)block_end[-bidx_CRC].crc);

    if (!IS_MOVABLE(block))
        return false;

    int handle = ctx->handle_table - h_entry;
    BDEBUGF("%s(): moving \"%s\"(id=%d) by %d(%d)\n", __func__,
            block[fidx_NAME].name, handle, shift, shift*(int)sizeof(union buflib_data));
    new_block = block + shift;
    new_start = h_entry->alloc + shift*sizeof(union buflib_data);

    struct buflib_callbacks *ops = block[fidx_OPS].ops;

    /* If move must be synchronized with use, user should have specified a
       callback that handles this */
    if (ops && ops->sync_callback)
        ops->sync_callback(handle, true);

    bool retval = false;
    if (!ops || ops->move_callback(handle, h_entry->alloc, new_start)
                    != BUFLIB_CB_CANNOT_MOVE)
    {
        h_entry->alloc = new_start; /* update handle table */
        memmove(new_block, block, block->val * sizeof(union buflib_data));
        retval = true;
    }

    if (ops && ops->sync_callback)
        ops->sync_callback(handle, false);

    return retval;
}

/* Compact allocations and handle table, adjusting handle pointers as needed.
 * Return true if any space was freed or consolidated, false otherwise.
 */
static bool
buflib_compact(struct buflib_context *ctx)
{
    BDEBUGF("%s(): Compacting!\n", __func__);
    union buflib_data *block,
                      *hole = NULL;
    int shift = 0, len;
    /* Store the results of attempting to shrink the handle table */
    bool ret = handle_table_shrink(ctx);
    /* compaction has basically two modes of operation:
     *  1) the buffer is nicely movable: In this mode, blocks can be simply
     * moved towards the beginning. Free blocks add to a shift value,
     * which is the amount to move.
     *  2) the buffer contains unmovable blocks: unmovable blocks create
     * holes and reset shift. Once a hole is found, we're trying to fill
     * holes first, moving by shift is the fallback. As the shift is reset,
     * this effectively splits the buffer into portions of movable blocks.
     * This mode cannot be used if no holes are found yet as it only works
     * when it moves blocks across the portions. On the other side,
     * moving by shift only works within the same portion
     * For simplicity only 1 hole at a time is considered */
    for(block = find_first_free(ctx); block < ctx->alloc_end; block += len)
    {
        check_block_length(ctx, block);

        bool movable = true; /* cache result to avoid 2nd call to move_block */
        len = block->val;
        /* This block is free, add its length to the shift value */
        if (len < 0)
        {
            shift += len;
            len = -len;
            continue;
        }
        /* attempt to fill any hole */
        if (hole && -hole->val >= len)
        {
            intptr_t hlen = -hole->val;
            if ((movable = move_block(ctx, block, hole - block)))
            {
                ret = true;
                /* Move was successful. The memory at block is now free */
                block->val = -len;

                /* add its length to shift */
                shift += -len;
                /* Reduce the size of the hole accordingly
                 * but be careful to not overwrite an existing block */
                if (hlen != len)
                {
                    hole += len;
                    hole->val = len - hlen; /* negative */
                }
                else /* hole closed */
                    hole = NULL;
                continue;
            }
        }
        /* attempt move the allocation by shift */
        if (shift)
        {
            union buflib_data* target_block = block + shift;
            if (!movable || !move_block(ctx, block, shift))
            {
                /* free space before an unmovable block becomes a hole,
                 * therefore mark this block free and track the hole */
                target_block->val = shift;
                hole = target_block;
                shift = 0;
            }
            else
                ret = true;
        }
    }
    /* Move the end-of-allocation mark, and return true if any new space has
     * been freed.
     */
    ctx->alloc_end += shift;
    ctx->compact = true;
    return ret || shift;
}

/* Compact the buffer by trying both shrinking and moving.
 *
 * Try to move first. If unsuccesfull, try to shrink. If that was successful
 * try to move once more as there might be more room now.
 */
static bool
buflib_compact_and_shrink(struct buflib_context *ctx, unsigned shrink_hints)
{
    bool result = false;
    /* if something compacted before already there will be no further gain */
    if (!ctx->compact)
        result = buflib_compact(ctx);
    if (!result)
    {
        union buflib_data *this, *before;
        for(this = ctx->buf_start, before = this;
            this < ctx->alloc_end;
            before = this, this += abs(this->val))
        {
            check_block_length(ctx, this);
            if (this->val < 0)
                continue;

            struct buflib_callbacks *ops = this[fidx_OPS].ops;
            if (!ops || !ops->shrink_callback)
                continue;

            check_block_handle(ctx, this);
            union buflib_data* h_entry = this[fidx_HANDLE].handle;
            int handle = ctx->handle_table - h_entry;

            unsigned pos_hints = shrink_hints & BUFLIB_SHRINK_POS_MASK;
            /* adjust what we ask for if there's free space in the front
             * this isn't too unlikely assuming this block is
             * shrinkable but not movable */
            if (pos_hints == BUFLIB_SHRINK_POS_FRONT &&
                before != this && before->val < 0)
            {
                size_t free_space = (-before->val) * sizeof(union buflib_data);
                size_t wanted = shrink_hints & BUFLIB_SHRINK_SIZE_MASK;
                if (wanted < free_space) /* no shrink needed? */
                    continue;
                wanted -= free_space;
                shrink_hints = pos_hints | wanted;
            }

            char* data = h_entry->alloc;
            char* data_end = (char*)(this + this->val);
            bool last = (data_end == (char*)ctx->alloc_end);

            int ret = ops->shrink_callback(handle, shrink_hints,
                                           data, data_end - data);
            result |= (ret == BUFLIB_CB_OK);

            /* 'this' might have changed in the callback (if it shrinked
             * from the top or even freed the handle), get it again */
            this = handle_to_block(ctx, handle);

            /* The handle was possibly be freed in the callback,
             * re-run the loop with the handle before */
            if (!this)
                this = before;
            /* could also change with shrinking from back */
            else if (last)
                ctx->alloc_end = this + this->val;
        }
        /* shrinking was successful at least once, try compaction again */
        if (result)
            result |= buflib_compact(ctx);
    }

    return result;
}

/* Shift buffered items by size units, and update handle pointers. The shift
 * value must be determined to be safe *before* calling.
 */
static void
buflib_buffer_shift(struct buflib_context *ctx, int shift)
{
    memmove(ctx->buf_start + shift, ctx->buf_start,
        (ctx->alloc_end - ctx->buf_start) * sizeof(union buflib_data));
    ctx->buf_start += shift;
    ctx->alloc_end += shift;
    shift *= sizeof(union buflib_data);
    union buflib_data *handle;
    for (handle = ctx->last_handle; handle < ctx->handle_table; handle++)
        if (handle->alloc)
            handle->alloc += shift;
}

/* Shift buffered items up by size bytes, or as many as possible if size == 0.
 * Set size to the number of bytes freed.
 */
void*
buflib_buffer_out(struct buflib_context *ctx, size_t *size)
{
    if (!ctx->compact)
        buflib_compact(ctx);
    size_t avail = ctx->last_handle - ctx->alloc_end;
    size_t avail_b = avail * sizeof(union buflib_data);
    if (*size && *size < avail_b)
    {
        avail = (*size + sizeof(union buflib_data) - 1)
            / sizeof(union buflib_data);
        avail_b = avail * sizeof(union buflib_data);
    }
    *size = avail_b;
    void *ret = ctx->buf_start;
    buflib_buffer_shift(ctx, avail);
    return ret;
}

/* Shift buffered items down by size bytes */
void
buflib_buffer_in(struct buflib_context *ctx, int size)
{
    size /= sizeof(union buflib_data);
    buflib_buffer_shift(ctx, -size);
}

/* Allocate a buffer of size bytes, returning a handle for it.
 * Note: Buffers are movable since NULL is passed for "ops".
         Don't pass them to functions that call yield() */
int
buflib_alloc(struct buflib_context *ctx, size_t size)
{
    return buflib_alloc_ex(ctx, size, NULL, NULL);
}

/* Allocate a buffer of size bytes, returning a handle for it.
 *
 * The additional name parameter gives the allocation a human-readable name,
 * the ops parameter points to caller-implemented callbacks for moving and
 * shrinking.
 *
 * If you pass NULL for "ops", buffers are movable by default.
 * Don't pass them to functions that call yield() like I/O.
 * Buffers are only shrinkable when a shrink callback is given.
 */

int
buflib_alloc_ex(struct buflib_context *ctx, size_t size, const char *name,
                struct buflib_callbacks *ops)
{
    union buflib_data *handle, *block;
    size_t name_len = name ? B_ALIGN_UP(strlen(name)+1) : 0;
    bool last;
    /* This really is assigned a value before use */
    int block_len;
    size += name_len;
    size = (size + sizeof(union buflib_data) - 1) /
           sizeof(union buflib_data)
           + BUFLIB_NUM_FIELDS;
handle_alloc:
    handle = handle_alloc(ctx);
    if (!handle)
    {
        /* If allocation has failed, and compaction has succeded, it may be
         * possible to get a handle by trying again.
         */
        union buflib_data* last_block = find_block_before(ctx,
                                            ctx->alloc_end, false);
        struct buflib_callbacks* ops = last_block[fidx_OPS].ops;
        unsigned hints = 0;
        if (!ops || !ops->shrink_callback)
        {   /* the last one isn't shrinkable
             * make room in front of a shrinkable and move this alloc */
            hints = BUFLIB_SHRINK_POS_FRONT;
            hints |= last_block->val * sizeof(union buflib_data);
        }
        else if (ops && ops->shrink_callback)
        {   /* the last is shrinkable, make room for handles directly */
            hints = BUFLIB_SHRINK_POS_BACK;
            hints |= 16*sizeof(union buflib_data);
        }
        /* buflib_compact_and_shrink() will compact and move last_block()
         * if possible */
        if (buflib_compact_and_shrink(ctx, hints))
            goto handle_alloc;
        return -1;
    }

buffer_alloc:
    /* need to re-evaluate last before the loop because the last allocation
     * possibly made room in its front to fit this, so last would be wrong */
    last = false;
    for (block = find_first_free(ctx);; block += block_len)
    {
        /* If the last used block extends all the way to the handle table, the
         * block "after" it doesn't have a header. Because of this, it's easier
         * to always find the end of allocation by saving a pointer, and always
         * calculate the free space at the end by comparing it to the
         * last_handle pointer.
         */
        if(block == ctx->alloc_end)
        {
            last = true;
            block_len = ctx->last_handle - block;
            if ((size_t)block_len < size)
                block = NULL;
            break;
        }

        check_block_length(ctx, block);
        block_len = block->val;
        /* blocks with positive length are already allocated. */
        if(block_len > 0)
            continue;
        block_len = -block_len;
        /* The search is first-fit, any fragmentation this causes will be
         * handled at compaction.
         */
        if ((size_t)block_len >= size)
            break;
    }
    if (!block)
    {
        /* Try compacting if allocation failed */
        unsigned hint = BUFLIB_SHRINK_POS_FRONT |
                    ((size*sizeof(union buflib_data))&BUFLIB_SHRINK_SIZE_MASK);
        if (buflib_compact_and_shrink(ctx, hint))
        {
            goto buffer_alloc;
        } else {
            handle_free(ctx, handle);
            return -2;
        }
    }

    /* Set up the allocated block, by marking the size allocated, and storing
     * a pointer to the handle.
     */
    block[fidx_LEN].val = size;
    block[fidx_HANDLE].handle = handle;
    block[fidx_OPS].ops = ops;
    if (name_len > 0)
        strcpy(block[fidx_NAME].name, name);

    size_t bsize = BUFLIB_NUM_FIELDS + name_len/sizeof(union buflib_data);
    union buflib_data *block_end = block + bsize;
    block_end[-bidx_BSIZE].val = bsize;
    block_end[-bidx_CRC].crc = calc_block_crc(block, block_end);

    handle->alloc = (char*)&block_end[-bidx_USER];

    BDEBUGF("buflib_alloc_ex: size=%d handle=%p clb=%p crc=0x%0x name=\"%s\"\n",
            (unsigned int)size, (void *)handle, (void *)ops,
            (unsigned int)block_end[-bidx_CRC].crc, name ? name : "");

    block += size;
    /* alloc_end must be kept current if we're taking the last block. */
    if (last)
        ctx->alloc_end = block;
    /* Only free blocks *before* alloc_end have tagged length. */
    else if ((size_t)block_len > size)
        block->val = size - block_len;
    /* Return the handle index as a positive integer. */
    return ctx->handle_table - handle;
}

static union buflib_data*
find_first_free(struct buflib_context *ctx)
{
    union buflib_data *ret;
    for(ret = ctx->buf_start; ret < ctx->alloc_end; ret += ret->val)
    {
        check_block_length(ctx, ret);
        if (ret->val < 0)
            break;
    }

    /* ret is now either a free block or the same as alloc_end, both is fine */
    return ret;
}

/* Finds the free block before block, and returns NULL if it's not free */
static union buflib_data*
find_block_before(struct buflib_context *ctx, union buflib_data* block,
                  bool is_free)
{
    union buflib_data *ret        = ctx->buf_start,
                      *next_block = ret;

    /* no previous block */
    if (next_block == block)
        return NULL;

    /* find the block that's before the current one */
    while (next_block != block)
    {
        check_block_length(ctx, ret);
        ret = next_block;
        next_block += abs(ret->val);
    }

    /* don't return it if the found block isn't free */
    if (is_free && ret->val >= 0)
        return NULL;

    return ret;
}

/* Free the buffer associated with handle_num. */
int
buflib_free(struct buflib_context *ctx, int handle_num)
{
    union buflib_data *handle = ctx->handle_table - handle_num,
                      *freed_block = handle_to_block(ctx, handle_num),
                      *block, *next_block;
    /* We need to find the block before the current one, to see if it is free
     * and can be merged with this one.
     */
    block = find_block_before(ctx, freed_block, true);
    if (block)
    {
        block->val -= freed_block->val;
    }
    else
    {
        /* Otherwise, set block to the newly-freed block, and mark it free,
         * before continuing on, since the code below expects block to point
         * to a free block which may have free space after it. */
        block = freed_block;
        block->val = -block->val;
    }

    next_block = block - block->val;
    /* Check if we are merging with the free space at alloc_end. */
    if (next_block == ctx->alloc_end)
        ctx->alloc_end = block;
    /* Otherwise, the next block might still be a "normal" free block, and the
     * mid-allocation free means that the buffer is no longer compact.
     */
    else {
        ctx->compact = false;
        if (next_block->val < 0)
            block->val += next_block->val;
    }
    handle_free(ctx, handle);
    handle->alloc = NULL;

    return 0; /* unconditionally */
}

static size_t
free_space_at_end(struct buflib_context* ctx)
{
    /* subtract 5 elements for
     * val, handle, meta_len, ops and the handle table entry*/
    ptrdiff_t diff = (ctx->last_handle - ctx->alloc_end - BUFLIB_NUM_FIELDS);
    diff -= 16; /* space for future handles */
    diff *= sizeof(union buflib_data); /* make it bytes */
    diff -= 16; /* reserve 16 for the name */

    if (diff > 0)
        return diff;
    else
        return 0;
}

/* Return the maximum allocatable contiguous memory in bytes */
size_t
buflib_allocatable(struct buflib_context* ctx)
{
    size_t free_space = 0, max_free_space = 0;
    intptr_t block_len;

    /* make sure buffer is as contiguous as possible  */
    if (!ctx->compact)
        buflib_compact(ctx);

    /* now look if there's free in holes */
    for(union buflib_data *block = find_first_free(ctx);
        block < ctx->alloc_end;
        block += block_len)
    {
        check_block_length(ctx, block);
        block_len = block->val;

        if (block_len < 0)
        {
            block_len = -block_len;
            free_space += block_len;
            continue;
        }

        /* an unmovable section resets the count as free space
         * can't be contigous */
        if (!IS_MOVABLE(block))
        {
            if (max_free_space < free_space)
                max_free_space = free_space;
            free_space = 0;
        }
    }

    /* select the best */
    max_free_space = MAX(max_free_space, free_space);
    max_free_space *= sizeof(union buflib_data);
    max_free_space = MAX(max_free_space, free_space_at_end(ctx));

    if (max_free_space > 0)
        return max_free_space;
    else
        return 0;
}

/* Return the amount of unallocated memory in bytes (even if not contiguous) */
size_t
buflib_available(struct buflib_context* ctx)
{
    size_t free_space = 0;

    /* add up all holes */
    for(union buflib_data *block = find_first_free(ctx);
        block < ctx->alloc_end;
        block += abs(block->val))
    {
        check_block_length(ctx, block);
        if (block->val < 0)
            free_space += -block->val;
    }

    free_space *= sizeof(union buflib_data); /* make it bytes */
    free_space += free_space_at_end(ctx);

    return free_space;
}

/*
 * Allocate all available (as returned by buflib_available()) memory and return
 * a handle to it
 *
 * This grabs a lock which can only be unlocked by buflib_free() or
 * buflib_shrink(), to protect from further allocations (which couldn't be
 * serviced anyway).
 */
int
buflib_alloc_maximum(struct buflib_context* ctx, const char* name, size_t *size, struct buflib_callbacks *ops)
{
    /* limit name to 16 since that's what buflib_available() accounts for it */
    char buf[16];

    /* ignore ctx->compact because it's true if all movable blocks are contiguous
     * even if the buffer has holes due to unmovable allocations */
    unsigned hints;
    size_t bufsize = ctx->handle_table - ctx->buf_start;
    bufsize = MIN(BUFLIB_SHRINK_SIZE_MASK, bufsize*sizeof(union buflib_data)); /* make it bytes */
    /* try as hard as possible to free up space. allocations are
     * welcome to give up some or all of their memory */
    hints = BUFLIB_SHRINK_POS_BACK | BUFLIB_SHRINK_POS_FRONT | bufsize;
    /* compact until no space can be gained anymore */
    while (buflib_compact_and_shrink(ctx, hints));

    *size = buflib_allocatable(ctx);
    if (*size <= 0) /* OOM */
        return -1;

    strlcpy(buf, name, sizeof(buf));

    return buflib_alloc_ex(ctx, *size, buf, ops);
}

/* Shrink the allocation indicated by the handle according to new_start and
 * new_size. Grow is not possible, therefore new_start and new_start + new_size
 * must be within the original allocation
 */
bool
buflib_shrink(struct buflib_context* ctx, int handle, void* new_start, size_t new_size)
{
    char* oldstart = buflib_get_data(ctx, handle);
    char* newstart = new_start;
    char* newend = newstart + new_size;

    /* newstart must be higher and new_size not "negative" */
    if (newstart < oldstart || newend < newstart)
        return false;
    union buflib_data *block = handle_to_block(ctx, handle),
                      *old_next_block = block + block->val,
                /* newstart isn't necessarily properly aligned but it
                 * needn't be since it's only dereferenced by the user code */
                      *aligned_newstart = (union buflib_data*)B_ALIGN_DOWN(newstart),
                      *aligned_oldstart = (union buflib_data*)B_ALIGN_DOWN(oldstart),
                      *new_next_block =   (union buflib_data*)B_ALIGN_UP(newend),
                      *new_block, metadata_size;

    /* growing is not supported */
    if (new_next_block > old_next_block)
        return false;

    metadata_size.val = aligned_oldstart - block;
    /* update val and the handle table entry */
    new_block = aligned_newstart - metadata_size.val;
    block[fidx_LEN].val = new_next_block - new_block;

    check_block_handle(ctx, block);
    block[fidx_HANDLE].handle->alloc = newstart;
    if (block != new_block)
    {
        /* move metadata over, i.e. pointer to handle table entry and name
         * This is actually the point of no return. Data in the allocation is
         * being modified, and therefore we must successfully finish the shrink
         * operation */
        memmove(new_block, block, metadata_size.val*sizeof(metadata_size));
        /* mark the old block unallocated */
        block->val = block - new_block;
        /* find the block before in order to merge with the new free space */
        union buflib_data *free_before = find_block_before(ctx, block, true);
        if (free_before)
            free_before->val += block->val;

        /* We didn't handle size changes yet, assign block to the new one
         * the code below the wants block whether it changed or not */
        block = new_block;
    }

    /* update crc of the metadata */
    union buflib_data *new_h_entry = new_block[fidx_HANDLE].handle;
    union buflib_data *new_block_end = h_entry_to_block_end(ctx, new_h_entry);
    new_block_end[-bidx_CRC].crc = calc_block_crc(new_block, new_block_end);

    /* Now deal with size changes that create free blocks after the allocation */
    if (old_next_block != new_next_block)
    {
        if (ctx->alloc_end == old_next_block)
            ctx->alloc_end = new_next_block;
        else if (old_next_block->val < 0)
        {   /* enlarge next block by moving it up */
            new_next_block->val = old_next_block->val - (old_next_block - new_next_block);
        }
        else if (old_next_block != new_next_block)
        {   /* creating a hole */
            /* must be negative to indicate being unallocated */
            new_next_block->val = new_next_block - old_next_block;
        }
    }

    return true;
}

const char* buflib_get_name(struct buflib_context *ctx, int handle)
{
    union buflib_data *data = ALIGN_DOWN(buflib_get_data(ctx, handle), sizeof (*data));
    size_t len = data[-bidx_BSIZE].val;
    if (len <= BUFLIB_NUM_FIELDS)
        return NULL;

    data -= len;
    return data[fidx_NAME].name;
}

#ifdef DEBUG

void *buflib_get_data(struct buflib_context *ctx, int handle)
{
    if (handle <= 0)
        buflib_panic(ctx, "invalid handle access: %d", handle);

    return (void*)(ctx->handle_table[-handle].alloc);
}

void buflib_check_valid(struct buflib_context *ctx)
{
    for(union buflib_data *block = ctx->buf_start;
        block < ctx->alloc_end;
        block += abs(block->val))
    {
        check_block_length(ctx, block);
        if (block->val < 0)
            continue;

        check_block_handle(ctx, block);
        union buflib_data *h_entry = block[fidx_HANDLE].handle;
        union buflib_data *block_end = h_entry_to_block_end(ctx, h_entry);
        uint32_t crc = calc_block_crc(block, block_end);
        if (crc != block_end[-bidx_CRC].crc)
        {
            buflib_panic(ctx, "crc mismatch: 0x%08x, expected: 0x%08x",
                         (unsigned int)crc, (unsigned int)block_end[-bidx_CRC].crc);
        }
    }
}
#endif

#ifdef BUFLIB_DEBUG_BLOCKS
void buflib_print_allocs(struct buflib_context *ctx,
                                        void (*print)(int, const char*))
{
    union buflib_data *this, *end = ctx->handle_table;
    char buf[128];
    for(this = end - 1; this >= ctx->last_handle; this--)
    {
        if (!this->alloc) continue;

        int handle_num = end - this;
        void* alloc_start = this->alloc;
        union buflib_data *block_start = handle_to_block(ctx, handle_num);
        const char* name = buflib_get_name(ctx, handle_num);
        intptr_t alloc_len = block_start[fidx_LEN];

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

    for(union buflib_data *block = ctx->buf_start;
        block != ctx->alloc_end;
        block += abs(block->val))
    {
        check_block_length(ctx, block);

        snprintf(buf, sizeof(buf), "%8p: val: %4ld (%s)",
                 block, (long)block->val,
                 block->val > 0 ? block[fidx_NAME].name : "<unallocated>");
        print(i++, buf);
    }
}
#endif

#ifdef BUFLIB_DEBUG_BLOCK_SINGLE
int buflib_get_num_blocks(struct buflib_context *ctx)
{
    int i = 0;
    for(union buflib_data *block = ctx->buf_start;
        block < ctx->alloc_end;
        block += abs(block->val))
    {
        check_block_length(ctx, block);
        ++i;
    }
    return i;
}

void buflib_print_block_at(struct buflib_context *ctx, int block_num,
                           char* buf, size_t bufsize)
{
    for(union buflib_data *block = ctx->buf_start;
        block < ctx->alloc_end;
        block += abs(block->val))
    {
        check_block_length(ctx, block);

        if (block_num-- == 0)
        {
            snprintf(buf, bufsize, "%8p: val: %4ld (%s)",
                     block, (long)block->val,
                     block->val > 0 ? block[fidx_NAME].name : "<unallocated>");
        }
    }
}
#endif

static void check_block_length(struct buflib_context *ctx,
                               union buflib_data *block)
{
    if (BUFLIB_PARANOIA & PARANOIA_CHECK_LENGTH)
    {
        intptr_t length = block[fidx_LEN].val;

        /* Check the block length does not pass beyond the end */
        if (length == 0 || block > ctx->alloc_end - abs(length))
        {
            buflib_panic(ctx, "block len wacky [%p]=%ld",
                         (void*)&block[fidx_LEN], (long)length);
        }
    }
}

static void check_handle(struct buflib_context *ctx,
                         union buflib_data *h_entry)
{
    if (BUFLIB_PARANOIA & PARANOIA_CHECK_HANDLE)
    {
        /* For the pointer to be valid there needs to be room for a minimum
         * size block header, so we add BUFLIB_NUM_FIELDS to ctx->buf_start. */
        void *alloc       = h_entry->alloc;
        void *alloc_begin = ctx->buf_start + BUFLIB_NUM_FIELDS;
        void *alloc_end   = ctx->alloc_end;
        /* buflib allows zero length allocations, so alloc_end is inclusive */
        if (alloc < alloc_begin || alloc > alloc_end)
        {
            buflib_panic(ctx, "alloc outside buf [%p]=%p, %p-%p",
                         h_entry, alloc, alloc_begin, alloc_end);
        }
    }
}

static void check_block_handle(struct buflib_context *ctx,
                               union buflib_data *block)
{
    if (BUFLIB_PARANOIA & PARANOIA_CHECK_BLOCK_HANDLE)
    {
        intptr_t length = block[fidx_LEN].val;
        union buflib_data *h_entry = block[fidx_HANDLE].handle;

        /* Check the handle pointer is properly aligned */
        /* TODO: Can we ensure the compiler doesn't optimize this out?
         * I dunno, maybe the compiler can assume the pointer is always
         * properly aligned due to some C standard voodoo?? */
        if (!IS_ALIGNED((uintptr_t)h_entry, alignof(*h_entry)))
        {
            buflib_panic(ctx, "handle unaligned [%p]=%p",
                         &block[fidx_HANDLE], h_entry);
        }

        /* Check the pointer is actually inside the handle table */
        if (h_entry < ctx->last_handle || h_entry >= ctx->handle_table)
        {
            buflib_panic(ctx, "handle out of bounds [%p]=%p",
                         &block[fidx_HANDLE], h_entry);
        }

        /* Now check the allocation is within the block.
         * This is stricter than check_handle(). */
        void *alloc       = h_entry->alloc;
        void *alloc_begin = block;
        void *alloc_end   = block + length;
        /* buflib allows zero length allocations, so alloc_end is inclusive */
        if (alloc < alloc_begin || alloc > alloc_end)
        {
            buflib_panic(ctx, "alloc outside block [%p]=%p, %p-%p",
                         h_entry, alloc, alloc_begin, alloc_end);
        }
    }
}
