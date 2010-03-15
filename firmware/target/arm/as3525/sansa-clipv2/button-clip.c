/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Bertrik Sikken
 * Copyright (C) 2008 François Dinel
 * Copyright ©   2008-2009 Rafaël Carré
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
#include "button-target.h"
#include "as3525v2.h"
#ifndef BOOTLOADER
#include "backlight.h"
#endif

void button_init_device(void)
{
    GPIOA_DIR &= ~((1<<7) | (1<<3));
    GPIOD_DIR &= ~((1<<2) | (1<<1) | (1<<0));
    GPIOD_PIN(3) = 1<<3;
    GPIOD_PIN(4) = 1<<4;
    GPIOD_PIN(5) = 1<<5;
    GPIOD_DIR |= ((1<<5) | (1<<4) | (1<<3));

    /* get initial readings */
    button_read_device();
    button_read_device();
    button_read_device();
}

int button_read_device(void)
{
    static int row = 0;
    static int buttons = 0;
    static unsigned power_counter = 0;

    if(button_hold())
    {
        power_counter = HZ;
        return 0;
    }

    /* direct GPIO connections */
    /* read power, but not if hold button was just released, since
     * you basically always hit power due to the slider mechanism after
     * releasing hold (wait 1 sec) */
    if (power_counter)
        power_counter--;

    if (GPIOA_PIN(7) && !power_counter)
        buttons |= BUTTON_POWER;
    else
        buttons &= ~BUTTON_POWER;

    /* This is a keypad using D3-D5 as columns and D0-D2 as rows */
    switch (row) {

    case 0:
        buttons &= ~(BUTTON_VOL_UP | BUTTON_UP);

        (void)GPIOD_PIN(0); /* D3D0 is unused */

        if (!GPIOD_PIN(1))
            buttons |= BUTTON_VOL_UP;

        if (!GPIOD_PIN(2))
            buttons |= BUTTON_UP;

        GPIOD_PIN(3) = 1<<3;
        GPIOD_PIN(4) = 0x00;
        row++;
        break;

    case 1:
        buttons &= ~(BUTTON_LEFT | BUTTON_SELECT | BUTTON_RIGHT);

        if (!GPIOD_PIN(0))
            buttons |= BUTTON_LEFT;

        if (!GPIOD_PIN(1))
            buttons |= BUTTON_SELECT;

        if (!GPIOD_PIN(2))
            buttons |= BUTTON_RIGHT;

        GPIOD_PIN(4) = 1<<4;
        GPIOD_PIN(5) = 0x00;
        row++;
        break;

    case 2:
        buttons &= ~(BUTTON_DOWN | BUTTON_VOL_DOWN | BUTTON_HOME);

        if (!GPIOD_PIN(0))
            buttons |= BUTTON_DOWN;

        if (!GPIOD_PIN(1))
            buttons |= BUTTON_VOL_DOWN;

        if (!GPIOD_PIN(2))
            buttons |= BUTTON_HOME;

        GPIOD_PIN(5) = 1<<5;
        GPIOD_PIN(3) = 0x00;

    default:
        row = 0;
        break;
    }

    return buttons;
}

bool button_hold(void)
{
#ifndef BOOTLOADER
    static bool hold_button_old = false;
#endif
    bool hold_button = (GPIOA_PIN(3) != 0);

#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    return hold_button;
}
