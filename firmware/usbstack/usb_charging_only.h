/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef USB_CHARGING_ONLY_H
#define USB_CHARGING_ONLY_H

#include "usb_ch9.h"

void usb_charging_only_init(void);
int usb_charging_only_set_first_endpoint(int endpoint);
int usb_charging_only_set_first_interface(int interface);
int usb_charging_only_get_config_descriptor(unsigned char *dest,int max_packet_size);
bool usb_charging_only_control_request(struct usb_ctrlrequest* req);

#endif

