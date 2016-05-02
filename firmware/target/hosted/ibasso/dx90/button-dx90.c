/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include "button.h"

#include "button-ibasso.h"


int handle_button_event(__u16 code, __s32 value, int last_btns)
{
    int button = BUTTON_NONE;

    switch(code)
    {
        case EVENT_CODE_BUTTON_PWR:
        {
            button = BUTTON_POWER;
            break;
        }

        case EVENT_CODE_BUTTON_PWR_LONG:
        {
            button = BUTTON_POWER_LONG;
            break;
        }

        case EVENT_CODE_BUTTON_VOLPLUS:
        {
            button = BUTTON_VOL_UP;
            break;
        }

        case EVENT_CODE_BUTTON_VOLMINUS:
        {
            button = BUTTON_VOL_DOWN;
            break;
        }

        case EVENT_CODE_BUTTON_REV:
        {
            button = BUTTON_LEFT;
            break;
        }

        case EVENT_CODE_BUTTON_PLAY:
        {
            button = BUTTON_PLAY;
            break;
        }

        case EVENT_CODE_BUTTON_NEXT:
        {
            button = BUTTON_RIGHT;
            break;
        }

        default:
        {
            return BUTTON_NONE;
        }
    }

    if(button == BUTTON_RIGHT && ((last_btns & BUTTON_LEFT) == BUTTON_LEFT))
    {
        /* Workaround for a wrong feedback, only present with DX90: the kernel
         * sometimes report right press in the middle of a [left press, left release]
         * interval, which is clearly wrong. */
        button = BUTTON_LEFT;
    }

    int buttons = last_btns;
    if(value == EVENT_VALUE_BUTTON_PRESS)
    {
        buttons = (last_btns | button);
    }
    else
    {
        buttons = (last_btns & (~button));
    }

    return buttons;
}
