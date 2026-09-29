/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#include <stdbool.h>
#include "config.h"
#include "power.h"
#include "system.h"
#include "usb.h"

/* Power is held on by GPIO C0, active high - as on the rk27xx generic
 * board. The original firmware raises it at boot and, to switch off,
 * drives it low and waits for the supply to go. */
#define POWER_HOLD_PIN      (1 << 0)    /* GPIO C0 */

void power_init(void)
{
    GPIO_PCDR |= POWER_HOLD_PIN;
    GPIO_PCCON |= POWER_HOLD_PIN;       /* output */
}

void power_off(void)
{
    disable_irq();
    GPIO_PCCON |= POWER_HOLD_PIN;
    GPIO_PCDR &= ~POWER_HOLD_PIN;
    while (1)
    {
    }
}

unsigned int power_input_status(void)
{
    return (usb_detect() == USB_INSERTED) ? POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return usb_detect() == USB_INSERTED;
}
