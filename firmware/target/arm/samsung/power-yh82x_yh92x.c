/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Mark Arigo
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

/* Created from power.c using some iPod code, and some custom stuff based on 
   GPIO analysis
*/

#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "logf.h"
#include "usb.h"
#include "fmradio_3wire.h"

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    GPIO_SET_BITWISE(GPIOL_ENABLE, 0x04);
    GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x04);
    if (status)
        GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x04);
    else
        GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x04);
    return status;
}
#endif

void power_init(void)
{
#if CONFIG_TUNER
    fmradio_3wire_init();
#endif
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    if (GPIOL_INPUT_VAL & 0x80)
        status = POWER_INPUT_MAIN_CHARGER;

    if (GPIOD_INPUT_VAL & 0x10)
        status |= POWER_INPUT_USB_CHARGER;

    return status;
}

void ide_power_enable(bool on)
{
#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
    if (on)
    {
        GPIO_CLEAR_BITWISE(GPIOF_OUTPUT_VAL, 0x10);
        DEV_EN |= DEV_IDE0;
    }
    else
    {
        DEV_EN &= ~DEV_IDE0;
        GPIO_SET_BITWISE(GPIOF_OUTPUT_VAL, 0x10);
    }
#else
    (void)on;
    /* We do nothing */
#endif
}

bool ide_powered(void)
{
#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
    return ((GPIOF_INPUT_VAL & 0x10) == 0);
#else
    /* pretend we are always powered - we don't turn it off */
    return true;
#endif
}

void power_off(void)
{
    /* power off bit */
    GPIOK_ENABLE |= 0x40;
    GPIOK_OUTPUT_VAL &= ~0x40;
    GPIOK_OUTPUT_EN |= 0x40;
}
