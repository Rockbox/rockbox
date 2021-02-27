/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "adc.h"
#include "axp173.h"

void adc_init(void)
{
    /* Enable all ADCs, except TS which isn't connected */
    axp173_reg_write(0x82, 0xfe);

    /* Set ADC sample rate to 25 Hz (slowest rate supported) */
    axp173_reg_clrset(0x84, 0xc0, 0);
}

unsigned short adc_read(int adc)
{
    int value = axp173_adc_read(adc);
    if(value < 0)
        return 0;

    value = axp173_adc_conv(adc, value);
    if(adc == ADC_BATTERY_POWER)
        return value / 1000;
    else
        return value;
}
