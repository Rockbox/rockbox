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
#include "s5l8700.h"
#include "power.h"

/*  Power handling for the S5L8700 based Samsung YP-S3

    Pins involved in power management:
    * P0.1: stay powered up (even with the USB cable unplugged)
    * P1.1: USB power detect
    * P4.7: tuner power/enable
    * P5.2: unknown output
    * P5.3: unknown output, related to charging (perhaps charge current?)
    * P5.4: charge status input (only valid if charger enabled)
    * P5.6: charger enable
*/

void power_off(void)
{
    /* take down P0.1 to power off (plugged USB cable overrides this though) */
    PDAT0 &= ~(1 << 1);

    while(1); /* wait for system to shut down */
}

void power_init(void)
{
    /* configure P0.1 as output for power-up and stay powered up */
    PCON0 = (PCON0 & ~(3 << 2)) | (1 << 2);
    PDAT0 |= (1 << 1);

    /* configure P1.1 as input for USB power detect */
    PCON1 = (PCON1 & ~0x000000F0) | 0x00000000;

    /* configure P4.7 as output for tuner power and turn power off */
    PCON4 = (PCON4 & ~0xF0000000) | 0x10000000;
    PDAT4 &= ~(1 << 7);

    /* configure P5.2 / P5.3 / P5.6 as output, P5.4 as input */
    PCON5 = (PCON5 & ~0x0F0FFF00) | 0x01001100;
    PDAT5 &= ~((1 << 2) | (1 << 3) | (1 << 6));
}

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    /* check USB power on P1.1 */
    if (PDAT1 & (1 << 1)) {
        return POWER_INPUT_USB;
    }
    
    return POWER_INPUT_NONE;
}

bool charging_state(void)
{
    /* check if charger is enabled */
    if (PDAT5 & (1 << 6)) {
        /* check if charging is busy */
        return (PDAT5 & (1 << 4));
    }
    return false;
}
#endif /* CONFIG_CHARGING */

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    if (status) {
        PDAT4 |= (1 << 7);
    }
    else {
        PDAT4 &= ~(1 << 7);
    }
    /* TODO what should we return here? */
    return status;    
}

bool tuner_powered(void)
{
    return (PDAT4 & (1 << 7));
}
#endif /* CONFIG_TUNER */

