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

}

bool button_hold(void)
{
    return (GPIO5_READ & 4) ? false : true;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;
    static bool hold_button = false;
    bool hold_button_old;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);

    if (!button_hold())
    {          
        data = adc_read(ADC_BUTTONS);
        if (data < 0x35c)
        {
            if (data < 0x151)
                if (data < 0xc7)
                    if (data < 0x41)
                        btn = BUTTON_LEFT;
                    else
                        btn = BUTTON_RIGHT;
                else
                    btn = BUTTON_SELECT;
            else
                if (data < 0x268)
                    if (data < 0x1d7)
                        btn = BUTTON_UP;
                    else
                        btn = BUTTON_DOWN;
                else
                    if (data < 0x2f9)
                        btn = BUTTON_EQ;
                    else
                        btn = BUTTON_MODE;
        }

        if (adc_read(ADC_BUTTON_PLAY) < 0x64)
            btn |= BUTTON_PLAY;
    }
    return btn;
}
