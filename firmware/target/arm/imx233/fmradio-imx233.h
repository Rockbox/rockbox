/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef __fmradio_imx233__
#define __fmradio_imx233__

#include "config.h"
#include "tuner.h"
#include "fmradio_i2c.h"
#include "tuner.h"

/** This driver implements fmradio i2c and tuner power in a generic way. It
 * currently provides two possible implementation of i2c:
 * - hardware using i2c-imx233 driver
 * - software using generic-i2c driver
 * And it provides tuner power by toggling an optional GPIO.
 * It can be tweaked using the following defines
 * and variables:
 * - IMX233_FMRADIO_I2C
 * - IMX233_FMRADIO_POWER
 *
 * The available values of IMX233_FMRADIO_I2C are:
 * - FMI_HW: use hardware i2c driver
 * - FMI_SW: use software i2c driver, needs additional defines:
 *   + FMI_SW_SDA_BANK: the SDA pin bank
 *   + FMI_SW_SDA_PIN: the SDA pin within bank
 *   + FMI_SW_SCL_BANK: the SCL pin bank
 *   + FMI_SW_SCL_PIN: the SCL pin bank
 * The available values of IMX233_FMRADIO_POWER are:
 * - FMP_NONE: tuner has no power control
 * - FMP_GPIO: tuner power is controlled by a GPIO, needs additional defines:
 *   + FMP_GPIO_BANK: pin bank
 *   + FMP_GPIO_PIN: pin within bank
 *   + FMP_GPIO_INVERTED: define if inverted, default is active high
 *   + FMP_GPIO_DELAY: delay to power up/down (in ticks)
 */

/* i2c method */
#define FMI_HW      0
#define FMI_SW      1

/* power method */
#define FMP_NONE    0
#define FMP_GPIO    1

#endif /* __fmradio_imx233__ */
 
