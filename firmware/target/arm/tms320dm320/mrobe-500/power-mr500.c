/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "lcd.h"
#include "pcf50606.h"
#include "backlight.h"
#include "backlight-target.h"
#include "lcd-remote-target.h"

void power_init(void)
{
    /* Initialize IDE power pin */
    /* set GIO17 (ATA power) on and output */
    /*  17: output, non-inverted, no-irq, falling edge, no-chat, normal */
    dm320_set_io(17, false, false, false, false, false, 0x00);
    ide_power_enable(true); /* Power up the drive */
    
    /* Charger detect */
    /*  25: input, non-inverted, no-irq, falling edge, no-chat, normal */
    dm320_set_io(25, false, false, false, false, false, 0x00);
    
    /* Power down pin */
    /*  26: input, non-inverted, no-irq, falling edge, no-chat, normal */
    dm320_set_io(26, false, false, false, false, false, 0x00);
    IO_GIO_BITCLR1  =   1<<10; /* Make sure it is not active */
}

unsigned int power_input_status(void)
{
	/* Charger is active low */
	if(!(IO_GIO_BITSET1&(1<<9)))
	{
		return POWER_INPUT_MAIN_CHARGER;
	}
    return POWER_INPUT_NONE;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) 
{
    return false;
}

void ide_power_enable(bool on)
{
    if (on)
        IO_GIO_BITCLR1  =   (1<<1);
    else
        IO_GIO_BITSET1  =   (1<<1);
}

bool ide_powered(void)
{
    return !(IO_GIO_BITSET1&(1<<1));
}

void power_off(void)
{
    /* turn off backlight and wait for 1 second */
    _backlight_off();
#if defined(HAVE_REMOTE_LCD)
    lcd_remote_sleep();
#endif
    lcd_sleep();
    sleep(HZ);
    /* Hard shutdown */
    IO_GIO_BITSET1  =   1<<10;
}
