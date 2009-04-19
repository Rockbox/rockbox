/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Tomer Shalev
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
#include "kernel.h"
#include "usb_hid.h"
#include "usb_class_driver.h"
#define LOGF_ENABLE
#include "logf.h"

#ifdef USB_HID

#define CONCAT(low, high)               ((high << 8) | low)
#define SIZE_VALUE                      0x01
/* HID main items (HID1_11.pdf, page 38) */
#define INPUT                           (0x80 | SIZE_VALUE)
#define OUTPUT                          (0x90 | SIZE_VALUE)
#define FEATURE                         (0xb0 | SIZE_VALUE)
#define COLLECTION                      (0xa0 | SIZE_VALUE)
#define COLLECTION_APPLICATION            CONCAT(COLLECTION, 0x01)
#define END_COLLECTION                  0xc0
/* HID global items (HID1_11.pdf, page 45) */
#define USAGE_PAGE                      (0x04 | SIZE_VALUE)
#define LOGICAL_MINIMUM                 (0x14 | SIZE_VALUE)
#define LOGICAL_MAXIMUM                 (0x24 | SIZE_VALUE)
#define PHYSICAL_MINIMUM                (0x34 | SIZE_VALUE)
#define PHYSICAL_MAXIMUM                (0x44 | SIZE_VALUE)
#define UNIT_EXPONENT                   (0x54 | SIZE_VALUE)
#define UNIT                            (0x64 | SIZE_VALUE)
#define REPORT_SIZE                     (0x74 | SIZE_VALUE)
#define REPORT_ID                       (0x84 | SIZE_VALUE)
#define REPORT_COUNT                    (0x94 | SIZE_VALUE)
#define PUSH                            (0xa4 | SIZE_VALUE)
#define POP                             (0xb4 | SIZE_VALUE)
/* Hut1_12.pdf, Table 1, page 14 */
#define USAGE_PAGE_CONSUMER               CONCAT(USAGE_PAGE, 0x0c)
/* Hut1_12.pdf, Table 17, page 77 */
#define CONSUMER_USAGE                  0x09
#define CONSUMER_USAGE_CONTROL            CONCAT(CONSUMER_USAGE, 0x01)
#define CONSUMER_USAGE_MUTE               CONCAT(CONSUMER_USAGE, 0xe2)
#define CONSUMER_USAGE_VOLUME_INCREMENT   CONCAT(CONSUMER_USAGE, 0xe9)
#define CONSUMER_USAGE_VOLUME_DECREMENT   CONCAT(CONSUMER_USAGE, 0xea)
/* Hut1_12.pdf, Table 4, page 20 */
#define CONSUMER_CONTROL                  CONCAT(COLLECTION_APPLICATION, 0x01)

#define USB_DT_HID                 0x21
#define USB_DT_REPORT              0x22
#define USB_DT_PHYSICAL_DESCRIPTOR 0x23

/* serial interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
                                interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0
};

/* USB_DT_HID: Endpoint descriptor */
struct usb_hid_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wBcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType0;
    uint16_t wDescriptorLength0;
} __attribute__ ((packed));

/* USB_DT_REPORT: Endpoint descriptor */
static struct usb_hid_descriptor __attribute__((aligned(2))) hid_descriptor =
{
    .bLength            = sizeof(struct usb_hid_descriptor),
    .bDescriptorType    = USB_DT_HID,
    .wBcdHID            = 0x0100,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .bDescriptorType0   = 0x22,
    .wDescriptorLength0 = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2))) endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

/* USB_DT_REPORT: Endpoint descriptor */
struct usb_report_descriptor {
    uint16_t wUsagePage;
    uint16_t wUsage;
    uint16_t wCollection;
    uint16_t wCollectionItems[12];
    uint8_t wEndCollection;
} __attribute__ ((packed));

static struct usb_report_descriptor __attribute__((aligned(2))) report_descriptor =
{
    .wUsagePage        = USAGE_PAGE_CONSUMER,
    .wUsage            = CONSUMER_USAGE_CONTROL,
    .wCollection       = COLLECTION_APPLICATION,
    .wCollectionItems  = {
        CONCAT(LOGICAL_MINIMUM, 0x0),
        CONCAT(LOGICAL_MAXIMUM, 0x1),
        USAGE_PAGE_CONSUMER,
        CONSUMER_USAGE_MUTE,
        CONSUMER_USAGE_VOLUME_INCREMENT,
        CONSUMER_USAGE_VOLUME_DECREMENT,
        CONCAT(REPORT_COUNT, 0x3),
        CONCAT(REPORT_SIZE, 0x1),
        CONCAT(INPUT, 0x42),
        CONCAT(REPORT_COUNT, 0x5),
        CONCAT(REPORT_SIZE, 0x1),
        CONCAT(INPUT, 0x01)
    },
    .wEndCollection    = END_COLLECTION
};

static int ep_in;
static int usb_interface;

int usb_hid_request_endpoints(struct usb_class_driver *drv)
{
    ep_in = usb_core_request_endpoint(USB_DIR_IN, drv);
    if (ep_in < 0)
        return -1;

    return 0;
}

int usb_hid_set_first_interface(int interface)
{
    usb_interface = interface;

    return interface + 1;
}


int usb_hid_get_config_descriptor(unsigned char *dest,int max_packet_size)
{
    unsigned char *orig_dest = dest;

    logf("hid: config desc.");
    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DESCRIPTOR(dest, interface_descriptor);

    hid_descriptor.wDescriptorLength0 = sizeof(report_descriptor);
    PACK_DESCRIPTOR(dest, hid_descriptor);

    /* Ignore max_packet_size and set to 1 bytes long packet size */
    (void)max_packet_size;
    endpoint_descriptor.wMaxPacketSize = 1;
    endpoint_descriptor.bInterval = 8;

    endpoint_descriptor.bEndpointAddress = ep_in;
    PACK_DESCRIPTOR(dest, endpoint_descriptor);

    return (dest - orig_dest);
}

void usb_hid_init_connection(void)
{
    logf("hid: init connection");
}

/* called by usb_code_init() */
void usb_hid_init(void)
{
    logf("hid: init");
}

void usb_hid_disconnect(void)
{
    logf("hid: disconnect");
}

/* called by usb_core_transfer_complete() */
void usb_hid_transfer_complete(int ep,int dir, int status, int length)
{
    (void)ep;
    (void)dir;
    (void)status;
    (void)length;

    logf("hid: transfer complete. ep %d, dir %d, status %d ,length %d",
        ep, dir, status, length);
}

/* HID-only class specific requests */
#define USB_HID_GET_REPORT   0x01
#define USB_HID_GET_IDLE     0x02
#define USB_HID_GET_PROTOCOL 0x03
#define USB_HID_SET_REPORT   0x09
#define USB_HID_SET_IDLE     0x0a
#define USB_HID_SET_PROTOCOL 0x0b

/* called by usb_core_control_request() */
bool usb_hid_control_request(struct usb_ctrlrequest* req, unsigned char* dest)
{
    bool handled = false;

    switch(req->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD: {
                switch(req->wValue>>8) { /* type */
                    case USB_DT_REPORT: {
                        logf("hid: report");
                        if (dest == NULL) {
                            logf("dest is NULL!");
                        }
                        if (dest) {
                            unsigned char *orig_dest = dest;
                            PACK_DESCRIPTOR(dest, report_descriptor);
                            if(usb_drv_send(EP_CONTROL, orig_dest, dest - orig_dest))
                                break;
                            usb_core_ack_control(req);

                        }
                        handled = true;
                        break;
                    }
                    default:
                        logf("hid: unsup. std. req");
                        break;
                }
                break;
            }

        case USB_TYPE_CLASS: {
                switch (req->bRequest) {
                    case USB_HID_SET_IDLE:
                        logf("hid: set idle");
                        usb_core_ack_control(req);
                        handled = true;
                        break;
                    default:
                        //logf("hid: unsup. cls. req");
                        logf("%d: unsup. cls. req", req->bRequest);
                        break;
                }
                break;
            }

        case USB_TYPE_VENDOR:
            logf("hid: unsup. ven. req");
            break;
    }
    return handled;
}

void usb_hid_send(unsigned char *data, int length)
{
    (void)data;
    (void)(length);

    logf("hid: send %d bytes: \"%s\"", length, data);
}

#endif /*USB_HID*/
