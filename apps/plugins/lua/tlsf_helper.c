/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Marcin Bukat
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

#include "plugin.h"
#include <tlsf.h>
#include "lua.h"
static const unsigned int sentinel = 0xBA5EFAC7;
#define SENTINEL(n) (sentinel ^ (n))

static char *pluginbuf_ptr = NULL;
static size_t pluginbuf_size = 0;
static char *audiobuf_ptr = NULL;
static size_t audiobuf_size = 0;

static void set_sentinel(void* buf, size_t size)
{
    size_t i;
    unsigned int *b = (int*) buf;
    for(i = 0; i < size / sizeof(sentinel); i++)
        *b++ = SENTINEL(i);
}

static size_t check_sentinel(void* buf, size_t size)
{
    const size_t sz = size / sizeof(sentinel);
    size_t unused = 0;
    size_t i;
    unsigned int *b = (int*) buf;
    for(i = 0; i < sz; i++)
        if (b[i] == SENTINEL(i))
        {
            unused++;
            while(++i < sz && b[i] == SENTINEL(i) && ++unused)
            {;;}
        }
    return unused * sizeof(sentinel);
}

size_t rock_get_allocated_bytes(void)
{
    return pluginbuf_size + audiobuf_size;
}

size_t rock_get_unused_bytes(void)
{
    size_t unused = 0;
    if (pluginbuf_size)
        unused += check_sentinel(pluginbuf_ptr, pluginbuf_size);
    if (audiobuf_size)
        unused += check_sentinel(audiobuf_ptr, audiobuf_size);
    return unused;
}

void *get_new_area(size_t *size)
{
    if (pluginbuf_ptr == NULL)
    {
        pluginbuf_ptr = rb->plugin_get_buffer(size);

        pluginbuf_size = *size;
        set_sentinel(pluginbuf_ptr, pluginbuf_size);

        /* kill tlsf signature if any */
        memset(pluginbuf_ptr, 0, 4);

        return pluginbuf_ptr;
    }

    /* only grab the next area if lua already tried + failed to garbage collect*/
    if (audiobuf_ptr == NULL && (get_lua_OOM())->count > 0)
    {
        /* grab audiobuffer */
        audiobuf_ptr = rb->plugin_get_audio_buffer(size);

        audiobuf_size = *size;
        set_sentinel(audiobuf_ptr, audiobuf_size);

        return audiobuf_ptr;
    }

    return ((void *) ~0);
}
