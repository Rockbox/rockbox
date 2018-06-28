/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
#include "inttypes.h"
#include "power.h"
#include "panic.h"
#include "system.h"
#include "usb_core.h"   /* for usb_charging_maxcurrent_change */
#include "adc.h"

void power_off(void)
{
    GPIO_PCCON &= ~(1<<0);
    while(1);
}

void power_init(void)
{
    GPIO_PCDR |= (1<<0);
    GPIO_PCCON |= (1<<0);

    GPIO_PADR &= ~(1<<7);  /* MUTE */
    GPIO_PACON |= (1<<7);
}

unsigned int power_input_status(void)
{
    return (usb_detect() == USB_INSERTED) ? POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return (adc_read(ADC_EXTRA) < 512);
}
