/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "system.h"
#include "settings.h"
#include "radio.h"
#include "buffering.h"
#include "playback.h" /* bufopen_user_data */
#include "file.h"
#include "kernel.h"
#include "string-extra.h"
#include "filefuncs.h"

#define MAX_RADIOART_IMAGES 10
struct radioart {
    int handle;
    long last_tick;
    struct dim dim;
    char name[MAX_FMPRESET_LEN+1];
};

static char* buf;
static struct radioart radioart[MAX_RADIOART_IMAGES];

static int find_oldest_image(void)
{
    int i;
    long oldest_tick = radioart[0].last_tick;
    int oldest_idx = 0;
    for(i=1;i<MAX_RADIOART_IMAGES;i++)
    {
        if (radioart[i].last_tick < oldest_tick)
        {
            oldest_tick = radioart[i].last_tick;
            oldest_idx = i;
        }
    }
    return oldest_idx;
}
static int load_radioart_image(struct radioart *ra, const char* preset_name, 
                               struct dim *dim)
{
    char path[MAX_PATH];
    struct bufopen_bitmap_data user_data;
#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(false);
#endif
    snprintf(path, sizeof(path), FMPRESET_PATH "/%s.bmp",preset_name);
    if (!file_exists(path))
        snprintf(path, sizeof(path), FMPRESET_PATH "/%s.jpg",preset_name);
    if (!file_exists(path))
    {
#ifndef HAVE_NOISY_IDLE_MODE
        cpu_idle_mode(true);
#endif
        return -1;
    }
    strlcpy(ra->name, preset_name, MAX_FMPRESET_LEN+1);
    ra->dim.height = dim->height;
    ra->dim.width = dim->width;
    ra->last_tick = current_tick;
    user_data.embedded_albumart = NULL;
    user_data.dim = &ra->dim;
    ra->handle = bufopen(path, 0, TYPE_BITMAP, &user_data);
    if (ra->handle == ERR_BUFFER_FULL)
    {
        int i = find_oldest_image();
        bufclose(i);
        ra->handle = bufopen(path, 0, TYPE_BITMAP, &ra->dim);
    }
#ifndef HAVE_NOISY_IDLE_MODE
    cpu_idle_mode(true);
#endif
    return ra->handle;    
}
int radio_get_art_hid(struct dim *requested_dim)
{
    int preset = radio_current_preset();
    int free_idx = -1;
    const char* preset_name;

    if (!buf || radio_scan_mode() || preset < 0)
        return -1;

    preset_name = radio_get_preset_name(preset);
    for (int i=0; i<MAX_RADIOART_IMAGES; i++)
    {
        if (radioart[i].handle < 0)
        {
            free_idx = i;
        }
        else if (!strcmp(radioart[i].name, preset_name) &&
                 radioart[i].dim.width == requested_dim->width &&
                 radioart[i].dim.height == requested_dim->height)
        {
            radioart[i].last_tick = current_tick;
            return radioart[i].handle;
        }
    }
    if (free_idx >= 0)
    {
        return load_radioart_image(&radioart[free_idx], 
                                   preset_name, requested_dim);
    }
    else
    {
        int i = find_oldest_image();
        bufclose(radioart[i].handle);
        return load_radioart_image(&radioart[i],
                                   preset_name, requested_dim);        
    }
        
    return -1;
}

static void buffer_reset_handler(void *data)
{
    buf = NULL;
    for(int i=0;i<MAX_RADIOART_IMAGES;i++)
    {
        if (radioart[i].handle >= 0)
            bufclose(radioart[i].handle);
        radioart[i].handle = -1;
        radioart[i].name[0] = '\0';
    }

    (void)data;
}

void radioart_init(bool entering_screen)
{
    if (entering_screen)
    {
        /* grab control over buffering */
        size_t bufsize;
        buf = audio_get_buffer(false, &bufsize);
        buffering_reset(buf, bufsize);
        /* one-shot */
        add_event(BUFFER_EVENT_BUFFER_RESET, true, buffer_reset_handler);
    }
    else /* init at startup */
    {
        for(int i=0;i<MAX_RADIOART_IMAGES;i++)
        {
            radioart[i].handle = -1;
            radioart[i].name[0] = '\0';
        }
    }
}
