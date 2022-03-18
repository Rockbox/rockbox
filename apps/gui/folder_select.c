/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2012 Jonathan Gordon
 * Copyright (C) 2012 Thomas Martitz
* * Copyright (C) 2021 William Wilgus
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
#include <stdlib.h>
#include <string.h>
#include "inttypes.h"
#include "config.h"
#include "core_alloc.h"
#include "filetypes.h"
#include "lang.h"
#include "language.h"
#include "list.h"
#include "plugin.h"
#include "splash.h"

/* Define LOGF_ENABLE to enable logf output in this file */
//#define LOGF_ENABLE
#include "logf.h"

/*
 * Order for changing child states:
 * 1) expand folder (skip to 3 if empty, skip to 4 if cannot be opened)
 * 2) collapse and select
 * 3) unselect (skip to 1)
 * 4) do nothing
 */

enum child_state {
    EXPANDED,
    SELECTED,
    COLLAPSED,
    EACCESS,
};

struct child {
    char* name;
    struct folder *folder;
    enum child_state state;
};

struct folder {
    char *name;
    struct child *children;
    struct folder* previous;
    uint16_t children_count;
    uint16_t depth;
};

static char *buffer_front, *buffer_end;

static struct
{
    int32_t len; /* keeps count versus maxlen to give buffer full notification */
    uint32_t val; /* hash of all selected items */
    char buf[3];/* address used as identifier -- only \0 written to it */
    char maxlen_exceeded; /*0,1*/
} hashed;

static inline void get_hash(const char *key, uint32_t *hash, int len)
{
    *hash = crc_32(key, len, *hash);
}

static char* folder_alloc(size_t size)
{
    char* retval;
    /* 32-bit aligned */
    size = ALIGN_UP(size, 4);
    if (buffer_front + size > buffer_end)
    {
        return NULL;
    }
    retval = buffer_front;
    buffer_front += size;
    return retval;
}

static char* folder_alloc_from_end(size_t size)
{
    if (buffer_end - size < buffer_front)
    {
        return NULL;
    }
    buffer_end -= size;
    return buffer_end;
}
#if 0
/* returns the buffer size required to store the path + \0 */
static int get_full_pathsz(struct folder *start)
{
    int reql = 0;
    struct folder *next = start;
    do
    {
        reql += strlen(next->name) + 1;
    } while ((next = next->previous));

    if (start->name[0] != '/') reql--;
    if (--reql < 0) reql = 0;
    return reql;
}
#endif

static size_t get_full_path(struct folder *start, char *dst, size_t dst_sz)
{
    size_t pos = 0;
    struct folder *prev, *cur = NULL, *next = start;
    dst[0] = '\0'; /* for strlcat to do its thing */
    /* First traversal R->L mutate nodes->previous to point at child */
    while (next->previous != NULL) /* stop at the root */
    {
#define PATHMUTATE()              \
        ({                        \
            prev = cur;           \
            cur = next;           \
            next = cur->previous;\
            cur->previous = prev; \
        })
        PATHMUTATE();
    }
    /*swap the next and cur nodes to reverse direction */
    prev = next;
    next = cur;
    cur = prev;
    /* Second traversal L->R mutate nodes->previous to point back at parent
     * copy strings to buf as they go by */
    while (next != NULL)
    {
        PATHMUTATE();
        pos = strlcat(dst, cur->name, dst_sz);
        /* do not append slash to paths starting with slash */
        if (cur->name[0] != '/')
            pos = strlcat(dst, "/", dst_sz);
    }
    logf("get_full_path: (%d)[%s]", (int)pos, dst);
    return pos;
#undef PATHMUTATE
}

/* support function for qsort() */
static int compare(const void* p1, const void* p2)
{
    struct child *left = (struct child*)p1;
    struct child *right = (struct child*)p2;
    return strcasecmp(left->name, right->name);
}

static struct folder* load_folder(struct folder* parent, char *folder)
{
    DIR *dir;
    char fullpath[MAX_PATH];

    struct dirent *entry;
    int child_count = 0;
    char *first_child = NULL;
    size_t len = 0;

    struct folder* this = (struct folder*)folder_alloc(sizeof(struct folder));
    if (this == NULL)
        goto fail;

    if (parent)
    {
        len = get_full_path(parent, fullpath, sizeof(fullpath));
        if (len >= sizeof(fullpath))
            goto fail;
    }
    strlcpy(&fullpath[len], folder, sizeof(fullpath) - len);
    logf("load_folder: [%s]", fullpath);

    dir = opendir(fullpath);
    if (dir == NULL)
        goto fail;
    this->previous = parent;
    this->name = folder;
    this->children = NULL;
    this->children_count = 0;
    if (parent)
        this->depth = parent->depth + 1;

    while ((entry = readdir(dir))) {
        /* skip anything not a directory */
        if ((dir_get_info(dir, entry).attribute & ATTR_DIRECTORY) == 0) {
            continue;
        }
        /* skip . and .. */
        char *dn = entry->d_name;
        if ((dn[0] == '.') && (dn[1] == '\0' || (dn[1] == '.' && dn[2] == '\0')))
            continue;
        /* copy entry name to end of buffer, save pointer */
        int len = strlen((char *)entry->d_name);
        char *name = folder_alloc_from_end(len+1); /*for NULL*/
        if (name == NULL)
        {
            closedir(dir);
            goto fail;
        }
        memcpy(name, (char *)entry->d_name, len+1);
        child_count++;
        first_child = name;
    }
    closedir(dir);
    /* now put the names in the array */
    this->children = (struct child*)folder_alloc(sizeof(struct child) * child_count);

    if (this->children == NULL)
        goto fail;

    while (child_count)
    {
        struct child *child = &this->children[this->children_count++];
        child->name = first_child;
        child->folder = NULL;
        child->state = COLLAPSED;
        while(*first_child++ != '\0'){};/* move to next name entry */
        child_count--;
    }
    qsort(this->children, this->children_count, sizeof(struct child), compare);

    return this;
fail:
    return NULL;
}

struct folder* load_root(void)
{
    static struct child root_child;
    /* reset the root for each call */
    root_child.name = "/";
    root_child.folder = NULL;
    root_child.state = COLLAPSED;

    static struct folder root = {
        .name = "",
        .children = &root_child,
        .children_count = 1,
        .depth = 0,
        .previous = NULL,
    };

    return &root;
}

static int count_items(struct folder *start)
{
    int count = 0;
    int i;

    for (i=0; i<start->children_count; i++)
    {
        struct child *foo = &start->children[i];
        if (foo->state == EXPANDED)
            count += count_items(foo->folder);
        count++;
    }
    return count;
}

static struct child* find_index(struct folder *start, int index, struct folder **parent)
{
    int i = 0;
    *parent = NULL;

    while (i < start->children_count)
    {
        struct child *foo = &start->children[i];
        if (i == index)
        {
            *parent = start;
            return foo;
        }
        i++;
        if (foo->state == EXPANDED)
        {
            struct child *bar = find_index(foo->folder, index - i, parent);
            if (bar)
            {
                return bar;
            }
            index -= count_items(foo->folder);
        }
    }
    return NULL;
}

static const char * folder_get_name(int selected_item, void * data,
                                   char * buffer, size_t buffer_len)
{
    struct folder *root = (struct folder*)data;
    struct folder *parent;
    struct child *this = find_index(root, selected_item , &parent);

    char *buf = buffer;
    if ((int)buffer_len > parent->depth)
    {
        int i = parent->depth;
        while(--i > 0) /* don't indent the parent /folders */
            *buf++ = '\t';
    }
    *buf = '\0';
    strlcat(buffer, this->name, buffer_len);

    if (this->state == EACCESS)
    {   /* append error message to the entry if unaccessible */
        size_t len = strlcat(buffer, " ( ", buffer_len);
        if (buffer_len > len)
        {
            snprintf(&buffer[len], buffer_len - len, str(LANG_READ_FAILED), ")");
        }
    }

    return buffer;
}

static enum themable_icons folder_get_icon(int selected_item, void * data)
{
    struct folder *root = (struct folder*)data;
    struct folder *parent;
    struct child *this = find_index(root, selected_item, &parent);

    switch (this->state)
    {
        case SELECTED:
            return Icon_Cursor;
        case COLLAPSED:
            return Icon_Folder;
        case EXPANDED:
            return Icon_Submenu;
        case EACCESS:
            return Icon_Questionmark;
    }
    return Icon_NOICON;
}

static int child_set_state_expand(struct child *this, struct folder *parent)
{
    int newstate = EACCESS;
    if (this->folder == NULL)
        this->folder = load_folder(parent, this->name);

    if (this->folder != NULL)
    {
        if(this->folder->children_count == 0)
            newstate = SELECTED;
        else
            newstate = EXPANDED;
    }
    this->state = newstate;
    return newstate;
}

static int folder_action_callback(int action, struct gui_synclist *list)
{
    struct folder *root = (struct folder*)list->data;
    struct folder *parent;
    struct child *this = find_index(root, list->selected_item, &parent), *child;
    int i;

    if (action == ACTION_STD_OK)
    {
        switch (this->state)
        {
            case EXPANDED:
                this->state = SELECTED;
                break;
            case SELECTED:
                this->state = COLLAPSED;
                break;
            case COLLAPSED:
                child_set_state_expand(this, parent);
                break;
            case EACCESS:
                /* cannot open, do nothing */
                return action;
        }
        action = ACTION_REDRAW;
    }
    else if (action == ACTION_STD_CONTEXT)
    {
        switch (this->state)
        {
            case EXPANDED:
                for (i = 0; i < this->folder->children_count; i++)
                {
                    child = &this->folder->children[i];
                    switch (child->state)
                    {
                        case SELECTED:
                        case EXPANDED:
                            child->state = COLLAPSED;
                            break;
                        case COLLAPSED:
                            child->state = SELECTED;
                            break;
                        case EACCESS:
                            break;
                    }
                }
                break;
            case SELECTED:
            case COLLAPSED:
                if (child_set_state_expand(this, parent) != EACCESS)
                {
                    for (i = 0; i < (this->folder->children_count); i++)
                    {
                        child = &this->folder->children[i];
                        child->state = SELECTED;
                    }
                }
                break;
            case EACCESS:
                /* cannot open, do nothing */
                return action;
        }
        action = ACTION_REDRAW;
    }
    if (action == ACTION_REDRAW)
        list->nb_items = count_items(root);
    return action;
}

static struct child* find_from_filename(const char* filename, struct folder *root)
{
    if (!root)
        return NULL;
    const char *slash = strchr(filename, '/');
    struct child *this;

    /* filenames beginning with a / are specially treated as the
     * loop below can't handle them. they can only occur on the first,
     * and not recursive, calls to this function.*/
    if (filename[0] == '/') /* in the loop nothing starts with '/' */
    {
        logf("find_from_filename [%s]", filename);
        /* filename begins with /. in this case root must be the
         * top level folder */
        this = &root->children[0];
        if (filename[1] == '\0')
        {   /* filename == "/" */
            return this;
        }
        else /* filename == "/XXX/YYY". cascade down */
            goto cascade;
    }

    for (int i = 0; i < root->children_count; i++)
    {
        this = &root->children[i];
        /* when slash == NULL n will be really large but \0 stops the compare */
        if (strncasecmp(this->name, filename, slash - filename) == 0)
        {
            if (slash == NULL)
            {   /* filename == XXX */
                return this;
            }
            else
                goto cascade;
        }
    }
    return NULL;

cascade:
    /* filename == XXX/YYY. cascade down */
    child_set_state_expand(this, root);
    while (slash[0] == '/') slash++; /* eat slashes */
    return find_from_filename(slash, this->folder);
}

static int select_paths(struct folder* root, const char* filenames)
{
    /* Takes a list of filenames in a ':' delimited string
       splits filenames at the ':' character loads each into buffer
       selects each file in the folder list

       if last item or only item the rest of the string is copied to the buffer
       *End the last item WITHOUT the ':' character /.rockbox/eqs:/.rockbox/wps\0*
   */
    char buf[MAX_PATH];
    const int buflen = sizeof(buf);

    const char *fnp = filenames;
    const char *lastfnp = fnp;
    const char *sstr;
    off_t len;

    while (fnp)
    {
        fnp = strchr(fnp, ':');
        if (fnp)
        {
            len = fnp - lastfnp;
            fnp++;
        }
        else /* no ':' get the rest of the string */
            len = strlen(lastfnp);

        sstr = lastfnp;
        lastfnp = fnp;
        if (len <= 0 || len + 1 >= buflen)
            continue;
        strlcpy(buf, sstr, len + 1);
        struct child *item = find_from_filename(buf, root);
        if (item)
            item->state = SELECTED;
    }

    return 0;
}

static void save_folders_r(struct folder *root, char* dst, size_t maxlen, size_t buflen)
{
    size_t len;
    struct folder *curfolder;
    char* name;

    for (int i = 0; i < root->children_count; i++)
    {
        struct child *this = &root->children[i];
        if (this->state == SELECTED)
        {
            if (this->folder == NULL)
            {
                curfolder = root;
                name = this->name;
                logf("save_folders_r: this->name[%s]", name);
            }
            else
            {
                curfolder = this->folder->previous;
                name = this->folder->name;
                logf("save_folders_r: this->folder->name[%s]", name);
            }

            len = get_full_path(curfolder, buffer_front, buflen);

            if (len + 2 >= buflen)
                continue;

            len += snprintf(&buffer_front[len], buflen - len, "%s:", name);
            logf("save_folders_r: [%s]", buffer_front);
            if (dst != hashed.buf)
            {
                int dlen = strlen(dst);
                if (dlen + len >= maxlen)
                    continue;
                strlcpy(&dst[dlen], buffer_front, maxlen - dlen);
            }
            else
            {
                if (hashed.len + len >= maxlen)
                {
                    hashed.maxlen_exceeded = 1;
                    continue;
                }
                get_hash(buffer_front, &hashed.val, len);
                hashed.len += len;
            }
        }
        else if (this->state == EXPANDED)
            save_folders_r(this->folder, dst, maxlen, buflen);
    }
}

static uint32_t save_folders(struct folder *root, char* dst, size_t maxlen)
{
    hashed.len = 0;
    hashed.val = 0;
    hashed.maxlen_exceeded = 0;
    size_t len = buffer_end - buffer_front;
    dst[0] = '\0';
    save_folders_r(root, dst, maxlen, len);
    len = strlen(dst);
    /* fix trailing ':' */
    if (len > 1) dst[len-1] = '\0';
    /*Notify - user will probably not see save dialog if nothing new got added*/
    if (hashed.maxlen_exceeded > 0) splash(HZ *2, ID2P(LANG_SHOWDIR_BUFFER_FULL));
    return hashed.val;
}

bool folder_select(char* setting, int setting_len)
{
    struct folder *root;
    struct simplelist_info info;
    size_t buf_size;

    buffer_front = plugin_get_buffer(&buf_size);
    buffer_end = buffer_front + buf_size;
    logf("%d bytes free", (int)(buffer_end - buffer_front));
    root = load_root();

    logf("folders in: %s", setting);
    /* Load previous selection(s) */
    select_paths(root, setting);
    /* get current hash to check for changes later */
    uint32_t hash = save_folders(root, hashed.buf, setting_len);
    simplelist_info_init(&info, str(LANG_SELECT_FOLDER),
            count_items(root), root);
    info.get_name = folder_get_name;
    info.action_callback = folder_action_callback;
    info.get_icon = folder_get_icon;
    simplelist_show_list(&info);
    logf("%d bytes free", (int)(buffer_end - buffer_front));
    /* done editing. check for changes */
    if (hash != save_folders(root, hashed.buf, setting_len))
    {  /* prompt for saving changes and commit if yes */
        if (yesno_pop(ID2P(LANG_SAVE_CHANGES)))
        {
            save_folders(root, setting, setting_len);
            settings_save();
            logf("folders out: %s", setting);
            return true;
        }
    }
    return false;
}
