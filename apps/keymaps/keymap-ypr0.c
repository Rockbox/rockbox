/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: keymap-sdl.c 28704 2010-11-29 11:28:53Z teru $
 *
 * Copyright (C) 2011 Lorenzo Miori
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

/* Button Code Definitions for Samsung YP-R0 target */

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
    { ACTION_STD_PREV,           BUTTON_UP,                     BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,     BUTTON_UP|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_DOWN,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,     BUTTON_DOWN|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_STD_CANCEL,         BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_BACK|BUTTON_REL,        BUTTON_BACK },

    { ACTION_STD_OK,             BUTTON_SELECT|BUTTON_REL,      BUTTON_SELECT },
    { ACTION_STD_OK,             BUTTON_RIGHT,                  BUTTON_NONE },

    { ACTION_STD_QUICKSCREEN,    BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CONTEXT,        BUTTON_SELECT|BUTTON_REPEAT,   BUTTON_SELECT },
    { ACTION_STD_MENU,           BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },

    { ACTION_STD_CONTEXT,        BUTTON_MENU|BUTTON_REL,        BUTTON_NONE },
    { ACTION_STD_QUICKSCREEN,    BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_STD_KEYLOCK,        BUTTON_POWER|BUTTON_REL,       BUTTON_POWER },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,           BUTTON_SELECT|BUTTON_REL,      BUTTON_SELECT },
    { ACTION_WPS_STOP,           BUTTON_POWER|BUTTON_REPEAT,    BUTTON_NONE },

    { ACTION_WPS_BROWSE,         BUTTON_BACK|BUTTON_REL,        BUTTON_BACK },
    { ACTION_WPS_MENU,           BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },

    /* NOTE: this is available only enabling AB-Repeat mode */
    { ACTION_WPS_HOTKEY,         BUTTON_USER|BUTTON_REL,        BUTTON_USER },
    { ACTION_WPSAB_SINGLE,       BUTTON_USER|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_KEYLOCK,        BUTTON_POWER|BUTTON_REL,       BUTTON_POWER },
    { ACTION_WPS_CONTEXT,        BUTTON_SELECT|BUTTON_REPEAT,   BUTTON_SELECT },
    { ACTION_WPS_QUICKSCREEN,    BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_WPS_SKIPPREV,       BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,       BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,       BUTTON_LEFT|BUTTON_REL,        BUTTON_LEFT|BUTTON_REPEAT },

    { ACTION_WPS_SKIPNEXT,       BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT },
    { ACTION_WPS_SEEKFWD,        BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,       BUTTON_RIGHT|BUTTON_REL,       BUTTON_RIGHT|BUTTON_REPEAT },


    { ACTION_WPS_VOLUP,          BUTTON_UP|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_VOLUP,          BUTTON_UP,                     BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,        BUTTON_DOWN|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,        BUTTON_DOWN,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,           BUTTON_USER|BUTTON_REPEAT,     BUTTON_USER },
    { ACTION_TREE_STOP,          BUTTON_POWER|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_TREE_HOTKEY,        BUTTON_USER|BUTTON_REL,        BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_settings[]  = {

    { ACTION_SETTINGS_INC,       BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_LEFT|BUTTON_REPEAT,     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_TREE)
}; /* button_context_settings */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,              BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = { //check
    { ACTION_STD_OK,     BUTTON_SELECT|BUTTON_REL, BUTTON_SELECT },
    { ACTION_STD_CANCEL, BUTTON_BACK,              BUTTON_NONE   },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_colorchooser */

static const struct button_mapping button_context_eq[]  = {

    { ACTION_STD_CANCEL, BUTTON_MENU|BUTTON_REL, BUTTON_MENU },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_eq */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {

    { ACTION_SETTINGS_INC,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {

    { ACTION_QS_TOP,            BUTTON_UP,                      BUTTON_NONE },
    { ACTION_QS_TOP,            BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,           BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_QS_DOWN,           BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,           BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,           BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,          BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,          BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_MENU,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {

    { ACTION_PS_INC_SMALL,      BUTTON_UP,                      BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_PS_SLOWER,         BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },

    { ACTION_PS_RESET,          BUTTON_SELECT,                  BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_USER,                    BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_MENU|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_BACK|BUTTON_REL,         BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {

    { ACTION_KBD_UP,           BUTTON_UP,                       BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                     BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_KBD_SELECT,       BUTTON_SELECT,                   BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_BACK|BUTTON_REL,          BUTTON_BACK },
    { ACTION_KBD_DONE,         BUTTON_MENU|BUTTON_REL,          BUTTON_MENU },
    { ACTION_KBD_BACKSPACE,    BUTTON_USER,                     BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_POWER,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_keyboard */

static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,            BUTTON_SELECT | BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_FM_PRESET,          BUTTON_MENU | BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_FM_STOP,            BUTTON_POWER | BUTTON_REL,     BUTTON_POWER },
    { ACTION_FM_MODE,            BUTTON_MENU | BUTTON_REL,      BUTTON_MENU },
    { ACTION_FM_EXIT,            BUTTON_BACK | BUTTON_REL,      BUTTON_BACK },
    { ACTION_FM_PLAY,            BUTTON_SELECT | BUTTON_REL,    BUTTON_SELECT },
    { ACTION_FM_NEXT_PRESET,     BUTTON_USER | BUTTON_RIGHT,    BUTTON_NONE },
    { ACTION_FM_PREV_PRESET,     BUTTON_USER | BUTTON_LEFT,     BUTTON_NONE },
    /* Volume */
    { ACTION_SETTINGS_INC,       BUTTON_UP | BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_UP,                     BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_DOWN | BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_DOWN,                   BUTTON_NONE },
    /* Tuning */
    { ACTION_STD_PREV,           BUTTON_LEFT,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,     BUTTON_LEFT | BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_RIGHT,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,     BUTTON_RIGHT | BUTTON_REPEAT,  BUTTON_NONE },
}; /* button_context_radio */

const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_MAINMENU:
        case CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_SETTINGS:
            return button_context_settings;

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
