/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: power-nano2g.c 28190 2010-10-01 18:09:10Z Buschel $
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
#include "s5l8702.h"
#include "power.h"
#include "panic.h"
#include "pmu-target.h"
#include "usb_core.h"   /* for usb_charging_maxcurrent_change */

static int idepowered;

void power_off(void)
{
    pmu_set_wake_condition(0x42); /* USB inserted or EXTON1 */
    pmu_enter_standby();

    while(1);
}

void power_init(void)
{
    idepowered = false;

    /* DOWN1CTL: CPU DVM step time = 30us (default: no DVM) */
    pmu_write(0x20, 2);
}

void ide_power_enable(bool on)
{
    idepowered = on;
    pmu_hdd_power(on);
}

bool ide_powered()
{
    return idepowered;
}

#if CONFIG_CHARGING

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    bool suspend_charging = (maxcurrent < 100);
    bool fast_charging = (maxcurrent >= 500);

    /* This GPIO is connected to the LTC4066's SUSP pin */
    /* Setting it high prevents any power being drawn over USB */
    /* which supports USB suspend */
    GPIOCMD = 0xb070e | (suspend_charging ? 1 : 0);

    /* This GPIO is connected to the LTC4066's HPWR pin */
    /* Setting it low limits current to 100mA, setting it high allows 500mA */
    GPIOCMD = 0xb060e | (fast_charging ? 1 : 0);
}
#endif

unsigned int power_input_status(void)
{
    return (PDAT(12) & 8) ? POWER_INPUT_NONE : POWER_INPUT_MAIN_CHARGER;
}

bool charging_state(void)
{
    return false; //TODO: Figure out
}
#endif /* CONFIG_CHARGING */
