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
#include <string.h>
#include <stdlib.h>

#include "settings.h"
#include "skin_buffer.h"
#include "wps_internals.h"
#include "skin_engine.h"

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))

static struct skin_backdrop {
    char name[MAX_FILENAME+1];
    char *buffer;
    enum screen_type screen;
} backdrops[SKINNABLE_SCREENS_COUNT*NB_SCREENS];

void skin_backdrop_init(void)
{
    int i;
    for(i=0;i<SKINNABLE_SCREENS_COUNT*NB_SCREENS;i++)
    {
        backdrops[i].name[0] = '\0';
        backdrops[i].buffer = NULL;
    }
}

/* load a backdrop into the skin buffer.
 * reuse buffers if the file is already loaded */
char* skin_backdrop_load(char* backdrop, char *bmpdir, enum screen_type screen)
{
    int i;
    struct skin_backdrop *bdrop = NULL;
    char filename[MAX_PATH];
    size_t buf_size;
    bool loaded = false;
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    if (screen == SCREEN_REMOTE)
        buf_size = REMOTE_LCD_BACKDROP_BYTES;
    else
#endif
        buf_size = LCD_BACKDROP_BYTES;

    if (backdrop[0] == '-')
    {
#if NB_SCREENS > 1
        if (screen == SCREEN_REMOTE)
        {
            return NULL; /* remotes don't have a backdrop setting (yet!) */
        }
        else
#endif
        {
            char settings_bdrop = global_settings.backdrop_file[0];
            if (settings_bdrop == '\0' || settings_bdrop == '-')
            {
                return NULL; /* backdrop setting not set */
            }
            snprintf(filename, sizeof(filename), "%s/%s.bmp",
                     BACKDROP_DIR, global_settings.backdrop_file);
        }
    }
    else
    {
        get_image_filename(backdrop, bmpdir, filename, sizeof(filename));
    }
    
    for(i=0;i<SKINNABLE_SCREENS_COUNT*NB_SCREENS;i++)
    {
        if (!strcmp(backdrops[i].name, backdrop) && backdrops[i].screen == screen)
        {
            return backdrops[i].buffer;
        }
        else if (!bdrop && backdrops[i].buffer == NULL)
        {
            bdrop = &backdrops[i];
        }
    }
    if (!bdrop)
        return NULL; /* too many backdrops loaded */
    
    bdrop->buffer = skin_buffer_alloc(buf_size);
    if (!bdrop->buffer)
        return NULL;
    loaded = screens[screen].backdrop_load(filename, bdrop->buffer);
    bdrop->screen = screen;
    strlcpy(bdrop->name, backdrop, MAX_FILENAME+1);
    
    return loaded ? bdrop->buffer : NULL;
}
#else

void skin_backdrop_init(void)
{
}
#endif
