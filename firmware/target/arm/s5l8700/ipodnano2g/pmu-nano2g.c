/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "kernel.h"
#include "i2c-s5l8700.h"

static struct mutex pmu_adc_mutex;

int pmu_read_multiple(int address, int count, unsigned char* buffer)
{
    return i2c_read(0xe6, address, count, buffer);
}

int pmu_write_multiple(int address, int count, unsigned char* buffer)
{
    return i2c_write(0xe6, address, count, buffer);
}

unsigned char pmu_read(int address)
{
    unsigned char tmp;

    pmu_read_multiple(address, 1, &tmp);

    return tmp;
}

int pmu_write(int address, unsigned char val)
{
    return pmu_write_multiple(address, 1, &val);
}

void pmu_init(void)
{
    mutex_init(&pmu_adc_mutex);
}

int pmu_read_adc(unsigned int adc)
{
    int data = 0;
    mutex_lock(&pmu_adc_mutex);
    pmu_write(0x54, 5 | (adc << 4));
    while ((data & 0x80) == 0)
    {
        yield();
        data = pmu_read(0x57);
    }
    int value = (pmu_read(0x55) << 2) | (data & 3);
    mutex_unlock(&pmu_adc_mutex);
    return value;
}

/* millivolts */
int pmu_read_battery_voltage(void)
{
    return pmu_read_adc(0) * 6;
}

/* milliamps */
int pmu_read_battery_current(void)
{
    return pmu_read_adc(2);
}

void pmu_ldo_on_in_standby(unsigned int ldo, int onoff)
{
    if (ldo < 4)
    {
        unsigned char newval = pmu_read(0x3B) & ~(1 << (2 * ldo));
        if (onoff) newval |= 1 << (2 * ldo);
        pmu_write(0x3B, newval);
    }
    else if (ldo < 8)
    {
        unsigned char newval = pmu_read(0x3C) & ~(1 << (2 * (ldo - 4)));
        if (onoff) newval |= 1 << (2 * (ldo - 4));
        pmu_write(0x3C, newval);
    }
}

void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage)
{
    if (ldo > 6) return;
    pmu_write(0x2d + (ldo << 1), voltage);
}

void pmu_ldo_power_on(unsigned int ldo)
{
    if (ldo > 6) return;
    pmu_write(0x2e + (ldo << 1), 1);
}

void pmu_ldo_power_off(unsigned int ldo)
{
    if (ldo > 6) return;
    pmu_write(0x2e + (ldo << 1), 0);
}

void pmu_set_wake_condition(unsigned char condition)
{
    pmu_write(0xd, condition);
}

void pmu_enter_standby(void)
{
    pmu_write(0xc, 1);
}

void pmu_read_rtc(unsigned char* buffer)
{
    pmu_read_multiple(0x59, 7, buffer);
}

void pmu_write_rtc(unsigned char* buffer)
{
    pmu_write_multiple(0x59, 7, buffer);
}
