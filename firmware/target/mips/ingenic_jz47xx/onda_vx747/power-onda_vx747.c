/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include "jz4740.h"

/* TQ7051 chip */
#define CHARGE_STAT_GPIO (32*1+30) /* STAT port */
#define USB_CHARGER_GPIO (32*3+28)

#if CONFIG_CHARGING
/* Detect which power sources are present. */
unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    if (__gpio_get_pin(USB_CHARGER_GPIO))
        status |= POWER_INPUT_USB_CHARGER;

    if(!__gpio_get_pin(CHARGE_STAT_GPIO))
        status |= POWER_INPUT_USB;

    return status;
}
#endif

void power_init(void)
{
    __gpio_as_input(USB_CHARGER_GPIO);
    __gpio_as_input(CHARGE_STAT_GPIO);
}

bool charging_state(void)
{
    return power_input_status() & POWER_INPUT_USB;
}

#if CONFIG_TUNER
static bool tuner_on = false;
bool tuner_power(bool status)
{
    if (status != tuner_on)
    {
        tuner_on = status;
        status = !status;
    }

    return status;    
}

bool tuner_powered(void)
{
    return tuner_on;
}
#endif
