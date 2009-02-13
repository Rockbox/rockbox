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
#define UNK_GPIO (32*1+30) /* STAT port */
#define USB_CHARGER_GPIO (32*3+28)

/* Detect which power sources are present. */
unsigned int power_input_status(void)
{
    unsigned int status = 0;

    if (__gpio_get_pin(USB_CHARGER_GPIO))
        status |= POWER_INPUT_USB_CHARGER;

    return status;
}

void power_init(void)
{
    __gpio_as_input(USB_CHARGER_GPIO);
}

bool charging_state(void)
{
    return false;
}
