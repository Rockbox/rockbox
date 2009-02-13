/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "logf.h"
#include "usb.h"

void power_init(void)
{
    /* power off bit */
    GPIOB_ENABLE |= 0x80;
    GPIOB_OUTPUT_VAL |= 0x80;
    GPIOB_OUTPUT_EN |= 0x80;

    /* charger inserted bit */
    GPIOE_ENABLE |= 0x20;
    GPIOE_INPUT_VAL |= 0x20;

#if CONFIG_TUNER
    /* fm antenna? */
    GPIOE_ENABLE |= 0x40;
    GPIOE_OUTPUT_EN |= 0x40;
    GPIOE_OUTPUT_VAL &= ~0x40; /* off */
#endif
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    /* AC charger */
    if (GPIOE_INPUT_VAL & 0x20)
        status |= POWER_INPUT_MAIN_CHARGER;

    /* Do nothing with USB for now
    if (GPIOE_INPUT_VAL & 0x4)
        status |= POWER_INPUT_USB_CHARGER;
    */

    return status;
}

void ide_power_enable(bool on)
{
    (void)on;
    /* We do nothing */
}


bool ide_powered(void)
{
    /* pretend we are always powered - we don't turn it off */
    return true;
}

void power_off(void)
{
    /* power off bit */
    GPIOB_ENABLE |= 0x80;
    GPIOB_OUTPUT_VAL &= ~0x80;
    GPIOB_OUTPUT_EN |= 0x80;
}

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    if (status)
        GPIOE_OUTPUT_VAL |= 0x40;
    else
        GPIOE_OUTPUT_VAL &= ~0x40;

    return status;
}
#endif
