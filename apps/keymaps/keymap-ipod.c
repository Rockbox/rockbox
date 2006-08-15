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

/* Button Code Definitions for ipod target */

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

CONTEXT_CUSTOM|1 = the standard list/tree defines (without directions)


*/

struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,          BUTTON_SCROLL_BACK,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_SCROLL_BACK },
    { ACTION_STD_NEXT,          BUTTON_SCROLL_FWD,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_SCROLL_FWD },
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                        BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT,                       BUTTON_NONE },

    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    { ACTION_STD_MENU,          BUTTON_MENU|BUTTON_REL,             BUTTON_MENU },
    { ACTION_STD_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,          BUTTON_MENU },
    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT },
    { ACTION_STD_CANCEL,        BUTTON_PLAY|BUTTON_REPEAT,          BUTTON_PLAY },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */
struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,          BUTTON_PLAY|BUTTON_REL,      BUTTON_PLAY },
    { ACTION_TREE_STOP,         BUTTON_PLAY|BUTTON_REPEAT,   BUTTON_PLAY },
    
    LAST_ITEM_IN_LIST
}; /* button_context_tree */

struct button_mapping button_context_tree_scroll_lr[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|1),
};

struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,      BUTTON_PLAY|BUTTON_REL,         BUTTON_PLAY },
    { ACTION_WPS_STOP,      BUTTON_PLAY|BUTTON_REPEAT,      BUTTON_PLAY },
    { ACTION_WPS_SKIPPREV,  BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,  BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_LEFT },
    { ACTION_WPS_STOPSEEK,  BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_SKIPNEXT,  BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SEEKFWD,   BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_RIGHT },
    { ACTION_WPS_STOPSEEK,  BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_VOLDOWN,   BUTTON_SCROLL_BACK,                 BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_SCROLL_BACK|BUTTON_REPEAT,   BUTTON_SCROLL_BACK },
    { ACTION_WPS_VOLUP,     BUTTON_SCROLL_FWD,                  BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_SCROLL_FWD|BUTTON_REPEAT,    BUTTON_SCROLL_FWD },
    { ACTION_WPS_BROWSE,    BUTTON_SELECT|BUTTON_REL,           BUTTON_SELECT },
    { ACTION_WPS_CONTEXT,   BUTTON_SELECT|BUTTON_REPEAT,        BUTTON_SELECT },
    { ACTION_WPS_MENU,          BUTTON_MENU|BUTTON_REL,         BUTTON_MENU },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,      BUTTON_MENU },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_SCROLL_FWD,         BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_SCROLL_FWD|BUTTON_REPEAT,  BUTTON_SCROLL_FWD },
    { ACTION_SETTINGS_DEC,          BUTTON_SCROLL_BACK,          BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_SCROLL_BACK|BUTTON_REPEAT,  BUTTON_SCROLL_BACK },
    { ACTION_STD_PREV,              BUTTON_LEFT,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_LEFT|BUTTON_REPEAT,    BUTTON_LEFT },
    { ACTION_STD_NEXT,              BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_RIGHT },
    { ACTION_STD_CANCEL,            BUTTON_MENU|BUTTON_REL,      BUTTON_MENU }, /* rel so bmark screen works */
    
    LAST_ITEM_IN_LIST
}; /* button_context_settings */

struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_PLAY,                  BUTTON_NONE },
    LAST_ITEM_IN_LIST
}; /* button_context_settings_yesno */

struct button_mapping button_context_bmark[]  = {
    { ACTION_BMARK_DELETE,          BUTTON_MENU|BUTTON_REPEAT,       BUTTON_MENU },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_settings_bmark */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;
            
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_tree_scroll_lr;
            /* else fall through to CUSTOM|1 */
        case CONTEXT_CUSTOM|1:
            return button_context_tree;
            
        case CONTEXT_LIST:
        case CONTEXT_MAINMENU:
            break;
        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGSGRAPHICAL:
            return button_context_settings;
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
