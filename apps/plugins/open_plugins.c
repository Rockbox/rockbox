/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

static int fd_dat;
static struct gui_synclist lists;
struct open_plugin_entry_t op_entry;
static const uint32_t open_plugin_csum = OPEN_PLUGIN_CHECKSUM;
static const off_t op_entry_sz = sizeof(struct open_plugin_entry_t);

/* we only need the names for the first menu so don't bother reading paths yet */
const off_t op_name_sz = OPEN_PLUGIN_NAMESZ + (op_entry.name - (char*)&op_entry);

static uint32_t op_entry_add_path(const char *key, const char *plugin, const char *parameter, bool use_key);

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

static bool op_entry_read(int fd, int selected_item, off_t data_sz)
{
    rb->memset(&op_entry, 0, op_entry_sz);
    op_entry.lang_id = -1;
    return ((selected_item >= 0) &&
        (rb->lseek(fd, selected_item  * op_entry_sz, SEEK_SET) >= 0) &&
        (rb->read(fd, &op_entry, data_sz) == data_sz));
}

static bool op_entry_read_name(int fd, int selected_item)
{
    return op_entry_read(fd, selected_item, op_name_sz);
}

static int op_entry_checksum(void)
{
    if (op_entry.checksum != open_plugin_csum)
    {
        return 0;
    }
    return 1;
}

static int op_entry_read_opx(const char *path)
{
    int ret = -1;
    off_t filesize;
    int fd_opx;
    int len;

    len = rb->strlen(path);
    if(len > OP_LEN && rb->strcasecmp(&((path)[len-OP_LEN]), "." OP_EXT) == 0)
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

static void op_entry_export(int selection)
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
        op_entry.checksum = open_plugin_csum;
}

static void op_entry_set_name(void)
{
    char tmp_buf[OPEN_PLUGIN_NAMESZ+1];
    rb->strlcpy(tmp_buf, op_entry.name, OPEN_PLUGIN_NAMESZ);
    if (rb->kbd_input(tmp_buf, OPEN_PLUGIN_NAMESZ, NULL) >= 0)
        rb->strlcpy(op_entry.name, tmp_buf, OPEN_PLUGIN_NAMESZ);
}

static int op_entry_set_path(void)
{
    int ret = 0;
    struct browse_context browse;
    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];

    if (op_entry.path[0] == '\0')
        rb->strcpy(op_entry.path, PLUGIN_DIR"/");

    rb->browse_context_init(&browse, SHOW_ALL, BROWSE_SELECTONLY, rb->str(LANG_ADD),
                         Icon_Plugin, op_entry.path, NULL);

    browse.buf = tmp_buf;
    browse.bufsize = OPEN_PLUGIN_BUFSZ;

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
    struct browse_context browse;
    char tmp_buf[OPEN_PLUGIN_BUFSZ+1];

    if (op_entry.param[0] == '\0')
        rb->strcpy(tmp_buf, "/");
    else
        rb->strcpy(tmp_buf, op_entry.param);

    rb->browse_context_init(&browse, SHOW_ALL, BROWSE_SELECTONLY, "",
                         Icon_Plugin, tmp_buf, NULL);

    browse.buf = tmp_buf;
    browse.bufsize = OPEN_PLUGIN_BUFSZ;

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

static int op_et_exclude_hash(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;

    if (op_entry->hash == 0 || op_entry->name[0] == '\0')
        return 0;

    if (data)
    {
        uint32_t *hash = data;
        if (op_entry->hash != *hash)
            return 1;
    }
    return 0;
}

static int op_et_exclude_builtin(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;
    (void)data;

    if (op_entry->lang_id >= 0)
        return 0;
    else if(op_entry->hash == 0 || op_entry->name[0] == '\0')
        return 0;

    return 1;
}

static int op_et_exclude_user(struct open_plugin_entry_t *op_entry, int item, void *data)
{
    (void)item;
    (void)data;

    if (op_entry->lang_id < 0)
        return 0;
    else if (op_entry->hash == 0 || op_entry->name[0] == '\0')
        return 0;

    return 1;
}

static int op_entry_transfer(int fd, int fd_tmp,
                            int(*compfn)(struct open_plugin_entry_t*, int, void*),
                            void *data)
{
    int entries = -1;
    if (fd_tmp >= 0 && fd >= 0 && rb->lseek(fd, 0, SEEK_SET) == 0)
    {
        entries = 0;
        while (rb->read(fd, &op_entry, op_entry_sz) == op_entry_sz)
        {
            if (compfn && compfn(&op_entry, entries, data) > 0 && op_entry_checksum() > 0)
            {
                rb->write(fd_tmp, &op_entry, op_entry_sz);
                entries++;
            }
        }
    }
    return entries + 1;
}

static uint32_t op_entry_add_path(const char *key, const char *plugin, const char *parameter, bool use_key)
{
    int len;
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

        len = rb->strlen(pos);
        if(len > ROCK_LEN && rb->strcasecmp(&(pos[len-ROCK_LEN]), "." ROCK_EXT) == 0)
        {
            fd_tmp = rb->open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_tmp < 0)
                return 0;

            /* path */
            if (plugin != op_entry.path)
                rb->strlcpy(op_entry.path, plugin, OPEN_PLUGIN_BUFSZ);

            if(parameter)
            {
                if (parameter[0] == '\0' &&
                    _yesno_pop(ID2P(LANG_PARAMETER)))
                {
                    op_entry_set_param();
                }
                else if (parameter != op_entry.param)
                    rb->strlcpy(op_entry.param, parameter, OPEN_PLUGIN_BUFSZ);

                /* hash on the parameter path if it is a file */
                if (op_entry.lang_id <0 && key == op_entry.path &&
                    rb->file_exists(op_entry.param))
                {
                    open_plugin_get_hash(op_entry.path, &newhash);
                    op_entry.hash = newhash;
                }
            }
            op_entry_set_checksum();
            rb->write(fd_tmp, &op_entry, op_entry_sz); /* add new entry first */
        }
        else if(op_entry_read_opx(plugin) == op_entry_sz)
        {
            fd_tmp = rb->open(OPEN_PLUGIN_DAT ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_tmp < 0)
                return 0;

            if (op_entry.lang_id <0 && rb->file_exists(op_entry.param))
                open_plugin_get_hash(op_entry.param, &hash);
            else
                open_plugin_get_hash(op_entry.path, &hash);

            op_entry.hash = hash;
            op_entry_set_checksum();
            rb->write(fd_tmp, &op_entry, op_entry_sz); /* add new entry first */
        }
        else
        {
            if (op_entry.lang_id != LANG_SHORTCUTS)
                rb->splashf(HZ * 2, rb->str(LANG_OPEN_PLUGIN_NOT_A_PLUGIN), pos);
            return 0;
        }
    }

    if (op_entry_transfer(fd_dat, fd_tmp, op_et_exclude_hash, &hash) > 0)
    {
        rb->close(fd_tmp);
        rb->close(fd_dat);
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
            }
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
    if (fd_dat && rb->lseek(fd_dat, 0, SEEK_SET) == 0)
    {
        while (resave == false &&
               rb->read(fd_dat, &op_entry, op_entry_sz) == op_entry_sz)
        {
            if (op_entry.hash == 0)
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

    rb->gui_synclist_set_icon_callback(&lists,NULL);
    rb->gui_synclist_set_voice_callback(&lists, list_voice_cb);
    rb->gui_synclist_set_nb_items(&lists,items);
    rb->gui_synclist_limit_scroll(&lists,true);
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
            case 0:case 1:case 2:case 3:case 5:
                return ACTION_STD_OK;
            case 4: /*blank*/
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
    int action = 0;

    if (!op_entry_read(fd_dat, selection, op_entry_sz))
        return;

    uint32_t crc = rb->crc_32(&op_entry, op_entry_sz, 0xffffffff);

    synclist_set(MENU_ID_EDIT, 2, 8, 2);
    rb->gui_synclist_draw(&lists);

    while (!exit)
    {
        action = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);

        if (rb->gui_synclist_do_button(&lists,&action,LIST_WRAP_UNLESS_HELD))
            continue;
        selected_item = rb->gui_synclist_get_sel_pos(&lists);
        switch (action)
        {
            case ACTION_STD_OK:
                if (selected_item == 0)
                    op_entry_set_name();
                else if (selected_item == 2)
                    op_entry_set_path();
                else if (selected_item == 4)
                    op_entry_set_param();
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
}

static int context_menu(int selection)
{
    int selected_item;
    if (op_entry_read(fd_dat, selection, op_entry_sz))
    {
        MENUITEM_STRINGLIST(menu, op_entry.name, context_menu_cb,
            ID2P(LANG_RUN), ID2P(LANG_EDIT), ID2P(LANG_REMOVE), ID2P(LANG_EXPORT),
            ID2P(VOICE_BLANK), ID2P(LANG_ADD), ID2P(LANG_BACK));

        selected_item =  rb->do_menu(&menu, 0, NULL, false);
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
                op_entry_export(selection);
                break;
            case 4: /*blank*/
                break;
            case 5: /*add*/
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
            goto reopen_datfile;
        }
    }

    if (parameter)
    {
        path = (char*)parameter;
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
                    ret = op_entry_run();
                    if (ret == PLUGIN_GOTO_PLUGIN)
                        exit = true;
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

    if (items < 1 && !exit)
    {
        char* cur_filename = rb->plugin_get_current_filename();

        if (op_entry_add_path(rb->str(LANG_ADD), cur_filename, "-add", true))
        {
            rb->close(fd_dat);
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

            if (rb->gui_synclist_do_button(&lists,&action,LIST_WRAP_UNLESS_HELD))
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
                case ACTION_STD_OK:
                    if (op_entry_read(fd_dat, selection, op_entry_sz))
                    {
                        ret = op_entry_run();
                        exit = true;
                    }
                    break;
                case ACTION_STD_CANCEL:
                {
                    selection = -2;
                    exit = true;
                    break;
                }
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
