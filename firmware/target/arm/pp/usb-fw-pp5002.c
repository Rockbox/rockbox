/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
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
#include "ata.h"
#include "cpu.h"
#include "string.h"
#include "system.h"
#include "usb.h"

void usb_pin_init(void)
{
#if defined(IPOD_1G2G)
    /* GPIO C bit 7 is firewire detect */
    GPIOC_ENABLE    |=  0x80;
    GPIOC_OUTPUT_EN &= ~0x80;
#elif defined(IPOD_3G)
    /* GPIO D bit 4 is USB detect */
    GPIOD_ENABLE    |=  0x10;
    GPIOD_OUTPUT_EN &= ~0x10;
#endif
}

/* No different for now */
void usb_init_device(void) __attribute__((alias("usb_pin_init")));

void usb_enable(bool on)
{
    /* This device specific code will eventually give way to proper USB
       handling, which should be the same for all PP5002 targets. */
    if (on)
    {
#ifdef IPOD_ARCH
    /* For iPod, we can only do one thing with USB mode atm - reboot
       into the flash-based disk-mode.  This does not return. */

        ata_sleepnow(); /* Immediately spindown the disk. */
        sleep(HZ*2);

        memcpy((void *)0x40017f00, "diskmodehotstuff\1", 17);

        system_reboot(); /* Reboot */
#endif
    }
}

int usb_detect(void)
{
#if defined(IPOD_1G2G)
    /* GPIO C bit 7 is firewire detect */
    if (!(GPIOC_INPUT_VAL & 0x80))
        return USB_INSERTED;
#endif

#if defined(IPOD_3G)
    /* GPIO D bit 4 is USB detect */
    if (GPIOD_INPUT_VAL & 0x10)
        return USB_INSERTED;
#endif

    return USB_EXTRACTED;
}
bool usb_plugged(void) __attribute__((alias("usb_detect")));
