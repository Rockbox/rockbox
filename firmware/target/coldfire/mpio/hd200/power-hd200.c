/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include "kernel.h"
#include "system.h"
#include "lcd.h"
#include "power.h"

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    (void)status;
    if (status)
    {
        and_l(~(1<<17), &GPIO1_OUT);
    }
    else
    {
        or_l((1<<17), &GPIO1_OUT);
    }

    return status;
}
#endif /* #if CONFIG_TUNER */

void power_init(void)
{
    /* GPIO53 has to be high - low resets device */
    /* GPIO49 is FM related */
    or_l((1<<21)|(1<<17), &GPIO1_OUT);
    or_l((1<<21)|(1<<17), &GPIO1_ENABLE);
    or_l((1<<21)|(1<<17)|(1<<14), &GPIO1_FUNCTION);

    and_l(~(1<<15), &GPIO_OUT);
    or_l((1<<15),&GPIO_ENABLE);
    or_l((1<<15),&GPIO_FUNCTION);

    or_l((1<<23), &GPIO_OUT);
    and_l(~(1<<23), &GPIO_ENABLE);
    or_l((1<<23), &GPIO_FUNCTION);

#ifndef BOOTLOADER
    /* The boot loader controls the power */
    ide_power_enable(true);
#endif
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;
/* GPIO46 is AC plug detect (low = AC plugged) */
    if (!(GPIO1_READ & (1<<14)))
            status |= POWER_INPUT_MAIN_CHARGER;

    return status;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    if (!(GPIO1_READ & (1<<14)))
        return (GPIO_READ & (1<<30) )?false:true;
    else
        return false;
}

void ide_power_enable(bool on)
{
   (void)on;
   if (on)
        and_l(~(1<<31),&GPIO_OUT);
    else
        or_l((1<<31),&GPIO_OUT);

    or_l((1<<31),&GPIO_ENABLE);
    or_l((1<<31),&GPIO_FUNCTION);

}

bool ide_powered(void)
{
    return true;
}

void power_off(void)
{
    lcd_shutdown();
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~(1<<21), &GPIO1_OUT); /* pull KEEPACT low */
    asm("halt");
    while(1);
}
