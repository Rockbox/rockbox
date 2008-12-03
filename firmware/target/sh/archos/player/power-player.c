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

void power_init(void)
{
}

unsigned int power_input_status(void)
{
    /* Player */
    return ((PADR & 1) == 0) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}

void ide_power_enable(bool on)
{
    bool touched = false;

    if(on)
    {
        or_b(0x10, &PBDRL);
        touched = true;
    }
#ifdef HAVE_ATA_POWER_OFF
    if(!on)
    {
        and_b(~0x10, &PBDRL);
        touched = true;
    }
#endif /* HAVE_ATA_POWER_OFF */

/* late port preparation, else problems with read/modify/write 
   of other bits on same port, while input and floating high */
    if (touched)
    {
        or_b(0x10, &PBIORL); /* PB4 is an output */
        PBCR2 &= ~0x0300; /* GPIO for PB4 */
    }
}


bool ide_powered(void)
{
#ifdef HAVE_ATA_POWER_OFF
    /* This is not correct for very old players, since these are unable to
     * control hd power. However, driving the pin doesn't hurt, because it
     * is not connected anywhere */
    if ((PBCR2 & 0x0300) || !(PBIORL & 0x10)) /* not configured for output */
        return false; /* would be floating low, disk off */
    else
        return (PBDRL & 0x10) != 0;
#else /* !defined(NEEDS_ATA_POWER_ON) && !defined(HAVE_ATA_POWER_OFF) */
    return true; /* pretend always powered if not controlable */
#endif
}

void power_off(void)
{
    disable_irq();
    and_b(~0x08, &PADRH);
    or_b(0x08, &PAIORH);
    while(1);
}
