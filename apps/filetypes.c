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
/*#define LOGF_ENABLE*/
#include "logf.h"

/* max filetypes (plugins & icons stored here) */
#define MAX_FILETYPES 192
/* max viewer plugins */
#define MAX_VIEWERS 56

static void read_builtin_types_init(void) INIT_ATTR;
static void read_viewers_config_init(void) INIT_ATTR;
static void read_config_init(int fd) INIT_ATTR;

/* a table for the known file types */
static const struct filetype inbuilt_filetypes[] = {
    { "mp3",  FILE_ATTR_AUDIO },
    { "mp2",  FILE_ATTR_AUDIO },
    { "mpa",  FILE_ATTR_AUDIO },
    { "mp1",  FILE_ATTR_AUDIO },
    { "ogg",  FILE_ATTR_AUDIO },
    { "oga",  FILE_ATTR_AUDIO },
    { "wma",  FILE_ATTR_AUDIO },
    { "wmv",  FILE_ATTR_AUDIO },
    { "asf",  FILE_ATTR_AUDIO },
    { "wav",  FILE_ATTR_AUDIO },
    { "flac", FILE_ATTR_AUDIO },
    { "ac3",  FILE_ATTR_AUDIO },
    { "a52",  FILE_ATTR_AUDIO },
    { "mpc",  FILE_ATTR_AUDIO },
    { "wv",   FILE_ATTR_AUDIO },
    { "m4a",  FILE_ATTR_AUDIO },
    { "m4b",  FILE_ATTR_AUDIO },
    { "mp4",  FILE_ATTR_AUDIO },
    { "mod",  FILE_ATTR_AUDIO },
    { "mpga", FILE_ATTR_AUDIO },
    { "shn",  FILE_ATTR_AUDIO },
    { "aif",  FILE_ATTR_AUDIO },
    { "aiff", FILE_ATTR_AUDIO },
    { "spx" , FILE_ATTR_AUDIO },
    { "opus", FILE_ATTR_AUDIO },
    { "sid",  FILE_ATTR_AUDIO },
    { "adx",  FILE_ATTR_AUDIO },
    { "nsf",  FILE_ATTR_AUDIO },
    { "nsfe", FILE_ATTR_AUDIO },
    { "spc",  FILE_ATTR_AUDIO },
    { "ape",  FILE_ATTR_AUDIO },
    { "mac",  FILE_ATTR_AUDIO },
    { "sap" , FILE_ATTR_AUDIO },
    { "rm",   FILE_ATTR_AUDIO },
    { "ra",   FILE_ATTR_AUDIO },
    { "rmvb", FILE_ATTR_AUDIO },
    { "cmc",  FILE_ATTR_AUDIO },
    { "cm3",  FILE_ATTR_AUDIO },
    { "cmr",  FILE_ATTR_AUDIO },
    { "cms",  FILE_ATTR_AUDIO },
    { "dmc",  FILE_ATTR_AUDIO },
    { "dlt",  FILE_ATTR_AUDIO },
    { "mpt",  FILE_ATTR_AUDIO },
    { "mpd",  FILE_ATTR_AUDIO },
    { "rmt",  FILE_ATTR_AUDIO },
    { "tmc",  FILE_ATTR_AUDIO },
    { "tm8",  FILE_ATTR_AUDIO },
    { "tm2",  FILE_ATTR_AUDIO },
    { "oma",  FILE_ATTR_AUDIO },
    { "aa3",  FILE_ATTR_AUDIO },
    { "at3",  FILE_ATTR_AUDIO },
    { "mmf",  FILE_ATTR_AUDIO },
    { "au",   FILE_ATTR_AUDIO },
    { "snd",  FILE_ATTR_AUDIO },
    { "vox",  FILE_ATTR_AUDIO },
    { "w64",  FILE_ATTR_AUDIO },
    { "tta",  FILE_ATTR_AUDIO },
    { "ay",   FILE_ATTR_AUDIO },
    { "vtx",  FILE_ATTR_AUDIO },
    { "gbs",  FILE_ATTR_AUDIO },
    { "hes",  FILE_ATTR_AUDIO },
    { "sgc",  FILE_ATTR_AUDIO },
    { "vgm",  FILE_ATTR_AUDIO },
    { "vgz",  FILE_ATTR_AUDIO },
    { "kss",  FILE_ATTR_AUDIO },
    { "aac",  FILE_ATTR_AUDIO },
    { "m3u",  FILE_ATTR_M3U },
    { "m3u8", FILE_ATTR_M3U },
    { "cfg",  FILE_ATTR_CFG },
    { "wps",  FILE_ATTR_WPS },
#ifdef HAVE_REMOTE_LCD
    { "rwps", FILE_ATTR_RWPS },
#endif
#if CONFIG_TUNER
    { "fmr",  FILE_ATTR_FMR },
    { "fms",  FILE_ATTR_FMS },
#endif
    { "lng",  FILE_ATTR_LNG   },
    { "rock", FILE_ATTR_ROCK  },
    { "lua",  FILE_ATTR_LUA   },
    { "opx",  FILE_ATTR_OPX   },
    { "fnt",  FILE_ATTR_FONT  },
    { "kbd",  FILE_ATTR_KBD   },
    { "bmark",FILE_ATTR_BMARK },
    { "cue",  FILE_ATTR_CUE   },
    { "sbs",  FILE_ATTR_SBS   },
#ifdef HAVE_REMOTE_LCD
    { "rsbs", FILE_ATTR_RSBS },
#if CONFIG_TUNER
    { "rfms", FILE_ATTR_RFMS },
#endif
#endif
#ifdef BOOTFILE_EXT
    { BOOTFILE_EXT,  FILE_ATTR_MOD },
#endif
#ifdef BOOTFILE_EXT2
    { BOOTFILE_EXT2, FILE_ATTR_MOD },
#endif
};

struct fileattr_icon_voice {
    int tree_attr;
    uint16_t icon;
    uint16_t voiceclip;
};

/* a table for the known file types icons & voice clips */
static const struct fileattr_icon_voice inbuilt_attr_icons_voices[] = {
    { FILE_ATTR_AUDIO, Icon_Audio,     VOICE_EXT_MPA },
    { FILE_ATTR_M3U,   Icon_Playlist,  LANG_PLAYLIST },
    { FILE_ATTR_CFG,   Icon_Config,    VOICE_EXT_CFG },
    { FILE_ATTR_WPS,   Icon_Wps,       VOICE_EXT_WPS },
#ifdef HAVE_REMOTE_LCD
    {FILE_ATTR_RWPS,   Icon_Wps,       VOICE_EXT_RWPS },
#endif
#if CONFIG_TUNER
    { FILE_ATTR_FMR,   Icon_Preset,    LANG_FMR },
    { FILE_ATTR_FMS,   Icon_Wps,       VOICE_EXT_FMS },
#endif
    { FILE_ATTR_LNG,   Icon_Language,  LANG_LANGUAGE },
    { FILE_ATTR_ROCK,  Icon_Plugin,    VOICE_EXT_ROCK },
    { FILE_ATTR_LUA,   Icon_Plugin,    VOICE_EXT_ROCK },
    { FILE_ATTR_OPX,   Icon_Plugin,    VOICE_EXT_ROCK },
    { FILE_ATTR_FONT,  Icon_Font,      VOICE_EXT_FONT },
    { FILE_ATTR_KBD,   Icon_Keyboard,  VOICE_EXT_KBD },
    { FILE_ATTR_BMARK, Icon_Bookmark,  VOICE_EXT_BMARK },
    { FILE_ATTR_CUE,   Icon_Bookmark,  VOICE_EXT_CUESHEET },
    { FILE_ATTR_SBS,   Icon_Wps,       VOICE_EXT_SBS },
#ifdef HAVE_REMOTE_LCD
    { FILE_ATTR_RSBS,  Icon_Wps,       VOICE_EXT_RSBS },
#if CONFIG_TUNER
    { FILE_ATTR_RFMS,  Icon_Wps,       VOICE_EXT_RFMS },
#endif
#endif
#if defined(BOOTFILE_EXT) || defined(BOOTFILE_EXT2)
    { FILE_ATTR_MOD,   Icon_Firmware,  VOICE_EXT_AJZ },
#endif
};

long tree_get_filetype_voiceclip(int attr)
{
    if (global_settings.talk_filetype)
    {
        size_t count = ARRAY_SIZE(inbuilt_attr_icons_voices);
        /* try to find a voice ID for the extension, if known */
        attr &= FILE_ATTR_MASK; /* file type */

        for (size_t i = count - 1; i < count; i--)
        {
            if (attr == inbuilt_attr_icons_voices[i].tree_attr)
            {
                logf("%s found attr %d id %d", __func__, attr,
                     inbuilt_attr_icons_voices[i].voiceclip);
                return inbuilt_attr_icons_voices[i].voiceclip;
            }
        }
    }
    logf("%s not found attr %d", __func__, attr);
    return -1;
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

static int strdup_handle, strdup_cur_idx;
static size_t strdup_bufsize;
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
    unknown_file.color = -1;
    if (!global_settings.colors_file[0] || global_settings.colors_file[0] == '-')
        return;

    fd = open_pathfmt(buffer, sizeof(buffer), O_RDONLY,
                      THEME_DIR "/%s.colours", global_settings.colors_file);
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

    fd = open_pathfmt(buffer, sizeof(buffer), O_RDONLY,
                      ICON_DIR "/%s.icons", global_settings.viewers_icon_file);
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

static void read_viewers_config_init(void)
{
    int fd = open(VIEWERS_CONFIG, O_RDONLY);
    if(fd < 0)
        return;

    off_t filesz = filesize(fd);
    if(filesz <= 0)
        goto out;

    /* estimate bufsize with the filesize, will not be larger */
    strdup_bufsize = (size_t)filesz;
    strdup_handle = core_alloc_ex(strdup_bufsize, &ops);
    if(strdup_handle <= 0)
        goto out;

    read_config_init(fd);
    core_shrink(strdup_handle, NULL, strdup_cur_idx);

  out:
    close(fd);
}

void filetype_init(void)
{
    /* set the directory item first */
    filetypes[0].extension = NULL;
    filetypes[0].plugin = NULL;
    filetypes[0].attr   = 0;
    filetypes[0].icon   = Icon_Folder;

    viewer_count = 0;
    filetype_count = 1;

    read_builtin_types_init();
    read_viewers_config_init();
    read_viewer_theme_file();
#ifdef HAVE_LCD_COLOR
    read_color_theme_file();
#endif
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

static void read_builtin_types_init(void)
{
    int tree_attr; 
    size_t count = ARRAY_SIZE(inbuilt_filetypes);
    size_t icon_count = ARRAY_SIZE(inbuilt_attr_icons_voices);
    for(size_t i = 0; (i < count) && (filetype_count < MAX_FILETYPES); i++)
    {
        filetypes[filetype_count].extension = inbuilt_filetypes[i].extension;
        filetypes[filetype_count].plugin = NULL;

        tree_attr = inbuilt_filetypes[i].tree_attr;
        filetypes[filetype_count].attr   = tree_attr>>8;
        if (filetypes[filetype_count].attr > highest_attr)
            highest_attr = filetypes[filetype_count].attr;

        filetypes[filetype_count].icon = unknown_file.icon;
        for (size_t j = icon_count - 1; j < icon_count; j--)
        {
            if (tree_attr == inbuilt_attr_icons_voices[j].tree_attr)
            {
                filetypes[filetype_count].icon = inbuilt_attr_icons_voices[j].icon;
                break;
            }
        }
        filetype_count++;
    }
}

static void read_config_init(int fd)
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

char* filetype_get_plugin(const struct entry* file, char *buffer, size_t buffer_len)
{
    int index = find_attr(file->attr);
    if (index < 0 || !buffer)
        return NULL;
    struct file_type *ft_indexed = &filetypes[index];

    /* attempt to find a suitable viewer by file extension */
    if(ft_indexed->plugin == NULL && ft_indexed->extension != NULL)
    {
        struct file_type *ft;
        int i = filetype_count;
        while (--i > index)
        {
            ft = &filetypes[i];
            if (ft->plugin == NULL || ft->extension == NULL)
                continue;
            else if (strcmp(ft->extension, ft_indexed->extension) == 0)
            {
                /*splashf(HZ*3, "Found %d %s %s", i, ft->extension, ft->plugin);*/
                ft_indexed = ft;
                break;
            }
        }
    }
    if (ft_indexed->plugin == NULL)
        return NULL;

    snprintf(buffer, buffer_len, "%s/%s." ROCK_EXTENSION,
             PLUGIN_DIR, ft_indexed->plugin);
    return buffer;
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
    snprintf(viewer_filename, MAX_FILENAME, "%s." ROCK_EXTENSION,
             filetypes[viewers[selected_item]].plugin);
    talk_file_or_spell(PLUGIN_DIR, viewer_filename,
                       NULL, false);
    return 0;
}

int filetype_list_viewers(const char* current_file)
{
    struct simplelist_info info;
    simplelist_info_init(&info, str(LANG_ONPLAY_OPEN_WITH), viewer_count, NULL);
    info.get_name = openwith_get_name;
    info.get_icon = global_settings.show_icons?openwith_get_icon:NULL;
    info.get_talk = openwith_get_talk;

    int ret = simplelist_show_list(&info);

    if (info.selection >= 0) /* run user selected viewer */
    {
        char plugin[MAX_PATH];
        int i = viewers[info.selection];
        snprintf(plugin, MAX_PATH, "%s/%s." ROCK_EXTENSION,
                    PLUGIN_DIR, filetypes[i].plugin);
        ret = plugin_load(plugin, current_file);
    }
    return ret;
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
    snprintf(plugin_name, MAX_PATH, "%s/%s." ROCK_EXTENSION,
             PLUGIN_DIR, filetypes[i].plugin);
    return plugin_load(plugin_name, file);
}
