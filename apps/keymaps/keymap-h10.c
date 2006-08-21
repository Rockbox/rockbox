/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) Barry Wardell 2006
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_SCROLL_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_SCROLL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_SCROLL_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_SCROLL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    
    { ACTION_STD_CONTEXT,     BUTTON_FF,                         BUTTON_NONE },    
    { ACTION_STD_CANCEL,      BUTTON_LEFT,                       BUTTON_NONE },
    { ACTION_STD_OK,          BUTTON_RIGHT,                      BUTTON_NONE },
    { ACTION_STD_MENU,        BUTTON_REW|BUTTON_REL,             BUTTON_REW },
    { ACTION_STD_QUICKSCREEN, BUTTON_REW|BUTTON_REPEAT,          BUTTON_REW },
    { ACTION_STD_CANCEL,      BUTTON_POWER,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,     BUTTON_PLAY|BUTTON_REL,   BUTTON_PLAY },
    { ACTION_WPS_STOP,     BUTTON_PLAY|BUTTON_REPEAT,BUTTON_PLAY },
    { ACTION_WPS_SKIPPREV, BUTTON_REW|BUTTON_REL,    BUTTON_REW},
    { ACTION_WPS_SEEKBACK, BUTTON_REW|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_REW|BUTTON_REL,    BUTTON_REW|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT, BUTTON_FF|BUTTON_REL,     BUTTON_FF },
    { ACTION_WPS_SEEKFWD,  BUTTON_FF|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_STOPSEEK, BUTTON_FF|BUTTON_REL,     BUTTON_FF|BUTTON_REPEAT },
    
    { ACTION_WPS_VOLDOWN, BUTTON_SCROLL_DOWN,              BUTTON_NONE },
    { ACTION_WPS_VOLDOWN, BUTTON_SCROLL_DOWN|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_WPS_VOLUP,   BUTTON_SCROLL_UP,                BUTTON_NONE },
    { ACTION_WPS_VOLUP,   BUTTON_SCROLL_UP|BUTTON_REPEAT,  BUTTON_NONE },
    
    { ACTION_WPS_BROWSE,        BUTTON_LEFT|BUTTON_REL,       BUTTON_LEFT },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_LEFT },
    { ACTION_WPS_CONTEXT,       BUTTON_RIGHT|BUTTON_REL,      BUTTON_RIGHT },
    { ACTION_WPS_MENU,          BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_RIGHT },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_PLAY|BUTTON_LEFT,      BUTTON_PLAY },
    { ACTION_WPS_ID3SCREEN,     BUTTON_PLAY|BUTTON_RIGHT,     BUTTON_PLAY },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,      BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_PREV,          BUTTON_LEFT,                      BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_POWER,                     BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_settings */

const struct button_mapping button_context_tree[]  = {
    { ACTION_NONE,        BUTTON_PLAY,                BUTTON_NONE },
    { ACTION_TREE_WPS,    BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_TREE_STOP,   BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    
    LAST_ITEM_IN_LIST
}; /* button_context_tree */

const struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_tree_scroll_lr */

const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_RIGHT,              BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_settings_yesno */

const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV, BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_QS_DOWNINV, BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_LEFT,                      BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_LEFT|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_RIGHT,                     BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_RIGHT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_CANCEL, BUTTON_POWER,                     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_quickscreen */

const struct button_mapping button_context_settings_r_is_inc[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_SCROLL_UP,                BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,     BUTTON_SCROLL_UP|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_SCROLL_DOWN,              BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,     BUTTON_SCROLL_DOWN|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_STD_OK,             BUTTON_PLAY,                     BUTTON_NONE },
    { ACTION_STD_CANCEL,         BUTTON_POWER,                    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_settings_r_is_inc */

const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_SCROLL_UP,                 BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_SCROLL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_SCROLL_DOWN,               BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_SCROLL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                      BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,           BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,          BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_PLAY,                      BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_POWER,                     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_pitchscreen */



/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
            
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_SETTINGS_EQ:
        case CONTEXT_SETTINGS_TIME:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
  	        return button_context_settings_r_is_inc;
  	    case CONTEXT_SETTINGS:
            return button_context_settings;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
            
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_tree_scroll_lr;
            /* else fall through to CUSTOM|1 */
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_LIST:
        case CONTEXT_MAINMENU:
        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
