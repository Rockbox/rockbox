/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Thomas Martitz
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

#include "system.h"
#include "settings.h"
#include "appevents.h"
#include "screens.h"
#include "screen_access.h"
#include "skin_engine/skin_engine.h"
#include "skin_engine/wps_internals.h"
#include "viewport.h"
#include "statusbar.h"
#include "statusbar-skinned.h"
#include "debug.h"


/* currently only one wps_state is needed */
extern struct wps_state     wps_state; /* from wps.c */
static struct gui_wps       sb_skin[NB_SCREENS]      = {{ .data = NULL }};
static struct wps_data      sb_skin_data[NB_SCREENS] = {{ .wps_loaded = 0 }};
static struct wps_sync_data sb_skin_sync_data        = { .do_full_update = false };

/* initial setup of wps_data  */
static int update_delay = DEFAULT_UPDATE_DELAY;


void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile)
{
    struct wps_data *data = sb_skin[screen].data;

    int success;
    success = buf && skin_data_load(screen, data, buf, isfile);

    if (success)
    {  /* hide the sb's default viewport because it has nasty effect with stuff
        * not part of the statusbar,
        * hence .sbs's without any other vps are unsupported*/
        struct skin_viewport *vp = find_viewport(VP_DEFAULT_LABEL, data);
        struct skin_token_list *next_vp = data->viewports->next;

        if (!next_vp)
        {    /* no second viewport, let parsing fail */
            success = false;
        }
        /* hide this viewport, forever */
        vp->hidden_flags = VP_NEVER_VISIBLE;
    }

    if (!success && isfile)
        sb_create_from_settings(screen);
}

struct viewport *sb_skin_get_info_vp(enum screen_type screen)
{
    return &find_viewport(VP_INFO_LABEL, sb_skin[screen].data)->vp;
}

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1)
char* sb_get_backdrop(enum screen_type screen)
{
    return sb_skin[screen].data->backdrop;
}

bool sb_set_backdrop(enum screen_type screen, char* filename)
{
    if (!filename)
    {
        sb_skin[screen].data->backdrop = NULL;
        return true;
    }
    else if (!sb_skin[screen].data->backdrop)
    {
        /* need to make room on the buffer */
        size_t buf_size;
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
        if (screen == SCREEN_REMOTE)
            buf_size = REMOTE_LCD_BACKDROP_BYTES;
        else
#endif
            buf_size = LCD_BACKDROP_BYTES;
        sb_skin[screen].data->backdrop = skin_buffer_alloc(buf_size);
        if (!sb_skin[screen].data->backdrop)
            return false;          
    }
  
    if (!screens[screen].backdrop_load(filename, sb_skin[screen].data->backdrop))
        sb_skin[screen].data->backdrop = NULL;
    return sb_skin[screen].data->backdrop != NULL;
}
        
#endif
void sb_skin_update(enum screen_type screen, bool force)
{
    static long next_update = 0;
    int i = screen;
    if (TIME_AFTER(current_tick, next_update) || force)
    {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        /* currently, all remotes are readable without backlight
         * so still update those */
        if (lcd_active() || (i != SCREEN_MAIN))
#endif
            skin_update(&sb_skin[i], force?
                    WPS_REFRESH_ALL : WPS_REFRESH_NON_STATIC);
        next_update = current_tick + update_delay; /* don't update too often */
        sb_skin[SCREEN_MAIN].sync_data->do_full_update = false;
    }
}

void do_sbs_update_callback(void *param)
{
    (void)param;
    /* the WPS handles changing the actual id3 data in the id3 pointers
     * we imported, we just want a full update */
    sb_skin_sync_data.do_full_update = true;
    /* force timeout in wps main loop, so that the update is instantly */
    queue_post(&button_queue, BUTTON_NONE, 0);
}

void sb_skin_set_update_delay(int delay)
{
    update_delay = delay;
}

/* This creates and loads a ".sbs" based on the user settings for:
 *  - regular statusbar
 *  - colours
 *  - ui viewport
 *  - backdrop
 */
void sb_create_from_settings(enum screen_type screen)
{
    char buf[128], *ptr, *ptr2;
    int len, remaining = sizeof(buf);
    
    ptr = buf;
    ptr[0] = '\0';
    
    /* %Vi viewport, colours handled by the parser */
#if NB_SCREENS > 1
    if (screen == SCREEN_REMOTE)
        ptr2 = global_settings.remote_ui_vp_config;
    else
#endif
        ptr2 = global_settings.ui_vp_config;
    
    if (ptr2[0] && ptr2[0] != '-') /* from ui viewport setting */
    {
        len = snprintf(ptr, remaining, "%%ax%%Vi|%s|\n", ptr2);
        while ((ptr2 = strchr(ptr, ',')))
            *ptr2 = '|';
    }
    else
    {
        int y = 0, height;
        switch (statusbar_position(screen))
        {
            case STATUSBAR_TOP:
                y = STATUSBAR_HEIGHT;
            case STATUSBAR_BOTTOM:
                height = screens[screen].lcdheight - STATUSBAR_HEIGHT;
                break;
            default:
                height = screens[screen].lcdheight;
        }
        len = snprintf(ptr, remaining, "%%ax%%Vi|0|%d|-|%d|1|-|-|\n", y, height);
    }
    sb_skin_data_load(screen, buf, false);
}

void sb_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_ALBUMART
        sb_skin_data[i].albumart = NULL;
        sb_skin_data[i].playback_aa_slot = -1;
#endif
        sb_skin[i].data = &sb_skin_data[i];
        sb_skin[i].display = &screens[i];
        /* Currently no seperate wps_state needed/possible
           so use the only available ( "global" ) one */
        sb_skin[i].state = &wps_state;
        sb_skin[i].sync_data = &sb_skin_sync_data;
    }
}
