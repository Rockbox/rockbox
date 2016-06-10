/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Frederik M.J. Vestre
 * Copyright (C) 2006 Adam Gashlin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*
   Based on the malloc in codeclib Copyright (C) 2005 Dave Chapman
 */

#include "mallocer.h"

unsigned char* mallocbuf[MEMPOOL_MAX];
long mem_ptr[MEMPOOL_MAX];
size_t bufsize[MEMPOOL_MAX];

#include "plugin.h"

int wpw_init_mempool(unsigned char mempool)
{
    mem_ptr[mempool] = 0;
    mallocbuf[mempool] = (unsigned char *)rb->plugin_get_buffer(&bufsize[mempool]);
    return 0;
}

int wpw_init_mempool_pdm(unsigned char mempool,
                         unsigned char* mem,long memsize)
{
    mem_ptr[mempool] = 0;
    mallocbuf[mempool] = mem;
    bufsize[mempool]=memsize;
    return 0;
}

void wpw_reset_mempool(unsigned char mempool)
{
    mem_ptr[mempool]=0;
}

void wpw_destroy_mempool(unsigned char mempool)
{
    mem_ptr[mempool] = 0;
    mallocbuf[mempool] =0;
    bufsize[mempool]=0;
}

long wpw_available(unsigned char mempool)
{
    return bufsize[mempool]-mem_ptr[mempool];
}

void* wpw_malloc(unsigned char mempool,size_t size)
{
    void* x;

    if (mem_ptr[mempool] + size > bufsize[mempool] )
        return NULL;

    x=&mallocbuf[mempool][mem_ptr[mempool]];
    mem_ptr[mempool]+=(size+3)&~3; /* Keep memory 32-bit aligned */

    return(x);
}

void* wpw_calloc(unsigned char mempool,size_t nmemb, size_t size)
{
    void* x;
    x = wpw_malloc(mempool,nmemb*size);
    if (x == NULL)
        return NULL;

    rb->memset(x,0,nmemb*size);
    return(x);
}

void codec_free(unsigned char mempool,void* ptr)
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
