/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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

/* Button Code Definitions for rk27xx reference design target */

#include "config.h"
#include "action.h"
#include "button.h"

/* 
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code } 
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping 
 */
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,       BUTTON_REW,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_FF,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_CONTEXT,    BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_STD_CANCEL,     BUTTON_VOL,                 BUTTON_NONE },
    { ACTION_STD_OK,         BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_STD_MENU,       BUTTON_M,                   BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,       BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY },
    { ACTION_WPS_SKIPNEXT,   BUTTON_FF|BUTTON_REL,       BUTTON_FF   },
    { ACTION_WPS_SKIPPREV,   BUTTON_REW|BUTTON_REL,      BUTTON_REW  },
    { ACTION_WPS_SEEKBACK,   BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,    BUTTON_FF|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,   BUTTON_REW|BUTTON_REL,      BUTTON_REW|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,   BUTTON_FF|BUTTON_REL,       BUTTON_FF|BUTTON_REPEAT },
    { ACTION_WPS_STOP,       BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY },
    { ACTION_WPS_VOLDOWN,    BUTTON_REW|BUTTON_VOL,      BUTTON_NONE  },
    { ACTION_WPS_VOLDOWN,    BUTTON_REW|BUTTON_VOL|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_WPS_VOLUP,      BUTTON_FF|BUTTON_VOL,       BUTTON_NONE  },
    { ACTION_WPS_VOLUP,      BUTTON_FF|BUTTON_VOL|BUTTON_REPEAT,    BUTTON_NONE },

    { ACTION_WPS_BROWSE,     BUTTON_VOL|BUTTON_REL,      BUTTON_VOL },
    { ACTION_WPS_MENU,       BUTTON_M|BUTTON_REL,        BUTTON_M   },
    { ACTION_WPS_CONTEXT,    BUTTON_M|BUTTON_REPEAT,     BUTTON_M   },
    { ACTION_STD_KEYLOCK,    BUTTON_M|BUTTON_VOL,        BUTTON_NONE },

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
        case CONTEXT_SETTINGS|CONTEXT_REMOTE:
        default:
            return button_context_standard;
    } 
    return button_context_standard;
}
