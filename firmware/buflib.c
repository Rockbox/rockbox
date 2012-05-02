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

/* The main goal of this design is fast fetching of the pointer for a handle.
 * For that reason, the handles are stored in a table at the end of the buffer
 * with a fixed address, so that returning the pointer for a handle is a simple
 * table lookup. To reduce the frequency with which allocated blocks will need
 * to be moved to free space, allocations grow up in address from the start of
 * the buffer. The buffer is treated as an array of union buflib_data. Blocks
 * start with a length marker, which is included in their length. Free blocks
 * are marked by negative length. Allocated blocks have a positiv length marker,
 * and additional metadata forllowing that: It follows a pointer
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
 * Example:
 * |<- alloc block #1 ->|<- unalloc block ->|<- alloc block #2      ->|<-handle table->|
 * |L|H|C|cccc|L2|XXXXXX|-L|YYYYYYYYYYYYYYYY|L|H|C|cc|L2|XXXXXXXXXXXXX|AAA|
 *
 * L - length marker (negative if block unallocated)
 * H - handle table enry pointer
 * C - pointer to struct buflib_callbacks
 * c - variable sized string identifier
 * L2 - second length marker for string identifier
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

#define IS_MOVABLE(a) (!a[2].ops || a[2].ops->move_callback)
static union buflib_data* find_first_free(struct buflib_context *ctx);
static union buflib_data* find_block_before(struct buflib_context *ctx,
                                            union buflib_data* block,
                                            bool is_free);
/* Initialize buffer manager */
void
buflib_init(struct buflib_context *ctx, void *buf, size_t size)
{
    union buflib_data *bd_buf = buf;

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

    BDEBUGF("buflib initialized with %lu.%2lu kiB",
            (unsigned long)size / 1024, ((unsigned long)size%1000)/10);
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
            return NULL;
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

/* Shrink the handle table, returning true if its size was reduced, false if
 * not
 */
static inline
bool
handle_table_shrink(struct buflib_context *ctx)
{
    bool rv;
    union buflib_data *handle;
    for (handle = ctx->last_handle; !(handle->alloc); handle++);
    if (handle > ctx->first_free_handle)
        ctx->first_free_handle = handle - 1;
    rv = handle != ctx->last_handle;
    ctx->last_handle = handle;
    return rv;
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
    union buflib_data *new_block, *tmp = block[1].handle;
    struct buflib_callbacks *ops = block[2].ops;
    if (!IS_MOVABLE(block))
        return false;

    int handle = ctx->handle_table - tmp;
    BDEBUGF("%s(): moving \"%s\"(id=%d) by %d(%d)\n", __func__, block[3].name,
            handle, shift, shift*(int)sizeof(union buflib_data));
    new_block = block + shift;
    new_start = tmp->alloc + shift*sizeof(union buflib_data);

    /* disable IRQs to make accessing the buffer from interrupt context safe. */
    /* protect the move callback, as a cached global pointer might be updated
     * in it. and protect "tmp->alloc = new_start" for buflib_get_data() */
    /* call the callback before moving */
    if (ops && ops->sync_callback)
    {
        ops->sync_callback(handle, true);
    }
    else
    {
        disable_irq();
    }

    bool retval = false;
    if (!ops || ops->move_callback(handle, tmp->alloc, new_start)
                    != BUFLIB_CB_CANNOT_MOVE)
    {
        tmp->alloc = new_start; /* update handle table */
        memmove(new_block, block, block->val * sizeof(union buflib_data));
        retval = true;
    }

    if (ops && ops->sync_callback)
    {
        ops->sync_callback(handle, false);
    }
    else
    {
        enable_irq();
    }

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
            if (this->val > 0 && this[2].ops
                              && this[2].ops->shrink_callback)
            {
                int ret;
                int handle = ctx->handle_table - this[1].handle;
                char* data = this[1].handle->alloc;
                bool last = (this+this->val) == ctx->alloc_end;
                unsigned pos_hints = shrink_hints & BUFLIB_SHRINK_POS_MASK;
                /* adjust what we ask for if there's free space in the front
                 * this isn't too unlikely assuming this block is
                 * shrinkable but not movable */
                if (pos_hints == BUFLIB_SHRINK_POS_FRONT
                    && before != this && before->val < 0)
                {   
                    size_t free_space = (-before->val) * sizeof(union buflib_data);
                    size_t wanted = shrink_hints & BUFLIB_SHRINK_SIZE_MASK;
                    if (wanted < free_space) /* no shrink needed? */
                        continue;
                    wanted -= free_space;
                    shrink_hints = pos_hints | wanted;
                }
                ret = this[2].ops->shrink_callback(handle, shrink_hints,
                                            data, (char*)(this+this->val)-data);
                result |= (ret == BUFLIB_CB_OK);
                /* this might have changed in the callback (if
                 * it shrinked from the top), get it again */
                this = handle_to_block(ctx, handle);
                /* could also change with shrinking from back */
                if (last)
                    ctx->alloc_end = this + this->val;
            }
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
    union buflib_data *handle, *block;
    size_t name_len = name ? B_ALIGN_UP(strlen(name)+1) : 0;
    bool last;
    /* This really is assigned a value before use */
    int block_len;
    size += name_len;
    size = (size + sizeof(union buflib_data) - 1) /
           sizeof(union buflib_data)
           /* add 4 objects for alloc len, pointer to handle table entry and
            * name length, and the ops pointer */
           + 4;
handle_alloc:
    handle = handle_alloc(ctx);
    if (!handle)
    {
        /* If allocation has failed, and compaction has succeded, it may be
         * possible to get a handle by trying again.
         */
        union buflib_data* last_block = find_block_before(ctx,
                                            ctx->alloc_end, false);
        struct buflib_callbacks* ops = last_block[2].ops;
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
    for (block = find_first_free(ctx);;block += block_len)
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
            handle->val=1;
            handle_free(ctx, handle);
            return -2;
        }
    }

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
    union buflib_data* ret = ctx->buf_start;
    while(ret < ctx->alloc_end)
    {
        if (ret->val < 0)
            break;
        ret += ret->val;
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

    /* find the block that's before the current one */
    while (next_block < block)
    {
        ret = next_block;
        next_block += abs(ret->val);
    }

    /* If next_block == block, the above loop didn't go anywhere. If it did,
     * and the block before this one is empty, that is the wanted one
     */
    if (next_block == block && ret < block)
    {
        if (is_free && ret->val >= 0) /* NULL if found block isn't free */
            return NULL;
        return ret;
    }
    return NULL;
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
    /* Otherwise, set block to the newly-freed block, and mark it free, before
     * continuing on, since the code below exects block to point to a free
     * block which may have free space after it.
     */
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
     * val, handle, name_len, ops and the handle table entry*/
    ptrdiff_t diff = (ctx->last_handle - ctx->alloc_end - 5);
    diff -= 16; /* space for future handles */
    diff *= sizeof(union buflib_data); /* make it bytes */
    diff -= 16; /* reserve 16 for the name */

    if (diff > 0)
        return diff;
    else
        return 0;
}

/* Return the maximum allocatable memory in bytes */
size_t
buflib_available(struct buflib_context* ctx)
{
    union buflib_data *this;
    size_t free_space = 0, max_free_space = 0;

    /* make sure buffer is as contiguous as possible  */
    if (!ctx->compact)
        buflib_compact(ctx);

    /* now look if there's free in holes */
    for(this = find_first_free(ctx); this < ctx->alloc_end; this += abs(this->val))
    {
        if (this->val < 0)
        {
            free_space += -this->val;
            continue;
        }
        /* an unmovable section resets the count as free space
         * can't be contigous */
        if (!IS_MOVABLE(this))
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

    *size = buflib_available(ctx);
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
    block[0].val = new_next_block - new_block;

    block[1].handle->alloc = newstart;
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
    int i = 0;
    for(union buflib_data* this = ctx->buf_start;
                           this < ctx->alloc_end;
                           this += abs(this->val))
    {
        i++;
    }
    return i;
}

void buflib_print_block_at(struct buflib_context *ctx, int block_num,
                                char* buf, size_t bufsize)
{
    union buflib_data* this = ctx->buf_start;
    while(block_num > 0 && this < ctx->alloc_end)
    {
        this += abs(this->val);
        block_num -= 1;
    }
    snprintf(buf, bufsize, "%8p: val: %4ld (%s)",
                            this, (long)this->val,
                            this->val > 0? this[3].name:"<unallocated>");
}

#endif
