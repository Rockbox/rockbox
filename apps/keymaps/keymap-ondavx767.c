/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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

/* Button Code Definitions for the Onda VX767 target */
/* This needs from someone with the actual target! */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/* The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 *
 * CAVEAT: This will allways return the action without
 * pre_button_code (pre_button_code = BUTTON_NONE)
 * if it is found before 'falling through'
 * to a lower 'chained' context.
 *
 * Example: button = UP|REL, last_button = UP;
 *  while looking in CONTEXT_WPS there is an action defined
 *  {ACTION_FOO, BUTTON_UP|BUTTON_REL, BUTTON_NONE}
 *  then ACTION_FOO in CONTEXT_WPS will be returned
 *  EVEN THOUGH you are expecting a fully matched
 *  ACTION_BAR from CONTEXT_STD
 *  {ACTION_BAR, BUTTON_UP|BUTTON_REL, BUTTON_UP}
 *
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
*/

static const struct button_mapping button_context_standard[] = {


    LAST_ITEM_IN_LIST
}; /* button_context_standard */

static const struct button_mapping button_context_wps[] = {


    LAST_ITEM_IN_LIST
}; /* button_context_wps */

/* get_context_mapping returns a pointer to one of the above defined arrays depending on the context */
const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:      { return button_context_standard; }
        case CONTEXT_WPS:      { return button_context_wps; }

        case CONTEXT_LIST:
        case CONTEXT_SETTINGS:
        case CONTEXT_SETTINGS|CONTEXT_REMOTE:

        case CONTEXT_TREE:
        case CONTEXT_MAINMENU:

        default:               { return button_context_standard; }
    }
    return button_context_standard;
}
