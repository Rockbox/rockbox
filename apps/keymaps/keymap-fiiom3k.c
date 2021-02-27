/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

/* Button Code Definitions for FiiO M3K target */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* {Action Code,    Button code,    Prereq button code } */

static const struct button_mapping button_context_standard[] = {
    {ACTION_STD_PREV,           BUTTON_UP,                          BUTTON_NONE},
    {ACTION_STD_PREVREPEAT,     BUTTON_UP|BUTTON_REPEAT,            BUTTON_NONE},
    {ACTION_STD_NEXT,           BUTTON_DOWN,                        BUTTON_NONE},
    {ACTION_STD_NEXTREPEAT,     BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_STD_PREV,           BUTTON_SCROLL_BACK,                 BUTTON_NONE},
    {ACTION_STD_PREVREPEAT,     BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_NONE},
    {ACTION_STD_NEXT,           BUTTON_SCROLL_FWD,                  BUTTON_NONE},
    {ACTION_STD_NEXTREPEAT,     BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_NONE},
    {ACTION_STD_OK,             BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {ACTION_STD_CANCEL,         BUTTON_BACK|BUTTON_REL,             BUTTON_BACK},
    {ACTION_STD_CONTEXT,        BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT},
    {ACTION_STD_CONTEXT,        BUTTON_MENU|BUTTON_REL,             BUTTON_MENU},
    {ACTION_STD_MENU,           BUTTON_BACK|BUTTON_REPEAT,          BUTTON_BACK},
    {ACTION_STD_QUICKSCREEN,    BUTTON_MENU|BUTTON_REPEAT,          BUTTON_MENU},
    {ACTION_STD_KEYLOCK,        BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {ACTION_STD_HOTKEY,         BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[] = {
    {ACTION_WPS_PLAY,           BUTTON_PLAY|BUTTON_REL,             BUTTON_PLAY},
    {ACTION_WPS_PLAY,           BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {ACTION_WPS_STOP,           BUTTON_POWER|BUTTON_REPEAT,         BUTTON_POWER},
    {ACTION_WPS_VOLUP,          BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_WPS_VOLUP,          BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
    {ACTION_WPS_VOLDOWN,        BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_WPS_VOLDOWN,        BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    {ACTION_WPS_SKIPNEXT,       BUTTON_RIGHT|BUTTON_REL,            BUTTON_RIGHT},
    {ACTION_WPS_SKIPPREV,       BUTTON_LEFT|BUTTON_REL,             BUTTON_LEFT},
    {ACTION_WPS_SEEKFWD,        BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE},
    {ACTION_WPS_STOPSEEK,       BUTTON_RIGHT|BUTTON_REL,            BUTTON_RIGHT|BUTTON_REPEAT},
    {ACTION_WPS_SEEKBACK,       BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_WPS_STOPSEEK,       BUTTON_LEFT|BUTTON_REL,             BUTTON_LEFT|BUTTON_REPEAT},
    {ACTION_WPS_BROWSE,         BUTTON_BACK|BUTTON_REPEAT,          BUTTON_BACK},
    {ACTION_WPS_MENU,           BUTTON_BACK|BUTTON_REL,             BUTTON_BACK},
    {ACTION_WPS_CONTEXT,        BUTTON_MENU|BUTTON_REL,             BUTTON_MENU},
    {ACTION_WPS_QUICKSCREEN,    BUTTON_MENU|BUTTON_REPEAT,          BUTTON_MENU},
    {ACTION_STD_KEYLOCK,        BUTTON_POWER|BUTTON_REL,            BUTTON_POWER},
    {ACTION_WPS_HOTKEY,         BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_PLAY},
    {ACTION_WPS_VIEW_PLAYLIST,  BUTTON_SCROLL_FWD,                  BUTTON_NONE},
    {ACTION_WPS_VIEW_PLAYLIST,  BUTTON_SCROLL_BACK,                 BUTTON_NONE},
    {ACTION_WPS_ABSETA_PREVDIR, BUTTON_UP|BUTTON_REPEAT,            BUTTON_UP},
    {ACTION_WPS_ABSETB_NEXTDIR, BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_DOWN},
    {ACTION_WPS_ABRESET,        BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT},
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_tree[] = {
    {ACTION_TREE_STOP,          BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_PLAY},
    {ACTION_TREE_WPS,           BUTTON_BACK|BUTTON_REPEAT,          BUTTON_BACK},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_list[] = {
    {ACTION_LISTTREE_PGUP,      BUTTON_LEFT,                        BUTTON_NONE},
    {ACTION_LISTTREE_PGUP,      BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_LISTTREE_PGDOWN,    BUTTON_RIGHT,                       BUTTON_NONE},
    {ACTION_LISTTREE_PGDOWN,    BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE},
    {ACTION_LIST_VOLUP,         BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_LIST_VOLUP,         BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
    {ACTION_LIST_VOLDOWN,       BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_LIST_VOLDOWN,       BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_settings[] = {
    {ACTION_SETTINGS_INC,           BUTTON_UP,                          BUTTON_NONE},
    {ACTION_SETTINGS_INCREPEAT,     BUTTON_UP|BUTTON_REPEAT,            BUTTON_NONE},
    {ACTION_SETTINGS_INC,           BUTTON_SCROLL_BACK,                 BUTTON_NONE},
    {ACTION_SETTINGS_INCREPEAT,     BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_NONE},
    {ACTION_SETTINGS_INCBIGSTEP,    BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_SETTINGS_DEC,           BUTTON_DOWN,                        BUTTON_NONE},
    {ACTION_SETTINGS_DECREPEAT,     BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_SETTINGS_DEC,           BUTTON_SCROLL_FWD,                  BUTTON_NONE},
    {ACTION_SETTINGS_DECREPEAT,     BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_NONE},
    {ACTION_SETTINGS_DECBIGSTEP,    BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_SETTINGS_RESET,         BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT},
    {ACTION_STD_NEXT,               BUTTON_RIGHT,                       BUTTON_NONE},
    {ACTION_STD_NEXTREPEAT,         BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE},
    {ACTION_STD_PREV,               BUTTON_LEFT,                        BUTTON_NONE},
    {ACTION_STD_PREVREPEAT,         BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_eq[] = {
    {ACTION_SETTINGS_INC,           BUTTON_RIGHT,                       BUTTON_NONE},
    {ACTION_SETTINGS_INCREPEAT,     BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE},
    {ACTION_SETTINGS_INCBIGSTEP,    BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_SETTINGS_DEC,           BUTTON_LEFT,                        BUTTON_NONE},
    {ACTION_SETTINGS_DECREPEAT,     BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_SETTINGS_DECBIGSTEP,    BUTTON_VOL_DOWN,                    BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_eq */

static const struct button_mapping button_context_quickscreen[] = {
    {ACTION_QS_TOP,                 BUTTON_UP,                          BUTTON_NONE},
    {ACTION_QS_DOWN,                BUTTON_DOWN,                        BUTTON_NONE},
    {ACTION_QS_LEFT,                BUTTON_LEFT,                        BUTTON_NONE},
    {ACTION_QS_RIGHT,               BUTTON_RIGHT,                       BUTTON_NONE},
    {ACTION_QS_VOLUP,               BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_QS_VOLDOWN,             BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_STD_CANCEL,             BUTTON_SELECT,                      BUTTON_NONE},
    {ACTION_STD_CANCEL,             BUTTON_POWER,                       BUTTON_NONE},
    {ACTION_STD_CANCEL,             BUTTON_BACK,                        BUTTON_NONE},
    LAST_ITEM_IN_LIST
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[] = {
    {ACTION_PS_INC_SMALL,           BUTTON_UP,                          BUTTON_NONE},
    {ACTION_PS_INC_SMALL,           BUTTON_SCROLL_BACK,                 BUTTON_NONE},
    {ACTION_PS_INC_BIG,             BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_PS_DEC_SMALL,           BUTTON_DOWN,                        BUTTON_NONE},
    {ACTION_PS_DEC_SMALL,           BUTTON_SCROLL_FWD,                  BUTTON_NONE},
    {ACTION_PS_DEC_BIG,             BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_PS_NUDGE_LEFT,          BUTTON_LEFT,                        BUTTON_NONE},
    {ACTION_PS_NUDGE_RIGHT,         BUTTON_RIGHT,                       BUTTON_NONE},
    {ACTION_PS_NUDGE_LEFTOFF,       BUTTON_LEFT|BUTTON_REL,             BUTTON_NONE},
    {ACTION_PS_NUDGE_RIGHTOFF,      BUTTON_RIGHT|BUTTON_REL,            BUTTON_NONE},
    {ACTION_PS_TOGGLE_MODE,         BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT},
    {ACTION_PS_RESET,               BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT},
    {ACTION_PS_EXIT,                BUTTON_POWER,                       BUTTON_NONE},
    {ACTION_PS_FASTER,              BUTTON_BACK,                        BUTTON_NONE},
    {ACTION_PS_SLOWER,              BUTTON_MENU,                        BUTTON_NONE},
    LAST_ITEM_IN_LIST
}; /* button_context_pitchscreen */

static const struct button_mapping button_context_yesnoscreen[] = {
    {ACTION_YESNO_ACCEPT,           BUTTON_PLAY,                        BUTTON_NONE},
    {ACTION_YESNO_ACCEPT,           BUTTON_SELECT,                      BUTTON_NONE},
    {ACTION_STD_CANCEL,             BUTTON_BACK,                        BUTTON_NONE},
    {ACTION_STD_CANCEL,             BUTTON_POWER,                       BUTTON_NONE},
    LAST_ITEM_IN_LIST
}; /* button_context_yesnoscreen */

static const struct button_mapping button_context_keyboard[] = {
    {ACTION_KBD_UP,                 BUTTON_UP,                          BUTTON_NONE},
    {ACTION_KBD_DOWN,               BUTTON_DOWN,                        BUTTON_NONE},
    {ACTION_KBD_LEFT,               BUTTON_LEFT,                        BUTTON_NONE},
    {ACTION_KBD_RIGHT,              BUTTON_RIGHT,                       BUTTON_NONE},
    {ACTION_KBD_UP,                 BUTTON_UP|BUTTON_REPEAT,            BUTTON_NONE},
    {ACTION_KBD_DOWN,               BUTTON_DOWN|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_KBD_LEFT,               BUTTON_LEFT|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_KBD_RIGHT,              BUTTON_RIGHT|BUTTON_REPEAT,         BUTTON_NONE},
    {ACTION_KBD_SELECT,             BUTTON_SELECT,                      BUTTON_NONE},
    {ACTION_KBD_BACKSPACE,          BUTTON_BACK,                        BUTTON_NONE},
    {ACTION_KBD_BACKSPACE,          BUTTON_BACK|BUTTON_REPEAT,          BUTTON_NONE},
    {ACTION_KBD_DONE,               BUTTON_PLAY,                        BUTTON_NONE},
    {ACTION_KBD_ABORT,              BUTTON_POWER,                       BUTTON_NONE},
    {ACTION_KBD_PAGE_FLIP,          BUTTON_MENU,                        BUTTON_NONE},
    {ACTION_KBD_CURSOR_LEFT,        BUTTON_VOL_DOWN,                    BUTTON_NONE},
    {ACTION_KBD_CURSOR_LEFT,        BUTTON_VOL_DOWN|BUTTON_REPEAT,      BUTTON_NONE},
    {ACTION_KBD_CURSOR_RIGHT,       BUTTON_VOL_UP,                      BUTTON_NONE},
    {ACTION_KBD_CURSOR_RIGHT,       BUTTON_VOL_UP|BUTTON_REPEAT,        BUTTON_NONE},
    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        default:
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_tree;
        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings;
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_settings_eq;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesnoscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
    }
}
