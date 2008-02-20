/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 by Björn Stenberg
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

#ifdef USB_BENCHMARK

static int current_length;

static unsigned char _input_buffer[16384];
static unsigned char* input_buffer = USB_IRAM_ORIGIN + 1024;

static void ack_control(struct usb_ctrlrequest* req);

static enum {
    IDLE,
    SENDING,
    RECEIVING
} state = IDLE;


void usb_benchmark_init(void)
{
    int i;
    for (i=0; i<128; i++)
        input_buffer[i] = i;
}

void usb_benchmark_control_request(struct usb_ctrlrequest* req)
{
    int todo;
    //usb_max_pkt_size = sizeof _input_buffer;
    usb_max_pkt_size = 64;

    switch (req->bRequest) {
        case 1: /* read */
            ack_control(req);
            current_length = req->wValue * req->wIndex;
            logf("bench: read %d", current_length);
            todo = MIN(usb_max_pkt_size, current_length);
            state = SENDING;
            usb_drv_reset_endpoint(EP_BENCHMARK, true);
            usb_drv_send(EP_BENCHMARK, &input_buffer, todo);
            current_length -= todo;
            break;

        case 2: /* write */
            ack_control(req);
            current_length = req->wValue * req->wIndex;
            logf("bench: write %d", current_length);
            state = RECEIVING;
            usb_drv_reset_endpoint(EP_BENCHMARK, false);
            usb_drv_recv(EP_BENCHMARK, &input_buffer, sizeof _input_buffer);
            break;
    }
}

void usb_benchmark_transfer_complete(bool in)
{
    (void)in;

    /* see what remains to transfer */
    if (current_length == 0) {
        logf("we're done");
        state = IDLE;
        return; /* we're done */
    }

    switch (state)
    {
        case SENDING: {
            int todo = MIN(usb_max_pkt_size, current_length);
            if (in == false) {
                logf("unexpected ep_rx");
                break;
            }
    
            logf("bench: %d more tx", current_length);
            usb_drv_send(EP_BENCHMARK, &input_buffer, todo);
            current_length -= todo;
            input_buffer[0]++;
            break;
        }

        case RECEIVING:
            if (in == true) {
                logf("unexpected ep_tx");
                break;
            }

            /* re-prime endpoint */
            usb_drv_recv(EP_BENCHMARK, &input_buffer, sizeof _input_buffer);
            input_buffer[0]++;
            break;

        default:
            break;
                 
    }
}

static void ack_control(struct usb_ctrlrequest* req)
{
    if (req->bRequestType & 0x80)
        usb_drv_recv(EP_CONTROL, NULL, 0);
    else
        usb_drv_send(EP_CONTROL, NULL, 0);
}
#endif /*USB_BENCHMARK*/
