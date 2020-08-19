/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 by William Wilgus
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

#ifndef __PCTOOL__

#include "plugin.h"
#include "open_plugin.h"
#include "pathfuncs.h"
#include "splash.h"
#include "lang.h"

struct open_plugin_entry_t open_plugin_entry;

static const int op_entry_sz = sizeof(struct open_plugin_entry_t);

uint32_t open_plugin_add_path(const char *key, const char *plugin, const char *parameter)
{
    int len;
    uint32_t hash;
    char *pos;
    int fd = 0;

    /*strlcpy(plug_entry.key, key, sizeof(plug_entry.key));*/
    open_plugin_entry.lang_id = P2ID((unsigned char*)key);
    key = P2STR((unsigned char *)key);

    open_plugin_get_hash(key, &hash);
    open_plugin_entry.hash = hash;

    if (plugin)
    {
        /* name */
        if (path_basename(plugin, (const char **)&pos) == 0)
            pos = "\0";

        len = strlcpy(open_plugin_entry.name, pos, OPEN_PLUGIN_NAMESZ);

        if(len > 5 && strcasecmp(&(pos[len-5]), ".rock") == 0)
        {
            fd = open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (!fd)
                return 0;

            /* path */
            strlcpy(open_plugin_entry.path, plugin, OPEN_PLUGIN_BUFSZ);

            if(parameter)
                strlcpy(open_plugin_entry.param, parameter, OPEN_PLUGIN_BUFSZ);
            else
                open_plugin_entry.param[0] = '\0';

            write(fd, &open_plugin_entry, op_entry_sz);
        }
        else
        {
            if (open_plugin_entry.lang_id != LANG_SHORTCUTS)
                splashf(HZ / 2, str(LANG_OPEN_PLUGIN_NOT_A_PLUGIN), pos);
            return 0;
        }
    }

    int fd1 = open(OPEN_PLUGIN_DAT, O_RDONLY);
    if (fd1)
    {
        while (read(fd1, &open_plugin_entry, op_entry_sz) == op_entry_sz)
        {
            if (open_plugin_entry.hash != hash)
                write(fd, &open_plugin_entry, op_entry_sz);
        }
        close(fd1);
    }
    close(fd);

    if(fd1)
    {
        remove(OPEN_PLUGIN_DAT);
        rename(OPEN_PLUGIN_DAT ".tmp", OPEN_PLUGIN_DAT);
    }
    else
        hash = 0;

    return hash;
}

void open_plugin_browse(const char *key)
{
    struct browse_context browse;
    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];
    open_plugin_get_entry(key, &open_plugin_entry);

    if (open_plugin_entry.path[0] == '\0')
        strcpy(open_plugin_entry.path, PLUGIN_DIR"/");

    browse_context_init(&browse, SHOW_ALL, BROWSE_SELECTONLY, "",
                         Icon_Plugin, open_plugin_entry.path, NULL);

    browse.buf = tmp_buf;
    browse.bufsize = OPEN_PLUGIN_BUFSZ;

    if (rockbox_browse(&browse) == GO_TO_PREVIOUS)
        open_plugin_add_path(key, tmp_buf, NULL);
}

static int open_plugin_hash_get_entry(uint32_t hash, struct open_plugin_entry_t *entry)
{
    int ret = -1, record = -1;

    if (entry)
    {
        int fd = open(OPEN_PLUGIN_DAT, O_RDONLY);

        if (fd)
        {
            while (read(fd, entry, op_entry_sz) == op_entry_sz)
            {
                record++;
                if (entry->hash == hash)
                {
                    ret = record;
                    break;
                }
            }
            close(fd);
        }
        if (ret < 0)
        {
            memset(entry, 0, op_entry_sz);
            entry->lang_id = -1;
        }
    }

    return ret;
}

int open_plugin_get_entry(const char *key, struct open_plugin_entry_t *entry)
{
    uint32_t hash;
    key = P2STR((unsigned char *)key);

    open_plugin_get_hash(key, &hash);
    return open_plugin_hash_get_entry(hash, entry);
}

int open_plugin_run(const char *key)
{
    int ret = 0;
    const char *path;
    const char *param;

    open_plugin_get_entry(key, &open_plugin_entry);

    path = open_plugin_entry.path;
    param = open_plugin_entry.param;
    if (param[0] == '\0')
        param = NULL;

    if (path)
        ret = plugin_load(path, param);

    return ret;
}

#endif /* ndef __PCTOOL__ */
