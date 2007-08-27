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
 * Based on linux/drivers/usb/gadget/config.c
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

static int usb_descriptor_fillbuf(void* buf, unsigned buflen, struct usb_descriptor_header** src)
{
    uint8_t* dest = buf;

    if (!src) {
        return -EINVAL;
    }

    /* fill buffer from src[] until null descriptor ptr */
    for (; 0 != *src; src++) {
        unsigned len = (*src)->bLength;

        logf("len: %d", len);

        if (len > buflen)
            return -EINVAL;
        memcpy(dest, *src, len);
        buflen -= len;
        dest += len;
    }
    return dest - (uint8_t *)buf;
}

int usb_stack_configdesc(const struct usb_config_descriptor* config, void* buf, unsigned length, struct usb_descriptor_header** desc)
{
    struct usb_config_descriptor* cp = buf;
    int len;

    if (length < USB_DT_CONFIG_SIZE || !desc) {
        return -EINVAL;
    }

    /* config descriptor first */
    *cp = *config; 

    /* then interface/endpoint/class/vendor/... */
    len = usb_descriptor_fillbuf(USB_DT_CONFIG_SIZE + (uint8_t*)buf, length - USB_DT_CONFIG_SIZE, desc);

    if (len < 0) {
        return len;
    }

    len += USB_DT_CONFIG_SIZE;
    if (len > 0xffff) {
        return -EINVAL;
    }

    /* patch up the config descriptor */
    cp->bLength = USB_DT_CONFIG_SIZE;
    cp->bDescriptorType = USB_DT_CONFIG;
    cp->wTotalLength = len;
    cp->bmAttributes |= USB_CONFIG_ATT_ONE;

    return len;
}
