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
    int children_count;
    int depth;

    struct folder* previous;
};

static char *buffer_front, *buffer_end;
static char* folder_alloc(size_t size)
{
    char* retval;
    /* 32-bit aligned */
    size = (size + 3) & ~3;
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

static void get_full_path_r(struct folder *start, char* dst)
{
    if (start->previous)
        get_full_path_r(start->previous, dst);

    if (start->name && start->name[0] && strcmp(start->name, "/"))
    {
        strlcat(dst, "/", MAX_PATH);
        strlcat(dst, start->name, MAX_PATH);
    }
}

static char* get_full_path(struct folder *start)
{
    static char buffer[MAX_PATH];

    if (strcmp(start->name, "/"))
    {
        buffer[0] = 0;
        get_full_path_r(start, buffer);
    }
    else /* get_full_path_r() does the wrong thing for / */
        return "/";

    return buffer;
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
    char* path = get_full_path(parent);
    char fullpath[MAX_PATH];
    struct dirent *entry;
    struct folder* this = (struct folder*)folder_alloc(sizeof(struct folder));
    int child_count = 0;
    char *first_child = NULL;

    if (!strcmp(folder,"/"))
        strlcpy(fullpath, folder, 2);
    else
        snprintf(fullpath, MAX_PATH, "%s/%s", parent ? path : "", folder);

    if (!this)
        return NULL;
    dir = opendir(fullpath);
    if (!dir)
        return NULL;
    this->previous = parent;
    this->name = folder;
    this->children = NULL;
    this->children_count = 0;
    this->depth = parent ? parent->depth + 1 : 0;

    while ((entry = readdir(dir))) {
        int len = strlen((char *)entry->d_name);
        struct dirinfo info;

        info = dir_get_info(dir, entry);

        /* skip anything not a directory */
        if ((info.attribute & ATTR_DIRECTORY) == 0) {
            continue;
        }
        /* skip directories . and .. */
        if ((!strcmp((char *)entry->d_name, ".")) ||
            (!strcmp((char *)entry->d_name, ".."))) {
            continue;
        }
        char *name = folder_alloc_from_end(len+1);
        if (!name)
            return NULL;
        memcpy(name, (char *)entry->d_name, len+1);
        child_count++;
        first_child = name;
    }
    closedir(dir);
    /* now put the names in the array */
    this->children = (struct child*)folder_alloc(sizeof(struct child) * child_count);

    if (!this->children)
        return NULL;
    while (child_count)
    {
        this->children[this->children_count].name = first_child;
        this->children[this->children_count].folder = NULL;
        this->children[this->children_count].state = COLLAPSED;
        this->children_count++;
        first_child += strlen(first_child) + 1;
        child_count--;
    }
    qsort(this->children, this->children_count, sizeof(struct child), compare);

    return this;
}

struct folder* load_root(void)
{
    static struct child root_child;

    root_child.name = "/";
    root_child.folder = NULL;
    root_child.state = COLLAPSED;

    static struct folder root = {
        .name = "",
        .children = &root_child,
        .children_count = 1,
        .depth = -1,
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

    buffer[0] = '\0';

    if (parent->depth >= 0)
        for(int i = 0; i <= parent->depth; i++)
            strcat(buffer, "\t");

    strlcat(buffer, this->name, buffer_len);

    if (this->state == EACCESS)
    {   /* append error message to the entry if unaccessible */
        size_t len = strlcat(buffer, " (", buffer_len);
        if (buffer_len > len)
        {
            snprintf(&buffer[len], buffer_len - len, str(LANG_READ_FAILED),
                        this->name);
            strlcat(buffer, ")", buffer_len);
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
                if (this->folder == NULL)
                    this->folder = load_folder(parent, this->name);
                this->state = this->folder ? (this->folder->children_count == 0 ?
                        SELECTED : EXPANDED) : EACCESS;
                break;
            case EACCESS:
                /* cannot open, do nothing */
                return action;
        }
        list->nb_items = count_items(root);
        return ACTION_REDRAW;
    }
    else if (action == ACTION_STD_CONTEXT)
    {
        switch (this->state)
        {
            case EXPANDED:
                for (i = 0; i < this->folder->children_count; i++)
                {
                    child = &this->folder->children[i];
                    if (child->state == SELECTED ||
                        child->state == EXPANDED)
                        child->state = COLLAPSED;
                    else if (child->state == COLLAPSED)
                        child->state = SELECTED;
                }
                break;
            case SELECTED:
            case COLLAPSED:
                if (this->folder == NULL)
                    this->folder = load_folder(parent, this->name);
                this->state = this->folder ? (this->folder->children_count == 0 ?
                        SELECTED : EXPANDED) : EACCESS;
                if (this->state == EACCESS)
                    break;
                for (i = 0; i < this->folder->children_count; i++)
                {
                    child = &this->folder->children[i];
                    child->state = SELECTED;
                }
                break;
            case EACCESS:
                /* cannot open, do nothing */
                return action;
        }
        list->nb_items = count_items(root);
        return ACTION_REDRAW;
    }
            
        
    return action;
}

static struct child* find_from_filename(char* filename, struct folder *root)
{
    char *slash = strchr(filename, '/');
    int i = 0;
    if (slash)
        *slash = '\0';
    if (!root)
        return NULL;

    struct child *this;

    /* filenames beginning with a / are specially treated as the
     * loop below can't handle them. they can only occur on the first,
     * and not recursive, calls to this function.*/
    if (slash == filename)
    {
        /* filename begins with /. in this case root must be the
         * top level folder */
        this = &root->children[0];
        if (!slash[1])
        {   /* filename == "/" */
            return this;
        }
        else
        {
            /* filename == "/XXX/YYY". cascade down */
            if (!this->folder)
                this->folder = load_folder(root, this->name);
            this->state = EXPANDED;
            /* recurse with XXX/YYY */
            return find_from_filename(slash+1, this->folder);
        }
    }

    while (i < root->children_count)
    {
        this = &root->children[i];
        if (!strcasecmp(this->name, filename))
        {
            if (!slash)
            {   /* filename == XXX */
                return this;
            }
            else
            {
                /* filename == XXX/YYY. cascade down */
                if (!this->folder)
                    this->folder = load_folder(root, this->name);
                this->state = EXPANDED;
                return find_from_filename(slash+1, this->folder);
            }
        }
        i++;
    }
    return NULL;
}

/* _modifies_ buf */
int select_paths(struct folder* root, char* buf)
{
    struct child *item = find_from_filename(buf, root);
    if (item)
        item->state = SELECTED;
    return 0;
}

static void save_folders_r(struct folder *root, char* dst, size_t maxlen)
{
    int i = 0;

    while (i < root->children_count)
    {
        struct child *this = &root->children[i];
        if (this->state == SELECTED)
        {
            if (this->folder)
                snprintf(buffer_front, buffer_end - buffer_front,
                        "%s:", get_full_path(this->folder));
            else
            {
                char *p = get_full_path(root);
                snprintf(buffer_front, buffer_end - buffer_front,
                        "%s/%s:", strcmp(p, "/") ? p : "",
                                  strcmp(this->name, "/") ? this->name : "");
            }
            strlcat(dst, buffer_front, maxlen);
        }
        else if (this->state == EXPANDED)
            save_folders_r(this->folder, dst, maxlen);
        i++;
    }
}

static void save_folders(struct folder *root, char* dst, size_t maxlen)
{
    int len;
    dst[0] = '\0';
    save_folders_r(root, dst, maxlen);
    len = strlen(dst);
    /* fix trailing ':' */
    if (len > 1) dst[len-1] = '\0';
}

bool folder_select(char* setting, int setting_len)
{
    struct folder *root;
    struct simplelist_info info;
    size_t buf_size;
    /* 32 separate folders should be Enough For Everybody(TM) */
    char *vect[32];
    char copy[setting_len];
    int nb_items;

    /* copy onto stack as split_string() modifies it */
    strlcpy(copy, setting, setting_len);
    nb_items = split_string(copy, ':', vect, ARRAYLEN(vect));

    buffer_front = plugin_get_buffer(&buf_size);
    buffer_end = buffer_front + buf_size;
    root = load_root();

    if (nb_items > 0)
    {
        for(int i = 0; i < nb_items; i++)
            select_paths(root, vect[i]);
    }

    simplelist_info_init(&info, str(LANG_SELECT_FOLDER),
            count_items(root), root);
    info.get_name = folder_get_name;
    info.action_callback = folder_action_callback;
    info.get_icon = folder_get_icon;
    simplelist_show_list(&info);

    /* done editing. check for changes */
    save_folders(root, copy, setting_len);
    if (strcmp(copy, setting))
    {   /* prompt for saving changes and commit if yes */
        if (yesno_pop(ID2P(LANG_SAVE_CHANGES)))
        {
            strcpy(setting, copy);
            settings_save();
            return true;
        }
    }
    return false;
}
