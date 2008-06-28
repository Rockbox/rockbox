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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* *
 * Button Code Definitions for archos player targets
 *
 */

#include "config.h"
#include "action.h"
#include "button.h"

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_LEFT,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_LEFT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_STD_CONTEXT,    BUTTON_PLAY|BUTTON_REPEAT, BUTTON_PLAY },
    { ACTION_STD_CANCEL,     BUTTON_STOP,               BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_ON,                 BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_PLAY|BUTTON_REL,    BUTTON_PLAY },
    { ACTION_STD_MENU,       BUTTON_MENU,               BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,      BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_WPS_SKIPNEXT,  BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,  BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT },
    { ACTION_WPS_SEEKBACK,  BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,   BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,  BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,  BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_STOP,      BUTTON_STOP,                BUTTON_NONE },
    
    { ACTION_WPS_VOLDOWN,   BUTTON_MENU|BUTTON_LEFT,    BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,   BUTTON_MENU|BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_MENU|BUTTON_RIGHT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_MENU|BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    
    { ACTION_WPS_BROWSE,    BUTTON_ON|BUTTON_REL,       BUTTON_ON },
    { ACTION_WPS_MENU,      BUTTON_MENU|BUTTON_REL,     BUTTON_MENU },
    { ACTION_WPS_CONTEXT,   BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_WPS_ID3SCREEN, BUTTON_MENU|BUTTON_ON,      BUTTON_NONE },
    { ACTION_STD_KEYLOCK,   BUTTON_MENU|BUTTON_STOP,    BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

static const struct button_mapping button_context_settings[] = {
    { ACTION_SETTINGS_INC,          BUTTON_RIGHT,                 BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_RIGHT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,      BUTTON_ON,     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_listtree */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,  BUTTON_PLAY,   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,      BUTTON_PLAY|BUTTON_ON,   BUTTON_PLAY },
    { ACTION_BMS_DELETE,      BUTTON_PLAY|BUTTON_ON,   BUTTON_ON },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_settings_bmark */

/*****************************************************************************
 *    Remote control mappings
 *****************************************************************************/

static const struct button_mapping remote_button_context_standard[]  = {
    { ACTION_STD_PREV,      BUTTON_RC_LEFT,     BUTTON_NONE },
    { ACTION_STD_NEXT,      BUTTON_RC_RIGHT,    BUTTON_NONE },
    { ACTION_STD_CANCEL,    BUTTON_RC_STOP,     BUTTON_NONE },
    { ACTION_STD_OK,        BUTTON_RC_PLAY,     BUTTON_NONE },

    LAST_ITEM_IN_LIST
};

static const struct button_mapping remote_button_context_wps[]  = {
    { ACTION_WPS_PLAY,      BUTTON_RC_PLAY,     BUTTON_NONE },
    { ACTION_WPS_SKIPNEXT,  BUTTON_RC_RIGHT,    BUTTON_NONE },
    { ACTION_WPS_SKIPPREV,  BUTTON_RC_LEFT,     BUTTON_NONE },
    { ACTION_WPS_STOP,      BUTTON_RC_STOP,     BUTTON_NONE },
    
    { ACTION_WPS_VOLDOWN,   BUTTON_RC_VOL_DOWN, BUTTON_NONE },
    { ACTION_WPS_VOLUP,     BUTTON_RC_VOL_UP,   BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
};


static const struct button_mapping* get_context_mapping_remote( int context )
{
    context ^= CONTEXT_REMOTE;
    
    switch (context)
    {
        case CONTEXT_WPS:
            return remote_button_context_wps;

        default:
            return remote_button_context_standard;
    }
}

const struct button_mapping* get_context_mapping( int context )
{
    if (context&CONTEXT_REMOTE)
        return get_context_mapping_remote(context);
    
    switch (context)
    {
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_SETTINGS:
            return button_context_settings;
            
        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;

        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:
            return button_context_tree;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_STD:
        case CONTEXT_LIST:
        default:
            return button_context_standard;
    }
}
