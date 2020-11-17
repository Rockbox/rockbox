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
#include "string.h"
#include "usb_mtp.h"
#include "usb_class_driver.h"

#define MTP_USB_CLASS    USB_CLASS_STILL_IMAGE
#define MTP_USB_SUBCLASS 0x1
#define MTP_USB_PROTOCOL 0x1

#define MTP_TYPE_UNDEF    0x0000
#define MTP_TYPE_INT8     0x0001
#define MTP_TYPE_UINT8    0x0002
#define MTP_TYPE_INT16    0x0003
#define MTP_TYPE_UINT16   0x0004
#define MTP_TYPE_INT32    0x0005
#define MTP_TYPE_UINT32   0x0006
#define MTP_TYPE_INT64    0x0007
#define MTP_TYPE_UINT64   0x0008
#define MTP_TYPE_INT128   0x0009
#define MTP_TYPE_UINT128  0x000A
#define MTP_TYPE_AINT8    0x4001
#define MTP_TYPE_AUINT8   0x4002
#define MTP_TYPE_AINT16   0x4003
#define MTP_TYPE_AUINT16  0x4004
#define MTP_TYPE_AINT32   0x4005
#define MTP_TYPE_AUINT32  0x4006
#define MTP_TYPE_AINT64   0x4007
#define MTP_TYPE_AUINT64  0x4008
#define MTP_TYPE_AINT128  0x4009
#define MTP_TYPE_AUINT128 0x400A
#define MTP_TYPE_STR      0xFFFF

static int ep_int;
static int ep_bulk_in;
static int ep_bulk_out;
static int usb_interface;

static unsigned char send_buffer[4096];
static int send_buffer_size;
static unsigned char recv_buffer[4096];
static int recv_buffer_size;

static struct usb_interface_descriptor __attribute__((aligned(2)))
    interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 3,
    .bInterfaceClass    = MTP_USB_CLASS,
    .bInterfaceSubClass = MTP_USB_SUBCLASS,
    .bInterfaceProtocol = MTP_USB_PROTOCOL,
    .iInterface         = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2)))
    int_endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2)))
    bulk_endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

int usb_mtp_request_endpoints(struct usb_class_driver *drv)
{
    ep_int = usb_core_request_endpoint(USB_ENDPOINT_XFER_INT, USB_DIR_IN, drv);
    if (ep_int < 0)
        goto error;

    ep_bulk_in = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN, drv);
    if (ep_bulk_in < 0)
        goto error;

    ep_bulk_out = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT, drv);
    if (ep_bulk_out < 0)
        goto error;

    return 0;

error:
    if (ep_int != 0)
        usb_core_release_endpoint(ep_int);
    if (ep_bulk_in != 0)
        usb_core_release_endpoint(ep_bulk_in);
    if (ep_bulk_out != 0)
        usb_core_release_endpoint(ep_bulk_out);
    return 1;
}

int usb_mtp_set_first_interface(int interface)
{
    usb_interface = interface;
    return interface + 1;
}

int usb_mtp_get_config_descriptor(unsigned char *dest, int max_packet_size)
{
    unsigned char *orig_dest = dest;

    /* interface descriptor */
    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DATA(&dest, interface_descriptor);

    /* interrupt endpoint */
    int_endpoint_descriptor.wMaxPacketSize = 8;
    int_endpoint_descriptor.bInterval = 8;
    int_endpoint_descriptor.bEndpointAddress = ep_int;
    PACK_DATA(&dest, int_endpoint_descriptor);

    bulk_endpoint_descriptor.wMaxPacketSize = max_packet_size;

    /* bulk input endpoint */
    bulk_endpoint_descriptor.bEndpointAddress = ep_bulk_in;
    PACK_DATA(&dest, bulk_endpoint_descriptor);

    /* bulk output endpoint */
    bulk_endpoint_descriptor.bEndpointAddress = ep_bulk_out;
    PACK_DATA(&dest, bulk_endpoint_descriptor);

    return (dest - orig_dest);
}

void usb_mtp_init_connection(void)
{
    return;
}

void usb_mtp_init(void)
{
    send_buffer_size = sizeof(send_buffer);
    recv_buffer_size = sizeof(recv_buffer);
}

void usb_mtp_disconnect(void)
{
    return;
}

void usb_mtp_transfer_complete(int ep, int dir, int status, int length)
{
    /* only want data from the bulk endpoints */
    if (ep == ep_int || ep == EP_CONTROL)
        return;
}

bool usb_mtp_control_request(struct usb_ctrlrequest* req, unsigned char* dest)
{
    return true;
}
