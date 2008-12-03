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
#include "pcf50606.h"
#include "usb.h"
#include "logf.h"


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
    pcf50606_init();
}


#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    if (GPIO1_READ & 0x00400000)
        status |= POWER_INPUT_MAIN_CHARGER;

#ifdef HAVE_USB_POWER
    if (usb_detect() == USB_INSERTED && pcf50606_usb_charging_enabled())
        status |= POWER_INPUT_USB_CHARGER;
    /* CHECK: Can the device be powered from USB w/o charging it? */
#endif

    return status;
}

#ifdef HAVE_USB_POWER
bool usb_charging_enable(bool on)
{
    bool rc = false;
    int irqlevel;
    logf("usb_charging_enable(%s)\n", on ? "on" : "off" );
    irqlevel = disable_irq_save();
    pcf50606_set_usb_charging(on);
    rc = on;
    restore_irq(irqlevel);
    return rc;
}
#endif /* HAVE_USB_POWER */

#endif /* CONFIG_CHARGING */

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return (GPIO_READ & 0x00800000)?true:false;
}

bool usb_charging_enabled(void)
{
    bool rc = false;
    /* TODO: read the state of the GPOOD2 register...
     * (this also means to set the irq level here) */
    rc = pcf50606_usb_charging_enabled();

    logf("usb charging %s", rc ? "enabled" : "disabled" );
    return rc;
}

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
