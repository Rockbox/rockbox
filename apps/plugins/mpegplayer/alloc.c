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

long mem_ptr;
long bufsize;
unsigned char* mallocbuf;

void mpeg2_alloc_init(unsigned char* buf, int mallocsize)
{
    mem_ptr = 0;
    bufsize = mallocsize;
    mallocbuf = buf;
  
    return;
}

void * mpeg2_malloc (unsigned size, mpeg2_alloc_t reason)
{
    void* x;

    (void)reason;

    DEBUGF("mpeg2_malloc(%d,%d)\n",size,reason);
    if (mem_ptr + (long)size > bufsize) {
        DEBUGF("OUT OF MEMORY\n");
        return NULL;
    }
    
    x=&mallocbuf[mem_ptr];
    mem_ptr+=(size+3)&~3; /* Keep memory 32-bit aligned */

    return(x);
}

void mpeg2_free(void* ptr) {
    (void)ptr;
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


/* The following are expected by libmad */
void* codec_malloc(size_t size)
{
      return mpeg2_malloc(size,-3);
}

void* codec_calloc(size_t nmemb, size_t size)
{
    void* ptr;

    ptr = mpeg2_malloc(nmemb*size,-3);

    if (ptr)
        rb->memset(ptr,0,size);
    
    return ptr;
}

void codec_free(void* ptr) {
    (void)ptr;
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
