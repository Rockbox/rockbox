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

/* Define LOGF_ENABLE to enable logf output in this file */
//#define LOGF_ENABLE
#include "logf.h"

#define ROCK_EXT "rock"
#define ROCK_LEN 5
#define OP_EXT "opx"
#define OP_LEN 4

struct open_plugin_entry_t open_plugin_entry = {0};

static const uint32_t open_plugin_csum = OPEN_PLUGIN_CHECKSUM;

static const int op_entry_sz = sizeof(struct open_plugin_entry_t);

static const char* strip_rockbox_root(const char *path)
{
    int dlen = strlen(ROCKBOX_DIR);
    if (strncmp(path, ROCKBOX_DIR, dlen) == 0)
        path+= dlen;
    return path;
}

static inline void op_clear_entry(struct open_plugin_entry_t *entry)
{
    if (entry == NULL)
        return;
    memset(entry, 0, op_entry_sz);
    entry->lang_id = -1;
}

static int op_entry_checksum(struct open_plugin_entry_t *entry)
{
    if (entry == NULL || entry->checksum != open_plugin_csum)
        return 0;
    return 1;
}

static int op_find_entry(int fd, struct open_plugin_entry_t *entry,
                                    uint32_t hash, int32_t lang_id)
{
    int ret = OPEN_PLUGIN_NOT_FOUND;
    int record = 0;
    if (hash == 0)
        hash = OPEN_PLUGIN_SEED;
    if (fd >= 0)
    {
        logf("OP find_entry *Searching* hash: %x lang_id: %d", hash, lang_id);

        while (read(fd, entry, op_entry_sz) == op_entry_sz)
        {
            if (entry->lang_id == lang_id || entry->hash == hash ||
               (lang_id == OPEN_PLUGIN_LANG_IGNOREALL))/* return first entry found */
            {
                ret = record;
                /* NULL terminate fields NOTE -- all are actually +1 larger */
                entry->name[OPEN_PLUGIN_NAMESZ] = '\0';
                /*entry->key[OPEN_PLUGIN_BUFSZ] = '\0';*/
                entry->path[OPEN_PLUGIN_BUFSZ] = '\0';
                entry->param[OPEN_PLUGIN_BUFSZ] = '\0';
                logf("OP find_entry *Found* hash: %x lang_id: %d",
                     entry->hash, entry->lang_id);
                logf("OP find_entry rec: %d name: %s %s %s", record,
                     entry->name, entry->path, entry->param);
                break;
            }
            record++;
        }
    }

    /* sanity check */
    if (ret > OPEN_PLUGIN_NOT_FOUND && op_entry_checksum(entry) <= 0)
    {
        splash(HZ * 2, "OpenPlugin Invalid entry");
        ret = OPEN_PLUGIN_NOT_FOUND;
    }
    if (ret == OPEN_PLUGIN_NOT_FOUND)
        op_clear_entry(entry);

    return ret;
}

static int op_update_dat(struct open_plugin_entry_t *entry, bool clear)
{
    int fd;
    uint32_t hash;
    int32_t lang_id;
    if (entry == NULL|| entry->hash == 0)
    {
        logf("OP update *No entry*");
        return OPEN_PLUGIN_NOT_FOUND;
    }

    hash = entry->hash;
    lang_id = entry->lang_id;
    if (lang_id <= OPEN_PLUGIN_LANG_INVALID)
        lang_id = OPEN_PLUGIN_LANG_IGNORE;

    logf("OP update hash: %x lang_id: %d", hash, lang_id);
    logf("OP update name: %s clear: %d", entry->name, (int) clear);
    logf("OP update %s %s %s", entry->name, entry->path, entry->param);

#if (CONFIG_STORAGE & STORAGE_ATA) /* Harddrive -- update existing */
    logf("OP update *Updating entries* %s", OPEN_PLUGIN_DAT);
    fd = open(OPEN_PLUGIN_DAT, O_RDWR | O_CREAT, 0666);

    if (fd < 0)
        return OPEN_PLUGIN_NOT_FOUND;
    /* Only read the hash lang id and checksum */
    uint32_t hash_langid_csum[3] = {0};
    const off_t hlc_sz = sizeof(hash_langid_csum);
    while (read(fd, &hash_langid_csum, hlc_sz) == hlc_sz)
    {
        if ((hash_langid_csum[0] == hash || (int32_t)hash_langid_csum[1] == lang_id) &&
             hash_langid_csum[2] == open_plugin_csum)
        {
            logf("OP update *Entry Exists* hash: %x langid: %d",
                hash_langid_csum[0], (int32_t)hash_langid[1]);
            lseek(fd, 0-hlc_sz, SEEK_CUR);/* back to the start of record */
            break;
        }
        lseek(fd, op_entry_sz - hlc_sz, SEEK_CUR); /* finish record */
    }
    write(fd, entry, op_entry_sz);
    close(fd);
#else /* Everyone else make a temp file */
    logf("OP update *Copying entries* %s", OPEN_PLUGIN_DAT ".tmp");
    fd = open(OPEN_PLUGIN_DAT ".tmp", O_RDWR  | O_CREAT | O_TRUNC, 0666);

    if (fd < 0)
        return OPEN_PLUGIN_NOT_FOUND;
    write(fd, entry, op_entry_sz);

    int fd1 = open(OPEN_PLUGIN_DAT, O_RDONLY);
    if (fd1 >= 0)
    {
        /* copy non-duplicate entries back from original */
        while (read(fd1, entry, op_entry_sz) == op_entry_sz)
        {
            if (entry->hash != hash && entry->lang_id != lang_id &&
               op_entry_checksum(entry) > 0)
            {
                write(fd, entry, op_entry_sz);
            }
        }
        close(fd1);
        remove(OPEN_PLUGIN_DAT);
    }
    if (!clear) /* retrieve original entry */
    {
        logf("OP update *Loading original entry*");
        lseek(fd, 0, SEEK_SET);
        op_find_entry(fd, entry, hash, lang_id);
    }
    close(fd);
    rename(OPEN_PLUGIN_DAT ".tmp", OPEN_PLUGIN_DAT);
#endif

    if (clear)
    {
        logf("OP update *Clearing entry*");
        op_clear_entry(entry);
    }

    return 0;
}

static int op_get_entry(uint32_t hash, int32_t lang_id,
                        struct open_plugin_entry_t *entry, const char *dat_file)
{
    int opret = OPEN_PLUGIN_NOT_FOUND;

    if (entry != NULL)
    {
        /* Is the entry we want already loaded? */
        if(hash != 0 && entry->hash == hash)
            return OPEN_PLUGIN_NEEDS_FLUSHED;

        if(lang_id <= OPEN_PLUGIN_LANG_INVALID)
        {
            lang_id = OPEN_PLUGIN_LANG_IGNORE;
            if (hash == 0)/* no hash or langid -- returns first entry found */
                lang_id = OPEN_PLUGIN_LANG_IGNOREALL;
        }
        else if(entry->lang_id == lang_id)
        {
            return OPEN_PLUGIN_NEEDS_FLUSHED;
        }

        /* if another entry is loaded; flush it to disk before we destroy it */
        op_update_dat(&open_plugin_entry, true);

        logf("OP get_entry hash: %x lang id: %d db: %s", hash, lang_id, dat_file);

        int fd = open(dat_file, O_RDONLY);
        if(fd < 0)
            return OPEN_PLUGIN_NOT_FOUND;
        opret = op_find_entry(fd, entry, hash, lang_id);
        close(fd);
    }

    return opret;
}

uint32_t open_plugin_add_path(const char *key, const char *plugin, const char *parameter)
{
    int len;
    uint32_t hash;
    int32_t lang_id;
    char *pos = "\0";

    if(key == NULL)
    {
        logf("OP add_path No Key, *Clearing entry*");
        op_clear_entry(&open_plugin_entry);
        return 0;
    }

    lang_id = P2ID((unsigned char*)key);
    const char *skey = P2STR((unsigned char *)key);
    logf("OP add_path key: %s lang id: %d", skey, lang_id);
    open_plugin_get_hash(strip_rockbox_root(skey), &hash);

    if(open_plugin_entry.hash != hash)
    {
        logf("OP add_path *Flush entry*");
        /* the entry in ram needs saved */
        op_update_dat(&open_plugin_entry, true);
    }

    if (plugin)
    {
        open_plugin_entry.hash     = hash;
        open_plugin_entry.lang_id  = lang_id;
        open_plugin_entry.checksum = open_plugin_csum;
        /* name */
        if (path_basename(plugin, (const char **)&pos) == 0)
            pos = "\0";

        len = strlcpy(open_plugin_entry.name, pos, OPEN_PLUGIN_NAMESZ);
        if (len > ROCK_LEN && strcasecmp(&(pos[len-ROCK_LEN]), "." ROCK_EXT) == 0)
        {
            /* path */
            strlcpy(open_plugin_entry.path, plugin, OPEN_PLUGIN_BUFSZ);

            if(!parameter)
                parameter = "";
            strlcpy(open_plugin_entry.param, parameter, OPEN_PLUGIN_BUFSZ);
            goto retnhash;
        }
        else if (len > OP_LEN && strcasecmp(&(pos[len-OP_LEN]), "." OP_EXT) == 0)
        {
            op_get_entry(0, OPEN_PLUGIN_LANG_IGNORE, &open_plugin_entry, plugin);
            goto retnhash;
        }
    }

    logf("OP add_path Invalid, *Clearing entry*");
    if (lang_id != LANG_SHORTCUTS) /* from shortcuts menu */
        splashf(HZ * 2, str(LANG_OPEN_PLUGIN_NOT_A_PLUGIN), pos);
    op_clear_entry(&open_plugin_entry);
    hash = 0;

retnhash:
    logf("OP add_path name: %s %s %s",
         open_plugin_entry.name,
         open_plugin_entry.path,
         open_plugin_entry.param);
    return hash;
}

void open_plugin_browse(const char *key)
{
    logf("OP browse");
    struct browse_context browse;
    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];
    open_plugin_get_entry(key, &open_plugin_entry);

    logf("OP browse key: %s name: %s",
        (key ? P2STR((unsigned char *)key):"No Key") ,open_plugin_entry.name);
    logf("OP browse %s %s", open_plugin_entry.path, open_plugin_entry.param);

    if (open_plugin_entry.path[0] == '\0')
        strcpy(open_plugin_entry.path, PLUGIN_DIR"/");

    browse_context_init(&browse, SHOW_ALL, BROWSE_SELECTONLY, "",
                         Icon_Plugin, open_plugin_entry.path, NULL);

    browse.buf = tmp_buf;
    browse.bufsize = OPEN_PLUGIN_BUFSZ;

    if (rockbox_browse(&browse) == GO_TO_PREVIOUS)
        open_plugin_add_path(key, tmp_buf, NULL);
}

int open_plugin_get_entry(const char *key, struct open_plugin_entry_t *entry)
{
    if (key == NULL || entry == NULL)
        return OPEN_PLUGIN_NOT_FOUND;
    int opret;
    uint32_t hash = 0;
    int32_t lang_id = P2ID((unsigned char *)key);
    const char* skey = P2STR((unsigned char *)key); /* string|LANGPTR => string */

    if (lang_id <= OPEN_PLUGIN_LANG_INVALID)
        open_plugin_get_hash(strip_rockbox_root(skey), &hash); /* in open_plugin.h */

    opret = op_get_entry(hash, lang_id, entry, OPEN_PLUGIN_DAT);
    logf("OP entry hash: %x lang id: %d ret: %d key: %s", hash, lang_id, opret, skey);

    if (opret == OPEN_PLUGIN_NOT_FOUND && lang_id > OPEN_PLUGIN_LANG_INVALID) 
    {   /* try rb defaults */
        opret = op_get_entry(hash, lang_id, entry, OPEN_RBPLUGIN_DAT);
        logf("OP rb_entry hash: %x lang id: %d ret: %d key: %s", hash, lang_id, opret, skey);
        /* add to the user plugin.dat file if found */
        op_update_dat(entry, false);

    }
    logf("OP entry ret: %s", (opret == OPEN_PLUGIN_NOT_FOUND ? "Not Found":"Found"));
    return opret;
}

int open_plugin_run(const char *key)
{
    int ret = 0;
    int opret = open_plugin_get_entry(key, &open_plugin_entry);
    if (opret == OPEN_PLUGIN_NEEDS_FLUSHED)
        op_update_dat(&open_plugin_entry, false);
    const char *path = open_plugin_entry.path;
    const char *param = open_plugin_entry.param;

    logf("OP run key: %s ret: %d name: %s",
        (key ? P2STR((unsigned char *)key):"No Key"), opret, open_plugin_entry.name);
    logf("OP run: %s %s %s", open_plugin_entry.name, path, param);

    if (param[0] == '\0')
        param = NULL;

    ret = plugin_load(path, param);

    if (ret != GO_TO_PLUGIN)
        op_clear_entry(&open_plugin_entry);

    return ret;
}

void open_plugin_cache_flush(void)
{
    logf("OP *cache flush*");
    /* start_in_screen == 0 is 'Previous Screen' it is actually
     *  defined as (GO_TO_PREVIOUS = -2) + 2 for *Legacy?* reasons AFAICT */
    if (global_settings.start_in_screen == 0 &&
        global_status.last_screen == GO_TO_PLUGIN &&
        open_plugin_entry.lang_id > OPEN_PLUGIN_LANG_INVALID)
    {
        /* flush the last item as LANG_PREVIOUS_SCREEN if the user wants to resume */
        open_plugin_entry.lang_id = LANG_PREVIOUS_SCREEN;
    }
    op_update_dat(&open_plugin_entry, true);
}

#endif /* ndef __PCTOOL__ */
