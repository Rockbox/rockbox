/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "cpu.h"
#include "button.h"
#include "adc.h"
#include "backlight.h"
#include "touchscreen-target.h"
#include <stdlib.h>

void button_init_device(void)
{
    /* Configure GPIOA 4 (POWER) and 8 (HOLD) for input */
    GPIOA_DIR &= ~0x110;

    /* Configure GPIOB 4 (button pressed) for input */
    GPIOB_DIR &= ~0x10;

    touchscreen_init_device();
}

bool button_hold(void)
{
    return (GPIOA & 0x8) ? false : true;
}

int button_read_device(int *data)
{
    int btn = BUTTON_NONE;
    int adc;
    static int old_data = 0;

    static bool hold_button = false;
#ifndef BOOTLOADER
    bool hold_button_old;
#endif

    *data = old_data;

#ifndef BOOTLOADER
    hold_button_old = hold_button;
#endif
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    if (hold_button)
        return BUTTON_NONE;

    if (GPIOB & 0x4)
    {
        adc = adc_read(ADC_BUTTONS);

        /* The following contains some arbitrary, but working, guesswork */
        if (adc < 0x038) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x048) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS);
        } else if (adc < 0x058) {
            btn |= (BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x070) {
            btn |= BUTTON_PLUS;
        } else if (adc < 0x090) {
            btn |= (BUTTON_MINUS  | BUTTON_MENU);
        } else if (adc < 0x150) {
            btn |= BUTTON_MINUS;
        } else if (adc < 0x200) {
            btn |= BUTTON_MENU;
        }
    }

    btn |= touchscreen_read_device(data, &old_data);

    if (!(GPIOA & 0x4))
        btn |= BUTTON_POWER;

    if (btn & BUTTON_TOUCHSCREEN && !is_backlight_on(true))
        old_data = *data = 0;

    return btn;
}
