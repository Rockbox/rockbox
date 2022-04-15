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

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

struct dir_stats {
    char dirname[MAX_PATH];
    int len;
    unsigned int dir_count;
    unsigned int file_count;
    unsigned long long byte_count;
};

enum props_types {
    PROPS_FILE = 0,
    PROPS_ID3,
    PROPS_DIR
};

static int props_type = PROPS_FILE;

static struct mp3entry id3;

static char str_filename[MAX_PATH];
static char str_dirname[MAX_PATH];
static char str_size[64];
static char str_dircount[64];
static char str_filecount[64];
static char str_date[64];
static char str_time[64];

static unsigned long nsize;
static int32_t size_unit;
static struct tm tm;

#define NUM_FILE_PROPERTIES 5
static const unsigned char* const props_file[] =
{
    ID2P(LANG_PROPERTIES_PATH),       str_dirname,
    ID2P(LANG_PROPERTIES_FILENAME),   str_filename,
    ID2P(LANG_PROPERTIES_SIZE),       str_size,
    ID2P(LANG_PROPERTIES_DATE),       str_date,
    ID2P(LANG_PROPERTIES_TIME),       str_time,
};

#define NUM_DIR_PROPERTIES 4
static const unsigned char* const props_dir[] =
{
    ID2P(LANG_PROPERTIES_PATH),       str_dirname,
    ID2P(LANG_PROPERTIES_SUBDIRS),    str_dircount,
    ID2P(LANG_PROPERTIES_FILES),      str_filecount,
    ID2P(LANG_PROPERTIES_SIZE),       str_size,
};

static const int32_t units[] =
{
    LANG_BYTE,
    LANG_KIBIBYTE,
    LANG_MEBIBYTE,
    LANG_GIBIBYTE
};

static unsigned human_size_log(unsigned long long size)
{
    const size_t n = sizeof(units)/sizeof(units[0]);

    unsigned i;
    /* margin set at 10K boundary: 10239 B +1 => 10 KB */
    for(i=0; i < n-1 && size >= 10*1024; i++)
        size >>= 10; /* div by 1024 */

    return i;
}

static bool file_properties(const char* selected_file)
{
    bool found = false;
    DIR* dir;
    struct dirent* entry;

    dir = rb->opendir(str_dirname);
    if (dir)
    {
        while(0 != (entry = rb->readdir(dir)))
        {
            struct dirinfo info = rb->dir_get_info(dir, entry);
            if(!rb->strcmp(entry->d_name, str_filename))
            {
                unsigned log;
                log = human_size_log((unsigned long)info.size);
                nsize = ((unsigned long)info.size) >> (log*10);
                size_unit = units[log];
                rb->snprintf(str_size, sizeof str_size, "%lu %s", nsize, rb->str(size_unit));
                rb->gmtime_r(&info.mtime, &tm);
                rb->snprintf(str_date, sizeof str_date, "%04d/%02d/%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
                rb->snprintf(str_time, sizeof str_time, "%02d:%02d:%02d",
                    tm.tm_hour, tm.tm_min, tm.tm_sec);

                int fd = rb->open(selected_file, O_RDONLY);
                if (fd >= 0)
                {
                    if (rb->get_metadata(&id3, fd, selected_file))
                        props_type = PROPS_ID3;

                    rb->close(fd);
                }
                found = true;
                break;
            }
        }
        rb->closedir(dir);
    }
    return found;
}

static bool _dir_properties(struct dir_stats *stats)
{
    /* recursively scan directories in search of files
       and informs the user of the progress */
    bool result;
    static long lasttick=0;
    int dirlen;
    DIR* dir;
    struct dirent* entry;

    result = true;
    dirlen = rb->strlen(stats->dirname);
    dir = rb->opendir(stats->dirname);
    if (!dir)
    {
        rb->splashf(HZ*2, "%s", stats->dirname);
        return false; /* open error */
    }

    /* walk through the directory content */
    while(result && (0 != (entry = rb->readdir(dir))))
    {
        struct dirinfo info = rb->dir_get_info(dir, entry);
        /* append name to current directory */
        rb->snprintf(stats->dirname+dirlen, stats->len-dirlen, "/%s",
                     entry->d_name);

        if (info.attribute & ATTR_DIRECTORY)
        {
            if (!rb->strcmp((char *)entry->d_name, ".") ||
                !rb->strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            stats->dir_count++; /* new directory */
            if (*rb->current_tick - lasttick > (HZ/8))
            {
                unsigned log;
                lasttick = *rb->current_tick;
                rb->lcd_clear_display();
                rb->lcd_puts(0,0,"SCANNING...");
                rb->lcd_puts(0,1,stats->dirname);
                rb->lcd_putsf(0,2,"Directories: %d", stats->dir_count);
                rb->lcd_putsf(0,3,"Files: %d", stats->file_count);
                log = human_size_log(stats->byte_count);
                rb->lcd_putsf(0,4,"Size: %lu %s",
                              (unsigned long)(stats->byte_count >> (10*log)),
                              rb->str(units[log]));
                rb->lcd_update();
            }

             /* recursion */
            result = _dir_properties(stats);
        }
        else
        {
            stats->file_count++; /* new file */
            stats->byte_count += info.size;
        }
        if(ACTION_STD_CANCEL == rb->get_action(CONTEXT_STD,TIMEOUT_NOBLOCK))
            result = false;
        rb->yield();
    }
    rb->closedir(dir);
    return result;
}

static bool dir_properties(const char* selected_file, struct dir_stats *stats)
{
    unsigned log;

    rb->strlcpy(stats->dirname, selected_file, MAX_PATH);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    if (!_dir_properties(stats))
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
        return false;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    rb->strlcpy(str_dirname, selected_file, MAX_PATH);
    rb->snprintf(str_dircount, sizeof str_dircount, "%d", stats->dir_count);
    rb->snprintf(str_filecount, sizeof str_filecount, "%d", stats->file_count);
    log = human_size_log(stats->byte_count);
    nsize = (long) (stats->byte_count >> (log*10));
    size_unit = units[log];
    rb->snprintf(str_size, sizeof str_size, "%ld %s", nsize, rb->str(size_unit));
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
    else if (PROPS_FILE == props_type)
        rb->strlcpy(buffer, selected_item >= (int)(ARRAY_SIZE(props_file)) ? "ERROR" :
                                (char *) p2str(props_file[selected_item]), buffer_len);

    return buffer;
}

static int speak_property_selection(int selected_item, void *data)
{
    struct dir_stats *stats = data;
    int32_t id = P2ID((props_type == PROPS_DIR  ? props_dir : props_file)[selected_item]);
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
        rb->talk_number(nsize, true);
        rb->talk_id(size_unit, true);
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
    int button;

    rb->gui_synclist_init(&properties_lists, &get_props, stats, false, 2, NULL);
    rb->gui_synclist_set_title(&properties_lists,
                               rb->str(props_type == PROPS_DIR ?
                                                LANG_PROPERTIES_DIRECTORY_PROPERTIES :
                                                LANG_PROPERTIES_FILE_PROPERTIES),
                               NOICON);
    rb->gui_synclist_set_icon_callback(&properties_lists, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&properties_lists, speak_property_selection);
    rb->gui_synclist_set_nb_items(&properties_lists,
                                  2 * (props_type == PROPS_FILE ? NUM_FILE_PROPERTIES :
                                                                  NUM_DIR_PROPERTIES));
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
            case ACTION_STD_CANCEL:
                return false;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
}

static bool determine_file_or_dir(void)
{
    DIR* dir;
    struct dirent* entry;

    dir = rb->opendir(str_dirname);
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

enum plugin_status plugin_start(const void* parameter)
{
    static struct dir_stats stats =
    {
        .len = MAX_PATH,
        .dir_count  = 0,
        .file_count  = 0,
        .byte_count  = 0,
    };

    const char *file = parameter;
    if(!parameter || (file[0] != '/')) return PLUGIN_ERROR;

#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif

    const char* file_name = rb->strrchr(file, '/') + 1;
    int dirlen = (file_name - file);

    rb->strlcpy(str_dirname, file, dirlen + 1);
    rb->snprintf(str_filename, sizeof str_filename, "%s", file+dirlen);

    if(!determine_file_or_dir())
    {
        /* weird: we couldn't find the entry. This Should Never Happen (TM) */
        rb->splashf(0, "File/Dir not found: %s", file);
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

    /* get the info depending on its_a_dir */
    if(!(props_type == PROPS_DIR ? dir_properties(file, &stats) : file_properties(file)))
    {
        /* something went wrong (to do: tell user what it was (nesting,...) */
        rb->splash(0, ID2P(LANG_PROPERTIES_FAIL));
        rb->action_userabort(TIMEOUT_BLOCK);
        return PLUGIN_OK;
    }

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);

    bool usb = props_type == PROPS_ID3 ? rb->browse_id3(&id3, 0, 0) :
                                             browse_file_or_dir(&stats);

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);

    return usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
