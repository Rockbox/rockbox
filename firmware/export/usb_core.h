/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Björn Stenberg
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
#ifndef USB_CORE_H
#define USB_CORE_H

#ifndef BOOTLOADER

//#define USB_SERIAL
#define USB_STORAGE
#define USB_CHARGING_ONLY
#else /* BOOTLOADER */
#define USB_CHARGING_ONLY
#endif /* BOOTLOADER */

#include "usb_ch9.h"
#include "usb.h"

/* endpoints */
#define EP_CONTROL 0
#if CONFIG_CPU == IMX31L
#define NUM_ENDPOINTS 8
#define USBDEVBSS_ATTR  DEVBSS_ATTR
#else
#define USBDEVBSS_ATTR  IBSS_ATTR
#define NUM_ENDPOINTS 3
#endif

extern int usb_max_pkt_size;

struct usb_class_driver;

void usb_core_init(void);
void usb_core_exit(void);
void usb_core_control_request(struct usb_ctrlrequest* req);
void usb_core_transfer_complete(int endpoint, int dir, int status, int length);
void usb_core_bus_reset(void);
bool usb_core_any_exclusive_storage(void);
void usb_core_enable_driver(int driver,bool enabled);
bool usb_core_driver_enabled (int driver);
void usb_core_handle_transfer_completion(
                             struct usb_transfer_completion_event_data* event);
int usb_core_ack_control(struct usb_ctrlrequest* req);

int usb_core_request_endpoint(int dir, struct usb_class_driver *drv);
void usb_core_release_endpoint(int dir);

#ifdef HAVE_HOTSWAP
void usb_core_hotswap_event(int volume,bool inserted);
#endif

#ifdef HAVE_USB_POWER
unsigned short usb_allowed_current(void);
#endif

#endif

