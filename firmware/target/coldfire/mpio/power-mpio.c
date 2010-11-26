/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
    if (status)
        and_l(~(1<<17), &GPIO1_OUT);
    else
        or_l((1<<17), &GPIO1_OUT);

    return status;
}
#endif /* #if CONFIG_TUNER */

void power_init(void)
{
    /* GPIO53 has to be high - low resets device
     * it is setup in crt0.S
     */

    /* GPIO50 is ATA power (default OFF) */
    or_l((1<<18), &GPIO1_OUT);
    or_l((1<<18), &GPIO1_ENABLE);
    or_l((1<<18), &GPIO1_FUNCTION);

    /* GPIO49 is FM power (default OFF) */
    or_l((1<<17), &GPIO1_OUT);
    or_l((1<<17), &GPIO1_ENABLE);
    or_l((1<<17), &GPIO1_FUNCTION);

    /* GPIO46 is wall charger detect (input) */
    and_l(~(1<<14), &GPIO1_ENABLE);
    or_l((1<<14), &GPIO1_FUNCTION);

    /* GPIO31 needs to be low */
    and_l(~(1<<31), &GPIO_OUT);
    or_l((1<<31),&GPIO_ENABLE);
    or_l((1<<31),&GPIO_FUNCTION);
   
    /* turn off charger by default*/
    or_l((1<<23), &GPIO_OUT);
    or_l((1<<23), &GPIO_ENABLE);
    or_l((1<<23), &GPIO_FUNCTION);

    /* high current charge mode */
    or_l((1<<15), &GPIO_OUT);
    or_l((1<<15),&GPIO_ENABLE);
    or_l((1<<15),&GPIO_FUNCTION);

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
    {
        status |= POWER_INPUT_MAIN_CHARGER;
        /* tristate GPIO23 to start charging cycle */
        and_l(~(1<<23), &GPIO_ENABLE);
    }
    else
    {
        /* drive GPIO23 high to enter LTC1733 shutdown mode */
        or_l((1<<23), &GPIO_ENABLE);
    }
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
    if (on)
        and_l(~(1<<18),&GPIO1_OUT);
    else
        or_l((1<<18),&GPIO1_OUT);
}

bool ide_powered(void)
{
    return (GPIO1_OUT & (1<<18))?false:true;
}

void power_off(void)
{
#ifdef HAVE_LCD_SHUTDOWN
    lcd_shutdown();
#endif
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~(1<<21), &GPIO1_OUT); /* pull KEEPACT low */
    asm("halt");
    while(1);
}
