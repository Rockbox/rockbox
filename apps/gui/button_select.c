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
/* TODO Find better way to accumulate button ID and names */
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


/* these button names were taken from pluginlib_actions.h which was from the bubbles plugin, so may need tweaking */
static const struct
{
    int btn;
    char* btntext;
}button_direction_map[] =
{
    /*Standard Buttons */
#ifdef BUTTON_UP
    {  BUTTON_UP, "Up"},
#endif

#ifdef BUTTON_DOWN
    {  BUTTON_DOWN, "Down"},
#endif

#ifdef BUTTON_LEFT
    {  BUTTON_LEFT, "Left"},
#endif

#ifdef BUTTON_RIGHT
    {  BUTTON_RIGHT, "Right"},
#endif

    /* Touchscreens */
#ifdef HAVE_TOUCHSCREEN
    {  BUTTON_BOTTOMRIGHT, "Bottom Right"},
    {  BUTTON_CENTER, "Center"},
    {  BUTTON_TOPMIDDLE, "Top Middle"},
    {  BUTTON_BOTTOMMIDDLE, "Bottom Middle"},
    {  BUTTON_MIDLEFT, "Middle Left"},
    {  BUTTON_MIDRIGHT, "Middle Right"},
#endif

    /* now the bad ones that don't have standard names for the buttons */
#if (CONFIG_KEYPAD == IRIVER_H10_PAD)
    {  BUTTON_SCROLL_UP, "Scroll Up"},
    {  BUTTON_SCROLL_DOWN, "Scroll Down"},
#endif
    {0, "Directions"} /* Category name */
};


static const struct
{
    int btn;
    char* btntext;
}button_action_map[] =
{
    /* Scrollwheels */
#ifdef HAVE_SCROLLWHEEL
    {  BUTTON_SCROLL_BACK, "Scroll Back"},
    {  BUTTON_SCROLL_FWD, "Scroll Fwd"},
#endif

    /*Standard Buttons */
#ifdef BUTTON_HOME
    {  BUTTON_HOME, "Home"},
#endif

#ifdef BUTTON_VOL_UP
    {  BUTTON_VOL_UP, "Vol +"},
#endif

#ifdef BUTTON_VOL_DOWN
    {  BUTTON_VOL_DOWN, "Vol -"},
#endif

#ifdef BUTTON_SELECT
    {  BUTTON_SELECT, "Select"},
#endif

#ifdef BUTTON_POWER
    {  BUTTON_POWER, "Pwr"},
#endif

#ifdef BUTTON_MENU
    {  BUTTON_MENU, "MENU"},
#endif

#ifdef BUTTON_PLAY
    {  BUTTON_PLAY, "Play"},
#endif

#ifdef BUTTON_STOP
    {  BUTTON_STOP, "Stop"},
#endif

#ifdef BUTTON_REW
    {  BUTTON_REW, "Rew"},
#endif

#ifdef BUTTON_FF
    {  BUTTON_FF, "FFwd"},
#endif

#ifdef BUTTON_REC
    {  BUTTON_REC, "Rec"},
#endif

#ifdef BUTTON_ON
    {  BUTTON_ON, "On"},
#endif

#ifdef BUTTON_OFF
    {  BUTTON_OFF, "Off"},
#endif

#ifdef BUTTON_BACK
    {  BUTTON_BACK, "Back"},
#endif

    /* now the bad ones that don't have standard names for the buttons */
#if (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD) \
    || (CONFIG_KEYPAD == PHILIPS_SA9200_PAD) \
    || (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
    {  BUTTON_PREV, "Prev"},
    {  BUTTON_NEXT, "Next"},

#elif (CONFIG_KEYPAD == RK27XX_GENERIC_PAD)
    {  BUTTON_M, "Menu"},
    {  BUTTON_REW|BUTTON_M, "Left"},
    {  BUTTON_FF|BUTTON_M, "Right"},

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    {  BUTTON_RC_STOP, "Stop"},
    {  BUTTON_RC_MENU, "Menu"},
    {  BUTTON_RC_ON, "On"},
    {  BUTTON_RC_BITRATE, "Bitrate"},
    {  BUTTON_RC_SOURCE, "Source"},
    {  BUTTON_RC_VOL_DOWN, "Vol -"},
    {  BUTTON_RC_VOL_UP, "Vol +"},

#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
   || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {  BUTTON_MENU|BUTTON_SELECT, "Menu"},
    {  BUTTON_PLAY|BUTTON_SELECT, "Play"},

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
    {  BUTTON_RC_MODE, "Mode"},

#elif (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
    {  BUTTON_EQ, "Eq"},

#elif (CONFIG_KEYPAD == MROBE500_PAD)
    {  BUTTON_RC_HEART, "Heart"},

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
    {  BUTTON_MINUS}, "Minus"

#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
    {  BUTTON_RC_REC, "Rec"},
    {  BUTTON_RC_MODE, "Mode"},

#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
    {  BUTTON_OK, "Ok"},

#elif (CONFIG_KEYPAD == MPIO_HD200_PAD)
    {  BUTTON_FUNC, "Func"},

#elif (CONFIG_KEYPAD == MPIO_HD300_PAD)
    {  BUTTON_ENTER, "Enter"},

#endif
    /*Remote Buttons*/
#if defined(HAVE_REMOTE_LCD)
#if (CONFIG_KEYPAD == IAUDIO_X5M5_PAD) || \
 (CONFIG_KEYPAD == IRIVER_H10_PAD) || \
 (CONFIG_KEYPAD == GIGABEAT_PAD) || \
 (CONFIG_KEYPAD == IAUDIO_M3_PAD) || \
 (CONFIG_KEYPAD == GIGABEAT_S_PAD)
    {  BUTTON_RC_FF, "FFwd"},
    {  BUTTON_RC_REW, "Rew"},
    {  BUTTON_RC_VOL_DOWN, "Vol -"},
    {  BUTTON_RC_VOL_UP, "Vol +"},
#elif (CONFIG_KEYPAD == PLAYER_PAD) || \
 (CONFIG_KEYPAD == RECORDER_PAD)
    {  BUTTON_RC_VOL_UP, "Vol +"},
    {  BUTTON_RC_VOL_DOWN, "Vol -"},
    {  BUTTON_RC_LEFT, "Left"},
    {  BUTTON_RC_RIGHT, "Right"},
#elif (CONFIG_REMOTE_KEYPAD == MROBE_REMOTE)
    {  BUTTON_RC_PLAY, "Play"},
    {  BUTTON_RC_DOWN, "Down"},
    {  BUTTON_RC_REW, "Rew"},
    {  BUTTON_RC_FF, "FFwd"},
#endif
#endif
    {0, "Actions"} /* Category name */
};


/* Or | all keyvalues that user selected */
static int calculate_btnmask_r(struct category *root, int btnmask)
{
    int i = 0;
    while (i < root->children_count)
    {
        struct child *this = &root->children[i];
        if (this->state == SELECTED)
            btnmask |= this->key_value;

        else if (this->state == EXPANDED)
            btnmask = calculate_btnmask_r(this->category, btnmask);
        i++;
    }
return btnmask;
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


static struct child* find_index(struct category *start, int index, struct category **parent)
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


/* simplelist uses this callback to change the states of the categories/children */
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


/* supply your original button mask and the page header ie. User do this..
    if user selects new buttons and chooses to save settings
    new button mask returned otherwise, original is returned
*/
int button_select(int btnmask, char* headermsg)
{
    struct simplelist_info info;
    int btnmask_orig=btnmask;

    unsigned int dir_count=ARRAYLEN(button_direction_map);
    int cat_value=0;
    struct child direction_child[dir_count];
    for (unsigned i = 0; i < dir_count; i++)
    {
        direction_child[i].name = button_direction_map[i].btntext;
        direction_child[i].category = &empty;
        direction_child[i].key_value=button_direction_map[i].btn;
        direction_child[i].state = direction_child[i].key_value & btnmask ?
                                                      SELECTED : DESELECTED;
        cat_value |= button_direction_map[i].btn;/*or| all the values under this category for select all*/
    }

    struct category direction = {
        .name = "",
        .children = (struct child*) &direction_child,
        .children_count = dir_count-1,
        .depth = 1,
        .previous = NULL,
        .key_value = cat_value,
    };

    unsigned int act_count=ARRAYLEN(button_action_map);
    cat_value=0;
    struct child action_child[act_count];
    for (unsigned i = 0; i < act_count; i++)
    {
        action_child[i].name = button_action_map[i].btntext;
        action_child[i].category = &empty;
        action_child[i].key_value=button_action_map[i].btn;
        action_child[i].state = action_child[i].key_value & btnmask ?
                                                SELECTED : DESELECTED;
        cat_value |= button_action_map[i].btn;
    }

    struct category action = {
        .name = "",
        .children = (struct child*) &action_child,
        .children_count = act_count-1,
        .depth = 1,
        .previous = NULL,
        .key_value = cat_value,
    };

    unsigned int i=0;
    if (dir_count > 1)
        i++;
    if (act_count > 1)
        i++;

    struct child root_child[i];
    i=0;
    if (dir_count > 1)
    {
        root_child[i].name = direction_child[dir_count-1].name; /* loaded from last item in array */
        root_child[i].category = &direction;
        root_child[i].state = EXPANDED;
        root_child[i].key_value = direction.key_value;
        i++;
    };

    if (act_count > 1)
    {
        root_child[i].name = action_child[act_count-1].name; /* loaded from last item in array */
        root_child[i].category = &action;
        root_child[i].state = EXPANDED;
        root_child[i].key_value = action.key_value;
        i++;    
    };
    struct category root = {
        .name = "",
        .children = (struct child*) &root_child,
        .children_count = i,
        .depth = -1,
        .previous = NULL,
    };

    simplelist_info_init(&info, headermsg,
                count_items(&root), &root);
    info.get_name = item_get_name;
    info.action_callback = item_action_callback;
    info.get_icon = item_get_icon;
    simplelist_show_list(&info);

    /* done selecting buttons. check for changes */
    btnmask=calculate_btnmask_r(&root,0);

    if (btnmask_orig != btnmask)
    {   /* prompt for saving changes and return new mask if yes */
        if (yesno_pop(ID2P(LANG_SAVE_CHANGES)))
            return btnmask;
    }

    return btnmask_orig;
}
