/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Driver for TPS 65021 Power Management IC
 *
 * Copyright (c) 2011 Tomasz Moń
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
#include "config.h"
#if CONFIG_I2C == I2C_DM320
#include "i2c-dm320.h"
#endif
#include "logf.h"
#include "tps65021.h"

/* (7-bit) address is 0x48, the LSB is read/write flag */
#define TPS65021_ADDR (0x48 << 1)

static void tps65021_write_reg(unsigned reg, unsigned value)
{
    unsigned char data[2];

    data[0] = reg;
    data[1] = value;

#if CONFIG_I2C == I2C_DM320
    if (i2c_write(TPS65021_ADDR, data, 2) != 0)
#else
    #warning Implement tps65021_write_reg()
#endif
    {
       logf("TPS65021 error reg=0x%x", reg);
       return;
    }
}

void tps65021_init(void)
{
#ifdef SANSA_CONNECT
    /* PWM mode */
    tps65021_write_reg(0x04, 0xB2);

    /* Set core voltage to 1.5V */
    tps65021_write_reg(0x06, 0x1C);

    /* Set LCM (LDO1) to 2.85V, Set CODEC and USB (LDO2) to 1.8V */
    tps65021_write_reg(0x08, 0x36);
#endif
}
