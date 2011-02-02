/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include <stdio.h>
#include "buffer.h"
#include "panic.h"
#include "logf.h"

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
unsigned char audiobuffer[(MEMORYSIZE*1024-256)*1024];
unsigned char *audiobufend = audiobuffer + sizeof(audiobuffer);
#else
/* defined in linker script */
extern unsigned char audiobuffer[];
#endif

unsigned char *audiobuf;

#ifdef BUFFER_ALLOC_DEBUG
static unsigned char *audiobuf_orig_start;

struct buffer_start_marker
{
    unsigned int magic;
    size_t buffer_size;
};
#define BUF_MAGIC 0xDEADD0D0

struct buffer_end_marker
{
    unsigned int magic;
    int last;
};
#endif /* BUFFER_ALLOC_DEBUG */

void buffer_init(void)
{
    /* 32-bit aligned */
    audiobuf = (void *)(((unsigned long)audiobuffer + 3) & ~3);
#ifdef BUFFER_ALLOC_DEBUG
    audiobuf_orig_start = audiobuf;
#endif /* BUFFER_ALLOC_DEBUG */
}

void *buffer_alloc(size_t size)
{
    void *retval = audiobuf;
#ifdef BUFFER_ALLOC_DEBUG
    struct buffer_start_marker *start;
    struct buffer_end_marker *end;
#endif /* BUFFER_ALLOC_DEBUG */

    /* 32-bit aligned */
    size = (size + 3) & ~3;

#ifdef BUFFER_ALLOC_DEBUG
    retval +=sizeof(struct buffer_start_marker);
    if(size>0)
    {
        end=(struct buffer_end_marker*)(audiobuf - sizeof(struct buffer_end_marker));
        if(end->magic == BUF_MAGIC)
        {
            end->last=0;
        }
        start=(struct buffer_start_marker*)audiobuf;
        start->magic = BUF_MAGIC;
        start->buffer_size = size;
        end=(struct buffer_end_marker*)(audiobuf+sizeof(struct buffer_start_marker)+size);
        end->magic = BUF_MAGIC;
        end->last = 1;

        audiobuf = ((unsigned char *)end) + sizeof(struct buffer_end_marker);
    }

    logf("Alloc %x %d",(unsigned int)retval,size);
#else /* !BUFFER_ALLOC_DEBUG */
    audiobuf += size;
#endif /* BUFFER_ALLOC_DEBUG */

    if (audiobuf > audiobufend) {
        panicf("OOM: %d bytes", (int) size);
    }
    
    return retval;
}

#ifdef BUFFER_ALLOC_DEBUG
void buffer_alloc_check(char *name)
{
    unsigned char *buf_ptr = audiobuf_orig_start;
    struct buffer_start_marker *start;
    struct buffer_end_marker *end;


    while(buf_ptr < audiobuf)
    {
        start=(struct buffer_start_marker*)buf_ptr;
        if(start->magic != BUF_MAGIC)
        {
            panicf("%s corrupted buffer %x start", name,(unsigned int)buf_ptr+sizeof(struct buffer_start_marker));
        }
        end=(struct buffer_end_marker*)(buf_ptr+sizeof(struct buffer_start_marker)+start->buffer_size);
        if(end->magic != BUF_MAGIC)
        {
            panicf("%s corrupted %x end", name,(unsigned int)buf_ptr+sizeof(struct buffer_start_marker));
        }
        if(end->last)
            break;
        buf_ptr=((unsigned char *)end)+sizeof(struct buffer_end_marker);
    }
}
#endif /* BUFFER_ALLOC_DEBUG */
