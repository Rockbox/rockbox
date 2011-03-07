/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Stuart Martin
 * RTC config saving code (C) 2002 by hessu@hes.iki.fi
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
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include "inttypes.h"
#include "config.h"
#include "action.h"
#include "crc32.h"
#include "settings.h"
#include "wps.h"
#include "file.h"
#include "buffer.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "skin_engine.h"
#include "skin_buffer.h"
#include "statusbar-skinned.h"

static bool skins_initialising = true;

/* App uses the host malloc to manage the buffer */
#ifdef APPLICATION
#define skin_buffer NULL
void theme_init_buffer(void)
{
    skins_initialising = false;
}
#else
static char *skin_buffer = NULL;
void theme_init_buffer(void)
{
    skin_buffer = buffer_alloc(SKIN_BUFFER_SIZE);
    skins_initialising = false;
}
#endif

void settings_apply_skins(void)
{
    int i, j;
    skin_buffer_init(skin_buffer, SKIN_BUFFER_SIZE);
    
#ifdef HAVE_LCD_BITMAP
    skin_backdrop_init();
    skin_font_init();
#endif
    gui_sync_skin_init();

    /* Make sure each skin is loaded */
    for (i=0; i<SKINNABLE_SCREENS_COUNT; i++)
    {
        FOR_NB_SCREENS(j)
            skin_get_gwps(i, j);
    }
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    skin_backdrops_preload(); /* should maybe check the retval here... */
#endif
    viewportmanager_theme_changed(THEME_STATUSBAR);
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    FOR_NB_SCREENS(i)
        skin_backdrop_show(sb_get_backdrop(i));
#endif
}


char* wps_default_skin(enum screen_type screen);
char* default_radio_skin(enum screen_type screen);

struct wps_state     wps_state               = { .id3 = NULL };
static struct gui_skin_helper {
    int (*preproccess)(enum screen_type screen, struct wps_data *data);
    int (*postproccess)(enum screen_type screen, struct wps_data *data);
    char* (*default_skin)(enum screen_type screen);
} skin_helpers[SKINNABLE_SCREENS_COUNT] = {
    [CUSTOM_STATUSBAR] = { sb_preproccess, sb_postproccess, sb_create_from_settings },
    [WPS] = { NULL, NULL, wps_default_skin },
#if CONFIG_TUNER
    [FM_SCREEN] = { NULL, NULL, default_radio_skin }
#endif
};
    
static struct gui_skin {
    struct gui_wps      gui_wps;
    struct wps_data     data;
    char                *buffer_start;
    size_t              buffer_usage;
    
    bool                needs_full_update;
} skins[SKINNABLE_SCREENS_COUNT][NB_SCREENS];


void gui_sync_skin_init(void)
{
    int i, j;
    for(j=0; j<SKINNABLE_SCREENS_COUNT; j++)
    {
        FOR_NB_SCREENS(i)
        {
            skins[j][i].buffer_start = NULL;
            skins[j][i].needs_full_update = true;
#ifdef HAVE_ALBUMART
            skins[j][i].data.albumart = NULL;
            skins[j][i].data.playback_aa_slot = -1;
#endif
            skins[j][i].gui_wps.data = &skins[j][i].data;
            skins[j][i].data.wps_loaded = false;
            skins[j][i].gui_wps.display = &screens[i];
        }
    }
}

void skin_load(enum skinnable_screens skin, enum screen_type screen,
               const char *buf, bool isfile)
{
    bool loaded = false;
    
    if (skin_helpers[skin].preproccess)
        skin_helpers[skin].preproccess(screen, &skins[skin][screen].data);
    
    if (buf && *buf)
        loaded = skin_data_load(screen, &skins[skin][screen].data, buf, isfile);

    if (!loaded && skin_helpers[skin].default_skin)
        loaded = skin_data_load(screen, &skins[skin][screen].data,
                                skin_helpers[skin].default_skin(screen), false);
    
    skins[skin][screen].needs_full_update = true;
    if (skin_helpers[skin].postproccess)
        skin_helpers[skin].postproccess(screen, &skins[skin][screen].data);
}

static bool loading_a_sbs = false;
struct gui_wps *skin_get_gwps(enum skinnable_screens skin, enum screen_type screen)
{
    if (!loading_a_sbs && skins[skin][screen].data.wps_loaded == false)
    {
        char buf[MAX_PATH*2];
        char *setting = NULL, *ext = NULL;
        switch (skin)
        {
            case CUSTOM_STATUSBAR:
#ifdef HAVE_LCD_BITMAP
                if (skins_initialising)
                {
                    /* still loading, buffers not initialised yet,
                     * viewport manager calls into the sbs code, not really
                     * caring if the sbs has loaded or not, so just return
                     * the gwps, this is safe. */
                    return &skins[skin][screen].gui_wps;
                }
                /* during the sbs load it will call skin_get_gwps() a few times
                 * which will eventually stkov the viewportmanager, so make
                 * sure we don't let that happen */
                loading_a_sbs = true;
#if defined(HAVE_REMOTE_LCD) && NB_SCREENS > 1
                if (screen == SCREEN_REMOTE)
                {
                    setting = global_settings.rsbs_file;
                    ext = "rsbs";
                }
                else
#endif
                {
                    setting = global_settings.sbs_file;
                    ext = "sbs";
                }
#else
                return &skins[skin][screen].gui_wps;
#endif /* HAVE_LCD_BITMAP */
                break;
            case WPS:
#if defined(HAVE_REMOTE_LCD) && NB_SCREENS > 1
                if (screen == SCREEN_REMOTE)
                {
                    setting = global_settings.rwps_file;
                    ext = "rwps";
                }
                else
#endif
                {
                    setting = global_settings.wps_file;
                    ext = "wps";
                }
                break;
#if CONFIG_TUNER
            case FM_SCREEN:
#if defined(HAVE_REMOTE_LCD) && NB_SCREENS > 1
                if (screen == SCREEN_REMOTE)
                {
                    setting = global_settings.rfms_file;
                    ext = "rfms";
                }
                else
#endif
                {
                    setting = global_settings.fms_file;
                    ext = "fms";
                }
                break;
#endif
            default:
                return NULL;
        }
        
        buf[0] = '\0'; /* force it to reload the default */
        if (strcmp(setting, "rockbox_failsafe"))
        {
            snprintf(buf, sizeof buf, WPS_DIR "/%s.%s", setting, ext);
        }
        cpu_boost(true);
        skin_load(skin, screen, buf, true);
        cpu_boost(false);
        loading_a_sbs = false;
    }
        
    return &skins[skin][screen].gui_wps;
}

struct wps_state *skin_get_global_state(void)
{
    return &wps_state;
}

/* This is called to find out if we the screen needs a full update.
 * if true you MUST do a full update as the next call will return false */
bool skin_do_full_update(enum skinnable_screens skin,
                            enum screen_type screen)
{
    bool ret = skins[skin][screen].needs_full_update;
    skins[skin][screen].needs_full_update = false;
    return ret;
}

/* tell a skin to do a full update next time */
void skin_request_full_update(enum skinnable_screens skin)
{
    int i;
    FOR_NB_SCREENS(i)
        skins[skin][i].needs_full_update = true;
}
