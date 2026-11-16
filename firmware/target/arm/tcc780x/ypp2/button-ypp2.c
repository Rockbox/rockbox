/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "kernel.h"
#include "button.h"
#include "touchscreen.h"
#ifndef BOOTLOADER
#include "touch-target.h"
static int last_touch_data;
#endif

void button_init_device(void)
{
    GPIOA_DIR &= ~((1u << 20) | (1u << 19)); /* VOL+ / VOL- */
    GPIOC_DIR &= ~((1u << 31) | (1u << 20)); /* HOLD / POWER */

#ifndef BOOTLOADER
    last_touch_data = 0;
    /* The ClearPad worker owns the touch bus and publishes cached input. */
    ypp2_touch_init();
#endif
}

bool button_hold(void)
{
    /* HOLD is active low on GPIOC31. */
    return (GPIOC & (1u << 31)) == 0;
}

int button_read_device(int *data)
{
    int btn = BUTTON_NONE;
    *data = 0;

    if (button_hold())
        return BUTTON_NONE;

    if (GPIOA & (1u << 20))
        btn |= BUTTON_PLUS;
    if (GPIOA & (1u << 19))
        btn |= BUTTON_MINUS;
    /* POWER is PWR_IN_CPU / EINT2 on GPIOC20. */
    if (GPIOC & (1u << 20))
        btn |= BUTTON_POWER;

#ifndef BOOTLOADER
    int x, y;
    if (ypp2_touch_read(&x, &y))
    {
        btn |= touchscreen_to_pixels(x, y, data);
        last_touch_data = *data;
    }
    /* Supply the last contact position for the common button tick's
     * BUTTON_REL event. ClearPad release packets carry zero coordinates. */
    *data = last_touch_data;
#endif

    return btn;
}
