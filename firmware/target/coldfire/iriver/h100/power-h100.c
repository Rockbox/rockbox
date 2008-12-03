/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "spdif.h"

#if CONFIG_TUNER
bool tuner_power(bool status)
{
    (void)status;
    return true;
}
#endif /* #if CONFIG_TUNER */

void power_init(void)
{
    or_l(0x00080000, &GPIO1_OUT);
    or_l(0x00080000, &GPIO1_ENABLE);
    or_l(0x00080000, &GPIO1_FUNCTION);

#ifndef BOOTLOADER
    /* The boot loader controls the power */
    ide_power_enable(true);
#endif
    or_l(0x80000000, &GPIO_ENABLE);
    or_l(0x80000000, &GPIO_FUNCTION);
#ifdef HAVE_SPDIF_POWER
    spdif_power_enable(false);
#endif
}

unsigned int power_input_status(void)
{
    return (GPIO1_READ & 0x00400000) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return (power_input_status() & POWER_INPUT_CHARGER) != 0;
}

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on)
{
    or_l(0x01000000, &GPIO1_FUNCTION);
    or_l(0x01000000, &GPIO1_ENABLE);

#ifdef SPDIF_POWER_INVERTED
    if(!on)
#else
    if(on)
#endif
        and_l(~0x01000000, &GPIO1_OUT);
    else
        or_l(0x01000000, &GPIO1_OUT);

#ifndef BOOTLOADER
    /* Make sure the feed is reset */
    spdif_set_output_source(spdif_get_output_source(NULL)
        IF_SPDIF_POWER_(, true));
#endif
}

bool spdif_powered(void)
{
    bool state = (GPIO1_READ & 0x01000000)?false:true;
#ifdef SPDIF_POWER_INVERTED
    return !state;
#else
    return state;
#endif /* SPDIF_POWER_INVERTED */
}
#endif /* HAVE_SPDIF_POWER */

void ide_power_enable(bool on)
{
    if(on)
        and_l(~0x80000000, &GPIO_OUT);
    else
        or_l(0x80000000, &GPIO_OUT);
}

bool ide_powered(void)
{
    return (GPIO_OUT & 0x80000000)?false:true;
}

void power_off(void)
{
    set_irq_level(DISABLE_INTERRUPTS);
    and_l(~0x00080000, &GPIO1_OUT);
    asm("halt");
    while(1);
}
