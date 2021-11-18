/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for SI4709 FM Radio Chip, using /dev/si470x (si4709.ko) 
 *      Samsung YP-R0 & Samsung YP-R1
 *
 * Copyright (c) 2012 Lorenzo Miori
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

/* system includes */
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "stdint.h"
#include "string.h"

/* application includes */
#include "kernel.h"
#include "radio-ypr.h"
#include "power.h"
#include "gpio-target.h"
#include "gpio-ypr.h"
#include "panic.h"
#include "i2c-target.h"

/* 7bits I2C address for Si4709
 * (apparently not selectable by pins or revisions) */
#define SI4709_I2C_SLAVE_ADDR   0x10

/** i2c file handle */
static int radio_dev = -1;

/* toggle radio RST pin */
static void power_toggle(bool set)
{
    /* setup the GPIO port, as in OF */
    gpio_set_iomux(GPIO_FM_BUS_EN, CONFIG_ALT3);
    gpio_set_pad(GPIO_FM_BUS_EN, PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH);
    gpio_direction_output(GPIO_FM_BUS_EN);
    gpio_set(GPIO_FM_BUS_EN, set);
}

void radiodev_open(void)
{
    int retval = -1;    /* ioctl return value */

    /* open the I2C bus where the chip is attached to */
    radio_dev = open(I2C_BUS_FM_RADIO, O_RDWR);

    if (radio_dev == -1)
    {
        panicf("%s: failed to open /dev/i2c-1 device - %d", __FUNCTION__, retval);
    }
    else
    {
        /* device is open, go on */

        /* set the slave address for the handle.
         * Some other modules might have set the same slave address
         * e.g. another module. Let's do a I2C_SLAVE_FORCE which does
         * not care about looking for other init'ed i2c slaves */
        retval = ioctl(radio_dev, I2C_SLAVE_FORCE, SI4709_I2C_SLAVE_ADDR);

        if (retval == -1)
        {
            /* the ioctl call should never fail, if radio_dev is valid */
            panicf("%s: failed to set slave address - %d", __FUNCTION__, retval);
        }
        else
        {
            /* initialization completed */
        }
    }

    /* i2c subsystem ready, now toggle power to the chip */
    power_toggle(true);
    /* 100ms reset delay */
    sleep(HZ/10);

}

void radiodev_close(void)
{
    /* power the chip down */
    power_toggle(false);

    /* close the i2c subsystem */
    if (radio_dev != -1)
    {
        (void)close(radio_dev);
    }
    else
    {
        /* not opened */
    }

    /* set back to safe error value */
    radio_dev = -1;
}

/* Low-level i2c channel access: write */
int fmradio_i2c_write(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    return write(radio_dev, buf, count);
}

/* Low-level i2c channel access: read */
int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    return read(radio_dev, buf, count);
}
