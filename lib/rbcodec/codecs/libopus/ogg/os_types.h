#include "config.h"
#include <stdint.h>
#include "codeclib.h"

#ifdef SIMULATOR

#include <stdio.h>

static inline void* _ogg_malloc(size_t size)
{
    void *buf;

    printf("ogg_malloc %d", size);
    buf = codec_malloc(size);
    printf(" = %p\n", buf);
    
    return buf;
}

static inline void* _ogg_calloc(size_t nmemb, size_t size)
{
    printf("ogg_calloc %d %d\n", nmemb, size);
    return codec_calloc(nmemb, size);
}

static inline void* _ogg_realloc(void *ptr, size_t size)
{
    void *buf;

    printf("ogg_realloc %p %d", ptr, size);
    buf = codec_realloc(ptr, size);
    printf(" = %p\n", buf);
    return buf;
}

static inline void _ogg_free(void *ptr)
{
    printf("ogg_free %p\n", ptr);
    codec_free(ptr);
}

#else

#define _ogg_malloc  codec_malloc
#define _ogg_calloc  codec_calloc
#define _ogg_realloc codec_realloc
#define _ogg_free    codec_free

#endif

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;

