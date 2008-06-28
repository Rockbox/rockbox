/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

/* Custom written for the TPJ-1022 based on analysis of the GPIO data */

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"

void button_init_device(void)
{
    /* No hardware initialisation required as it is done by the bootloader */
}

bool button_hold(void)
{
  return false;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    unsigned char state;
    static bool hold_button = false;

#if 0
    /* light handling */
    if (hold_button && !button_hold())
    {
        backlight_on();
    }
#endif

    hold_button = button_hold();
    if (!hold_button)
    {
        /* Read normal buttons */
        state = GPIOA_INPUT_VAL;
        if ((state & 0x2) == 0) btn |= BUTTON_REW;
        if ((state & 0x4) == 0) btn |= BUTTON_FF;
        if ((state & 0x80) == 0) btn |= BUTTON_RIGHT;

        /* Buttons left to figure out:
           button_hold()
           BUTTON_POWER
           BUTTON_LEFT
           BUTTON_UP
           BUTTON_DOWN
           BUTTON_MENU
           BUTTON_REC  
           BUTTON_AB
           BUTTON_PLUS
           BUTTON_MINUS
        */
    }
    
    return btn;
}
