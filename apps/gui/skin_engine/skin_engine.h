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

#include "skin_fonts.h"
#include "tag_table.h"

#include "wps_internals.h" /* TODO: remove this line.. shoudlnt be needed */

enum skinnable_screens {
    CUSTOM_STATUSBAR,
    WPS,
#if CONFIG_TUNER
    FM_SCREEN,
#endif
    
    
    SKINNABLE_SCREENS_COUNT
};


#ifdef HAVE_LCD_BITMAP
#define MAIN_BUFFER ((2*LCD_HEIGHT*LCD_WIDTH*LCD_DEPTH/8) \
                    + (SKINNABLE_SCREENS_COUNT * LCD_BACKDROP_BYTES))

#if (NB_SCREENS > 1)
#define REMOTE_BUFFER (2*(LCD_REMOTE_HEIGHT*LCD_REMOTE_WIDTH*LCD_REMOTE_DEPTH/8) \
                      + (SKINNABLE_SCREENS_COUNT * REMOTE_LCD_BACKDROP_BYTES))
#else
#define REMOTE_BUFFER 0
#endif


#define SKIN_BUFFER_SIZE (MAIN_BUFFER + REMOTE_BUFFER + SKIN_FONT_SIZE) + \
                         (WPS_MAX_TOKENS * \
                         (sizeof(struct wps_token) + (sizeof(struct skin_element))))
#endif

#ifdef HAVE_LCD_CHARCELLS
#define SKIN_BUFFER_SIZE (LCD_HEIGHT * LCD_WIDTH) * 64 + \
                         (WPS_MAX_TOKENS * \
                         (sizeof(struct wps_token) + (sizeof(struct skin_element))))
#endif


#ifdef HAVE_TOUCHSCREEN
int skin_get_touchaction(struct wps_data *data, int* edge_offset,
                         struct touchregion **retregion);
void skin_disarm_touchregions(struct wps_data *data);
#endif

/* Do a update_type update of the skinned screen */
void skin_update(struct gui_wps *gwps, unsigned int update_type);

/*
 * setup up the skin-data from a format-buffer (isfile = false)
 * or from a skinfile (isfile = true)
 */
bool skin_data_load(enum screen_type screen, struct wps_data *wps_data,
                    const char *buf, bool isfile);

/* call this in statusbar toggle handlers if needed */
void skin_statusbar_changed(struct gui_wps*);

bool skin_has_sbs(enum screen_type screen, struct wps_data *data);


/* load a backdrop into the skin buffer.
 * reuse buffers if the file is already loaded */
char* skin_backdrop_load(char* backdrop, char *bmpdir, enum screen_type screen);
void skin_backdrop_init(void);


/* do the button loop as often as required for the peak meters to update
 * with a good refresh rate. 
 * gwps is really gwps[NB_SCREENS]! don't wrap this in FOR_NB_SCREENS()
 */
int skin_wait_for_action(struct gui_wps *gwps, int context, int timeout);
#endif

#endif
