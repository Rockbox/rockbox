/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "string.h"
#include <ctype.h>

#include "settings.h"
#include "debug.h"
#include "lang.h"
#include "kernel.h"
#include "plugin.h"
#include "filetypes.h"
#include "screens.h"
#include "dir.h"
#include "file.h"
#include "splash.h"
#include "core_alloc.h"
#include "icons.h"
#include "logf.h"

/* max filetypes (plugins & icons stored here) */
#if CONFIG_CODEC == SWCODEC
#define MAX_FILETYPES 192
#else
#define MAX_FILETYPES 128
#endif
/* max viewer plugins */
#ifdef HAVE_LCD_BITMAP
#define MAX_VIEWERS 56
#else
#define MAX_VIEWERS 24
#endif

/* a table for the known file types */
static const struct filetype inbuilt_filetypes[] = {
    { "mp3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mp2", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpa", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
#if CONFIG_CODEC == SWCODEC
    /* Temporary hack to allow playlist creation */
    { "mp1", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ogg", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "oga", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wma", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wmv", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "asf", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wav", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "flac",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ac3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "a52", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wv",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "m4a", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "m4b", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mp4", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mod", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "shn", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "aif", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "aiff",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "spx" ,FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "opus",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "sid", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "adx", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "nsf", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "nsfe",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "spc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ape", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mac", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "sap" ,FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "rm",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ra",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "rmvb",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cmc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cm3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cmr", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cms", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "dmc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "dlt", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpt", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpd", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "rmt", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tmc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tm8", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tm2", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "oma", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "aa3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "at3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mmf", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "au",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "snd", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "vox", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "w64", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tta", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ay", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "gbs", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "hes", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "sgc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "vgm", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "vgz", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "kss", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
#endif
    { "m3u", FILE_ATTR_M3U, Icon_Playlist, LANG_PLAYLIST },
    { "m3u8",FILE_ATTR_M3U, Icon_Playlist, LANG_PLAYLIST },
    { "cfg", FILE_ATTR_CFG, Icon_Config,   VOICE_EXT_CFG },
    { "wps", FILE_ATTR_WPS, Icon_Wps,      VOICE_EXT_WPS },
#ifdef HAVE_REMOTE_LCD
    { "rwps",FILE_ATTR_RWPS, Icon_Wps,     VOICE_EXT_RWPS },
#endif
#if CONFIG_TUNER
    { "fmr", FILE_ATTR_FMR, Icon_Preset, LANG_FMR },
    { "fms", FILE_ATTR_FMS, Icon_Wps, VOICE_EXT_FMS },
#endif
    { "lng", FILE_ATTR_LNG, Icon_Language, LANG_LANGUAGE },
    { "rock",FILE_ATTR_ROCK,Icon_Plugin,   VOICE_EXT_ROCK },
    { "lua", FILE_ATTR_LUA, Icon_Plugin,   VOICE_EXT_ROCK },
#ifdef HAVE_LCD_BITMAP
    { "fnt", FILE_ATTR_FONT,Icon_Font,     VOICE_EXT_FONT },
    { "kbd", FILE_ATTR_KBD, Icon_Keyboard, VOICE_EXT_KBD },
#endif
    { "bmark",FILE_ATTR_BMARK, Icon_Bookmark,  VOICE_EXT_BMARK },
    { "cue",  FILE_ATTR_CUE,   Icon_Bookmark,  VOICE_EXT_CUESHEET },
#ifdef HAVE_LCD_BITMAP
    { "sbs",  FILE_ATTR_SBS,  Icon_Wps,   VOICE_EXT_SBS },
#endif
#ifdef HAVE_REMOTE_LCD
    { "rsbs", FILE_ATTR_RSBS, Icon_Wps,   VOICE_EXT_RSBS },
#if CONFIG_TUNER
    { "rfms", FILE_ATTR_RFMS, Icon_Wps, VOICE_EXT_RFMS },
#endif
#endif
#ifdef BOOTFILE_EXT
    { BOOTFILE_EXT, FILE_ATTR_MOD, Icon_Firmware, VOICE_EXT_AJZ },
#endif
#ifdef BOOTFILE_EXT2
    { BOOTFILE_EXT2, FILE_ATTR_MOD, Icon_Firmware, VOICE_EXT_AJZ },
#endif
};

void tree_get_filetypes(const struct filetype** types, int* count)
{
    *types = inbuilt_filetypes;
    *count = sizeof(inbuilt_filetypes) / sizeof(*inbuilt_filetypes);
}

#define ROCK_EXTENSION "rock"

struct file_type {
    enum themable_icons icon; /* the icon which shall be used for it, NOICON if unknown */
    unsigned char  attr; /* FILE_ATTR_MASK >> 8 */
    char* plugin; /* Which plugin to use, NULL if unknown, or builtin */
    char* extension; /* NULL for none */
};
static struct file_type filetypes[MAX_FILETYPES];
static int custom_filetype_icons[MAX_FILETYPES];
static bool custom_icons_loaded = false;
#ifdef HAVE_LCD_COLOR
static int custom_colors[MAX_FILETYPES];
#endif
struct filetype_unknown {
    int icon;
#ifdef HAVE_LCD_COLOR
    int color;
#endif
};
static struct filetype_unknown unknown_file = {
    .icon = Icon_NOICON,
#ifdef HAVE_LCD_COLOR
    .color = -1,
#endif
};

/* index array to filetypes used in open with list. */
static int viewers[MAX_VIEWERS];
static int filetype_count = 0;
static unsigned char highest_attr = 0;
static int viewer_count = 0;

static int strdup_handle, strdup_bufsize, strdup_cur_idx;
static int move_callback(int handle, void* current, void* new)
{
    /*could compare to strdup_handle, but ops is only used once */
    (void)handle;
    size_t diff = new - current;
#define FIX_PTR(x) \
    { if ((void*)x >= current && (void*)x < (current+strdup_bufsize)) x+= diff; }
    for(int i = 0; i < filetype_count; i++)
    {
        FIX_PTR(filetypes[i].extension);
        FIX_PTR(filetypes[i].plugin);
    }
    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

static char *filetypes_strdup(char* string)
{
    char *buffer = core_get_data(strdup_handle) + strdup_cur_idx;
    strdup_cur_idx += strlcpy(buffer, string, strdup_bufsize-strdup_cur_idx)+1;
    return buffer;
}

static char *filetypes_store_plugin(char *plugin, int n)
{
    int i;
    /* if the plugin is in the list already, use it. */
    for (i=0; i<viewer_count; i++)
    {
        if (!strcmp(filetypes[viewers[i]].plugin, plugin))
            return filetypes[viewers[i]].plugin;
    }
    /* otherwise, allocate buffer */
    if (viewer_count < MAX_VIEWERS)
        viewers[viewer_count++] = n;
    return filetypes_strdup(plugin);
}

static int find_extension(const char* extension)
{
    int i;
    if (!extension)
        return -1;
    for (i=1; i<filetype_count; i++)
    {
        if (filetypes[i].extension &&
            !strcasecmp(extension, filetypes[i].extension))
            return i;
    }
    return -1;
}

static void read_builtin_types(void);
static void read_config(int fd);
#ifdef HAVE_LCD_COLOR
/* Colors file format is similar to icons:
 * ext:hex_color
 * load a colors file from a theme with:
 * filetype colours: filename.colours */
void read_color_theme_file(void) {
    char buffer[MAX_PATH];
    int fd;
    char *ext, *color;
    int i;
    for (i = 0; i < MAX_FILETYPES; i++) {
        custom_colors[i] = -1;
    }
    snprintf(buffer, MAX_PATH, THEME_DIR "/%s.colours",
             global_settings.colors_file);
    fd = open(buffer, O_RDONLY);
    if (fd < 0)
        return;
    while (read_line(fd, buffer, MAX_PATH) > 0)
    {
        if (!settings_parseline(buffer, &ext, &color))
            continue;
        if (!strcasecmp(ext, "folder"))
        {
            hex_to_rgb(color, &custom_colors[0]);
            continue;
        }
        if (!strcasecmp(ext, "???"))
        {
            hex_to_rgb(color, &unknown_file.color);
            continue;
        }
        i = find_extension(ext);
        if (i >= 0)
            hex_to_rgb(color, &custom_colors[i]);
    }
    close(fd);
}
#endif
#ifdef HAVE_LCD_BITMAP
void read_viewer_theme_file(void)
{
    char buffer[MAX_PATH];
    int fd;
    char *ext, *icon;
    int i;
    int *icon_dest;
    global_status.viewer_icon_count = 0;
    custom_icons_loaded = false;
    custom_filetype_icons[0] = Icon_Folder;
    for (i=1; i<filetype_count; i++)
    {
        custom_filetype_icons[i] = filetypes[i].icon;
    }
    
    snprintf(buffer, MAX_PATH, "%s/%s.icons", ICON_DIR, 
             global_settings.viewers_icon_file);
    fd = open(buffer, O_RDONLY);
    if (fd < 0)
        return;
    while (read_line(fd, buffer, MAX_PATH) > 0)
    {
        if (!settings_parseline(buffer, &ext, &icon))
            continue;
        i = find_extension(ext);
        if (i >= 0)
            icon_dest = &custom_filetype_icons[i];
        else if (!strcmp(ext, "???"))
            icon_dest = &unknown_file.icon;
        else
            icon_dest = NULL;

        if (icon_dest)
        {
            if (*icon == '*')
                *icon_dest = atoi(icon+1);
            else if (*icon == '-')
                *icon_dest = Icon_NOICON;
            else if (*icon >= '0' && *icon <= '9')
            {
                int number = atoi(icon);
                if (number > global_status.viewer_icon_count)
                    global_status.viewer_icon_count++;
                *icon_dest = Icon_Last_Themeable + number;
            }
        }
    }
    close(fd);
    custom_icons_loaded = true;
}
#endif

void  filetype_init(void)
{
    /* set the directory item first */
    filetypes[0].extension = NULL;
    filetypes[0].plugin = NULL;
    filetypes[0].attr   = 0;
    filetypes[0].icon   = Icon_Folder;

    /* estimate bufsize with the filesize, will not be larger */
    viewer_count = 0;
    filetype_count = 1;

    int fd = open(VIEWERS_CONFIG, O_RDONLY);
    if (fd < 0)
        return;

    strdup_bufsize = filesize(fd);
    strdup_handle = core_alloc_ex("filetypes", strdup_bufsize, &ops);
    if (strdup_handle <= 0)
    {
        close(fd);
        return;
    }
    read_builtin_types();
    read_config(fd);
    close(fd);
#ifdef HAVE_LCD_BITMAP
    read_viewer_theme_file();
#endif
#ifdef HAVE_LCD_COLOR
    read_color_theme_file();
#endif
    core_shrink(strdup_handle, core_get_data(strdup_handle), strdup_cur_idx);
}

/* remove all white spaces from string */
static void rm_whitespaces(char* str)
{
    char *s = str;
    while (*str)
    {
        if (!isspace(*str))
        {
            *s = *str;
            s++;
        }
        str++;
    }
    *s = '\0';
}

static void read_builtin_types(void)
{
    int count = sizeof(inbuilt_filetypes)/sizeof(*inbuilt_filetypes), i;
    for(i=0; i<count && (filetype_count < MAX_FILETYPES); i++)
    {
        filetypes[filetype_count].extension = inbuilt_filetypes[i].extension;
        filetypes[filetype_count].plugin = NULL;
        filetypes[filetype_count].attr   = inbuilt_filetypes[i].tree_attr>>8;
        if (filetypes[filetype_count].attr > highest_attr)
            highest_attr = filetypes[filetype_count].attr;
        filetypes[filetype_count].icon   = inbuilt_filetypes[i].icon;
        filetype_count++;
    }
}

static void read_config(int fd)
{
    char line[64], *s, *e;
    char *extension, *plugin;
    /* config file is in the format
       <extension>,<plugin>,<icon code>
       ignore line if either of the first two are missing */
    while (read_line(fd, line, sizeof line) > 0)
    {
        if (filetype_count >= MAX_FILETYPES)
        {
            splash(HZ, ID2P(LANG_FILETYPES_FULL));
            break;
        }
        rm_whitespaces(line);
        /* get the extension */
        s = line;
        e = strchr(s, ',');
        if (!e)
            continue;
        *e = '\0';
        extension = s;

        /* get the plugin */
        s = e+1;
        e = strchr(s, ',');
        if (!e)
            continue;
        *e = '\0';
        plugin = s;

        if (!strcmp("???", extension))
        {
            /* get the icon */
            s = e+1;
            if (*s == '*')
                unknown_file.icon = atoi(s+1);
            else if (*s == '-')
                unknown_file.icon = Icon_NOICON;
            else if (*s >= '0' && *s <= '9')
                unknown_file.icon = Icon_Last_Themeable + atoi(s);
            continue;
        }

        /* ok, store this plugin/extension, check icon after */
        struct file_type *file_type = &filetypes[filetype_count];
        file_type->extension = filetypes_strdup(extension);
        file_type->plugin = filetypes_store_plugin(plugin, filetype_count);
        file_type->attr = highest_attr +1;
        file_type->icon = Icon_Questionmark;
        highest_attr++;
        /* get the icon */
        s = e+1;
        if (*s == '*')
            file_type->icon = atoi(s+1);
        else if (*s == '-')
            file_type->icon = Icon_NOICON;
        else if (*s >= '0' && *s <= '9')
            file_type->icon = Icon_Last_Themeable + atoi(s);
        filetype_count++;
    }
}

int filetype_get_attr(const char* file)
{
    char *extension = strrchr(file, '.');
    int i;
    if (!extension)
        return 0;
    extension++;

    i = find_extension(extension);
    if (i >= 0)
        return (filetypes[i].attr<<8)&FILE_ATTR_MASK;
    return 0;
}

static int find_attr(int attr)
{
    int i;
    /* skip the directory item */
    if ((attr & ATTR_DIRECTORY)==ATTR_DIRECTORY)
        return 0;
    for (i=1; i<filetype_count; i++)
    {
        if ((attr>>8) == filetypes[i].attr)
            return i;
    }
    return -1;
}

#ifdef HAVE_LCD_COLOR
int filetype_get_color(const char * name, int attr)
{
    char *extension;
    int i;
    if ((attr & ATTR_DIRECTORY)==ATTR_DIRECTORY)
        return custom_colors[0];
    extension = strrchr(name, '.');
    if (!extension)
        return unknown_file.color;
    extension++;

    i = find_extension(extension);
    if (i >= 0)
        return custom_colors[i];
    return unknown_file.color;
}
#endif

int filetype_get_icon(int attr)
{
    int index = find_attr(attr);
    if (index < 0)
        return unknown_file.icon;
    if (custom_icons_loaded)
        return custom_filetype_icons[index];
    return filetypes[index].icon;
}

char* filetype_get_plugin(const struct entry* file)
{
    static char plugin_name[MAX_PATH];
    int index = find_attr(file->attr);
    if (index < 0)
        return NULL;
    if (filetypes[index].plugin == NULL)
        return NULL;
    snprintf(plugin_name, MAX_PATH, "%s/%s.%s", 
             PLUGIN_DIR, filetypes[index].plugin, ROCK_EXTENSION);
    return plugin_name;
}

bool filetype_supported(int attr)
{
    return find_attr(attr) >= 0;
}

/**** Open With Screen ****/
struct cb_data {
    const char *current_file;
};

static enum themable_icons openwith_get_icon(int selected_item, void * data)
{
    (void)data;
    return filetypes[viewers[selected_item]].icon;
}

static const char* openwith_get_name(int selected_item, void * data,
                                     char * buffer, size_t buffer_len)
{
    (void)data; (void)buffer; (void)buffer_len;
    const char *s = strrchr(filetypes[viewers[selected_item]].plugin, '/');
    if (s)
        return s+1;
    else return filetypes[viewers[selected_item]].plugin;
}

static int openwith_get_talk(int selected_item, void * data)
{
    (void)data;
    char viewer_filename[MAX_FILENAME];
    snprintf(viewer_filename, MAX_FILENAME, "%s.%s",
             filetypes[viewers[selected_item]].plugin, ROCK_EXTENSION);
    talk_file_or_spell(PLUGIN_DIR, viewer_filename,
                       NULL, false);
    return 0;
}

static int openwith_action_callback(int action, struct gui_synclist *lists)
{
    struct cb_data *info = (struct cb_data *)lists->data;
    int i;
    if (action == ACTION_STD_OK)
    {
        char plugin[MAX_PATH];
        i = viewers[gui_synclist_get_sel_pos(lists)];
        snprintf(plugin, MAX_PATH, "%s/%s.%s",
                    PLUGIN_DIR, filetypes[i].plugin, ROCK_EXTENSION);
        plugin_load(plugin, info->current_file);
        return ACTION_STD_CANCEL;
    }
    return action;
}

int filetype_list_viewers(const char* current_file)
{
    struct simplelist_info info;
    struct cb_data data = { current_file };
#ifndef HAVE_LCD_BITMAP
    if (viewer_count == 0)
    {
        splash(HZ*2, ID2P(LANG_NO_VIEWERS));
        return PLUGIN_OK;
    }
#endif
    simplelist_info_init(&info, str(LANG_ONPLAY_OPEN_WITH), viewer_count, &data);
    info.action_callback = openwith_action_callback;
    info.get_name = openwith_get_name;
    info.get_icon = global_settings.show_icons?openwith_get_icon:NULL;
    info.get_talk = openwith_get_talk;
    return simplelist_show_list(&info);
}

int filetype_load_plugin(const char* plugin, const char* file)
{
    int i;
    char plugin_name[MAX_PATH];
    char *s;

    for (i=0;i<filetype_count;i++)
    {
        if (filetypes[i].plugin)
        {
            s = strrchr(filetypes[i].plugin, '/');
            if (s)
            {
                if (!strcmp(s+1, plugin))
                    break;
            }
            else if (!strcmp(filetypes[i].plugin, plugin))
                break;
        }
    }
    if (i >= filetype_count)
        return PLUGIN_ERROR;
    snprintf(plugin_name, MAX_PATH, "%s/%s.%s",
             PLUGIN_DIR, filetypes[i].plugin, ROCK_EXTENSION);
    return plugin_load(plugin_name, file);
}
