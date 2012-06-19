/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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
 
#ifndef _SKIN_ENGINE_H
#define _SKIN_ENGINE_H

#ifndef PLUGIN

#include "tag_table.h"

#include "wps_internals.h" /* TODO: remove this line.. shoudlnt be needed */

enum skinnable_screens {
#ifdef HAVE_LCD_BITMAP
    CUSTOM_STATUSBAR,
#endif
    WPS,
#if CONFIG_TUNER
    FM_SCREEN,
#endif
    
    
    SKINNABLE_SCREENS_COUNT
};

#ifdef HAVE_TOUCHSCREEN
int skin_get_touchaction(struct wps_data *data, int* edge_offset,
                         struct touchregion **retregion);
void skin_disarm_touchregions(struct wps_data *data);
#endif

/* Do a update_type update of the skinned screen */
void skin_update(enum skinnable_screens skin, enum screen_type screen,
                 unsigned int update_type);

/*
 * setup up the skin-data from a format-buffer (isfile = false)
 * or from a skinfile (isfile = true)
 */
bool skin_data_load(enum screen_type screen, struct wps_data *wps_data,
                    const char *buf, bool isfile);

bool skin_has_sbs(enum screen_type screen, struct wps_data *data);


/* load a backdrop into the skin buffer.
 * reuse buffers if the file is already loaded */
char* skin_backdrop_load(char* backdrop, char *bmpdir, enum screen_type screen);
void skin_backdrop_init(void);
int skin_backdrop_assign(char* backdrop, char *bmpdir,
                          enum screen_type screen);
bool skin_backdrops_preload(void);
void skin_backdrop_show(int backdrop_id);
void skin_backdrop_load_setting(void);
void skin_backdrop_unload(int backdrop_id);
#define BACKDROP_BUFFERNAME "#backdrop_buffer#"
void* skin_backdrop_get_buffer(int backdrop_id);

/* do the button loop as often as required for the peak meters to update
 * with a good refresh rate. 
 * gwps is really gwps[NB_SCREENS]! don't wrap this in FOR_NB_SCREENS()
 */
int skin_wait_for_action(enum skinnable_screens skin, int context, int timeout);

void skin_load(enum skinnable_screens skin, enum screen_type screen,
               const char *buf, bool isfile);
struct gui_wps *skin_get_gwps(enum skinnable_screens skin, enum screen_type screen);
struct wps_state *skin_get_global_state(void);
void gui_sync_skin_init(void);

void skin_unload_all(void);

bool skin_do_full_update(enum skinnable_screens skin, enum screen_type screen);
void skin_request_full_update(enum skinnable_screens skin);

#endif /* !PLUGIN */
#endif
