/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

/* Button Code Definitions for the toshiba gigabeat target */
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

/* copied from Meizu M6SP keymap */
static const struct button_mapping button_context_standard[]  = {
#if 0 /* disabled for now, there is no BUTTON_UP/DOWN yet */
    { ACTION_STD_PREV,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
#endif 
    { ACTION_STD_CANCEL,        BUTTON_LEFT,                BUTTON_NONE },

    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,BUTTON_SELECT },

    { ACTION_STD_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU },
    { ACTION_STD_MENU,          BUTTON_MENU|BUTTON_REL,     BUTTON_MENU },

    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,   BUTTON_SELECT },
    { ACTION_STD_OK,            BUTTON_RIGHT,               BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */


const struct button_mapping* get_context_mapping(int context)
{
    (void)context;

    /* TODO add more button contexts */
    return button_context_standard;
}
