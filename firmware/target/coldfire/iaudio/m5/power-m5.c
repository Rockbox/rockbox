/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "power.h"
#include "pcf50606.h"
#include "lcd-remote-target.h"

#ifndef SIMULATOR

void power_init(void)
{
    /* Charger detect */
    and_l(~0x01000000, &GPIO1_ENABLE);
    or_l(0x01000000, &GPIO1_FUNCTION);
    
    pcf50606_init();
}

bool charger_inserted(void)
{     
    return (GPIO1_READ & 0x01000000) != 0;
}

void ide_power_enable(bool on)
{
    /* GPOOD3 */
    int level = disable_irq_save();
    pcf50606_write(0x3c, on ? 0x07 : 0x00);
    restore_irq(level);
}

bool ide_powered(void)
{
    int level = disable_irq_save();
    int value = pcf50606_read(0x3c);
    restore_irq(level);
    return (value & 0x07) != 0;
}

void power_off(void)
{
    lcd_remote_poweroff();
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~0x00000008, &GPIO_OUT); /* Set KEEPACT low */
    asm("halt");
}

#endif /* SIMULATOR */
