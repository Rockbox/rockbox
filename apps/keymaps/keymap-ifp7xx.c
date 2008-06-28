/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Tomasz Malesinski
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

/* Button Code Definitions for iriver iFP7xx target */

#include "config.h"
#include "action.h"
#include "button.h"

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
    
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_PLAY,                BUTTON_NONE },
    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_SELECT },

    { ACTION_STD_QUICKSCREEN,   BUTTON_MODE|BUTTON_REPEAT,  BUTTON_MODE }, 
    { ACTION_STD_MENU,          BUTTON_MODE|BUTTON_REL,     BUTTON_MODE },
    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,   BUTTON_SELECT },
    { ACTION_STD_OK,            BUTTON_RIGHT,               BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,          BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_WPS_SKIPNEXT,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,      BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_ABSETB_NEXTDIR,       BUTTON_PLAY|BUTTON_RIGHT,         BUTTON_PLAY },
    { ACTION_WPS_ABSETA_PREVDIR,       BUTTON_PLAY|BUTTON_LEFT,          BUTTON_PLAY },
    { ACTION_WPS_STOP,          BUTTON_EQ,                      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP,                      BUTTON_NONE },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_MODE|BUTTON_REPEAT,      BUTTON_MODE },
    { ACTION_WPS_MENU,          BUTTON_MODE|BUTTON_REL,         BUTTON_MODE },
    { ACTION_WPS_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },
    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_WPS_ABRESET,       BUTTON_PLAY|BUTTON_SELECT,      BUTTON_PLAY },
    { ACTION_WPS_ID3SCREEN,     BUTTON_PLAY|BUTTON_MODE,        BUTTON_PLAY },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_NONE,                  BUTTON_RIGHT,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_r_is_inc[]  = {
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
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_MODE,     BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_settings_bmark */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,    BUTTON_UP,                      BUTTON_NONE },
    { ACTION_QS_DOWNINV,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    
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
    { ACTION_PS_RESET,          BUTTON_MODE,                BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_PLAY,                BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                           BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,             BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                          BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT|BUTTON_REL,              BUTTON_SELECT },
    { ACTION_KBD_DONE,         BUTTON_MODE,                           BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_PLAY,                           BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                             BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                           BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,             BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_TIME:
            return button_context_settings_r_is_inc;
        case CONTEXT_SETTINGS:
            return button_context_settings;
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
