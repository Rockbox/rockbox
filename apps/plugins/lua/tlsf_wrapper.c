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

static char *tlsf_pool = NULL;
static char *audiobuf_ptr = NULL;

void *rb_realloc(void *mem, size_t size)
{
    size_t audiobuf_size = 0;
    size_t pluginbuf_size = 0;
    void *ptr;

    if (tlsf_pool == NULL)
    {
        tlsf_pool = rb->plugin_get_buffer(&pluginbuf_size);
        init_memory_pool(pluginbuf_size, tlsf_pool);
    }

    ptr = tlsf_realloc(mem, size);

    if (ptr == NULL)
    {
        if (audiobuf_ptr == NULL)
        {
            audiobuf_ptr = rb->plugin_get_audio_buffer(&audiobuf_size);
            add_new_area(tlsf_pool, audiobuf_size, (void *)audiobuf_ptr);

            ptr = tlsf_realloc(mem, size);
        }
    }

    return ptr;
}

void *rb_malloc(size_t size)
{
    size_t audiobuf_size = 0;
    size_t pluginbuf_size = 0;
    void *ptr;

    if (tlsf_pool == NULL)
    {
        tlsf_pool = rb->plugin_get_buffer(&pluginbuf_size);
        init_memory_pool(pluginbuf_size, tlsf_pool);
    }

    ptr = tlsf_malloc(size);

    if (ptr == NULL)
    {
        if (audiobuf_ptr == NULL)
        {
            audiobuf_ptr = rb->plugin_get_audio_buffer(&audiobuf_size);
            add_new_area(tlsf_pool, audiobuf_size, (void *)audiobuf_ptr);

            ptr = tlsf_malloc(size);
        }
    }

    return ptr;
}
