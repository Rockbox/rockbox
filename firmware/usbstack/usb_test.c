/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "string.h"
#include "system.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "usb_class_driver.h"
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "storage.h"
#include "disk.h"
#include "fat.h"
/* Needed to get at the audio buffer */
#include "audio.h"
#include "usb_storage.h"
#if CONFIG_RTC
#include "timefuncs.h"
#endif
#include "core_alloc.h"
#include "panic.h"

#define READ_BUFFER_SIZE (1024*64)
#define WRITE_BUFFER_SIZE (1024*64)

#define ALLOCATE_BUFFER_SIZE MAX(READ_BUFFER_SIZE,WRITE_BUFFER_SIZE)

static struct usb_interface_descriptor __attribute__((aligned(2)))
                                       interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
    .bInterfaceSubClass = 0xff,
    .bInterfaceProtocol = 0,
    .iInterface         = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2)))
                                      endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

static int usb_interface;
static int ep_in, ep_out;
static void *data_buffer;

/* called by usb_core_init() */
void usb_test_init(void)
{
    logf("usb_test_init done");
}

int usb_test_request_endpoints(struct usb_class_driver *drv)
{
    ep_in = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN, drv);

    if(ep_in<0)
        return -1;

    ep_out = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT,
            drv);

    if(ep_out<0) {
        usb_core_release_endpoint(ep_in);
        return -1;
    }

    return 0;
}

int usb_test_set_first_interface(int interface)
{
    usb_interface = interface;
    return interface + 1;
}

int usb_test_get_config_descriptor(unsigned char *dest,int max_packet_size)
{
    unsigned char *orig_dest = dest;

    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DATA(&dest, interface_descriptor);

    endpoint_descriptor.wMaxPacketSize = max_packet_size;

    endpoint_descriptor.bEndpointAddress = ep_in;
    PACK_DATA(&dest, endpoint_descriptor);

    endpoint_descriptor.bEndpointAddress = ep_out;
    PACK_DATA(&dest, endpoint_descriptor);

    return (dest - orig_dest);
}

static int usb_handle;
void usb_test_init_connection(void)
{
    static struct buflib_callbacks dummy_ops;
    // Add 31 to handle worst-case misalignment
    usb_handle = core_alloc_ex("usb storage", ALLOCATE_BUFFER_SIZE + 31, &dummy_ops);
    if (usb_handle < 0)
        panicf("%s(): OOM", __func__);

    unsigned char *buffer = core_get_data(usb_handle);
#if defined(UNCACHED_ADDR) && CONFIG_CPU != AS3525
    data_buffer = (void *)UNCACHED_ADDR((unsigned int)(buffer+31) & 0xffffffe0);
#else
    data_buffer = (void *)((unsigned int)(buffer+31) & 0xffffffe0);
#endif
    commit_discard_dcache();

    usb_drv_recv_loop(ep_out, data_buffer, WRITE_BUFFER_SIZE);
    usb_drv_send_nonblocking(ep_in, data_buffer, READ_BUFFER_SIZE);
}

void usb_test_disconnect(void)
{
    if (usb_handle > 0)
        usb_handle = core_free(usb_handle);
}

/* called by usb_core_transfer_complete() */
void usb_test_transfer_complete(int ep,int dir,int status,int length)
{
    (void) ep;
    (void) dir;
    (void) status;
    (void) length;
}

/* called by usb_core_control_request() */
bool usb_test_control_request(struct usb_ctrlrequest* req, unsigned char* dest)
{
    (void) req;
    (void) dest;
    return false;
}
