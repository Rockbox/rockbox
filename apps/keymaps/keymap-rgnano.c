/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Hairo R. Carela
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

/* Button Code Definitions for Android targets */

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
    { ACTION_STD_PREV,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_B,                   BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_A,                   BUTTON_NONE },
    { ACTION_STD_MENU,          BUTTON_Y|BUTTON_REL,        BUTTON_Y },
    { ACTION_STD_CONTEXT,       BUTTON_START|BUTTON_REL,    BUTTON_START },
    { ACTION_STD_QUICKSCREEN,   BUTTON_R|BUTTON_REL,        BUTTON_R },
    { ACTION_STD_KEYLOCK,       BUTTON_FN|BUTTON_START,     BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,              BUTTON_A|BUTTON_REL,            BUTTON_A },
    { ACTION_WPS_STOP,              BUTTON_A|BUTTON_REPEAT,         BUTTON_A },
    { ACTION_WPS_STOP,              BUTTON_L|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_WPS_SKIPPREV,          BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SKIPNEXT,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SEEKBACK,          BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,           BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,          BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_VOLDOWN,           BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,           BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLUP,             BUTTON_UP,                      BUTTON_NONE },
    { ACTION_WPS_VOLUP,             BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_MENU,              BUTTON_B|BUTTON_REL,            BUTTON_B },
    { ACTION_WPS_CONTEXT,           BUTTON_START|BUTTON_REL,        BUTTON_START },
    { ACTION_WPS_HOTKEY,            BUTTON_L|BUTTON_REL,            BUTTON_L },
    { ACTION_WPS_QUICKSCREEN,       BUTTON_R|BUTTON_REL,            BUTTON_R },
    { ACTION_WPS_BROWSE,            BUTTON_Y|BUTTON_REL,            BUTTON_Y },
    { ACTION_WPS_ID3SCREEN,         BUTTON_X|BUTTON_REL,            BUTTON_X },
    { ACTION_WPS_PITCHSCREEN,       BUTTON_R|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_WPS_ABSETA_PREVDIR,    BUTTON_A|BUTTON_LEFT,           BUTTON_A },
    { ACTION_WPS_ABSETB_NEXTDIR,    BUTTON_A|BUTTON_RIGHT,          BUTTON_A },
    { ACTION_WPS_ABRESET,           BUTTON_A|BUTTON_UP,             BUTTON_A },
    { ACTION_WPS_ABRESET,           BUTTON_A|BUTTON_DOWN,           BUTTON_A },
    { ACTION_STD_KEYLOCK,           BUTTON_FN|BUTTON_START,         BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[] = {
    { ACTION_LIST_VOLUP,      BUTTON_FN|BUTTON_A,                  BUTTON_NONE },
    { ACTION_LIST_VOLUP,      BUTTON_FN|BUTTON_A|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,    BUTTON_FN|BUTTON_Y,                  BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,    BUTTON_FN|BUTTON_Y|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,   BUTTON_LEFT,                         BUTTON_NONE },
    { ACTION_LISTTREE_PGUP,   BUTTON_LEFT|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN, BUTTON_RIGHT,                        BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN, BUTTON_RIGHT|BUTTON_REPEAT,          BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_X|BUTTON_REL,         BUTTON_X },
    { ACTION_TREE_HOTKEY, BUTTON_L|BUTTON_REL,         BUTTON_L },
    { ACTION_TREE_STOP,   BUTTON_L|BUTTON_REPEAT,      BUTTON_NONE},

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,  BUTTON_A,  BUTTON_NONE },

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
    { ACTION_BMS_DELETE,    BUTTON_Y,         BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_TOP,        BUTTON_UP,                        BUTTON_NONE },
    { ACTION_QS_TOP,        BUTTON_UP|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN,                      BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                     BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_QS_VOLUP,      BUTTON_FN|BUTTON_A,               BUTTON_NONE },
    { ACTION_QS_VOLUP,      BUTTON_FN|BUTTON_A|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_VOLDOWN,    BUTTON_FN|BUTTON_Y,               BUTTON_NONE },
    { ACTION_QS_VOLDOWN,    BUTTON_FN|BUTTON_Y|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_A,                   BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_Y,                   BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_B,                   BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                      BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_L,                       BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_L|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_R,                       BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_R|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_A,                       BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_Y|BUTTON_REL,            BUTTON_Y },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_X,                       BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_START|BUTTON_REL,        BUTTON_START },
    { ACTION_KBD_ABORT,        BUTTON_B|BUTTON_REL,            BUTTON_B },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_keyboard */

static const struct button_mapping button_context_radio[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */

const struct button_mapping* get_context_mapping(int context)
{
    switch (context & ~CONTEXT_LOCKED)
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
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_right_is_inc;

        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_colorchooser;
        case CONTEXT_SETTINGS_EQ:
            return button_context_eq;

        case CONTEXT_SETTINGS_TIME:
            return button_context_time;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_FM:
            return button_context_radio;
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
