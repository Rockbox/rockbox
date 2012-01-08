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
#include "button.h"
#include "as3525v2.h"
#include "system.h"
#include "kernel.h"

static int keyscan(void)
{
    static int buttons, row;
    static const int matrix[2][3] = {
        { BUTTON_RIGHT, BUTTON_SELECT, BUTTON_UP },
        { BUTTON_HOME,  BUTTON_DOWN,   BUTTON_LEFT },
    };

    for (int i = 0; i < 3; i++)
        if (GPIOC_PIN(3 + i))
            buttons |=  matrix[row][i];
        else
            buttons &= ~matrix[row][i];

    /* prepare next row */
    GPIOC_PIN(1) = row << 1;
    row ^= 1;
    GPIOC_PIN(2) = row << 2;

    /* delay a bit if interrupts are disabled, to be sure next row will read */
    if (!irq_enabled())
        for (volatile int i = 0; i < 0x500; i++) ;

    return buttons;
}

void button_init_device(void)
{
    /* GPIO A6, A7 and D6 are direct button inputs */
    GPIOA_DIR &= ~(1 << 6 | 1<< 7);
    GPIOD_DIR &= ~(1 << 6);

    /* GPIO C1, C2, C3, C4, C5 are used in a column/row key scan matrix */
    GPIOC_DIR |= ((1 << 1) | (1 << 2));
    GPIOC_DIR &= ~((1 << 3) | (1 << 4) | (1 << 5));

    /* initial reading */
    button_read_device();
    sleep(1);
    button_read_device();
    sleep(1);
}

int button_read_device(void)
{
    int buttons = 0;

    if (GPIOD_PIN(6))
        buttons |= BUTTON_POWER;
    if (GPIOA_PIN(6))
        buttons |= BUTTON_VOL_DOWN;
    if (GPIOA_PIN(7))
        buttons |= BUTTON_VOL_UP;

    return buttons | keyscan();
}
