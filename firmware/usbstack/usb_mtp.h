/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 James Buren
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
#ifndef USB_MTP_H
#define USB_MTP_H

#include "usb_ch9.h"
#include "usb_core.h"

int usb_mtp_request_endpoints(struct usb_class_driver *drv);
int usb_mtp_set_first_interface(int interface);
int usb_mtp_get_config_descriptor(unsigned char *dest, int max_packet_size);
void usb_mtp_init_connection(void);
void usb_mtp_init(void);
void usb_mtp_disconnect(void);
void usb_mtp_transfer_complete(int ep, int dir, int status, int length);
bool usb_mtp_control_request(struct usb_ctrlrequest* req, unsigned char* dest);

#endif
