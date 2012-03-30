/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sparmann
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
#include "usb.h"
#include "usb-s3c6400x.h"
#include "cpu.h"

#ifdef HAVE_USBSTACK
#include "usb_core.h"
#include "usb_drv.h"
#include "power.h"

void usb_enable(bool on)
{
    if (on) usb_core_init();
    else usb_core_exit();
}

int usb_detect(void)
{
    if (charger_inserted())
        return USB_INSERTED;
    return USB_EXTRACTED;
}

void usb_init_device(void)
{
    /* Power up the core clocks to allow writing
       to some registers needed to power it down */
    PCGCCTL = 0;
#if CONFIG_CPU==S5L8701
    PWRCON &= ~0x4000;
    PWRCONEXT &= ~0x800;
    INTMSK |= INTMSK_USB_OTG;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) &= ~0x4;
    PWRCON(1) &= ~0x8;
    VIC0INTENABLE |= 1 << 19;
#endif

    usb_drv_exit();
}
#else
void usb_enable(bool on)
{
    (void)on;
}

int usb_detect(void)
{
    return USB_EXTRACTED;
}

void usb_init_device(void)
{
    DCTL = DCTL_pwronprgdone | DCTL_sftdiscon;

    ORSTCON = 1;  /* Put the PHY into reset (needed to get current down) */
    PCGCCTL = 1;  /* Shut down PHY clock */
    OPHYPWR = 0xF;  /* PHY: Power down */

#if CONFIG_CPU==S5L8701
    PWRCON |= 0x4000;
    PWRCONEXT |= 0x800;
#elif CONFIG_CPU==S5L8702
    PWRCON(0) |= 0x4;
    PWRCON(1) |= 0x8;
#endif
}
#endif
