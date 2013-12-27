/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include <unistd.h>
#include <fcntl.h>
#include "sys/ioctl.h"
#include "kernel.h"

#include "mcs5000.h"

static int mcs5000_dev = -1;
static int mcs5000_hand_setting = RIGHT_HAND;
static struct mutex mcs5000_mutex;

void mcs5000_init(void) {
    mutex_init(&mcs5000_mutex);
    mcs5000_dev = open("/dev/r1Touch", O_RDONLY);
}

void mcs5000_close(void) {
    if (mcs5000_dev > 0) {
        close(mcs5000_dev);
    }
}

int mcs5000_ioctl(int request, void *data) {
    int ret = ioctl(mcs5000_dev, request, data);
    return ret;
}

void mcs5000_power(void) {
    mcs5000_ioctl(DEV_CTRL_TOUCH_ON, NULL);
    mcs5000_ioctl(DEV_CTRL_TOUCH_IDLE, NULL);
    mcs5000_set_hand(mcs5000_hand_setting);
}

void mcs5000_shutdown(void) {
    /* save setting before shutting down the device */
    mcs5000_ioctl(DEV_CTRL_TOUCH_FLUSH, NULL);
    mcs5000_ioctl(DEV_CTRL_TOUCH_RESET, NULL);
    mcs5000_ioctl(DEV_CTRL_TOUCH_OFF, NULL);
}

void mcs5000_set_hand(int hand) {
    switch (hand) {
        case RIGHT_HAND:
            ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RIGHTHAND);
            break;
        case LEFT_HAND:
            ioctl(mcs5000_dev, DEV_CTRL_TOUCH_LEFTHAND);
            break;
        default:
            break;
    }
    mcs5000_hand_setting = hand;
}

void mcs5000_set_sensitivity(int level) {
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_SET_SENSE, &level);
}

int mcs5000_read(mcs5000_raw_data *touchData) {
    int ret;
    mutex_lock(&mcs5000_mutex);
    ret = read(mcs5000_dev, touchData, sizeof(mcs5000_raw_data));
    mutex_unlock(&mcs5000_mutex);
    return ret;
}

/* Generic register getter and setter
 * WARNING: the kernel module has a no correct
 * mutual exclusion mechanisms in the ioctl calls,
 * so before any read or write we have to disable the watchdog
 * and thus sampling...
 * Of course these have mainly a debug function
 */

int mcs5000_read_reg(unsigned char reg, unsigned char* data)
{
    int ret = 0;
    mcs5000_i2c_data i2c_data = { .count = MCS5000_REG_SIZE, .addr = MCS5000_SLAVE_ADDRESS };
    i2c_data.pData[0] = reg;
    mutex_lock(&mcs5000_mutex);
    mcs5000_ioctl(DEV_CTRL_TOUCH_FLUSH, NULL);
    mcs5000_ioctl(DEV_CTRL_TOUCH_SLEEP, NULL);
    mcs5000_ioctl(DEV_CTRL_TOUCH_DISABLE_WDOG, NULL);
    ret = mcs5000_ioctl(DEV_CTRL_TOUCH_I2C_WRITE, &i2c_data);
    i2c_data.pData[0] = 0;
    i2c_data.count = 1;
    i2c_data.addr = 0x40;
    ret &= mcs5000_ioctl(DEV_CTRL_TOUCH_I2C_READ, &i2c_data);
    *data = i2c_data.pData[0];
//     for (int i = 0; i < 255; i++)
//     {
//         if (i2c_data.pData[i] > 0)
//             *data= i2c_data.pData[i];
//     }
    mcs5000_ioctl(DEV_CTRL_TOUCH_WAKE, NULL);
    mcs5000_ioctl(DEV_CTRL_TOUCH_ENABLE_WDOG, NULL);
    mutex_unlock(&mcs5000_mutex);
    /* Samsung has always to do things unconventionally, 0 fail, 1 success */
    return ret;
}

int mcs5000_write_reg(unsigned char reg, unsigned char* data)
{
    mcs5000_i2c_data i2c_data = { .count = MCS5000_REG_SIZE, .addr = MCS5000_SLAVE_ADDRESS };
    i2c_data.pData[0] = reg;
    i2c_data.pData[0] = *data;
    mutex_lock(&mcs5000_mutex);
    mcs5000_ioctl(DEV_CTRL_TOUCH_I2C_WRITE, &i2c_data);
    *data = i2c_data.pData[0];
    mcs5000_ioctl(DEV_CTRL_TOUCH_ENABLE_WDOG, NULL);
    mutex_unlock(&mcs5000_mutex);
    
    return 0;
}
