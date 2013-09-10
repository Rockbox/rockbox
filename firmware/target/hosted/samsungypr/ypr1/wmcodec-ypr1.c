/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * I2C bus wrapper for WM1808 codec on SAMSUNG YP-R1
 *
 * Copyright (c) 2013 Lorenzo Miori
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
#include "wmcodec.h"
#include "audio.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/**
 * YP-R1's kernel has ALSA implementation of the WM1808, but it
 * unfortunately doesn't export any function to play with register.
 * For that reason we control the I2C bus directly, letting RB driver to do the rest
 * Assumption: no other ALSA applications are using the mixer!
 */

static int wmcodec_dev = -1;

/* The ONLY tested freq for now is 44100, others are just stubs!! */
const struct wmc_srctrl_entry wmc_srctrl_table[HW_NUM_FREQ] =
{
//     [HW_FREQ_8] = /* PLL = 65.536MHz */
//     {
//         .plln    = 7 | WMC_PLL_PRESCALE,
//         .pllk1   = 0x2f,            /* 12414886 */
//         .pllk2   = 0x0b7,
//         .pllk3   = 0x1a6,
//         .mclkdiv = WMC_MCLKDIV_8,   /*  2.0480 MHz */
//         .filter  = WMC_SR_8KHZ,
//     },
//     [HW_FREQ_11] = /* PLL = off */
//     {
//         .mclkdiv = WMC_MCLKDIV_6,   /*  2.8224 MHz */
//         .filter  = WMC_SR_12KHZ,
//     },
//     [HW_FREQ_12] = /* PLL = 73.728 MHz */
//     {
//         .plln    = 8 | WMC_PLL_PRESCALE,
//         .pllk1   = 0x2d,            /* 11869595 */
//         .pllk2   = 0x08e,
//         .pllk3   = 0x19b,
//         .mclkdiv = WMC_MCLKDIV_6,   /*  3.0720 MHz */
//         .filter  = WMC_SR_12KHZ,
//     },
//     [HW_FREQ_16] = /* PLL = 65.536MHz */
//     {
//         .plln    = 7 | WMC_PLL_PRESCALE,
//         .pllk1   = 0x2f,            /* 12414886 */
//         .pllk2   = 0x0b7,
//         .pllk3   = 0x1a6,
//         .mclkdiv = WMC_MCLKDIV_4,   /*  4.0960 MHz */
//         .filter  = WMC_SR_16KHZ,
//     },
//     [HW_FREQ_22] = /* PLL = off */
//     {
//         .mclkdiv = WMC_MCLKDIV_3,   /*  5.6448 MHz */
//         .filter  = WMC_SR_24KHZ,
//     },
//     [HW_FREQ_24] = /* PLL = 73.728 MHz */
//     {
//         .plln    = 8 | WMC_PLL_PRESCALE,
//         .pllk1   = 0x2d,            /* 11869595 */
//         .pllk2   = 0x08e,
//         .pllk3   = 0x19b,
//         .mclkdiv = WMC_MCLKDIV_3,   /*  6.1440 MHz */
//         .filter  = WMC_SR_24KHZ,
//     },
//     [HW_FREQ_32] = /* PLL = 65.536MHz */
//     {
//         .plln    = 7 | WMC_PLL_PRESCALE,
//         .pllk1   = 0x2f,            /* 12414886 */
//         .pllk2   = 0x0b7,
//         .pllk3   = 0x1a6,
//         .mclkdiv = WMC_MCLKDIV_2,   /*  8.1920 MHz */
//         .filter  = WMC_SR_32KHZ,
//     },
    [HW_FREQ_44] = /* PLL = on */
    {
        .plln    = 3 | (1 << 3),
        .pllk1   = 0x18,            /* 11289600 */
        .pllk2   = 0x111,
        .pllk3   = 0x139,
        .mclkdiv = WMC_MCLKDIV_2,
        .filter  = WMC_SR_48KHZ,
    },
//     [HW_FREQ_48] = /* PLL = 73.728 MHz */
//     {
//         .plln    = 8 | WMC_PLL_PRESCALE,
//         .pllk1   = 0x2d,            /* 11869595 */
//         .pllk2   = 0x08e,
//         .pllk3   = 0x19b,
//         .mclkdiv = WMC_MCLKDIV_1_5, /* 12.2880 MHz */
//         .filter  = WMC_SR_48KHZ,
//     },
};

void audiohw_init(void)
{
    /* First of all we need to open the device */
    wmcodec_dev = open("/dev/i2c-1", O_RDWR);
    if (wmcodec_dev < 0) {
        printf("Failed to open /dev/i2c-1 device!\n");
    }

    /* Let's set the slave address and if no error we are ready!*/
    int addr = 0x1a;
    if (ioctl(wmcodec_dev, I2C_SLAVE, addr) < 0) {
        printf("Failed to set slave address!\n");
    }
}

void wmcodec_write(int reg, int data)
{
    unsigned char data2[2];
    /* |aaaaaaad|dddddddd| */
    data2[0] = (reg << 0x1) | ((data >> 8) & 0x1);
    data2[1] = data;

    if (write(wmcodec_dev, data2, 2) < 0) {
        printf("I2C device write error!\n");
    }
}

void audiohw_enable_headphone_jack(bool enable)
{
    /* We don't use this facility: we have a separate GPIO for that */
    (void)enable;
}
