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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "system.h"
#include "audiohw.h"
#include "wmcodec.h"
#include "audio.h"
#include "panic.h"
#include "logf.h"


#define I2C_SLAVE 0x0703

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
    /* TODO fix PLL frequencies also for the other available rates */
    [HW_FREQ_44] = /* PLL = on */
    {
        .plln    = 3 | (1 << 3),
        .pllk1   = 0x18,            /* 11289600 */
        .pllk2   = 0x111,
        .pllk3   = 0x139,
        .mclkdiv = WMC_MCLKDIV_2,
        .filter  = WMC_SR_48KHZ,
    },
};

void audiohw_init(void)
{
    /* First of all we need to open the device */
    wmcodec_dev = open("/dev/i2c-1", O_RDWR);
    if (wmcodec_dev < 0)
        panicf("Failed to open /dev/i2c-1 device!\n");

    /* Let's set the slave address and if no error we are ready!*/
    int addr = 0x1a;
    if (ioctl(wmcodec_dev, I2C_SLAVE, addr) < 0)
        logf("Failed to set slave address!\n");
}

void wmcodec_write(int reg, int data)
{
    unsigned char data2[2];
    /* |aaaaaaad|dddddddd| */
    data2[0] = (reg << 0x1) | ((data >> 8) & 0x1);
    data2[1] = data;

    if (write(wmcodec_dev, data2, 2) < 0)
        panicf("I2C device write error!\n");
}

void audiohw_enable_headphone_jack(bool enable)
{
    /* We don't use this facility: we have a separate GPIO for that */
    (void)enable;
}
