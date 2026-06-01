/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 William Wilgus
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

/* open_plugins.rock interfaces with the open_plugin core
 *
 * When opened directly it acts as a viewer for the plugin.dat file
 * this allows you to edit the paths and parameters for
 * core shortcuts as well as your added plugins
 *
 * If a plugin is supplied to the viewer it is added to the dat file
 *
 * If instead the plugin has previously been added then it is run
 * with the parameters previously supplied
 */

#if defined(DEBUG) || defined(SIMULATOR)
    #define logf(...) rb->debugf(__VA_ARGS__); rb->debugf("\n")
#elif defined(ROCKBOX_HAS_LOGF)
    #define logf rb->logf
#else
    #define logf(...) do { } while(0)
#endif

#include "plugin.h"
#include "lang_enum.h"
#include "../open_plugin.h"

#define ROCK_EXT "rock"
#define ROCK_LEN 5

#define OP_EXT "opx"
#define OP_LEN 4

#define OP_PLUGIN_RESTART (PLUGIN_GOTO_PLUGIN | 0x8000)

#define MENU_ID_MAIN "0"
#define MENU_ID_EDIT "1"

#define MAX_ENTRIES 128

static int fd_dat = -1;
static struct gui_synclist lists;
static struct open_plugin_entry_t op_entry;
static const uint32_t open_plugin_csum = OPEN_PLUGIN_CHECKSUM;
static const off_t op_entry_sz = sizeof(struct open_plugin_entry_t);

/* we only need the names for the first menu so don't bother reading paths yet */
const off_t op_name_sz = OPEN_PLUGIN_NAMESZ + (op_entry.name - (char*)&op_entry);
/*static char op_names[MAX_ENTRIES][OPEN_PLUGIN_NAMESZ + 1] = {0};*/


/* Forward declarations*/
static uint32_t op_entry_add_path(const char *key, const char *plugin, const char *parameter, bool use_key);
static int import_from_config(const char* cfgfile);
static int export_to_config(const char* cfgfile);

static bool hash_exists(uint32_t hash, bool remove)
{
    /* keeps track of hash entries
     * supply hash = 0, remove = true to erase all entries
     * supply hash = 0, remove = false; returns true if empty entries are available
     * supply a hash, remove = false; returns false if the hash does not exist & adds hash to the db
     * supply a hash, remove = true; returns true and removes the hash from the db
     * returns false if hash is unique
     */
    static uint32_t seen_hashes[MAX_ENTRIES] = {0};
    int i;
    if (hash == 0)
    {
        if (remove)
        {
            logf("removing all hashes");
            for(i = 0; i < MAX_ENTRIES; i++)
                seen_hashes[i] = 0;
        }
        return seen_hashes[MAX_ENTRIES - 1] == 0; /* true if empty entries exist */
    }
    for(i = 0; i < MAX_ENTRIES; i++)
    {
        uint32_t current_hash = seen_hashes[i];
        if (current_hash == 0 && !remove)
        {
            logf("(%d) added new hash: %x", i, hash);
            seen_hashes[i] = hash;
            break;
        }
        if (current_hash == hash)
        {
            if (remove)
            {
                logf("(%d) remove hash: %x", i, hash);
                for (int j = i + 1; j < MAX_ENTRIES; j++, i++)
                {
                    /* move everything above down 1*/
                    uint32_t next_hash = seen_hashes[j];
                    seen_hashes[i] = next_hash;
                    if (next_hash == 0)
                        break;
                }
                if (i + 1 == MAX_ENTRIES) /* handle last entry */
                {
                    seen_hashes[i] = 0;
                }
            }
            return true;
        }
    }
    return false; /* hash is unique */
}

static bool _yesno_pop(const char* text)
{
    const char *lines[]={text};
    const struct text_message message={lines, 1};
    bool ret = (rb->gui_syncyesno_run(&message,NULL,NULL)== YESNO_YES);
    FOR_NB_SCREENS(i)
        rb->screens[i]->clear_viewport();
    return ret;
}

static size_t pathbasename(const char *name, const char **nameptr)
{
    const char *p = name;
    const char *q = p;
    const char *r = q;

    while (*(p = GOBBLE_PATH_SEPCH(p)))
    {
        q = p;
        p = GOBBLE_PATH_COMP(++p);
        r = p;
    }

    if (r == name && p > name)
        q = p, r = q--; /* root - return last slash */
    /* else path is an empty string */

    *nameptr = q;
    return r - q;
}

static int op_entry_checksum(void)
{
    if (op_entry.checksum != open_plugin_csum +
        (op_entry.lang_id <= OPEN_PLUGIN_LANG_INVALID ? 0 : LANG_LAST_INDEX_IN_ARRAY))
    {
        return 0;
    }
    return 1;
}

static bool op_entry_read(int fd, int selected_item, off_t data_sz)
{
    rb->memset(&op_entry, 0, op_entry_sz);
    op_entry.lang_id = -1;
    return ((selected_item >= 0) && (fd >= 0) &&
        (rb->lseek(fd, selected_item  * op_entry_sz, SEEK_SET) >= 0) &&
        (rb->read(fd, &op_entry, data_sz) == data_sz) && op_entry_checksum() > 0);
}

static bool op_entry_read_name(int fd, int selected_item)
{
    return op_entry_read(fd, selected_item, op_name_sz);
}

static int op_entry_read_opx(const char *path)
{
    int ret = -1;
    off_t filesize;
    int fd_opx;

    if(rb->filetype_get_attr(path) == FILE_ATTR_OPX)
    {
        fd_opx = rb->open(path, O_RDONLY);
        if (fd_opx >= 0)
        {
            filesize = rb->filesize(fd_opx);
            ret = filesize;
            if (filesize == op_entry_sz && !op_entry_read(fd_opx, 0, op_entry_sz))
                ret = 0;
            else if (op_entry_checksum() <= 0)
                ret = 0;
            rb->close(fd_opx);
        }
    }
    return ret;
}

static void op_entry_config_export(void)
{
    int i = 0;
    int len;
    char buf[MAX_PATH + 1];

    do
    {
        i++;
        rb->snprintf(buf, sizeof(buf), "%s/%s%d%s", ROCKBOX_DIR, "open_plugins", i, ".cfg");
    } while (i < 100 && rb->file_exists(buf));

    if( rb->kbd_input(buf, MAX_PATH, NULL ) == 0 )
    {
        len = rb->strlen(buf);
        if(len > 4 && buf[len] != PATH_SEPCH &&
            rb->strcasecmp(&((buf)[len-4]), ".cfg") != 0)
        {
            rb->strcat(buf, ".cfg");
        }

        if (export_to_config(buf) > 0)
            return;
    }

    rb->splashf( 2*HZ, "Save Failed (%s)", buf );

}

static void op_entry_export_opx(int selection)
{
    int len;
    int fd = -1;
    char filename [MAX_PATH + 1];

    if (!op_entry_read(fd_dat, selection, op_entry_sz) || op_entry_checksum() <= 0)
        goto failure;

    rb->snprintf(filename, MAX_PATH, "%s/%s", PLUGIN_APPS_DIR, op_entry.name);

    if( !rb->kbd_input( filename, MAX_PATH, NULL ) )
    {
        len = rb->strlen(filename);
        if(len > OP_LEN && filename[len] != PATH_SEPCH &&
            rb->strcasecmp(&((filename)[len-OP_LEN]), "." OP_EXT) != 0)
        {
            rb->strcat(filename, "." OP_EXT);
        }

        fd = rb->open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

        if (fd >= 0 && rb->write(fd, &op_entry, op_entry_sz) == op_entry_sz)
        {
            rb->close(fd);
            rb->splashf( 1*HZ, "File Saved (%s)", filename );
            return;
        }
         rb->close(fd);
    }

failure:
    rb->splashf( 2*HZ, "Save Failed (%s)", filename );

}

static void op_entry_set_checksum(void)
{
        op_entry.checksum = open_plugin_csum +
        (op_entry.lang_id <= OPEN_PLUGIN_LANG_INVALID ? 0 : LANG_LAST_INDEX_IN_ARRAY);
}

static bool op_entry_set_name(void)
{
    char tmp_buf[OPEN_PLUGIN_NAMESZ+1];
    rb->strlcpy(tmp_buf, op_entry.name, OPEN_PLUGIN_NAMESZ);
    uint32_t crc = rb->crc_32(tmp_buf, sizeof(tmp_buf), 0xffffffff);

    if (rb->kbd_input(tmp_buf, OPEN_PLUGIN_NAMESZ, NULL) >= 0)
    {
        rb->strlcpy(op_entry.name, tmp_buf, OPEN_PLUGIN_NAMESZ);
        return crc != rb->crc_32(tmp_buf, sizeof(tmp_buf), 0xffffffff);
    }
    return false;
}

static int op_entry_set_path(void)
{
    int ret = 0;
    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];

    if (op_entry.path[0] == '\0')
        rb->strcpy(op_entry.path, PLUGIN_DIR"/");

    struct browse_context browse = {
        .dirfilter = SHOW_ALL,
        .flags = BROWSE_SELECTONLY | BROWSE_DIRFILTER,
        .title = rb->str(LANG_ADD),
        .icon = Icon_Plugin,
        .root = op_entry.path,
        .buf = tmp_buf,
        .bufsize = sizeof(tmp_buf),
    };

    if (rb->rockbox_browse(&browse) == GO_TO_PREVIOUS)
    {
        ret = rb->strlcpy(op_entry.path, tmp_buf, OPEN_PLUGIN_BUFSZ);
        if (ret > OPEN_PLUGIN_BUFSZ)
            ret = 0;
    }
    return ret;
}

static int op_entry_set_param_path(void)
{
    int ret = 0;
    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];

    if (op_entry.param[0] == '\0')
        rb->strcpy(tmp_buf, "/");
    else
        rb->strcpy(tmp_buf, op_entry.param);

    struct browse_context browse = {
        .dirfilter = SHOW_ALL,
        .flags = BROWSE_SELECTONLY | BROWSE_DIRFILTER,
        .title = rb->str(LANG_PARAMETER),
        .icon = Icon_Plugin,
        .root = tmp_buf,
        .buf = tmp_buf,
        .bufsize = sizeof(tmp_buf),
    };

    if (rb->rockbox_browse(&browse) == GO_TO_PREVIOUS)
    {
        ret = rb->strlcpy(op_entry.param, tmp_buf, OPEN_PLUGIN_BUFSZ);
        if (ret > OPEN_PLUGIN_BUFSZ)
            ret = 0;
    }
    return ret;
}

static void op_entry_set_param(void)
{
    if (_yesno_pop(ID2P(LANG_BROWSE)))
        op_entry_set_param_path();

    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];
    rb->strlcpy(tmp_buf, op_entry.param, OPEN_PLUGIN_BUFSZ);
    if (rb->kbd_input(tmp_buf, OPEN_PLUGIN_BUFSZ, NULL) >= 0)
        rb->strlcpy(op_entry.param, tmp_buf, OPEN_PLUGIN_BUFSZ);
}

static int op_et_exclude_invalid(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;
    (void)data;
    if (op_entry->hash == 0 || op_entry->name[0] == '\0')
    {
        logf("Exclude entry %d - Bad hash", item);
        return 0;
    }
    if (op_entry->checksum != open_plugin_csum +
        (op_entry->lang_id <= OPEN_PLUGIN_LANG_INVALID ? 0 : LANG_LAST_INDEX_IN_ARRAY))
    {
        logf("Exclude entry %d - Bad csum", item);
        return 0;
    }
    if (!rb->file_exists(op_entry->path))
    {
        logf("Exclude entry %d - Bad path '%s'", item, op_entry->path);
        return 0;
    }
    logf("%s Include entry %d %s - hash %x", __func__, item,
         (op_entry->lang_id >= 0 ? "[Builtin]" : "[User]"), op_entry->hash);
    return 1;
}

static int op_et_exclude_hash(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;

    if (data)
    {
        uint32_t *hash = data;

        if (op_entry->hash != *hash)
            return op_et_exclude_invalid(op_entry, item, data);
    }
    logf("Exclude entry %d - hash %x", item, op_entry->hash);
    return 0;
}

static int op_et_exclude_builtin(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;
    (void)data;

    if (op_entry->lang_id >= 0)
    {
        logf("Exclude entry %d - Builtin", item);
        return 0;
    }

    return op_et_exclude_invalid(op_entry, item, data);
}

static int op_et_exclude_user(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;
    (void)data;

    if (op_entry->lang_id < 0)
    {
        logf("Exclude entry %d - User", item);
        return 0;
    }
    return op_et_exclude_invalid(op_entry, item, data);
}

static int op_entry_transfer(int fd, int fd_tmp,
                            int(*compfn)(struct open_plugin_entry_t*, int, void*),
                            void *data)
{
    int entries = -1;
    int skipped = 0;
    if (fd_tmp >= 0 && fd >= 0 && rb->lseek(fd, 0, SEEK_SET) == 0)
    {
        entries = 0;
        while (rb->read(fd, &op_entry, op_entry_sz) == op_entry_sz)
        {
            if (compfn && compfn(&op_entry, entries + skipped, data) > 0 && op_entry_checksum() > 0)
            {
                rb->write(fd_tmp, &op_entry, op_entry_sz);
                entries++;
            }
            else
                skipped++;
        }
        logf("%s %d entries %d skipped", __func__, entries, skipped);
    }
    else
    {
        logf("%s Error: fd %d, fd_tmp %d, seekfd: %d", __func__,
             fd, fd_tmp, (int) rb->lseek(fd, 0, SEEK_SET));
    }
    return entries + 1;
}

#if 0
static int op_entry_exists(int fd)
{
    logf("%s? %s - %s", __func__, op_entry.name, op_entry.path);
    /* returns index of the entry that already exists if not found returns -1 */
    static struct open_plugin_entry_t op_entry_rd;
    int entries = 0;
    if (fd >= 0 && rb->lseek(fd, 0, SEEK_SET) == 0)
    {
        while (rb->read(fd, &op_entry_rd, op_entry_sz) == op_entry_sz)
        {
            logf("rd: %s - %s", op_entry_rd.name, op_entry_rd.path);
            if (rb->memcmp(&op_entry_rd, &op_entry, op_entry_sz) == 0)
            {
                logf("%s entry exists @ %d", op_entry_rd.name, entries);
                return entries;
            }
            entries++;
        }
    }
    logf("%s is unique in %d entries", op_entry.name, entries);
    return -1;
}
#endif

static bool browse_configs(void)
{
    int entries = 0;
    char tmp_buf[MAX_PATH+1];

    struct browse_context browse = {
        .dirfilter = SHOW_CFG,
        .flags = BROWSE_SELECTONLY | BROWSE_DIRFILTER,
        .title = rb->str(LANG_CUSTOM_CFG),
        .icon = Icon_Plugin,
        .root = ROCKBOX_DIR,
        .buf = tmp_buf,
        .bufsize = sizeof(tmp_buf),
    };

    if (rb->rockbox_browse(&browse) == GO_TO_PREVIOUS)
    {
        logf("import from %s\n", tmp_buf);
        if(rb->filetype_get_attr(tmp_buf) == FILE_ATTR_CFG)
        {
            entries = import_from_config(tmp_buf);
            rb->splashf(HZ *2, "Imported %d entries", entries);
        }
    }
    return (entries > 0);
}

static uint32_t op_entry_add_path(const char *key, const char *plugin, const char *parameter, bool use_key)
{
    char buf[MAX_PATH];
    uint32_t hash;
    uint32_t newhash;
    char *pos = "";;
    int fd_tmp = -1;
    use_key = (use_key == true && key != NULL);

    if (key)
    {
        open_plugin_get_hash(key, &hash);
        op_entry.hash = hash;
    }
    else if (op_entry.lang_id < 0 && plugin)
    {
        /* need to keep the old hash so we can remove the old entry */
        hash = op_entry.hash;
        open_plugin_get_hash(plugin, &newhash);
        op_entry.hash = newhash;
    }
    else
        hash = op_entry.hash;

    if (plugin)
    {
        int fattr = rb->filetype_get_attr(plugin);
        /* name */
        if (use_key)
        {
            op_entry.lang_id = -1;
            rb->strlcpy(op_entry.name, key, OPEN_PLUGIN_NAMESZ);
        }

        if (pathbasename(plugin, (const char **)&pos) == 0)
            pos = "\0";
        if (op_entry.name[0] == '\0' || op_entry.lang_id >= 0)
            rb->strlcpy(op_entry.name, pos, OPEN_PLUGIN_NAMESZ);

        if ((!parameter || parameter[0] == '\0') && fattr != FILE_ATTR_ROCK && fattr != FILE_ATTR_OPX)
        {
            rb->strlcpy(op_entry.param, plugin, OPEN_PLUGIN_BUFSZ);
            parameter = op_entry.param;
            plugin = rb->filetype_get_plugin(fattr, buf, sizeof(buf));
            if (!plugin)
            {
                rb->splashf(HZ * 2, ID2P(LANG_OPEN_PLUGIN_NOT_A_PLUGIN), pos);
                return 0;
            }
        }

        if(fattr == FILE_ATTR_ROCK)
        {
            fd_tmp = rb->open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_tmp < 0)
                return 0;

            /* path */
            if (plugin != op_entry.path)
                rb->strlcpy(op_entry.path, plugin, OPEN_PLUGIN_BUFSZ);

            if(parameter)
            {
                /* param matches lang_id so we want to set parameter */
                bool needs_param = op_entry.lang_id >= 0
                        && (rb->strcmp(parameter, rb->str(op_entry.lang_id)) == 0);
                if (needs_param
                    || (parameter[0] == '\0' && _yesno_pop(ID2P(LANG_PARAMETER))))
                {
                    if (needs_param)
                        op_entry.param[0] = '\0';
                    op_entry_set_param();
                }
                else if (parameter != op_entry.param)
                    rb->strlcpy(op_entry.param, parameter, OPEN_PLUGIN_BUFSZ);
            }

            op_entry_set_checksum();

            if (!use_key)
            {
                if (op_entry.lang_id <0)
                {
                    open_plugin_get_hash(op_entry.name, &hash);
                    op_entry.hash = hash;
                }
                if (hash_exists(op_entry.hash, false) &&
                    _yesno_pop(ID2P(LANG_REALLY_OVERWRITE)) == false)
                {
                    logf("%s error: duplicate key exists: '%s' lang id: %d hash: %x",
                         __func__, key, op_entry.lang_id, op_entry.hash);
                    rb->close(fd_tmp);
                    rb->remove(OPEN_PLUGIN_DAT ".tmp");
                    return op_entry.hash;
                }
            }

            rb->write(fd_tmp, &op_entry, op_entry_sz); /* add new entry first */
        }
        else if(op_entry_read_opx(plugin) == op_entry_sz)
        {
            fd_tmp = rb->open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_tmp < 0)
                return 0;

            if (op_entry.lang_id <0)
            {
                open_plugin_get_hash(op_entry.name, &hash);
            }

            op_entry.hash = hash;
            op_entry_set_checksum();

            if (hash_exists(op_entry.hash, false))
            {
                logf("%s error: duplicate key exists: '%s' lang id: %d hash: %x",
                     __func__, key, op_entry.lang_id, op_entry.hash);
                rb->close(fd_tmp);
                rb->remove(OPEN_PLUGIN_DAT ".tmp");
                return op_entry.hash;
            }

            rb->write(fd_tmp, &op_entry, op_entry_sz); /* add new entry first */
        }
        else
        {
            if (op_entry.lang_id != LANG_SHORTCUTS)
                rb->splashf(HZ * 2, ID2P(LANG_OPEN_PLUGIN_NOT_A_PLUGIN), pos);
            return 0;
        }
    }

    /*logf("OP add key: '%s' lang id: %d hash: %x", op_entry.name, op_entry.lang_id, op_entry.hash);*/
    if (op_entry_transfer(fd_dat, fd_tmp, op_et_exclude_hash, &hash) > 0)
    {
        rb->close(fd_tmp);
        rb->close(fd_dat);
        fd_dat = -1;
        rb->remove(OPEN_PLUGIN_DAT);
        rb->rename(OPEN_PLUGIN_DAT ".tmp", OPEN_PLUGIN_DAT);
    }
    else
    {
        rb->close(fd_tmp);
        rb->remove(OPEN_PLUGIN_DAT ".tmp");
        hash = 0;
    }

    return hash;
}

void op_entry_browse_add(int selection)
{
    logf("%s", __func__);
    char* key;
    op_entry_read(fd_dat, selection, op_entry_sz);
    if (op_entry_set_path() > 0)
    {
        if (op_entry.lang_id >= 0)
            key = rb->str(op_entry.lang_id);
        else
            key = op_entry.path;

        op_entry_add_path(key, op_entry.path, NULL, false);
    }
}

static void op_entry_remove(int selection)
{
    int entries = rb->lseek(fd_dat, 0, SEEK_END) / op_entry_sz;
    int32_t hash = 0;
    int lang_id = -1;

    if (entries > 0 && _yesno_pop(ID2P(LANG_REMOVE)))
    {
        op_entry_read(fd_dat, selection, op_entry_sz);
        if (rb->lseek(fd_dat, selection  * op_entry_sz, SEEK_SET) >= 0)
        {
            if (op_entry.lang_id >= 0)
            {
                lang_id = op_entry.lang_id;
                hash = op_entry.hash;
                if(lang_id == LANG_START_SCREEN
                   && rb->global_settings->start_in_screen == GO_TO_PLUGIN +2)
                {
                    rb->global_settings->start_in_screen = GO_TO_ROOT + 2; /* default */
                }
            }
            hash_exists(op_entry.hash, true);
            rb->memset(&op_entry, 0, op_entry_sz);
            op_entry.lang_id = lang_id;
            op_entry.hash = hash;
            rb->write(fd_dat, &op_entry, op_entry_sz);
        }
    }
}

static void op_entry_remove_empty(void)
{
    bool resave = false;
    if (fd_dat >= 0 && rb->lseek(fd_dat, 0, SEEK_SET) == 0)
    {
        while (resave == false &&
               rb->read(fd_dat, &op_entry, op_entry_sz) == op_entry_sz)
        {
            if (op_entry.hash == 0 || !op_entry_checksum())
                resave = true;
        }
    }

    if (resave)
    {
        int fd_tmp = rb->open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_tmp < 0)
            return;

        if ((op_entry_transfer(fd_dat, fd_tmp, &op_et_exclude_user, NULL)
            + op_entry_transfer(fd_dat, fd_tmp, &op_et_exclude_builtin, NULL)) > 0)
        {
            rb->close(fd_tmp);
            rb->close(fd_dat);
            fd_dat = -1;
            rb->remove(OPEN_PLUGIN_DAT);
            rb->rename(OPEN_PLUGIN_DAT ".tmp", OPEN_PLUGIN_DAT);
        }
        else
        {
            rb->close(fd_tmp);
            rb->remove(OPEN_PLUGIN_DAT ".tmp");
        }
    }

}

static int op_entry_run(void)
{
    int ret = PLUGIN_ERROR;
    char* path;
    char* param;
    if (op_entry.hash != 0 && op_entry.path[0] != '\0')
    {
        //rb->splash(1, ID2P(LANG_OPEN_PLUGIN));
        path = op_entry.path;
        param = op_entry.param;
        if (param[0] == '\0')
            param = NULL;

        ret = rb->plugin_open(path, param);
    }
    return ret;
}

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    /*TODO memoize names so we don't keep reading the disk when not necessary */
    if (data == (void*) &MENU_ID_MAIN) /* check address */
    {
        if (op_entry_read_name(fd_dat, selected_item))
        {
            if (op_entry.lang_id >= 0)
                rb->snprintf(buf, buf_len, "%s [%s] ",
                             rb->str(op_entry.lang_id), op_entry.name);
            else if (rb->strlcpy(buf, op_entry.name, buf_len) >= buf_len)
                rb->strcpy(&buf[buf_len-10], " ...");
        }
        else
            return "?";
    }
    else /* op_entry should already be loaded */
    {
        switch(selected_item)
        {
            case 0:
                return ID2P(LANG_NAME);
            case 1:
                if (op_entry.lang_id >= 0)
                    rb->snprintf(buf, buf_len, "%s [%s] ",
                                 rb->str(op_entry.lang_id), op_entry.name);
                else if (rb->strlcpy(buf, op_entry.name, buf_len) >= buf_len)
                    rb->strcpy(&buf[buf_len-10], " ...");
                break;
            case 2:
                return ID2P(LANG_DISPLAY_FULL_PATH);
            case 3:
                if (rb->strlcpy(buf, op_entry.path, buf_len) >= buf_len)
                    rb->strcpy(&buf[buf_len-10], " ...");
                break;
            case 4:
                return ID2P(LANG_PARAMETER);
            case 5:
                if (op_entry.param[0] == '\0')
                    return "[NULL]";
                else if (rb->strlcpy(buf, op_entry.param, buf_len) >= buf_len)
                    rb->strcpy(&buf[buf_len-10], " ...");
                break;
            case 6:
                return "";
            case 7:
                return ID2P(LANG_BACK);
            default:
                return "?";
        }
    }

    return buf;
}

static int list_voice_cb(int list_index, void* data)
{
    if (data == (void*) &MENU_ID_MAIN) /* check address */
    {
        if (op_entry_read_name(fd_dat, list_index))
        {
            if (op_entry.lang_id >= 0)
            {
                rb->talk_id(op_entry.lang_id, false);
                rb->talk_id(VOICE_PAUSE, true);
                rb->talk_force_enqueue_next();
            }
            return rb->talk_spell(op_entry.name, false);
        }
    }
    else
    {
        switch(list_index)
        {
            case 0:
                rb->talk_id(LANG_NAME, false);
                rb->talk_id(VOICE_PAUSE, true);

                if (op_entry.lang_id >= 0)
                {
                    rb->talk_id(op_entry.lang_id, true);
                    rb->talk_id(VOICE_PAUSE, true);
                    rb->talk_force_enqueue_next();
                }
                return rb->talk_spell(op_entry.name, false);
            case 2:
                return rb->talk_id(LANG_DISPLAY_FULL_PATH, false);
            case 4:
                return rb->talk_id(LANG_PARAMETER, false);
            case 6:
                return rb->talk_id(LANG_BACK, false);
            default:
                return 0;
        }
    }

    return 0;
}

static void synclist_set(char* menu_id, int selection, int items, int sel_size)
{
    if (selection < 0)
        selection = 0;

    rb->gui_synclist_init(&lists,list_get_name_cb,
                          menu_id, false, sel_size, NULL);

    rb->gui_synclist_set_voice_callback(&lists, list_voice_cb);
    rb->gui_synclist_set_nb_items(&lists,items);
    rb->gui_synclist_select_item(&lists, selection);
    list_voice_cb(selection, menu_id);
}

static int context_menu_cb(int action,
                            const struct menu_item_ex *this_item,
                            struct gui_synclist *this_list)
{
    (void)this_item;

    int selection = rb->gui_synclist_get_sel_pos(this_list);

    if(action == ACTION_ENTER_MENUITEM)
    {
        if (selection == 0 &&
            op_entry.lang_id >= 0 && op_entry.lang_id != LANG_OPEN_PLUGIN)
        {
            rb->gui_synclist_set_title(this_list,
                                       rb->str(op_entry.lang_id), 0);
        }
    }
    else if ((action == ACTION_STD_OK))
    {
        /*Run, Edit, Remove, Export, Blank, Import, Add, Back*/
        switch(selection)
        {
            case 0:case 1:case 2:case 3:case 4:case 5:case 7:
                return ACTION_STD_OK;
            case 6: /*blank*/
                break;
            default:
                return ACTION_STD_CANCEL;
        }
        rb->gui_synclist_draw(this_list); /* redraw */
        return 0;
    }

    return action;
}

static void edit_menu(int selection)
{
    int selected_item;
    bool exit = false;
    bool name_set = false;
    int action = 0;

    if (!op_entry_read(fd_dat, selection, op_entry_sz))
        return;

    if (op_entry.hash != 0) /* remove old hash */
        hash_exists(op_entry.hash, true);

    uint32_t crc = rb->crc_32(&op_entry, op_entry_sz, 0xffffffff);

    synclist_set(MENU_ID_EDIT, 2, 8, 2);
    rb->gui_synclist_draw(&lists);

    while (!exit)
    {
        action = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);

        if (rb->gui_synclist_do_button(&lists, &action))
            continue;
        selected_item = rb->gui_synclist_get_sel_pos(&lists);
        switch (action)
        {
            case ACTION_STD_OK:
                if (selected_item == 0)
                {
                    name_set = op_entry_set_name();
                }
                else if (selected_item == 2)
                    op_entry_set_path();
                else if (selected_item == 4)
                {
                    op_entry_set_param();
                    /* if user already set the name they probably don't want us to change it */
                    if (!name_set && op_entry.lang_id < 0 && rb->file_exists(op_entry.param))
                    {
                        char *slash = rb->strrchr(op_entry.param, '/');
                        if(slash)
                            rb->strlcpy(op_entry.name, slash+1, OPEN_PLUGIN_NAMESZ);
                    }
                }
                else
                    exit = true;

                rb->gui_synclist_draw(&lists);
                break;
            case ACTION_STD_CANCEL:
                exit = true;
                break;
        }
    }

    if (crc != rb->crc_32(&op_entry, op_entry_sz, 0xffffffff) &&
        _yesno_pop(ID2P(LANG_SAVE)) == true)
    {
        char *param = op_entry.param;
        if (param[0] == '\0')
            param = NULL;

        op_entry_add_path(NULL, op_entry.path, param, false);
        fd_dat = rb->open(OPEN_PLUGIN_DAT, O_RDWR, 0666);
    }
    else
        hash_exists(op_entry.hash, false);
}

static int context_menu(int selection)
{
    int selected_item;
    if (op_entry_read(fd_dat, selection, op_entry_sz))
    {
        MENUITEM_STRINGLIST(menu, op_entry.name, context_menu_cb,
            ID2P(LANG_RUN), ID2P(LANG_EDIT), ID2P(LANG_REMOVE), ID2P(LANG_EXPORT),
            ID2P(LANG_SAVE_SETTINGS), ID2P(LANG_CUSTOM_CFG), ID2P(VOICE_BLANK),
            ID2P(LANG_ADD), ID2P(LANG_BACK));

        selected_item =  rb->do_menu(&menu, 0, NULL, false);
        logf("%s %d", __func__, selected_item);
        switch (selected_item)
        {
            case 0: /*run*/
                return PLUGIN_GOTO_PLUGIN;
            case 1: /*edit*/
                edit_menu(selection);
                break;
            case 2: /*remove*/
                op_entry_remove(selection);
                break;
            case 3: /*export*/
                op_entry_export_opx(selection);
                break;
            case 4: /* export to cfg */
                op_entry_config_export();
            case 5: /*import from cfg*/
                if (browse_configs())
                {
                    rb->plugin_open(rb->plugin_get_current_filename(), "\0");
                    return OP_PLUGIN_RESTART;
                }
                break;
            case 6: /*blank*/
                break;
            case 7: /*add*/
                op_entry_browse_add(-1);
                rb->plugin_open(rb->plugin_get_current_filename(), "\0");
                return OP_PLUGIN_RESTART;
            default:
                break;

        }
        return PLUGIN_OK;
    }
    return PLUGIN_ERROR;
}

/* Read up to buffer_size chars from a quoted string
 * within fd into buffer and return number of bytes read.
 * A string starts with a quote character (single or double quote)
 * and ends with a matching closing quote. Neither opening or closing quotes
 * are stored in buffer. Too small buf or no opening and closing quote is an error.
 * If an error occurs, -1 is returned (and buffer is cleared).
 * If buffer too small file will still be advanced to the closing quote/LF/EOF
 */
static int read_quoted_string(int fd, char* buffer, int buffer_size)
{
    int pos = 0;
    char ch;
    char quote = '\0';
    /*logf("%s fd: %d bufsz: %d", __func__, fd, buffer_size);*/
    while (rb->read(fd, &ch, 1) == 1)
    {
        if (ch == '\n') /*LF marks end of line*/
        {
            rb->lseek(fd, -1, SEEK_CUR);/*back up cursor to LF so calling fn sees it*/
            break; /* fail */
        }
        if (quote == '\0' &&
           (ch == '\'' || ch == '"')) /*handle single or double quotes*/
        {
            quote = ch;
        }
        else if (quote != '\0')
        {
            if (ch == quote)
            {
                if (pos < buffer_size)
                {
                    buffer[pos] = '\0'; /*end quote*/
                    return pos + 1;
                }
                break; /*fail*/
            }
            if (pos < buffer_size)
            {
                buffer[pos] = ch; /*inside quote*/
                pos++;
            }
        }
    }

    /*fail*/
    /*logf("Error %s", __func__);*/
    buffer[0] = '\0';
    return -1;
}

static int lang_english_to_id(const char *english)
{
    int i;
    unsigned char *ptr;
    size_t ptrlen, len = rb->strlen(english);
    for (i = 0; i < LANG_LAST_INDEX_IN_ARRAY; i++) {
        ptr = rb->language_strings[i];
        ptrlen = rb->strlen((char *)ptr);
        if ((ptrlen == len) && rb->memcmp(ptr, english, ptrlen) == 0)
            return i;
    }
    return -1;
}

static bool op_entry_config_import(int cfg_fd, int fd_tmp)
{
    /* NOTE: assumes cfg_fd is valid */
    /*"key", "name", "path", "param"*/
    /*"Start Screen", "logo.rock", "/.rockbox/rocks/demos/logo.rock", ""*/
    /*"[USER]", "text_viewer.rock", "/.rockbox/rocks/viewers/text_viewer.rock", "/text.txt"*/
    rb->memset(&op_entry, 0, op_entry_sz);

    static char errmsg[MAX_PATH];
    static char keybuf[MAX_PATH];
    int32_t lang_id;
    uint32_t hash;

    if (read_quoted_string(cfg_fd, keybuf, sizeof(keybuf)) < 0)
    {
        logf("%s error: importing key entry @ %d", __func__, 0);
        rb->snprintf(errmsg, sizeof(errmsg), "importing key entry  @ %d", 1);
        rb->splashf(HZ*2, ID2P(LANG_ERROR_FORMATSTR), errmsg);
        return false; /* fail */
    }

    lang_id = lang_english_to_id(keybuf);
    if (lang_id < 0)
    {
        int rd = read_quoted_string(cfg_fd, keybuf, sizeof(keybuf)); /* grab name field */
        if(rd < 0)
        {
            logf("%s error: importing key entry @ %d", __func__, 1);
            rb->snprintf(errmsg, sizeof(errmsg), "importing key entry  @ %d", 1);
            rb->splashf(HZ*2, ID2P(LANG_ERROR_FORMATSTR), errmsg);
            return false; /* fail */
        }
        rb->lseek(cfg_fd, -(rd+2), SEEK_CUR); /* restore position to read name again */
    }

    int i, bufsz;
    char *field[3] = {op_entry.name, op_entry.path, op_entry.param};
    for (i = 0, bufsz = OPEN_PLUGIN_NAMESZ; i < (int)ARRAYLEN(field); i++)
    {
        if (read_quoted_string(cfg_fd, field[i], bufsz) < 0)
        {
            logf("%s error: importing entry @ %d", __func__, i);
            logf("OP import key: '%s' name: '%s' '%s' '%s'", keybuf,
                    op_entry.name, op_entry.path, op_entry.param);
            rb->memset(&op_entry, 0, op_entry_sz);
            rb->snprintf(errmsg, sizeof(errmsg), "importing entry %s @ %d", keybuf, i);
            rb->splashf(HZ*2, ID2P(LANG_ERROR_FORMATSTR), errmsg);
            return false; /* fail */
        }
        bufsz = OPEN_PLUGIN_BUFSZ;
    }

    if (!rb->file_exists(op_entry.path))
    {
        logf("%s error: '%s' '%s' does not exist", __func__, keybuf, op_entry.path);
        rb->splashf(HZ*2, ID2P(LANG_PLUGIN_CANT_OPEN), op_entry.path);
        return false; /* fail */
    }

    if (lang_id <0)
    {
        open_plugin_get_hash(op_entry.name, &hash);

    }
    else
    {
        open_plugin_get_hash(keybuf, &hash);
    }

    op_entry.hash     = hash;
    op_entry.lang_id  = lang_id;

    /*logf("OP import key: '%s' name: '%s' '%s' '%s'", keybuf,
         op_entry.name, op_entry.path, op_entry.param);*/

    op_entry_set_checksum();

    if (fd_tmp >= 0 && fd_dat >= 0)
    {
        if (hash_exists(op_entry.hash, false))
        {
            logf("%s error: duplicate key exists: '%s' lang id: %d hash: %x",
                 __func__, keybuf, lang_id, hash);
            return false;
        }

        logf("writing to tmp: %s - %s", op_entry.name, op_entry.path);

        if (rb->write(fd_tmp, &op_entry, op_entry_sz) != op_entry_sz)/* add new entry */
            return false;
    }
    else
    {
        logf("%s error: bad fd dat: %d tmp: %d", __func__, fd_dat, fd_tmp);
        return false;
    }

    return true;
}

static int import_from_config(const char* cfgfile)
{
    logf("%s() %s\r\n", __func__, cfgfile);
    int fd;
    char ch;
    int entries = 0;
    static char line[sizeof("openplugin:")];

    fd = rb->open_utf8(cfgfile, O_RDONLY);
    if (fd < 0)
        return -1;

    int fd_tmp = rb->open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_tmp < 0)
    {
        logf("%s error: can not open '%s'", __func__, OPEN_PLUGIN_DAT ".tmp");
        return -1;
    }

    while ((rb->read(fd, line, sizeof line) - 1) == sizeof(line) - 1)
    {
        if (rb->strncasecmp(line, "openplugin:", sizeof("openplugin:") -1) == 0)
        {
            if (op_entry_config_import(fd, fd_tmp))
                entries++;
        }

        while (rb->read(fd, &ch, 1) == 1) /* continue reading till EOL */
        {
            if (ch == '\n')
                break;
        }
    } /* while(...) */

    rb->close(fd);
#if 1
    if (entries > 0 && op_entry_transfer(fd_dat, fd_tmp, &op_et_exclude_user, NULL) > 0 &&
            op_entry_transfer(fd_dat, fd_tmp, &op_et_exclude_builtin, NULL) > 0)
#else
    if (entries > 0 && op_entry_transfer(fd_dat, fd_tmp, op_et_exclude_invalid, 0) >= 0)
#endif
    {
        logf("%s imported %d entries", __func__, entries);
        rb->close(fd_tmp);
        rb->close(fd_dat);
        fd_dat = -1;
        rb->remove(OPEN_PLUGIN_DAT);
        rb->rename(OPEN_PLUGIN_DAT ".tmp", OPEN_PLUGIN_DAT);
    }
    else
    {
        logf("%s error: can not transfer entries", __func__);
        if (entries > 0)
        {
            logf("%s error: can not transfer entries to '%s'", __func__, OPEN_PLUGIN_DAT ".tmp");
            entries = -1;
        }
        rb->close(fd_tmp);
        rb->remove(OPEN_PLUGIN_DAT ".tmp");
    }

    return entries;
}

static int export_to_config(const char* cfgfile)
{
    logf("%s() %s\r\n", __func__, cfgfile);
    int cfg_fd;
    char *lang_name;
    int  fd;
    int index = 0;

    cfg_fd = rb->open_utf8(cfgfile,  O_WRONLY | O_CREAT | O_TRUNC);
    if (cfg_fd < 0)
        return index;

    fd = rb->open_utf8(OPEN_PLUGIN_DAT, O_RDONLY);

    if (fd < 0)
    {
        logf("%s error: opening %s", __func__, OPEN_PLUGIN_DAT);
        rb->close(cfg_fd);
        rb->remove(cfgfile);
        return index;  /* OPEN_PLUGIN_NOT_FOUND */
    }
    while (op_entry_read(fd, index, op_entry_sz))
    {
        index++;
        /* don't save the LANG_OPEN_PLUGIN entry -- it is for internal use */
        if (op_entry.lang_id == LANG_OPEN_PLUGIN)
        {
            continue;
        }
        if (op_entry.lang_id >=0)
            lang_name = rb->str(op_entry.lang_id);
        else
            lang_name = "[USER]"; /* needs to be an invalid lang string */
        bool dblquote = rb->strchr(op_entry.name, '"') != NULL ||
                        rb->strchr(op_entry.param, '"') != NULL;

        const char* fmtstr = "%s: \"%s\", \"%s\", \"%s\", \"%s\"\n";

        if (dblquote) /* if using double quotes export with single quotes */
        {
            fmtstr = "%s: '%s', '%s', '%s', '%s'\n";
        }
        rb->fdprintf(cfg_fd,fmtstr, OPEN_PLUGIN_CFGNAME,
                         lang_name, op_entry.name, op_entry.path, op_entry.param);

        logf("openplugin[%d]: \"%s\", \"%s\", \"%s\", \"%s\"\n lang id: %d hash: %x, csum: %x\n",
             index, lang_name, op_entry.name, op_entry.path, op_entry.param,
             op_entry.lang_id, op_entry.hash, op_entry.checksum);
    }
    rb->close(fd);
    rb->close(cfg_fd);
    logf("%s exported %d entries", __func__, index);
    if (index == 0)
    {
        /* Nothing to export */
        rb->remove(cfgfile);
    }

    return index;
}

enum plugin_status plugin_start(const void* parameter)
{
    int ret = PLUGIN_OK;
    uint32_t hash = 0;
    int item = -1;
    int selection = -1;
    int action;
    int items;
    int res;
    char *path;
    bool exit = false;

    const int creat_flags = O_RDWR | O_CREAT;

reopen_datfile:

    fd_dat = rb->open(OPEN_PLUGIN_DAT, creat_flags, 0666);

    hash_exists(0, true);
    while (rb->read(fd_dat, &op_entry, op_entry_sz) == op_entry_sz)
    {
        hash_exists(op_entry.hash, false);
    }

    if (!fd_dat)
        exit = true;

    items = rb->lseek(fd_dat, 0, SEEK_END) / op_entry_sz;

    if (parameter)
    {
        path = (char*)parameter;
        while (path[0] == ' ')
            path++;

        if (rb->strncasecmp(path, "-add", 4) == 0)
        {
            parameter = NULL;
            op_entry_browse_add(-1);
            rb->close(fd_dat);
            fd_dat = -1;
            goto reopen_datfile;
        }
        if(rb->filetype_get_attr(path) == FILE_ATTR_CFG)
        {
            int ret = import_from_config(path);
            rb->close(fd_dat);
            if (ret >= 0)
                return PLUGIN_OK;
            return PLUGIN_ERROR;
        }

        res = op_entry_read_opx(path);
        if (res >= 0)
        {
            if (res == op_entry_sz)
            {
                exit = true;
                ret = op_entry_run();
            }
        }
        else
        {
            open_plugin_get_hash(parameter, &hash);
            rb->lseek(fd_dat, 0, SEEK_SET);
            while (rb->read(fd_dat, &op_entry, op_entry_sz) == op_entry_sz)
            {
                item++;
                if (op_entry.hash == hash)
                {
                    selection = item;
                    break;
                }
            }

            if (selection >= 0)
            {
                if (op_entry_read(fd_dat, selection, op_entry_sz))
                {
                    /* param matches lang_id so we want to set the parameter */
                    if (op_entry.lang_id >= 0
                        && rb->strcmp(op_entry.param, rb->str(op_entry.lang_id)) == 0)
                    {
                        op_entry_add_path(NULL, op_entry.path, op_entry.param, false);
                        exit = true;
                    }
                    else
                    {
                        ret = op_entry_run();
                        if (ret == PLUGIN_GOTO_PLUGIN)
                            exit = true;
                    }
                }
            }
            else
            {
                op_entry_read(fd_dat, selection, op_entry_sz);
                if (op_entry_add_path(parameter, parameter, "\0", false) > 0)
                {
                    selection = 0;
                    items++;
                    fd_dat = rb->open(OPEN_PLUGIN_DAT, creat_flags, 0666);
                    if (!fd_dat)
                        exit = true;
                }
            }
        }/* OP_EXT */
    }

    for (int i = items - 1; i > 0 && !exit; i--)
    {
        if (!op_entry_read(fd_dat, i, op_entry_sz))
            items--;
    }

    if (items < 1 && !exit)
    {
        char* cur_filename = rb->plugin_get_current_filename();

        if (op_entry_add_path(rb->str(LANG_ADD), cur_filename, "-add", true))
        {
            rb->close(fd_dat);
            fd_dat = -1;
            parameter = NULL;
            goto reopen_datfile;
        }
        rb->close(fd_dat);
        return PLUGIN_ERROR;
    }

    if (!exit)
    {
        synclist_set(MENU_ID_MAIN, selection, items, 1);
        rb->gui_synclist_draw(&lists);

        while (!exit && fd_dat >= 0)
        {
            action = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);

            if (rb->gui_synclist_do_button(&lists, &action))
                continue;
            selection = rb->gui_synclist_get_sel_pos(&lists);
            switch (action)
            {
                case ACTION_STD_CONTEXT:
                    ret = context_menu(selection);
                    if (ret == OP_PLUGIN_RESTART)
                    {
                        ret = PLUGIN_GOTO_PLUGIN;
                        exit = true;
                        break;
                    }
                    else if (ret != PLUGIN_GOTO_PLUGIN)
                    {
                        synclist_set(MENU_ID_MAIN, selection, items, 1);
                        rb->gui_synclist_draw(&lists);
                        break;
                    }
                    /* fallthrough */
                case ACTION_STD_OK:
                    if (op_entry_read(fd_dat, selection, op_entry_sz))
                    {
                        ret = op_entry_run();
                        exit = true;
                    }
                    break;
                case ACTION_STD_CANCEL:
                case ACTION_STD_MENU:
                {
                    selection = -2;
                    exit = true;
                    break;
                }
                default:
                    if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
                    {
                        op_entry_remove_empty();
                        rb->close(fd_dat);
                        return PLUGIN_USB_CONNECTED;
                    }
                    break;
            }
        }
        op_entry_remove_empty();
    }
    rb->close(fd_dat);
    if (ret != PLUGIN_OK)
        return ret;
    else
        return PLUGIN_OK;
}
