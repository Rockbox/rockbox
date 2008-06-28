/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2007 by Mark Arigo
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
#include "backlight.h"

void button_init_device(void)
{
    /* TODO...for now, hardware initialisation is done by the c200 bootloader */
}

bool button_hold(void)
{
    return !(GPIOI_INPUT_VAL & 0x80);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    static bool hold_button = false;
    bool hold_button_old;

    /* Hold */
    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    /* device buttons */
    if (!hold_button)
    {
        if ( (GPIOB_INPUT_VAL & 0x20)) btn |= BUTTON_POWER;
        if (!(GPIOG_INPUT_VAL & 0x10)) btn |= BUTTON_UP;
        if (!(GPIOH_INPUT_VAL & 0x01)) btn |= BUTTON_DOWN;
        if (!(GPIOI_INPUT_VAL & 0x04)) btn |= BUTTON_LEFT;
        if (!(GPIOG_INPUT_VAL & 0x02)) btn |= BUTTON_RIGHT;
        if (!(GPIOL_INPUT_VAL & 0x04)) btn |= BUTTON_SELECT;
        if (!(GPIOG_INPUT_VAL & 0x01)) btn |= BUTTON_REC;
        if (!(GPIOL_INPUT_VAL & 0x10)) btn |= BUTTON_VOL_UP;
        if (!(GPIOL_INPUT_VAL & 0x20)) btn |= BUTTON_VOL_DOWN;
    }

    return btn;
}
