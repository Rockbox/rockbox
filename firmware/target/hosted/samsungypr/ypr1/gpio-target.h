/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for GPIO, using /dev/r1GPIO (r1Gpio.ko) of Samsung YP-R1
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

#ifndef GPIO_TARGET_H
#define GPIO_TARGET_H

/* Some meaningful pins used in the YP-R1 */

#define GPIO_HEADPHONE_SENSE        GPIO1_31
/* I2C bus for the SI4079 FM, WM1808 codec and RTC radio chip */
#define GPIO_I2C_CLK1               GPIO1_0
#define GPIO_I2C_DAT1               GPIO1_1
/* I2C bus for the fuel gauge MAX17040 */
#define GPIO_I2C_CLK2               GPIO2_12
#define GPIO_I2C_DAT2               GPIO2_13
/* SI4079 pins - powerup and interrupt */
#define GPIO_FM_SEARCH              GPIO1_4
#define GPIO_FM_BUS_EN              GPIO1_10
#define GPIO_MUTE                   GPIO2_17
#define EXT_POWER_DET               GPIO1_26
/* Low disabled, high enabled */
#define TV_OUT_ENABLE               GPIO1_17
/* Battery charging */
#define CHARGE_ENABLE               GPIO1_18
#define CHARGE_STATUS               GPIO_D13
/* This should be high when connecting a special port to the board... */
#define PBA_CHECK_ENABLED           GPIO2_1
/* TODO see if this is the source of massive battery drain
 * touchscreen and usb 3.3v power control line
 */
#define POWER_3V3_LINE_CONTROL      GPIO1_16

/* Keypad */

#define GPIO_VOL_UP_KEY             GPIO1_20
#define GPIO_VOL_DOWN_KEY           GPIO1_21
#define GPIO_POWER_KEY              GPIO2_16

#define GPIO_DEVICE                 "/dev/r1Gpio"
/* Strangely for whatever reason magic differs from R0 (A vs. G) */
#define GPIO_IOCTL_MAGIC            'A'

#endif /* GPIO_TARGET_H */
