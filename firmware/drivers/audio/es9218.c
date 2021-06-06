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

#include "audiohw.h"
#include "system.h"
#include "i2c-async.h"

void es9218_open(void)
{
    /* Enable power supply */
    es9218_set_reset_pin(0);
    es9218_set_power_pin(1);
    mdelay(2);

#if 1
    /* if a crystal oscillator is directly connected to the DAC,
     * this step is needed to boot the DAC's internal clock */
    es9218_set_reset_pin(1);
    udelay(70);
    es9218_set_reset_pin(0);
    udelay(50);
#endif

    /* Take chip out of reset */
    es9218_set_reset_pin(0);
    mdelay(2);
}

void es9218_close(void)
{
    /* Turn off power supply */
    es9218_set_power_pin(0);
}

int es9218_read(int reg)
{
    return i2c_reg_read1(ES9218_BUS, ES9218_ADDR, reg);
}

void es9218_write(int reg, uint8_t val)
{
    i2c_reg_write1(ES9218_BUS, ES9218_ADDR, reg, val);
}
