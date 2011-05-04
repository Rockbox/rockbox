/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"

void button_init_device(void)
{
}

int button_read_device(void)
{
    int res = 0;
    if(!imx233_get_gpio_input_mask(1, 0x40000000))
        res |= BUTTON_VOL_DOWN;
    /* The imx233 uses the voltage on the PSWITCH pin to detect power up/down
     * events as well as recovery mode. Since the power button is the power button
     * and the volume up button is recovery, it is not possible to know whether
     * power button is down when volume up is down (except if there is another
     * method but volume up and power don't seem to be wired to GPIO pins). */
    switch((HW_POWER_STS & HW_POWER_STS__PSWITCH_BM) >> HW_POWER_STS__PSWITCH_BP)
    {
        case 1: res |= BUTTON_POWER; break;
        case 3: res |= BUTTON_VOL_UP; break;
        default: break;
    }
    return res;
}
