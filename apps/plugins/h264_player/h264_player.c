/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 David Cormier
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
#include "video_playback.h"

enum plugin_status plugin_start(const void *parameter)
{
    const char *path = parameter;
    size_t buffer_size = 0;
    void *buffer;
    int result;

    if (path == NULL || path[0] == '\0')
    {
        rb->splash(HZ * 2, "No video file");
        return PLUGIN_ERROR;
    }

    buffer = rb->plugin_get_audio_buffer(&buffer_size);
    if (buffer == NULL || buffer_size == 0)
    {
        rb->splash(HZ * 2, "Not enough video memory");
        return PLUGIN_ERROR;
    }

    result = video_h264_play(path, buffer, buffer_size);
    rb->button_clear_queue();
    rb->plugin_release_audio_buffer();

    if (result == 2)
        return PLUGIN_USB_CONNECTED;
    return result < 0 ? PLUGIN_ERROR : PLUGIN_OK;
}
