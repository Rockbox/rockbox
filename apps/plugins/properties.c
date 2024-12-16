/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Peter D'Hoye
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
#include "plugin.h"
#include "lib/mul_id3.h"
#include "lib/simple_viewer.h"

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

enum props_types {
    PROPS_FILE = 0,
    PROPS_PLAYLIST,
    PROPS_ID3,
    PROPS_MUL_ID3,
    PROPS_DIR
};

static struct gui_synclist properties_lists;
static struct mp3entry id3;
static struct tm tm;
static unsigned long display_size;
static int32_t lang_size_unit;
static int props_type, mul_id3_count, skipped_count;

static char str_filename[MAX_PATH], str_dirname[MAX_PATH],
            str_size[64], str_dircount[64], str_filecount[64],
            str_audio_filecount[64], str_date[64], str_time[64];


#define NUM_FILE_PROPERTIES 5
#define NUM_PLAYLIST_PROPERTIES (1 + NUM_FILE_PROPERTIES)
static const unsigned char* const props_file[] =
{
    ID2P(LANG_PROPERTIES_PATH),       str_dirname,
    ID2P(LANG_PROPERTIES_FILENAME),   str_filename,
    ID2P(LANG_PROPERTIES_SIZE),       str_size,
    ID2P(LANG_PROPERTIES_DATE),       str_date,
    ID2P(LANG_PROPERTIES_TIME),       str_time,

    ID2P(LANG_MENU_SHOW_ID3_INFO),    "...",
};

#define NUM_DIR_PROPERTIES 4
#define NUM_AUDIODIR_PROPERTIES (1 + NUM_DIR_PROPERTIES)
static const unsigned char* const props_dir[] =
{
    ID2P(LANG_PROPERTIES_PATH),       str_dirname,
    ID2P(LANG_PROPERTIES_SUBDIRS),    str_dircount,
    ID2P(LANG_PROPERTIES_FILES),      str_filecount,
    ID2P(LANG_PROPERTIES_SIZE),       str_size,

    ID2P(LANG_MENU_SHOW_ID3_INFO),    str_audio_filecount,
};

static bool dir_properties(const char* selected_file, struct dir_stats *stats)
{
    rb->strlcpy(stats->dirname, selected_file, sizeof(stats->dirname));

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
#endif
    bool success = collect_dir_stats(stats, NULL);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
    if (!success)
        return false;

    rb->strlcpy(str_dirname, selected_file, sizeof(str_dirname));
    rb->snprintf(str_dircount, sizeof str_dircount, "%d", stats->dir_count);
    rb->snprintf(str_filecount, sizeof str_filecount, "%d", stats->file_count);
    rb->snprintf(str_audio_filecount, sizeof str_filecount, "%d",
                 stats->audio_file_count);
    display_size = human_size(stats->byte_count, &lang_size_unit);
    rb->snprintf(str_size, sizeof str_size, "%lu %s", display_size,
                 rb->str(lang_size_unit));
    return true;
}

static const unsigned char* p2str(const unsigned char* p)
{
    int id = P2ID(p);
    return (id != -1) ? rb->str(id) : p;
}

static const char * get_props(int selected_item, void* data,
                              char *buffer, size_t buffer_len)
{
    (void)data;
    if (PROPS_DIR == props_type)
        rb->strlcpy(buffer, selected_item >= (int)(ARRAY_SIZE(props_dir)) ?
                    "ERROR" : (char *) p2str(props_dir[selected_item]), buffer_len);
    else
        rb->strlcpy(buffer, selected_item >= (int)(ARRAY_SIZE(props_file)) ?
                    "ERROR" : (char *) p2str(props_file[selected_item]), buffer_len);
    return buffer;
}

static int speak_property_selection(int selected_item, void *data)
{
    struct dir_stats *stats = data;
    int32_t id = P2ID((props_type == PROPS_DIR ?
                       props_dir : props_file)[selected_item]);
    rb->talk_id(id, false);
    switch (id)
    {
    case LANG_PROPERTIES_PATH:
        rb->talk_fullpath(str_dirname, true);
        break;
    case LANG_PROPERTIES_FILENAME:
        rb->talk_file_or_spell(str_dirname, str_filename, NULL, true);
        break;
    case LANG_PROPERTIES_SIZE:
        rb->talk_number(display_size, true);
        rb->talk_id(lang_size_unit, true);
        break;
    case LANG_PROPERTIES_DATE:
        rb->talk_date(&tm, true);
        break;
    case LANG_PROPERTIES_TIME:
        rb->talk_time(&tm, true);
        break;
    case LANG_PROPERTIES_SUBDIRS:
        rb->talk_number(stats->dir_count, true);
        break;
    case LANG_PROPERTIES_FILES:
        rb->talk_number(stats->file_count, true);
        break;
    default:
        rb->talk_spell(props_file[selected_item + 1], true);
        break;
    }
    return 0;
}

static void setup_properties_list(struct dir_stats *stats)
{
    int nb_props;
    if (props_type == PROPS_FILE)
        nb_props = NUM_FILE_PROPERTIES;
    else if (props_type == PROPS_PLAYLIST)
        nb_props = NUM_PLAYLIST_PROPERTIES;
    else
        nb_props = NUM_DIR_PROPERTIES;

    rb->gui_synclist_init(&properties_lists, &get_props, stats, false, 2, NULL);
    rb->gui_synclist_set_title(&properties_lists,
                               rb->str(props_type == PROPS_DIR ?
                                       LANG_PROPERTIES_DIRECTORY_PROPERTIES :
                                       LANG_PROPERTIES_FILE_PROPERTIES),
                               NOICON);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&properties_lists, speak_property_selection);
    rb->gui_synclist_set_nb_items(&properties_lists, nb_props*2);
}

static int browse_file_or_dir(struct dir_stats *stats)
{
    if (props_type == PROPS_DIR && stats->audio_file_count)
        rb->gui_synclist_set_nb_items(&properties_lists, NUM_AUDIODIR_PROPERTIES*2);
    rb->gui_synclist_draw(&properties_lists);
    rb->gui_synclist_speak_item(&properties_lists);
    while(true)
    {
        int button = rb->get_action(CONTEXT_LIST, HZ);
        /* HZ so the status bar redraws corectly */
        if (rb->gui_synclist_do_button(&properties_lists, &button))
            continue;
        switch(button)
        {
            case ACTION_STD_OK:;
                int sel_pos = rb->gui_synclist_get_sel_pos(&properties_lists);

                /* "Show Track Info..." selected? */
                if ((props_type == PROPS_PLAYLIST || props_type == PROPS_DIR) &&
                    sel_pos == (props_type == PROPS_DIR ?
                                ARRAY_SIZE(props_dir) : ARRAY_SIZE(props_file)) - 2)
                    return -1;
                else
                {
                    /* Display field in fullscreen */
                    FOR_NB_SCREENS(i)
                        rb->viewportmanager_theme_enable(i, false, NULL);
                    if (props_type == PROPS_DIR)
                        view_text((char *) p2str(props_dir[sel_pos]),
                                  (char *)       props_dir[sel_pos + 1]);
                    else
                        view_text((char *) p2str(props_file[sel_pos]),
                                  (char *)       props_file[sel_pos + 1]);
                    FOR_NB_SCREENS(i)
                        rb->viewportmanager_theme_undo(i, false);

                    rb->gui_synclist_set_title(&properties_lists,
                               rb->str(props_type == PROPS_DIR ?
                                       LANG_PROPERTIES_DIRECTORY_PROPERTIES :
                                       LANG_PROPERTIES_FILE_PROPERTIES),
                               NOICON);
                    rb->gui_synclist_draw(&properties_lists);
                }
                break;
            case ACTION_STD_CANCEL:
                return 0;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
                break;
        }
    }
}

static bool determine_props_type(const char *file)
{
    if (file[0] == PATH_SEPCH)
    {
        const char* basename = rb->strrchr(file, PATH_SEPCH) + 1;
        const int dir_len = (basename - file);
        if ((int) sizeof(str_dirname) <= dir_len)
            return false;
        rb->strlcpy(str_dirname, file, dir_len + 1);
        rb->strlcpy(str_filename, basename, sizeof str_filename);
        struct dirent* entry;
        DIR* dir = rb->opendir(str_dirname);
        if (!dir)
            return false;
        while(0 != (entry = rb->readdir(dir)))
        {
            if(rb->strcmp(entry->d_name, str_filename))
                continue;

            struct dirinfo info = rb->dir_get_info(dir, entry);
            if (info.attribute & ATTR_DIRECTORY)
                props_type = PROPS_DIR;
            else
            {
                display_size = human_size(info.size, &lang_size_unit);
                rb->snprintf(str_size, sizeof str_size, "%lu %s",
                             display_size, rb->str(lang_size_unit));
                rb->gmtime_r(&info.mtime, &tm);
                rb->snprintf(str_date, sizeof str_date, "%04d/%02d/%02d",
                             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
                rb->snprintf(str_time, sizeof str_time, "%02d:%02d:%02d",
                             tm.tm_hour, tm.tm_min, tm.tm_sec);

                if (rb->filetype_get_attr(entry->d_name) == FILE_ATTR_M3U)
                    props_type = PROPS_PLAYLIST;
                else
                    props_type = rb->get_metadata(&id3, -1, file) ?
                                 PROPS_ID3 : PROPS_FILE;
            }
            rb->closedir(dir);
            return true;
        }
        rb->closedir(dir);
    }
    else if (!rb->strcmp(file, MAKE_ACT_STR(ACTIVITY_DATABASEBROWSER)))
    {
        props_type = PROPS_MUL_ID3;
        return true;
    }
    return false;
}

bool mul_id3_add(const char *file_name)
{
    if (!file_name || !rb->get_metadata(&id3, -1, file_name))
        skipped_count++;
    else
    {
        collect_id3(&id3, mul_id3_count == 0);
        mul_id3_count++;
    }
    return true;
}

/* Assemble track info from a dir, a playlist, or a database table */
static bool assemble_track_info(const char *filename, struct dir_stats *stats)
{
    if (props_type == PROPS_DIR)
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
#endif
        rb->strlcpy(stats->dirname, filename, sizeof(stats->dirname));
        rb->splash_progress_set_delay(HZ/2); /* hide progress bar for 0.5s */
        bool success = collect_dir_stats(stats, &mul_id3_add);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
        if (!success)
            return false;
    }
    else if(props_type == PROPS_PLAYLIST &&
            !rb->playlist_entries_iterate(filename, NULL, &mul_id3_add))
        return false;
#ifdef HAVE_TAGCACHE
    else if (props_type == PROPS_MUL_ID3 &&
             !rb->tagtree_subentries_do_action(&mul_id3_add))
        return false;
#endif

    if (mul_id3_count == 0)
    {
        rb->splashf(HZ*2, "None found");
        return false;
    }
    else if (mul_id3_count > 1) /* otherwise, the retrieved id3 can be used as-is */
        finalize_id3(&id3);

    if (skipped_count > 0)
        rb->splashf(HZ*2, "Skipped %d", skipped_count);

    return true;
}

enum plugin_status plugin_start(const void* parameter)
{
    static struct dir_stats stats;
    const char *file = parameter;
#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif
    int ret = file && determine_props_type(file);
    if (!ret)
    {
        rb->splashf(0, "Could not find: %s", file ?: "(NULL)");
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

    if (props_type == PROPS_MUL_ID3)
        ret = assemble_track_info(NULL, NULL);
    else if (props_type != PROPS_ID3)
    {
        setup_properties_list(&stats);               /* Show title during dir scan */
        if (props_type == PROPS_DIR)
            ret = dir_properties(file, &stats);
    }
    if (!ret)
    {
        if (!stats.canceled)
        {
            rb->splash(0, ID2P(LANG_PROPERTIES_FAIL));    /* TODO: describe error */
            rb->action_userabort(TIMEOUT_BLOCK);
        }
    }
    else if (props_type == PROPS_ID3)
        /* Track Info for single file */
        ret = rb->browse_id3(&id3, 0, 0, &tm, 1, &view_text);
    else if (props_type == PROPS_MUL_ID3)
        /* database tracks */
        ret = rb->browse_id3(&id3, 0, 0, NULL, mul_id3_count, &view_text);
    else if ((ret = browse_file_or_dir(&stats)) < 0)
        ret = assemble_track_info(file, &stats) ?
              /* playlist or folder tracks */
              rb->browse_id3(&id3, 0, 0, NULL, mul_id3_count, &view_text) :
              (stats.canceled ? 0 : -1);

    return ret == -1 ? PLUGIN_ERROR : ret == 1 ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
