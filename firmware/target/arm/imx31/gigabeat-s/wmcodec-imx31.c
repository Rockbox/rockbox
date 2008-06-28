/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gigabeat S specific code for the WM8978 codec
 *
 * Copyright (C) 2008 Michael Sevakis
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
#include "system.h"
#include "kernel.h"
#include "sound.h"
#include "i2c-imx31.h"

/* NOTE: Some port-specific bits will have to be moved away (node and GPIO
 * writes) for cleanest implementation. */

static struct i2c_node wm8978_i2c_node =
{
    .num  = I2C1_NUM,
    .ifdr = I2C_IFDR_DIV192, /* 66MHz/.4MHz = 165, closest = 192 = 343750Hz */
                             /* Just hard-code for now - scaling may require 
                              * updating */
    .addr = WMC_I2C_ADDR,
};

void audiohw_init(void)
{
    /* USB PLL = 338.688MHz, /30 = 11.2896MHz = 256Fs */
    imx31_regmod32(&CLKCTL_PDR1,
                   PDR1_SSI1_PODFw(64-1) | PDR1_SSI2_PODFw(5-1),
                   PDR1_SSI1_PODF | PDR1_SSI2_PODF);
    imx31_regmod32(&CLKCTL_PDR1,
                   PDR1_SSI1_PRE_PODFw(4-1) | PDR1_SSI2_PRE_PODFw(1-1),
                   PDR1_SSI1_PRE_PODF | PDR1_SSI2_PRE_PODF);
    i2c_enable_node(&wm8978_i2c_node, true);

    audiohw_preinit();

    GPIO3_DR |= (1 << 21); /* Turn on analogue LDO */
}

void audiohw_enable_headphone_jack(bool enable)
{
    if (enable)
    {
        GPIO3_DR |= (1 << 22); /* Turn on headphone jack output */
    }
    else
    {
        GPIO3_DR &= ~(1 << 22); /* Turn off headphone jack output */
    }
}

void wmcodec_write(int reg, int data)
{
    unsigned char d[2];
    /* |aaaaaaad|dddddddd| */
    d[0] = (reg << 1) | ((data & 0x100) >> 8);
    d[1] = data;
    i2c_write(&wm8978_i2c_node, d, 2);
}
