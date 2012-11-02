#include "plugin.h"
#include "dumb_alloc.h"
#include "stdlib.h"

struct alloc_t {
    void *head;
    size_t size;
};

static struct alloc_t ctx = {NULL,0};

void init_memory_pool(void *pool, size_t size)
{
    ctx.head = pool;
    ctx.size = size;
}

void *malloc(size_t size)
{
    void *ptr = NULL;
    size = (size + 3) & ~3;

   if (ctx.size >= size)
   {
       ptr = ctx.head;
       ctx.head = (char *)ptr + size;
       ctx.size -= size;
   }

   return ptr;
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr;
    ptr = malloc(nmemb*size);

    if (ptr)
        rb->memset(ptr, 0, nmemb*size);

    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    (void)ptr;
    return malloc(size);
}

void free(void *ptr)
{
    (void)ptr;
    return;
}
