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

/* serial interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
    interface_descriptor =
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

#define BUFFER_SIZE 512
static unsigned char send_buffer[BUFFER_SIZE]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));
static unsigned char receive_buffer[32]
    USB_DEVBSS_ATTR __attribute__((aligned(32)));

static void sendout(void);

static int buffer_start;
/* The number of bytes to transfer that haven't been given to the USB stack yet */
static int buffer_length;
/* The number of bytes to transfer that have been given to the USB stack */
static int buffer_transitlength;
static bool active = false;

static int ep_in, ep_out;
static int usb_interface;

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

    return 0;
}

int usb_serial_set_first_interface(int interface)
{
    usb_interface = interface;
    return interface + 1;
}

int usb_serial_get_config_descriptor(unsigned char *dest, int max_packet_size)
{
    unsigned char *orig_dest = dest;

    interface_descriptor.bInterfaceNumber = usb_interface;
    PACK_DATA(dest, interface_descriptor);

    endpoint_descriptor.wMaxPacketSize = max_packet_size;

    endpoint_descriptor.bEndpointAddress = ep_in;
    PACK_DATA(dest, endpoint_descriptor);

    endpoint_descriptor.bEndpointAddress = ep_out;
    PACK_DATA(dest, endpoint_descriptor);

    return (dest - orig_dest);
}

/* called by usb_core_control_request() */
bool usb_serial_control_request(struct usb_ctrlrequest* req, unsigned char* dest)
{
    bool handled = false;

    (void)dest;
    switch (req->bRequest) {
        default:
            logf("serial: unhandeld req %d", req->bRequest);
    }
    return handled;
}

void usb_serial_init_connection(void)
{
    /* prime rx endpoint */
    usb_drv_recv(ep_out, receive_buffer, sizeof receive_buffer);

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
    active = true;
}

void usb_serial_disconnect(void)
{
    active = false;
}

static void sendout(void)
{
    buffer_transitlength = MIN(buffer_length,BUFFER_SIZE-buffer_start);
    /* For unknown reasons packets larger than 96 bytes are not sent. We play
     * safe and limit to 32. TODO: find the real bug */
    buffer_transitlength = MIN(buffer_transitlength,32);
    if(buffer_transitlength > 0)
    {
        buffer_length -= buffer_transitlength;
        usb_drv_send_nonblocking(ep_in, &send_buffer[buffer_start],
                buffer_transitlength);
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
            usb_drv_recv(ep_out, receive_buffer, sizeof receive_buffer);
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
