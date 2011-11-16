/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "backlight.h"
#include "backlight-target.h"
#include "avr-sansaconnect.h"

void power_init(void)
{
}

void power_off(void)
{
    avr_hid_reset_codec();
    avr_hid_power_off();
}

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    return POWER_INPUT_NONE;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return false;
}
#endif

void ide_power_enable(bool on)
{
  (void)on;
}

