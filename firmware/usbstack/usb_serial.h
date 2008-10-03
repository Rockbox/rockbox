/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
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
#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "usb_ch9.h"

int usb_serial_request_endpoints(struct usb_class_driver *);
int usb_serial_set_first_interface(int interface);
int usb_serial_get_config_descriptor(unsigned char *dest,int max_packet_size);
void usb_serial_init_connection(void);
void usb_serial_init(void);
void usb_serial_disconnect(void);
void usb_serial_transfer_complete(int ep,int dir, int status, int length);
bool usb_serial_control_request(struct usb_ctrlrequest* req);

void usb_serial_send(unsigned char *data,int length);

#endif

