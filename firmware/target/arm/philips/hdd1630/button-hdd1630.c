/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

/* Remember last buttons, to make single buzz sound */
int btn_old;

/*
 * Generate a click sound from the player (not in headphones yet)
 * TODO: integrate this with the "key click" option
 */
void button_click(void)
{
    GPO32_ENABLE |= 0x2000;
    GPIOD_OUTPUT_VAL |= 0x8;
    udelay(1000);
    GPO32_VAL &= ~0x2000;
}

void button_init_device(void)
{
    /* TODO...for now, hardware initialisation is done by the bootloader */
}

bool button_hold(void)
{
    return !(GPIOJ_INPUT_VAL & 0x8);
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

   /* device buttons */
    if (!hold_button)
    {
        /* These are the correct button definitions
        if (!(GPIOA_INPUT_VAL & 0x01)) btn |= BUTTON_MENU;
        if (!(GPIOA_INPUT_VAL & 0x02)) btn |= BUTTON_VOL_UP;
        if (!(GPIOA_INPUT_VAL & 0x04)) btn |= BUTTON_VOL_DOWN;
        if (!(GPIOA_INPUT_VAL & 0x08)) btn |= BUTTON_VIEW;
        if (!(GPIOD_INPUT_VAL & 0x20)) btn |= BUTTON_PLAYLIST;
        if (!(GPIOD_INPUT_VAL & 0x40)) btn |= BUTTON_POWER;
        */

        /* This is a hack until the touchpad works */
        if (!(GPIOA_INPUT_VAL & 0x01)) btn |= BUTTON_LEFT;   /* BUTTON_MENU */
        if (!(GPIOA_INPUT_VAL & 0x02)) btn |= BUTTON_UP;     /* BUTTON_VOL_UP */
        if (!(GPIOA_INPUT_VAL & 0x04)) btn |= BUTTON_DOWN;   /* BUTTON_VOL_DOWN */
        if (!(GPIOA_INPUT_VAL & 0x08)) btn |= BUTTON_RIGHT;  /* BUTTON_VIEW */
        if (!(GPIOD_INPUT_VAL & 0x20)) btn |= BUTTON_SELECT; /* BUTTON_PLAYLIST */
        if (!(GPIOD_INPUT_VAL & 0x40)) btn |= BUTTON_POWER;
    }

    if ((btn != btn_old) && (btn != BUTTON_NONE))
        button_click();

    btn_old = btn;

    return btn;
}

bool headphones_inserted(void)
{
    return (GPIOE_INPUT_VAL & 0x80) ? true : false;
}
