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

static struct radioart radioart[MAX_RADIOART_IMAGES];
#ifdef HAVE_RECORDING
static bool allow_buffer_access = true; /* If we are recording dont touch the buffers! */
#endif
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
    ra->handle = bufopen(path, 0, TYPE_BITMAP, &ra->dim);
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
    int i, free_idx = -1;
    const char* preset_name;
    if (radio_scan_mode() || preset < 0)
        return -1;
#ifdef HAVE_RECORDING
    if (!allow_buffer_access)
        return -1;
#endif
    preset_name = radio_get_preset_name(preset);
    for(i=0;i<MAX_RADIOART_IMAGES;i++)
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
static void playback_restarting_handler(void *data)
{
    (void)data;
    int i;
    for(i=0;i<MAX_RADIOART_IMAGES;i++)
    {
        if (radioart[i].handle >= 0)
            bufclose(radioart[i].handle);
        radioart[i].handle = -1;
        radioart[i].name[0] = '\0';
    }
}
#ifdef HAVE_RECORDING
static void recording_started_handler(void *data)
{
    (void)data;
    allow_buffer_access = false;
    playback_restarting_handler(NULL);
}
static void recording_stopped_handler(void *data)
{
    (void)data;
    allow_buffer_access = true;
}
#endif

void radioart_init(bool entering_screen)
{
    int i;
    if (entering_screen)
    {
        for(i=0;i<MAX_RADIOART_IMAGES;i++)
        {
            radioart[i].handle = -1;
            radioart[i].name[0] = '\0';
        }
        add_event(PLAYBACK_EVENT_START_PLAYBACK, true, playback_restarting_handler);
    }
    else
    {
#if defined(HAVE_RECORDING)
        add_event(RECORDING_EVENT_START, false, recording_started_handler);
        add_event(RECORDING_EVENT_STOP, false, recording_stopped_handler);
#endif
    }
}
