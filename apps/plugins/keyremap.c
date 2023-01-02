/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 William Wilgus
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

#include "plugin.h"
#include "lang_enum.h"

#include "lib/action_helper.h"
#include "lib/button_helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/printcell_helper.h"
#include "lib/kbd_helper.h"

#ifdef ROCKBOX_HAS_LOGF
#define logf rb->logf
#else
#define logf(...) do { } while(0)
#endif

/* CORE_KEYREMAP_FILE */
#include "../core_keymap.h"
#define KMFDIR ROCKBOX_DIR
#define KMFEXT1 ".kmf"
#define KMFEXT2 ".kmf.old"
#define KMFUSER "user_keyremap"
#define MAX_BUTTON_COMBO 5
#define MAX_BUTTON_NAME 32
#define MAX_MENU_NAME 64
#define TEST_COUNTDOWN_MS 1590

struct context_flags {
    char * name;
    int flag;
};

/* flags added to context_name[] */
static struct context_flags context_flags[] = {
    {"UNKNOWN", 0},/* index 0 is an Error */
#ifndef HAS_BUTTON_HOLD
    {"LOCKED", CONTEXT_LOCKED},
#endif
    /*{"PLUGIN", CONTEXT_PLUGIN}, need a custom action list and a way to supply */
#if BUTTON_REMOTE != 0
    {"REMOTE", CONTEXT_REMOTE},
#ifndef HAS_BUTTON_HOLD
    {"REMOTE_LOCKED", CONTEXT_REMOTE | CONTEXT_LOCKED},
#endif
#endif /* BUTTON_REMOTE != 0 */
};

static struct keyremap_buffer_t {
    char * buffer;
    size_t buf_size;
    char *front;
    char *end;
} keyremap_buffer;


struct action_mapping_t {
    int context;
    int display_pos;
    struct button_mapping map;
};

static struct user_context_data_t {
    struct action_mapping_t *ctx_map;
    int ctx_count;
    struct action_mapping_t *act_map;
    int act_count;
} ctx_data;

/* set keys keymap */
static struct setkeys_data_t {
    /* save state in the set keys action view list */
    int view_columns;
    int view_lastcol;
    uint32_t crc32;
} keyset;

/* test keys data */
static struct testkey_data_t {
    struct button_mapping *keymap;
    int action;
    int context;
    int index;
    int countdown;
} keytest;

static struct context_menu_data_t {
    char *menuid;
    int ctx_index;
    int act_index;
    int act_edit_index;
    //const char * ctx_fmt;
    const char * act_fmt;
} ctx_menu_data;
#define ACTIONFMT_LV0 "$%s$%s$%s$%s"
#define ACTVIEW_HEADER "$Context$Action$Button$PreBtn"

#define GOTO_ACTION_DEFAULT_HANDLER (PLUGIN_OK + 1)
#define MENU_ID(x) (((void*)&mainmenu[x]))
#define MENU_MAX_DEPTH 4
/* this enum sets menu order */
enum {
    M_ROOT = 0,
    M_SETKEYS,
    M_TESTKEYS,
    M_RESETKEYS,
    M_EXPORTKEYS,
    M_IMPORTKEYS,
    M_SAVEKEYS,
    M_LOADKEYS,
    M_DELKEYS,
    M_TMPCORE,
    M_SETCORE,
    M_DELCORE,
    M_EXIT,
    M_LAST_MAINITEM, //MAIN MENU ITEM COUNT
/*Menus not directly accessible from main menu*/
    M_ACTIONS = M_LAST_MAINITEM,
    M_BUTTONS,
    M_CONTEXTS,
    M_CONTEXT_EDIT,
    M_LAST_ITEM, //ITEM COUNT
};

struct mainmenu { const char *name; void *menuid; int index; int items;};
static struct mainmenu mainmenu[M_LAST_ITEM] = {
#define MENU_ITEM(ID, NAME, COUNT) [ID]{NAME, MENU_ID(ID), (int)ID, (int)COUNT}
MENU_ITEM(M_ROOT, "Key Remap Plugin", M_LAST_MAINITEM - 1),
MENU_ITEM(M_SETKEYS, "Edit Keymap", 1),
MENU_ITEM(M_TESTKEYS, "Test Keymap", 4),
MENU_ITEM(M_RESETKEYS, "Reset Keymap", 1),
MENU_ITEM(M_EXPORTKEYS, "Export Text Keymap", 1),
MENU_ITEM(M_IMPORTKEYS, "Import Text Keymap", 1),
MENU_ITEM(M_SAVEKEYS, "Save Native Keymap", 1),
MENU_ITEM(M_LOADKEYS, "Load Native Keymaps", 1),
MENU_ITEM(M_DELKEYS, "Delete Keymaps", 1),
MENU_ITEM(M_TMPCORE, "Temp Core Remap", 1),
MENU_ITEM(M_SETCORE, "Set Core Remap", 1),
MENU_ITEM(M_DELCORE, "Remove Core Remap", 1),
MENU_ITEM(M_EXIT, ID2P(LANG_MENU_QUIT), 0),
MENU_ITEM(M_ACTIONS, "Actions", LAST_ACTION_PLACEHOLDER),
MENU_ITEM(M_BUTTONS, "Buttons", -1), /* Set at runtime in plugin_start: */
MENU_ITEM(M_CONTEXTS, "Contexts", LAST_CONTEXT_PLACEHOLDER * ARRAYLEN(context_flags)),
MENU_ITEM(M_CONTEXT_EDIT, "", 5),
#undef MENU_ITEM
};

DIR* kmffiles_dirp = NULL;
static int core_savecount = 0;/* so we don't overwrite the last .old file with revisions */

/* global lists, for everything */
static struct gui_synclist lists;

static void menu_useract_set_positions(void);
static size_t lang_strlcpy(char * dst, const char *src, size_t len)
{
    unsigned char **language_strings = rb->language_strings;
    return rb->strlcpy(dst, P2STR((unsigned char*)src), len);
}

/* Menu stack macros */
static int mlastsel[MENU_MAX_DEPTH * 2 + 1] = {0};
#define PUSH_MENU(id, item) \
    if(mlastsel[0] < MENU_MAX_DEPTH * 2) {mlastsel[++mlastsel[0]] = id;mlastsel[++mlastsel[0]] = item;}
#define POP_MENU(id, item) { item = (mlastsel[0] > 0 ? mlastsel[mlastsel[0]--]:0); \
                id = (mlastsel[0] > 0 ? mlastsel[mlastsel[0]--]:0); }
#define PEEK_MENU_ITEM(item) { item = (mlastsel[0] > 0 ? mlastsel[mlastsel[0]]:0);}
#define PEEK_MENU_ID(id) {id = (mlastsel[0] > 1 ? mlastsel[mlastsel[0]-1]:0);}
#define PEEK_MENU(id, item) PEEK_MENU_ITEM(item) PEEK_MENU_ID(id)
#define SET_MENU_ITEM(item) \
    if(mlastsel[0] > 1) {mlastsel[mlastsel[0]] = item;}

static struct mainmenu *mainitem(int selected_item)
{
    static struct mainmenu empty = {0};
    if (selected_item >= 0 && selected_item < (int) ARRAYLEN(mainmenu))
        return &mainmenu[selected_item];
    else
        return &empty;
}
/* Forward Declarations */
static const char *edit_keymap_name_cb(int selected_item, void* data,char* buf, size_t buf_len);
static int keyremap_import_file(char *filenamebuf, size_t bufsz);
static void synclist_set(int id, int selected_item, int items, int sel_size);

static int prompt_filename(char *buf, size_t bufsz)
{
#define KBD_LAYOUT "abcdefghijklmnop\nqrstuvwxyz |()[]\n1234567890 /._-+\n\n" \
                   "\nABCDEFGHIJKLMNOP\nQRSTUVWXYZ |()[]\n1234567890 /._-+"
    unsigned short kbd[sizeof(KBD_LAYOUT) + 10];
    unsigned short *kbd_p = kbd;
    if (!kbd_create_layout(KBD_LAYOUT, kbd, sizeof(kbd)))
        kbd_p = NULL;

#undef KBD_LAYOUT
    return rb->kbd_input(buf, bufsz, kbd_p) + 1;
}

static void synclist_set_update(int id, int selected_item, int items, int sel_size)
{
    SET_MENU_ITEM(lists.selected_item + 1); /* update selected for previous menu*/
    synclist_set(id, selected_item, items, sel_size);
}

/* returns the actual context & index of the flag is passed in *flagidx */
static int ctx_strip_flagidx(int ctx, int *flagidx)
{
    int ctx_out = ctx % (LAST_CONTEXT_PLACEHOLDER);
    *flagidx = 0;
    if (ctx_out != ctx)
    {
        while (ctx >= LAST_CONTEXT_PLACEHOLDER)
        {
            (*flagidx)++;
            ctx -= LAST_CONTEXT_PLACEHOLDER;
        }
        if (*flagidx >= (int)ARRAYLEN(context_flags))
            *flagidx = 0; /* unknown flag */

        logf("%s ctx: (%d) %s flag idx: (%d) %s\n", __func__,
             ctx, context_name(ctx), *flagidx, context_flags[*flagidx].name);
    }
    return ctx_out;
}

/* combines context name and flag name using '_' to join them */
static char *ctx_name_and_flag(int index)
{
    static char ctx_namebuf[MAX_MENU_NAME];
    char *ctx_name = "?";
    int flagidx;
    int ctx = ctx_strip_flagidx(index, &flagidx);
    if (flagidx == 0)
    {
        ctx_name = context_name(ctx);
    }
    else if (flagidx >= 0 && flagidx < (int)ARRAYLEN(context_flags))
    {
        rb->snprintf(ctx_namebuf, sizeof(ctx_namebuf), "%s_%s",
        context_name(ctx), context_flags[flagidx].name);
        ctx_name = ctx_namebuf;
    }
    return ctx_name;
}

static int btnval_to_index(unsigned int btnvalue)
{
    int index = -1;
    for (int i = 0; i < available_button_count; i++)
    {
        const struct available_button *btn = &available_buttons[i];
        if (btnvalue == btn->value)
        {
            index = i;
            break;
        }
    }
    return index;
}

static int btnval_to_name(char *buf, size_t bufsz, unsigned int btnvalue)
{
    int index = btnval_to_index(btnvalue);
    if (index >= 0)
    {
        return rb->snprintf(buf, bufsz, "%s", available_buttons[index].name);
    }
    else /* this provides names for button combos e.g.(BUTTON_UP|BUTTON_REPEAT) */
    {
        int res = get_button_names(buf, bufsz, btnvalue);
        if (res > 0 && res <= (int)bufsz)
            return res;
    }
    return rb->snprintf(buf, bufsz, "%s", "BUTTON_UNKNOWN");
}

static int keyremap_check_extension(const char* filename)
{
    int found = 0;
    unsigned int len = rb->strlen(filename);
    if (len > sizeof(KMFEXT1) &&
        rb->strcmp(&filename[len - sizeof(KMFEXT1) + 1], KMFEXT1) == 0)
    {
        found = 1;
    }
    else if (len > sizeof(KMFEXT2) &&
        rb->strcmp(&filename[len - sizeof(KMFEXT2) + 1], KMFEXT2) == 0)
    {
        found = 2;
    }
    return found;
}

static int keyremap_count_files(const char *directory)
{
    int nfiles = 0;
    DIR* kmfdirp = NULL;
    kmfdirp = rb->opendir(directory);
    if (kmfdirp != NULL)
    {
        struct dirent *entry;
        while ((entry = rb->readdir(kmfdirp)))
        {
            /* skip directories */
            if ((rb->dir_get_info(kmfdirp, entry).attribute & ATTR_DIRECTORY) != 0)
                continue;
            if (keyremap_check_extension(entry->d_name) > 0)
                nfiles++;
        }
        rb->closedir(kmfdirp);
    }
    return nfiles;
}

static void keyremap_reset_buffer(void)
{
    keyremap_buffer.front = keyremap_buffer.buffer;
    keyremap_buffer.end = keyremap_buffer.front + keyremap_buffer.buf_size;
    rb->memset(keyremap_buffer.front, 0, keyremap_buffer.buf_size);
    ctx_data.ctx_map = (struct action_mapping_t*)keyremap_buffer.front;
    ctx_data.ctx_count = 0;
    ctx_data.act_map = (struct action_mapping_t*) keyremap_buffer.end;
    ctx_data.act_count = 0;

}

static size_t keyremap_write_entries(int fd, struct button_mapping *data, int entry_count)
{
    if (fd < 0 || entry_count <= 0 || !data)
        goto fail;
    size_t bytes_req = sizeof(struct button_mapping) * entry_count;
    size_t bytes = rb->write(fd, data, bytes_req);
    if (bytes == bytes_req)
        return bytes_req;
fail:
    return 0;
}

static size_t keyremap_write_header(int fd, int entry_count)
{
    if (fd < 0 || entry_count < 0)
        goto fail;
    struct button_mapping header = {KEYREMAP_VERSION, KEYREMAP_HEADERID, entry_count};
    return keyremap_write_entries(fd, &header, 1);
fail:
    return 0;
}

static int keyremap_open_file(const char *filename, int *fd, size_t *fsize)
{
    int count = 0;

    while (filename && fd && fsize)
    {
        *fsize = 0;
        *fd = rb->open(filename, O_RDONLY);
        if (*fd)
        {
            *fsize = rb->filesize(*fd);

            count = *fsize / sizeof(struct button_mapping);

            if (count * sizeof(struct button_mapping) != *fsize)
            {
                count = -10;
                break;
            }

            if (count > 1)
            {
                struct button_mapping header = {0};
                rb->read(*fd, &header, sizeof(struct button_mapping));
                if (KEYREMAP_VERSION == header.action_code &&
                    KEYREMAP_HEADERID == header.button_code &&
                    header.pre_button_code == count)
                {
                    count--;
                    *fsize -= sizeof(struct button_mapping);
                }
                else /* Header mismatch */
                {
                    count = -20;
                    break;
                }
            }
        }
        break;
    }
    return count;
}

static int keyremap_map_is_valid(struct action_mapping_t *amap, int context)
{
    int ret = (amap->context == context ? 1: 0);
    struct button_mapping entry = amap->map;
    if (entry.action_code == (int)CONTEXT_STOPSEARCHING ||
        entry.button_code == BUTTON_NONE)
    {
        ret = 0;
    }

    return ret;
}

static struct button_mapping *keyremap_create_temp(int *entries)
{
    logf("%s()", __func__);
    struct button_mapping *tempkeymap;
    int action_offset = 1; /* start of action entries (ctx_count + 1)*/
    int entry_count = 1;/* (ctx_count + ctx_count + act_count) */
    int index = 0;
    int i, j;

    /* count includes a single stop sentinel for the list of contexts */
    /* and a stop sentinel for each group of action remaps as well */
    for (i = 0; i < ctx_data.ctx_count; i++)
    {
        int actions_this_ctx = 0;
        int context = ctx_data.ctx_map[i].context;
        /* how many actions are contained in each context? */
        for (j = 0; j < ctx_data.act_count; j++)
        {
            if (keyremap_map_is_valid(&ctx_data.act_map[j], context) > 0)
            {
                actions_this_ctx++;
            }
        }
        /* only count contexts with remapped actions */
        if (actions_this_ctx != 0)
        {
            action_offset++;
            entry_count += actions_this_ctx + 2;
        }
    }
    size_t keymap_bytes = (entry_count) * sizeof(struct button_mapping);
    *entries = entry_count;
    logf("%s() entry count: %d", __func__, entry_count);
    logf("keyremap bytes: %zu, avail: %zu", keymap_bytes,
         (keyremap_buffer.end - keyremap_buffer.front));
    if (keyremap_buffer.front + keymap_bytes < keyremap_buffer.end)
    {
        tempkeymap = (struct button_mapping *) keyremap_buffer.front;
        rb->memset(tempkeymap, 0, keymap_bytes);
        struct button_mapping *entry = &tempkeymap[index++];
        for (i = 0; i < ctx_data.ctx_count; i++)
        {
            int actions_this_ctx = 0;
            int flagidx;
            int context = ctx_data.ctx_map[i].context;
            /* how many actions are contained in each context? */
            for (j = 0; j < ctx_data.act_count; j++)
            {
                if (keyremap_map_is_valid(&ctx_data.act_map[j], context) > 0)
                {
                    actions_this_ctx += 1;
                }
            }
            /*Don't save contexts with no actions */
            if (actions_this_ctx == 0){ continue; }

            /* convert context x flag to context | flag */
            context = ctx_strip_flagidx(ctx_data.ctx_map[i].context, &flagidx);
            context |= context_flags[flagidx].flag;

            entry->action_code = CORE_CONTEXT_REMAP(context);
            entry->button_code = action_offset; /* offset of first action entry */
            entry->pre_button_code = actions_this_ctx; /* entries (excluding sentinel) */
            entry = &tempkeymap[index++];
            logf("keyremap found context: %d index: %d entries: %d",
                 context, action_offset, actions_this_ctx);
            action_offset += actions_this_ctx + 1; /* including sentinel */
        }
        /* context sentinel */
        entry->action_code = CONTEXT_STOPSEARCHING;;
        entry->button_code = BUTTON_NONE;
        entry->pre_button_code = BUTTON_NONE;
        entry = &tempkeymap[index++];

        for (i = 0; i < ctx_data.ctx_count; i++)
        {
            int actions_this_ctx = 0;
            int context = ctx_data.ctx_map[i].context;

            for (int j = 0; j < ctx_data.act_count; j++)
            {
                if (keyremap_map_is_valid(&ctx_data.act_map[j], context) > 0)
                {
                    struct button_mapping map = ctx_data.act_map[j].map;
                    entry->action_code = map.action_code;
                    entry->button_code = map.button_code;
                    entry->pre_button_code = map.pre_button_code;
                    entry = &tempkeymap[index++];
                    actions_this_ctx++;
                    logf("keyremap: found ctx: %d, act: %d btn: %d pbtn: %d",
                        context, map.action_code, map.button_code, map.pre_button_code);
                }
            }
            /*Don't save sentinel for contexts with no actions */
            if (actions_this_ctx == 0){ continue; }

            /* action sentinel */
            entry->action_code = CONTEXT_STOPSEARCHING;;
            entry->button_code = BUTTON_NONE;
            entry->pre_button_code = BUTTON_NONE;
            entry = &tempkeymap[index++];
        }
    }
    else
    {
        rb->splashf(HZ *2, "Out of Memory");
        logf("keyremap: create temp OOM");
        *entries = 0;
        tempkeymap = NULL;
    }

    return tempkeymap;
}

static int keyremap_save_current(const char *filename)
{
    int status = 0;
    int entry_count;/* (ctx_count + ctx_count + act_count + 1) */

    struct button_mapping *keymap = keyremap_create_temp(&entry_count);

    if (keymap == NULL || entry_count <= 3) /* there should be atleast 4 entries */
        return status;
    keyset.crc32 =
     rb->crc_32(keymap, entry_count * sizeof(struct button_mapping), 0xFFFFFFFF);
    int keyfd = rb->open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (keyremap_write_header(keyfd, entry_count + 1) > 0)
    {
        if (keyremap_write_entries(keyfd, keymap, entry_count) > 0)
            status = 1;
    }
    rb->close(keyfd);

    return status;
}

static void keyremap_save_user_keys(bool notify)
{
    char buf[MAX_PATH];
    int i = 0;
    do
    {
        i++;
        rb->snprintf(buf, sizeof(buf), "%s/%s%d%s", KMFDIR, KMFUSER, i, KMFEXT1);
    } while (i < 100 && rb->file_exists(buf));

    if (notify && prompt_filename(buf, sizeof(buf)) <= 0)
    {
        return;
    }

    if (keyremap_save_current(buf) == 0)
    {
        logf("Error Saving");
        if(notify)
            rb->splash(HZ *2, "Error Saving");
    }
    else if (notify)
        rb->splashf(HZ *2, "Saved %s", buf);
}

static int keyremap_export_current(char *filenamebuf, size_t bufsz)
{
    filenamebuf[bufsz - 1] = '\0';
    int i, j;
    int ctx_count = 0;
    size_t entrylen;

    int entry_count = ctx_data.ctx_count + ctx_data.act_count + 1;

    if (entry_count < 3) /* the header is not counted should be at least 3 entries */
    {
        logf("%s: Not enough entries", __func__);
        return 0;
    }
    int fd = rb->open(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd < 0)
        return -1;
    rb->fdprintf(fd, "# Key Remap\n# Device: %s\n" \
                     "# Entries: %d\n\n", MODEL_NAME, entry_count - 1);
    rb->fdprintf(fd, "# Each entry should be PROPER_CASE and on its own line\n" \
                     "# Comments run to end of line \n");

    for (i = 0; i <= entry_count; i++)
    {
        entrylen = 0;
        rb->memset(filenamebuf, 0, bufsz);
        edit_keymap_name_cb(i, MENU_ID(M_EXPORTKEYS), filenamebuf, bufsz);
        if (i == 0)
        {
            ctx_menu_data.act_fmt = "    {%s%s, %s, %s},\n\n";
            continue;
        }
        else if (i == entry_count)
        {
            rb->strlcpy(filenamebuf, "}\n\n", bufsz);
        }
        char last = '\0';
        for (j = 0; j < (int)bufsz ;j++)
        {
            char ch = filenamebuf[j];
            if (ch == '$' && last == '\0') /*CONTEXT*/
            {
                if (ctx_count > 0)
                    rb->fdprintf(fd, "}\n");
                ctx_count++;
                filenamebuf[j] = '\n';
            }
            if (ch == '\n' && last == '\n')
            {
                entrylen = j;
                break;
            }
            else if (ch == '\0')
                filenamebuf[j] = ',';
            last = ch;
        }

        size_t bytes = rb->write(fd, filenamebuf, entrylen);
        if (bytes != entrylen)
        {
            entry_count = -2;
            goto fail;
        }
    }

fail:
    rb->close(fd);

    return entry_count;
}

static void keyremap_export_user_keys(void)
{
    const bool notify = true;
    char buf[MAX_PATH];
    int i = 0;
    do
    {
        i++;
        rb->snprintf(buf, sizeof(buf), "%s/%s%d%s", "", KMFUSER, i, ".txt");
    } while (i < 100 && rb->file_exists(buf));

    if (notify && prompt_filename(buf, sizeof(buf)) <= 0)
    {
        return;
    }

    if (keyremap_export_current(buf, sizeof(buf)) <= 0)
    {
        if(notify)
            rb->splash(HZ *2, "Error Saving");
    }
    else if (notify)
    {
        rb->snprintf(buf, sizeof(buf), "%s/%s%d%s", "", KMFUSER, i, ".txt");
        rb->splashf(HZ *2, "Saved %s", buf);
    }
}

static void keyremap_import_user_keys(void)
{
    char buf[MAX_PATH];
    struct browse_context browse = {
        .dirfilter = SHOW_ALL,
        .flags = BROWSE_SELECTONLY,
        .title = "Select Keymap",
        .icon = Icon_Plugin,
        .buf = buf,
        .bufsize = sizeof(buf),
        .root = "/",
    };

    if (rb->rockbox_browse(&browse) == GO_TO_PREVIOUS)
    {
        int ret = keyremap_import_file(buf, sizeof(buf));
        if (ret <= 0)
        {
            keyremap_reset_buffer();
            rb->splash(HZ *2, "Error Opening");
        }
        else
            rb->splashf(HZ * 2, "Loaded Text Keymap ");
    }
}

static int keymap_add_context_entry(int context)
{
    int remap_context = CORE_CONTEXT_REMAP(context);
    for (int i = 0; i < ctx_data.ctx_count; i++)
    {
        if (ctx_data.ctx_map[i].map.action_code == remap_context)
            goto fail; /* context exists */
    }
    if (keyremap_buffer.front + sizeof(struct action_mapping_t) > keyremap_buffer.end)
        goto fail;
    keyremap_buffer.front += sizeof(struct action_mapping_t);
    ctx_data.ctx_map[ctx_data.ctx_count].context = context;
    ctx_data.ctx_map[ctx_data.ctx_count].display_pos = -1;
    ctx_data.ctx_map[ctx_data.ctx_count].map.action_code = remap_context;
    ctx_data.ctx_map[ctx_data.ctx_count].map.button_code = 0;
    ctx_data.ctx_map[ctx_data.ctx_count].map.pre_button_code = 0;
    ctx_data.ctx_count++;
    menu_useract_set_positions();
    return ctx_data.ctx_count;
fail:
    return 0;
}

static int keymap_add_button_entry(int context, int action_code,
                                   int button_code, int pre_button_code)
{
    bool hasctx = false;
    for (int i = 0; i < ctx_data.ctx_count; i++)
    {
        if (ctx_data.ctx_map[i].context == context)
        {
            hasctx = true;
            break;
        }
    }
    if (!hasctx || keyremap_buffer.end - sizeof(struct action_mapping_t) < keyremap_buffer.front)
        goto fail;
    keyremap_buffer.end -= sizeof(struct action_mapping_t);
    ctx_data.act_map = (struct action_mapping_t*) keyremap_buffer.end;
    ctx_data.act_map[0].context = context;
    ctx_data.act_map[0].display_pos = -1;
    ctx_data.act_map[0].map.action_code = action_code;
    ctx_data.act_map[0].map.button_code = button_code;
    ctx_data.act_map[0].map.pre_button_code = pre_button_code;
    ctx_data.act_count++;
    menu_useract_set_positions();
    return ctx_data.act_count;
fail:
    return 0;
}

static int get_action_from_str(char *pfield, size_t bufsz)
{
    int act = -1;
    for (int i=0;i < LAST_ACTION_PLACEHOLDER; i++)
    {
        if (rb->strncasecmp(pfield, action_name(i), bufsz) == 0)
        {
            logf("Action Found: %s (%d)", pfield, i);
            act = i;
            break;
        }
    }
    return act;
}

static int get_button_from_str(char *pfield, size_t bufsz)
{
    int btn = -1;
    for (int i=0;i < available_button_count; i++)
    {
        const struct available_button* abtn = &available_buttons[i];
        if (rb->strncasecmp(pfield, abtn->name, bufsz) == 0)
        {
            logf("Button Found: %s (%lx)", abtn->name, abtn->value);
            btn = abtn->value;
            break;
        }
    }
    if (btn < 0) /* Not Fatal */
    {
        logf("Invalid Button %s", pfield);
        rb->splashf(HZ, "Invalid Button %s", pfield);
    }
    return btn;
}

static int parse_action_import_entry(int context, char * pbuf, size_t bufsz)
{
    size_t bufleft;
    int count = 0;
    char ch;
    char *pfirst = NULL;
    char *pfield;
    int field = -1;
    int act = -1;
    int button = BUTTON_NONE;
    int prebtn = BUTTON_NONE;
    while ((ch = *(pbuf)) != '\0')
    {
        pfield = NULL; /* Valid names */
        if ((ch >= 'A' && ch <= 'Z') || ch == '_')
        {
            if (pfirst == NULL)
                pfirst = pbuf;
        }
        else if (ch == ',')
        {
            if (pfirst != NULL)
            {
                field++;
                pfield = pfirst;
                pfirst = NULL;
                *pbuf = '\0';
            }
        }
        else if (ch == ' ' || ch == '|'){;}
        else
            pfirst = NULL;

        if (field == 1 && pfirst != NULL && pbuf[1] == '\0')
        {
                field++;
                pfield = pfirst;
                pfirst = NULL;
        }

        if (pfield != NULL)
        {
            char *pf;

            if (field == 0) /* action */
            {
                char *pact = pfield;
                pf = pfield;
                while ((ch = *(pf)) != '\0')
                {
                    if(ch == ' ')
                        *pf = '\0';
                    else
                        pf++;
                }
                bufleft = bufsz - (pact - pbuf);
                act = get_action_from_str(pact, bufleft);

                if (act < 0)
                {
                    logf("Error Action Expected: %s", pact);
                    return -1;
                }
            }
            else if (field == 1 || field == 2) /* button / pre_btn */
            {
                char *pbtn = pfield;
                pf = pfield;
                while ((ch = *(pf)) != '\0')
                {
                    if (pf[1] == '\0') /* last item? */
                    {
                        pf++;
                        ch = '|';
                        pfield = NULL;
                    }
                    else if (ch == ' ' || ch == '|')
                    {
                        *pf = '\0';
                    }

                    if(ch == '|')
                    {
                        bufleft = bufsz - (pbtn - pbuf);
                        int btn = get_button_from_str(pbtn, bufleft);

                        if (btn > BUTTON_NONE)
                        {
                            if (field == 1)
                                button |= btn;
                            else if (field == 2)
                                prebtn |= btn;
                        }

                        if (pfield != NULL)
                        {
                            pf++;
                            while ((ch = *(pf)) != '\0')
                            {
                                if (*pf == ' ')
                                    pf++;
                                else
                                    break;
                            }
                            pbtn = pf;
                        }
                    }
                    else
                        pf++;
                }

                if (act < 0)
                {
                    logf("Error Action Expected: %s", pfield);
                    return -1;
                }
            }

            pfield = NULL;
        }

        pbuf++;
    }
    if (field == 2)
    {
        count = keymap_add_button_entry(context, act, button, prebtn);
        if (count > 0)
        {
            logf("Added: Ctx: %d, Act: %d, Btn: %d PBtn: %d",
                context, act, button, prebtn);
        }
    }
    return count;
}

static int keyremap_import_file(char *filenamebuf, size_t bufsz)
{
    size_t bufleft;
    int count = 0;
    int line = 0;
    filenamebuf[bufsz - 1] = '\0';
    logf("keyremap: import %s", filenamebuf);
    int fd = rb->open(filenamebuf, O_RDONLY);
    if (fd < 0)
        return -1;

    char ch;
    char *pbuf;
    char *pfirst;

    char *pctx = NULL;
    char *pact;
    int ctx = -1;

    keyremap_reset_buffer();
next_line:
    while (rb->read_line(fd, filenamebuf, (int) bufsz) > 0)
    {
        line++;


        pbuf = filenamebuf;
        pfirst = NULL;
        pact = NULL;
        char *pcomment = rb->strchr(pbuf, '#');
        if (pcomment != NULL)
        {
            logf("ln: %d: Skipped Comment: %s", line, pcomment);
            *pcomment = '\0';
        }

        while ((ch = *(pbuf)) != '\0')
        {
            /* PARSE CONTEXT = { */
            if ((ch >= 'A' && ch <= 'Z') || ch == '_')
            {
                if (pfirst == NULL)
                    pfirst = pbuf;
            }
            else if (ch == ' ')
            {
                if (ctx < 0 && pfirst != NULL)
                {
                    *pbuf = '\0';
                    pctx = pfirst;
                    pfirst = NULL;
                }
            }
            else if (ch == '=')
            {
                if (ctx < 0 && pfirst != NULL)
                {
                    *pbuf = '\0';
                    pbuf++;
                    pctx = pfirst;
                    pfirst = NULL;
                }
                while ((ch = *(pbuf)) != '\0')
                {
                    if (ch == '{')
                        break;
                    pbuf++;
                }
                if (ch == '{' && pctx != NULL)
                {
                    bufleft = bufsz - (pctx - filenamebuf);
                    ctx = -1;
                    int ctx_x_flag_count = (LAST_CONTEXT_PLACEHOLDER 
                                           * ARRAYLEN(context_flags));

                    for (int i=0;i < ctx_x_flag_count ;i++)
                    {
                        /* context x flag */
                        if (rb->strncasecmp(pctx, ctx_name_and_flag(i), bufleft) == 0)
                        {
                            logf("ln: %d: Context Found: %s (%d)", line, pctx, i);
                            if (keymap_add_context_entry(i) <= 0)
                                logf("ln: %d: Context Exists: %s (%d)", line, pctx, i);
                            ctx = i;
                            goto next_line;

                        }
                    }
                    logf("ln: %d: ERROR { Context Expected got: %s", line, pctx);
                    goto fail;
                }
            }
            else if (ch == '}')
            {
                if (ctx >= 0)
                    ctx = -1;
                else
                {
                    logf("ln: %d: ERROR no context, unexpected close {", line);
                    goto fail;
                }
            }
            else if (ch == '{') /* PARSE FIELDS { ACTION, BUTTON, PREBTN } */
            {
                int res = 0;
                if (ctx >= 0)
                {
                    pfirst = pbuf;

                    while ((ch = *(pbuf)) != '\0')
                    {
                        if (ch == '}')
                        {
                            pact = pfirst + 1;
                            pfirst = NULL;
                            *pbuf = '\0';
                            pbuf = "";
                            continue;
                        }
                        pbuf++;
                    }
                    if (pact != NULL)
                    {
                        bufleft = bufsz - (pact - filenamebuf);
                        logf("ln: %d: Entry Found: {%s} (%d)", line, pact, 0);
                        res = parse_action_import_entry(ctx, pact, bufleft);
                    }
                }
                if (res <= 0)
                {
                    logf("ln: %d: ERROR action entry expected", line);
                    goto fail;
                }
                else
                {
                    pbuf = "";
                    continue;
                }
            }
            else
                pfirst = NULL;
            pbuf++;
        }

    }
    rb->close(fd);
    count = ctx_data.ctx_count + ctx_data.act_count;
    return count;

fail:
    rb->close(fd);
    rb->splashf(HZ * 2, "Error @ line %d", line);
    return 0;
}

static int keyremap_load_file(const char *filename)
{
    logf("keyremap: load %s", filename);
    int fd = -1;
    size_t fsize = 0;
    size_t bytes;
    struct button_mapping entry;
    int count = keyremap_open_file(filename, &fd, &fsize);
    logf("keyremap: entries %d", count);
    /* actions are indexed from the first entry after the header save this pos */
    off_t firstpos = rb->lseek(fd, 0, SEEK_CUR);
    off_t ctxpos = firstpos;

    if (count > 0)
    {
        keyremap_reset_buffer();
        while(--count > 0)
        {
            rb->lseek(fd, ctxpos, SEEK_SET); /* next context remap entry */
            bytes = rb->read(fd, &entry, sizeof(struct button_mapping));
            ctxpos = rb->lseek(fd, 0, SEEK_CUR);
            if (bytes != sizeof(struct button_mapping))
            {
                count = -10;
                goto fail;
            }
            if (entry.action_code == (int)CONTEXT_STOPSEARCHING)
            {
                logf("keyremap: end of context entries ");
                break;
            }
            if ((entry.action_code & CONTEXT_REMAPPED) == CONTEXT_REMAPPED)
            {
                int context = (entry.action_code & ~CONTEXT_REMAPPED);
                for (int i = ARRAYLEN(context_flags) - 1; i > 0; i--) /* don't check idx 0*/
                {
                    /* convert context | flag to context x flag */
                    if ((context & context_flags[i].flag) ==  context_flags[i].flag)
                    {
                        logf("found ctx flag %s", context_flags[i].name);
                        context &= ~context_flags[i].flag;
                        context += i * LAST_CONTEXT_PLACEHOLDER;
                    }
                }
                int offset = entry.button_code;
                int entries = entry.pre_button_code;
                if (offset == 0 || entries <= 0)
                {
                    logf("keyremap: error reading offset");
                    count = -15;
                    goto fail;
                }
                logf("keyremap found context: %d file offset: %d entries: %d",
                     context, offset, entries);

                keymap_add_context_entry(context);

                off_t entrypos = firstpos + (offset * sizeof(struct button_mapping));
                rb->lseek(fd, entrypos, SEEK_SET);
                for (int i = 0; i < entries; i++)
                {
                    bytes = rb->read(fd, &entry, sizeof(struct button_mapping));
                    if (bytes == sizeof(struct button_mapping))
                    {
                        int act = entry.action_code;
                        int button = entry.button_code;
                        int prebtn = entry.pre_button_code;

                        if (act == (int)CONTEXT_STOPSEARCHING || button == BUTTON_NONE)
                        {
                            logf("keyremap: entry invalid");
                            goto fail;
                        }
                        logf("keyremap: found ctx: %d, act: %d btn: %d pbtn: %d",
                            context, act, button, prebtn);
                        keymap_add_button_entry(context, act, button, prebtn);
                    }
                    else
                        goto fail;
                }
            }
            else
            {
                logf("keyremap: Invalid context entry");
                keyremap_reset_buffer();
                count = -20;
                goto fail;
            }
        }
    }

    int entries = 0;
    struct button_mapping *keymap = keyremap_create_temp(&entries);
    if (keymap != NULL)
    {
        keyset.crc32 =
        rb->crc_32(keymap, entries * sizeof(struct button_mapping), 0xFFFFFFFF);
    }
fail:
    rb->close(fd);
    if (count <= 0)
        rb->splashf(HZ * 2, "Error Loading %sz", filename);
    return count;
}

static const struct button_mapping* test_get_context_map(int context)
{
    (void)context;

    if (keytest.keymap != NULL && keytest.context >= 0)
    {
        int mapidx = keytest.keymap[keytest.index].button_code;
        int mapcnt = keytest.keymap[keytest.index].pre_button_code;
        /* make fallthrough to the test context*/
        keytest.keymap[mapidx + mapcnt].action_code = keytest.context;
        static const struct button_mapping *testctx[] = { NULL };
        testctx[0] = &keytest.keymap[mapidx];
        return testctx[0];
    }
    else
        return NULL;
}

static const char *kmffiles_name_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    /* found kmf filenames returned by each call kmffiles_dirp keeps state
     * selected_item = 0 resets state */

    (void)data;
    buf[0] = '\0';
    if (selected_item == 0)
    {
        rb->closedir(kmffiles_dirp);
        kmffiles_dirp = rb->opendir(KMFDIR);
    }
    if (kmffiles_dirp != NULL)
    {
        struct dirent *entry;
        while ((entry = rb->readdir(kmffiles_dirp)))
        {
            /* skip directories */
            if ((rb->dir_get_info(kmffiles_dirp, entry).attribute & ATTR_DIRECTORY) != 0)
                continue;
            if (keyremap_check_extension(entry->d_name) > 0)
            {
                rb->snprintf(buf, buf_len, "%s", entry->d_name);
                return buf;
            }
        }
    }
    return "Error!";
}

static void menu_useract_set_positions(void)
{
    /* responsible for item ordering to display action edit interface */
    int display_pos = 0; /* start at item 0*/
    int i, j;
    for (i = 0; i < ctx_data.ctx_count; i++)
    {
        int context = ctx_data.ctx_map[i].context;
        ctx_data.ctx_map[i].display_pos = display_pos++;
        /* how many actions are contained in this context? */
        for (j = 0; j < ctx_data.act_count; j++)
        {
            if (ctx_data.act_map[j].context == context)
            {
                ctx_data.act_map[j].display_pos = display_pos++;
            }
        }
    }
}

static const char *menu_useract_items_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    static char buf_button[MAX_BUTTON_NAME * MAX_BUTTON_COMBO];
    static char buf_prebtn[MAX_BUTTON_NAME * MAX_BUTTON_COMBO];
    int i;
    int szbtn = sizeof("BUTTON");
    int szctx = sizeof("CONTEXT");
    int szact = sizeof("ACTION");
    const char* ctxfmt = "%s";

    if (data == MENU_ID(M_EXPORTKEYS))
    {
        szbtn = 0;
        szctx = 0;
        szact = 0;
        ctxfmt = "$%s = {\n\n";
    }
    buf[0] = '\0';
    for(i = 0; i < ctx_data.ctx_count; i ++)
    {
        if (ctx_data.ctx_map[i].display_pos == selected_item)
        {
            if (ctx_data.act_count == 0)
                rb->snprintf(buf, buf_len, "%s$%s",
                    ctx_name_and_flag(ctx_data.ctx_map[i].context),
                    "Select$to add$actions");
            else
                rb->snprintf(buf, buf_len, ctxfmt, ctx_name_and_flag(ctx_data.ctx_map[i].context));
            return buf;
        }
    }
    for(i = 0; i < ctx_data.act_count; i ++)
    {
        if (ctx_data.act_map[i].display_pos == selected_item)
        {
            int context = ctx_data.act_map[i].context;
            char ctxbuf[action_helper_maxbuffer];
            char *pctxbuf = "\0";
            char *pactname;
            if (data != MENU_ID(M_EXPORTKEYS))
            {
                pctxbuf = ctxbuf;
                rb->snprintf(ctxbuf, sizeof(ctxbuf), ctxfmt, ctx_name_and_flag(context));
                pctxbuf += szctx;//sizeof("CONTEXT")
            }
            struct button_mapping * bm = &ctx_data.act_map[i].map;
            pactname = action_name(bm->action_code);
            pactname += szact;//sizeof("ACTION")
            /* BUTTON & PRE_BUTTON */
            btnval_to_name(buf_button, sizeof(buf_button), bm->button_code);
            btnval_to_name(buf_prebtn, sizeof(buf_prebtn), bm->pre_button_code);

            rb->snprintf(buf, buf_len, ctx_menu_data.act_fmt, pctxbuf,
                pactname, buf_button + szbtn, buf_prebtn + szbtn);
            return buf;
        }
    }
    return "Error!";
}

static const char *edit_keymap_name_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    buf[0] = '\0';
    if (selected_item == 0)
    {
        rb->snprintf(buf, buf_len, "Add Context");
        ctx_menu_data.act_index = -1;
        ctx_menu_data.ctx_index = -1;
        ctx_menu_data.act_fmt = ACTIONFMT_LV0;
    }
    else if (ctx_data.ctx_count > 0)
    {
        return menu_useract_items_cb(selected_item - 1, data, buf, buf_len);
    }
    return buf;
}

static const char *test_keymap_name_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    (void)data;
    buf[0] = '\0';
    if (keytest.context >= 0)
    {
        if (selected_item == 0)
            rb->snprintf(buf, buf_len, "< %s >", ctx_name_and_flag(keytest.context));
        else if (selected_item == 1)
        {
            if (keytest.countdown >= 10)
                rb->snprintf(buf, buf_len, "Testing %d", keytest.countdown / 10);
            else
                rb->snprintf(buf, buf_len, "Start test");
        }
        else if (selected_item == 2)
            rb->snprintf(buf, buf_len, "%s", action_name(keytest.action));
    }
    return buf;
}

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    buf[0] = '\0';
    const struct mainmenu *cur = (struct mainmenu *) data;
    if (data == MENU_ID(M_ROOT))
        return mainitem(selected_item + 1)->name;
    else if (selected_item >= cur->items - 1)
    {
        return ID2P(LANG_BACK);
    }
    if (data == MENU_ID(M_SETKEYS))
    {
        return edit_keymap_name_cb(selected_item, data, buf, buf_len);
    }
    else if (data == MENU_ID(M_BUTTONS))
    {
        const struct available_button *btn = &available_buttons[selected_item];
        rb->snprintf(buf, buf_len, "%s: [0x%X] ", btn->name, (unsigned int) btn->value);
        return buf;
    }
    else if (data == MENU_ID(M_ACTIONS))
    {
        return action_name(selected_item);
    }
    else if (data == MENU_ID(M_CONTEXTS))
    {
        return ctx_name_and_flag(selected_item);
    }
    else if (data == MENU_ID(M_DELKEYS) || data == MENU_ID(M_LOADKEYS))
    {
        /* need to iterate the callback for the items off screen to
         * keep ordering, this limits the menu to only the main screen :( */
        int start_item = lists.start_item[SCREEN_MAIN];
        if (start_item != 0 && start_item == selected_item)
        {
            for (int i = 0; i < start_item; i++)
                kmffiles_name_cb(i, data, buf, buf_len);
        }
        return kmffiles_name_cb(selected_item, data, buf, buf_len);
    }
    else if (data == MENU_ID(M_TESTKEYS))
    {
        return test_keymap_name_cb(selected_item, data, buf, buf_len);
    }
    return buf;
}

static int list_voice_cb(int list_index, void* data)
{
    if (!rb->global_settings->talk_menu)
        return -1;

    if (data == MENU_ID(M_ROOT))
    {
        const char * name = mainitem(list_index)->name;
        long id = P2ID((const unsigned char *)name);
            if(id>=0)
                rb->talk_id(id, true);
            else
                rb->talk_spell(name, true);
    }
    else
    {
        char buf[MAX_MENU_NAME];
        const char* name = list_get_name_cb(list_index, data, buf, sizeof(buf));
        long id = P2ID((const unsigned char *)name);
        if(id>=0)
            rb->talk_id(id, true);
        else
            rb->talk_spell(name, true);
    }
    return 0;
}

int menu_action_root(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
#ifdef ROCKBOX_HAS_LOGF
    char logfnamebuf[64];
#endif

    if (*action == ACTION_STD_OK)
    {
        struct mainmenu *cur = mainitem(selected_item + 1);
        if (cur != NULL)
        {
#ifdef ROCKBOX_HAS_LOGF
            lang_strlcpy(logfnamebuf, cur->name, sizeof(logfnamebuf));
            logf("Root select menu -> %s(%d) ", logfnamebuf, cur->index);
#endif
            if (cur->menuid == MENU_ID(M_SETKEYS))
            {
                keyset.view_lastcol = -1;
            }
            else if (cur->menuid == MENU_ID(M_TMPCORE))
            {
                int entry_count;/* (ctx_count + ctx_count + act_count + 1) */
                struct button_mapping *keymap = keyremap_create_temp(&entry_count);
                if (rb->core_set_keyremap(keymap, entry_count) >= 0)
                    rb->splash(HZ *2, "Keymap Applied");
                else
                    rb->splash(HZ *2, "Error Applying");

                goto default_handler;
            }
            else if (cur->menuid == MENU_ID(M_SETCORE))
            {
                if (rb->file_exists(CORE_KEYREMAP_FILE) && 0 == core_savecount++)
                {
                    rb->rename(CORE_KEYREMAP_FILE, CORE_KEYREMAP_FILE".old"); /* make a backup */
                }
                if (keyremap_save_current(CORE_KEYREMAP_FILE) == 0)
                    rb->splash(HZ *2, "Error Saving");
                else
                {
                    rb->splash(HZ *2, "Saved");
                    int entry_count;/* (ctx_count + ctx_count + act_count + 1) */
                    struct button_mapping *keymap = keyremap_create_temp(&entry_count);
                    rb->core_set_keyremap(keymap, entry_count);
                }
                goto default_handler;
            }
            else if (cur->menuid == MENU_ID(M_DELCORE))
            {
                rb->core_set_keyremap(NULL, -1);
                if (rb->file_exists(CORE_KEYREMAP_FILE))
                {
                    rb->rename(CORE_KEYREMAP_FILE, KMFDIR "/core_deleted" KMFEXT2);
                    rb->splash(HZ *2, "Removed");
                }
                else
                    rb->splash(HZ *2, "Error Removing");

                goto default_handler;
            }
            else if (cur->menuid == MENU_ID(M_SAVEKEYS))
            {
                keyremap_save_user_keys(true);
                goto default_handler;
            }
            else if (cur->menuid == MENU_ID(M_EXPORTKEYS))
            {
                keyremap_export_user_keys();
                goto default_handler;
            }
            else if (cur->menuid == MENU_ID(M_IMPORTKEYS))
            {
                keyremap_import_user_keys();
                goto default_handler;
            }
            else if (cur->menuid == MENU_ID(M_DELKEYS) ||
                     cur->menuid == MENU_ID(M_LOADKEYS))
            {
                cur->items = keyremap_count_files(KMFDIR) + 1;
            }
            else if (cur->menuid == MENU_ID(M_TESTKEYS))
            {
                int entries = 0;
                keytest.keymap = keyremap_create_temp(&entries);
                if (entries > 0)
                {
                    struct button_mapping *entry = &keytest.keymap[0];
                    if (entry->action_code != (int)CONTEXT_STOPSEARCHING
                        && (entry->action_code & CONTEXT_REMAPPED) == CONTEXT_REMAPPED)
                    {
                        keytest.context = (entry->action_code & ~CONTEXT_REMAPPED);
                    }
                    else
                        keytest.context = -1;
                }
                else
                {
                    keytest.keymap = NULL;
                    keytest.context = -1;
                }
                keytest.action = ACTION_NONE;
                keytest.countdown = 0;
            }
            else if (cur->menuid == MENU_ID(M_RESETKEYS))
            {
                if (rb->yesno_pop("Delete Current Entries?") == true)
                {
                    keyremap_reset_buffer();
                }
                goto default_handler;
            }
        }

        if (cur->menuid == NULL || cur->menuid == MENU_ID(M_EXIT))
        {
            logf("Root menu %s", (cur->menuid) == NULL ? "NULL":"Exit");
            *action = ACTION_STD_CANCEL;
            *exit = true;
        }
        else
        {
#ifdef ROCKBOX_HAS_LOGF
            lang_strlcpy(logfnamebuf, cur->name, sizeof(logfnamebuf));
            logf("Root load menu -> %s(%d) ", logfnamebuf, cur->index);
#endif
            synclist_set_update(cur->index, 0, cur->items, 1);
            rb->gui_synclist_draw(lists);
            *action = ACTION_NONE;
        }
    }

    return PLUGIN_OK;
default_handler:
    return GOTO_ACTION_DEFAULT_HANDLER;
}

int menu_action_setkeys(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
    (void) exit;
    int i;
    struct mainmenu *cur = (struct mainmenu *)lists->data;
    if (*action == ACTION_STD_OK)
    {
        if (selected_item == 0) /*add_context*/
        {
            const struct mainmenu *mainm = &mainmenu[M_CONTEXTS];
            synclist_set_update(mainm->index, 0, mainm->items, 1);
            rb->gui_synclist_draw(lists);
            goto default_handler;
        }
        else if (selected_item < lists->nb_items - 1)/* not back*/
        {
            bool add_action = false;
            for (i = 0; i < ctx_data.ctx_count; i++)
            {
                if (ctx_data.ctx_map[i].display_pos == selected_item - 1)
                {
                    add_action = true;
                    break;
                }
            }

            if (add_action)
            {
                keymap_add_button_entry(ctx_data.ctx_map[i].context, ACTION_NONE, BUTTON_NONE, BUTTON_NONE);
                cur->items++;
                lists->nb_items++;
                goto default_handler;
            }
            else
            {
                keyset.view_lastcol = printcell_increment_column(lists, 1, true);
                *action = ACTION_NONE;
            }
        }
    }
    else if (*action == ACTION_STD_CANCEL)
    {
        keyset.view_lastcol = printcell_increment_column(lists, -1, true);
        if (keyset.view_lastcol != keyset.view_columns - 1)
        {
            *action = ACTION_NONE;
        }
    }
    else if (*action == ACTION_STD_CONTEXT)
    {
        int col = keyset.view_lastcol;
        for (i = 0; i < ctx_data.act_count; i++)
        {
            if (ctx_data.act_map[i].display_pos == selected_item - 1)
            {
                int context = ctx_data.act_map[i].context;
                int act     = ctx_data.act_map[i].map.action_code;
                int button  = ctx_data.act_map[i].map.button_code;
                int prebtn  = ctx_data.act_map[i].map.pre_button_code;

                if (col < 0)
                {
                    rb->splashf(HZ, "short press increments columns");

                }
                else if (col == 0) /* Context */
                {
                    const struct mainmenu *mainm = &mainmenu[M_CONTEXTS];
                    synclist_set_update(mainm->index, context, mainm->items, 1);
                    rb->gui_synclist_draw(lists);
                    goto default_handler;

                }
                else if (col == 1) /* Action */
                {
                    const struct mainmenu *mainm = &mainmenu[M_ACTIONS];
                    synclist_set_update(mainm->index, act, mainm->items, 1);
                    rb->gui_synclist_draw(lists);
                    goto default_handler;
                }
                else if (col == 2) /* Button */
                {
                    const struct mainmenu *mainm = &mainmenu[M_BUTTONS];
                    int btnidx = btnval_to_index(button);
                    synclist_set_update(mainm->index, btnidx, mainm->items, 1);
                    rb->gui_synclist_draw(lists);
                    goto default_handler;
                }
                else if (col == 3) /* PreBtn */
                {
                    const struct mainmenu *mainm = &mainmenu[M_BUTTONS];
                    int pbtnidx = btnval_to_index(prebtn);
                    synclist_set_update(mainm->index, pbtnidx, mainm->items, 1);
                    rb->gui_synclist_draw(lists);
                    goto default_handler;
                }
            }
        }
    }
    return PLUGIN_OK;
default_handler:
    return GOTO_ACTION_DEFAULT_HANDLER;
}

int menu_action_testkeys(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
    (void) exit;
    (void) lists;
    static int last_count = 0;
    if (keytest.keymap != NULL && keytest.countdown >= 10 && keytest.context >= 0)
    {
        int newact = rb->get_custom_action(CONTEXT_PLUGIN, HZ / 10, test_get_context_map);
        if (newact == ACTION_NONE)
        {
            keytest.countdown--;
            if (last_count - keytest.countdown > 10)
                keytest.action = newact;
        }
        else
        {
            last_count = keytest.countdown;
            keytest.action = newact;
        }
        *action = ACTION_REDRAW;
        goto default_handler;
    }

    if (*action == ACTION_STD_CANCEL && selected_item == 0 && keytest.keymap != NULL)
    {
        keytest.index -= 2;
        if (keytest.index < 0)
            keytest.index = 0;
        *action = ACTION_STD_OK;
    }

    if (*action == ACTION_STD_OK)
    {
        if (selected_item == 0)
        {
            if (keytest.keymap != NULL)
            {
                struct button_mapping *entry = &keytest.keymap[++keytest.index];
                if (entry->action_code != (int) CONTEXT_STOPSEARCHING
                    && (entry->action_code & CONTEXT_REMAPPED) == CONTEXT_REMAPPED)
                {
                    keytest.context = (entry->action_code & ~CONTEXT_REMAPPED);
                }
                else
                {
                    entry = &keytest.keymap[0];
                    if (entry->action_code != (int)CONTEXT_STOPSEARCHING
                        && (entry->action_code & CONTEXT_REMAPPED) == CONTEXT_REMAPPED)
                    {
                        keytest.context = (entry->action_code & ~CONTEXT_REMAPPED);
                        keytest.index = 0;
                    }
                    else
                    {
                        keytest.keymap = NULL;
                        keytest.context = -1;
                        keytest.index = -1;
                        keytest.action = ACTION_NONE;
                        keytest.countdown = 0;
                    }
                }
            }
        }
        else if (selected_item == 1)
        {
            keytest.countdown = TEST_COUNTDOWN_MS / 10;
            keytest.action = ACTION_NONE;
        }
    }

    return PLUGIN_OK;
default_handler:
    return GOTO_ACTION_DEFAULT_HANDLER;
}

int menu_action_listfiles(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
    (void) exit;
    int i;
    if (*action == ACTION_STD_OK)
    {
        struct mainmenu *cur = (struct mainmenu *)lists->data;
        if (selected_item >= (cur->items) - 1)/*back*/
        {
            *action = ACTION_STD_CANCEL;
        }
        else
        {
            /* iterate to the desired file */
            char buf[MAX_PATH];
            int len = rb->snprintf(buf, sizeof(buf), "%s/", KMFDIR);
            if (len < (int) sizeof(buf))
            {
                char *name = &buf[len];
                size_t bufleft = sizeof(buf) - len;
                list_get_name_cb(0, lists->data, name, bufleft);
                for (i = 1; i <= selected_item; i++)
                {
                    list_get_name_cb(i, lists->data, name, bufleft);
                }

                if (lists->data == MENU_ID(M_DELKEYS))
                {
                    const char *lines[] = {ID2P(LANG_DELETE), buf};
                    const struct text_message message={lines, 2};
                    if (rb->gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES)
                    {
                        rb->remove(buf);
                        cur->items--;
                        lists->nb_items--;
                        *action = ACTION_NONE;
                    }
                }
                else if (lists->data == MENU_ID(M_LOADKEYS))
                {
                    if (keyremap_load_file(buf) > 0)
                    {
                        rb->splashf(HZ * 2, "Loaded %s ", buf);
                        *action = ACTION_STD_CANCEL;
                    }
                }
            }
        }
    }
    return PLUGIN_OK;
}

int menu_action_submenus(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
#ifdef ROCKBOX_HAS_LOGF
    char logfnamebuf[64];
#endif
    (void) exit;
    if (*action == ACTION_STD_OK)
    {
            struct mainmenu *cur = (struct mainmenu *)lists->data;
            if (selected_item >= (cur->items) - 1)/*back*/
            {
                *action = ACTION_STD_CANCEL;
            }
            else if (lists->data == MENU_ID(M_ACTIONS))
            {
#ifdef ROCKBOX_HAS_LOGF
                const char *name = list_get_name_cb(selected_item, lists->data, logfnamebuf, sizeof(logfnamebuf));
                logf("ACT %s %d (0x%X)", name, selected_item, selected_item);
#endif
                int id, item;
                POP_MENU(id, item);
                POP_MENU(id, item);
                const struct mainmenu *lastm = &mainmenu[id];

                if (id == M_SETKEYS)
                {
                    for (int i = 0; i < ctx_data.act_count; i++)
                    {
                        if (ctx_data.act_map[i].display_pos == item - 2)
                        {
                            int col = keyset.view_lastcol;
                            struct button_mapping * bm = &ctx_data.act_map[i].map;
                            if (col == 1) /* Action */
                            {
                                bm->action_code = selected_item;
                            }
                            break;
                        }
                    }
                }
                synclist_set(lastm->index, item-1, lastm->items, 1);
                rb->gui_synclist_draw(lists);
            }
            else if (lists->data == MENU_ID(M_CONTEXTS))
            {
#ifdef ROCKBOX_HAS_LOGF
                const char *name = list_get_name_cb(selected_item, lists->data, logfnamebuf, sizeof(logfnamebuf));
                logf("CTX %s %d (0x%X)", name, selected_item, selected_item);
#endif
                int id, item;
                POP_MENU(id, item);
                POP_MENU(id, item);
                struct mainmenu *lastm = &mainmenu[id];
                if (id == M_SETKEYS)
                {
                    bool found = false;
                    int newctx = selected_item;

                    for (int i = 0; i < ctx_data.act_count; i++)
                    {
                        if (ctx_data.act_map[i].display_pos == item - 2)
                        {
                            int col = keyset.view_lastcol;
                            if (col == 0) /* Context */
                            {
                                ctx_data.act_map[i].context = newctx;
                                /* check if this context exists (if not create it) */
                                for (int j = 0; j < ctx_data.ctx_count; j++)
                                {
                                    if (ctx_data.ctx_map[j].context == newctx)
                                    {
                                        found = true;;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                    if (found == false)
                    {
                        keymap_add_context_entry(newctx);
                        lastm->items++;
                    }
                }
                synclist_set(lastm->index, item-1, lastm->items, 1);
                rb->gui_synclist_draw(lists);
            }
            else if (lists->data == MENU_ID(M_BUTTONS))
            {
#ifdef ROCKBOX_HAS_LOGF
                const char *name = list_get_name_cb(selected_item, lists->data, logfnamebuf, sizeof(logfnamebuf));
                logf("BTN %s", name);
#endif
                int id, item;
                POP_MENU(id, item);
                POP_MENU(id, item);
                const struct mainmenu *lastm = &mainmenu[id];

                if (id == M_SETKEYS)
                {
                    for (int i = 0; i < ctx_data.act_count; i++)
                    {
                        if (ctx_data.act_map[i].display_pos == item - 2)
                        {
                            int col = keyset.view_lastcol;
                            struct button_mapping * bm = &ctx_data.act_map[i].map;
                            const struct available_button *btn = &available_buttons[selected_item];
                            if (col == 2) /* BUTTON*/
                            {
                                if (btn->value == BUTTON_NONE)
                                    bm->button_code = btn->value;
                                else
                                    bm->button_code |= btn->value;
                            }
                            else if (col == 3) /* PREBTN */
                            {
                                if (btn->value == BUTTON_NONE)
                                    bm->pre_button_code = btn->value;
                                else
                                    bm->pre_button_code |= btn->value;
                            }
                            break;
                        }
                    }
                }
                synclist_set(lastm->index, item-1, lastm->items, 1);
                rb->gui_synclist_draw(lists);
            }
    }
    return PLUGIN_OK;
}

static void cleanup(void *parameter)
{
    (void)parameter;
    keyremap_save_user_keys(false);
}

int menu_action_cb(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
    int status = PLUGIN_OK;
    /* Top Level Menu Actions */
    if (lists->data == MENU_ID(M_ROOT))
    {
        status = menu_action_root(action, selected_item, exit, lists);
    }
    else if (lists->data == MENU_ID(M_SETKEYS))
    {
        status = menu_action_setkeys(action, selected_item, exit, lists);
    }
    else if (lists->data == MENU_ID(M_TESTKEYS))
    {
        status = menu_action_testkeys(action, selected_item, exit, lists);
    }
    else if (lists->data == MENU_ID(M_DELKEYS))
    {
        status = menu_action_listfiles(action, selected_item, exit, lists);
    }
    else if (lists->data == MENU_ID(M_LOADKEYS))
    {
        status = menu_action_listfiles(action, selected_item, exit, lists);
    }

    if (status == GOTO_ACTION_DEFAULT_HANDLER)
        goto default_handler;
    /* Submenu Actions */
    menu_action_submenus(action, selected_item, exit, lists);

    /* Global cancel */
    if (*action == ACTION_STD_CANCEL)
    {
        logf("CANCEL BUTTON");
        if (lists->data == MENU_ID(M_TESTKEYS))
        {
            keytest.keymap = NULL;
            keytest.context = -1;
        }

        if (lists->data != MENU_ID(M_ROOT))
        {
            int id, item;
            POP_MENU(id, item);
            POP_MENU(id, item);
            const struct mainmenu *lastm = &mainmenu[id];
            synclist_set(lastm->index, item-1, lastm->items, 1);
            rb->gui_synclist_draw(lists);
        }
        else
        {
            /* save changes? */
            if (ctx_data.act_count + ctx_data.ctx_count >= 2)
            {
                int entries = 0;
                struct button_mapping *keymap = keyremap_create_temp(&entries);
                if (keymap != NULL && keyset.crc32 != rb->crc_32(keymap,
                    entries * sizeof(struct button_mapping), 0xFFFFFFFF))
                {
                    if (rb->yesno_pop("Save Keymap?") == true)
                    {
                        keyremap_save_user_keys(true);
                        goto default_handler;
                    }
                }
            }
            *exit = true;
        }
    }
default_handler:
    if (rb->default_event_handler_ex(*action, cleanup, NULL) == SYS_USB_CONNECTED)
    {
        *exit = true;
        return PLUGIN_USB_CONNECTED;
    }
    return PLUGIN_OK;
}

static void synclist_set(int id, int selected_item, int items, int sel_size)
{
    void* menu_id = MENU_ID(id);
    static char menu_title[64]; /* title is used by every menu */
    PUSH_MENU(id, selected_item);

    if (items <= 0)
        return;
    if (selected_item < 0)
        selected_item = 0;

    list_voice_cb(0, menu_id);
    rb->gui_synclist_init(&lists,list_get_name_cb,
                          menu_id, false, sel_size, NULL);

    rb->gui_synclist_set_icon_callback(&lists,NULL);
    rb->gui_synclist_set_voice_callback(&lists, list_voice_cb);
    rb->gui_synclist_set_nb_items(&lists,items);
    rb->gui_synclist_select_item(&lists, selected_item);
    printcell_enable(&lists, false, false);

    if (menu_id == MENU_ID(M_ROOT))
    {
        lang_strlcpy(menu_title, mainmenu[M_ROOT].name, sizeof(menu_title));
        rb->gui_synclist_set_title(&lists, menu_title, Icon_Plugin);
    }
    else if (menu_id == MENU_ID(M_SETKEYS))
    {
        printcell_enable(&lists, true, true);
        keyset.view_columns = printcell_set_columns(&lists, ACTVIEW_HEADER, Icon_Rockbox);
        int curcol = printcell_increment_column(&lists, 0, true);
        if (keyset.view_lastcol >= keyset.view_columns)
            keyset.view_lastcol = -1;
        /* restore column position */
        while (keyset.view_lastcol > -1 && curcol != keyset.view_lastcol)
        {
            curcol = printcell_increment_column(&lists, 1, true);
        }
        keyset.view_lastcol = curcol;
    }
    else
    {
        int id;
        PEEK_MENU_ID(id);
        lang_strlcpy(menu_title, mainitem(id)->name, sizeof(menu_title));
        rb->gui_synclist_set_title(&lists, menu_title, Icon_Submenu_Entered);
        /* if (menu_title[0] == '$'){ printcell_enable(&lists, true, true); } */
    }

}

static void keyremap_set_buffer(void* buffer, size_t buf_size)
{
    /* set up the keyremap action buffer
     * contexts start at the front and work towards the back
     * actions start at the back and work towards front
     * if they meet buffer is out of space (checked by ctx & btn add entry fns)
     */
    keyremap_buffer.buffer = buffer;
    keyremap_buffer.buf_size = buf_size;
    keyremap_reset_buffer();
}

enum plugin_status plugin_start(const void* parameter)
{
    int ret = PLUGIN_OK;
    int selected_item = -1;
    int action;
    bool redraw = true;
    bool exit = false;
    if (parameter)
    {
        //
    }

    size_t buf_size;
    void* buffer = rb->plugin_get_buffer(&buf_size);
    keyremap_set_buffer(buffer, buf_size);

    mainmenu[M_BUTTONS].items = available_button_count;
    /* add back item to each submenu */
    for (int i = 1; i < M_LAST_ITEM; i++)
        mainmenu[i].items += 1;

#if 0
    keymap_add_context_entry(CONTEXT_WPS);
    keymap_add_button_entry(CONTEXT_WPS, ACTION_WPS_PLAY, BUTTON_VOL_UP, BUTTON_NONE);
    keymap_add_button_entry(CONTEXT_WPS, ACTION_WPS_STOP, BUTTON_VOL_DOWN | BUTTON_REPEAT, BUTTON_NONE);

    keymap_add_context_entry(CONTEXT_MAINMENU);
    keymap_add_button_entry(CONTEXT_MAINMENU, ACTION_STD_PREV, BUTTON_VOL_UP, BUTTON_NONE);
    keymap_add_button_entry(CONTEXT_MAINMENU, ACTION_STD_NEXT, BUTTON_VOL_DOWN, BUTTON_NONE);

    keymap_add_context_entry(CONTEXT_STD);
    keymap_add_button_entry(CONTEXT_STD, ACTION_STD_OK, BUTTON_VOL_UP, BUTTON_NONE);
    keymap_add_button_entry(CONTEXT_STD, ACTION_STD_OK, BUTTON_VOL_DOWN, BUTTON_NONE);
#endif

    keyset.crc32 = 0;
    keyset.view_lastcol = -1;
    if (!exit)
    {
        const struct mainmenu *mainm = &mainmenu[M_ROOT];
        synclist_set(mainm->index, 0, mainm->items, 1);
        rb->gui_synclist_draw(&lists);

        while (!exit)
        {
            action = rb->get_action(CONTEXT_LIST, HZ / 10);

            if (action == ACTION_NONE)
            {
                if (redraw)
                {
                    action = ACTION_REDRAW;
                    redraw = false;
                }
            }
            else
                redraw = true;

            ret = menu_action_cb(&action, selected_item, &exit, &lists);
            if (rb->gui_synclist_do_button(&lists, &action))
                continue;
            selected_item = rb->gui_synclist_get_sel_pos(&lists);

            mainmenu[M_SETKEYS].items = ctx_data.ctx_count + ctx_data.act_count + 2;
        }
    }
    rb->closedir(kmffiles_dirp);

    return ret;
}



#if 0 /* Alt Example */
static int write_keyremap(const char*filename, struct button_mapping *remap_data, int count)
{
    size_t res = 0;
    if (!filename || !remap_data || count <= 0)
        goto fail;
    int fd = rb->open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        goto fail;
    if (keyremap_write_header(fd, count + 1) > 0)
    {
        res = keyremap_write_entries(fd, remap_data, count);
        res += sizeof(struct button_mapping); /*for header */
    }
fail:
    rb->close(fd);
    return res;
}

void alt_example(void)
{
    struct button_mapping kmap[8];

    kmap[0].action_code = CORE_CONTEXT_REMAP(CONTEXT_WPS);
    kmap[0].button_code = 3; /*OFFSET*/
    kmap[0].pre_button_code = 1; /*COUNT*/

    kmap[1].action_code = CORE_CONTEXT_REMAP(CONTEXT_MAINMENU);
    kmap[1].button_code = 5; /*OFFSET*/
    kmap[1].pre_button_code = 2; /*COUNT*/

    kmap[2].action_code = CONTEXT_STOPSEARCHING;
    kmap[2].button_code = BUTTON_NONE;
    kmap[2].pre_button_code = BUTTON_NONE;

    kmap[3].action_code = ACTION_WPS_PLAY;
    kmap[3].button_code = BUTTON_VOL_UP;
    kmap[3].pre_button_code = BUTTON_NONE;

    kmap[4].action_code = CONTEXT_STOPSEARCHING;
    kmap[4].button_code = BUTTON_NONE;
    kmap[4].pre_button_code = BUTTON_NONE;

    kmap[5].action_code = ACTION_STD_NEXT;
    kmap[5].button_code = BUTTON_VOL_DOWN;
    kmap[5].pre_button_code = BUTTON_NONE;

    kmap[6].action_code = ACTION_STD_PREV;
    kmap[6].button_code = BUTTON_VOL_UP;
    kmap[6].pre_button_code = BUTTON_NONE;

    kmap[7].action_code = CONTEXT_STOPSEARCHING;
    kmap[7].button_code = BUTTON_NONE;
    kmap[7].pre_button_code = BUTTON_NONE;

    write_keyremap(CORE_KEYREMAP_FILE, kmap, 8);

    return ret;
}
#endif
