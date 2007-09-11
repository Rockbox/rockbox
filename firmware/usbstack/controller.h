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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _USBSTACK_CONTROLLER_H_
#define _USBSTACK_CONTROLLER_H_

struct usb_controller {
    const char* name;
    enum usb_controller_type type;
    enum usb_device_speed speed;
    void (*init)(void);
    void (*shutdown)(void);
    void (*irq)(void);
    void (*start)(void);
    void (*stop)(void);
    void* controller_ops;
    struct usb_device_driver* device_driver;
    struct usb_host_driver* host_driver;
    struct usb_ep* ep0;
    struct usb_ep endpoints;
};

struct usb_dcd_controller_ops {
    /* endpoint management */
    int (*enable)(struct usb_ep* ep, struct usb_endpoint_descriptor* desc);
    int (*disable)(struct usb_ep* ep);
    int (*set_halt)(struct usb_ep* ep, bool hald);

    /* transmitting */
    int (*send)(struct usb_ep* ep, struct usb_response* req);
    int (*receive)(struct usb_ep* ep, struct usb_response* res);

    /* ep0 */
    struct usb_ep* ep0;
};

int usb_controller_register(struct usb_controller* ctrl);
int usb_controller_unregister(struct usb_controller* ctrl);

/*
 * dcd - device controller driver
 */
void usb_dcd_init(void);
void usb_dcd_shutdown(void);

/*
 * hcd - host controller driver
 */
void usb_hcd_init(void);
void usb_hcd_shutdown(void);

#endif /*_USBSTACK_CONTROLLER_H_*/
