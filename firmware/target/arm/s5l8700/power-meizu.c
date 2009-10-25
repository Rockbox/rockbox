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

/*  Power handling for S5L8700 based Meizu players

    The M3 and M6 players appear to use the same pins for power, USB detection
    and charging status.
*/

void power_off(void)
{
    /* take down PWRON_M (P1.3) */
    PDAT1 &= ~(1 << 3);
}

void power_init(void)
{
    /* configure PWRON_M (P1.3) as output and set it to keep power turned on */
    PCON1 = (PCON1 & ~(0xF << 12)) | (0x1 << 12);
    PDAT1 |= (1 << 3);
    
    /* configure PBSTAT (P1.4) as input */
    PCON1 &= ~(0xF << 16);
    
    /* configure CHRG (P5.7) as input to monitor charging state */
    PCON5 &= ~(0xF << 28);
    
    /* enable USB2.0 function controller to allow VBUS monitoring */
    PWRCON &= ~(1 << 15);
}

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    /* check VBUS in the USB2.0 function controller */
    if (USB_TR & (1 << 15)) {
        return POWER_INPUT_USB;
    }
    
    return POWER_INPUT_NONE;
}

bool charging_state(void)
{
    /* CHRG pin goes low when charging */
    return ((PDAT5 & (1 << 7)) == 0);
}
#endif /* CONFIG_CHARGING */

#if CONFIG_TUNER
static bool tuner_on = false;

bool tuner_power(bool status)
{
    if (status != tuner_on)
    {
        tuner_on = status;
        status = !status;
    }

    return status;    
}

bool tuner_powered(void)
{
    return tuner_on; /* No debug info */
}
#endif /* CONFIG_TUNER */

