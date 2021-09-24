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
 * Copyright (C) 2021 by Tomasz Moń
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
#include "usb_serial.h"
#include "usb_class_driver.h"
/*#define LOGF_ENABLE*/
#include "logf.h"

#define CDC_SUBCLASS_ACM         0x02
#define CDC_PROTOCOL_NONE        0x00

/* Class-Specific Request Codes */
#define SET_LINE_CODING          0x20
#define GET_LINE_CODING          0x21
#define SET_CONTROL_LINE_STATE   0x22

#define SUBTYPE_HEADER           0x00
#define SUBTYPE_CALL_MANAGEMENT  0x01
#define SUBTYPE_ACM              0x02
#define SUBTYPE_UNION            0x06

/* Support SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE requests
 * and SERIAL_STATE notification.
 */
#define ACM_CAP_LINE_CODING      0x02

struct cdc_header_descriptor {
    uint8_t  bFunctionLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubtype;
    uint16_t bcdCDC;
} __attribute__((packed));

struct cdc_call_management_descriptor {
    uint8_t bFunctionLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bmCapabilities;
    uint8_t bDataInterface;
} __attribute__((packed));

struct cdc_acm_descriptor {
    uint8_t bFunctionLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bmCapabilities;
} __attribute__((packed));

struct cdc_union_descriptor {
    uint8_t  bFunctionLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubtype;
    uint8_t  bControlInterface;
    uint8_t  bSubordinateInterface0;
} __attribute__((packed));

struct cdc_line_coding {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
} __attribute__((packed));

static struct usb_interface_assoc_descriptor
    association_descriptor =
{
    .bLength            = sizeof(struct usb_interface_assoc_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE_ASSOCIATION,
    .bFirstInterface    = 0,
    .bInterfaceCount    = 2,
    .bFunctionClass     = USB_CLASS_COMM,
    .bFunctionSubClass  = CDC_SUBCLASS_ACM,
    .bFunctionProtocol  = CDC_PROTOCOL_NONE,
    .iFunction          = 0
};

static struct usb_interface_descriptor
    control_interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_COMM,
    .bInterfaceSubClass = CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = CDC_PROTOCOL_NONE,
    .iInterface         = 0
};

static struct cdc_header_descriptor
    header_descriptor =
{
    .bFunctionLength    = sizeof(struct cdc_header_descriptor),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = SUBTYPE_HEADER,
    .bcdCDC             = 0x0110
};

static struct cdc_call_management_descriptor
    call_management_descriptor =
{
    .bFunctionLength    = sizeof(struct cdc_call_management_descriptor),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = SUBTYPE_CALL_MANAGEMENT,
    .bmCapabilities     = 0,
    .bDataInterface     = 0
};

static struct cdc_acm_descriptor
    acm_descriptor =
{
    .bFunctionLength    = sizeof(struct cdc_acm_descriptor),
    .bDescriptorType    = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = SUBTYPE_ACM,
    .bmCapabilities     = ACM_CAP_LINE_CODING
};

static struct cdc_union_descriptor
    union_descriptor =
{
    .bFunctionLength        = sizeof(struct cdc_union_descriptor),
    .bDescriptorType        = USB_DT_CS_INTERFACE,
    .bDescriptorSubtype     = SUBTYPE_UNION,
    .bControlInterface      = 0,
    .bSubordinateInterface0 = 0
};

static struct usb_interface_descriptor
    data_interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = USB_CLASS_CDC_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0
};

static struct usb_endpoint_descriptor
    endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

union line_coding_buffer
{
    struct cdc_line_coding data;
    unsigned char raw[64];
};

static union line_coding_buffer line_coding USB_DEVBSS_ATTR;

/* send_buffer: local ring buffer.
 * transit_buffer: used to store aligned data that will be sent by the USB
 * driver. PP502x needs boost for high speed USB, but still works up to
 * around 100 bytes without boost, we play safe and limit packet size to 32
 * bytes, it doesn't hurt because data can be sent over several transfers.
 */
#define BUFFER_SIZE 512
#define TRANSIT_BUFFER_SIZE 32
#define RECV_BUFFER_SIZE 32
static unsigned char send_buffer[BUFFER_SIZE];
static unsigned char transit_buffer[TRANSIT_BUFFER_SIZE]
    USB_DEVBSS_ATTR __attribute__((aligned(4)));
static unsigned char receive_buffer[512]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));

static void sendout(void);

static int buffer_start;
/* The number of bytes to transfer that haven't been given to the USB stack yet */
static int buffer_length;
/* The number of bytes to transfer that have been given to the USB stack */
static int buffer_transitlength;
static bool active = false;

static int ep_in, ep_out, ep_int;
static int control_interface, data_interface;

int usb_serial_request_endpoints(struct usb_class_driver *drv)
{
    ep_in = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_IN, drv);
    if (ep_in < 0)
        return -1;

    ep_out = usb_core_request_endpoint(USB_ENDPOINT_XFER_BULK, USB_DIR_OUT,
            drv);
    if (ep_out < 0) {
        usb_core_release_endpoint(ep_in);
        return -1;
    }

    /* Optional interrupt endpoint. While the code does not actively use it,
     * it is needed to get out-of-the-box serial port experience on Windows
     * and Linux. If this endpoint is not available, only CDC Data interface
     * will be exported (can still work on Linux with manual modprobe).
     */
    ep_int = usb_core_request_endpoint(USB_ENDPOINT_XFER_INT, USB_DIR_IN, drv);

    return 0;
}

int usb_serial_set_first_interface(int interface)
{
    control_interface = interface;
    data_interface = interface + 1;
    return interface + 2;
}

int usb_serial_get_config_descriptor(unsigned char *dest, int max_packet_size)
{
    unsigned char *orig_dest = dest;

    association_descriptor.bFirstInterface         = control_interface;
    control_interface_descriptor.bInterfaceNumber  = control_interface;
    call_management_descriptor.bDataInterface      = data_interface;
    union_descriptor.bControlInterface             = control_interface;
    union_descriptor.bSubordinateInterface0        = data_interface;
    data_interface_descriptor.bInterfaceNumber     = data_interface;

    if (ep_int > 0)
    {
        PACK_DATA(&dest, association_descriptor);
        PACK_DATA(&dest, control_interface_descriptor);
        PACK_DATA(&dest, header_descriptor);
        PACK_DATA(&dest, call_management_descriptor);
        PACK_DATA(&dest, acm_descriptor);
        PACK_DATA(&dest, union_descriptor);

        /* Notification endpoint. Set wMaxPacketSize to 64 as it is valid
         * both on Full and High speed. Note that max_packet_size is for bulk.
         * Maximum bInterval for High Speed is 16 and for Full Speed is 255.
         */
        endpoint_descriptor.bEndpointAddress = ep_int;
        endpoint_descriptor.bmAttributes     = USB_ENDPOINT_XFER_INT;
        endpoint_descriptor.wMaxPacketSize   = 64;
        endpoint_descriptor.bInterval        = 16;
        PACK_DATA(&dest, endpoint_descriptor);
    }

    PACK_DATA(&dest, data_interface_descriptor);
    endpoint_descriptor.bEndpointAddress = ep_in;
    endpoint_descriptor.bmAttributes     = USB_ENDPOINT_XFER_BULK;
    endpoint_descriptor.wMaxPacketSize   = max_packet_size;
    endpoint_descriptor.bInterval        = 0;
    PACK_DATA(&dest, endpoint_descriptor);

    endpoint_descriptor.bEndpointAddress = ep_out;
    PACK_DATA(&dest, endpoint_descriptor);

    return (dest - orig_dest);
}

/* called by usb_core_control_request() */
bool usb_serial_control_request(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest)
{
    bool handled = false;

    (void)dest;
    (void)reqdata;

    if (req->wIndex != control_interface)
    {
        return false;
    }

    if (req->bRequestType == (USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE))
    {
        if (req->bRequest == SET_LINE_CODING)
        {
            if (req->wLength == sizeof(struct cdc_line_coding))
            {
                /* Receive line coding into local copy */
                if (!reqdata)
                {
                    usb_drv_control_response(USB_CONTROL_RECEIVE, line_coding.raw,
                                             sizeof(struct cdc_line_coding));
                }
                else
                {
                    usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
                }

                handled = true;
            }
        }
        else if (req->bRequest == SET_CONTROL_LINE_STATE)
        {
            if (req->wLength == 0)
            {
                /* wValue holds Control Signal Bitmap that is simply ignored here */
                usb_drv_control_response(USB_CONTROL_ACK, NULL, 0);
                handled = true;
            }
        }
    }
    else if (req->bRequestType == (USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE))
    {
        if (req->bRequest == GET_LINE_CODING)
        {
            if (req->wLength == sizeof(struct cdc_line_coding))
            {
                /* Send back line coding so host is happy */
                usb_drv_control_response(USB_CONTROL_ACK, line_coding.raw,
                                         sizeof(struct cdc_line_coding));
                handled = true;
            }
        }
    }

    return handled;
}

void usb_serial_init_connection(void)
{
    /* prime rx endpoint */
    usb_drv_recv_nonblocking(ep_out, receive_buffer, RECV_BUFFER_SIZE);

    /* we come here too after a bus reset, so reset some data */
    buffer_transitlength = 0;
    if(buffer_length>0)
    {
        sendout();
    }
    active=true;
}

/* called by usb_code_init() */
void usb_serial_init(void)
{
    logf("serial: init");
    buffer_start = 0;
    buffer_length = 0;
    buffer_transitlength = 0;
}

void usb_serial_disconnect(void)
{
    active = false;
}

static void sendout(void)
{
    buffer_transitlength = MIN(buffer_length,BUFFER_SIZE-buffer_start);
    if(buffer_transitlength > 0)
    {
        buffer_transitlength = MIN(buffer_transitlength,TRANSIT_BUFFER_SIZE);
        buffer_length -= buffer_transitlength;
        memcpy(transit_buffer,&send_buffer[buffer_start],buffer_transitlength);
        usb_drv_send_nonblocking(ep_in,transit_buffer,buffer_transitlength);
    }
}

void usb_serial_send(const unsigned char *data,int length)
{
    int freestart, available_end_space, i;

    if (!active||length<=0)
        return;

    i=buffer_start+buffer_length+buffer_transitlength;
    freestart=i%BUFFER_SIZE;
    available_end_space=BUFFER_SIZE-i;

    if (0>=available_end_space)
    {
        /* current buffer wraps, so new data can't wrap */
        int available_space = BUFFER_SIZE -
            (buffer_length + buffer_transitlength);

        length = MIN(length,available_space);
        memcpy(&send_buffer[freestart],data,length);
        buffer_length+=length;
    }
    else
    {
        /* current buffer doesn't wrap, so new data might */
        int first_chunk = MIN(length,available_end_space);

        memcpy(&send_buffer[freestart],data,first_chunk);
        length-=first_chunk;
        buffer_length+=first_chunk;
        if(length>0)
        {
            /* wrap */
            memcpy(&send_buffer[0],&data[first_chunk],MIN(length,buffer_start));
            buffer_length+=MIN(length,buffer_start);
        }
    }

    if (buffer_transitlength==0)
        sendout();
    /* else do nothing. The transfer completion handler will pick it up */
}

/* called by usb_core_transfer_complete() */
void usb_serial_transfer_complete(int ep,int dir, int status, int length)
{
    (void)ep;
    (void)length;

    switch (dir) {
        case USB_DIR_OUT:
            logf("serial: %s", receive_buffer);
            /* Data received. TODO : Do something with it ? */

            /* Get the next bit */
            usb_drv_recv_nonblocking(ep_out, receive_buffer, RECV_BUFFER_SIZE);
            break;

        case USB_DIR_IN:
            /* Data sent out. Update circular buffer */
            if(status == 0)
            {
                /* TODO: Handle (length != buffer_transitlength) */

                buffer_start=(buffer_start+buffer_transitlength)%BUFFER_SIZE;
                buffer_transitlength = 0;
            }

            if(buffer_length>0)
                sendout();
            break;
    }
}
