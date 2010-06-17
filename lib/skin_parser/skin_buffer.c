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

#ifdef ROCKBOX
#define SKIN_BUFFER_SIZE (400*1024) /* Excessivly large for now */
static unsigned char buffer[SKIN_BUFFER_SIZE];
static unsigned char *buffer_front = NULL; /* start of the free space,
                                              increases with allocation*/
static size_t buf_size = SKIN_BUFFER_SIZE;
#endif

void skin_buffer_init(size_t size)
{
#if 0 /* this will go in again later probably */
    if (buffer == NULL)
    {
        buf_size = SKIN_BUFFER_SIZE;/* global_settings.skin_buf_size */

        buffer = buffer_alloc(buf_size);
        buffer_front = buffer;
        buffer_back = bufer + buf_size;
    }
    else
#elif defined(ROCKBOX)
    {
        /* reset the buffer.... */
        buffer_front = buffer;
        //TODO: buf_size = size;
    }
#endif
}

/* Allocate size bytes from the buffer */
void* skin_buffer_alloc(size_t size)
{
    void *retval = NULL;
#ifdef ROCKBOX    
    retval = buffer_front;
    buffer_front += size;
    /* 32-bit aligned */
    buffer_front = (void *)(((unsigned long)buffer_front + 3) & ~3);
#else
    retval = malloc(size);
#endif
    return retval;
}
