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

/* TODO: the YP-CP3's power-hold line is not known yet. The rk27xx generic
 * board holds power on PC0; driving a pin whose job on this board is unknown
 * could do anything, so nothing is touched. Whatever the boot ROM or the
 * original firmware left holding power stays holding it, and power_off()
 * cannot switch the player off. */
void power_init(void)
{
}

void power_off(void)
{
    disable_irq();
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
