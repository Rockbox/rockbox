/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ascodec-target.h 26116 2010-05-17 20:53:25Z funman $
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

#define GPIO_HEADPHONE_SENSE          GPIO1_5
//26
#define GPIO_EXT_PWR_SENSE            GPIO1_26
//59
#define GPIO_SD_SENSE                 GPIO2_24

void gpio_init(void);
void gpio_close(void);
int gpio_control_struct(int request, R0GPIOInfo pin);
int gpio_control(int request, int num, int mode, int val);

#endif