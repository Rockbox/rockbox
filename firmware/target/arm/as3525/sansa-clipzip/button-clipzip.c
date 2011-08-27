/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#include "config.h"

#include "button-target.h"
#include "as3525v2.h"
#include "kernel.h"
#include "system-target.h"

void button_init_device(void)
{
    /* GPIO A6, A7 and D6 are direct button inputs */
    GPIOA_DIR &= ~(1 << 6);
    GPIOA_DIR &= ~(1 << 7);
    GPIOD_DIR &= ~(1 << 6);
    
    /* GPIO C1, C2, C3, C4, C5 are used in a column/row key scan matrix */
    GPIOC_DIR |= ((1 << 1) | (1 << 2));
    GPIOC_DIR &= ~((1 << 3) | (1 << 4) | (1 << 5));
}

/*  TODO:
    Instead of using udelay to wait for buttons to settle, we could use a
    simple state machine to alternate between key matrix rows (like we do on
    the clip) and this way avoid burning cycles in the udelay.
    
    TODO:
    Figure out the real mappings from GPIOs to buttons.
    The current mapping is just an educated guess.
*/
int button_read_device(void)
{
    int buttons = 0;

    /* power */
    if (GPIOD_PIN(6)) {
        buttons |= BUTTON_POWER;
    }

    /* volume */
    if (GPIOA_PIN(6)) {
        buttons |= BUTTON_VOL_DOWN;
    }
    if (GPIOA_PIN(7)) {
        buttons |= BUTTON_VOL_UP;
    }

    /* key matrix buttons, first row */
    GPIOC_PIN(1) = (1 << 1);
    GPIOC_PIN(2) = 0;
    udelay(500);
    
    if (GPIOC_PIN(3)) {
        buttons |= BUTTON_LEFT;
    }
    if (GPIOC_PIN(4)) {
        buttons |= BUTTON_SELECT;
    }
    if (GPIOC_PIN(5)) {
        buttons |= BUTTON_RIGHT;
    }
    
    /* key matrix buttons, second row */
    GPIOC_PIN(1) = 0;
    GPIOC_PIN(2) = (1 << 2);
    udelay(500);
    
    if (GPIOC_PIN(3)) {
        buttons |= BUTTON_UP;
    }
    if (GPIOC_PIN(4)) {
        buttons |= BUTTON_HOME;
    }
    if (GPIOC_PIN(5)) {
        buttons |= BUTTON_DOWN;
    }
    
    /* deselect scan rows */
    GPIOC_PIN(2) = 0;

    return buttons;
}

