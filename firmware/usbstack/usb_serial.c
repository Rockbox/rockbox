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
#include "system.h"
#include "usb_core.h"
#include "usb_drv.h"

//#define LOGF_ENABLE
#include "logf.h"

static unsigned char _transfer_buffer[16];
static unsigned char* transfer_buffer;

/* called by usb_code_init() */
void usb_serial_init(void)
{
    logf("serial: init");
    transfer_buffer = (void*)UNCACHED_ADDR(&_transfer_buffer);
}

/* called by usb_core_transfer_complete() */
void usb_serial_transfer_complete(int endpoint)
{
    switch (endpoint) {
        case EP_RX:
            logf("serial: %s", transfer_buffer);

            /* re-prime endpoint */
            usb_drv_recv(EP_RX, transfer_buffer, sizeof _transfer_buffer);

            /* echo back :) */
            usb_drv_send(EP_TX, transfer_buffer, sizeof transfer_buffer);
            break;

        case EP_TX:
            break;
    }
}

/* called by usb_core_control_request() */
bool usb_serial_control_request(struct usb_ctrlrequest* req)
{
    /* note: interrupt context */

    bool handled = false;
    switch (req->bRequest) {
        case USB_REQ_SET_CONFIGURATION:
            logf("serial: set config");
            /* prime rx endpoint */
            usb_drv_recv(EP_RX, transfer_buffer, sizeof _transfer_buffer);
            handled = true;
            break;

        default:
            logf("serial: unhandeld req %d", req->bRequest);
    }

    return handled;
}
