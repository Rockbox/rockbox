/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Barry Wardell 2006
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

/* Button Code Definitions for iriver H10 target */

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
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_SCROLL_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_SCROLL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_SCROLL_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_SCROLL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_MENU,        BUTTON_POWER|BUTTON_REL,           BUTTON_POWER },
    { ACTION_STD_OK,          BUTTON_RIGHT|BUTTON_REL,           BUTTON_RIGHT },
    { ACTION_STD_CONTEXT,     BUTTON_RIGHT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_LEFT|BUTTON_REL,            BUTTON_LEFT },
    { ACTION_STD_QUICKSCREEN, BUTTON_LEFT|BUTTON_REPEAT,         BUTTON_NONE },
    
    /* TODO: this is a bit of a hack so that we can exit some debug screens
     * (audio, tagcache, dircache, hwinfo, stack, ports). They don't like it
     * when ACTION_STD_CANCEL has anything other than BUTTON_NONE for prereq.
     */
    { ACTION_STD_CANCEL,      BUTTON_POWER|BUTTON_RIGHT,         BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping remote_button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* remote_button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,     BUTTON_PLAY|BUTTON_REL,   BUTTON_PLAY },
    { ACTION_WPS_STOP,     BUTTON_PLAY|BUTTON_REPEAT,BUTTON_PLAY },
    { ACTION_WPS_SKIPPREV, BUTTON_REW|BUTTON_REL,    BUTTON_REW},
    { ACTION_WPS_SEEKBACK, BUTTON_REW|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_REW|BUTTON_REL,    BUTTON_REW|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT, BUTTON_FF|BUTTON_REL,     BUTTON_FF },
    { ACTION_WPS_SEEKFWD,  BUTTON_FF|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_FF|BUTTON_REL,     BUTTON_FF|BUTTON_REPEAT },
    
    { ACTION_WPS_ABSETB_NEXTDIR,    BUTTON_POWER|BUTTON_FF,     BUTTON_POWER },
    { ACTION_WPS_ABSETA_PREVDIR,    BUTTON_POWER|BUTTON_REW,    BUTTON_POWER },
    { ACTION_WPS_ABRESET,           BUTTON_POWER|BUTTON_PLAY,   BUTTON_POWER },
    
    { ACTION_WPS_VOLDOWN, BUTTON_SCROLL_DOWN,              BUTTON_NONE },
    { ACTION_WPS_VOLDOWN, BUTTON_SCROLL_DOWN|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_WPS_VOLUP,   BUTTON_SCROLL_UP,                BUTTON_NONE },
    { ACTION_WPS_VOLUP,   BUTTON_SCROLL_UP|BUTTON_REPEAT,  BUTTON_NONE },
    
    { ACTION_WPS_BROWSE,        BUTTON_LEFT|BUTTON_REL,       BUTTON_LEFT },
    { ACTION_WPS_CONTEXT,       BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_RIGHT },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_LEFT },
    { ACTION_WPS_MENU,          BUTTON_POWER|BUTTON_REL,      BUTTON_POWER },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_PLAY|BUTTON_LEFT,      BUTTON_PLAY },
    { ACTION_WPS_ID3SCREEN,     BUTTON_PLAY|BUTTON_RIGHT,     BUTTON_PLAY },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping remote_button_context_wps[]  = {
    { ACTION_WPS_PLAY,     BUTTON_RC_PLAY|BUTTON_REL,   BUTTON_RC_PLAY },
    { ACTION_WPS_STOP,     BUTTON_RC_PLAY|BUTTON_REPEAT,BUTTON_RC_PLAY },
    { ACTION_WPS_SKIPPREV, BUTTON_RC_REW|BUTTON_REL,    BUTTON_RC_REW},
    { ACTION_WPS_SEEKBACK, BUTTON_RC_REW|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_RC_REW|BUTTON_REL,    BUTTON_RC_REW|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT, BUTTON_RC_FF|BUTTON_REL,     BUTTON_RC_FF },
    { ACTION_WPS_SEEKFWD,  BUTTON_RC_FF|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_RC_FF|BUTTON_REL,     BUTTON_RC_FF|BUTTON_REPEAT },
    
    { ACTION_WPS_VOLDOWN, BUTTON_RC_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_WPS_VOLDOWN, BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_VOLUP,   BUTTON_RC_VOL_UP,                    BUTTON_NONE },
    { ACTION_WPS_VOLUP,   BUTTON_RC_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_WPS_PITCHSCREEN,   BUTTON_RC_PLAY|BUTTON_LEFT,    BUTTON_RC_PLAY },
    { ACTION_WPS_ID3SCREEN,     BUTTON_RC_PLAY|BUTTON_RIGHT,   BUTTON_RC_PLAY },
    
    LAST_ITEM_IN_LIST
}; /* remote_button_context_wps */

static const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,      BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_PREV,          BUTTON_FF,                        BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_FF|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_REW,                       BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_REW|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_SETTINGS_RESET,    BUTTON_PLAY,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings */

static const struct button_mapping button_context_list[]  = {
    { ACTION_LISTTREE_PGUP,         BUTTON_REW|BUTTON_REL,       BUTTON_REW },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_FF|BUTTON_REL,        BUTTON_FF  },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping remote_button_context_list[]  = {
    { ACTION_LISTTREE_PGUP,         BUTTON_RC_REW|BUTTON_REL,   BUTTON_RC_REW },
    { ACTION_LISTTREE_PGDOWN,       BUTTON_RC_FF|BUTTON_REL,    BUTTON_RC_FF  },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_TREE_STOP,   BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_tree */

static const struct button_mapping remote_button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_RC_PLAY|BUTTON_REL,     BUTTON_RC_PLAY },
    { ACTION_TREE_STOP,   BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_RC_PLAY },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_TREE_ROOT_INIT,    BUTTON_REW|BUTTON_REPEAT,     BUTTON_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_FF|BUTTON_REPEAT,      BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_without_combo */

static const struct button_mapping remote_button_context_listtree_scroll_without_combo[]  = {
    { ACTION_TREE_ROOT_INIT,    BUTTON_RC_REW|BUTTON_REPEAT,  BUTTON_RC_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_REPEAT,  BUTTON_NONE   },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_REPEAT,   BUTTON_NONE   },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_without_combo */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_LISTTREE_PGUP,     BUTTON_REW|BUTTON_REPEAT,     BUTTON_REW },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_FF|BUTTON_REPEAT,      BUTTON_FF  },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_PLAY,       BUTTON_PLAY },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REL,        BUTTON_REW|BUTTON_PLAY },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_PLAY,       BUTTON_REW|BUTTON_REL },
    { ACTION_TREE_ROOT_INIT,    BUTTON_REW|BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_REW|BUTTON_PLAY },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_FF|BUTTON_PLAY,        BUTTON_PLAY },
    { ACTION_TREE_PGRIGHT,      BUTTON_FF|BUTTON_REL,         BUTTON_FF|BUTTON_PLAY },
    { ACTION_TREE_PGRIGHT,      BUTTON_FF|BUTTON_PLAY,        BUTTON_FF|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_FF|BUTTON_PLAY|BUTTON_REPEAT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_with_combo */

static const struct button_mapping remote_button_context_listtree_scroll_with_combo[]  = {
    { ACTION_LISTTREE_PGUP,     BUTTON_RC_REW|BUTTON_REPEAT,                 BUTTON_RC_REW },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RC_FF|BUTTON_REPEAT,                  BUTTON_RC_FF  },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_RC_PLAY,                BUTTON_RC_PLAY },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_REL,                    BUTTON_RC_REW|BUTTON_RC_PLAY },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_RC_PLAY,                BUTTON_RC_REW|BUTTON_REL },
    { ACTION_TREE_ROOT_INIT,    BUTTON_RC_REW|BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_RC_REW|BUTTON_RC_PLAY },
    { ACTION_TREE_PGLEFT,       BUTTON_RC_REW|BUTTON_RC_PLAY|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_RC_PLAY,                 BUTTON_RC_PLAY },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_REL,                     BUTTON_RC_FF|BUTTON_RC_PLAY },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_RC_PLAY,                 BUTTON_RC_FF|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_RC_FF|BUTTON_RC_PLAY|BUTTON_REPEAT,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_with_combo */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_RIGHT,              BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV, BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_QS_DOWNINV, BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_REW,                       BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_REW|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_FF,                        BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_FF|BUTTON_REPEAT,          BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_quickscreen */

static const struct button_mapping remote_button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV, BUTTON_RC_VOL_UP,                    BUTTON_NONE },
    { ACTION_QS_DOWNINV, BUTTON_RC_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_RC_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_RC_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_RC_REW,                       BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_RC_REW|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_RC_FF,                        BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_RC_FF|BUTTON_REPEAT,          BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_quickscreen */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_SCROLL_UP,                BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_SCROLL_UP|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_SCROLL_DOWN,              BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_SCROLL_DOWN|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_REW,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,     BUTTON_REW|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_FF,                       BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,     BUTTON_FF|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_NONE,               BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_STD_OK,             BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_NONE,               BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_LEFT,                     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_right_is_inc */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_REW,                       BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_REW|BUTTON_REL,            BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_FF,                        BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_FF|BUTTON_REL,             BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_POWER,                     BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_PLAY,                      BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_LEFT,                      BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_pitchscreen */

static const struct button_mapping remote_button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_RC_VOL_UP,                 BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_RC_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_RC_VOL_DOWN,               BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_RC_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_RC_REW,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_RC_REW|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RC_FF,                     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RC_FF|BUTTON_REL,          BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_RC_PLAY,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_pitchscreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                      BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                     BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_REW,                       BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_PLAY|BUTTON_REL,           BUTTON_PLAY },
    { ACTION_KBD_ABORT,        BUTTON_POWER,                     BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_REW|BUTTON_PLAY,           BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_REW|BUTTON_REL,            BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,       BUTTON_REW,       BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,             BUTTON_PLAY,                BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_recscreen */

static const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_PRESET,      BUTTON_RIGHT | BUTTON_REL,             BUTTON_RIGHT },
    { ACTION_FM_MENU,        BUTTON_RIGHT | BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_FM_MODE,        BUTTON_LEFT,                           BUTTON_NONE },
    { ACTION_FM_STOP,        BUTTON_PLAY | BUTTON_REPEAT,           BUTTON_PLAY },
    { ACTION_FM_EXIT,        BUTTON_POWER,                          BUTTON_NONE },
    { ACTION_FM_PLAY,        BUTTON_PLAY | BUTTON_REL,              BUTTON_PLAY },
    { ACTION_SETTINGS_INC,       BUTTON_SCROLL_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_SCROLL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_SCROLL_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_SCROLL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,       BUTTON_REW,                            BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_REW|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_FF,                             BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_FF|BUTTON_REPEAT,               BUTTON_NONE },

    LAST_ITEM_IN_LIST
    
};

static const struct button_mapping* get_context_mapping_remote( int context )
{
    context ^= CONTEXT_REMOTE;
    
    switch (context)
    {
        case CONTEXT_STD:
        case CONTEXT_MAINMENU:
            return remote_button_context_standard;
            
        case CONTEXT_WPS:
            return remote_button_context_wps;

        case CONTEXT_LIST:
            return remote_button_context_list;
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return remote_button_context_listtree_scroll_without_combo;
            else 
                return remote_button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return remote_button_context_tree;
            
        case CONTEXT_QUICKSCREEN:
            return remote_button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return remote_button_context_pitchscreen;
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;

        default:
            return remote_button_context_standard;
    }
}

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    if (context&CONTEXT_REMOTE)
        return get_context_mapping_remote(context);

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
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS_RECTRIGGER:
            return button_context_settings_right_is_inc;
            
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
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
        case CONTEXT_FM:
            return button_context_radio;
        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
