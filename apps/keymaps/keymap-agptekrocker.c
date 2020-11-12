/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"
#define BTN_VOL_COMBO (BUTTON_VOLUP | BUTTON_VOLDOWN)
/* {Action Code,    Button code,    Prereq button code } */

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_UP,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_UP|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_DOWN,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_SELECT|BUTTON_REL,    BUTTON_SELECT },
    { ACTION_STD_OK,         BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,     BUTTON_LEFT,                 BUTTON_NONE },
    { ACTION_STD_CONTEXT,    BUTTON_SELECT|BUTTON_REPEAT, BUTTON_SELECT },
    { ACTION_STD_MENU,       BUTTON_POWER,                BUTTON_NONE },

/*    ACTION_STD_QUICKSCREEN,
 *    ACTION_STD_KEYLOCK
 *    ACTION_STD_REC
 *    ACTION_STD_HOTKEY

 *    { ACTION_STD_QUICKSCREEN, BUTTON_SELECT|BUTTON_REPEAT,         BUTTON_SELECT },
 */

    LAST_ITEM_IN_LIST
}; /* button_context_standard */


static const struct button_mapping button_context_mainmenu[]  = {
    { ACTION_TREE_WPS,       BUTTON_POWER,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_TREE),
}; /* button_context_mainmenu as with sansa clip mapping - "back" button returns you to WPS */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,        BUTTON_SELECT|BUTTON_REL,     BUTTON_SELECT },
    { ACTION_WPS_SEEKBACK,    BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,     BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_LEFT|BUTTON_REL,       BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,    BUTTON_RIGHT|BUTTON_REL,      BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,    BUTTON_RIGHT|BUTTON_REL,      BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,    BUTTON_LEFT|BUTTON_REL,       BUTTON_LEFT },
    { ACTION_WPS_STOP,        BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_SELECT },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOLDOWN,               BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOLDOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOLUP,                 BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOLUP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_BROWSE,      BUTTON_UP|BUTTON_REPEAT,      BUTTON_UP },
/*    ACTION_WPS_ID3SCREEN    optional */
    { ACTION_WPS_CONTEXT,     BUTTON_DOWN|BUTTON_REL,       BUTTON_DOWN },
    { ACTION_WPS_QUICKSCREEN, BUTTON_DOWN|BUTTON_REPEAT,    BUTTON_DOWN },
    { ACTION_WPS_MENU,        BUTTON_UP|BUTTON_REL,         BUTTON_UP }, /* this should be the same as ACTION_STD_MENU */
/*    ACTION_WPS_VIEW_PLAYLIST
 *    ACTION_WPS_LIST_BOOKMARKS,  optional
 *    ACTION_WPS_CREATE_BOOKMARK, optional
 */

    { ACTION_STD_KEYLOCK,     BUTTON_POWER,                 BUTTON_NONE },
    { ACTION_WPS_HOTKEY,      BTN_VOL_COMBO|BUTTON_REL,     BTN_VOL_COMBO},

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,      BUTTON_VOLUP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_VOLUP|BUTTON_REPEAT,   BUTTON_NONE },
/*    ACTION_SETTINGS_INCBIGSTEP */
    { ACTION_SETTINGS_DEC,       BUTTON_VOLDOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_VOLDOWN|BUTTON_REPEAT, BUTTON_NONE },
/*    ACTION_SETTINGS_DECBIGSTEP */
    { ACTION_SETTINGS_RESET,     BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_SELECT },
/*    ACTION_SETTINGS_SET, Used by touchscreen targets */
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings */

static const struct button_mapping button_context_list[]  = {
/*    ACTION_LISTTREE_PGUP, optional
 *    ACTION_LISTTREE_PGDOWN, optional
 */

#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,   BUTTON_VOLUP,                 BUTTON_NONE },
    { ACTION_LIST_VOLUP,   BUTTON_VOLUP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOLDOWN,               BUTTON_NONE },
    { ACTION_LIST_VOLDOWN, BUTTON_VOLDOWN|BUTTON_REPEAT, BUTTON_NONE },
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */


static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_HOTKEY,     BTN_VOL_COMBO|BUTTON_REL,    BTN_VOL_COMBO},
/*    ACTION_TREE_ROOT_INIT,
 *    ACTION_TREE_PGLEFT, optional
 *    ACTION_TREE_PGRIGHT, optional
 *    ACTION_TREE_STOP,
 *    ACTION_TREE_WPS,
 *    ACTION_TREE_HOTKEY,
 */
/*    { ACTION_TREE_WPS, BUTTON_UP|BUTTON_REL, BUTTON_UP },
 *    { ACTION_TREE_STOP,   BUTTON_POWER|BUTTON_REL,      BUTTON_POWER },
 *    { ACTION_TREE_HOTKEY, BUTTON_REC|BUTTON_REL,        BUTTON_REC },
 */
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_tree */


static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT, BUTTON_SELECT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_TOP,   BUTTON_UP|BUTTON_REL,       BUTTON_NONE },
    { ACTION_QS_TOP,   BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_DOWN,  BUTTON_DOWN|BUTTON_REL,     BUTTON_NONE },
    { ACTION_QS_DOWN,  BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_QS_LEFT,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_QS_LEFT,  BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_QS_RIGHT, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_QS_RIGHT, BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_CANCEL, BUTTON_SELECT, BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_quickscreen */

static const struct button_mapping button_context_settings_time[] = {
    { ACTION_STD_PREV,           BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_STD_PREVREPEAT,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_STD_NEXT,           BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_STD_NEXTREPEAT,     BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT },
    { ACTION_STD_CANCEL,         BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_STD_OK,             BUTTON_SELECT|BUTTON_REL,   BUTTON_SELECT },
    { ACTION_SETTINGS_INC,       BUTTON_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST
    //LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_settings_time */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                    BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                  BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                  BUTTON_NONE },
/*    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,       BUTTON_NONE }, */
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                 BUTTON_NONE },
/*    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,      BUTTON_NONE }, */
    { ACTION_PS_TOGGLE_MODE,    BUTTON_SELECT|BUTTON_REL,     BUTTON_SELECT },
    { ACTION_PS_RESET,          BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_SELECT },
    { ACTION_PS_EXIT,           BUTTON_POWER,                 BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_pitchscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                             BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                            BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_POWER|BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_POWER|BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_POWER|BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_POWER|BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_VOLDOWN,                          BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_VOLDOWN|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_VOLUP,                            BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_VOLUP|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_POWER|BUTTON_SELECT,              BUTTON_POWER },
    { ACTION_KBD_BACKSPACE,    BUTTON_DOWN,                             BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_DOWN|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT,                           BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_UP,                               BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_POWER,                            BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_POWER|BUTTON_REL,                 BUTTON_POWER },
    { ACTION_KBD_MORSE_SELECT, BUTTON_SELECT|BUTTON_REL,                BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;

        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_MAINMENU:
            return button_context_mainmenu;
        case CONTEXT_TREE:
            return button_context_tree;
        case CONTEXT_LIST:
            return button_context_list;

        case CONTEXT_SETTINGS:
	case CONTEXT_SETTINGS_EQ:
            return button_context_settings;

        case CONTEXT_SETTINGS_TIME:
            return button_context_settings_time;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;

        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;

        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;

        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;

        case CONTEXT_KEYBOARD:
        case CONTEXT_MORSE_INPUT:
            return button_context_keyboard;

        default:
            return button_context_standard;
    }
    return button_context_standard;
}
