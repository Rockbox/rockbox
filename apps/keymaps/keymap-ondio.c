/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* *
 * Button Code Definitions for archos ondio fm/sp targets
 */

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|CONTEXT_TREE = the standard list/tree defines (without directions)


*/

const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_CONTEXT,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT },
    { ACTION_STD_CANCEL,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_STD_MENU,       BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU },
    { ACTION_STD_CANCEL,     BUTTON_OFF,                 BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,       BUTTON_OFF|BUTTON_REL,      BUTTON_OFF },
    { ACTION_WPS_SKIPNEXT,   BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,   BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,   BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,    BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,   BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,   BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_STOP,       BUTTON_OFF|BUTTON_REPEAT,   BUTTON_OFF },
    { ACTION_WPS_VOLDOWN,    BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_VOLUP,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_WPS_VOLUP,      BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_BROWSE,     BUTTON_MENU|BUTTON_REL,     BUTTON_MENU },
    { ACTION_WPS_CONTEXT,    BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU },
    /* { ACTION_WPS_MENU,       BUTTON_NONE,                BUTTON_NONE }, we can't have that */
    { ACTION_STD_KEYLOCK,   BUTTON_MENU|BUTTON_DOWN,      BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_OK,                BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,                BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,            BUTTON_MENU,                BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};
const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_MENU|BUTTON_REL,       BUTTON_MENU },
    { ACTION_TREE_STOP,   BUTTON_OFF,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_listtree */
const struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_MENU|BUTTON_LEFT, BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_MENU|BUTTON_LEFT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_MENU|BUTTON_RIGHT|BUTTON_REL, BUTTON_MENU|BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_MENU|BUTTON_RIGHT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_MENU|BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_MENU|BUTTON_RIGHT|BUTTON_REL, BUTTON_MENU|BUTTON_RIGHT|BUTTON_REPEAT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

const struct button_mapping button_context_yesno[] = {
    { ACTION_YESNO_ACCEPT,      BUTTON_RIGHT,    BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; 

struct button_mapping button_context_bmark[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                 BUTTON_NONE },
    { ACTION_BMS_DELETE,      BUTTON_LEFT|BUTTON_REPEAT,   BUTTON_LEFT },
    { ACTION_BMS_EXIT,        BUTTON_LEFT|BUTTON_REL,      BUTTON_LEFT },
    { ACTION_BMS_SELECT,       BUTTON_RIGHT,   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_bmark */

const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_MENU,                BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_OFF,                 BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

const struct button_mapping button_context_recscreen[]  = {
    { ACTION_REC_PAUSE,             BUTTON_MENU|BUTTON_REL,     BUTTON_MENU },  
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */

const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                           BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,             BUTTON_NONE },   
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                          BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,            BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_MENU|BUTTON_REL,                BUTTON_MENU },
    { ACTION_KBD_DONE,         BUTTON_MENU|BUTTON_REPEAT,             BUTTON_NONE },
    { ACTION_KBD_ABORT,        BUTTON_OFF,                            BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                             BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                           BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,             BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

const struct button_mapping button_context_radio[]  = {
    { ACTION_FM_MENU,          BUTTON_MENU | BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_FM_RECORD_DBLPRE, BUTTON_MENU,                           BUTTON_NONE},
    { ACTION_FM_RECORD,        BUTTON_MENU | BUTTON_REL,              BUTTON_NONE },
    { ACTION_FM_STOP,          BUTTON_OFF | BUTTON_REL,               BUTTON_OFF },
    { ACTION_FM_EXIT,          BUTTON_OFF | BUTTON_REPEAT,            BUTTON_OFF },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
    
};

const struct button_mapping* get_context_mapping( int context )
{
    switch( context )
    {
        case CONTEXT_STD:
            return button_context_standard;
            
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_SETTINGS:
            return button_context_settings;
            
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
            
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_TREE:     
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_tree_scroll_lr;
            /* else fall through to CUSTOM|CONTEXT_TREE */
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;

        case CONTEXT_LIST:
        case CONTEXT_MAINMENU:
        default:
            return button_context_standard;
    }
}
