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
#include <stdlib.h>
#include "string-extra.h"
#include "settings.h"
#include "wps_internals.h"
#include "skin_engine.h"
#include "skin_buffer.h"

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))

#define NB_BDROPS SKINNABLE_SCREENS_COUNT*NB_SCREENS
static struct skin_backdrop {
    char name[MAX_PATH];
    char *buffer;
    enum screen_type screen;
    bool loaded;
} backdrops[NB_BDROPS];

#define NB_BDROPS SKINNABLE_SCREENS_COUNT*NB_SCREENS

void skin_backdrop_init(void)
{
    int i;
    for (i=0; i<NB_BDROPS; i++)
    {
        backdrops[i].name[0] = '\0';
        backdrops[i].buffer = NULL;
        backdrops[i].loaded = false;
    }
}

int skin_backdrop_assign(char* backdrop, char *bmpdir,
                         enum screen_type screen)
{
    char filename[MAX_PATH];
    int i, free = -1;
    if (!backdrop)
        return -1;
    if (backdrop[0] == '-')
    {
        filename[0] = '-';
        filename[1] = '\0';
        filename[2] = '\0'; /* we check this later to see if we actually have an
                               image to load. != '\0' means display the image */
    }
    else
    {
        get_image_filename(backdrop, bmpdir, filename, sizeof(filename));
    }
    for (i=0; i<NB_BDROPS; i++)
    {
        if (!backdrops[i].name[0] && free < 0)
            free = i;
        if (!strcmp(backdrops[i].name, filename) && backdrops[i].screen == screen)
        {
            break;
        }
    }
    if (i < NB_BDROPS)
        return i;
    else if (free >= 0)
    {
        strlcpy(backdrops[free].name, filename,
                sizeof (backdrops[free].name));
        backdrops[free].buffer = NULL;
        backdrops[free].screen = screen;
        return free;
    }
    return -1;
}

bool skin_backdrops_preload(void)
{
    bool retval = true;
    int i;
    char *filename;
    for (i=0; i<NB_BDROPS; i++)
    {
        if (backdrops[i].name[0] && !backdrops[i].buffer)
        {
            size_t buf_size;
            enum screen_type screen = backdrops[i].screen;
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
            if (screen == SCREEN_REMOTE)
                buf_size = REMOTE_LCD_BACKDROP_BYTES;
            else
#endif
                buf_size = LCD_BACKDROP_BYTES;

            filename = backdrops[i].name;
            if (screen == SCREEN_MAIN && global_settings.backdrop_file[0] &&
                global_settings.backdrop_file[0] != '-' && filename[0] == '-')
            {
                char* temp = filename+2; /* slightly hacky to get a buffer */
                size_t size = sizeof(backdrops[i].name) - 2;
                snprintf(temp, size, BACKDROP_DIR "/%s.bmp", global_settings.backdrop_file);
                filename = temp;
            }
            if (*filename && *filename != '-')
            {
                backdrops[i].buffer = (char*)skin_buffer_alloc(buf_size);
                backdrops[i].loaded = backdrops[i].buffer && 
                         screens[screen].backdrop_load(filename, backdrops[i].buffer);
                if (!backdrops[i].loaded)
                    retval = false;
            }
            if (backdrops[i].name[0] == '-' && backdrops[i].loaded)
                backdrops[i].name[2] = '.';
        }
    }
    return retval;
}

void skin_backdrop_show(int backdrop_id)
{
    if (backdrop_id < 0)
        return;
    enum screen_type screen = backdrops[backdrop_id].screen;
    if ((backdrops[backdrop_id].loaded == false) ||
        (backdrops[backdrop_id].name[0] == '-' &&
        backdrops[backdrop_id].name[2] == '\0'))
    {
        screens[screen].backdrop_show(NULL);
    }
    else if (backdrops[backdrop_id].buffer)
        screens[screen].backdrop_show(backdrops[backdrop_id].buffer);
}

void skin_backdrop_unload(int backdrop_id)
{
    backdrops[backdrop_id].buffer = NULL;
}

void skin_backdrop_load_setting(void)
{
    int i;
    char filename[MAX_PATH];
    for(i=0;i<SKINNABLE_SCREENS_COUNT*NB_SCREENS;i++)
    {
        if (backdrops[i].name[0] == '-' && backdrops[i].screen == SCREEN_MAIN)
        {
            if (global_settings.backdrop_file[0] &&
                global_settings.backdrop_file[0] != '-')
            {
                if (!backdrops[i].buffer)
                    backdrops[i].buffer = (char*)skin_buffer_alloc(LCD_BACKDROP_BYTES);
                snprintf(filename, sizeof filename, BACKDROP_DIR "/%s.bmp",
                         global_settings.backdrop_file);
                bool loaded = backdrops[i].buffer && 
                              screens[SCREEN_MAIN].backdrop_load(filename,
                                                            backdrops[i].buffer);
                backdrops[i].name[2] = loaded ? '.' : '\0';
                return;
            }
            else
                backdrops[i].name[2] = '\0';
        }
#if NB_SCREENS > 1
        else if (backdrops[i].name[0] == '-')
        {
            backdrops[i].name[2] = '\0';
            return;
        }
#endif
    }
}
    
#else

void skin_backdrop_init(void)
{
}
#endif
