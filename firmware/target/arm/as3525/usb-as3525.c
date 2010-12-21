/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

#include <stdbool.h>
#include "config.h"
#include "usb.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif
#include "usb-target.h"
#include "power.h"
#include "as3525.h"

static bool bus_activity = 0;
static bool connected = 0;

void usb_enable(bool on)
{
#if defined(HAVE_USBSTACK) && defined(USE_ROCKBOX_USB)
    if (on)
        usb_core_init();
    else
        usb_core_exit();
#else
    (void)on;
#endif
}

void usb_insert_int(void)
{
    connected = 1;
}

void usb_remove_int(void)
{
    connected = 0;
    bus_activity = 0;
}

void usb_drv_usb_detect_event(void)
{
    /* Bus activity seen */
    bus_activity = 1;
}

int usb_detect(void)
{
#if CONFIG_CPU == AS3525v2 && !defined(USE_ROCKBOX_USB)
    /* Rebooting on USB plug can crash these players in a state where
     * hardware power off (pressing the power button) doesn't work anymore
     * TODO: Implement USB in rockbox for these players */
    return USB_EXTRACTED;
#elif defined(USB_DETECT_BY_DRV)
    if(bus_activity && connected)
        return USB_INSERTED;
    else if(connected)
        return USB_POWERED;
    else
        return USB_UNPOWERED;
#else
    return connected?USB_INSERTED:USB_EXTRACTED;
#endif
}
