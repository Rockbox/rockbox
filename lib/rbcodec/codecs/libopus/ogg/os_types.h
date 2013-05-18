#ifndef OS_TYPES_H
#define OS_TYPES_H
#include "codeclib.h"
#include <stdint.h>
#include <tlsf.h>

static inline void ogg_malloc_init(void)
{
    size_t bufsize;
    void* buf = ci->codec_get_buffer(&bufsize);
    init_memory_pool(bufsize, buf);
}

static inline void ogg_malloc_destroy(void)
{
    size_t bufsize;
    void* buf = ci->codec_get_buffer(&bufsize);
    destroy_memory_pool(buf);
}

static inline void *_ogg_malloc(size_t size)
{
    void* x = tlsf_malloc(size);
    DEBUGF("ogg_malloc %zu = %p\n", size, x);
    return x;
}

static inline void *_ogg_calloc(size_t nmemb, size_t size)
{
    void *x = tlsf_calloc(nmemb, size);
    DEBUGF("ogg_calloc %zu %zu\n", nmemb, size);
    return x;
}

static inline void *_ogg_realloc(void *ptr, size_t size)
{
    void *x = tlsf_realloc(ptr, size);
    DEBUGF("ogg_realloc %p %zu = %p\n", ptr, size, x);
    return x;
}

static inline void _ogg_free(void* ptr)
{
    DEBUGF("ogg_free %p\n", ptr);
    tlsf_free(ptr);
}

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;
#endif /* OS_TYPES_H */

