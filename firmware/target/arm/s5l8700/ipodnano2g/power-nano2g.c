/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Bertrik Sikken
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
#include "s5l8700.h"
#include "power.h"
#include "ftl-target.h"
#include <string.h>
#include "panic.h"
#include "pmu-target.h"
#include "lcd.h"


void power_off(void)
{
    pmu_switch_power(0, 0);
    pmu_switch_power(2, 0);
    pmu_switch_power(3, 0);
    pmu_switch_power(4, 0);
    pmu_switch_power(6, 0);
    pmu_switch_power(7, 0);

    pmu_write(0x36, pmu_read(0x36) & 0xF0);
    pmu_write(0x34, pmu_read(0x34) & 0xF0);
    pmu_write(0xD, pmu_read(0xD) | 0x40);
    pmu_write(0xD, pmu_read(0xD) | 0x02);
    pmu_write(0xC, 1);

    while(1);
}

void power_init(void)
{
    /* TODO */
}

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    /* TODO */
    return POWER_INPUT_NONE;
}

bool charging_state(void)
{
    /* TODO */
    return false;
}
#endif /* CONFIG_CHARGING */
