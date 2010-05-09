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
#include "skin_engine.h"
#include "wps_internals.h"
#include "skin_tokens.h"
#include "skin_buffer.h"

/* skin buffer management.
 * This module is used to allocate space in a single global skin buffer for
 * tokens for both/all screens.
 *
 * This is mostly just copy/paste from firmware/buffer.c
 *
 *
 * MAIN_ and REMOTE_BUFFER are just for reasonable size calibration,
 * both screens can use the whole buffer as they need; it's not split
 * between screens
 *
 * Buffer can be allocated from either "end" of the global buffer.
 * items with unknown sizes get allocated from the start (0->)      (data)
 * items with known sizes get allocated from the end (<-buf_size)   (tokens)
 * After loading 2 skins the buffer will look like this:
 *  |tokens skin1|images skin1|tokens s2|images s2|---SPACE---|data skin2|data skin1|
 * Make sure to never start allocating from the beginning before letting us know
 * how much was used. and RESPECT THE buf_free RETURN VALUES!
 *
 */


#ifdef HAVE_LCD_BITMAP
#define MAIN_BUFFER ((2*LCD_BACKDROP_BYTES) \
                    + (SKINNABLE_SCREENS_COUNT * LCD_BACKDROP_BYTES))

#if (NB_SCREENS > 1)
#define REMOTE_BUFFER (2*REMOTE_LCD_BACKDROP_BYTES) \
                      + (SKINNABLE_SCREENS_COUNT * REMOTE_LCD_BACKDROP_BYTES))
#else
#define REMOTE_BUFFER 0
#endif


#define SKIN_BUFFER_SIZE (MAIN_BUFFER + REMOTE_BUFFER) + \
                         (WPS_MAX_TOKENS * sizeof(struct wps_token))
#endif

#ifdef HAVE_LCD_CHARCELLS
#define SKIN_BUFFER_SIZE (LCD_HEIGHT * LCD_WIDTH) * 64 + \
                         (WPS_MAX_TOKENS * sizeof(struct wps_token))
#endif

static unsigned char buffer[SKIN_BUFFER_SIZE];
static unsigned char *buffer_front = NULL; /* start of the free space,
                                              increases with allocation*/
static unsigned char *buffer_back  = NULL; /* end of the free space
                                              decreases with allocation */
static size_t buf_size = SKIN_BUFFER_SIZE;

void skin_buffer_init(void)
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
#endif
    {
        /* reset the buffer.... */
        buffer_front = buffer;
        buffer_back = buffer + buf_size;
    }
}

/* get the number of bytes currently being used */
size_t skin_buffer_usage(void)
{
    return buf_size - (buffer_back-buffer_front);
}

size_t skin_buffer_freespace(void)
{
    return buffer_back-buffer_front;
}

/* Allocate size bytes from the buffer
 * allocates from the back end (data end)
 */
void* skin_buffer_alloc(size_t size)
{
    if (skin_buffer_freespace() <= size)
    {
        return NULL;
    }
    buffer_back -= size;
    /* 32-bit aligned */
    buffer_back = (void *)(((unsigned long)buffer_back) & ~3);

    memset(buffer_back, 0, size);
    return buffer_back;
}

/* Get a pointer to the skin buffer and the count of how much is free
 * used to do your own buffer management.
 * Any memory used will be overwritten next time wps_buffer_alloc()
 * is called unless skin_buffer_increment() is called first
 *
 * This is from the start of the buffer, it is YOUR responsility to make
 * sure you dont ever use more then *freespace, and bear in mind this will only
 * be valid untill skin_buffer_alloc() is next called...
 * so call skin_buffer_increment() and skin_buffer_freespace() regularly
 */
void* skin_buffer_grab(size_t *freespace)
{
    *freespace = buf_size - skin_buffer_usage();
    return buffer_front;
}

/* Use after skin_buffer_grab() to specify how much buffer was used */
void skin_buffer_increment(size_t used, bool align)
{
    buffer_front += used;
    if (align)
    {
        /* 32-bit aligned */
        buffer_front = (void *)(((unsigned long)buffer_front + 3) & ~3);
    }
}

/* free previously skin_buffer_increment()'ed space. This just moves the pointer
 * back 'used' bytes so make sure you actually want to do this */
void skin_buffer_free_from_front(size_t used)
{
    buffer_front -= used;
    /* 32-bit aligned */
    buffer_front = (void *)(((unsigned long)buffer_front + 3) & ~3);
}


