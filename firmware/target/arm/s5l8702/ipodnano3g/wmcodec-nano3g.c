/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * iPod Nano 3G code for the Wolfson codec (WM1870)
 *
 * Copyright (c) 2010 Michael Sparmann
 * Copyright (C) 2026 by Andrew Rice
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
#include "wmcodec.h"

/* The PCM driver sets the codec up (audiohw_preinit()) and controls its
 * MCLK */
void audiohw_init(void)
{
}

/* The codec runs from a 12 MHz MCLK here, so its rate codes are the USB
 * mode ones, with a BCLK divider, as the original firmware sets them. */
unsigned short wmcodec_sampctrl(unsigned long rate)
{
    static const struct
    {
        unsigned long rate;
        unsigned short sampctrl;
    } rates[] =
    {
        { SAMPR_8,  0x10d }, { SAMPR_11, 0x133 }, { SAMPR_12, 0x111 },
        { SAMPR_16, 0x115 }, { SAMPR_22, 0x137 }, { SAMPR_24, 0x139 },
        { SAMPR_32, 0x119 }, { SAMPR_44, 0x123 }, { SAMPR_48, 0x081 },
        { SAMPR_88, 0x0bf }, { SAMPR_96, 0x09d },
    };
    unsigned int i;

    for (i = 0; i < ARRAYLEN(rates); i++)
        if (rates[i].rate == rate)
            return rates[i].sampctrl;
    return 0;
}

/* 7-bit register and 9-bit value, as two bytes to address 0x34 on I2C
 * bus 0. The codec does not acknowledge reads. */
void wmcodec_write(int reg, int data)
{
    unsigned char d = data & 0xff;

    i2c_write(0, 0x34, (reg << 1) | ((data >> 8) & 1), 1, &d);
}
