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
#include "button-target.h"
#include "as3525v2.h"
#include "kernel.h"

/* FIXME : use Clipv1 like driver (FS#10285) */

void button_init_device(void)
{
    GPIOA_DIR &= ~((1<<7) | (1<<3));
    GPIOD_DIR &= ~((1<<2) | (1<<1) | (1<<0));
    GPIOD_PIN(3) = 1<<3;
    GPIOD_PIN(4) = 1<<4;
    GPIOD_PIN(5) = 1<<5;
    GPIOD_DIR |= ((1<<5) | (1<<4) | (1<<3));
}

int button_read_device(void)
{
    int result = 0;
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
        result |= BUTTON_POWER;

    /* This is a keypad using D3-D5 as columns and D0-D2 as rows */

    GPIOD_PIN(3) = 0x00; /* activate D3 */
    asm volatile ("nop\nnop\nnop\nnop\nnop\n"); /* wait a bit for reliable results */

    /* D3D0 is unused */

    if (!GPIOD_PIN(1))
        result |= BUTTON_VOL_UP;

    if (!GPIOD_PIN(2))
        result |= BUTTON_UP;

    GPIOD_PIN(3) = 1<<3;

    GPIOD_PIN(4) = 0x00; /* activate D4 */
    asm volatile ("nop\nnop\nnop\nnop\nnop\n");

    if (!GPIOD_PIN(0))
        result |= BUTTON_LEFT;

    if (!GPIOD_PIN(1))
        result |= BUTTON_SELECT;

    if (!GPIOD_PIN(2))
        result |= BUTTON_RIGHT;

    GPIOD_PIN(4) = 1<<4;

    GPIOD_PIN(5) = 0x00; /* activate D5 */
    asm volatile ("nop\nnop\nnop\nnop\nnop\n");

    if (!GPIOD_PIN(0))
        result |= BUTTON_DOWN;

    if (!GPIOD_PIN(1))
        result |= BUTTON_VOL_DOWN;

    if (!GPIOD_PIN(2))
        result |= BUTTON_HOME;

    GPIOD_PIN(5) = 1<<5;

    return result;
}

bool button_hold(void)
{
    return (GPIOA_PIN(3) != 0);
}
