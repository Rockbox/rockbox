/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#ifndef _POWER_H_
#define _POWER_H_

#if CONFIG_CHARGING
enum power_input_flags {
    /* No external power source? Default. */
    POWER_INPUT_NONE = 0x00,
 
    /* Main power source is available (AC?), the default other than
     * battery if for instance USB and others cannot be distinguished or
     * USB is the only possibility. */
    POWER_INPUT_MAIN = 0x01,

    /* USB power source is available (and is discernable from MAIN). */
    POWER_INPUT_USB  = 0x02,

    /* Something is plugged. */
    POWER_INPUT = 0x0f,

    /* POWER_INPUT_*_CHARGER of course implies presence of the respective
     * power source. */

    /* Battery not included in CHARGER (it can't charge itself) */

    /* Charging is possible on main. */
    POWER_INPUT_MAIN_CHARGER = 0x10 | POWER_INPUT_MAIN,

    /* Charging is possible on USB. */
    POWER_INPUT_USB_CHARGER = 0x20 | POWER_INPUT_USB,

    /* Charging is possible from something. */
    POWER_INPUT_CHARGER = 0xf0,

#ifdef HAVE_BATTERY_SWITCH
    /* Battery is powering device or is available to power device. It
     * could also be used if the battery is hot-swappable to indicate if
     * it is present ("switch" as verb vs. noun). */
    POWER_INPUT_BATTERY = 0x100,
#endif
};

/* Returns detailed power input status information from device. */
unsigned int power_input_status(void);

/* Shortcuts */
/* Returns true if any power source that is connected is capable of
 * charging the batteries.
 * > (power_input_status() & POWER_INPUT_CHARGER) != 0 */
bool charger_inserted(void);

/* Returns true if any power input is connected - charging-capable
 * or not.
 * > (power_input_status() & POWER_INPUT) != 0 */
bool power_input_present(void);
#endif /* CONFIG_CHARGING */

void power_off(void);
void ide_power_enable(bool on);

#if CONFIG_CHARGING >= CHARGING_MONITOR
bool charging_state(void);
#endif

#ifndef SIMULATOR

void power_init(void);

bool ide_powered(void);
#endif

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on);
bool spdif_powered(void);
#endif

#if CONFIG_TUNER
bool tuner_power(bool status);
bool tuner_powered(void);
#endif

#endif /* _POWER_H_ */
