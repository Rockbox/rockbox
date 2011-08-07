
/*
   Based on the wiki viewer mallocer 
   Copyright (C) 2005 Dave Chapman
   
   @ Modified to decompress memory buffer by gama
 */

#include "mallocer.h"
#include "codeclib.h"

unsigned char* mallocbuffer[MEMPOOL_MAX];
long memory_ptr[MEMPOOL_MAX];
size_t buffersize[MEMPOOL_MAX];

int wpw_init_mempool(unsigned char mempool)
{
    memory_ptr[mempool] = 0;
    mallocbuffer[mempool] = (unsigned char *)ci->codec_get_buffer(&buffersize[mempool]);
    // memset(mallocbuf[mempool], 0, bufsize[mempool]);
    return 0;
}

int wpw_init_mempool_pdm(unsigned char mempool,
                         unsigned char* mem,long memsize)
{
    memory_ptr[mempool] = 0;
    mallocbuffer[mempool] = mem;
    buffersize[mempool]=memsize;
    return 0;
}

void wpw_reset_mempool(unsigned char mempool)
{
    memory_ptr[mempool]=0;
}

void wpw_destroy_mempool(unsigned char mempool)
{
    memory_ptr[mempool] = 0;
    mallocbuffer[mempool] =0;
    buffersize[mempool]=0;
}

long wpw_available(unsigned char mempool)
{
    return buffersize[mempool]-memory_ptr[mempool];
}

void* wpw_malloc(unsigned char mempool,size_t size)
{
    void* x;

    if (memory_ptr[mempool] + size > buffersize[mempool] )
        return NULL;

    x=&mallocbuffer[mempool][memory_ptr[mempool]];
    memory_ptr[mempool]+=(size+3)&~3; /* Keep memory 32-bit aligned */

    return(x);
}

void* wpw_calloc(unsigned char mempool,size_t nmemb, size_t size)
{
    void* x;
    x = wpw_malloc(mempool,nmemb*size);
    if (x == NULL)
        return NULL;

    memset(x,0,nmemb*size);
    return(x);
}

void wpw_free(unsigned char mempool,void* ptr)
{
    (void)ptr;
    (void)mempool;
}

void* wpw_realloc(unsigned char mempool,void* ptr, size_t size)
{
    void* x;
    (void)ptr;
    x = wpw_malloc(mempool,size);
    return(x);
}
