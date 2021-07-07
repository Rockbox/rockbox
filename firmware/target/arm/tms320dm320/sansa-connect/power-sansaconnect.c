/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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

#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "backlight.h"
#include "backlight-target.h"
#include "avr-sansaconnect.h"
#include "i2c-dm320.h"
#include "logf.h"


const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3450
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3400, 3508, 3630, 3703, 3727, 3750, 3803, 3870, 3941, 4026, 4142 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3540, 3788, 3860, 3890, 3916, 3956, 4016, 4085, 4164, 4180, 4190
};

/* (7-bit) address is 0x48, the LSB is read/write flag */
#define TPS65021_ADDR (0x48 << 1)

static void tps65021_write_reg(unsigned reg, unsigned value)
{
    unsigned char data[2];

    data[0] = reg;
    data[1] = value;

    if (i2c_write(TPS65021_ADDR, data, 2) != 0)
    {
        logf("TPS65021 error reg=0x%x", reg);
    }
}

void power_init(void)
{
    /* Enable LDO */
    tps65021_write_reg(0x03, 0xFD);

    /* PWM mode */
    tps65021_write_reg(0x04, 0xB2);

    /* Set core voltage to 1.5V */
    tps65021_write_reg(0x06, 0x1C);

    /* Set LCM (LDO1) to 2.85V, Set CODEC and USB (LDO2) to 1.8V */
    tps65021_write_reg(0x08, 0x36);

    /* Enable internal charger */
    avr_hid_enable_charger();
}

void power_off(void)
{
    /* Disable GIO0 and GIO2 interrupts */
    IO_INTC_EINT1 &= ~(INTR_EINT1_EXT2 | INTR_EINT1_EXT0);

    avr_hid_power_off();
}

void ide_power_enable(bool on)
{
  (void)on;
}
