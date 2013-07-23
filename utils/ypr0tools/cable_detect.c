/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Samsung YP-R1 runtime USB cable detection
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define IOCTL_PMU_MAGIC                 'A'

#define E_IOCTL_PMU_GET_BATT_LVL        0
#define E_IOCTL_PMU_GET_CHG_STATUS      1
#define E_IOCTL_PMU_IS_EXT_PWR          2
#define E_IOCTL_PMU_STOP_CHG            3
#define E_IOCTL_PMU_START_CHG           4
#define E_IOCTL_PMU_IS_EXT_PWR_OVP      5
#define E_IOCTL_PMU_LCD_DIM_CTRL        6
#define E_IOCTL_PMU_CORE_CTL_HIGH       7
#define E_IOCTL_PMU_CORE_CTL_LOW        8
#define E_IOCTL_PMU_TSP_USB_PWR_OFF     9

#define IOCTL_PMU_GET_BATT_LVL          _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_GET_BATT_LVL)
#define IOCTL_PMU_GET_CHG_STATUS        _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_GET_CHG_STATUS)
#define IOCTL_PMU_IS_EXT_PWR            _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_IS_EXT_PWR)
#define IOCTL_PMU_STOP_CHG              _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_STOP_CHG)
#define IOCTL_PMU_START_CHG             _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_START_CHG)
#define IOCTL_PMU_IS_EXT_PWR_OVP        _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_IS_EXT_PWR_OVP)
#define IOCTL_PMU_LCD_DIM_CTRL          _IOW(IOCTL_PMU_MAGIC, E_IOCTL_PMU_LCD_DIM_CTRL, int)
#define IOCTL_PMU_CORE_CTL_HIGH         _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_CORE_CTL_HIGH)
#define IOCTL_PMU_CORE_CTL_LOW          _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_CORE_CTL_LOW)
#define IOCTL_PMU_TSP_USB_PWR_OFF       _IO(IOCTL_PMU_MAGIC, E_IOCTL_PMU_TSP_USB_PWR_OFF)

/*
 * This is a very simple program that runs on device.
 * It returns error code either 0 when (power/usb) cable
 * is not connected or >= 1 if connected.
 */
int main(void)
{
    int state = -1;
    int dev = open("/dev/r1Pmu", O_RDONLY);
    state = ioctl(dev, IOCTL_PMU_IS_EXT_PWR);
    close(dev);
    return state;
}
