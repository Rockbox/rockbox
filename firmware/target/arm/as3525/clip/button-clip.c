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
#include "button-target.h"
#include "as3525.h"

void button_init_device(void)
{
    GPIOA_DIR &= ~((1<<7) | (1<<3));
    GPIOB_DIR &= ~((1<<2) | (1<<1) | (1<<0));
    GPIOC_PIN(4) = 0x00;
    GPIOC_PIN(5) = 0x00;
    GPIOC_PIN(6) = 0x00;
    GPIOC_DIR |= ((1<<6) | (1<<5) | (1<<4));
}

int button_read_device(void)
{
    int result = 0;

    /* direct GPIO connections */

    if (GPIOA_PIN(7))
        result |= BUTTON_POWER;

    if (GPIOA_PIN(3))
        result |= BUTTON_HOLD;

    /* This is a keypad using C4-C6 as columns and B0-B2 as rows */
    GPIOC_PIN(4) = (1<<4);

    /* C4B0 is unused */

    if (GPIOB_PIN(1))
        result |= BUTTON_VOLUP;

    if (GPIOB_PIN(2))
        result |= BUTTON_PLAY;

    GPIOC_PIN(4) = 0x00;

    GPIOC_PIN(5) = (1<<5);

    if (GPIOB_PIN(0))
        result |= BUTTON_LEFT;

    if (GPIOB_PIN(1))
        result |= BUTTON_SELECT;

    if (GPIOB_PIN(2))
        result |= BUTTON_RIGHT;

    GPIOC_PIN(5) = 0x00;

    GPIOC_PIN(6) = (1<<6);

    if (GPIOB_PIN(0))
        result |= BUTTON_DOWN;

    if (GPIOB_PIN(1))
        result |= BUTTON_VOLDOWN;

    if (GPIOB_PIN(2))
        result |= BUTTON_HOME;

    GPIOC_PIN(6) = 0x00;

    return result;
}

bool button_hold(void)
{
    return (GPIOA_PIN(3) != 0);
}
