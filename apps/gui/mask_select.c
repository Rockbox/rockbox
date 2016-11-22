/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Copyright (C) 2016 William Wilgus
 * Derivative of folder_select.c by:
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
/* TODO add language defines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lang.h"
#include "language.h"
#include "list.h"
#include "plugin.h"
#include "mask_select.h"
#include "talk.h"

/*
 * Order for changing child states:
 * 1) expand folder
 * 2) collapse and select
 * 3) deselect (skip to 1)
 */

enum child_state {
    EXPANDED,
    SELECTED,
    COLLAPSED,
    DESELECTED,
};

/* Children of main categories */
struct child {
    const char* name;
    struct category *category;
    enum child_state state;
    int key_value;
};

/* Main Categories in root */
struct category {
    const char *name;
    struct child *children;
    int children_count;
    int depth;
    struct category* previous;
    int key_value; /*values of all children OR|D*/
};

/* empty category for children of root only one level needed */
static struct category empty = {
     .name           = "",
     .children       = NULL,
     .children_count = 0,
     .depth          = 1,
     .previous       = NULL,
};

/* Or | all keyvalues that user selected */
static int calculate_mask_r(struct category *root, int mask)
{
    int i = 0;
    while (i < root->children_count)
    {
        struct child *this = &root->children[i];
        if (this->state == SELECTED)
            mask |= this->key_value;

        else if (this->state == EXPANDED)
            mask = calculate_mask_r(this->category, mask);
        i++;
    }
return mask;
}

static int count_items(struct category *start)
{
    int count = 0;
    int i;

    for (i=0; i<start->children_count; i++)
    {
        struct child *foo = &start->children[i];
        if (foo->state == EXPANDED)
            count += count_items(foo->category);
        count++;
    }
    return count;
}

static struct child* find_index(struct category *start,
                                int index,struct category **parent)
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
            struct child *bar = find_index(foo->category, index - i, parent);
            if (bar)
            {
                return bar;
            }
            index -= count_items(foo->category);
        }
    }
    return NULL;
}

/* simplelist uses this callback to change
    the states of the categories/children */
static int item_action_callback(int action, struct gui_synclist *list)
{
    struct category *root = (struct category*)list->data;
    struct category *parent;
    struct child *this = find_index(root, list->selected_item, &parent);

    if (action == ACTION_STD_OK)
    {
        switch (this->state)
        {
            case EXPANDED:
                this->state = SELECTED;
                if (global_settings.talk_menu)
                    talk_id(LANG_ON, false);
                break;
            case SELECTED:
                this->state = this->category->children_count == 0 ?
                        DESELECTED : COLLAPSED;
                if (global_settings.talk_menu && this->category->children_count == 0)
                    talk_id(LANG_OFF, false);
                break;
            case COLLAPSED:
                if (this->category == NULL)
                    this->category = root;
                this->state = this->category->children_count == 0 ?
                        SELECTED : EXPANDED;
                if (global_settings.talk_menu && this->category->children_count == 0)
                    talk_id(LANG_ON, false);
                break;
            case DESELECTED:
                this->state = SELECTED;
                if (global_settings.talk_menu)
                    talk_id(LANG_ON, false);
                break;

            default:
                /* do nothing */
                return action;
        }
        list->nb_items = count_items(root);
        return ACTION_REDRAW;
    }

    return action;
}

static const char * item_get_name(int selected_item, void * data,
                                   char * buffer, size_t buffer_len)
{
    struct category *root = (struct category*)data;
    struct category *parent;
    struct child *this = find_index(root, selected_item , &parent);

    buffer[0] = '\0';

    if (parent->depth >= 0)
        for(int i = 0; i <= parent->depth; i++)
            strcat(buffer, "\t\0");

    /* state of selection needs icons so if icons are disabled use text*/
    if (!global_settings.show_icons)
        {
            if (this->state == SELECTED)
                strcat(buffer, "+\0");
            else
                strcat(buffer,"  \0");
        }
    strlcat(buffer, P2STR((const unsigned char *)this->name), buffer_len);

    return buffer;
}

static int item_get_talk(int selected_item, void *data)
{
    struct category *root = (struct category*)data;
    struct category *parent;
    struct child *this = find_index(root, selected_item , &parent);
    if (global_settings.talk_menu)
        {
            long id = P2ID((const unsigned char *)(this->name));
            if(id>=0)
                talk_id(id, true);
            else
                talk_spell(this->name, true);
            talk_id(VOICE_PAUSE,true);
            if (this->state == SELECTED)
                talk_id(LANG_ON, true);
            else if (this->state == DESELECTED)
                talk_id(LANG_OFF, true);
        }
    return 0;
}

static enum themable_icons item_get_icon(int selected_item, void * data)
{
    struct category *root = (struct category*)data;
    struct category *parent;
    struct child *this = find_index(root, selected_item, &parent);

    switch (this->state)
    {
        case SELECTED:
            return Icon_Submenu;
        case COLLAPSED:
            return Icon_NOICON;
        case EXPANDED:
            return Icon_Submenu_Entered;
        default:
            return Icon_NOICON;
    }
    return Icon_NOICON;
}

/* supply your original mask,the page header (ie. User do this..), mask items
    and count, they will be selected automatically if the mask includes
    them. If user selects new items and chooses to save settings
    new mask returned otherwise, original is returned
*/
int mask_select(int mask, const unsigned char* headermsg,
                             struct s_mask_items *mask_items, size_t items)
{
    struct simplelist_info info;

    struct child action_child[items];
            for (unsigned i = 0; i < items; i++)
            {
                action_child[i].name      = mask_items[i].name;
                action_child[i].category  = &empty;
                action_child[i].key_value = mask_items[i].bit_value;
                action_child[i].state     = mask_items[i].bit_value & mask ?
                                                SELECTED : DESELECTED;
            }

   struct category root = {
        .name           = "",
        .children       = (struct child*) &action_child,
        .children_count = items,
        .depth          = -1,
        .previous       = NULL,
    };

    simplelist_info_init(&info, P2STR(headermsg), count_items(&root), &root);
    info.get_name        = item_get_name;
    info.action_callback = item_action_callback;
    info.get_icon        = item_get_icon;
    info.get_talk        = item_get_talk;
    simplelist_show_list(&info);

    return calculate_mask_r(&root,0);
}
