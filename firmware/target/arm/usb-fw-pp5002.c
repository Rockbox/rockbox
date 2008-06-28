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
#include "usb-target.h"

void usb_init_device(void)
{
    /* TODO: add USB init for iPod 3rd gen */

#if defined(IPOD_1G2G) || defined(IPOD_3G)
    /* GPIO C bit 7 is firewire detect */
    GPIOC_ENABLE    |=  0x80;
    GPIOC_OUTPUT_EN &= ~0x80;
#endif
}

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
#if defined(IPOD_1G2G) || defined(IPOD_3G)
    /* GPIO C bit 7 is firewire detect */
    if (!(GPIOC_INPUT_VAL & 0x80))
        return USB_INSERTED;
#endif

    /* TODO: add USB detection for iPod 3rd gen */

    return USB_EXTRACTED;
}
