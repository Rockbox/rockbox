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
    char* name;
    struct category *category;
    enum child_state state;
    int key_value;
};


/* Main Categories in root */
struct category {
    char *name;
    struct child *children;
    int children_count;
    int depth;
    struct category* previous;
    int key_value; /*values of all children OR|D*/
};


/* empty category for children of root only one level needed */
static struct category empty = {
     .name = "",
     .children = NULL,
     .children_count = 0,
     .depth = 1,
     .previous = NULL,
};

/* Or | all keyvalues that user selected */
static int calculate_actmask_r(struct category *root, int actmask)
{
    int i = 0;
    while (i < root->children_count)
    {
        struct child *this = &root->children[i];
        if (this->state == SELECTED)
            actmask |= this->key_value;

        else if (this->state == EXPANDED)
            actmask = calculate_actmask_r(this->category, actmask);
        i++;
    }
return actmask;
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
                break;
            case SELECTED:
                this->state = this->category->children_count == 0 ?
                        DESELECTED : COLLAPSED;
                break;
            case COLLAPSED:
                if (this->category == NULL)
                    this->category = root;
                this->state = this->category->children_count == 0 ?
                        SELECTED : EXPANDED;
                break;
            case DESELECTED:
                this->state = SELECTED;
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
            strcat(buffer, "\t");

    strlcat(buffer, this->name, buffer_len);

    return buffer;
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


/* supply your original action mask and the page header ie. User do this..
    if user selects new actions and chooses to save settings
    new action mask returned otherwise, original is returned
*/
int action_select(int actmask, char* headermsg)
{
    struct simplelist_info info;
    int actmask_orig=actmask;
   
    static struct child action_child[4];
            action_child[0].name = "Volume + / -";
            action_child[0].category = &empty;
            action_child[0].key_value=SEL_ACTION_VOL;
            action_child[0].state = SEL_ACTION_VOL & actmask ?
                                                SELECTED : DESELECTED;
            action_child[1].name = "Play";
            action_child[1].category = &empty;
            action_child[1].key_value=SEL_ACTION_PLAY;
            action_child[1].state = SEL_ACTION_PLAY & actmask ?
                                                SELECTED : DESELECTED;
            action_child[2].name = "Seek";
            action_child[2].category = &empty;
            action_child[2].key_value=SEL_ACTION_SEEK;
            action_child[2].state = SEL_ACTION_SEEK & actmask ?
                                                SELECTED : DESELECTED;
            action_child[3].name = "Skip";
            action_child[3].category = &empty;
            action_child[3].key_value=SEL_ACTION_SKIP;
            action_child[3].state = SEL_ACTION_SKIP & actmask ?
                                                SELECTED : DESELECTED;

   static struct category root = {
        .name = "",
        .children = (struct child*) &action_child,
        .children_count = ARRAYLEN(action_child),
        .depth = -1,
        .previous = NULL,
    };

    simplelist_info_init(&info, headermsg,
                count_items(&root), &root);
    info.get_name = item_get_name;
    info.action_callback = item_action_callback;
    info.get_icon = item_get_icon;
    simplelist_show_list(&info);

    /* done selecting, check for changes */
    actmask=calculate_actmask_r(&root,0);

    if (actmask_orig != actmask)
    {   /* prompt for saving changes and return new mask if yes */
        if (yesno_pop(ID2P(LANG_SAVE_CHANGES)))
            return actmask;
    }

    return actmask_orig;
}
