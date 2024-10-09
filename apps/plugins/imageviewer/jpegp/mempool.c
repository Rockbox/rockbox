#include <inttypes.h>
#include "plugin.h"

static unsigned char *mem_pool;
static unsigned char *mem_pool_start;
static size_t memory_size;

extern void *malloc(size_t size)
{
    if (size > memory_size)
        return NULL;

    memory_size -= size;
    unsigned char* ptr = mem_pool;

    mem_pool+= size;
    return ptr;
}

extern void *calloc(size_t nelem, size_t elem_size)
{
    unsigned char* ptr = malloc(nelem*elem_size);
    if (!ptr)
        return  NULL;
    rb->memset(ptr, 0, nelem*elem_size);
    return ptr;
}

extern void init_mem_pool(const unsigned char *buf, const ssize_t buf_size)
{
    //TODO: do we need this alignment? (copied from gif lib)
    unsigned char *memory_max;

    /* align buffer */
    mem_pool_start = mem_pool = (unsigned char *)((intptr_t)(buf + 3) & ~3);
    memory_max = (unsigned char *)((intptr_t)(mem_pool + buf_size) & ~3);
    memory_size = memory_max - mem_pool;
}

extern ssize_t freeze_mem_pool(void)
{
    mem_pool_start = mem_pool;
    return memory_size;
}

extern void clear_mem_pool(void)
{
    memory_size += mem_pool - mem_pool_start;
    mem_pool = mem_pool_start;
}