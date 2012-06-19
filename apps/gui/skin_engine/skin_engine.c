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
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "skin_engine.h"
#include "skin_buffer.h"
#include "statusbar-skinned.h"

#define FAILSAFENAME "rockbox_failsafe"

void skin_data_free_buflib_allocs(struct wps_data *wps_data);
char* wps_default_skin(enum screen_type screen);
char* default_radio_skin(enum screen_type screen);
static bool skins_initialised = false;

static char* get_skin_filename(char *buf, size_t buf_size,
                               enum skinnable_screens skin, enum screen_type screen);

struct wps_state     wps_state               = { .id3 = NULL };
static struct gui_skin_helper {
    int (*preproccess)(enum screen_type screen, struct wps_data *data);
    int (*postproccess)(enum screen_type screen, struct wps_data *data);
    char* (*default_skin)(enum screen_type screen);
    bool load_on_boot;
} skin_helpers[SKINNABLE_SCREENS_COUNT] = {
#ifdef HAVE_LCD_BITMAP
    [CUSTOM_STATUSBAR] = { sb_preproccess, sb_postproccess, sb_create_from_settings, true },
#endif
    [WPS] = { NULL, NULL, wps_default_skin, true },
#if CONFIG_TUNER
    [FM_SCREEN] = { NULL, NULL, default_radio_skin, false }
#endif
};

static struct gui_skin {
    char                filename[MAX_PATH];
    struct gui_wps      gui_wps;
    struct wps_data     data;
    char                *buffer_start;
    size_t              buffer_usage;
    bool                failsafe_loaded;

    bool                needs_full_update;
} skins[SKINNABLE_SCREENS_COUNT][NB_SCREENS];


static void gui_skin_reset(struct gui_skin *skin)
{
    skin->filename[0] = '\0';
    skin->buffer_start = NULL;
    skin->failsafe_loaded = false;
    skin->needs_full_update = true;
    skin->gui_wps.data = &skin->data;
    memset(skin->gui_wps.data, 0, sizeof(struct wps_data));
    skin->data.wps_loaded = false;
    skin->data.buflib_handle = -1;
    skin->data.tree = -1;
#ifdef HAVE_TOUCHSCREEN
    skin->data.touchregions = -1;
#endif
#ifdef HAVE_SKIN_VARIABLES
    skin->data.skinvars = -1;
#endif
#ifdef HAVE_LCD_BITMAP
    skin->data.font_ids = -1;
    skin->data.images = -1;
#endif
#ifdef HAVE_ALBUMART
    skin->data.albumart = -1;
    skin->data.playback_aa_slot = -1;
#endif
#ifdef HAVE_BACKDROP_IMAGE
    skin->gui_wps.data->backdrop_id = -1;
#endif
}

void gui_sync_skin_init(void)
{
    int j;
    for(j=0; j<SKINNABLE_SCREENS_COUNT; j++)
    {
        FOR_NB_SCREENS(i)
        {
            skin_data_free_buflib_allocs(&skins[j][i].data);
            gui_skin_reset(&skins[j][i]);
            skins[j][i].gui_wps.display = &screens[i];
        }
    }
}

void skin_unload_all(void)
{
    gui_sync_skin_init();
}

void settings_apply_skins(void)
{
    int i;
    char filename[MAX_PATH];
    static bool first_run = true;

#ifdef HAVE_LCD_BITMAP
    skin_backdrop_init();
#endif
    skins_initialised = true;

    /* Make sure each skin is loaded */
    for (i=0; i<SKINNABLE_SCREENS_COUNT; i++)
    {
        FOR_NB_SCREENS(j)
        {
            get_skin_filename(filename, MAX_PATH, i,j);

            if (!first_run)
            {
                skin_data_free_buflib_allocs(&skins[i][j].data);
#ifdef HAVE_BACKDROP_IMAGE
                if (skins[i][j].data.backdrop_id >= 0)
                    skin_backdrop_unload(skins[i][j].data.backdrop_id);
#endif
            }
            gui_skin_reset(&skins[i][j]);
            skins[i][j].gui_wps.display = &screens[j];
            if (skin_helpers[i].load_on_boot)
                skin_get_gwps(i, j);
        }
    }
    first_run = false;
    viewportmanager_theme_changed(THEME_STATUSBAR);
#ifdef HAVE_BACKDROP_IMAGE
    FOR_NB_SCREENS(i)
        skin_backdrop_show(sb_get_backdrop(i));
#endif
}

void skin_load(enum skinnable_screens skin, enum screen_type screen,
               const char *buf, bool isfile)
{
    bool loaded = false;

    if (skin_helpers[skin].preproccess)
        skin_helpers[skin].preproccess(screen, &skins[skin][screen].data);

    if (buf && *buf)
        loaded = skin_data_load(screen, &skins[skin][screen].data, buf, isfile);
    if (loaded)
        strcpy(skins[skin][screen].filename, buf);

    if (!loaded && skin_helpers[skin].default_skin)
    {
        loaded = skin_data_load(screen, &skins[skin][screen].data,
                                skin_helpers[skin].default_skin(screen), false);
        skins[skin][screen].failsafe_loaded = loaded;
    }

    skins[skin][screen].needs_full_update = true;
    if (skin_helpers[skin].postproccess)
        skin_helpers[skin].postproccess(screen, &skins[skin][screen].data);
#ifdef HAVE_BACKDROP_IMAGE
    if (loaded)
        skin_backdrops_preload();
#endif
}

static char* get_skin_filename(char *buf, size_t buf_size,
                                enum skinnable_screens skin, enum screen_type screen)
{
    (void)screen;
    char *setting = NULL, *ext = NULL;
    switch (skin)
    {
#ifdef HAVE_LCD_BITMAP
        case CUSTOM_STATUSBAR:
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
            break;
#endif
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
    if (strcmp(setting, FAILSAFENAME) && strcmp(setting, "-"))
    {
        snprintf(buf, buf_size, WPS_DIR "/%s.%s", setting, ext);
    }
    return buf;
}

struct gui_wps *skin_get_gwps(enum skinnable_screens skin, enum screen_type screen)
{
#ifdef HAVE_LCD_BITMAP
    if (skin == CUSTOM_STATUSBAR && !skins_initialised)
        return &skins[skin][screen].gui_wps;
#endif

    if (skins[skin][screen].data.wps_loaded == false)
    {
        char filename[MAX_PATH];
        char *buf = get_skin_filename(filename, MAX_PATH, skin, screen);
        cpu_boost(true);
        skins[skin][screen].filename[0] = '\0';
        skin_load(skin, screen, buf, true);
        cpu_boost(false);
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
    FOR_NB_SCREENS(i)
        skins[skin][i].needs_full_update = true;
}
