/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

void power_init(void)
{
}

unsigned int power_input_status(void)
{
    return POWER_INPUT_NONE;
}

void ide_power_enable(bool on)
{
    (void)on;
    /* We do nothing on the iPod */
}


bool ide_powered(void)
{
    /* pretend we are always powered - we don't turn it off on the ipod */
    return true;
}

void power_off(void)
{
    /* Give things a second to settle before cutting power */
    sleep(HZ);
    
    //GPIOF_OUTPUT_VAL &=~ 0x20;
}
