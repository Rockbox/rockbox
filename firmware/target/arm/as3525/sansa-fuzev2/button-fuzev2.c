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

extern void scrollwheel(unsigned wheel_value);

#ifdef HAS_BUTTON_HOLD
static bool hold_button = false;
#endif
void button_init_device(void)
{   /* activate the wheel */
    volatile int i;
    GPIOA_DIR &= ~(1<<6|1<<7);
    GPIOC_DIR = 0;
    GPIOB_DIR |= (1<<4)|(1<<3)|(1<<0);
    for(i = 20; i; i--) nop;
    GPIOB_PIN(4) = 1<<4;
}

unsigned read_GPIOA_67(void)
{
    unsigned ret = 0;
    volatile int i;
    DBOP_CTRL |= 1<<19;
    for(i = 20; i; i--) nop;
    GPIOA_DIR &= ~0xc0;
    for(i = 20; i; i--) nop;
    if (GPIOA_PIN(6) != 0)
        ret = 1<<0;
    for(i = 20; i; i--) nop;
    if (GPIOA_PIN(7) != 0)
        ret |= 1<<1;
    DBOP_CTRL &= ~(1<<19);
    for(i = 20; i; i--) nop;
    return ret;
}

void get_scrollwheel(void)
{
#if defined(HAVE_SCROLLWHEEL) && !defined(BOOTLOADER)
    /* scroll wheel handling */
    scrollwheel(read_GPIOA_67());
#endif
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
    unsigned gpiod6;

    get_scrollwheel();

    CCU_IO &= ~(1<<12);
    for(delay=8;delay;delay--) nop;

    GPIOB_PIN(3) = 1<<3;
    GPIOB_PIN(0) = 1<<0;

    for(delay = 500; delay; delay--)
        nop;
    gpiod6 = GPIOD_PIN(6);
    GPIOB_PIN(0) = 0;
    for(delay = 240; delay; delay--)
        nop;

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

    CCU_IO |= 1<<12;

#ifdef HAS_BUTTON_HOLD
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#else
    (void)hold_button_old;
#endif
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
