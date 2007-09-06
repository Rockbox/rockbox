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
 * Based on linux/drivers/usb/gadget/usbstring.c
 *   Copyright (C) 2003 David Brownell 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include "usbstack/core.h"

void into_usb_ctrlrequest(struct usb_ctrlrequest* request)
{
    char* type = "";
    char* req = "";
    char* extra = 0;

    logf("-usb request-");
    /* check if packet is okay */
    if (request->bRequestType == 0 &&
        request->bRequest == 0 &&
        request->wValue == 0 &&
        request->wIndex == 0 &&
        request->wLength == 0) {
        logf(" -> INVALID <-");
        return;
    }

    switch (request->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:
        type = "standard";

        switch (request->bRequest) {
        case USB_REQ_GET_STATUS:
            req  = "get status";
            break;
        case USB_REQ_CLEAR_FEATURE:
            req = "clear feature";
            break;
        case USB_REQ_SET_FEATURE:
            req = "set feature";
            break;
        case USB_REQ_SET_ADDRESS:
            req = "set address";
            break;
        case USB_REQ_GET_DESCRIPTOR:
            req = "get descriptor";

            switch (request->wValue >> 8) {
            case USB_DT_DEVICE:
                extra = "get device descriptor";
                break;
            case USB_DT_DEVICE_QUALIFIER:
                extra = "get device qualifier";
                break;
            case USB_DT_OTHER_SPEED_CONFIG:
                extra = "get other-speed config descriptor";
            case USB_DT_CONFIG:
                extra = "get configuration descriptor";
                break;
            case USB_DT_STRING:
                extra = "get string descriptor";
                break;
            case USB_DT_DEBUG:
                extra = "debug";
                break;
            }
            break;

            break;
        case USB_REQ_SET_DESCRIPTOR:
            req = "set descriptor";
            break;
        case USB_REQ_GET_CONFIGURATION:
            req = "get configuration";
            break;
        case USB_REQ_SET_CONFIGURATION:
            req = "set configuration";
            break;
        case USB_REQ_GET_INTERFACE:
            req = "get interface";
            break;
        case USB_REQ_SET_INTERFACE:
            req = "set interface";
            break;
        case USB_REQ_SYNCH_FRAME:
            req = "sync frame";
            break;
        default:
            req = "unkown";
            break;
        }

        break;
    case USB_TYPE_CLASS:
        type = "class";
        break;

    case USB_TYPE_VENDOR:
        type = "vendor";
        break;
    }

    logf(" -b 0x%x", request->bRequestType);
    logf(" -b 0x%x", request->bRequest);
    logf(" -b 0x%x", request->wValue);
    logf(" -b 0x%x", request->wIndex);
    logf(" -b 0x%x", request->wLength);
    logf(" -> t: %s", type);
    logf(" -> r: %s", req);
    if (extra != 0) {
        logf(" -> e: %s", extra);
    }
}

int usb_stack_get_string(struct usb_string* strings, int id, uint8_t* buf)
{
    struct usb_string* tmp;
    char* sp, *dp;
    int len;

    /* if id is 0, then we need to send back all supported
     * languages. In our case we only support one
     * language: en-us (0x0409) */
    if (id == 0) {
        buf [0] = 4;
        buf [1] = USB_DT_STRING;
        buf [2] = (uint8_t) 0x0409;
        buf [3] = (uint8_t) (0x0409 >> 8);
        return 4;
    }

    /* look for string */
    for (tmp = strings; tmp && tmp->s; tmp++) {
        if (tmp->id == id) {
            break;
        }
    }

    /* did we found it? */
    if (!tmp || !tmp->s) {
        return -EINVAL;
    }

    len = MIN ((size_t) 126, strlen (tmp->s));
    memset(buf + 2, 0, 2 * len);

    /* convert to utf-16le */
    sp = (char*)tmp->s;
    dp = (char*)&buf[2];

    while (*sp) {
        *dp++ = *sp++;
        *dp++ = 0;
    }

    /* write len and tag */
    buf [0] = (len + 1) * 2;
    buf [1] = USB_DT_STRING;

    return buf[0];
}
