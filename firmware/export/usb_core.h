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

#include "usb_ch9.h"
#include "usb.h"

/* endpoints */
#define EP_CONTROL 0

#define DIR_OUT 0
#define DIR_IN 1

/* The USB core is a device, and OUT is RX from that P.O.V */
#define DIR_RX DIR_OUT
#define DIR_TX DIR_IN

#define EP_DIR(ep) (((ep) & USB_ENDPOINT_DIR_MASK) ? DIR_IN : DIR_OUT)
#define EP_NUM(ep) ((ep) & USB_ENDPOINT_NUMBER_MASK)

extern int usb_max_pkt_size;

enum {
    USB_STRING_INDEX_LANGUAGE,
    USB_STRING_INDEX_MANUFACTURER,
    USB_STRING_INDEX_PRODUCT,
    USB_STRING_INDEX_SERIAL,
    USB_STRING_INDEX_MAX,
};

struct usb_class_driver;

void usb_core_init(void);
void usb_core_exit(void);
void usb_core_control_request(struct usb_ctrlrequest* req, void* data);
void usb_core_control_complete(int status);
void usb_core_legacy_control_request(struct usb_ctrlrequest* req);
void usb_core_transfer_complete(int endpoint,int dir,int status,int length);
void usb_core_bus_reset(void);
bool usb_core_any_exclusive_storage(void);
void usb_core_enable_driver(int driver,bool enabled);
bool usb_core_driver_enabled(int driver);
void usb_core_handle_transfer_completion(
        struct usb_transfer_completion_event_data* event);
void usb_core_handle_notify(long id, intptr_t data);
/* For controllers which handle SET ADDR and/or SET CONFIG in hardware */
void usb_core_notify_set_address(uint8_t addr);
void usb_core_notify_set_config(uint8_t config);

int usb_core_request_endpoint(int type, int dir,struct usb_class_driver* drv);
void usb_core_release_endpoint(int dir);

#ifdef HAVE_HOTSWAP
void usb_core_hotswap_event(int volume,bool inserted);
#endif

#endif

