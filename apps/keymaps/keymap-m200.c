/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Mark Arigo
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
/* Button Code Definitions for Sandisk c200 target */

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
    { ACTION_STD_PREV,          BUTTON_VOLUP,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_VOLUP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_STD_NEXT,          BUTTON_VOLDOWN,                    BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_VOLDOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_STD_OK,            BUTTON_RIGHT,                   BUTTON_NONE   },
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                    BUTTON_NONE   },

    { ACTION_STD_MENU,          BUTTON_MENU|BUTTON_REL,        BUTTON_MENU  },
    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },
//    { ACTION_STD_QUICKSCREEN,   BUTTON_REC|BUTTON_SELECT,       BUTTON_NONE   },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */


static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,          BUTTON_VOLUP|BUTTON_REL,           BUTTON_VOLUP },
    { ACTION_WPS_STOP,          BUTTON_VOLUP|BUTTON_REPEAT,        BUTTON_VOLUP },
   
    { ACTION_WPS_SKIPPREV,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,      BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },
    
    { ACTION_WPS_SKIPNEXT,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_SEEKFWD,       BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE  },
    { ACTION_WPS_STOPSEEK,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },
    
    { ACTION_WPS_ABSETB_NEXTDIR,BUTTON_MENU|BUTTON_RIGHT,      BUTTON_MENU },
    { ACTION_WPS_ABSETA_PREVDIR,BUTTON_MENU|BUTTON_LEFT,       BUTTON_MENU },
    { ACTION_WPS_ABRESET,       BUTTON_MENU|BUTTON_VOLUP,         BUTTON_MENU },
    
    { ACTION_WPS_MENU,          BUTTON_MENU|BUTTON_REL,        BUTTON_MENU  },
    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },    
    { ACTION_WPS_PITCHSCREEN,   BUTTON_SELECT|BUTTON_VOLUP,        BUTTON_SELECT },
    { ACTION_WPS_ID3SCREEN,     BUTTON_SELECT|BUTTON_VOLDOWN,      BUTTON_SELECT },
    { ACTION_WPS_CONTEXT,       BUTTON_VOLDOWN|BUTTON_REL,         BUTTON_VOLDOWN   },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_VOLDOWN|BUTTON_REPEAT,      BUTTON_VOLDOWN   },
    
    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_settings[] = {
    { ACTION_STD_CANCEL,        BUTTON_MENU,                   BUTTON_NONE },
    { ACTION_SETTINGS_RESET,    BUTTON_SELECT,                  BUTTON_NONE },

    { ACTION_SETTINGS_INC,      BUTTON_VOLUP,                      BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_VOLUP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_SETTINGS_DEC,      BUTTON_VOLDOWN,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_VOLDOWN|BUTTON_REPEAT,      BUTTON_NONE },
   
    { ACTION_STD_PREV,          BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    
    { ACTION_STD_NEXT,          BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings */

static const struct button_mapping button_context_list[]  = {
#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,        BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_LIST_VOLUP,        BUTTON_VOL_UP,                  BUTTON_NONE },

    { ACTION_LIST_VOLDOWN,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,      BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
#endif

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
  //    { ACTION_TREE_WPS,          BUTTON_REC|BUTTON_VOLUP,               BUTTON_REC },
  //    { ACTION_TREE_STOP,         BUTTON_REC|BUTTON_VOLUP|BUTTON_REPEAT, BUTTON_REC|BUTTON_VOLUP },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT },
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_LEFT },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_LEFT|BUTTON_REL,         BUTTON_LEFT|BUTTON_REPEAT },

    { ACTION_NONE,              BUTTON_RIGHT,                   BUTTON_NONE  },
    { ACTION_STD_OK,            BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE  },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT|BUTTON_REPEAT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_without_combo */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_TREE_ROOT_INIT,    BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },
    
//    { ACTION_TREE_PGLEFT,       BUTTON_REC|BUTTON_LEFT,         BUTTON_REC },
//    { ACTION_TREE_PGLEFT,       BUTTON_REC|BUTTON_LEFT|BUTTON_REPEAT, BUTTON_NONE },
    
//    { ACTION_TREE_PGRIGHT,      BUTTON_REC|BUTTON_RIGHT,        BUTTON_REC },
//    { ACTION_TREE_PGRIGHT,      BUTTON_REC|BUTTON_RIGHT|BUTTON_REPEAT,BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
}; /* button_context_listtree_scroll_with_combo */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,      BUTTON_SELECT,                  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_NONE,              BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_MENU|BUTTON_REL,        BUTTON_NONE },
    
    { ACTION_QS_DOWNINV,        BUTTON_VOLUP|BUTTON_REL,           BUTTON_NONE },
    { ACTION_QS_DOWNINV,        BUTTON_VOLUP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_QS_DOWN,           BUTTON_VOLDOWN|BUTTON_REL,         BUTTON_NONE },
    { ACTION_QS_DOWN,           BUTTON_VOLDOWN|BUTTON_REPEAT,      BUTTON_NONE },
    
    { ACTION_QS_LEFT,           BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_QS_LEFT,           BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_QS_RIGHT,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_QS_RIGHT,          BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_quickscreen */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    
    { ACTION_SETTINGS_DEC,      BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_CANCEL,        BUTTON_MENU,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_settings_right_is_inc */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,      BUTTON_VOLUP,                      BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_VOLUP|BUTTON_REPEAT,        BUTTON_NONE },

    { ACTION_PS_DEC_SMALL,      BUTTON_VOLDOWN,                    BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_VOLDOWN|BUTTON_REPEAT,      BUTTON_NONE },
    
    { ACTION_PS_NUDGE_LEFT,     BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_LEFT|BUTTON_REL,         BUTTON_NONE },
    
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_RIGHT|BUTTON_REL,        BUTTON_NONE },
    
    { ACTION_PS_RESET,          BUTTON_SELECT,                  BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_MENU,                   BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD),
}; /* button_context_pitchscreen */

/** Recording Screen **/
#ifdef HAVE_RECORDING
static const struct button_mapping button_context_recscreen[]  = {
    { ACTION_STD_MENU,          BUTTON_MENU|BUTTON_REL,        BUTTON_MENU  },
    { ACTION_REC_PAUSE,         BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_STD_CANCEL,        BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE   },
    { ACTION_REC_NEWFILE,       BUTTON_REC|BUTTON_REL,          BUTTON_REC    },

    { ACTION_SETTINGS_INC,      BUTTON_RIGHT,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_RIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_LEFT,                    BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_LEFT|BUTTON_REPEAT,      BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_recscreen */
#endif

/** FM Radio Screen **/
#if CONFIG_TUNER
static const struct button_mapping button_context_radio[]  = {
    { ACTION_NONE,              BUTTON_VOLUP,                      BUTTON_NONE },
    { ACTION_FM_MENU,           BUTTON_VOLDOWN,                    BUTTON_NONE },
    { ACTION_FM_PRESET,         BUTTON_SELECT,                  BUTTON_NONE },
    { ACTION_FM_STOP,           BUTTON_VOLUP|BUTTON_REPEAT,        BUTTON_VOLUP   },
    { ACTION_FM_MODE,           BUTTON_REC,                     BUTTON_NONE },
    { ACTION_FM_EXIT,           BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },
    { ACTION_FM_PLAY,           BUTTON_VOLUP|BUTTON_REL,           BUTTON_VOLUP   },
    { ACTION_SETTINGS_INC,      BUTTON_VOL_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS)
}; /* button_context_radio */
#endif

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_LEFT,                     BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_LEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT,                    BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_RIGHT|BUTTON_REPEAT,      BUTTON_NONE },
    
//    { ACTION_KBD_CURSOR_LEFT,  BUTTON_REC|BUTTON_LEFT,              BUTTON_NONE },
//    { ACTION_KBD_CURSOR_LEFT,  BUTTON_REC|BUTTON_LEFT|BUTTON_REPEAT,BUTTON_NONE },
//    { ACTION_KBD_CURSOR_RIGHT, BUTTON_REC|BUTTON_RIGHT,              BUTTON_NONE },
//    { ACTION_KBD_CURSOR_RIGHT, BUTTON_REC|BUTTON_RIGHT|BUTTON_REPEAT,BUTTON_NONE },
    
    { ACTION_KBD_UP,           BUTTON_VOLUP,                       BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_VOLUP|BUTTON_REPEAT,         BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_VOLDOWN,                     BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_VOLDOWN|BUTTON_REPEAT,       BUTTON_NONE },
    
//    { ACTION_KBD_BACKSPACE,    BUTTON_REC|BUTTON_VOLDOWN,          BUTTON_NONE },
//    { ACTION_KBD_BACKSPACE,    BUTTON_REC|BUTTON_VOLDOWN|BUTTON_REPEAT,BUTTON_NONE },
    
//    { ACTION_KBD_PAGE_FLIP,    BUTTON_REC|BUTTON_SELECT,        BUTTON_REC },

    { ACTION_KBD_SELECT,       BUTTON_SELECT,                   BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_SELECT|BUTTON_REPEAT,     BUTTON_SELECT },
    { ACTION_KBD_ABORT,        BUTTON_MENU,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

static const struct button_mapping button_context_bmark[]  = {
//    { ACTION_BMS_DELETE,       BUTTON_REC,                       BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
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
        case CONTEXT_SETTINGS_TIME:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS_COLOURCHOOSER:
        case CONTEXT_SETTINGS_EQ:
            return button_context_settings_right_is_inc;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
#if CONFIG_TUNER
        case CONTEXT_FM:
            return button_context_radio;
#endif
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
#ifdef HAVE_RECORDING
        case CONTEXT_RECSCREEN:
            return button_context_recscreen;
#endif
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;

        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
