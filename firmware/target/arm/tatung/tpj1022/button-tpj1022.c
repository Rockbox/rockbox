/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Robert Kukla
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

#include "system.h"
#include "button.h"

bool button_hold(void)
{
    return (GPIOK_INPUT_VAL & 0x40) ?  true : false; 
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;

    if (!button_hold())
    {
        btn = (GPIOA_INPUT_VAL & 0xfe) ^ 0xfe;

        if ((GPIOK_INPUT_VAL & 0x20) == 0) btn |= BUTTON_VOL_DOWN;

        /* to be found
        if ((GPIO?_INPUT_VAL & 0x??) == 0) btn |= BUTTON_MENU;
        if ((GPIO?_INPUT_VAL & 0x??) == 0) btn |= BUTTON_REC;
        if ((GPIO?_INPUT_VAL & 0x??) == 0) btn |= BUTTON_VOL_UP;
        if ((GPIO?_INPUT_VAL & 0x??) == 0) btn |= BUTTON_LEFT;
         */  
    }
    
    return btn;
}
