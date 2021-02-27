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
    /* no-op, AXP chip init is handled in power-fiiom3k.c */
}

unsigned short adc_read(int adc)
{
    int value = axp173_adc_read(adc);
    if(value < 0)
        return 0;

    if(adc == ADC_BATTERY_POWER) {
        /* this is a 24-bit ADC; bring it down to 16 bits */
        return value / 1000;
    } else {
        return value;
    }
}
