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
 * Copyright (C) 2009 Jonathan Gordon
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

#ifdef ROCKBOX
static size_t buf_size;
static unsigned char *buffer_start = NULL;
static unsigned char *buffer_front = NULL;
#endif

void skin_buffer_init(char* buffer, size_t size)
{
#if defined(ROCKBOX)
    buffer_start = buffer_front = buffer;
    buf_size = size;
#endif
}

/* Allocate size bytes from the buffer */
void* skin_buffer_alloc(size_t size)
{
    void *retval = NULL;
#ifdef ROCKBOX
    if (size > skin_buffer_freespace())
        return NULL;
    retval = buffer_front;
    buffer_front += size;
    /* 32-bit aligned */
    buffer_front = (void *)(((unsigned long)buffer_front + 3) & ~3);
#else
    retval = malloc(size);
#endif
    return retval;
}


#ifdef ROCKBOX
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
