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
#include "usb-target.h"
#include "power.h"
#include "as3525.h"

static int usb_status = USB_EXTRACTED;

#if CONFIG_CPU == AS3525v2 && !defined(USE_ROCKBOX_USB)
/* Rebooting on USB plug can crash these players in a state where
 * hardware power off (pressing the power button) doesn't work anymore
 * TODO: Implement USB in rockbox for these players */
#define USB_INSERT_INT_STATUS   USB_EXTRACTED
#undef USB_DETECT_BY_DRV
#undef USB_DETECT_BY_CORE
#undef USB_STATUS_BY_EVENT

#else /* !AS3525v2 */

#if defined(USB_DETECT_BY_DRV) || defined(USB_DETECT_BY_CORE)

#ifdef USB_STATUS_BY_EVENT
#define USB_INSERT_INT_STATUS   USB_INSERTED
#define USB_INSERT_INT_EVENT    USB_POWERED
#define USB_REMOVE_INT_EVENT    USB_UNPOWERED
#else
#define USB_INSERT_INT_STATUS   USB_POWERED
#endif /* USB_STATUS_BY_EVENT */

#else /* !USB_DETECT_BY_* */

#define USB_INSERT_INT_STATUS   USB_INSERTED
#ifdef USB_STATUS_BY_EVENT
#define USB_INSERT_INT_EVENT    USB_INSERTED
#define USB_REMOVE_INT_EVENT    USB_EXTRACTED
#endif /* USB_STATUS_BY_EVENT */

#endif /* USB_DETECT_BY_* */

#endif /* AS3525v2 */

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
    usb_status = USB_INSERT_INT_STATUS;
#ifdef USB_STATUS_BY_EVENT
    usb_status_event(USB_INSERT_INT_EVENT);
#endif
}

void usb_remove_int(void)
{
    usb_status = USB_EXTRACTED;
#ifdef USB_STATUS_BY_EVENT
    usb_status_event(USB_REMOVE_INT_EVENT);
#endif
}

void usb_drv_usb_detect_event(void)
{
#if defined(USB_DETECT_BY_DRV) || defined(USB_DETECT_BY_CORE)
    int oldstatus = disable_irq_save(); /* May come via USB thread */
    if (usb_status == USB_INSERT_INT_STATUS)
    {
        usb_status = USB_INSERTED;
#ifdef USB_STATUS_BY_EVENT
        usb_status_event(USB_INSERTED);
#endif
    }
    restore_irq(oldstatus);
#endif /* USB_DETECT_BY_* */
}

int usb_detect(void)
{
    return usb_status;
}
