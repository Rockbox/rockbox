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

void power_init(void)
{
    pmu_init();
    pmu_set_usblimit(false);  /* limit to 100mA */
}

void power_off(void)
{
    pmu_enter_standby();
    while(1);
}

#if CONFIG_CHARGING

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    bool fast_charge = (maxcurrent >= 500);
    pmu_set_usblimit(fast_charge);
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

/* As the original firmware decides it: the charger is
 * enabled (CHCTL bits 1..6), not suspended (SYSCTRLA bit 2), and not done:
 * STATUSB bits 1..2 clear. Measured: they read 0 while a 4.1 V cell
 * charges from USB, so they are taken as the charge-complete indication;
 * they have not been seen set. */
bool charging_state(void)
{
    if (!(power_input_status() & POWER_INPUT_CHARGER))
        return false;
    if (!(pmu_read(D1671_REG_CHCTL) & 0x7e))
        return false;
    if (pmu_read(D1671_REG_SYSCTRLA) & 0x04)
        return false;
    return !(pmu_read(D1671_REG_STATUSB) & 0x06);
}
#endif /* CONFIG_CHARGING */
