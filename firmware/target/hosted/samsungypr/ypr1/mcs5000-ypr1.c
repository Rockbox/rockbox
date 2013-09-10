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

#include "mcs5000.h"
#include "ioctl-ypr1.h"

/* TODO Settings like hand and sensitivity will be lost when shutting device off!! */

static int mcs5000_dev = -1;
static int mcs5000_hand_setting = RIGHT_HAND;

void mcs5000_init(void)
{
    mcs5000_dev = open("/dev/r1Touch", O_RDONLY);
}

void mcs5000_close(void)
{
    if (mcs5000_dev > 0)
        close(mcs5000_dev);
}

void mcs5000_power(void)
{
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_ON);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_IDLE);
    mcs5000_set_hand(mcs5000_hand_setting);
}

void mcs5000_shutdown(void)
{
    /* save setting before shutting down the device */
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_FLUSH);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RESET);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_OFF);
}

void mcs5000_set_hand(int hand)
{
    switch (hand)
    {
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

void mcs5000_set_sensitivity(int level)
{
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_SET_SENSE, &level);
}

int mcs5000_read(struct mcs5000_raw_data *touchData)
{
    /* work around GCC bug: aligned attribute is not applied to automatic
     * variables, and apparently this structure has a large alignment requirement
     * (if it's only automatic with implicit 8-byte alignment then the
     * touchscreen misbehaves). The bug seems fixed in gcc 4.6.x
     * See http://http://gcc.gnu.org/bugzilla/show_bug.cgi?id=16660
     * Also: packet and aligned attributes don't really work together so
     * the aligned attribute cannot be attached to the struct declaration */ 
    static struct mcs5000_raw_data touchpad_data __attribute__((aligned(256)));
    ssize_t ret;
    ret = read(mcs5000_dev, &touchpad_data, sizeof(struct mcs5000_raw_data));
    *touchData = touchpad_data;
    return ret;
}
