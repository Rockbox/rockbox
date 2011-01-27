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

#ifdef APPLICATION
#   define USE_HOST_MALLOC
#else
#   define USE_ROCKBOX_ALLOC
#endif

#endif

#ifdef USE_ROCKBOX_ALLOC
static size_t buf_size;
static unsigned char *buffer_start = NULL;
static unsigned char *buffer_front = NULL;
#endif

#ifdef USE_HOST_MALLOC

struct malloc_object {
    struct malloc_object *next;
    char buf[0];
};
static struct malloc_object *malloced_head = NULL, *malloced_tail = NULL;

static void skin_free_malloced(void)
{
    struct malloc_object *obj = malloced_head;
    struct malloc_object *this;
    while (obj)
    {
        this = obj;
        obj = this->next;
        free(this);
    }
    malloced_head = NULL;
    malloced_tail = NULL;
}

#endif

void skin_buffer_init(char* buffer, size_t size)
{
#ifdef USE_ROCKBOX_ALLOC
    buffer_start = buffer_front = buffer;
    buf_size = size;
#elif defined(USE_HOST_MALLOC)
    (void)buffer; (void)size;
    skin_free_malloced();
#endif
}

/* Allocate size bytes from the buffer */
void* skin_buffer_alloc(size_t size)
{
    void *retval = NULL;
#ifdef USE_ROCKBOX_ALLOC
    /* 32-bit aligned */
    size = (size + 3) & ~3;
    if (size > skin_buffer_freespace())
    {
        skin_error(MEMORY_LIMIT_EXCEEDED, NULL);
        return NULL;
    }
    retval = buffer_front;
    buffer_front += size;
#elif defined(USE_HOST_MALLOC)
    size_t malloc_size = sizeof(struct malloc_object) + size;
    struct malloc_object *obj = malloc(malloc_size);
    retval = &obj->buf;
    obj->next = NULL;
    if (malloced_tail == NULL)
        malloced_head = malloced_tail = obj;
    else
        malloced_tail->next = obj;
    malloced_tail = obj;
    
#else
    retval = malloc(size);
#endif
    return retval;
}


#ifdef USE_ROCKBOX_ALLOC
/* get the number of bytes currently being used */
size_t skin_buffer_usage(void)
{
    return buffer_front - buffer_start;
}
size_t skin_buffer_freespace(void)
{
    return buf_size - skin_buffer_usage();
}

static unsigned char *saved_buffer_pos = NULL;
void skin_buffer_save_position(void)
{
    saved_buffer_pos = buffer_front;
}
    
void skin_buffer_restore_position(void)
{
    if (saved_buffer_pos)
        buffer_front = saved_buffer_pos;
}
#endif
