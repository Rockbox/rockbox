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

#if CONFIG_CPU == AS3525
static int usb_status = USB_EXTRACTED;
#else
#ifdef SANSA_FUZEV2
#define USB_DETECT_PIN 3
#endif
#endif

void usb_enable(bool on)
{
#ifdef HAVE_USBSTACK
    if (on)
        usb_core_init();
    else
        usb_core_exit();
#else
    (void)on;
#endif
}

void usb_init_device(void)
{
#ifdef USB_DETECT_PIN
    GPIOA_DIR &= ~(1 << USB_DETECT_PIN); /* set as input */
#endif
}

#if CONFIG_CPU == AS3525
void usb_insert_int(void)
{
    usb_status = USB_INSERTED;
}

void usb_remove_int(void)
{
    usb_status = USB_EXTRACTED;
}

int usb_detect(void)
{
    return usb_status;
}
#else
int usb_detect(void)
{
#ifdef USB_DETECT_PIN
    if (GPIOA_PIN( USB_DETECT_PIN ))
        return USB_INSERTED;
    else
#endif
        return USB_EXTRACTED;
}
#endif
