/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for GPIO, using /dev/r0GPIO (r0Gpio.ko) of Samsung YP-R0
 *
 * Copyright (c) 2011 Lorenzo Miori
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

#ifndef GPIO_YPR0_H
#define GPIO_YPR0_H

#include "r0GPIOIoctl.h"

/* Some meaningful pins used in the R0 */

#define GPIO_HEADPHONE_SENSE        GPIO1_5
#define GPIO_EXT_PWR_SENSE          GPIO1_26
#define GPIO_SD_SENSE               GPIO2_27
#define GPIO_AS3543_INTERUPT        GPIO1_25
#define GPIO_PCB_VER_DETECT         GPIO_10
/* I2C bus for AS3543 codec */
#define GPIO_I2C_CLK0               GPIO_1_0
#define GPIO_I2C_DAT0               GPIO_1_1
/* I2C bus for the SI4079 FM radio chip */
#define GPIO_I2C_CLK1               GPIO_2_12
#define GPIO_I2C_DAT1               GPIO_2_13
#define GPIO_FM_SEARCH              GPIO1_4
#define GPIO_FM_BUS_EN              GPIO2_19

/* Keypad */

#define GPIO_BACK_KEY               GPIO2_29
#define GPIO_USER_KEY               GPIO2_30
#define GPIO_MENU_KEY               GPIO2_31
#define GPIO_POWER_KEY              GPIO2_16
#define GPIO_CENTRAL_KEY            GPIO3_5
#define GPIO_UP_KEY                 GPIO3_9
#define GPIO_DOWN_KEY               GPIO3_8
#define GPIO_LEFT_KEY               GPIO2_28
#define GPIO_RIGHT_KEY              GPIO3_7


void gpio_init(void);
void gpio_close(void);
int gpio_control_struct(int request, R0GPIOInfo pin);
int gpio_control(int request, int num, int mode, int val);

#endif
