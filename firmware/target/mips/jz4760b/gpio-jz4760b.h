/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 * Copyright (C) 2016 by Amaury Pouly
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
#ifndef __GPIO_JZ4760B_H__
#define __GPIO_JZ4760B_H__

#include "jz4760b_regs.h"

static inline void jz_gpio_set_drive(unsigned bank, unsigned pin, unsigned strength)
{
}

static inline void jz_gpio_set_output(unsigned bank, unsigned pin, bool value)
{
    if(value)
        GPIO_OUT_SET(bank) = 1 << pin;
    else
        GPIO_OUT_CLR(bank) = 1 << pin;
}

static inline bool jz_gpio_get_input(unsigned bank, unsigned pin)
{
    return (GPIO_IN(bank) >> pin) & 1;
}

#define __PIN_FUNCTION  (1 << 0)
#define __PIN_SELECT    (1 << 1)
#define __PIN_DIR       (1 << 2)
#define __PIN_TRIGGER   (1 << 3)

#define PIN_GPIO_OUT    __PIN_DIR
#define PIN_GPIO_IN     0

static inline void jz_gpio_set_function(unsigned bank, unsigned pin, unsigned function)
{
    /* NOTE the encoding of function gives the proper values to write */
    if(function & __PIN_FUNCTION)
        GPIO_FUNCTION_SET(bank) = 1 << pin;
    else
        GPIO_FUNCTION_CLR(bank) = 1 << pin;
    if(function & __PIN_SELECT)
        GPIO_SELECT_SET(bank) = 1 << pin;
    else
        GPIO_SELECT_CLR(bank) = 1 << pin;
    if(function & __PIN_DIR)
        GPIO_DIR_SET(bank) = 1 << pin;
    else
        GPIO_DIR_CLR(bank) = 1 << pin;
    if(function & __PIN_TRIGGER)
        GPIO_TRIGGER_SET(bank) = 1 << pin;
    else
        GPIO_TRIGGER_CLR(bank) = 1 << pin;
}

static inline void jz_gpio_enable_pullup(unsigned bank, unsigned pin, bool enable)
{
    /* warning: register meaning is "disable pullup" */
    if(enable)
        GPIO_PULL_CLR(bank) = 1 << pin;
    else
        GPIO_PULL_SET(bank) = 1 << pin;
}

#endif /* __GPIO_JZ4760B_H__ */
