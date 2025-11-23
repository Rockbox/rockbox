/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
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
#pragma once
#include "usb_core.h"
#include "usb_class_driver.h"

/* [2] P.32 Table 2-8 USB Device Vendor Request to set available current from accessory (USB Device Mode only) */
#define USB_REQ_APPLE_SET_AVAIL_CURRENT (0x40)

extern struct usb_class_driver_ep_allocation usb_iap_ep_allocs[2];

int  usb_iap_request_endpoints(struct usb_class_driver*);
int  usb_iap_set_first_interface(int interface);
int  usb_iap_get_config_descriptor(unsigned char* dest, int max_packet_size);
void usb_iap_init_connection(void);
bool usb_iap_set_alt_interface(int interface, int alt);
int usb_iap_get_alt_interface(int interface);
void usb_iap_init(void);
void usb_iap_disconnect(void);
void usb_iap_transfer_complete(int ep, int dir, int state, int length);
bool usb_iap_fast_transfer_complete(int ep, int dir, int status, int length);
bool usb_iap_control_request(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest);
int usb_iap_set_interface(int intf, int alt);
int usb_iap_get_interface(int intf);
void usb_iap_notify_event(intptr_t data);
