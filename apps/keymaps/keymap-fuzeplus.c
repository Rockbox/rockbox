/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © Amaury Pouly 2011
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

/* {Action Code,    Button code,    Prereq button code } */

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_LEFT,                BUTTON_NONE },
    { ACTION_STD_PREV,       BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    
    { ACTION_STD_NEXT,       BUTTON_RIGHT,                BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_DOWN,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_DOWN|BUTTON_REPEAT,   BUTTON_NONE },
    
    { ACTION_STD_CONTEXT,    BUTTON_SELECT|BUTTON_REPEAT, BUTTON_SELECT },
    { ACTION_STD_CANCEL,     BUTTON_BACK,                 BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_SELECT|BUTTON_REL,    BUTTON_SELECT },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,       BUTTON_PLAYPAUSE|BUTTON_REL, BUTTON_PLAYPAUSE },
    { ACTION_WPS_SKIPNEXT,   BUTTON_RIGHT|BUTTON_REL,     BUTTON_RIGHT },
    { ACTION_WPS_SKIPPREV,   BUTTON_LEFT|BUTTON_REL,      BUTTON_LEFT  },
    { ACTION_WPS_SEEKBACK,   BUTTON_LEFT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,    BUTTON_RIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,   BUTTON_LEFT|BUTTON_REL,      BUTTON_LEFT|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,   BUTTON_RIGHT|BUTTON_REL,     BUTTON_RIGHT|BUTTON_REPEAT },
    { ACTION_WPS_STOP,       BUTTON_PLAYPAUSE|BUTTON_REPEAT, BUTTON_PLAYPAUSE },
    { ACTION_WPS_VOLDOWN,    BUTTON_VOL_DOWN,             BUTTON_NONE  },
    { ACTION_WPS_VOLDOWN,    BUTTON_VOL_DOWN|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_WPS_VOLUP,      BUTTON_VOL_UP,               BUTTON_NONE  },
    { ACTION_WPS_VOLUP,      BUTTON_VOL_UP|BUTTON_REPEAT, BUTTON_NONE },

    { ACTION_WPS_MENU,       BUTTON_SELECT|BUTTON_REL,    BUTTON_SELECT   },
    { ACTION_WPS_CONTEXT,    BUTTON_SELECT|BUTTON_REPEAT, BUTTON_SELECT   },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */



/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
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
        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
