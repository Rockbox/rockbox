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

#ifndef _USBSTACK_DEVICE_H_
#define _USBSTACK_DEVICE_H_

/*
 * usb device driver
 */
struct usb_device_driver {
    const char* name;
    int (*bind)(void* controller_ops);
    void (*unbind)(void);
    int (*request)(struct usb_ctrlrequest* req);
    void (*suspend)(void);
    void (*resume)(void);
    void (*speed)(enum usb_device_speed speed);
    void* data; /* used to store controller specific ops struct */
};

int usb_device_driver_register(struct usb_device_driver* driver);

/* forward declaration */
struct usb_config_descriptor;
struct usb_descriptor_header;

int usb_stack_configdesc(const struct usb_config_descriptor* config,
                         void* buf, unsigned length,
                         struct usb_descriptor_header** desc);

#endif /*_USBSTACK_DEVICE_H_*/
