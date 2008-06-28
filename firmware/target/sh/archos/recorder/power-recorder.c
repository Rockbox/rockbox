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
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "usb.h"

bool charger_enabled;

void power_init(void)
{
    PBCR2 &= ~0x0c00;    /* GPIO for PB5 */
    or_b(0x20, &PBIORL); /* Set charging control bit to output */
    charger_enable(false); /* Default to charger OFF */
}

bool charger_inserted(void)
{
    /* Recorder */
    return adc_read(ADC_EXT_POWER) > 0x100;
}

void charger_enable(bool on)
{
    if(on)
    {
        and_b(~0x20, &PBDRL);
        charger_enabled = 1;
    } 
    else 
    {
        or_b(0x20, &PBDRL);
        charger_enabled = 0;
    }
}

void ide_power_enable(bool on)
{
    bool touched = false;

    if(on)
    {
        or_b(0x20, &PADRL);
        touched = true;
    }
#ifdef HAVE_ATA_POWER_OFF
    if(!on)
    {
        and_b(~0x20, &PADRL);
        touched = true;
    }
#endif /* HAVE_ATA_POWER_OFF */

/* late port preparation, else problems with read/modify/write 
   of other bits on same port, while input and floating high */
    if (touched)
    {
        or_b(0x20, &PAIORL); /* PA5 is an output */
        PACR2 &= 0xFBFF; /* GPIO for PA5 */
    }
}


bool ide_powered(void)
{
#ifdef HAVE_ATA_POWER_OFF
    if ((PACR2 & 0x0400) || !(PAIORL & 0x20)) /* not configured for output */
        return true; /* would be floating high, disk on */
    else
        return (PADRL & 0x20) != 0;
#else /* !defined(NEEDS_ATA_POWER_ON) && !defined(HAVE_ATA_POWER_OFF) */
    return true; /* pretend always powered if not controlable */
#endif
}

void power_off(void)
{
    disable_irq();
    and_b(~0x10, &PBDRL);
    or_b(0x10, &PBIORL);
    while(1);
}
