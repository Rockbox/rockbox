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
//#define LOGF_ENABLE
#include "logf.h"

#ifdef USB_HID

#define CONCAT(low, high)               ((high << 8) | low)
#define PACK_VAL1(dest, val)            *(dest)++ = (val) & 0xff
#define PACK_VAL2(dest, val)            PACK_VAL1((dest), (val)); \
                                        PACK_VAL1((dest), (val >> 8))

/* Documents avaiable here: http://www.usb.org/developers/devclass_docs/ */

#define HID_VER                         0x0110 /* 1.1 */
/* Subclass Codes (HID1_11.pdf, page 18) */
#define SUBCLASS_BOOT_INTERFACE         1
/* Protocol Codes (HID1_11.pdf, page 19) */
#define PROTOCOL_CODE_MOUSE             2
/* HID main items (HID1_11.pdf, page 38) */
#define INPUT                           0x80
#define COLLECTION                      0xa0
#define COLLECTION_APPLICATION          0x01
#define END_COLLECTION                  0xc0
/* Parts (HID1_11.pdf, page 40) */
#define PREFERERD                       (1 << 5)
#define NULL_STATE                      (1 << 6)
/* HID global items (HID1_11.pdf, page 45) */
#define USAGE_PAGE                      0x04
#define LOGICAL_MINIMUM                 0x14
#define LOGICAL_MAXIMUM                 0x24
#define REPORT_SIZE                     0x74
#define REPORT_ID                       0x84
#define REPORT_COUNT                    0x94
/* HID local items (HID1_11.pdf, page 50) */
#define USAGE_MINIMUM                   0x18
#define USAGE_MAXIMUM                   0x28
/* Types of class descriptors (HID1_11.pdf, page 59) */
#define USB_DT_HID                      0x21
#define USB_DT_REPORT                   0x22

/* (Hut1_12.pdf, Table 1, page 14) */
#define USAGE_PAGE_CONSUMER             0x0c

#define CONSUMER_USAGE                  0x09

/* HID-only class specific requests */
#define USB_HID_GET_REPORT              0x01
#define USB_HID_GET_IDLE                0x02
#define USB_HID_GET_PROTOCOL            0x03
#define USB_HID_SET_REPORT              0x09
#define USB_HID_SET_IDLE                0x0a
#define USB_HID_SET_PROTOCOL            0x0b

#define HID_BUF_SIZE_MSG                16
#define HID_BUF_SIZE_CMD                5
#define HID_BUG_SIZE_REPORT             32
#define HID_NUM_BUFFERS                 5

#define HID_BUF_INC(i) do { (i) = ((i) + 1) % HID_NUM_BUFFERS; } while (0)

/* hid interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
                                interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_HID,
    .bInterfaceSubClass = SUBCLASS_BOOT_INTERFACE,
    .bInterfaceProtocol = PROTOCOL_CODE_MOUSE,
    .iInterface         = 0
};

struct usb_hid_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wBcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType0;
    uint16_t wDescriptorLength0;
} __attribute__ ((packed));

static struct usb_hid_descriptor __attribute__((aligned(2))) hid_descriptor =
{
    .bLength            = sizeof(struct usb_hid_descriptor),
    .bDescriptorType    = USB_DT_HID,
    .wBcdHID            = HID_VER,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .bDescriptorType0   = USB_DT_REPORT,
    .wDescriptorLength0 = 0
};

static struct usb_endpoint_descriptor __attribute__((aligned(2)))
    endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

static unsigned char report_descriptor[HID_BUG_SIZE_REPORT]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));

static unsigned char send_buffer[HID_NUM_BUFFERS][HID_BUF_SIZE_MSG]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));
size_t send_buffer_len[HID_NUM_BUFFERS];
static int cur_buf_prepare;
static int cur_buf_send;

static uint16_t report_descriptor_len;
static bool active = false;
static int ep_in;
static int usb_interface;
static uint32_t report_id;

static void usb_hid_try_send_drv(void);

static void pack_parameter(unsigned char **dest, uint8_t parameter,
        uint32_t value)
{
    uint8_t size_value = 1; /* # of bytes */
    uint32_t v = value;

    while (v >>= 8)
        size_value++;

    **dest = parameter | size_value;
    (*dest)++;

    while (size_value--)
    {
        **dest = value & 0xff;
        (*dest)++;
        value >>= 8;
    }
}

int usb_hid_request_endpoints(struct usb_class_driver *drv)
{
    ep_in = usb_core_request_endpoint(USB_ENDPOINT_XFER_INT, USB_DIR_IN, drv);
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
    unsigned char *report, *orig_dest = dest;

    logf("hid: config desc.");

    /* Ignore given max_packet_size */
    (void)max_packet_size;

    /* TODO: Increment report_id for each report, and send command with
     * appropriate report id */
    report_id = 2;

    /* Initialize report descriptor */
    report = report_descriptor;
    pack_parameter(&report, USAGE_PAGE, USAGE_PAGE_CONSUMER);
    PACK_VAL2(report, CONCAT(CONSUMER_USAGE, CONSUMER_CONTROL));
    pack_parameter(&report, COLLECTION, COLLECTION_APPLICATION);
    pack_parameter(&report, REPORT_ID, report_id);
    pack_parameter(&report, REPORT_SIZE, 16);
    pack_parameter(&report, REPORT_COUNT, 2);
    pack_parameter(&report, LOGICAL_MINIMUM, 1);
    pack_parameter(&report, LOGICAL_MAXIMUM, 652);
    pack_parameter(&report, USAGE_MINIMUM, CONSUMER_CONTROL);
    pack_parameter(&report, USAGE_MAXIMUM, AC_SEND);
    pack_parameter(&report, INPUT, PREFERERD | NULL_STATE);
    PACK_VAL1(report, END_COLLECTION);
    report_descriptor_len = (uint16_t)((uint32_t)report -
            (uint32_t)report_descriptor);

    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DATA(dest, interface_descriptor);
    hid_descriptor.wDescriptorLength0 = report_descriptor_len;
    PACK_DATA(dest, hid_descriptor);

    endpoint_descriptor.wMaxPacketSize = 8;
    endpoint_descriptor.bInterval = 8;

    endpoint_descriptor.bEndpointAddress = ep_in;
    PACK_DATA(dest, endpoint_descriptor);

    return (dest - orig_dest);
}

void usb_hid_init_connection(void)
{
    logf("hid: init connection");

    active = true;
}

/* called by usb_core_init() */
void usb_hid_init(void)
{
    int i;

    logf("hid: init");

    for (i = 0; i < HID_NUM_BUFFERS; i++)
        send_buffer_len[i] = 0;

    cur_buf_prepare = 0;
    cur_buf_send = 0;

    active = true;
}

void usb_hid_disconnect(void)
{
    logf("hid: disconnect");
    active = false;
}

/* called by usb_core_transfer_complete() */
void usb_hid_transfer_complete(int ep, int dir, int status, int length)
{
    (void)ep;
    (void)dir;
    (void)status;
    (void)length;

    switch (dir) {
        case USB_DIR_OUT:
            break;
        case USB_DIR_IN: {
            if (status)
                break;

            send_buffer_len[cur_buf_send] = 0;
            HID_BUF_INC(cur_buf_send);
            usb_hid_try_send_drv();
            break;
        }
    }
}

/* called by usb_core_control_request() */
bool usb_hid_control_request(struct usb_ctrlrequest* req, unsigned char* dest)
{
    bool handled = false;

    switch(req->bRequestType & USB_TYPE_MASK) {
        case USB_TYPE_STANDARD: {
            unsigned char *orig_dest = dest;

            switch(req->wValue>>8) { /* type */
                case USB_DT_HID: {
                    logf("hid: type hid");
                    hid_descriptor.wDescriptorLength0 = report_descriptor_len;
                    PACK_DATA(dest, hid_descriptor);
                    break;
                }
                case USB_DT_REPORT: {
                    logf("hid: type report");
                    memcpy(dest, &report_descriptor, report_descriptor_len);
                    dest += report_descriptor_len;
                    break;
                }
                default:
                    logf("hid: unsup. std. req");
                    break;
            }
            if (dest != orig_dest &&
                    !usb_drv_send(EP_CONTROL, orig_dest, dest - orig_dest)) {
                usb_core_ack_control(req);
                handled = true;
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

static void usb_hid_try_send_drv(void)
{
    int rc;
    int length = send_buffer_len[cur_buf_send];

    if (!length)
        return;

    rc = usb_drv_send_nonblocking(ep_in, send_buffer[cur_buf_send], length);
    if (rc)
    {
        send_buffer_len[cur_buf_send] = 0;
        return;
    }
}

static void usb_hid_queue(unsigned char *data, int length)
{
    if (!active || length <= 0)
        return;

    /* Buffer overflow - item still in use */
    if (send_buffer_len[cur_buf_prepare])
        return;

    /* Prepare buffer for sending */
    if (length > HID_BUF_SIZE_MSG)
        length = HID_BUF_SIZE_MSG;
    memcpy(send_buffer[cur_buf_prepare], data, length);
    send_buffer_len[cur_buf_prepare] = length;

    HID_BUF_INC(cur_buf_prepare);
}

void usb_hid_send_consumer_usage(consumer_usage_page_t id)
{
    static unsigned char buf[HID_BUF_SIZE_CMD] USB_DEVBSS_ATTR
        __attribute__((aligned(32)));
    unsigned char *dest = buf;

    memset(buf, 0, sizeof(buf));

    logf("HID: Sending 0x%x", id);

    pack_parameter(&dest, 0, id);
    buf[0] = report_id;

   /* Key pressed */
    usb_hid_queue(buf, HID_BUF_SIZE_CMD);

    /* Key released */
    memset(buf, 0, sizeof(buf));
    buf[0] = report_id;
    usb_hid_queue(buf, HID_BUF_SIZE_CMD);

    usb_hid_try_send_drv();
}

#endif /*USB_HID*/
