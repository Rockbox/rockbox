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

#define ROCK_EXT "rock"
#define ROCK_LEN 5
#define OP_EXT "opx"
#define OP_LEN 4

struct open_plugin_entry_t open_plugin_entry = {0};

static const int op_entry_sz = sizeof(struct open_plugin_entry_t);

static int open_plugin_hash_get_entry(uint32_t hash,
                  struct open_plugin_entry_t *entry,
                               const char* dat_file);

static inline void op_clear_entry(struct open_plugin_entry_t *entry)
{
    if (entry)
    {
        memset(entry, 0, op_entry_sz);
        entry->lang_id = -1;
    }
}

static int op_update_dat(struct open_plugin_entry_t *entry)
{
    int fd, fd1;
    uint32_t hash;

    if (!entry || entry->hash == 0)
        return -1;

    hash = entry->hash;

    fd = open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (!fd)
        return -1;
    write(fd, entry, op_entry_sz);

    fd1 = open(OPEN_PLUGIN_DAT, O_RDONLY);
    if (fd1)
    {
        while (read(fd1, &open_plugin_entry, op_entry_sz) == op_entry_sz)
        {
            if (open_plugin_entry.hash != hash)
                write(fd, &open_plugin_entry, op_entry_sz);
        }
        close(fd1);
        remove(OPEN_PLUGIN_DAT);
    }
    close(fd);

    rename(OPEN_PLUGIN_DAT ".tmp", OPEN_PLUGIN_DAT);

    op_clear_entry(&open_plugin_entry);
    return 0;
}

uint32_t open_plugin_add_path(const char *key, const char *plugin, const char *parameter)
{
    int len;
    bool is_valid = false;
    uint32_t hash;
    int32_t lang_id;
    char *pos = "\0";

    if(!key)
    {
        op_clear_entry(&open_plugin_entry);
        return 0;
    }

    lang_id = P2ID((unsigned char*)key);
    key = P2STR((unsigned char *)key);

    open_plugin_get_hash(key, &hash);


    if(open_plugin_entry.hash != hash)
    {
        /* the entry in ram needs saved */
        op_update_dat(&open_plugin_entry);
    }

    if (plugin)
    {
        /* name */
        if (path_basename(plugin, (const char **)&pos) == 0)
            pos = "\0";

        len = strlcpy(open_plugin_entry.name, pos, OPEN_PLUGIN_NAMESZ);
        if (len > ROCK_LEN && strcasecmp(&(pos[len-ROCK_LEN]), "." ROCK_EXT) == 0)
        {
            is_valid = true;

            /* path */
            strlcpy(open_plugin_entry.path, plugin, OPEN_PLUGIN_BUFSZ);

            if(parameter)
                strlcpy(open_plugin_entry.param, parameter, OPEN_PLUGIN_BUFSZ);
            else
                open_plugin_entry.param[0] = '\0';
        }
        else if (len > OP_LEN && strcasecmp(&(pos[len-OP_LEN]), "." OP_EXT) == 0)
        {
            is_valid = true;
            open_plugin_hash_get_entry(0, &open_plugin_entry, plugin);
        }
    }

    if (!is_valid)
    {
        if (lang_id != LANG_SHORTCUTS) /* from shortcuts menu */
            splashf(HZ / 2, str(LANG_OPEN_PLUGIN_NOT_A_PLUGIN), pos);
        op_clear_entry(&open_plugin_entry);
        hash = 0;
    }
    else
    {
        open_plugin_entry.hash = hash;
        open_plugin_entry.lang_id = lang_id;
    }

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

static int open_plugin_hash_get_entry(uint32_t hash,
                  struct open_plugin_entry_t *entry,
                               const char* dat_file)
{
    int ret = -1, record = -1;

    if (entry)
    {

        if (hash != 0)
        {
            if(entry->hash == hash) /* hasn't been flushed yet? */
                return 0;
            else
                op_update_dat(&open_plugin_entry);
        }

        int fd = open(dat_file, O_RDONLY);

        if (fd)
        {
            while (read(fd, entry, op_entry_sz) == op_entry_sz)
            {
                record++;
                if (hash == 0 || entry->hash == hash)
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

    open_plugin_get_hash(key, &hash); /* in open_plugin.h */
    return open_plugin_hash_get_entry(hash, entry, OPEN_PLUGIN_DAT);
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

    if (ret != GO_TO_PLUGIN)
        op_clear_entry(&open_plugin_entry);

    return ret;
}

void open_plugin_cache_flush(void)
{
    op_update_dat(&open_plugin_entry);
}

#endif /* ndef __PCTOOL__ */
