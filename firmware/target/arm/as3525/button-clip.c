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
#include "button.h"
#include "as3525v2.h"
#ifndef BOOTLOADER
#include "backlight.h"
#endif

#if   defined(SANSA_CLIP)
# define OUT_PIN GPIOC_PIN
# define OUT_DIR GPIOC_DIR
# define INITIAL 0
# define IN_PIN  GPIOB_PIN
# define IN_DIR  GPIOB_DIR
#elif defined(SANSA_CLIPV2)
# define OUT_PIN GPIOD_PIN
# define OUT_DIR GPIOD_DIR
# define INITIAL 1
# define IN_PIN  GPIOD_PIN
# define IN_DIR  GPIOD_DIR
#endif

static const int rows[3] = {
#if   defined(SANSA_CLIP)
    4, 5, 6
#elif defined(SANSA_CLIPV2)
    3, 4, 5
#endif
};

void button_init_device(void)
{
    GPIOA_DIR &= ~((1<<7) | (1<<3));
    CCU_IO &= ~(3<<2);
    IN_DIR &= ~((1<<2) | (1<<1) | (1<<0));

    for (int i = 0; i < 3; i++) {
        OUT_PIN(rows[i]) = INITIAL << rows[i];
        OUT_DIR |= 1 << rows[i];
    }

    /* get initial readings */
    button_read_device();
    button_read_device();
    button_read_device();
}

int button_read_device(void)
{
    static int row, buttons;
    static unsigned power_counter;

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

    static const int matrix [3][3] = {
        { 0 /*unused*/, BUTTON_VOL_UP,   BUTTON_UP },
        { BUTTON_LEFT,  BUTTON_SELECT,   BUTTON_RIGHT },
        { BUTTON_DOWN,  BUTTON_VOL_DOWN, BUTTON_HOME },
    };

    for (int i = 0; i<3; i++)
        if (IN_PIN(i) ^ (INITIAL << i))
            buttons |=  matrix[row][i];
        else
            buttons &= ~matrix[row][i];

    /* prepare next row */
    OUT_PIN(rows[row]) = INITIAL << rows[row];
    row++;
    row %= 3;
    OUT_PIN(rows[row]) = (!INITIAL) << rows[row];

    return buttons;
}

bool button_hold(void)
{
#ifdef SANSA_CLIPV2
    GPIOA_DIR |= 1<<7;
    GPIOA_PIN(7) = 1<<7;

    int delay = 50;
    while(delay--)
        asm("nop");
#endif

    bool hold_button = (GPIOA_PIN(3) != 0);

#ifdef SANSA_CLIPV2
    GPIOA_PIN(7) = 0;
    GPIOA_DIR &= ~(1<<7);
#endif

#ifndef BOOTLOADER
    static bool hold_button_old = false;
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    return hold_button;
}
