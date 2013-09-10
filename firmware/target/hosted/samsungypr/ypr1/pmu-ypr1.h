/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#ifndef __PMU_YPR1_H__
#define __PMU_YPR1_H__

#include "sys/ioctl.h"

/**
 * This is the wrapper to r1Pmu.ko module with the possible
 * ioctl calls
 * The PMU controller is the MAX8819
 */

#define MAX8819_IOCTL_MAGIC                 'A'

#define E_MAX8819_IOCTL_GET_BATT_LVL        0
#define E_MAX8819_IOCTL_GET_CHG_STATUS      1
#define E_MAX8819_IOCTL_IS_EXT_PWR          2
#define E_MAX8819_IOCTL_STOP_CHG            3
#define E_MAX8819_IOCTL_START_CHG           4
#define E_MAX8819_IOCTL_IS_EXT_PWR_OVP      5
#define E_MAX8819_IOCTL_LCD_DIM_CTRL        6
#define E_MAX8819_IOCTL_CORE_CTL_HIGH       7
#define E_MAX8819_IOCTL_CORE_CTL_LOW        8
#define E_MAX8819_IOCTL_TSP_USB_PWR_OFF     9

#define MAX8819_IOCTL_GET_BATT_LVL          _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_GET_BATT_LVL)
#define MAX8819_IOCTL_GET_CHG_STATUS        _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_GET_CHG_STATUS)
#define MAX8819_IOCTL_IS_EXT_PWR            _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_IS_EXT_PWR)
#define MAX8819_IOCTL_STOP_CHG              _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_STOP_CHG)
#define MAX8819_IOCTL_START_CHG             _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_START_CHG)
#define MAX8819_IOCTL_IS_EXT_PWR_OVP        _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_IS_EXT_PWR_OVP)
#define MAX8819_IOCTL_LCD_DIM_CTRL          _IOW(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_LCD_DIM_CTRL, int)
#define MAX8819_IOCTL_CORE_CTL_HIGH         _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_CORE_CTL_HIGH)
#define MAX8819_IOCTL_CORE_CTL_LOW          _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_CORE_CTL_LOW)
#define MAX8819_IOCTL_TSP_USB_PWR_OFF       _IO(MAX8819_IOCTL_MAGIC, E_MAX8819_IOCTL_TSP_USB_PWR_OFF)

#define MAX8819_IOCTL_MAX_NR                (E_MAX8819_IOCTL_TSP_USB_PWR_OFF+1)

enum
{
    EXT_PWR_UNPLUGGED = 0,
    EXT_PWR_PLUGGED,
    EXT_PWR_NOT_OVP,
    EXT_PWR_OVP,
};

enum
{
    PMU_CHARGING = 0,
    PMU_NOT_CHARGING,
    PMU_FULLY_CHARGED,
};

enum
{
    BATT_LVL_OFF = 0,
    BATT_LVL_WARN,
    BATT_LVL_1,
    BATT_LVL_2,
    BATT_LVL_3,
    BATT_LVL_4,
};

void pmu_init(void);
void pmu_close(void);
int pmu_get_dev(void);
int pmu_ioctl(int request, int *data);

#endif
