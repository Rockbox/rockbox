/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

/* TODO */

/* Button Code Definitions for IHIFI 800 targets */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* {Action Code,    Button code,    Prereq button code } */

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */
static const struct button_mapping button_context_standard[] = {
    { ACTION_STD_PREV,        BUTTON_PREV,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_NEXT,                      BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_NEXT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_POWER|BUTTON_REL,          BUTTON_POWER },
    { ACTION_STD_OK,          BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_STD_CONTEXT,     BUTTON_POWER|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_HOME,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[] = {
    { ACTION_WPS_SKIPPREV,    BUTTON_PREV|BUTTON_REL,           BUTTON_PREV },
    { ACTION_WPS_SEEKBACK,    BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_PREV|BUTTON_REL,           BUTTON_PREV|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,    BUTTON_NEXT|BUTTON_REL,           BUTTON_NEXT },
    { ACTION_WPS_SEEKFWD,     BUTTON_NEXT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,    BUTTON_NEXT|BUTTON_REL,           BUTTON_NEXT|BUTTON_REPEAT },
    { ACTION_WPS_VOLUP,       BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_WPS_VOLUP,       BUTTON_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,     BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_PLAY,        BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_WPS_QUICKSCREEN, BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },
    { ACTION_WPS_CONTEXT,     BUTTON_POWER|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_MENU,        BUTTON_POWER|BUTTON_REL,          BUTTON_POWER },
    { ACTION_WPS_BROWSE,      BUTTON_HOME|BUTTON_REL,           BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[] = {
    { ACTION_LIST_VOLUP,      BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_LIST_VOLUP,      BUTTON_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,    BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,    BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[] = {
    { ACTION_BMS_DELETE,      BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_bmark */

/** Keyboard **/
static const struct button_mapping button_context_keyboard[] = {
    { ACTION_KBD_ABORT,       BUTTON_HOME,                      BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,   BUTTON_HOME|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_KBD_LEFT,        BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_KBD_LEFT,        BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_KBD_UP,          BUTTON_PREV,                      BUTTON_NONE },
    { ACTION_KBD_UP,          BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_KBD_DOWN,        BUTTON_NEXT,                      BUTTON_NONE },
    { ACTION_KBD_DOWN,        BUTTON_NEXT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_KBD_RIGHT,       BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_KBD_RIGHT,       BUTTON_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_SELECT,      BUTTON_PLAY,                      BUTTON_NONE },
    { ACTION_KBD_DONE,        BUTTON_POWER,                     BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

/** Pitchscreen **/
static const struct button_mapping button_context_pitchscreen[] = {
    { ACTION_PS_INC_SMALL,    BUTTON_PREV,                      BUTTON_NONE },
    { ACTION_PS_INC_BIG,      BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,    BUTTON_NEXT,                      BUTTON_NONE },
    { ACTION_PS_DEC_BIG,      BUTTON_NEXT|BUTTON_REPEAT,        BUTTON_NONE },   
    { ACTION_PS_NUDGE_LEFT,   BUTTON_HOME,                      BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF, BUTTON_HOME|BUTTON_REL,          BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,  BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_VOL_DOWN|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_RESET,        BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,  BUTTON_PLAY,                      BUTTON_NONE },
    { ACTION_PS_EXIT,         BUTTON_POWER,                     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchscreen */

/** Quickscreen **/
static const struct button_mapping button_context_quickscreen[] = {
    { ACTION_QS_TOP,          BUTTON_PREV,                      BUTTON_NONE },
    { ACTION_QS_TOP,          BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,         BUTTON_NEXT,                      BUTTON_NONE },
    { ACTION_QS_DOWN,         BUTTON_NEXT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_LEFT,         BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_QS_LEFT,         BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_RIGHT,        BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_QS_RIGHT,        BUTTON_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_HOME,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

/** Settings - General Mappings **/
static const struct button_mapping button_context_settings[] = {
    { ACTION_STD_PREV,        BUTTON_PREV,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_PREV|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_NEXT,                      BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_NEXT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_POWER|BUTTON_REL,          BUTTON_POWER },
    { ACTION_STD_OK,          BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_SETTINGS_RESET,  BUTTON_PLAY|BUTTON_REPEAT,        BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_vol_is_inc[] = {
    { ACTION_SETTINGS_INC,    BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,    BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_right_is_inc */

/** Tree **/
static const struct button_mapping button_context_tree[] = {
    { ACTION_TREE_WPS,        BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

/** Yes/No Screen **/
static const struct button_mapping button_context_yesnoscreen[] = {
    { ACTION_YESNO_ACCEPT,    BUTTON_PLAY,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesnoscreen */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_SETTINGS:
            return button_context_settings;
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_vol_is_inc;
        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            return button_context_tree;
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesnoscreen;
    }
    return button_context_standard;
}
