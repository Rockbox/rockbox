/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: arcotg_udc.c 12340 2007-02-16 22:13:21Z barrywardell $
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

#ifndef _USBSTACK_CORE_H_
#define _USBSTACK_CORE_H_

#include "linkedlist.h"
#include "usb_ch9.h"
#include "logf.h"
#include "system.h"

#include "usbstack.h"

/*
 * stack datatypes
 */
struct usb_response {
    void* buf;
    uint32_t length;
};

struct usb_ep {
    const char name[15];
    uint8_t type;
    uint32_t ep_num;    /* which endpoint? */
    uint32_t pipe_num;  /* which pipe? */
    uint32_t maxpacket;
    bool claimed;

    struct usb_endpoint_descriptor *desc;
    struct list_head list;
};

#include "usbstack/controller.h"
#include "usbstack/device.h"
#include "usbstack/host.h"

#define NUM_DRIVERS 3

/*
 * usb core
 */
struct usb_core {
    /* we can have maximum two controllers (one device, one host) */
    struct usb_controller* controller[2];
    struct usb_controller* active_controller;
    /* device driver used by stack */
    struct usb_device_driver* device_driver;
    /* for each type of driver use own array */
    struct usb_host_driver* host_drivers[NUM_DRIVERS];
    struct usb_device_driver* device_drivers[NUM_DRIVERS];
    enum usb_controller_type mode;
    bool running;
};

void usb_stack_irq(void);
void usb_stack_work(void);

/* endpoint configuration */
void usb_ep_autoconfig_reset(void);
struct usb_ep* usb_ep_autoconfig(struct usb_endpoint_descriptor* desc);

/* only used for debug */
void into_usb_ctrlrequest(struct usb_ctrlrequest* request);

extern struct usb_core usbcore;

#endif /*_USBSTACK_CORE_H_*/
