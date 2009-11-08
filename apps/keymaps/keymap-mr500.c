/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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

/* Button Code Definitions for the Olympus M:robe 500 target */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_RC_PLAY,                BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_RC_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_RC_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_OK,          BUTTON_RC_HEART|BUTTON_REL,    BUTTON_RC_HEART },
    { ACTION_STD_OK,          BUTTON_RC_FF,                  BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RC_FF|BUTTON_REPEAT,    BUTTON_RC_FF },

    { ACTION_STD_MENU,        BUTTON_RC_MODE,                BUTTON_NONE },
    { ACTION_STD_QUICKSCREEN, BUTTON_RC_MODE|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_CONTEXT,     BUTTON_RC_HEART|BUTTON_REPEAT, BUTTON_RC_HEART },
    { ACTION_STD_CANCEL,      BUTTON_POWER,                  BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_RC_REW,                 BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_RC_REW|BUTTON_REPEAT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
	{ ACTION_WPS_PLAY,		BUTTON_RC_PLAY|BUTTON_REL,	BUTTON_RC_PLAY },
    { ACTION_WPS_STOP,      BUTTON_RC_DOWN|BUTTON_REL,  BUTTON_RC_DOWN },

    { ACTION_WPS_SKIPNEXT,  BUTTON_RC_FF|BUTTON_REL,    BUTTON_RC_FF },
    { ACTION_WPS_SKIPPREV,  BUTTON_RC_REW|BUTTON_REL,   BUTTON_RC_REW },

    { ACTION_WPS_SEEKBACK,  BUTTON_RC_REW|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,   BUTTON_RC_FF|BUTTON_REPEAT, BUTTON_NONE },
    
    { ACTION_WPS_STOPSEEK,  BUTTON_RC_REW|BUTTON_REL,   
                                                BUTTON_RC_REW|BUTTON_REPEAT},
                                                
    { ACTION_WPS_STOPSEEK,  BUTTON_RC_FF|BUTTON_REL,
                                                BUTTON_RC_FF|BUTTON_REPEAT},

    { ACTION_WPS_VOLDOWN,   BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_RC_VOL_UP,                 BUTTON_NONE },

    { ACTION_WPS_QUICKSCREEN, BUTTON_RC_MODE|BUTTON_REPEAT,   BUTTON_RC_MODE },
    { ACTION_WPS_MENU,        BUTTON_RC_MODE|BUTTON_REL,      BUTTON_RC_MODE },
    { ACTION_WPS_MENU,        BUTTON_POWER|BUTTON_REL,        BUTTON_POWER },
    { ACTION_WPS_CONTEXT,     BUTTON_RC_HEART|BUTTON_REPEAT,  BUTTON_RC_HEART },

    { ACTION_WPS_BROWSE,      BUTTON_RC_HEART|BUTTON_REL,     BUTTON_RC_HEART },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[]  = {
#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,       BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_LIST_VOLUP,       BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,     BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,     BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_settings[]  = {

    { ACTION_SETTINGS_INC,       BUTTON_RC_PLAY,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RC_PLAY|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_RC_DOWN,                  BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_RC_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_RC_REW,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,     BUTTON_RC_REW|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_RC_FF,                    BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,     BUTTON_RC_FF|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_SETTINGS_RESET,     BUTTON_RC_MODE,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RC_FF,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RC_FF|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_RC_REW,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_RC_REW|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_RC_PLAY,               BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_RC_PLAY|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RC_DOWN,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RC_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_RC_MODE,               BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT, BUTTON_RC_PLAY,      BUTTON_NONE },
    { ACTION_YESNO_ACCEPT, BUTTON_POWER,        BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_eq */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_STD_CANCEL,       BUTTON_POWER,              BUTTON_NONE },
    { ACTION_STD_OK,           BUTTON_RC_HEART,           BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_STD_CANCEL,        BUTTON_RC_MODE,         BUTTON_NONE },
    { ACTION_QS_TOP,            BUTTON_RC_PLAY,         BUTTON_NONE },
    { ACTION_QS_DOWN,           BUTTON_RC_DOWN,         BUTTON_NONE },
    { ACTION_QS_LEFT,           BUTTON_RC_REW,          BUTTON_NONE },
    { ACTION_QS_RIGHT,          BUTTON_RC_FF,           BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_RC_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RC_FF|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_RC_HEART,                  BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_RC_MODE|BUTTON_REL,        BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_POWER|BUTTON_REL,          BUTTON_POWER },
    { ACTION_KBD_BACKSPACE,    BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_RC_PLAY,                   BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_RC_PLAY|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_RC_DOWN,                   BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_RC_DOWN|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_RC_HEART|BUTTON_REL,       BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

const struct button_mapping* target_get_context_mapping(int context)
{
    switch (context&(~CONTEXT_REMOTE))
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_MAINMENU:
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else
                return button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_SETTINGS:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
            return button_context_settings_right_is_inc;

        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_colorchooser;
        case CONTEXT_SETTINGS_EQ:
            return button_context_eq;

        case CONTEXT_SETTINGS_TIME:
            return button_context_time;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
    }
    return button_context_standard;
}

