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

#ifdef HAVE_USBSTACK
#include "usb_core.h"
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
#else
void usb_enable(bool on)
{
    (void)on;
}

int usb_detect(void)
{
    return USB_EXTRACTED;
}
#endif
