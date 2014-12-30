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
#include "config.h"
#include "system.h"
#include "usb.h"
#ifdef HAVE_USBSTACK
#include "usb_core.h"
#endif
#include "power.h"
#include "as3525.h"
#include "usb_drv.h"

static int usb_status = USB_EXTRACTED;

void usb_enable(bool on)
{
#if defined(HAVE_USBSTACK)
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
    usb_status = USB_INSERTED;
#ifdef USB_STATUS_BY_EVENT
    usb_status_event(USB_INSERTED);
#endif
}

void usb_remove_int(void)
{
    usb_status = USB_EXTRACTED;
#ifdef USB_STATUS_BY_EVENT
    usb_status_event(USB_EXTRACTED);
#endif
}

int usb_detect(void)
{
    return usb_status;
}

void usb_init_device(void)
{
}
