/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"


/*
    Button Code Definitions for iBasso DX50 & DX90.

    The format of the list is as follows

    { Action Code, Button code, Prereq button code }

    If there's no need to check the previous button's value, use BUTTON_NONE.
    CAVEAT: The action without prereq button code will allways be choosen if it has a
            lower index in the list.
    Insert LAST_ITEM_IN_LIST at the end of each mapping
*/


static const struct button_mapping button_context_standard[] =
{
    { ACTION_STD_CONTEXT,    BUTTON_PLAY | BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_STD_OK,         BUTTON_PLAY | BUTTON_REL,     BUTTON_PLAY },
    { ACTION_STD_PREV,       BUTTON_LEFT,                  BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_RIGHT,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_LEFT | BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_RIGHT | BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_CANCEL,     BUTTON_POWER,                 BUTTON_NONE },

    LAST_ITEM_IN_LIST
};


static const struct button_mapping button_context_wps[] =
{
    { ACTION_WPS_MENU,     BUTTON_POWER,                    BUTTON_NONE },
    { ACTION_WPS_CONTEXT,  BUTTON_PLAY | BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_PLAY,     BUTTON_PLAY | BUTTON_REL,        BUTTON_NONE },
    { ACTION_WPS_SEEKBACK, BUTTON_LEFT | BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,  BUTTON_RIGHT | BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_LEFT | BUTTON_REL,        BUTTON_LEFT | BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK, BUTTON_RIGHT | BUTTON_REL,       BUTTON_RIGHT | BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT, BUTTON_RIGHT | BUTTON_REL,       BUTTON_NONE },
    { ACTION_WPS_SKIPPREV, BUTTON_LEFT | BUTTON_REL,        BUTTON_NONE },
    { ACTION_WPS_VOLUP,    BUTTON_VOL_UP,                   BUTTON_NONE },
    { ACTION_WPS_VOLUP,    BUTTON_VOL_UP | BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,  BUTTON_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,  BUTTON_VOL_DOWN | BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST
};


static const struct button_mapping button_context_list[] =
{
    { ACTION_LIST_VOLUP,   BUTTON_VOL_UP | BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_LIST_VOLUP,   BUTTON_VOL_UP,                   BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOL_DOWN | BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
};


static const struct button_mapping button_context_tree[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
};


static const struct button_mapping button_context_listtree_scroll_with_combo[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM | CONTEXT_TREE),
};


static const struct button_mapping button_context_listtree_scroll_without_combo[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM | CONTEXT_TREE),
};


static const struct button_mapping button_context_settings[] =
{
    { ACTION_SETTINGS_INC,       BUTTON_VOL_UP | BUTTON_REL,      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_VOL_UP | BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_VOL_DOWN | BUTTON_REL,    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_VOL_DOWN | BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
};


static const struct button_mapping button_context_settings_right_is_inc[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
};


static const struct button_mapping button_context_mainmenu[] =
{
    { ACTION_TREE_WPS, BUTTON_POWER, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_TREE),
};


static const struct button_mapping button_context_yesno[] =
{
    { ACTION_YESNO_ACCEPT, BUTTON_PLAY, BUTTON_NONE },
    { ACTION_YESNO_ACCEPT, BUTTON_LEFT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
};


static const struct button_mapping button_context_colorchooser[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
};


static const struct button_mapping button_context_eq[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
};


static const struct button_mapping button_context_keyboard[] =
{
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                                BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT | BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                               BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT | BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_PLAY | BUTTON_LEFT,                  BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_PLAY | BUTTON_LEFT | BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_PLAY | BUTTON_RIGHT,                 BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_PLAY | BUTTON_RIGHT | BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_PLAY,                                BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_VOL_UP,                              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_VOL_UP | BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_VOL_DOWN,                            BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_VOL_DOWN | BUTTON_REPEAT,            BUTTON_NONE },

    LAST_ITEM_IN_LIST
};



static const struct button_mapping button_context_bmark[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
};


static const struct button_mapping button_context_time[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
};


static const struct button_mapping button_context_quickscreen[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};


static const struct button_mapping button_context_pitchscreen[] =
{
    { ACTION_PS_INC_SMALL, BUTTON_VOL_UP | BUTTON_REL,  BUTTON_NONE },
    { ACTION_PS_DEC_SMALL, BUTTON_VOL_DOWN |BUTTON_REL, BUTTON_NONE },
    { ACTION_PS_EXIT,      BUTTON_POWER,                BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};


static const struct button_mapping button_context_radio[] =
{
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
};


const struct button_mapping* target_get_context_mapping(int context)
{
    switch(context)
    {
        case CONTEXT_STD:                       { return button_context_standard; }
        case CONTEXT_WPS:                       { return button_context_wps; }
        case CONTEXT_LIST:                      { return button_context_list; }
        case CONTEXT_MAINMENU:                  { return button_context_mainmenu; }
        case CONTEXT_CUSTOM | CONTEXT_TREE:     { return button_context_tree; }
        case CONTEXT_SETTINGS:                  { return button_context_settings; }
        case CONTEXT_SETTINGS_COLOURCHOOSER:    { return button_context_colorchooser; }
        case CONTEXT_SETTINGS_EQ:               { return button_context_eq; }
        case CONTEXT_SETTINGS_TIME:             { return button_context_time; }
        case CONTEXT_KEYBOARD:                  { return button_context_keyboard; }
        case CONTEXT_FM:                        { return button_context_radio; }
        case CONTEXT_BOOKMARKSCREEN:            { return button_context_bmark; }
        case CONTEXT_QUICKSCREEN:               { return button_context_quickscreen; }
        case CONTEXT_PITCHSCREEN:               { return button_context_pitchscreen; }
        case CONTEXT_CUSTOM | CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_RECTRIGGER:       { return button_context_settings_right_is_inc; }
        case CONTEXT_TREE:
        {
            if(global_settings.hold_lr_for_scroll_in_list)
            {
                return button_context_listtree_scroll_without_combo;
            }
            return button_context_listtree_scroll_with_combo;
        }
    }

    return button_context_standard;
}
