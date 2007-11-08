#include <os_types.h>

static unsigned char *mallocbuf;
static size_t bufsize, tmp_ptr, mem_ptr;

void ogg_malloc_init(void)
{
    mallocbuf = ci->get_codec_memory(&bufsize);
    tmp_ptr = bufsize & ~3;
    mem_ptr = 0;
}

void *ogg_malloc(size_t size)
{
    void* x;

    size = (size + 3) & ~3;

    if (mem_ptr + size > tmp_ptr)
        return NULL;
    
    x = &mallocbuf[mem_ptr];
    mem_ptr += size; /* Keep memory 32-bit aligned */

    return x;
}

void *ogg_tmpmalloc(size_t size)
{
    size = (size + 3) & ~3;

    if (mem_ptr + size > tmp_ptr)
        return NULL;
    
    tmp_ptr -= size;
    return &mallocbuf[tmp_ptr];
}

void *ogg_calloc(size_t nmemb, size_t size)
{
    void *x;
    x = ogg_malloc(nmemb * size);
    if (x == NULL)
        return NULL;
    ci->memset(x, 0, nmemb * size);
    return x;
}

void *ogg_tmpcalloc(size_t nmemb, size_t size)
{
    void *x;
    x = ogg_tmpmalloc(nmemb * size);
    if (x == NULL)
        return NULL;
    ci->memset(x, 0, nmemb * size);
    return x;
}

void *ogg_realloc(void *ptr, size_t size)
{
    void *x;
    (void)ptr;
    x = ogg_malloc(size);
    return x;
}

long ogg_tmpmalloc_pos(void)
{
    return tmp_ptr;
}

void ogg_tmpmalloc_free(long pos)
{
    tmp_ptr = pos;
}
