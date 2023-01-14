/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Andrew Mahone
 * Copyright (C) 2011 Thomas Martitz
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
#ifndef _BUFLIB_H_
#define _BUFLIB_H_

#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Add extra checks to buflib_get_data to catch bad handles */
//#define BUFLIB_DEBUG_GET_DATA

/* Support integrity check */
//#define BUFLIB_DEBUG_CHECK_VALID

/* Support debug printing of memory blocks */
//#define BUFLIB_DEBUG_PRINT

/* Defined by the backend header. */
struct buflib_context;

/* Buflib callback return codes. */
#define BUFLIB_CB_OK            0
#define BUFLIB_CB_CANNOT_MOVE   1
#define BUFLIB_CB_CANNOT_SHRINK 1

/* Buflib shrink hints. */
#define BUFLIB_SHRINK_SIZE_MASK (~BUFLIB_SHRINK_POS_MASK)
#define BUFLIB_SHRINK_POS_FRONT (1u<<31)
#define BUFLIB_SHRINK_POS_BACK  (1u<<30)
#define BUFLIB_SHRINK_POS_MASK  (BUFLIB_SHRINK_POS_FRONT|BUFLIB_SHRINK_POS_BACK)

/**
 * Callbacks run by buflib to manage an allocation.
 */
struct buflib_callbacks
{
    /**
     * \brief Called when buflib wants to move the buffer
     * \param handle    Handle being moved
     * \param current   Current address of the buffer
     * \param new       New address the buffer would have after moving
     * \return BUFLIB_CB_OK - Allow the buffer to be moved.
     * \return BUFLIB_CB_CANNOT_MOVE - Do not allow the buffer to be moved.
     *
     * This callback allows you to fix up any pointers that might
     * be pointing to the buffer before it is moved. The task of
     * actually moving the buffer contents is performed by buflib
     * after the move callback returns, if movement is allowed.
     *
     * Care must be taken to ensure that the buffer is not accessed
     * from outside the move callback until the move is complete. If
     * this is a concern, eg. due to multi-threaded access, then you
     * must implement a sync_callback() and guard any access to the
     * buffer with a lock.
     *
     * If the move callback is NULL then buflib will never move
     * the allocation, as if you returned BUFLIB_CB_CANNOT_MOVE.
     */
    int (*move_callback)(int handle, void* current, void* new);

    /**
     * \brief Called when buflib wants to shrink the buffer
     * \param handle    Handle to shrink
     * \param hints     Hints regarding the shrink request
     * \param start     Current address of the buffer
     * \param size      Current size of the buffer as seen by buflib.
     *                  This may be rounded up compared to the nominal
     *                  allocation size due to alignment requirements.
     * \return BUFLIB_CB_OK - Was able to shrink the buffer.
     * \return BUFLIB_CB_CANNOT_SHRINK - Buffer cannot shrink.
     *
     * This callback is run by buflib when it runs out of memory
     * and starts a compaction run. Buflib will not actually try
     * to shrink or move memory, you must do that yourself and
     * call buflib_shrink() to report the new start address and
     * size of the buffer.
     *
     * If the shrink callback is NULL then buflib will regard the
     * buffer as non-shrinkable.
     */
    int (*shrink_callback)(int handle, unsigned hints,
                           void *start, size_t size);

    /**
     * \brief Called before and after attempting to move the buffer
     * \param handle    Handle being moved
     * \param lock      True to lock, false to unlock
     *
     * The purpose of this callback is to block access to the buffer
     * from other threads while a buffer is being moved, using a lock
     * such as a mutex.
     *
     * It is called with `sync_callback(handle, true)` before running
     * the move callback and `sync_callback(handle, false)` after the
     * move is complete, regardless of whether the buffer was actually
     * moved or not.
     */
    void (*sync_callback)(int handle, bool lock);
};

/**
 * A set of all NULL callbacks for use with allocations that need to stay
 * locked in RAM and not moved or shrunk. These type of allocations should
 * be avoided as much as possible to avoid memory fragmentation but it can
 * suitable for short-lived allocations.
 *
 * \note Use of this is discouraged. Prefer to use normal moveable
 *       allocations and pin them.
 */
extern struct buflib_callbacks buflib_ops_locked;

/**
 * \brief Intialize a buflib context
 * \param ctx       Context to initialize
 * \param buf       Buffer which will be used as the context's memory pool
 * \param size      Size of the buffer
 */
void buflib_init(struct buflib_context *ctx, void *buf, size_t size);

/**
 * Returns the amount of unallocated bytes. It does not mean this amount
 * can be actually allocated because they might not be contiguous.
 */
size_t buflib_available(struct buflib_context *ctx);

/**
 * Returns the size of the largest possible contiguous allocation, given
 * the current state of the memory pool. A larger allocation may still
 * succeed if compaction is able to create a larger contiguous area.
 */
size_t buflib_allocatable(struct buflib_context *ctx);

/**
 * \brief Relocate the buflib memory pool to a new address
 * \param ctx       Context to relocate
 * \param buf       New memory pool address
 * \return True if relocation should proceed, false if it cannot.
 *
 * Updates all pointers inside the buflib context to point to a new pool
 * address. You must call this function before moving the pool and move
 * the data manually afterwards only if this function returns true.
 *
 * This is intended from a move_callback() in buflib-on-buflib scenarios,
 * where the memory pool of the "inner" buflib is allocated from an "outer"
 * buflib.
 *
 * \warning This does not run any move callbacks, so it is not safe to
 *          use if any allocations require them.
 */
bool buflib_context_relocate(struct buflib_context *ctx, void *buf);

/**
 * \brief Allocate memory from buflib
 * \param ctx       Context to allocate from
 * \param size      Allocation size
 * \return Handle for the allocation (> 0) or a negative value on error
 *
 * This is the same as calling buflib_alloc_ex() with a NULL callbacks
 * struct. The resulting allocation can be moved by buflib; use pinning
 * if you need to prevent moves.
 *
 * Note that zero is not a valid handle, and will never be returned by
 * this function. However, this may change, and you should treat a zero
 * or negative return value as an allocation failure.
 */
int buflib_alloc(struct buflib_context *ctx, size_t size);

/**
 * \brief Allocate memory from buflib with custom buffer ops
 * \param ctx       Context to allocate from
 * \param size      Allocation size
 * \param ops       Pointer to ops struct or NULL if no ops are needed.
 * \return Handle for the allocation (> 0) or a negative value on error.
 *
 * Use this if you need to pass custom callbacks for responding to buflib
 * move or shrink operations. Passing a NULL ops pointer means the buffer
 * can be moved by buflib at any time.
 *
 * Note that zero is not a valid handle, and will never be returned by
 * this function. However, this may change, and you should treat a zero
 * or negative return value as an allocation failure.
 */
int buflib_alloc_ex(struct buflib_context *ctx, size_t size,
                    struct buflib_callbacks *ops);

/**
 * \brief Attempt a maximum size allocation
 * \param ctx       Context to allocate from
 * \param size      Size of the allocation will be written here on success.
 * \param ops       Pointer to ops struct or NULL if no ops are needed.
 * \return Handle for the allocation (> 0) or a negative value on error.
 *
 * Buflib will attempt to compact and shrink other allocations as much as
 * possible and then allocate the largest contigous free area. Since this
 * will consume effectively *all* available memory, future allocations are
 * likely to fail.
 *
 * \note There is rarely any justification to use this with the core_alloc
 *       context due to the impact it has on the entire system. You should
 *       change your code if you think you need this. Of course, if you are
 *       using a private buflib context then this warning does not apply.
 */
int buflib_alloc_maximum(struct buflib_context *ctx,
                         size_t *size, struct buflib_callbacks *ops);

/**
 * \brief Reduce the size of a buflib allocation
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 * \param newstart  New start address. Must be within the current bounds
 *                  of the allocation, as returned by buflib_get_data().
 * \param new_size  New size of the buffer.
 * \return True if shrinking was successful; otherwise, returns false and
 *         does not modify the allocation.
 *
 * Shrinking always succeeds provided the new allocation is contained
 * within the current allocation. A failure is always a programming
 * error, so you need not check for it and in the future the failure
 * case may be changed to a panic or undefined behavior with no return
 * code.
 *
 * The new start address and size need not have any particular alignment,
 * however buflib cannot work with unaligned addresses so there is rarely
 * any purpose to creating unaligned allocations.
 *
 * Shrinking is typically done from a shrink_callback(), but can be done
 * at any time if you want to reduce the size of a buflib allocation.
 */
bool buflib_shrink(struct buflib_context *ctx, int handle,
                   void *newstart, size_t new_size);

/**
 * \brief Increment an allocation's pin count
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 *
 * The pin count acts like a reference count. Buflib will not attempt to
 * move any buffer with a positive pin count, nor invoke any move or sync
 * callbacks. Hence, when pinned, it is safe to hold pointers to a buffer
 * across yields or use them for I/O.
 *
 * Note that shrink callbacks can still be invoked for pinned handles.
 */
void buflib_pin(struct buflib_context *ctx, int handle);

/**
 * \brief Decrement an allocation's pin count
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 */
void buflib_unpin(struct buflib_context *ctx, int handle);

/**
 * \brief Return the pin count of an allocation
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 * \return Current pin count; zero means the handle is not pinned.
 */
unsigned buflib_pin_count(struct buflib_context *ctx, int handle);

/**
 * \brief Free an allocation and return its memory to the pool
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 * \return Always returns zero (zero is not a valid handle, so this can
 *         be used to invalidate the variable containing the handle).
 */
int buflib_free(struct buflib_context *context, int handle);

/**
 * \brief Get a pointer to the buffer for an allocation
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 * \return Pointer to the allocation's memory.
 *
 * Note that buflib can move allocations in order to free up space when
 * making new allocations. For this reason, it's unsafe to hold a pointer
 * to a buffer across a yield() or any other operation that can cause a
 * context switch. This includes any function that may block, and even
 * some functions that might not block -- eg. if a low priority thread
 * acquires a mutex, calling mutex_unlock() may trigger a context switch
 * to a higher-priority thread.
 *
 * buflib_get_data() is a very cheap operation, however, costing only
 * a few pointer lookups. Don't hesitate to use it extensively.
 *
 * If you need to hold a pointer across a possible context switch, pin
 * the handle with buflib_pin() to prevent the buffer from being moved.
 * This is required when doing I/O into buflib allocations, for example.
 */
#ifdef BUFLIB_DEBUG_GET_DATA
void *buflib_get_data(struct buflib_context *ctx, int handle);
#else
static inline void *buflib_get_data(struct buflib_context *ctx, int handle);
#endif

/**
 * \brief Get a pinned pointer to a buflib allocation
 * \param ctx       Buflib context of the allocation
 * \param handle    Handle identifying the allocation
 * \return Pointer to the allocation's memory.
 *
 * Functionally equivalent to buflib_pin() followed by buflib_get_data(),
 * but this call is more efficient and should be preferred over separate
 * calls.
 *
 * To unpin the data, call buflib_put_data_pinned() and pass the pointer
 * returned by this function.
 */
static inline void *buflib_get_data_pinned(struct buflib_context *ctx, int handle);

/**
 * \brief Release a pinned pointer to a buflib allocation
 * \param ctx       Buflib context of the allocation
 * \param data      Pointer returned by buflib_get_data()
 *
 * Decrements the pin count, allowing the buffer to be moved once the
 * pin count drops to zero. This is more efficient than buflib_unpin()
 * and should be preferred when you have a pointer to the buflib data.
 */
static inline void buflib_put_data_pinned(struct buflib_context *ctx, void *data);

/**
 * \brief Shift allocations up to free space at the start of the pool
 * \param ctx       Context to operate on
 * \param size      Indicates number of bytes to free up, or 0 to free
 *                  up as much as possible. On return, the actual number
 *                  of bytes freed is written here.
 * \return Pointer to the start of the free area
 *
 * If `*size` is non-zero, the actual amount of space freed up might
 * be less than `*size`.
 *
 * \warning This will move data around in the pool without calling any
 *          move callbacks!
 * \warning This function is deprecated and will eventually be removed.
 */
void* buflib_buffer_out(struct buflib_context *ctx, size_t *size);

/**
 * \brief Shift allocations down into free space below the pool
 * \param ctx       Context to operate on
 * \param size      Number of bytes to add to the pool.
 *
 * This operation should only be used to return memory that was previously
 * taken from the pool with buflib_buffer_out(), by passing the same size
 * that you got from that function.
 *
 * \warning This will move data around in the pool without calling any
 *          move callbacks!
 * \warning This function is deprecated and will eventually be removed.
 */
void buflib_buffer_in(struct buflib_context *ctx, int size);

#ifdef BUFLIB_DEBUG_PRINT
/**
 * Return the number of blocks in the buffer, allocated or unallocated.
 *
 * Only available if BUFLIB_DEBUG_PRINT is defined.
 */
int buflib_get_num_blocks(struct buflib_context *ctx);

/**
 * Write a string describing the block at index block_num to the
 * provided buffer. The buffer will always be null terminated and
 * there is no provision to detect truncation. (A 40-byte buffer
 * is enough to contain any returned string.)
 *
 * Returns false if the block index is out of bounds, and writes
 * an empty string.
 *
 * Only available if BUFLIB_DEBUG_PRINT is defined.
 */
bool buflib_print_block_at(struct buflib_context *ctx, int block_num,
                           char *buf, size_t bufsize);
#endif

#ifdef BUFLIB_DEBUG_CHECK_VALID
/**
 * Check integrity of given buflib context
 */
void buflib_check_valid(struct buflib_context *ctx);
#endif

#if CONFIG_BUFLIB_BACKEND == BUFLIB_BACKEND_MEMPOOL
#include "buflib_mempool.h"
#elif CONFIG_BUFLIB_BACKEND == BUFLIB_BACKEND_MALLOC
#include "buflib_malloc.h"
#endif

#ifndef BUFLIB_ALLOC_OVERHEAD
# define BUFLIB_ALLOC_OVERHEAD 0
#endif

#endif /* _BUFLIB_H_ */
