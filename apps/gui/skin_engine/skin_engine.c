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
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include "inttypes.h"
#include "config.h"
#include "action.h"
#include "crc32.h"
#include "settings.h"
#include "rbpaths.h"
#include "wps.h"
#include "file.h"
#include "misc.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "gui/list.h"
#include "skin_engine.h"
#include "skin_buffer.h"
#include "statusbar-skinned.h"
#include "wps_internals.h"

#define FAILSAFENAME "rockbox_failsafe"

void skin_data_free_buflib_allocs(struct wps_data *wps_data);
#ifdef HAVE_ALBUMART
void playback_release_aa_slot(int slot);
#endif
char* wps_default_skin(enum screen_type screen);
char* default_radio_skin(enum screen_type screen);
static bool skins_initialised = false;

static char* get_skin_filename(char *buf, size_t buf_size,
                               enum skinnable_screens skin, enum screen_type screen);

static struct gui_skin_helper {
    int (*preproccess)(enum screen_type screen, struct wps_data *data);
    int (*postproccess)(enum screen_type screen, struct wps_data *data);
    char* (*default_skin)(enum screen_type screen);
    bool load_on_boot;
} skin_helpers[SKINNABLE_SCREENS_COUNT] = {
    [CUSTOM_STATUSBAR] = { sb_preproccess, sb_postproccess, sb_create_from_settings, true },
    [WPS] = { NULL, NULL, wps_default_skin, true },
#if CONFIG_TUNER
    [FM_SCREEN] = { NULL, NULL, default_radio_skin, false }
#endif
};

static struct gui_skin {
    struct gui_wps      gui_wps;
    struct wps_data     data;
    struct skin_stats   stats;
    bool                failsafe_loaded;

    bool                needs_full_update;
} skins[SKINNABLE_SCREENS_COUNT][NB_SCREENS];

int skin_get_num_skins(void)
{
    return SKINNABLE_SCREENS_COUNT;
}

struct skin_stats *skin_get_stats(int number, int screen)
{
    return &skins[number][screen].stats;
}

static void gui_skin_reset(struct gui_skin *skin)
{
    struct wps_data *data;
    skin->failsafe_loaded = false;
    skin->needs_full_update = true;
    skin->gui_wps.data = data = &skin->data;
#ifdef HAVE_ALBUMART
    struct skin_albumart *aa_save;
    unsigned char *buffer = get_skin_buffer(data);
    /* copy to temp var to protect against memset */
    if (buffer && (aa_save = SKINOFFSETTOPTR(buffer, data->albumart)))
    {
        short old_width, old_height;
        old_width = aa_save->width;
        old_height = aa_save->height;
        memset(data, 0, sizeof(struct wps_data));
        data->last_albumart_width = old_width;
        data->last_albumart_height = old_height;
    }
    else
#endif
        memset(data, 0, sizeof(struct wps_data));
    skin->data.wps_loaded = false;
    skin->data.buflib_handle = -1;
    skin->data.tree = -1;
#ifdef HAVE_TOUCHSCREEN
    skin->data.touchregions = -1;
#endif
#ifdef HAVE_SKIN_VARIABLES
    skin->data.skinvars = -1;
#endif
    skin->data.font_ids = -1;
    skin->data.images = -1;
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
#ifdef HAVE_BACKDROP_IMAGE
            if (skins[j][i].data.backdrop_id != -1)
                skin_backdrop_unload(skins[j][i].data.backdrop_id);
#endif
            gui_skin_reset(&skins[j][i]);
            skins[j][i].gui_wps.display = &screens[i];
        }
    }
}

void skin_unload_all(void)
{
    gui_sync_skin_init();
}

static void skin_reset_buffers(int item, int screen)
{
    skin_data_free_buflib_allocs(&skins[item][screen].data);
#ifdef HAVE_ALBUMART
    if (skins[item][screen].data.playback_aa_slot >= 0)
        playback_release_aa_slot(skins[item][screen].data.playback_aa_slot);
#endif
#ifdef HAVE_BACKDROP_IMAGE
    if (skins[item][screen].data.backdrop_id >= 0)
        skin_backdrop_unload(skins[item][screen].data.backdrop_id);
#endif
}

void settings_apply_skins(void)
{
    int i;
    char filename[MAX_PATH];

    if (audio_status() & AUDIO_STATUS_PLAY)
        audio_stop();

    bool first_run = skin_backdrop_init();
    skins_initialised = true;

    /* Make sure each skin is loaded */
    for (i=0; i<SKINNABLE_SCREENS_COUNT; i++)
    {
        FOR_NB_SCREENS(j)
        {
            get_skin_filename(filename, MAX_PATH, i,j);

            if (!first_run)
            {
                skin_reset_buffers(i, j);
            }
            gui_skin_reset(&skins[i][j]);
            skins[i][j].gui_wps.display = &screens[j];
            if (skin_helpers[i].load_on_boot)
                skin_get_gwps(i, j);
        }
    }

    /* any backdrop that was loaded with "-" has to be reloaded because
     * the setting may have changed */
    skin_backdrop_load_setting();
    viewportmanager_theme_changed(THEME_STATUSBAR);

    FOR_NB_SCREENS(i)
        skin_backdrop_show(sb_get_backdrop(i));
}

void skin_load(enum skinnable_screens skin, enum screen_type screen,
               const char *buf, bool isfile)
{
    bool loaded = false;

    if (skin_helpers[skin].preproccess)
        skin_helpers[skin].preproccess(screen, &skins[skin][screen].data);

    if (buf && *buf)
        loaded = skin_data_load(screen, &skins[skin][screen].data, buf, isfile,
                                &skins[skin][screen].stats);

    if (!loaded && skin_helpers[skin].default_skin)
    {
        loaded = skin_data_load(screen, &skins[skin][screen].data,
                                skin_helpers[skin].default_skin(screen), false,
                                &skins[skin][screen].stats);
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
    if (skin == CUSTOM_STATUSBAR && !skins_initialised)
        return &skins[skin][screen].gui_wps;

    if (skins[skin][screen].data.wps_loaded == false)
    {
        char filename[MAX_PATH];
        char *buf = get_skin_filename(filename, MAX_PATH, skin, screen);
        cpu_boost(true);
        skin_load(skin, screen, buf, true);
        cpu_boost(false);
    }
    return &skins[skin][screen].gui_wps;
}

/* This is called to find out if we the screen needs a full update.
 * if true you MUST do a full update as the next call will return false */
bool skin_do_full_update(enum skinnable_screens skin,
                            enum screen_type screen)
{
    struct viewport *vp = *(screens[screen].current_viewport);

    bool vp_is_dirty = ((vp->flags & VP_FLAG_VP_SET_CLEAN) == VP_FLAG_VP_DIRTY) &&
                       get_current_activity() == ACTIVITY_WPS;

    bool ret = (skins[skin][screen].needs_full_update || vp_is_dirty);
    skins[skin][screen].needs_full_update = false;
    return ret;
}

/* tell a skin to do a full update next time */
void skin_request_full_update(enum skinnable_screens skin)
{
    FOR_NB_SCREENS(i)
        skins[skin][i].needs_full_update = true;
}

bool dbg_skin_engine(void)
{
    struct simplelist_info info;
    int i, total = 0;
#if defined(HAVE_BACKDROP_IMAGE)
    int ref_count;
    char *path;
    size_t bytes;
    int path_prefix_len = strlen(ROCKBOX_DIR "/wps/");
#endif
    simplelist_info_init(&info, "Skin engine usage", 0, NULL);
    simplelist_set_line_count(0);
    FOR_NB_SCREENS(j) {
#if NB_SCREENS > 1
        simplelist_addline("%s display:",
                           j == 0 ? "Main" : "Remote");
#endif
        for (i = 0; i < skin_get_num_skins(); i++) {
            struct skin_stats *stats = skin_get_stats(i, j);
            if (stats->buflib_handles)
            {
                simplelist_addline("Skin ID: %d, %zd allocations",
                        i, stats->buflib_handles);
                simplelist_addline("\tskin: %zd bytes",
                        stats->tree_size);
                simplelist_addline("\tImages: %zd bytes",
                        stats->images_size);
                simplelist_addline("\tTotal: %zd bytes",
                        stats->tree_size + stats->images_size);
                total += stats->tree_size + stats->images_size;
            }
        }
    }
    simplelist_addline("Skin total usage: %d bytes", total);
#if defined(HAVE_BACKDROP_IMAGE)
    simplelist_addline("Backdrop Images:");
    i = 0;
    while (skin_backdrop_get_debug(i++, &path, &ref_count, &bytes)) {
        if (ref_count > 0) {

            if (!strncasecmp(path, ROCKBOX_DIR "/wps/", path_prefix_len))
                path += path_prefix_len;
            simplelist_addline("%s", path);
            simplelist_addline("\tref_count: %d", ref_count);
            simplelist_addline("\tsize: %zd", bytes);
            total += bytes;
        }
    }
    simplelist_addline("Total usage: %d bytes", total);
#endif
    return simplelist_show_list(&info);
}
