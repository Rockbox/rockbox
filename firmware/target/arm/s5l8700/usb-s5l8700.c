/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: usb-fw-pp502x.c 21932 2009-07-17 22:07:06Z roolku $
 *
 * Copyright (C) 2009 by ?????
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
#include "cpu.h"
#include "system.h"
#include "string.h"

void usb_init_device(void)
{
}

void usb_enable(bool on)
{
    /* This device specific code will eventually give way to proper USB
       handling, which should be the same for all S5L870x targets. */
    if (on)
    {
#ifdef IPOD_ARCH
        /* For iPod, we can only do one thing with USB mode atm - reboot
           into the flash-based disk-mode.  This does not return. */

        memcpy((void *)0x0002bf00, "diskmodehotstuff\1\0\0\0", 20);

        system_reboot(); /* Reboot */
#endif
    }
}

int usb_detect(void)
{
#if defined(IPOD_NANO2G)
    if ((PDAT14 & 0x8) == 0x0)
        return USB_INSERTED;
#endif

    return USB_EXTRACTED;
}
