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
#ifndef _USB_DRV_H
#define _USB_DRV_H

#include "usb_ch9.h"

#define EP_CONTROL 0

#define DIR_OUT 0
#define DIR_IN 1

#define EP_DIR(ep) (((ep) & USB_ENDPOINT_DIR_MASK) ? DIR_IN : DIR_OUT)
#define EP_NUM(ep) ((ep) & USB_ENDPOINT_NUMBER_MASK)

void usb_drv_init(void);
void usb_drv_exit(void);
void usb_drv_stall(int endpoint, bool stall,bool in);
int usb_drv_send(int endpoint, void* ptr, int length);
int usb_drv_send_nonblocking(int endpoint, void* ptr, int length);
int usb_drv_recv(int endpoint, void* ptr, int length);// blocking !
int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length);
int usb_drv_recv_setup(struct usb_ctrlrequest *req);
void usb_drv_set_address(int address);
int usb_drv_port_speed(void);
void usb_drv_configure_endpoint(int ep_num, int type);

/* USB_STRING_INITIALIZER(u"Example String") */
#define USB_STRING_INITIALIZER(S) { \
    sizeof(struct usb_string_descriptor) + sizeof(S) - sizeof(*S), \
    USB_DT_STRING, \
    S \
}

#endif /* _USB_DRV_H */
 
