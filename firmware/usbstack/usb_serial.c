/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
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
#include "string.h"
#include "system.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "kernel.h"

//#define LOGF_ENABLE
#include "logf.h"

#ifdef USB_SERIAL

/* serial interface */
struct usb_interface_descriptor __attribute__((aligned(2)))
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

struct usb_endpoint_descriptor __attribute__((aligned(2))) endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

#define BUFFER_SIZE 512 /* Max 16k because of controller limitations */
static unsigned char _send_buffer[BUFFER_SIZE] __attribute__((aligned(32)));
static unsigned char* send_buffer;

static unsigned char _receive_buffer[512] __attribute__((aligned(32)));
static unsigned char* receive_buffer;
static bool busy_sending = false;
static int buffer_start;
static int buffer_length;
static bool active = false;

static int usb_endpoint;
static int usb_interface;

static struct mutex sendlock;

static void sendout(void)
{
    if(buffer_start+buffer_length > BUFFER_SIZE)
    {
        /* Buffer wraps. Only send the first part */
        usb_drv_send_nonblocking(usb_endpoint, &send_buffer[buffer_start],
                                 (BUFFER_SIZE - buffer_start));
    }
    else
    {
        /* Send everything */
        usb_drv_send_nonblocking(usb_endpoint, &send_buffer[buffer_start],
                                 buffer_length);
    }
    busy_sending=true;
}

int usb_serial_get_config_descriptor(unsigned char *dest,int max_packet_size,
                                     int interface_number,int endpoint)
{
    endpoint_descriptor.wMaxPacketSize=max_packet_size;
    interface_descriptor.bInterfaceNumber=interface_number;


    memcpy(dest,&interface_descriptor,sizeof(struct usb_interface_descriptor));
    dest+=sizeof(struct usb_interface_descriptor);

    endpoint_descriptor.bEndpointAddress = endpoint | USB_DIR_IN,
    memcpy(dest,&endpoint_descriptor,sizeof(struct usb_endpoint_descriptor));
    dest+=sizeof(struct usb_endpoint_descriptor);

    endpoint_descriptor.bEndpointAddress = endpoint | USB_DIR_OUT,
    memcpy(dest,&endpoint_descriptor,sizeof(struct usb_endpoint_descriptor));
    return sizeof(struct usb_interface_descriptor) +
           2 * sizeof(struct usb_endpoint_descriptor);
}

void usb_serial_init_connection(int interface,int endpoint)
{
    usb_interface = interface;
    usb_endpoint = endpoint;

    /* prime rx endpoint */
    usb_drv_recv(usb_endpoint, receive_buffer, sizeof _receive_buffer);

    /* we come here too after a bus reset, so reset some data */
    mutex_lock(&sendlock);
    busy_sending = false;
    if(buffer_length>0)
    {
        sendout();
    }
    mutex_unlock(&sendlock);
}

/* called by usb_code_init() */
void usb_serial_init(void)
{
    logf("serial: init");
    send_buffer = (void*)UNCACHED_ADDR(&_send_buffer);
    receive_buffer = (void*)UNCACHED_ADDR(&_receive_buffer);
    busy_sending = false;
    buffer_start = 0;
    buffer_length = 0;
    active = true;
    mutex_init(&sendlock);
}

void usb_serial_disconnect(void)
{
    active = false;
}

void usb_serial_send(unsigned char *data,int length)
{
    if(!active)
        return;
    if(length<=0)
        return;
    mutex_lock(&sendlock);
    if(buffer_start+buffer_length > BUFFER_SIZE)
    { 
        /* current buffer wraps, so new data can't */
        int available_space = BUFFER_SIZE - buffer_length;
        length=MIN(length,available_space);
        memcpy(&send_buffer[(buffer_start+buffer_length)%BUFFER_SIZE],
               data,length);
        buffer_length+=length;
    }
    else
    {
        /* current buffer doesn't wrap, so new data might */
        int available_end_space = (BUFFER_SIZE - (buffer_start+buffer_length));
        int first_chunk = MIN(length,available_end_space);
        memcpy(&send_buffer[buffer_start + buffer_length],data,first_chunk);
        length-=first_chunk;
        buffer_length+=first_chunk;
        if(length>0)
        {
            /* wrap */
            memcpy(&send_buffer[0],&data[first_chunk],MIN(length,buffer_start));
            buffer_length+=MIN(length,buffer_start);
        }
    }
    if(busy_sending)
    {
        /* Do nothing. The transfer completion handler will pick it up */
    }
    else
    {
        sendout();
    }
    mutex_unlock(&sendlock);
}

/* called by usb_core_transfer_complete() */
void usb_serial_transfer_complete(bool in, int status, int length)
{
    switch (in) {
        case false:
            logf("serial: %s", receive_buffer);
            /* Data received. TODO : Do something with it ? */
            usb_drv_recv(usb_endpoint, receive_buffer, sizeof _receive_buffer);
            break;

        case true:
            mutex_lock(&sendlock);
            /* Data sent out. Update circular buffer */
            if(status == 0)
            {
                buffer_start = (buffer_start + length)%BUFFER_SIZE;
                buffer_length -= length;
            }
            busy_sending = false;

            if(buffer_length>0)
            {
                sendout();
            }
            mutex_unlock(&sendlock);
            break;
    }
}

/* called by usb_core_control_request() */
bool usb_serial_control_request(struct usb_ctrlrequest* req)
{
    bool handled = false;
    switch (req->bRequest) {
        default:
            logf("serial: unhandeld req %d", req->bRequest);
    }
    return handled;
}

#endif /*USB_SERIAL*/
