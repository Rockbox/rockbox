/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "pcf50606.h"
#include "i2c.h"

#define PCF50606_ADDR 0x10

int pcf50606_write(int address, unsigned char val)
{
    unsigned char data[] = { address, val };
    return i2c_write(PCF50606_ADDR, data, 2);
}

int pcf50606_write_multiple(int address, const unsigned char* buf, int count)
{
    int i;

    for (i = 0; i < count; i++)
        pcf50606_write(address + i, buf[i]);

    return 0;
}

int pcf50606_read(int address)
{
    unsigned char val = -1;
    i2c_readmem(PCF50606_ADDR, address, &val, 1);
    return val;
}

int pcf50606_read_multiple(int address, unsigned char* buf, int count)
{
    return i2c_readmem(PCF50606_ADDR, address, buf, count);
}

void pcf50606_init(void)
{
    // TODO
}

void pcf50606_reset_timeout(void)
{
    int level = disable_irq_save();
    pcf50606_write(PCF5060X_OOCC1, pcf50606_read(PCF5060X_OOCC1) | TOTRST);
    restore_irq(level);
}

void pcf50606_read_adc(int adc, short* res1, short* res2)
{
    int adcs1 = 0, adcs2 = 0, adcs3 = 0;

    int level = disable_irq_save();

    pcf50606_write(PCF5060X_ADCC2, (adc<<1) | 1); /* ADC start */

    do {
        adcs2 = pcf50606_read(PCF5060X_ADCS2);
    } while (!(adcs2 & 0x80));        /* Busy wait on ADCRDY flag */

    adcs1 = pcf50606_read(PCF5060X_ADCS1);
    if (res2 != NULL) adcs3 = pcf50606_read(PCF5060X_ADCS3);

    pcf50606_write(PCF5060X_ADCC2, 0);            /* ADC stop */

    restore_interrupt(level);

    if (res1 != NULL) *res1 = (adcs1 << 2) | (adcs2 & 3);
    if (res2 != NULL) *res2 = (adcs3 << 2) | ((adcs2 & 0xC) >> 2);
}
