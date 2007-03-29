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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#include "sprintf.h"
#include "settings.h"
#include "debug.h"
#include "lang.h"
#include "language.h"
#include "kernel.h"
#include "plugin.h"
#include "filetypes.h"
#include "screens.h"
#include "icons.h"
#include "dir.h"
#include "file.h"
#include "icons.h"
#include "splash.h"
#include "buffer.h"

/* max filetypes (plugins & icons stored here) */
#if CONFIG_CODEC == SWCODEC
#define MAX_FILETYPES 72
#else
#define MAX_FILETYPES 48
#endif

/* number of bytes for the binary icon */
#define ICON_LENGTH 6

/* mask for dynamic filetype info in attribute */
#define FILETYPES_MASK 0xFF00
#define ROCK_EXTENSION "rock"

struct file_type {
    ICON_NO_CONST  icon; /* the icon which shall be used for it, NOICON if unknown */
    bool  viewer; /* true if the rock is in viewers, false if in rocks */
    unsigned char  attr; /* FILETYPES_MASK >> 8 */ 
    char* plugin; /* Which plugin to use, NULL if unknown, or builtin */
    char* extension; /* NULL for none */
};
static struct file_type filetypes[MAX_FILETYPES];
static int filetype_count = 0;
static unsigned char heighest_attr = 0;

static char *filetypes_strdup(char* string)
{
    char *buffer = (char*)buffer_alloc(strlen(string)+1);
    strcpy(buffer, string);
    return buffer;
}
static void read_builtin_types(void);
static void read_config(char* config_file);

void  filetype_init(void)
{
    /* set the directory item first */
    filetypes[0].extension = NULL;
    filetypes[0].plugin = NULL;
    filetypes[0].attr   = 0;
    filetypes[0].icon   = 
#ifdef HAVE_LCD_BITMAP
                            (ICON_NO_CONST)&bitmap_icons_6x8[Icon_Folder];
#else
                            (ICON_NO_CONST)Icon_Folder;
#endif
    filetype_count = 1;
    read_builtin_types();
    read_config(VIEWERS_CONFIG);
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
    const struct filetype *types;
    int count, i;
    tree_get_filetypes(&types, &count);
    for(i=0; i<count && (filetype_count < MAX_FILETYPES); i++)
    {
        filetypes[filetype_count].extension = types[i].extension;
        filetypes[filetype_count].plugin = NULL;
        filetypes[filetype_count].attr   = types[i].tree_attr>>8;
        if (filetypes[filetype_count].attr > heighest_attr)
            heighest_attr = filetypes[filetype_count].attr;
        filetypes[filetype_count].icon   = 
#ifdef HAVE_LCD_BITMAP
                            (ICON_NO_CONST)&bitmap_icons_6x8[types[i].icon];
#else
                            (ICON_NO_CONST)types[i].icon;
#endif
        filetype_count++;
    }
}

static void read_config(char* config_file)
{
    char line[64], *s, *e;
    char extension[8], plugin[32];
#ifdef HAVE_LCD_BITMAP
    char icon[ICON_LENGTH];
    int good_icon;
#endif
    bool viewer;
    int fd = open(config_file, O_RDONLY);
    if (fd < 0)
        return;
    /* config file is in the for 
       <extension>,<plugin>,<icon code>
       ignore line if either of the first two are missing */
    while (read_line(fd, line, 64) > 0)
    {
        if (filetype_count >= MAX_FILETYPES)
        {
            gui_syncsplash(HZ, str(LANG_FILETYPES_FULL));
            break;
        }
        rm_whitespaces(line);
        /* get the extention */
        s = line;
        e = strchr(s, ',');
        if (!e)
            continue;
        *e = '\0';
        strcpy(extension, s);
    
        /* get the plugin */
        s = e+1;
        e = strchr(s, '/');
        if (!e)
            continue;
        *e = '\0';
        if (!strcasecmp("viewers", s))
            viewer = true;
        else
            viewer = false;
        s = e+1;
        e = strchr(s, ',');
        if (!e)
            continue;
        *e = '\0';
        strcpy(plugin, s);
        /* ok, store this plugin/extension, check icon after */
        filetypes[filetype_count].extension = filetypes_strdup(extension);
        filetypes[filetype_count].plugin = filetypes_strdup(plugin);
        filetypes[filetype_count].viewer = viewer;
        filetypes[filetype_count].attr = heighest_attr +1;
        heighest_attr++;
        /* get the icon */
#ifdef  HAVE_LCD_BITMAP
        s = e+1;
        good_icon = 1;
        if (strlen(s) == 12)
        {
            int i, j;
            char val[2]; 
            for (i = 0; good_icon && i < ICON_LENGTH; i++)
            {
                for (j=0; good_icon && j<2; j++)
                {
                    val[j] = tolower(s[i*2+j]);
                    if (val[j] >= 'a' && val[j] <= 'f')
                    {
                        val[j] = val[j] - 'a' + 10;
                    }
                    else if (val[j] >= '0' && val[j] <= '9')
                    {
                        val[j] = val[j] - '0';
                    }
                    else 
                        good_icon = 0;
                }
                icon[i]=((val[0]<<4) | val[1]);
            }
        }
        if (good_icon)
        {
            filetypes[filetype_count].icon = 
                                (ICON_NO_CONST)buffer_alloc(ICON_LENGTH);
            memcpy(filetypes[filetype_count].icon, icon, ICON_LENGTH);
        }
        else 
            filetypes[filetype_count].icon = NOICON;
#else
        filetypes[filetype_count].icon = Icon_Unknown;
#endif
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
    for (i=0; i<filetype_count; i++)
    {
        if (filetypes[i].extension && 
            !strcasecmp(extension, filetypes[i].extension))
            return (filetypes[i].attr<<8)&TREE_ATTR_MASK;
    }
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

ICON filetype_get_icon(int attr)
{
    int index = find_attr(attr);
    if (index < 0)
        return NOICON;
    return (ICON)filetypes[index].icon;
}

char* filetype_get_plugin(const struct entry* file)
{
    static char plugin_name[MAX_PATH];
    int index = find_attr(file->attr);
    if (index < 0)
        return NULL;
    snprintf(plugin_name, MAX_PATH, "%s/%s.%s", 
             filetypes[index].viewer? VIEWERS_DIR: PLUGIN_DIR,
                        filetypes[index].plugin, ROCK_EXTENSION);
    return plugin_name;
}

bool  filetype_supported(int attr)
{
    return find_attr(attr) >= 0;
}

int filetype_list_viewers(const char* current_file)
{
    int i, count = 0;
    char *strings[MAX_FILETYPES/2];
    struct menu_callback_with_desc cb_and_desc = 
        { NULL, ID2P(LANG_ONPLAY_OPEN_WITH), Icon_Plugin };
    struct menu_item_ex menu;
    
    for (i=0; i<filetype_count && count < (MAX_FILETYPES/2); i++)
    {
        if (filetypes[i].plugin)
        {
            int j;
            for (j=0;j<count;j++) /* check if the plugin is in the list yet */
            {
                if (!strcmp(strings[j], filetypes[i].plugin))
                    break;
            }
            if (j<count) 
                continue; /* it is so grab the next plugin */
            strings[count] = filetypes[i].plugin;
            count++;
        }
    }
#ifndef HAVE_LCD_BITMAP
    if (count == 0)
    {
        /* FIX: translation! */
        gui_syncsplash(HZ*2, (unsigned char *)"No viewers found");
        return PLUGIN_OK;
    }
#endif
    menu.flags = MT_RETURN_ID|MENU_HAS_DESC|MENU_ITEM_COUNT(count);
    menu.strings = (const char**)strings;
    menu.callback_and_desc = &cb_and_desc;
    i = do_menu(&menu, NULL);
    if (i >= 0)
        return filetype_load_plugin(strings[i], (void*)current_file);
    return i;
}

int filetype_load_plugin(const char* plugin, char* file)
{
    int fd;
    char plugin_name[MAX_PATH];
    snprintf(plugin_name, sizeof(plugin_name), "%s/%s.%s",
             VIEWERS_DIR, plugin, ROCK_EXTENSION);
    if ((fd = open(plugin_name,O_RDONLY))>=0)
    {
        close(fd);
        return plugin_load(plugin_name,file);
    }
    else
    { 
        snprintf(plugin_name, sizeof(plugin_name), "%s/%s.%s",
                 PLUGIN_DIR, plugin, ROCK_EXTENSION);
        if ((fd = open(plugin_name,O_RDONLY))>=0)
        {
            close(fd);
            return plugin_load(plugin_name,file);
        }
    }
    return PLUGIN_ERROR;
}
