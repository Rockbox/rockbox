/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: skin_buffer.c 25962 2010-05-12 09:31:40Z jdgordon $
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 * Copyright (C) 2010 Jonathan Gordon
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
#include <string.h>
#include <stdlib.h>

#include "skin_buffer.h"
#include "skin_parser.h"

/****************************************************************************
 * 
 *  This code handles buffer allocation for the entire skin system.
 *  This needs to work in 3 different situations:
 *    1) as a stand alone library. ROCKBOX isnt defined, alloc using malloc()
 *       and free the skin elements only (no callbacks doing more allocation)
 *    2) ROCKBOX builds for normal targets, alloc from a single big buffer
 *       which origionally came from the audio buffer, likely to run out of
 *       room with large themes. No need to free anything, just restore to
 *       the start of our buffer
 *    3) ROCKBOX "application/hosted" builds, alloc using the hosts malloc().
 *       We need to keep track of all allocations so they can be free()'d easily
 * 
 * 
 ****************************************************************************/


#ifdef ROCKBOX
#include "config.h"
#include "skin_debug.h"
static size_t buf_size;
static unsigned char *buffer_start = NULL;
static unsigned char *buffer_front = NULL;

#ifndef __PCTOOL__
long skin_buffer_to_offset(void *pointer)
{
    return pointer == NULL ? -1 : (void*)pointer - (void*)buffer_start;
}

void* skin_buffer_from_offset(long offset)
{
    return offset < 0 ? NULL : buffer_start + offset;
}
#endif

void skin_buffer_init(char* buffer, size_t size)
{
    buffer_start = buffer_front = buffer;
    buf_size = size;
}

/* Allocate size bytes from the buffer */
#ifdef DEBUG_SKIN_ALLOCATIONS
void* skin_buffer_alloc_ex(size_t size, char* debug)
{
    void *retval = NULL;
    printf("%d %s\n", size, debug);
#else
void* skin_buffer_alloc(size_t size)
{
    void *retval = NULL;
#endif
    /* align to long which is enough for most types */
    size = (size + sizeof(long) - 1) & ~(sizeof(long) - 1);
    if (size > skin_buffer_freespace())
    {
        skin_error(MEMORY_LIMIT_EXCEEDED, NULL);
        return NULL;
    }
    retval = buffer_front;
    buffer_front += size;
    return retval;
}

/* get the number of bytes currently being used */
size_t skin_buffer_usage(void)
{
    return buffer_front - buffer_start;
}
size_t skin_buffer_freespace(void)
{
    return buf_size - skin_buffer_usage();
}
#else
void* skin_buffer_alloc(size_t size)
{
    return malloc(size);
}
#endif
