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

#define BUFFER_SIZE 512 /* Max 16k because of controller limitations */
static unsigned char _send_buffer[BUFFER_SIZE] __attribute__((aligned(32)));
static unsigned char* send_buffer;

static unsigned char _receive_buffer[512] __attribute__((aligned(32)));
static unsigned char* receive_buffer;
static bool busy_sending = false;
static int buffer_start;
static int buffer_length;
static bool active = false;

static struct mutex sendlock;

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

void usb_serial_exit(void)
{
    active = false;
}

static void sendout(void)
{
    if(buffer_start+buffer_length > BUFFER_SIZE)
    {
        /* Buffer wraps. Only send the first part */
        usb_drv_send_nonblocking(EP_SERIAL, &send_buffer[buffer_start],(BUFFER_SIZE - buffer_start));
    }
    else
    {
        /* Send everything */
        usb_drv_send_nonblocking(EP_SERIAL, &send_buffer[buffer_start],buffer_length);
    }
    busy_sending=true;
}

void usb_serial_send(unsigned char *data,int length)
{
    if(!active)
        return;
    mutex_lock(&sendlock);
    if(buffer_start+buffer_length > BUFFER_SIZE)
    { 
        /* current buffer wraps, so new data can't */
        int available_space = BUFFER_SIZE - buffer_length;
        length=MIN(length,available_space);
        memcpy(&send_buffer[(buffer_start+buffer_length)%BUFFER_SIZE],data,length);
        buffer_length+=length;
    }
    else
    {
        /* current buffer doesn't wrap, so new data might */
        int available_end_space = (BUFFER_SIZE - (buffer_start + buffer_length));
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
            usb_drv_recv(EP_SERIAL, receive_buffer, sizeof _receive_buffer);
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

            if(buffer_length!=0)
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
        case USB_REQ_SET_CONFIGURATION:
            logf("serial: set config");
            /* prime rx endpoint */
            usb_drv_recv(EP_SERIAL, receive_buffer, sizeof _receive_buffer);
            handled = true;

            /* we come here too after a bus reset, so reset some data */
            busy_sending = false;
            sendout();
            break;

        default:
            logf("serial: unhandeld req %d", req->bRequest);
    }

    return handled;
}

#endif /*USB_SERIAL*/
