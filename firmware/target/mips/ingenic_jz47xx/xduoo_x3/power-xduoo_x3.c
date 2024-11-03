/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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
#include "power.h"
#include "cpu.h"

#define CHARGE_STAT_GPIO (32*1+6)  /* STAT port */
#define PIN_USB_DET      (32*4+19) /* USB connected */

/* Detect which power sources are present. */
unsigned int power_input_status(void)
{
    int rval = POWER_INPUT_NONE;
    if(!__gpio_get_pin(PIN_USB_DET))
        rval |= POWER_INPUT_USB;
    if(!__gpio_get_pin(CHARGE_STAT_GPIO))
	rval |= POWER_INPUT_USB_CHARGER;

    return rval;
}

void power_init(void)
{
    __gpio_as_input(CHARGE_STAT_GPIO);
    __gpio_disable_pull(CHARGE_STAT_GPIO);
}

bool charging_state(void)
{
    return (power_input_status() & POWER_INPUT_USB_CHARGER);
}
