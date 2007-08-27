/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
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

#include <string.h>
#include <ctype.h>
#include "usbstack/core.h"

/**
 * 
 * Naming Convention for Endpoint Names
 * 
 *    - ep1, ep2, ... address is fixed, not direction or type
 *    - ep1in, ep2out, ... address and direction are fixed, not type
 *    - ep1-bulk, ep2-bulk, ... address and type are fixed, not direction
 *    - ep1in-bulk, ep2out-iso, ... all three are fixed
 *    - ep-* ... no functionality restrictions
 *
 * Type suffixes are "-bulk", "-iso", or "-int".  Numbers are decimal.
 *
 */
static int ep_matches(struct usb_ep* ep, struct usb_endpoint_descriptor* desc);

void usb_ep_autoconfig_reset(void)
{
    struct usb_ep* ep = NULL;
    if (usbcore.active_controller == NULL) {
        return;
    }

    logf("resetting endpoints");
    list_for_each_entry(ep, &usbcore.active_controller->endpoints.list, list) {
        logf("reset %s", ep->name);
        ep->claimed = false;
    }
}

/**
 * Find a suitable endpoint for the requested endpoint descriptor.
 * @param desc usb descritpro to use for seraching.
 * @return NULL or a valid endpoint.
 */
struct usb_ep* usb_ep_autoconfig(struct usb_endpoint_descriptor* desc)
{
    struct usb_ep* ep = NULL;
    if (usbcore.active_controller == NULL) {
        logf("active controller NULL");
        return NULL;
    }

    list_for_each_entry(ep, &usbcore.active_controller->endpoints.list, list) {
        if (ep_matches (ep, desc)) {
            return ep;
        }
    }

    return NULL;
}

static int ep_matches(struct usb_ep* ep, struct usb_endpoint_descriptor* desc)
{
    uint8_t     type;
    const char* tmp;
    uint16_t    max;

    /* endpoint already claimed? */
    if (ep->claimed) {
        logf("!! claimed !!");
        return 0;
    }

    /* only support ep0 for portable CONTROL traffic */
    type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
    if (type == USB_ENDPOINT_XFER_CONTROL) {
        logf("type == control");
        return 0;
    }

    /* some other naming convention */
    if (ep->name[0] != 'e') {
        logf("wrong name");
        return 0;
    }

    /* type-restriction:  "-iso", "-bulk", or "-int".
     * direction-restriction:  "in", "out".
     */
    if (ep->name[2] != '-' ) {
        tmp = strrchr (ep->name, '-');
        if (tmp) {
            switch (type) {
            case USB_ENDPOINT_XFER_INT:
                /* bulk endpoints handle interrupt transfers,
                 * except the toggle-quirky iso-synch kind
                 */
                if (tmp[2] == 's') { // == "-iso"
                    return 0;
                }
                break;
            case USB_ENDPOINT_XFER_BULK:
                if (tmp[1] != 'b') { // != "-bulk"
                    return 0;
                }
                break;
            case USB_ENDPOINT_XFER_ISOC:
                if (tmp[2] != 's') { // != "-iso"
                    return 0;
                }
            }
        } else {
            tmp = ep->name + strlen (ep->name);
        }

        /* direction-restriction:  "..in-..", "out-.." */
        tmp--;
        if (!isdigit(*tmp)) {
            if (desc->bEndpointAddress & USB_DIR_IN) {
                if ('n' != *tmp) {
                    return 0;
                }
            } else {
                if ('t' != *tmp) {
                    return 0;
                }
            }
        }
    }


    /* endpoint maxpacket size is an input parameter, except for bulk
     * where it's an output parameter representing the full speed limit.
     * the usb spec fixes high speed bulk maxpacket at 512 bytes.
     */
    max = 0x7ff & desc->wMaxPacketSize;

    switch (type) {
    case USB_ENDPOINT_XFER_INT:
        /* INT:  limit 64 bytes full speed, 1024 high speed */
        if ((usbcore.active_controller->speed != USB_SPEED_HIGH) && (max > 64)) {
            return 0;
        }
        /* FALLTHROUGH */

    case USB_ENDPOINT_XFER_ISOC:
        if ((usbcore.active_controller->speed != USB_SPEED_HIGH) && (max > 1023)) {
            return 0;
        }
        break;
    }

    /* MATCH!! */

    /* report address */
    desc->bEndpointAddress |= ep->ep_num;

    /* report (variable) full speed bulk maxpacket */
    if (type == USB_ENDPOINT_XFER_BULK) {
        int size = max;

        /* min() doesn't work on bitfields with gcc-3.5 */
        if (size > 64) {
            size = 64;
        }
        desc->wMaxPacketSize = size;
    }

    /* save desc in endpoint */
    ep->desc = desc;

    return 1;
}
