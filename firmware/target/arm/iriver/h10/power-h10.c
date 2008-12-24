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

#if CONFIG_TUNER

bool tuner_power(bool status)
{
    (void)status;
    /* TODO: tuner power control */
    return true;
}

#endif /* #if CONFIG_TUNER */

void power_init(void)
{
}

unsigned int power_input_status(void)
{
    /* No separate source for USB and charges from USB on its own */
    return (GPIOF_INPUT_VAL & 0x08) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

void ide_power_enable(bool on)
{
    if(on){
        GPIO_CLEAR_BITWISE(GPIOF_OUTPUT_VAL, 0x01);
        DEV_EN |= DEV_IDE0;
    } else {
        DEV_EN &= ~DEV_IDE0;
        GPIO_SET_BITWISE(GPIOF_OUTPUT_VAL, 0x01);
    }
}


bool ide_powered(void)
{
    return ((GPIOF_INPUT_VAL & 0x1) == 0);
}

void power_off(void)
{
    GPIOF_OUTPUT_VAL &=~ 0x20;
    while(1);
}
