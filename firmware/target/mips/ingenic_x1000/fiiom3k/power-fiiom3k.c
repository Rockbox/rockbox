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

#include "power.h"
#include "system.h"
#include "axp173.h"
#include "i2c-x1000.h"

void power_init(void)
{
    i2c_set_freq(2, I2C_FREQ_400K);

    /* TODO: Figure out which power outputs are needed for what, and push
     *       what power control we can to the subsystems (LCD, etc)
     *
     * Right now just turn everything on to avoid problems
     */
    axp173_reg_set(0x12, 0x5f);
    axp173_reg_set(0x80, 0xc0);

    /* Short delay to give power outputs time to stabilize */
    mdelay(5);
}

void power_off(void)
{
    axp173_reg_set(0x32, 1 << 7);
    while(1);
}
