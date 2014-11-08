/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Szymon Dziok
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

#include "system.h"
#include "kernel.h"
#include "piezo.h"

void piezo_hw_voltage_on(void)
{
#ifndef SIMULATOR

#if defined(PHILIPS_SA9200)
    GPIOF_ENABLE     |=  0x08;
    GPIOF_OUTPUT_VAL |=  0x08;
    GPIOF_OUTPUT_EN  |=  0x08;
#else
    GPO32_ENABLE |= 0x2000;
    GPO32_VAL |= 0x2000;
#endif /* PHILIPS_SA9200 */

#endif
}

void piezo_hw_voltage_off(void)
{
#ifndef SIMULATOR

#if defined(PHILIPS_SA9200)
    GPIOF_OUTPUT_VAL &= ~0x08;
#else
    GPO32_VAL &= ~0x2000;
#endif /* PHILIPS_SA9200 */

#endif
}

void piezo_init(void)
{
}

void piezo_button_beep(bool beep, bool force)
{
    /* hw can only do a click */
    (void)beep;
    (void)force;
    piezo_hw_voltage_on();
    udelay(1000);
    piezo_hw_voltage_off();
}
