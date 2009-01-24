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
 *
 * $Id$
 * libmpeg2 sync history:
 * 2008-07-01 - CVS revision 1.13
 */

#include "plugin.h"
#include "mpegplayer.h"
#include <system.h>

/* Main allocator */
static off_t mem_ptr;
static size_t bufsize;
static unsigned char* mallocbuf;

/* libmpeg2 allocator */
static off_t mpeg2_mem_ptr SHAREDBSS_ATTR;
static size_t mpeg2_bufsize SHAREDBSS_ATTR;
static unsigned char *mpeg2_mallocbuf SHAREDBSS_ATTR;
static unsigned char *mpeg2_bufallocbuf SHAREDBSS_ATTR;

#if defined(DEBUG) || defined(SIMULATOR)
const char * mpeg_get_reason_str(int reason)
{
    const char *str;

    switch (reason)
    {
    case MPEG2_ALLOC_MPEG2DEC:
        str = "MPEG2_ALLOC_MPEG2DEC";
        break;
    case MPEG2_ALLOC_CHUNK:
        str = "MPEG2_ALLOC_CHUNK";
        break;
    case MPEG2_ALLOC_YUV:
        str = "MPEG2_ALLOC_YUV";
        break;
    case MPEG2_ALLOC_CONVERT_ID:
        str = "MPEG2_ALLOC_CONVERT_ID";
        break;
    case MPEG2_ALLOC_CONVERTED:
        str = "MPEG2_ALLOC_CONVERTED";
        break;
    case MPEG_ALLOC_MPEG2_BUFFER:
        str = "MPEG_ALLOC_MPEG2_BUFFER";
        break;
    case MPEG_ALLOC_AUDIOBUF:
        str = "MPEG_ALLOC_AUDIOBUF";
        break;
    case MPEG_ALLOC_PCMOUT:
        str = "MPEG_ALLOC_PCMOUT";
        break;
    case MPEG_ALLOC_DISKBUF:
        str = "MPEG_ALLOC_DISKBUF";
        break;
    case MPEG_ALLOC_CODEC_MALLOC:
        str = "MPEG_ALLOC_CODEC_MALLOC";
        break;
    case MPEG_ALLOC_CODEC_CALLOC:
        str = "MPEG_ALLOC_CODEC_CALLOC";
        break;
    default:
        str = "Unknown";
    }

    return str;        
}
#endif

static void * mpeg_malloc_internal (unsigned char *mallocbuf,
                                    off_t *mem_ptr,
                                    size_t bufsize,
                                    unsigned size,
                                    int reason)
{
    void *x;

    DEBUGF("mpeg_alloc_internal: bs:%lu s:%u reason:%s (%d)\n",
           bufsize, size, mpeg_get_reason_str(reason), reason);

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

void *mpeg_malloc_all(size_t *size_out, mpeg2_alloc_t reason)
{
    /* Can steal all but MIN_MEMMARGIN */
    if (bufsize - mem_ptr < MIN_MEMMARGIN)
        return NULL;

    *size_out = bufsize - mem_ptr - MIN_MEMMARGIN;
    return mpeg_malloc(*size_out, reason);
}

bool mpeg_alloc_init(unsigned char *buf, size_t mallocsize)
{
    mem_ptr = 0;
    /* Cache-align buffer or 4-byte align */
    mallocbuf = buf;
    bufsize = mallocsize;
    ALIGN_BUFFER(mallocbuf, bufsize, CACHEALIGN_UP(4));

    /* Separate allocator for video */
    mpeg2_mem_ptr = 0;
    mpeg2_mallocbuf = mallocbuf;
    mpeg2_bufallocbuf = mallocbuf;
    mpeg2_bufsize = CACHEALIGN_UP(LIBMPEG2_ALLOC_SIZE);

    if (mpeg_malloc_internal(mallocbuf, &mem_ptr,
                             bufsize, mpeg2_bufsize,
                             MPEG_ALLOC_MPEG2_BUFFER) == NULL)
    {
        return false;
    }

    IF_COP(invalidate_icache());
    return true;
}

/* allocate non-dedicated buffer space which mpeg2_mem_reset will free */
void * mpeg2_malloc(unsigned size, mpeg2_alloc_t reason)
{
    void *ptr = mpeg_malloc_internal(mpeg2_mallocbuf, &mpeg2_mem_ptr,
                                     mpeg2_bufsize, size, reason);
    /* libmpeg2 expects zero-initialized allocations */
    if (ptr)
        rb->memset(ptr, 0, size);

    return ptr;
}

/* allocate dedicated buffer - memory behind buffer pointer becomes dedicated
   so order is important */
void * mpeg2_bufalloc(unsigned size, mpeg2_alloc_t reason)
{
    void *buf = mpeg2_malloc(size, reason);

    if (buf == NULL)
        return NULL;

    mpeg2_bufallocbuf = &mpeg2_mallocbuf[mpeg2_mem_ptr];
    return buf;
}

/* return unused buffer portion and size */
void * mpeg2_get_buf(size_t *size)
{
    if ((size_t)mpeg2_mem_ptr + 32 >= mpeg2_bufsize)
        return NULL;

    *size = mpeg2_bufsize - mpeg2_mem_ptr;
    return &mpeg2_mallocbuf[mpeg2_mem_ptr];
}

/* de-allocate all non-dedicated buffer space */
void mpeg2_mem_reset(void)
{
    DEBUGF("mpeg2_mem_reset\n");
    mpeg2_mem_ptr = mpeg2_bufallocbuf - mpeg2_mallocbuf;
}

/* The following are expected by libmad */
void * codec_malloc(size_t size)
{
    void* ptr;

    ptr = mpeg_malloc_internal(mallocbuf, &mem_ptr,
                                bufsize, size, MPEG_ALLOC_CODEC_MALLOC);

    if (ptr)
        rb->memset(ptr,0,size);

    return ptr;
}

void * codec_calloc(size_t nmemb, size_t size)
{
    void* ptr;

    ptr = mpeg_malloc_internal(mallocbuf, &mem_ptr,
                               bufsize, nmemb*size,
                               MPEG_ALLOC_CODEC_CALLOC);

    if (ptr)
        rb->memset(ptr,0,size);

    return ptr;
}

void codec_free(void* ptr)
{
    DEBUGF("codec_free - %p\n", ptr);
#if 0
    mem_ptr = (void *)ptr - (void *)mallocbuf;
#endif
    (void)ptr;
}

