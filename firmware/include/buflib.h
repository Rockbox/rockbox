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
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef _BUFLIB_H_
#define _BUFLIB_H_
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* enable single block debugging */
#define BUFLIB_DEBUG_BLOCK_SINGLE

union buflib_data
{
    intptr_t val;                 /* length of the block in n*sizeof(union buflib_data).
                                     Includes buflib metadata overhead. A negative value
                                     indicates block is unallocated */
    char name[1];                 /* name, actually a variable sized string */
    struct buflib_callbacks* ops; /* callback functions for move and shrink. Can be NULL */
    char* alloc;                  /* start of allocated memory area */
    union buflib_data *handle;    /* pointer to entry in the handle table.
                                     Used during compaction for fast lookup */
    uint32_t crc;                 /* checksum of this data to detect corruption */
};

struct buflib_context
{
    union buflib_data *handle_table;
    union buflib_data *first_free_handle;
    union buflib_data *last_handle;
    union buflib_data *buf_start;
    union buflib_data *alloc_end;
    bool compact;
};

/**
 * This declares the minimal overhead that is required per alloc. These
 * are bytes that are allocated from the context's pool in addition
 * to the actually requested number of bytes.
 *
 * The total number of bytes consumed by an allocation is
 * BUFLIB_ALLOC_OVERHEAD + requested bytes + strlen(<name passed to
 * buflib_alloc_ex()) + pad to pointer size
 */
#define BUFLIB_ALLOC_OVERHEAD (6*sizeof(union buflib_data))

/**
 * Callbacks used by the buflib to inform allocation that compaction
 * is happening (before data is moved)
 *
 * Note that buflib tries to move to satisfy new allocations before shrinking.
 * So if you have something to resize try to do it outside of the callback.
 *
 * Regardless of the above, if the allocation is SHRINKABLE, but not
 * MUST_NOT_MOVE buflib will move the allocation before even attempting to
 * shrink.
 */
struct buflib_callbacks {
    /**
     * This is called before data is moved. Use this to fix up any cached
     * pointers pointing to inside the allocation. The size is unchanged.
     *
     * This is not needed if you don't cache the data pointer (but always
     * call buflib_get_data()) and don't pass pointer to the data to yielding
     * functions.
     *
     * handle: The corresponding handle
     * current: The current start of the allocation
     * new: The new start of the allocation, after data movement
     *
     * Return: Return BUFLIB_CB_OK, or BUFLIB_CB_CANNOT_MOVE if movement
     * is impossible at this moment.
     *
     * If NULL: this allocation must not be moved around
     * by the buflib when compaction occurs. Attention: Don't confuse
     * that with passing NULL for the whole callback structure
     * to buflib_alloc_ex(). This would enable moving buffers by default.
     * You have to pass NULL inside the "struct buflib_callbacks" structure.
     */
    int (*move_callback)(int handle, void* current, void* new);
    /**
     * This is called when the buflib desires to shrink a buffer
     * in order to satisfy new allocation. This happens when buflib runs
     * out of memory, e.g. because buflib_alloc_maximum() was called.
     * Move data around as you need to make space and call core_shrink() as
     * appropriate from within the callback to complete the shrink operation.
     * buflib will not move data as part of shrinking.
     *
     * hint: bit mask containing hints on how shrinking is desired (see below)
     * handle: The corresponding handle
     * start: The old start of the allocation
     *
     * Return: Return BUFLIB_CB_OK, or BUFLIB_CB_CANNOT_SHRINK if shirinking
     * is impossible at this moment.
     *
     * if NULL: this allocation cannot be resized.
     * It is recommended that allocation that must not move are
     * at least shrinkable
     */
    int (*shrink_callback)(int handle, unsigned hints, void* start, size_t old_size);
    /**
     * This is called when special steps must be taken for synchronization
     * both before the move_callback is called and after the data has been
     * moved.
     */
    void (*sync_callback)(int handle, bool sync_on);
};

/** A set of all NULL callbacks for use with allocations that need to stay
 * locked in RAM and not moved or shrunk. These type of allocations should
 * be avoided as much as possible to avoid memory fragmentation but it can
 * suitable for short-lived allocations. */
extern struct buflib_callbacks buflib_ops_locked;

#define BUFLIB_SHRINK_SIZE_MASK (~BUFLIB_SHRINK_POS_MASK)
#define BUFLIB_SHRINK_POS_FRONT (1u<<31)
#define BUFLIB_SHRINK_POS_BACK  (1u<<30)
#define BUFLIB_SHRINK_POS_MASK  (BUFLIB_SHRINK_POS_FRONT|BUFLIB_SHRINK_POS_BACK)

/**
 * Possible return values for the callbacks, some of them can cause
 * compaction to fail and therefore new allocations to fail
 */
/* Everything alright */
#define BUFLIB_CB_OK 0
/* Tell buflib that moving failed. Buflib may retry to move at any point */
#define BUFLIB_CB_CANNOT_MOVE 1
/* Tell buflib that resizing failed, possibly future making allocations fail */
#define BUFLIB_CB_CANNOT_SHRINK 1

/**
 * Initializes buflib with a caller allocated context instance and memory pool.
 *
 * The buflib_context instance needs to be passed to every other buflib
 * function. It's should be considered opaque, even though it is not yet
 * (that's to make inlining core_get_data() possible). The documentation
 * of the other functions will not describe the context
 * instance parameter further as it's obligatory.
 *
 * context: The new buflib instance to be initialized, allocated by the caller
 * size: The size of the memory pool
 */
void buflib_init(struct buflib_context *context, void *buf, size_t size);


/**
 * Returns the amount of unallocated bytes. It does not mean this amount
 * can be actually allocated because they might not be contiguous.
 *
 * Returns: The number of unallocated bytes in the memory pool.
 */
size_t buflib_available(struct buflib_context *ctx);

/**
 * Returns the biggest possible allocation that can be determined to succeed.
 *
 * Returns: The amount of bytes of the biggest unallocated, contiguous region.
 */
size_t buflib_allocatable(struct buflib_context *ctx);

/**
 * Relocates the fields in *ctx to the new buffer position pointed to by buf.
 * This does _not_ move any data but updates the pointers. The data has
 * to be moved afterwards manually and only if this function returned true.
 *
 * This is intended to be called from within a move_callback(), for
 * buflib-on-buflib scenarios (i.e. a new buflib instance backed by a buffer
 * that was allocated by another buflib instance). Be aware that if the parent
 * move_callback() moves the underlying buffer _no_ move_callback() of the
 * underlying buffer are called.
 *
 * Returns true of the relocation was successful. If it returns false no
 * change to *ctx was made.
 */
bool buflib_context_relocate(struct buflib_context *ctx, void *buf);

/**
 * Allocates memory from buflib's memory pool
 *
 * size: How many bytes to allocate
 *
 * This function passes NULL for the callback structure "ops", so buffers
 * are movable. Don't pass them to functions that yield().
 *
 * Returns: A positive integer handle identifying this allocation, or
 * a negative value on error (0 is also not a valid handle)
 */
int buflib_alloc(struct buflib_context *context, size_t size);


/**
 * Allocates memory from the buflib's memory pool with additional callbacks
 * and flags
 *
 * name: A string identifier giving this allocation a name
 * size: How many bytes to allocate
 * ops: a struct with pointers to callback functions (see above).
 *      if "ops" is NULL: Buffer is movable.
 *
 * Returns: A positive integer handle identifying this allocation, or
 * a negative value on error (0 is also not a valid handle)
 */
int buflib_alloc_ex(struct buflib_context *ctx, size_t size, const char *name,
                    struct buflib_callbacks *ops);


/**
 * Gets all available memory from buflib, for temporary use.
 *
 * Since this effectively makes all future allocations fail (unless
 * another allocation is freed in the meantime), you should definitely provide
 * a shrink callback if you plan to hold the buffer for a longer period. This
 * will allow buflib to permit allocations by shrinking the buffer returned by
 * this function.
 *
 * Note that this might return many more bytes than buflib_available() or
 * buflib_allocatable() return, because it aggressively compacts the pool
 * and even shrinks other allocations. However, do not depend on this behavior,
 * it may change.
 *
 * name: A string identifier giving this allocation a name
 * size: The actual size will be returned into size
 * ops: a struct with pointers to callback functions
 *
 * Returns: A positive integer handle identifying this allocation, or
 * a negative value on error (0 is also not a valid handle)
 */
int buflib_alloc_maximum(struct buflib_context* ctx, const char* name,
                    size_t *size, struct buflib_callbacks *ops);

/**
 * Queries the data pointer for the given handle. It's actually a cheap
 * operation, don't hesitate using it extensively.
 *
 * Notice that you need to re-query after every direct or indirect yield(),
 * because compaction can happen by other threads which may get your data
 * moved around (or you can get notified about changes by callbacks,
 * see further above).
 *
 * handle: The handle corresponding to the allocation
 *
 * Returns: The start pointer of the allocation
 */
#ifdef DEBUG
void* buflib_get_data(struct buflib_context *ctx, int handle);
#else
static inline void* buflib_get_data(struct buflib_context *ctx, int handle)
{
    return (void*)(ctx->handle_table[-handle].alloc);
}
#endif

/**
 * Shrink the memory allocation associated with the given handle
 * Mainly intended to be used with the shrink callback, but it can also
 * be called outside as well, e.g. to give back buffer space allocated
 * with buflib_alloc_maximum().
 *
 * Note that you must move/copy data around yourself before calling this,
 * buflib will not do this as part of shrinking.
 *
 * handle: The handle identifying this allocation
 * new_start: the new start of the allocation
 * new_size: the new size of the allocation
 *
 * Returns: true if shrinking was successful. Otherwise it returns false,
 * without having modified memory.
 *
 */
bool buflib_shrink(struct buflib_context *ctx, int handle, void* newstart, size_t new_size);

/**
 * Frees memory associated with the given handle
 *
 * Returns: 0 (to invalidate handles in one line, 0 is not a valid handle)
 */
int buflib_free(struct buflib_context *context, int handle);

/**
 * Moves the underlying buflib buffer up by size bytes (as much as
 * possible for size == 0) without moving the end. This effectively
 * reduces the available space by taking away manageable space from the
 * front. This space is not available for new allocations anymore.
 *
 * To make space available in the front, everything is moved up.
 * It does _NOT_ call the move callbacks
 * 
 *
 * size: size in bytes to move the buffer up (take away). The actual
 * bytes moved is returned in this
 * Returns: The new start of the underlying buflib buffer
 */
void* buflib_buffer_out(struct buflib_context *ctx, size_t *size);

/**
 * Moves the underlying buflib buffer down by size bytes without
 * moving the end. This grows the buflib buffer by adding space to the front.
 * The new bytes are available for new allocations.
 *
 * Everything is moved down, and the new free space will be in the middle.
 * It does _NOT_ call the move callbacks.
 *
 * size: size in bytes to move the buffer down (new free space)
 */
void buflib_buffer_in(struct buflib_context *ctx, int size);

/* debugging */

/**
 * Returns the name, as given to buflib_alloc() and buflib_allloc_ex(), of the
 * allocation associated with the given handle. As naming allocations
 * is optional, there might be no name associated.
 *
 * handle: The handle indicating the allocation
 *
 * Returns: A pointer to the string identifier of the allocation, or NULL
 * if none was specified with buflib_alloc_ex().
 */
const char* buflib_get_name(struct buflib_context *ctx, int handle);

/**
 * Prints an overview of all current allocations with the help
 * of the passed printer helper
 *
 * This walks only the handle table and prints only valid allocations
 *
 * Only available if BUFLIB_DEBUG_BLOCKS is defined
 */
void buflib_print_allocs(struct buflib_context *ctx, void (*print)(int, const char*));

/**
 * Prints an overview of all blocks in the buflib buffer, allocated
 * or unallocated, with the help of the passed printer helper
 *
 * This walks the entire buffer and prints unallocated space also.
 * The output is also different from buflib_print_allocs().
 *
 * Only available if BUFLIB_DEBUG_BLOCKS is defined
 */
void buflib_print_blocks(struct buflib_context *ctx, void (*print)(int, const char*));

/**
 * Gets the number of blocks in the entire buffer, allocated or unallocated
 *
 * Only available if BUFLIB_DEBUG_BLOCK_SIGNLE is defined
 */
int buflib_get_num_blocks(struct buflib_context *ctx);

/**
 * Print information about a single block as indicated by block_num
 * into buf
 *
 * buflib_get_num_blocks() beforehand to get the total number of blocks,
 * as passing an block_num higher than that is undefined
 *
 * Only available if BUFLIB_DEBUG_BLOCK_SIGNLE is defined
 */
void buflib_print_block_at(struct buflib_context *ctx, int block_num,
                            char* buf, size_t bufsize);

/**
 * Check integrity of given buflib context
 */
void buflib_check_valid(struct buflib_context *ctx);
#endif
