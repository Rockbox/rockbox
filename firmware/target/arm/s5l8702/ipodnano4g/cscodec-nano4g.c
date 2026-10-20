/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * S5L8702-specific code for Cirrus codecs
 *
 * Copyright (c) 2010 Michael Sparmann
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
#include "audiohw.h"
#include "i2c-s5l8702.h"
#include "s5l87xx.h"
#include "cscodec.h"
#include "pmu-target.h"

void audiohw_init(void)
{
    audiohw_preinit();
}

unsigned char cscodec_read(int reg)
{
    // TODO: not tested
    unsigned char data;
    i2c_read(0, 0x94, reg, 1, &data);
    return data;
}

void cscodec_write(int reg, unsigned char data)
{
    // TODO: not tested
    i2c_write(0, 0x94, reg, 1, &data);
}

void cscodec_power(bool state)
{
    unsigned char control;

    /* The codec runs from a D1759 LDO set to 1.8 V (register 0x21 = 6),
     * enabled by bit 2 of register 0x11. Apple's firmware leaves it on
     * when the codec is stopped, so only switch it on here. */
    if (!state)
        return;
    if (pmu_write(0x21, 6) == 0 &&
        pmu_read_multiple(0x11, 1, &control) == 0)
        pmu_write(0x11, control | 0x04);
}

void cscodec_reset(bool state)
{
    /* The codec reset line is GPIO 4.6, active low. */
    if (state)
        PDAT(4) &= ~(1 << 6);
    else
        PDAT(4) |= 1 << 6;
    PCON(4) |= 0x0f000000;
}

void cscodec_clock(bool state)
{
    /* MCLK is OSC0 (24 MHz) / 4 = 6 MHz, as Apple's firmware sets it:
     * select the divider with the gate (bit 15) left as it is, then
     * ungate 100 us later. */
    if (state)
    {
        CLKCON3 = (CLKCON3 & 0xffff8000) | 3;
        udelay(100);
        CLKCON3 &= ~0x8000;
    }
    else
        CLKCON3 |= 0x8000;
}
