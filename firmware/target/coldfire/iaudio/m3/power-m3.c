/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
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
#include "lcd.h"
#include "power.h"
#include "system.h"

void power_init(void)
{   
    /* Set KEEPACT */
    or_l(0x00040000, &GPIO_OUT); 
    or_l(0x00040000, &GPIO_ENABLE);
    or_l(0x00040000, &GPIO_FUNCTION);

    /* Charger detect */
    and_l(~0x00000020, &GPIO1_ENABLE);
    or_l(0x00000020, &GPIO1_FUNCTION);

#ifndef BOOTLOADER
    /* FIXME: Just disable the multi-colour LED for now. */
    and_l(~0x00000210, &GPIO1_OUT);
    and_l(~0x00008000, &GPIO_OUT);
#endif
}

unsigned int power_input_status(void)
{
    return ((GPIO1_READ & 0x00000020) == 0) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

void ide_power_enable(bool on)
{
    if (on)
    {
        or_l(0x00800000, &GPIO_OUT);
    }
    else
    {
        and_l(~0x00800000, &GPIO_OUT);
    }
}

bool ide_powered(void)
{
    return (GPIO_OUT & 0x00800000) != 0;
}

void power_off(void)
{
    lcd_poweroff();
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~0x00040000, &GPIO_OUT); /* Set KEEPACT low */
    asm("halt");
}

bool tuner_power(bool status)
{
    (void)status;
    return true;
}
