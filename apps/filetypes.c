/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *
 * $Id$
 *
 * Copyright (C) 2004 Henrik Backe
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

/* max plugin name size without extensions and path */
#define MAX_PLUGIN_LENGTH 32

/* max filetypes (plugins & icons stored here) */
#define MAX_FILETYPES 32

/* max exttypes (extensions stored here) */
#define MAX_EXTTYPES 32

/* string buffer length */
#define STRING_BUFFER_SIZE 512

/* number of bytes for the binary icon */
#define ICON_LENGTH 6

/* mask for dynamic filetype info in attribute */
#define FILETYPES_MASK 0xFF00

/* filenames */
#define ROCK_EXTENSION ".rock"
#define VIEWERS_CONFIG ROCKBOX_DIR "/viewers.config"
#define VIEWERS_DIR    ROCKBOX_DIR "/viewers"

/* global variables */
static int    cnt_filetypes;
static int    cnt_exttypes;
static struct ext_type  exttypes [MAX_EXTTYPES];
static struct file_type filetypes[MAX_FILETYPES];
static int    first_soft_exttype;
static int    first_soft_filetype;
static char*  next_free_string;
static char   plugin_name[sizeof(VIEWERS_DIR) + 7 + MAX_PLUGIN_LENGTH];
static char   string_buffer[STRING_BUFFER_SIZE];

/* prototypes */
#ifdef HAVE_LCD_BITMAP
static char*  string2icon(const char*);
#endif
static char*  get_string(const char*);
static int    find_attr_index(int);
static bool   read_config(const char*);
static void   rm_whitespaces(char*);
static void   scan_plugins(void);

/* initialize dynamic filetypes (called at boot from tree.c) */
void filetype_init(void)
{
    int cnt,i,ix;
    const struct filetype* ftypes;

    memset(exttypes,0,sizeof(exttypes));
    memset(filetypes,0,sizeof(filetypes));
    next_free_string=string_buffer;

/* The special filetype folder must always be stored at index 0 */
#ifdef HAVE_LCD_BITMAP
    if (!filetypes[0].icon)
        filetypes[0].icon = bitmap_icons_6x8[Folder];
#else
    if (!filetypes[0].icon)
        filetypes[0].icon = Folder;
    for (i=1; i < MAX_FILETYPES; i++)
        filetypes[i].icon = -1;
#endif

    /* register hardcoded filetypes */
    tree_get_filetypes(&ftypes, &cnt);
    cnt_exttypes=0;
    cnt_filetypes=0;

    for (i = 0; i < cnt ; i++)
    {
        ix = ((ftypes[i].tree_attr & FILETYPES_MASK) >> 8);
        if (ix < MAX_FILETYPES && i < MAX_EXTTYPES)
        {
#ifdef HAVE_LCD_BITMAP
            if (filetypes[ix].icon == NULL)
                filetypes[ix].icon=bitmap_icons_6x8[ftypes[i].icon];
#else
            if (filetypes[ix].icon == -1)
                filetypes[ix].icon=ftypes[i].icon;
#endif
            if (ix > cnt_filetypes)
                cnt_filetypes=ix;
            exttypes[cnt_exttypes].type=&filetypes[ix];
            exttypes[cnt_exttypes].extension=ftypes[i].extension;
            cnt_exttypes++;
        }
    }
    first_soft_exttype=cnt_exttypes;
    cnt_filetypes++;
    first_soft_filetype=cnt_filetypes;

    /* register dynamic filetypes */
    read_config(VIEWERS_CONFIG);
    scan_plugins();
}

/* get icon */
#ifdef HAVE_LCD_BITMAP
const char* filetype_get_icon(int attr)
#else
int   filetype_get_icon(int attr)
#endif
{
    int ix;

    ix = find_attr_index(attr);

    if (ix < 0)
    {
#ifdef HAVE_LCD_BITMAP
        return NULL;
#else
        return -1;
#endif
    }
    else
    {
        return filetypes[ix].icon;
    }
}

/* get plugin */
char* filetype_get_plugin(const struct entry* file)
{
    int ix;

    ix=find_attr_index(file->attr);

    if (ix < 0)
    {
        return NULL;
    }

    if ((filetypes[ix].plugin == NULL) ||
        (strlen(filetypes[ix].plugin) > MAX_PLUGIN_LENGTH))
        return NULL;

    snprintf(plugin_name, sizeof(plugin_name),
             VIEWERS_DIR "/%s.rock",filetypes[ix].plugin);

    return plugin_name;
}

/* check if filetype is supported */
bool filetype_supported(int attr)
{
    int ix;

    ix=find_attr_index(attr);

    /* hard filetypes and soft filetypes with plugins is supported */
    if (ix > 0)
        if (filetypes[ix].plugin || ix < first_soft_filetype)
            return true;

    return false;
}

/* get the "dynamic" attribute for an extension */
int filetype_get_attr(const char* name)
{
    int i;

    for (i=0; i < cnt_exttypes; i++)
    {
        if (exttypes[i].extension)
        {
            if (!strcasecmp(&name[strlen(name)-
                                 strlen(exttypes[i].extension)],
                            exttypes[i].extension))
            {
                return ((((unsigned long)exttypes[i].type -
                          (unsigned long)&filetypes[0]) /
                         sizeof(struct file_type)) << 8);
            }
        }
    }

    return 0;
}

/* fill a menu list with viewers (used in onplay.c) */
int filetype_load_menu(struct menu_item*  menu,int max_items)
{
    int i;
    int cnt=0;

    for (i=0; i < cnt_filetypes; i++)
    {
        if (filetypes[i].plugin)
        {
            menu[cnt].desc = filetypes[i].plugin;
            cnt++;
            if (cnt == max_items)
                break;
        }
    }
    return cnt;
}

/* start a plugin with an argument (called from onplay.c) */
int filetype_load_plugin(const char* plugin, char* file)
{
    snprintf(plugin_name,sizeof(plugin_name),"%s/%s.rock",
             VIEWERS_DIR,plugin);
    return plugin_load(plugin_name,file);
}

/* get index to filetypes[] from the file attribute */
static int find_attr_index(int attr)
{
    int ix;
    ix = ((attr & FILETYPES_MASK) >> 8);

    if ((attr & ATTR_DIRECTORY)==ATTR_DIRECTORY)
    {
        ix=0;
    }
    else
    {
        if (ix==0)
            ix=-1;
        if (ix > cnt_filetypes)
            ix=-1;
        else
            if ((filetypes[ix].plugin == NULL) &&
#ifdef HAVE_LCD_BITMAP
                (filetypes[ix].icon == NULL)
#else
                (filetypes[ix].icon == -1)
#endif
                )
                ix=-1;
    }

    return ix;
}

/* scan the plugin directory and register filetypes */
static void scan_plugins(void)
{
    DIR *dir;
    struct dirent *entry;
    char* cp;
    char* dot;
    char* dash;
    int   ix;
    int   i;
    bool  found;

    dir = opendir(VIEWERS_DIR);
    if(!dir)
        return;

    while (true)
    {
        /* exttypes[] full, bail out */
        if (cnt_exttypes >= MAX_EXTTYPES)
        {
            splash(HZ,true,str(LANG_FILETYPES_EXTENSION_FULL));
            break;
        }

        /* filetypes[] full, bail out */
        if (cnt_filetypes >= MAX_FILETYPES)
        {
            splash(HZ,true,str(LANG_FILETYPES_FULL));
            break;
        }

        entry = readdir(dir);

        if (!entry)
            break;

        /* skip directories */
        if ((entry->attribute & ATTR_DIRECTORY))
            continue;

        /* Skip FAT volume ID */
        if (entry->attribute & ATTR_VOLUME_ID)
            continue;

        /* filter out dotfiles and hidden files */
        if ((entry->d_name[0]=='.') ||
            (entry->attribute & ATTR_HIDDEN)) {
            continue;
        }

        /* filter out non rock files */
        if (strcasecmp(
             &entry->d_name[strlen(entry->d_name) - sizeof(ROCK_EXTENSION) + 1],
             ROCK_EXTENSION)) {
            continue;
        }

        /* filter out to long filenames */
        if (strlen(entry->d_name) > MAX_PLUGIN_LENGTH + 5)
        {
            splash(HZ,true,str(LANG_FILETYPES_PLUGIN_NAME_LONG));
            continue;
        }

        dot=strrchr(entry->d_name,'.');
        *dot='\0';
        dash=strchr(entry->d_name,'-');

        /* add plugin and extension */
        if (dash)
        {
            *dash='\0';
            ix=(filetype_get_attr(entry->d_name) >> 8);
            if (!ix)
            {
                cp=get_string(entry->d_name);
                if (cp)
                {
                    exttypes[cnt_exttypes].extension=cp;
                    exttypes[cnt_exttypes].type=&filetypes[cnt_filetypes];
#ifdef HAVE_LCD_BITMAP
                    exttypes[cnt_exttypes].type->icon = bitmap_icons_6x8[Plugin];
#else
                    exttypes[cnt_exttypes].type->icon = Plugin;
#endif
                    cnt_exttypes++;

                    *dash='-';
                    cp=get_string(entry->d_name);
                    if (cp)
                    {
                        filetypes[cnt_filetypes].plugin=cp;
                        cnt_filetypes++;
                    }
                    else
                        break;
                }
                else
                    break;
            }
            else
            {
                *dash='-';
                if (!filetypes[ix].plugin)
                {
                    cp=get_string(entry->d_name);
                    if (cp)
                    {
                        filetypes[cnt_filetypes].plugin=cp;
                        cnt_filetypes++;
                    }
                    else
                        break;
                }
            }
            *dash='-';
        }
        /* add plugin only */
        else
        {
            found=false;
            for (i = first_soft_filetype; i < cnt_filetypes; i++)
            {
                if (filetypes[i].plugin)
                    if (!strcasecmp(filetypes[i].plugin,entry->d_name))
                    {
                        found=true;
                        break;
                    }
            }

            if (!found)
            {
                cp=get_string(entry->d_name);
                if (cp)
                {
                    filetypes[cnt_filetypes].plugin=cp;
                    filetypes[cnt_filetypes].no_extension=true;
                    cnt_filetypes++;
                }
                else
                    break;
            }
        }
        *dot='.';
    }
    closedir(dir);
}

/* read config file (or cahe file) */
bool read_config(const char* file)
{
    enum {extension,
          plugin,
#ifdef HAVE_LCD_BITMAP
          icon,
#endif
          last};

    int   i;
    int   fd;
    char* end;
    char* cp;
    char* str[last];
    char   buf[80];

    fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;

    while (read_line(fd, buf, sizeof(buf)))
    {
        if (cnt_exttypes >= MAX_EXTTYPES)
        {
            splash(HZ,true,str(LANG_FILETYPES_EXTENSION_FULL));
            break;
        }

        if (cnt_filetypes >= MAX_FILETYPES)
        {
            splash(HZ,true,str(LANG_FILETYPES_FULL));
            break;
        }

        /* parse buffer */
        rm_whitespaces(buf);

        if (strlen(buf) == 0)
            continue;

        if (buf[0] == '#')
            continue;

        memset(str,0,sizeof(str));
        i=0;
        cp=buf;
        while (*cp==',') {
            cp++;
            i++;
        }
        str[i] = strtok_r(cp, ",", &end);
        i++;

        while (end && i < last)
        {
            if (end)
            {
                cp=end;
                while (*cp==',') {
                    cp++;
                    i++;
                }
            }
            str[i] = strtok_r(NULL, ",", &end);
            i++;
        }

        /* bail out if no icon and no plugin */
        if ((!str[plugin] || !strlen(str[plugin])) &&
#ifdef HAVE_LCD_BITMAP
            (!str[icon]   || !strlen(str[icon])) &&
#endif
            strlen(str[extension]))
            continue;

        /* bail out if no plugin and icon is incorrect*/
        if ((!str[plugin] || !strlen(str[plugin])) &&
#ifdef HAVE_LCD_BITMAP
            (strlen(str[icon]) != ICON_LENGTH*2) &&
#endif
            strlen(str[extension]))
            continue;

        /* bail out if no icon and no plugin and no extension*/
        if ((!str[plugin] || !strlen(str[plugin])) &&
#ifdef HAVE_LCD_BITMAP
            (!str[icon]   || !strlen(str[icon])) &&
#endif
            (!str[extension] || !strlen(str[extension])))
            continue;

        /* add extension */
        if (str[extension])
        {
            if (strlen(str[extension]))
            {
                cp=get_string(str[extension]);
                if (cp)
                {
                    exttypes[cnt_exttypes].type = &filetypes[cnt_filetypes];
                    exttypes[cnt_exttypes].extension = cp;
                    cnt_exttypes++;
                }
                else
                    break;

#ifdef HAVE_LCD_BITMAP
                /* add icon */
                if (str[icon])
                {
                    cp = string2icon(str[icon]);
                    if (cp)
                        filetypes[cnt_filetypes].icon = cp;
                    else
                        break;
                }
#endif
            }
        }

        /* are we able to start plugin from onplay.c ?*/
        if (str[plugin])
        {
            if (strlen(str[plugin]) > MAX_PLUGIN_LENGTH)
            {
                splash(HZ, true, str(LANG_FILETYPES_PLUGIN_NAME_LONG));
                str[plugin] = NULL;
            }
        }

        /* add plugin */
        if (str[plugin])
        {
            if (strlen(str[plugin]))
            {
                cp=strrchr(str[plugin], '.');
                if (cp)
                    *cp='\0';

                cp = get_string(str[plugin]);
                if (cp)
                    filetypes[cnt_filetypes].plugin = cp;
                else
                    break;
            }
        }

        if (filetypes[cnt_filetypes].plugin)
            cnt_filetypes++;
    }
    close(fd);

    return true;
}

#ifdef HAVE_LCD_BITMAP
/* convert an ascii hexadecimal icon to a binary icon */
static char* string2icon(const char* str)
{
    char tmp[ICON_LENGTH*2];
    char *cp;
    int i;

    if (strlen(str)!=ICON_LENGTH*2)
        return NULL;

    if ((sizeof(string_buffer) +
         (unsigned long) string_buffer -
         (unsigned long) next_free_string) < ICON_LENGTH)
    {
        splash(HZ,true,str(LANG_FILETYPES_STRING_BUFFER_EMPTY));
        return NULL;
    }

    for (i=0; i<12; i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            tmp[i]=str[i]-'0';
            continue;
        }

        if (str[i] >= 'a' && str[i] <= 'f')
        {
            tmp[i]=str[i]-'a'+10;
            continue;
        }

        if (str[i] >= 'A' && str[i] <= 'F')
        {
            tmp[i]=str[i]-'A'+10;
            continue;
        }

        return NULL;
    }

    cp=next_free_string;
    for (i = 0; i < ICON_LENGTH; i++)
        cp[i]=((tmp[i*2]<<4) | tmp[i*2+1]);

    next_free_string=&next_free_string[ICON_LENGTH];
    return cp;
}
#endif

/* get string from buffer */
static char* get_string(const char* str)
{
    unsigned int l=strlen(str)+1;
    char* cp;

    if (!str)
        return NULL;

    if (l <= (sizeof(string_buffer) +
             (unsigned long) string_buffer -
             (unsigned long) next_free_string))
    {
        strcpy(next_free_string,str);
        cp=next_free_string;
        next_free_string=&next_free_string[l];
        return cp;
    }
    else
    {
        splash(HZ,true,str(LANG_FILETYPES_STRING_BUFFER_EMPTY));
        return NULL;
    }
}

/* remove all white spaces from string */
static void rm_whitespaces(char* str)
{
    char *cp, *free;

    cp=str;
    free=cp;

    while (cp < &str[strlen(str)])
    {
        switch (*cp)
        {
            case ' '  :
            case '\t' :
            case '\r' :
                break;

            default:
                *free=*cp;
                free++;
                break;
        }
        cp++;
    }

    *free='\0';
}
