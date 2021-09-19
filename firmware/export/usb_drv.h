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
#include "kernel.h"

/** USB initialisation flow:
 * usb_init()
 *    -> usb_init_device()
 *       -> [soc specific one-time init]
 *       -> usb_drv_startup()
 * .....
 * [USB is plugged]
 * usb_enable(true)
 *    -> soc specific controller/clock init
 *    -> usb_core_init()
 *       -> usb_drv_init()
 *          -> usb_drv_int_enable(true)  [only if controller needs soc specific code for interrupt]
 *       -> for each usb driver, driver.init()
 * #ifdef USB_DETECT_BY_REQUEST
 *   [rockbox waits until first control request before proceeding]
 * #endif
 * [rockbox decides which usb drivers to enable, based on user preference and buttons]
 * -> if not exclusive mode, usb_attach()
 * -> if exclusive mode, usb_attach() call be called at any point starting from now
 *    (but after threads have acked usb mode and disk have been unmounted)
 * for each enabled driver
 *    -> driver.request_endpoints()
 *    -> driver.set_first_interface()
 * [usb controller/core start answering requests]
 * .....
 * [USB is unplugged]
 * usb_enable(false)
 *    -> usb_core_exit()
 *       -> for each enabled usb driver, driver.disconnect()
 *       -> usb_drv_exit()
 *          -> usb_drv_int_enable(false)  [ditto]
 *    -> soc specific controller/clock deinit */

enum usb_control_response {
    USB_CONTROL_ACK,
    USB_CONTROL_STALL,
    USB_CONTROL_RECEIVE,
};

/* one-time initialisation of the USB driver */
void usb_drv_startup(void);
void usb_drv_int_enable(bool enable); /* Target implemented */
/* enable and initialise the USB controller */
void usb_drv_init(void);
/* stop and disable and the USB controller */
void usb_drv_exit(void);
void usb_drv_int(void); /* Call from target INT handler */
void usb_drv_stall(int endpoint, bool stall,bool in);
bool usb_drv_stalled(int endpoint,bool in);
int usb_drv_send(int endpoint, void* ptr, int length);
int usb_drv_send_nonblocking(int endpoint, void* ptr, int length);
int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length);
void usb_drv_control_response(enum usb_control_response resp,
                              void* data, int length);
void usb_drv_set_address(int address);
void usb_drv_reset_endpoint(int endpoint, bool send);
bool usb_drv_powered(void);
int usb_drv_port_speed(void);
void usb_drv_cancel_all_transfers(void);
void usb_drv_set_test_mode(int mode);
bool usb_drv_connected(void);
int usb_drv_request_endpoint(int type, int dir);
void usb_drv_release_endpoint(int ep);

/* USB_STRING_INITIALIZER(u"Example String") */
#define USB_STRING_INITIALIZER(S) { \
    sizeof(struct usb_string_descriptor) + sizeof(S) - sizeof(*S), \
    USB_DT_STRING, \
    S \
}

#endif /* _USB_DRV_H */
