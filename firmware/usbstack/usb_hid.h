/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Tomer Shalev
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
#ifndef USB_HID_H
#define USB_HID_H

#include "usb_ch9.h"
#include "usb_core.h"
#include "usb_hid_usage_tables.h"

int usb_hid_request_endpoints(struct usb_class_driver *drv);
int usb_hid_set_first_interface(int interface);
int usb_hid_get_config_descriptor(unsigned char *dest, int max_packet_size);
void usb_hid_init_connection(void);
void usb_hid_init(void);
void usb_hid_disconnect(void);
void usb_hid_transfer_complete(int ep, int dir, int status, int length);
bool usb_hid_control_request(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest);

void usb_hid_send(usage_page_t usage_page, int id);

#endif

