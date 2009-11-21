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
 * Copyright (C) 2008 Rafaël Carré
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
#include "as3525.h"
#ifndef BOOTLOADER
#include "backlight.h"
#endif

/*  The Sansa Clip uses a button matrix that is scanned by selecting one of
    three rows and reading back the button states from the columns.

    In this driver, the row is changed at each call (i.e. once per tick).
    In one tick, column data from one row is read back and then the next row
    is selected for the following tick. This mechanism ensures that there is
    plenty time between selecting a row and reading the columns, avoiding the
    need for explicit delays.
*/


void button_init_device(void)
{
    GPIOA_DIR &= ~((1<<7) | (1<<3));
    GPIOB_DIR &= ~((1<<2) | (1<<1) | (1<<0));
    GPIOC_PIN(4) = 0;
    GPIOC_PIN(5) = 0;
    GPIOC_PIN(6) = 0;
    GPIOC_DIR |= ((1<<6) | (1<<5) | (1<<4));
    
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

    /* This is a keypad using C4-C6 as columns and B0-B2 as rows */
    switch (row) {

    case 0:
        buttons &= ~(BUTTON_VOL_UP | BUTTON_UP);

        (void)GPIOB_PIN(0); /* C4B0 is unused */

        if (GPIOB_PIN(1))
            buttons |= BUTTON_VOL_UP;

        if (GPIOB_PIN(2))
            buttons |= BUTTON_UP;

        GPIOC_PIN(4) = 0;
        GPIOC_PIN(5) = (1<<5);
        row++;
        break;

    case 1:
        buttons &= ~(BUTTON_LEFT | BUTTON_SELECT | BUTTON_RIGHT);

        if (GPIOB_PIN(0))
            buttons |= BUTTON_LEFT;

        if (GPIOB_PIN(1))
            buttons |= BUTTON_SELECT;

        if (GPIOB_PIN(2))
            buttons |= BUTTON_RIGHT;

        GPIOC_PIN(5) = 0;
        GPIOC_PIN(6) = (1<<6);
        row++;
        break;

    case 2:
        buttons &= ~(BUTTON_DOWN | BUTTON_VOL_DOWN | BUTTON_HOME);

        if (GPIOB_PIN(0))
            buttons |= BUTTON_DOWN;

        if (GPIOB_PIN(1))
            buttons |= BUTTON_VOL_DOWN;

        if (GPIOB_PIN(2))
            buttons |= BUTTON_HOME;

        GPIOC_PIN(6) = 0;
        GPIOC_PIN(4) = (1<<4);

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
