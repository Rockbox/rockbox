/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#include "system.h"
#include "button.h"
#include "backlight.h"

/*
 * TODO: Scrollwheel!
 */
 
#ifdef HAS_BUTTON_HOLD
static bool hold_button = false;
#endif
void button_init_device(void)
{
}

/*
 * Get button pressed from hardware
 */

int button_read_device(void)
{
    int btn = 0;
    volatile int delay;
    static bool hold_button_old = false;
    static long power_counter = 0;
    unsigned gpiod = GPIOD_DATA;
    unsigned gpioa_dir = GPIOA_DIR;
    unsigned gpiod6;
    for(delay = 500; delay; delay--) nop;
    CCU_IO &= ~(1<<12);
    for(delay=8;delay;delay--) nop;
    GPIOB_DIR |= 1<<3;
    GPIOB_PIN(3) = 1<<3;
    GPIOC_DIR = 0;
    GPIOB_DIR &= ~(1<<1);
    GPIOB_DIR |= 1<<0;
    GPIOB_PIN(0) = 1;
    for(delay = 500; delay; delay--)
        nop;
    gpiod6 = GPIOD_PIN(6);
    GPIOB_PIN(0) = 0;
    for(delay = 240; delay; delay--)
        nop;
    GPIOD_DIR = 0xff;
    GPIOA_DIR &= ~(1<<6|1<<7);
    GPIOD_DATA = 0;
    GPIOD_DIR = 0;
    if (GPIOC_PIN(1) & 1<<1)
        btn |= BUTTON_DOWN;
    if (GPIOC_PIN(2) & 1<<2)
        btn |= BUTTON_UP;
    if (GPIOC_PIN(3) & 1<<3)
        btn |= BUTTON_LEFT;
    if (GPIOC_PIN(4) & 1<<4)
        btn |= BUTTON_SELECT;
    if (GPIOC_PIN(5) & 1<<5)
        btn |= BUTTON_RIGHT;
    if (GPIOB_PIN(1) & 1<<1)
        btn |= BUTTON_HOME;
    if (gpiod6 & 1<<6)
    {   /* power/hold is on the same pin. we know it's hold if the bit isn't
         * set now anymore */
        if (GPIOD_PIN(6) & 1<<6)
        {
            hold_button = false;
            btn |= BUTTON_POWER;
        }
        else
        {
            hold_button = true;
        }
    }

    GPIOD_DIR = 0xff;
    GPIOD_DATA = gpiod;
    GPIOA_DIR = gpioa_dir;
    GPIOD_DIR = 0;
    CCU_IO |= 1<<12;
#ifdef HAS_BUTTON_HOLD
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
    if (hold_button)
    {
        power_counter = HZ;
        return 0;
    }
    /* read power, but not if hold button was just released, since
     * you basically always hit power due to the slider mechanism after releasing
     * (fuze only)
     */
    else if (power_counter > 0)
    {
        power_counter--;
        btn &= ~BUTTON_POWER;
    }
#endif
    return btn;
}

#ifdef HAS_BUTTON_HOLD
bool button_hold(void)
{
    return hold_button;
}
#endif
