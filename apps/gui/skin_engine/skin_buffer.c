/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: wps_parser.c 19880 2009-01-29 20:49:43Z mcuelenaere $
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
#include "config.h"
#include "buffer.h"
#include "settings.h"
#include "screen_access.h"

/* skin buffer management.
 * This module is used to allocate space in a single global skin buffer for
 * tokens for both/all screens.
 * 
 * This is mostly just copy/paste from firmware/buffer.c
 */

#define IMG_BUFSIZE (((LCD_HEIGHT*LCD_WIDTH*LCD_DEPTH/8) \
                        + (2*LCD_HEIGHT*LCD_WIDTH/8)) * NB_SCREENS)
                        
static unsigned char buffer_start[IMG_BUFSIZE], *buffer_pos = NULL;
static size_t buf_size = IMG_BUFSIZE;

void skin_buffer_init(void)
{
#if 0 /* this will go in again later probably */
    if (buffer_start == NULL)
    {
        buf_size = IMG_BUFSIZE;/* global_settings.skin_buf_size */
            
        buffer_start = buffer_alloc(buf_size);
        buffer_pos = buffer_start;
    }
    else
#endif
    {
        /* reset the buffer.... */
        buffer_pos = buffer_start;
    }
}

/* get the number of bytes currently being used */
size_t skin_buffer_usage(void)
{
    return buffer_pos-buffer_start;
}

/* Allocate size bytes from the buffer */
void* skin_buffer_alloc(size_t size)
{
    void* retval = buffer_pos;
    if (skin_buffer_usage()+size >= buf_size)
    {
        return NULL;
    }
    buffer_pos += size;
    /* 32-bit aligned */
    buffer_pos = (void *)(((unsigned long)buffer_pos + 3) & ~3);
    return retval;
}

/* Get a pointer to the skin buffer and the count of how much is free
 * used to do your own buffer management. 
 * Any memory used will be overwritten next time wps_buffer_alloc()
 * is called unless skin_buffer_increment() is called first
 */
void* skin_buffer_grab(size_t *freespace)
{
    *freespace = buf_size - skin_buffer_usage();
    return buffer_pos;
}

/* Use after skin_buffer_grab() to specify how much buffer was used */
void skin_buffer_increment(size_t used)
{
    buffer_pos += used;
    /* 32-bit aligned */
    buffer_pos = (void *)(((unsigned long)buffer_pos + 3) & ~3);
}

