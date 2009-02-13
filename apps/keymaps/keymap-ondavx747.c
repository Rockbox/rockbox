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

/* Button Code Definitions for the Onda VX747 target */
/* NB: Up/Down/Left/Right are not physical buttons - touchscreen emulation */

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

/*TODO*/
static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_VOL_DOWN,               BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_VOL_DOWN|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_VOL_UP,                 BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },

    { ACTION_STD_OK,          BUTTON_MENU|BUTTON_REL,        BUTTON_MENU },
    { ACTION_STD_CANCEL,      BUTTON_POWER,                  BUTTON_NONE },
    
    { ACTION_STD_QUICKSCREEN, BUTTON_VOL_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_CONTEXT,     BUTTON_MENU|BUTTON_REPEAT,     BUTTON_NONE },
    
    LAST_ITEM_IN_LIST
}; /* button_context_standard */

const struct button_mapping* target_get_context_mapping(int context)
{
    (void)context;
    return button_context_standard;
}
