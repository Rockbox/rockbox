
#ifndef __CORE_ALLOC_H__
#define __CORE_ALLOC_H__
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "buflib.h"

/* All functions below are wrappers for functions in buflib.h, except
 * they have a predefined context
 */
void core_allocator_init(void) INIT_ATTR;
int core_alloc(const char* name, size_t size);
int core_alloc_ex(const char* name, size_t size, struct buflib_callbacks *ops);
int core_alloc_maximum(const char* name, size_t *size, struct buflib_callbacks *ops);
bool core_shrink(int handle, void* new_start, size_t new_size);
int core_free(int handle);
size_t core_available(void);
size_t core_allocatable(void);
const char* core_get_name(int handle);
#ifdef DEBUG
void core_check_valid(void);
#endif

/* DO NOT ADD wrappers for buflib_buffer_out/in. They do not call
 * the move callbacks and are therefore unsafe in the core */

#ifdef BUFLIB_DEBUG_BLOCKS
void core_print_allocs(void (*print)(const char*));
void core_print_blocks(void (*print)(const char*));
#endif
#ifdef BUFLIB_DEBUG_BLOCK_SINGLE
int  core_get_num_blocks(void);
void core_print_block_at(int block_num, char* buf, size_t bufsize);
#endif

/* frees the debug test alloc created at initialization,
 * since this is the first any further alloc should force a compaction run */
bool core_test_free(void);

static inline void* core_get_data(int handle)
{
    extern struct buflib_context core_ctx;
    return buflib_get_data(&core_ctx, handle);
}

#endif /* __CORE_ALLOC_H__ */
