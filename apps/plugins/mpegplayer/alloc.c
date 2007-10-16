/*
 * alloc.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "plugin.h"

#include "mpeg2.h"

extern struct plugin_api* rb;

/* Main allocator */
static off_t mem_ptr;
static size_t bufsize;
static unsigned char* mallocbuf;

/* libmpeg2 allocator */
static off_t mpeg2_mem_ptr;
static size_t mpeg2_bufsize;
static unsigned char *mpeg2_mallocbuf;

static void * mpeg_malloc_internal (unsigned char *mallocbuf,
                                    off_t *mem_ptr,
                                    size_t bufsize,
                                    unsigned size,
                                    int reason)
{
    void *x;

    if ((size_t) (*mem_ptr + size) > bufsize)
    {
        DEBUGF("OUT OF MEMORY\n");
        return NULL;
    }
    
    x = &mallocbuf[*mem_ptr];
    *mem_ptr += (size + 3) & ~3; /* Keep memory 32-bit aligned */

    return x;
    (void)reason;
}

void *mpeg_malloc(size_t size, mpeg2_alloc_t reason)
{
    return mpeg_malloc_internal(mallocbuf, &mem_ptr, bufsize, size,
                                reason);
}

size_t mpeg_alloc_init(unsigned char *buf, size_t mallocsize,
                       size_t libmpeg2size)
{
    mem_ptr = 0;
    bufsize = mallocsize;
    /* Line-align buffer */
    mallocbuf = (char *)(((intptr_t)buf + 15) & ~15);
    /* Adjust for real size */
    bufsize -= mallocbuf - buf;

    /* Separate allocator for video */
    libmpeg2size = (libmpeg2size + 15) & ~15;
    if (mpeg_malloc_internal(mallocbuf, &mem_ptr,
                             bufsize, libmpeg2size, 0) == NULL)
    {
        return 0;
    }

    mpeg2_mallocbuf = mallocbuf;
    mpeg2_mem_ptr = 0;
    mpeg2_bufsize = libmpeg2size;

#if NUM_CORES > 1
    flush_icache();
#endif

    return bufsize - mpeg2_bufsize;
}

/* gcc may want to use memcpy before rb is initialised, so here's a trivial 
   implementation */

void *memcpy(void *dest, const void *src, size_t n) {
    size_t i;
    char* d=(char*)dest;
    char* s=(char*)src;

    for (i=0;i<n;i++)
        d[i]=s[i];

    return dest;
}

void * mpeg2_malloc(unsigned size, mpeg2_alloc_t reason)
{
    return mpeg_malloc_internal(mpeg2_mallocbuf, &mpeg2_mem_ptr,
                                mpeg2_bufsize, size, reason);
}

void mpeg2_free(void *ptr)
{
    mpeg2_mem_ptr = (void *)ptr - (void *)mpeg2_mallocbuf;
}

/* The following are expected by libmad */
void * codec_malloc(size_t size)
{
    return mpeg_malloc_internal(mallocbuf, &mem_ptr,
                                bufsize, size, -3);
}

void * codec_calloc(size_t nmemb, size_t size)
{
    void* ptr;

    ptr = mpeg_malloc_internal(mallocbuf, &mem_ptr,
                               bufsize, nmemb*size, -3);

    if (ptr)
        rb->memset(ptr,0,size);
    
    return ptr;
}

void codec_free(void* ptr)
{
    mem_ptr = (void *)ptr - (void *)mallocbuf;
}

void *memmove(void *dest, const void *src, size_t n)
{
    return rb->memmove(dest,src,n);
}

void *memset(void *s, int c, size_t n)
{
    return rb->memset(s,c,n);
}

void abort(void)
{
    rb->lcd_putsxy(0,0,"ABORT!");
    rb->lcd_update();

    while (1);
    /* Let's hope this is never called */
}
