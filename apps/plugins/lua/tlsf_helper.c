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

void *get_new_area(size_t *size)
{
    static char *pluginbuf_ptr = NULL;
    static char *audiobuf_ptr = NULL;

    if (pluginbuf_ptr == NULL)
    {
        pluginbuf_ptr = rb->plugin_get_buffer(size);

        /* kill tlsf signature if any */
        memset(pluginbuf_ptr, 0, 4);

        return pluginbuf_ptr;
    }

    if (audiobuf_ptr == NULL)
    {
        /* grab audiobuffer */
        audiobuf_ptr = rb->plugin_get_audio_buffer(size);

        return audiobuf_ptr;
    }

    return ((void *) ~0);
}
