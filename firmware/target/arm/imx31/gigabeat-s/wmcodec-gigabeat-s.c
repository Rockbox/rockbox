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
#include "audiohw.h"
#include "wmcodec.h"
#include "audio.h"
#include "i2s.h"
#include "i2c-imx31.h"

static struct i2c_node wm8978_i2c_node =
{
    .num  = I2C1_NUM,
    .ifdr = I2C_IFDR_DIV192, /* 66MHz/.4MHz = 165, closest = 192 = 343750Hz */
    .addr = WMC_I2C_ADDR,
};

/* For 16.9344MHz MCLK, codec as master. */
const struct wmc_srctrl_entry wmc_srctrl_table[HW_NUM_FREQ] =
{
    [HW_FREQ_8] = /* PLL = 65.536MHz */
    {
        .plln    = 7 | WMC_PLL_PRESCALE,
        .pllk1   = 0x2f,            /* 12414886 */
        .pllk2   = 0x0b7,
        .pllk3   = 0x1a6,
        .mclkdiv = WMC_MCLKDIV_8,   /*  2.0480 MHz */
        .filter  = WMC_SR_8KHZ,
    },
    [HW_FREQ_11] = /* PLL = off */
    {
        .mclkdiv = WMC_MCLKDIV_6,   /*  2.8224 MHz */
        .filter  = WMC_SR_12KHZ,
    },
    [HW_FREQ_12] = /* PLL = 73.728 MHz */
    {
        .plln    = 8 | WMC_PLL_PRESCALE,
        .pllk1   = 0x2d,            /* 11869595 */
        .pllk2   = 0x08e,
        .pllk3   = 0x19b,
        .mclkdiv = WMC_MCLKDIV_6,   /*  3.0720 MHz */
        .filter  = WMC_SR_12KHZ,
    },
    [HW_FREQ_16] = /* PLL = 65.536MHz */
    {
        .plln    = 7 | WMC_PLL_PRESCALE,
        .pllk1   = 0x2f,            /* 12414886 */
        .pllk2   = 0x0b7,
        .pllk3   = 0x1a6,
        .mclkdiv = WMC_MCLKDIV_4,   /*  4.0960 MHz */
        .filter  = WMC_SR_16KHZ,
    },
    [HW_FREQ_22] = /* PLL = off */
    {
        .mclkdiv = WMC_MCLKDIV_3,   /*  5.6448 MHz */
        .filter  = WMC_SR_24KHZ,
    },
    [HW_FREQ_24] = /* PLL = 73.728 MHz */
    {
        .plln    = 8 | WMC_PLL_PRESCALE,
        .pllk1   = 0x2d,            /* 11869595 */
        .pllk2   = 0x08e,
        .pllk3   = 0x19b,
        .mclkdiv = WMC_MCLKDIV_3,   /*  6.1440 MHz */
        .filter  = WMC_SR_24KHZ,
    },
    [HW_FREQ_32] = /* PLL = 65.536MHz */
    {
        .plln    = 7 | WMC_PLL_PRESCALE,
        .pllk1   = 0x2f,            /* 12414886 */
        .pllk2   = 0x0b7,
        .pllk3   = 0x1a6,
        .mclkdiv = WMC_MCLKDIV_2,   /*  8.1920 MHz */
        .filter  = WMC_SR_32KHZ,
    },
    [HW_FREQ_44] = /* PLL = off */
    {
        .mclkdiv = WMC_MCLKDIV_1_5, /* 11.2896 MHz */
        .filter  = WMC_SR_48KHZ,
    },
    [HW_FREQ_48] = /* PLL = 73.728 MHz */
    {
        .plln    = 8 | WMC_PLL_PRESCALE,
        .pllk1   = 0x2d,            /* 11869595 */
        .pllk2   = 0x08e,
        .pllk3   = 0x19b,
        .mclkdiv = WMC_MCLKDIV_1_5, /* 12.2880 MHz */
        .filter  = WMC_SR_48KHZ,
    },
};

void audiohw_init(void)
{
    i2s_reset();

    i2c_enable_node(&wm8978_i2c_node, true);

    audiohw_preinit();

    bitset32(&GPIO3_DR, (1 << 21)); /* Turn on analogue LDO */
}

void audiohw_enable_headphone_jack(bool enable)
{
    /* Turn headphone jack output on or off. */
    bitmod32(&GPIO3_DR, enable ? (1 << 22) : 0, (1 << 22));
}

void wmcodec_write(int reg, int data)
{
    unsigned char d[2];
    /* |aaaaaaad|dddddddd| */
    d[0] = (reg << 1) | ((data & 0x100) >> 8);
    d[1] = data;
    i2c_write(&wm8978_i2c_node, d, 2);
}
