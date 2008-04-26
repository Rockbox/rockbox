/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2008 by Frank Gevaerts
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

#ifdef USB_CHARGING_ONLY

/* charging_only interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
                                interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = USB_CLASS_VENDOR_SPEC,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0
};


static int usb_interface;

int usb_charging_only_set_first_endpoint(int endpoint)
{
    /* The dummy charging_only driver doesn't need an endpoint pair */
    return endpoint;
}
int usb_charging_only_set_first_interface(int interface)
{
    usb_interface = interface;
    return interface + 1;
}

int usb_charging_only_get_config_descriptor(unsigned char *dest,int max_packet_size)
{
    (void)max_packet_size;
    unsigned char *orig_dest = dest;

    interface_descriptor.bInterfaceNumber=usb_interface;
    memcpy(dest,&interface_descriptor,sizeof(struct usb_interface_descriptor));

    dest+=sizeof(struct usb_interface_descriptor);

    return (dest-orig_dest);
}

#endif /*USB_CHARGING_ONLY*/
