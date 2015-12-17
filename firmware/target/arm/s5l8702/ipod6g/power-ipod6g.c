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

    /* USB power configuration:
     *
     * GPIO C0 is probably related to the LTC4066's CLPROG
     * pin (see datasheet). Setting it high allows to double
     * the maximum current selected by HPWR:
     *
     *  GPIO B6     GPIO C0     USB current
     *  HPWR        CLPROG ???  limit (mA)
     *  -------     ----------  -----------
     *  0           0           100
     *  1           0           500
     *  0           1           200
     *  1           1           1000 ??? (max.seen ~750mA)
     *
     * USB current limit includes battery charge and device
     * consumption. Battery charge has it's own limit at
     * 330~340 mA (configured using RPROG).
     *
     * Setting either of GPIO C1 or GPIO C2 disables battery
     * charge, power needed for device consumptiom is drained
     * from USB or AC adaptor when present. If external power
     * is not present or it is insufficient or limited,
     * additional required power is drained from battery.
     */
    PCONB = (PCONB & 0x000000ff)
          | (0xe << 8)      /* route D+ to ADC2: off */
          | (0xe << 12)     /* route D- to ADC2: off */
          | (0x0 << 16)     /* USB related input, POL pin ??? */
          | (0x0 << 20)     /* USB related input, !CHRG pin ??? */
          | (0xf << 24)     /* HPWR: 500mA */
          | (0xe << 28);    /* USB suspend: off */

    PCONC = (PCONC & 0xffff0000)
          | (0xe << 0)      /* double HPWR limit: off */
          | (0xe << 4)      /* disable battery charge: off */
          | (0xe << 8)      /* disable battery charge: off */
          | (0x0 << 12);    /* USB inserted/not inserted */
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
