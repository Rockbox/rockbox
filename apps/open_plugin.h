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
#ifndef OPEN_PLUGIN_H
#define OPEN_PLUGIN_H

/* Open_plugin module
 * OP stores and retrieves plugin path and parameters by key
 * from a dictionary file
 *
 * plugins can load other plugins
 * return rb->plugin_open(path, parameter);
 */

#ifndef __PCTOOL__
/* open_plugin path lookup */
#define OPEN_PLUGIN_DAT PLUGIN_DIR "/plugin.dat"
#define OPEN_RBPLUGIN_DAT PLUGIN_DIR "/rb_plugins.dat"
#define OPEN_PLUGIN_BUFSZ MAX_PATH
#define OPEN_PLUGIN_NAMESZ 32

enum {
    OPEN_PLUGIN_LANG_INVALID    = (-1),
    OPEN_PLUGIN_LANG_IGNORE     = (-2),
    OPEN_PLUGIN_LANG_IGNOREALL  = (-3),
    OPEN_PLUGIN_NOT_FOUND     = (-1),
    OPEN_PLUGIN_NEEDS_FLUSHED = (-2),
};

struct open_plugin_entry_t
{
/* hash lang_id checksum need to be the first items */
    uint32_t hash;
    int32_t  lang_id;
    uint32_t  checksum;
    char name[OPEN_PLUGIN_NAMESZ+1];
    /*char key[OPEN_PLUGIN_BUFSZ+1];*/
    char path[OPEN_PLUGIN_BUFSZ+1];
    char param[OPEN_PLUGIN_BUFSZ+1];
};

#define OPEN_PLUGIN_CHECKSUM (uint32_t)              \
(                                                    \
    (sizeof(struct open_plugin_entry_t) << 16)     + \
    offsetof(struct open_plugin_entry_t, hash)     + \
    offsetof(struct open_plugin_entry_t, lang_id)  + \
    offsetof(struct open_plugin_entry_t, checksum) + \
    offsetof(struct open_plugin_entry_t, name)     + \
    /*offsetof(struct open_plugin_entry_t, key)+*/   \
    offsetof(struct open_plugin_entry_t, path)     + \
    offsetof(struct open_plugin_entry_t, param))

#define OPEN_PLUGIN_SEED 0x811C9DC5; //seed, 2166136261;
inline static void open_plugin_get_hash(const char *key, uint32_t *hash)
{
    /* Calculate modified FNV1a hash of string */
    const uint32_t p = 16777619;
    *hash = OPEN_PLUGIN_SEED;
    while(*key)
        *hash = (*key++ ^ *hash) * p;
}

#ifndef PLUGIN
extern struct open_plugin_entry_t open_plugin_entry;
uint32_t open_plugin_add_path(const char *key, const char *plugin, const char *parameter);
int open_plugin_get_entry(const char *key, struct open_plugin_entry_t *entry);
void open_plugin_browse(const char *key);
int open_plugin_run(const char *key);
void open_plugin_cache_flush(void); /* flush to disk */
#endif

#endif /*ndef __PCTOOL__ */
#endif /* OPEN_PLUGIN_H */
