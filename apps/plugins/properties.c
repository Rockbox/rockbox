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

static int props_type;

static struct mp3entry id3;
static int mul_id3_count;
static int skipped_count;

static char str_filename[MAX_PATH];
static char str_dirname[MAX_PATH];
static char str_size[64];
static char str_dircount[64];
static char str_filecount[64];
static char str_audio_filecount[64];
static char str_date[64];
static char str_time[64];

static unsigned long display_size;
static int32_t lang_size_unit;
static struct tm tm;

#define NUM_FILE_PROPERTIES 5
#define NUM_PLAYLIST_PROPERTIES 1 + NUM_FILE_PROPERTIES
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
#define NUM_AUDIODIR_PROPERTIES 1 + NUM_DIR_PROPERTIES
static const unsigned char* const props_dir[] =
{
    ID2P(LANG_PROPERTIES_PATH),       str_dirname,
    ID2P(LANG_PROPERTIES_SUBDIRS),    str_dircount,
    ID2P(LANG_PROPERTIES_FILES),      str_filecount,
    ID2P(LANG_PROPERTIES_SIZE),       str_size,

    ID2P(LANG_MENU_SHOW_ID3_INFO),    str_audio_filecount,
};

static bool file_properties(const char* selected_file)
{
    bool found = false;
    struct dirent* entry;
    DIR* dir = rb->opendir(str_dirname);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            struct dirinfo info = rb->dir_get_info(dir, entry);
            if(!rb->strcmp(entry->d_name, str_filename))
            {
                display_size = human_size(info.size, &lang_size_unit);
                rb->snprintf(str_size, sizeof str_size, "%lu %s",
                             display_size, rb->str(lang_size_unit));
                rb->gmtime_r(&info.mtime, &tm);
                rb->snprintf(str_date, sizeof str_date, "%04d/%02d/%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
                rb->snprintf(str_time, sizeof str_time, "%02d:%02d:%02d",
                    tm.tm_hour, tm.tm_min, tm.tm_sec);

                if (props_type != PROPS_PLAYLIST && rb->get_metadata(&id3, -1, selected_file))
                    props_type = PROPS_ID3;
                found = true;
                break;
            }
        }
        rb->closedir(dir);
    }
    return found;
}


static bool dir_properties(const char* selected_file, struct dir_stats *stats,
                           bool (*id3_cb)(const char*))
{
    bool success;

    rb->strlcpy(stats->dirname, selected_file, sizeof(stats->dirname));
    if (id3_cb)
        rb->splash_progress_set_delay(HZ / 2); /* hide progress bar for 0.5s */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    success = collect_dir_stats(stats, id3_cb);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    if (!success)
        return false;

    if (!id3_cb)
    {
        rb->strlcpy(str_dirname, selected_file, sizeof(str_dirname));
        rb->snprintf(str_dircount, sizeof str_dircount, "%d", stats->dir_count);
        rb->snprintf(str_filecount, sizeof str_filecount, "%d", stats->file_count);
        rb->snprintf(str_audio_filecount, sizeof str_filecount, "%d",
                     stats->audio_file_count);
        display_size = human_size(stats->byte_count, &lang_size_unit);
        rb->snprintf(str_size, sizeof str_size, "%lu %s", display_size,
                     rb->str(lang_size_unit));
    }
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
        rb->strlcpy(buffer, selected_item >= (int)(ARRAY_SIZE(props_dir)) ? "ERROR" :
                                (char *) p2str(props_dir[selected_item]), buffer_len);
    else
        rb->strlcpy(buffer, selected_item >= (int)(ARRAY_SIZE(props_file)) ? "ERROR" :
                                (char *) p2str(props_file[selected_item]), buffer_len);

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
        if (str_dirname[0] == '/')
        {
            char *start = str_dirname;
            char *ptr;
            while (0 != (ptr = rb->strchr(start, '/')))
            {
                *ptr = '\0';
                rb->talk_dir_or_spell(str_dirname, NULL, true);
                *ptr = '/';
                rb->talk_id(VOICE_CHAR_SLASH, true);
                start = ptr + 1;
            }
            if (*start)
                rb->talk_dir_or_spell(str_dirname, NULL, true);
        }
        else
        {
            rb->talk_spell(str_dirname, true);
        }
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

static int browse_file_or_dir(struct dir_stats *stats)
{
    struct gui_synclist properties_lists;
    int button, nb_items;

    if (props_type == PROPS_FILE)
        nb_items = NUM_FILE_PROPERTIES;
    else if (props_type == PROPS_PLAYLIST)
        nb_items = NUM_PLAYLIST_PROPERTIES;
    else if (stats->audio_file_count)
        nb_items = NUM_AUDIODIR_PROPERTIES;
    else
        nb_items = NUM_DIR_PROPERTIES;

    nb_items *= 2;

    rb->gui_synclist_init(&properties_lists, &get_props, stats, false, 2, NULL);
    rb->gui_synclist_set_title(&properties_lists,
                               rb->str(props_type == PROPS_DIR ?
                                                LANG_PROPERTIES_DIRECTORY_PROPERTIES :
                                                LANG_PROPERTIES_FILE_PROPERTIES),
                               NOICON);
    rb->gui_synclist_set_icon_callback(&properties_lists, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&properties_lists, speak_property_selection);
    rb->gui_synclist_set_nb_items(&properties_lists, nb_items);
    rb->gui_synclist_select_item(&properties_lists, 0);
    rb->gui_synclist_draw(&properties_lists);
    rb->gui_synclist_speak_item(&properties_lists);

    while(true)
    {
        button = rb->get_action(CONTEXT_LIST, HZ);
        /* HZ so the status bar redraws corectly */
        if (rb->gui_synclist_do_button(&properties_lists,&button))
            continue;
        switch(button)
        {
            case ACTION_STD_OK:
                if ((props_type == PROPS_PLAYLIST || props_type == PROPS_DIR) &&
                      rb->gui_synclist_get_sel_pos(&properties_lists)
                        == (props_type == PROPS_DIR ?
                            ARRAY_SIZE(props_dir) : ARRAY_SIZE(props_file)) - 2)
                    return -1;
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

static bool determine_file_or_dir(void)
{
    struct dirent* entry;
    DIR* dir = rb->opendir(str_dirname);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            if(!rb->strcmp(entry->d_name, str_filename))
            {
                struct dirinfo info = rb->dir_get_info(dir, entry);
                props_type = info.attribute & ATTR_DIRECTORY ? PROPS_DIR : PROPS_FILE;
                rb->closedir(dir);
                return true;
            }
        }
        rb->closedir(dir);
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

static bool has_pl_extension(const char* filename)
{
    char *dot = rb->strrchr(filename, '.');
    return (dot && (!rb->strcasecmp(dot, ".m3u") || !rb->strcasecmp(dot, ".m3u8")));
}

/* Assemble track info from a database table, the contents of a playlist file, or a dir */
static bool assemble_track_info(const char *filename, struct dir_stats *stats)
{
    if (!filename)
        props_type = PROPS_MUL_ID3;
    mul_id3_count = skipped_count = 0;

#ifdef HAVE_TAGCACHE
    if (props_type == PROPS_MUL_ID3 && !rb->tagtree_subentries_do_action(&mul_id3_add))
        return false;
    else
#endif
    {
        if (props_type == PROPS_DIR)
        {
            if (!dir_properties(filename, stats, &mul_id3_add))
                return false;
        }
        else if(props_type == PROPS_PLAYLIST)
        {
            if (!rb->playlist_entries_iterate(filename, NULL, &mul_id3_add))
                return false;
        }
    }

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
    int ret;
    static struct dir_stats stats;
    const char *file = parameter;
    if(!parameter)
        return PLUGIN_ERROR;

#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif
    if (file[0] == '/') /* single file or folder selected */
    {
        const char* file_name = rb->strrchr(file, '/') + 1;
        const int dirlen = (file_name - file);
        rb->strlcpy(str_dirname, file, dirlen + 1);
        rb->snprintf(str_filename, sizeof str_filename, "%s", file + dirlen);

        if(!determine_file_or_dir())
        {
            /* weird: we couldn't find the entry. This Should Never Happen (TM) */
            rb->splashf(0, "File/Dir not found: %s", file);
            rb->action_userabort(TIMEOUT_BLOCK);
            return PLUGIN_OK;
        }

        if (props_type == PROPS_FILE && has_pl_extension(file))
            props_type = PROPS_PLAYLIST;

        if(!(props_type == PROPS_DIR ?
             dir_properties(file, &stats, NULL) : file_properties(file)))
        {
            if (!stats.canceled)
            {
                /* something went wrong (to do: tell user what it was (nesting,...) */
                rb->splash(0, ID2P(LANG_PROPERTIES_FAIL));
                rb->action_userabort(TIMEOUT_BLOCK);
            }
            return PLUGIN_OK;
        }
    }
             /* database table selected */
    else if (rb->strcmp(file, MAKE_ACT_STR(ACTIVITY_DATABASEBROWSER))
             || !assemble_track_info(NULL, NULL))
        return PLUGIN_ERROR;

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);

    if (props_type == PROPS_ID3)
        ret = rb->browse_id3(&id3, 0, 0, &tm, 1);   /* Track Info for single file */
    else if (props_type == PROPS_MUL_ID3)
        ret = rb->browse_id3(&id3, 0, 0, NULL, mul_id3_count); /* database tracks */
    else if ((ret = browse_file_or_dir(&stats)) < 0)
        ret = assemble_track_info(file, &stats) ?    /* playlist or folder tracks */
                rb->browse_id3(&id3, 0, 0, NULL, mul_id3_count) :
                (stats.canceled ? 0 : -1);

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);

    switch (ret)
    {
        case 1:
            return PLUGIN_USB_CONNECTED;
        case -1:
            return PLUGIN_ERROR;
        default:
            return PLUGIN_OK;
    }
}
