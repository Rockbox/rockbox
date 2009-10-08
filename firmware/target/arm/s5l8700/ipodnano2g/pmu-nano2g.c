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
int pmu_initialized = 0;

void pmu_read_multiple(int address, int count, unsigned char* buffer)
{
    i2c_read(0xe6, address, count, buffer);
}

void pmu_write_multiple(int address, int count, unsigned char* buffer)
{
    i2c_write(0xe6, address, count, buffer);
}

unsigned char pmu_read(int address)
{
    unsigned char tmp;

    pmu_read_multiple(address, 1, &tmp);

    return tmp;
}

void pmu_write(int address, unsigned char val)
{
    pmu_write_multiple(address, 1, &val);
}

void pmu_init(void)
{
    if (pmu_initialized) return;
    mutex_init(&pmu_adc_mutex);
    pmu_initialized = 1;
}

int pmu_read_battery_voltage(void)
{
    int data = 0;
    if (!pmu_initialized) pmu_init();
    mutex_lock(&pmu_adc_mutex);
    pmu_write(0x54, 0x05);
    while ((data & 0x80) == 0)
    {
        yield();
        data = pmu_read(0x57);
    }
    int millivolts = ((pmu_read(0x55) << 2) | (data & 3)) * 6;
    mutex_unlock(&pmu_adc_mutex);
    return millivolts;
}

int pmu_read_battery_current(void)
{
    int data = 0;
    if (!pmu_initialized) pmu_init();
    mutex_lock(&pmu_adc_mutex);
    pmu_write(0x54, 0x25);
    while ((data & 0x80) == 0)
    {
        yield();
        data = pmu_read(0x57);
    }
    int milliamps = (pmu_read(0x55) << 2) | (data & 3);
    mutex_unlock(&pmu_adc_mutex);
    return milliamps;
}

void pmu_switch_power(int gate, int onoff)
{
    if (gate < 4)
    {
        unsigned char newval = pmu_read(0x3B) & ~(1 << (2 * gate));
        if (onoff) newval |= 1 << (2 * gate);
        pmu_write(0x3B, newval);
    }
    else if (gate < 7)
    {
        unsigned char newval = pmu_read(0x3C) & ~(1 << (2 * (gate - 4)));
        if (onoff) newval |= 1 << (2 * (gate - 4));
        pmu_write(0x3C, newval);
    }
}