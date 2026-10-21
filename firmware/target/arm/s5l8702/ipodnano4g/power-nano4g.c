/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "s5l87xx.h"
#include "power.h"
#include "panic.h"
#include "pmu-target.h"
#include "usb_core.h"   /* for usb_charging_maxcurrent_change */
#include "powermgmt.h"
#ifndef BOOTLOADER
#include "lcd.h"
#endif

void power_init(void)
{
    pmu_init();
    /* The USB input limit stays as the bootloader set it (500 mA with the
     * host configured): on a dead battery, 100 mA browns the SoC out */
}

void power_off(void)
{
//     /* USB inserted or EXTON1 */
//     pmu_set_wake_condition(/*TODO*/);
    pmu_enter_standby();
    while(1);
}

#if CONFIG_CHARGING

#ifdef HAVE_USB_CHARGING_ENABLE
/* Below this the battery cannot carry the SoC (Apple's BatteryTrap holds
 * the OS at the logo until it is reached) */
#define USB_LIMIT_LOW_BATTERY_MV    3450

void usb_charging_maxcurrent_change(int maxcurrent)
{
//     bool suspend_charging = (maxcurrent < 100);
    bool fast_charging = (maxcurrent >= 500);

    /* The USB stack asks for 100 mA until the host configures it, which
     * resets a Nano 4G running on USB power alone */
    if (!fast_charging && _battery_voltage() < USB_LIMIT_LOW_BATTERY_MV)
        fast_charging = true;
    pmu_set_usblimit(fast_charging);
}
#endif

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;
    if (usb_detect() == USB_INSERTED)
        status |= POWER_INPUT_USB_CHARGER;
    if (pmu_firewire_present())
        status |= POWER_INPUT_MAIN_CHARGER;
    return status;
}

bool charging_state(void)
{
    /* No input means not charging. 0x05 bits 1..2: charger running */
    if (power_input_status() == POWER_INPUT_NONE)
        return false;
    return (pmu_read(D1759_REG_STATUSB) & 0x06) != 0;
}
#endif /* CONFIG_CHARGING */
