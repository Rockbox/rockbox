/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|CONTEXT_TREE = the standard list/tree defines (without directions)


*/

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,          BUTTON_PREV,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_PREV|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_NEXT,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_NEXT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_REC|BUTTON_REL,       BUTTON_REC },
    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,    BUTTON_SELECT },
    { ACTION_STD_MENU,          BUTTON_REC|BUTTON_REPEAT,    BUTTON_REC },
    { ACTION_STD_QUICKSCREEN,   BUTTON_PLAY|BUTTON_REPEAT,   BUTTON_PLAY },
    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT, BUTTON_SELECT },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,          BUTTON_PLAY|BUTTON_REL,      BUTTON_PLAY },
    { ACTION_TREE_STOP,         BUTTON_PLAY|BUTTON_REPEAT,   BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_tree */

static const struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,              BUTTON_PREV,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_PREV|BUTTON_REL,     BUTTON_PREV },
    { ACTION_TREE_ROOT_INIT,    BUTTON_PREV|BUTTON_REPEAT,  BUTTON_PREV },
    { ACTION_TREE_PGLEFT,       BUTTON_PREV|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_PREV|BUTTON_REL,     BUTTON_PREV|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_NEXT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_NEXT|BUTTON_REL,    BUTTON_NEXT },
    { ACTION_TREE_PGRIGHT,      BUTTON_NEXT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_NEXT|BUTTON_REL,    BUTTON_NEXT|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_tree_scroll_lr */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,      BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_WPS_STOP,      BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },
    { ACTION_WPS_SKIPPREV,  BUTTON_PREV|BUTTON_REL,         BUTTON_PREV },
    { ACTION_WPS_SEEKBACK,  BUTTON_PREV|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_PREV|BUTTON_REL,         BUTTON_PREV|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,  BUTTON_NEXT|BUTTON_REL,        BUTTON_NEXT },
    { ACTION_WPS_SEEKFWD,   BUTTON_NEXT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_NEXT|BUTTON_REL,        BUTTON_NEXT|BUTTON_REPEAT },
    { ACTION_WPS_VOLDOWN,   BUTTON_VOL_DOWN,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_VOL_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_VOL_UP,                  BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_BROWSE,    BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    { ACTION_WPS_CONTEXT,   BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT },
    { ACTION_WPS_MENU,          BUTTON_REC|BUTTON_REL,         BUTTON_REC },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_SELECT|BUTTON_REPEAT,      BUTTON_SELECT },

    LAST_ITEM_IN_LIST,
}; /* button_context_wps */

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_VOL_UP,         BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_VOL_UP|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_VOL_DOWN,          BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_PREV,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_PREV|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_NEXT,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_NEXT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_OK,                BUTTON_SELECT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_STD_CANCEL,            BUTTON_REC|BUTTON_REL,      BUTTON_REC },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_yesno */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,          BUTTON_SELECT|BUTTON_REPEAT,       BUTTON_SELECT },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_TOP,        BUTTON_SELECT,                    BUTTON_NONE },
    { ACTION_QS_TOP,        BUTTON_SELECT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_PLAY,                    BUTTON_NONE },
    { ACTION_QS_DOWN,       BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_PREV,                    BUTTON_NONE },
    { ACTION_QS_LEFT,       BUTTON_PREV|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_NEXT,                   BUTTON_NONE },
    { ACTION_QS_RIGHT,      BUTTON_NEXT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_REC,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_VOL_UP,                BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_VOL_UP|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_PREV,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_PREV|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_NEXT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_NEXT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_PLAY,                BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_SELECT,                BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_REC,              BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_PREV|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_NEXT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchscreen */

const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_TREE:
        case CONTEXT_LIST:
        case CONTEXT_MAINMENU:

        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS|CONTEXT_REMOTE:
        default:
            return button_context_standard;
    }
    return button_context_standard;
}

