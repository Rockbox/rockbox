/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: keymap-h1x0_h3x0.c 17847 2008-06-28 18:10:04Z bagder $
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

/* Button Code Definitions for samsung yh-820 / yh-920 / yh-925 target */
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

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|CONTEXT_TREE = the standard list/tree defines (without directions)
CONTEXT_CUSTOM|CONTEXT_SETTINGS = the direction keys for the eq/col picker screens 
                                  i.e where up/down is inc/dec
               CONTEXT_SETTINGS = up/down is prev/next, l/r is inc/dec

*/


static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_PREV,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_CONTEXT,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_RIGHT },
    /* kludge: pressing 2 directional buttons is easy on this target  */
    { ACTION_STD_QUICKSCREEN,   BUTTON_LEFT|BUTTON_DOWN|BUTTON_REPEAT,
                                        BUTTON_LEFT|BUTTON_DOWN|BUTTON_REPEAT },

    { ACTION_STD_REC,           BUTTON_REC|BUTTON_REPEAT,       BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,          BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_WPS_STOP,          BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },

    { ACTION_WPS_SKIPNEXT,      BUTTON_FFWD|BUTTON_REL,         BUTTON_FFWD },
    { ACTION_WPS_SEEKFWD,       BUTTON_FFWD|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_FFWD|BUTTON_REL,         BUTTON_FFWD|BUTTON_REPEAT },

    { ACTION_WPS_SKIPPREV,      BUTTON_REW|BUTTON_REL,          BUTTON_REW },
    { ACTION_WPS_SEEKBACK,      BUTTON_REW|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_REW|BUTTON_REL,          BUTTON_REW|BUTTON_REPEAT },

    { ACTION_WPS_ABSETB_NEXTDIR,BUTTON_PLAY|BUTTON_RIGHT,       BUTTON_PLAY },
    { ACTION_WPS_ABSETA_PREVDIR,BUTTON_PLAY|BUTTON_LEFT,        BUTTON_PLAY },
    { ACTION_WPS_ABRESET,       BUTTON_PLAY|BUTTON_UP,          BUTTON_PLAY },

    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REL,         BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP,                      BUTTON_NONE },

    { ACTION_WPS_BROWSE,        BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },

    /* these match context_standard */
    { ACTION_WPS_MENU,          BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_CONTEXT,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_RIGHT },
    /* kludge: pressing 2 directional buttons is easy on this target  */
    { ACTION_WPS_QUICKSCREEN,   BUTTON_LEFT|BUTTON_DOWN|BUTTON_REPEAT,
                                        BUTTON_LEFT|BUTTON_DOWN|BUTTON_REPEAT },

    { ACTION_WPS_HOTKEY,        BUTTON_REC|BUTTON_REL,          BUTTON_REC },
    { ACTION_WPS_VIEW_PLAYLIST, BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_LEFT },
    { ACTION_WPS_REC,           BUTTON_REC|BUTTON_REPEAT,       BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */
   
static const struct button_mapping button_context_list[]  = {
#ifdef SAMSUNG_YH820
    { ACTION_LISTTREE_PGUP,     BUTTON_REC|BUTTON_UP,           BUTTON_REC },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_REC|BUTTON_DOWN,         BUTTON_REC },
#else
    { ACTION_LISTTREE_PGUP,     BUTTON_FFWD,                    BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_REW,                     BUTTON_NONE },
#endif

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,          BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_TREE_STOP,         BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },
    { ACTION_TREE_HOTKEY,       BUTTON_REC|BUTTON_REL,          BUTTON_REC },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_TREE_PGLEFT,       BUTTON_REC|BUTTON_LEFT,         BUTTON_REC },
    { ACTION_TREE_PGLEFT,       BUTTON_REC|BUTTON_LEFT|BUTTON_REPEAT,BUTTON_NONE },

    { ACTION_TREE_PGRIGHT,      BUTTON_REC|BUTTON_RIGHT,        BUTTON_REC },
    { ACTION_TREE_PGRIGHT,      BUTTON_REC|BUTTON_RIGHT|BUTTON_REPEAT,BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE)
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_LEFT },

    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REL,         BUTTON_REW|BUTTON_REPEAT },
    
    { ACTION_TREE_PGRIGHT,      BUTTON_FFWD|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_FFWD|BUTTON_REL,        BUTTON_FFWD|BUTTON_REPEAT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE)
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,      BUTTON_UP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_SETTINGS_DEC,      BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_NONE,              BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_STD_PREV,          BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_NEXT,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_SETTINGS_RESET,    BUTTON_PLAY,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_SETTINGS_DEC,      BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_PREV,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_STD_NEXT,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_SETTINGS_RESET,    BUTTON_PLAY,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,      BUTTON_RIGHT,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    { ACTION_STD_OK,            BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS)
}; /* button_context_settings_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    { ACTION_STD_OK,            BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS)
}; /* button_context_settings_context_eq */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,        BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_settings_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_STD_OK,            BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_STD_CANCEL,        BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_settings_bmark */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_NONE,              BUTTON_LEFT|BUTTON_DOWN|BUTTON_REL,BUTTON_NONE },

    { ACTION_QS_TOP,            BUTTON_UP|BUTTON_REL,           BUTTON_UP },
    { ACTION_QS_TOP,            BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_QS_DOWN,           BUTTON_DOWN|BUTTON_REL,         BUTTON_DOWN },
    { ACTION_QS_DOWN,           BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_QS_LEFT,           BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_QS_LEFT,           BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_QS_RIGHT,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_QS_RIGHT,          BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },

    { ACTION_STD_CANCEL,        BUTTON_PLAY,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_FFWD,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                      BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },

    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },

    { ACTION_PS_TOGGLE_MODE,    BUTTON_FFWD,                    BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_REW,                     BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_PLAY,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,             BUTTON_PLAY|BUTTON_REL,     BUTTON_NONE },
#ifdef SAMSUNG_YH820
    /* the yh-820 has a rec button */
    { ACTION_REC_NEWFILE,           BUTTON_REC,                 BUTTON_NONE },
#else
    /* the yh-920 & yh-925 have a rec switch */
    { ACTION_REC_NEWFILE,           BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_NONE },
#endif

    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },

    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_UP,           BUTTON_UP,                       BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,         BUTTON_NONE },

    { ACTION_KBD_DOWN,         BUTTON_DOWN,                     BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,       BUTTON_NONE },

    { ACTION_KBD_LEFT,         BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },

    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_KBD_CURSOR_LEFT,  BUTTON_REW|BUTTON_REL,           BUTTON_FFWD },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_REW|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_KBD_CURSOR_RIGHT, BUTTON_FFWD|BUTTON_REL,          BUTTON_FFWD },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_FFWD|BUTTON_REPEAT,       BUTTON_NONE },

    { ACTION_KBD_SELECT,       BUTTON_PLAY|BUTTON_REL,          BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_PLAY|BUTTON_RIGHT,        BUTTON_NONE },

    { ACTION_KBD_DONE,         BUTTON_PLAY|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_FFWD|BUTTON_PLAY,         BUTTON_FFWD },
    { ACTION_KBD_ABORT,        BUTTON_FFWD|BUTTON_REW,          BUTTON_FFWD },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

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
        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
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
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
    } 
    return button_context_standard;
}
