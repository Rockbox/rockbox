/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Frank Gevaerts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _USB_CLASS_DRIVER_H_
#define _USB_CLASS_DRIVER_H_

/* Common api, implemented by all class drivers */

struct usb_class_driver {
    bool enabled;
    bool needs_exclusive_ata;
    int usb_endpoint;
    int usb_interface;

    /* Asks the driver to put the interface descriptor and all other
       needed descriptor for this driver at dest, for the given settings.
       Returns the number of bytes taken by these descriptors. */
    int (*get_config_descriptor)(unsigned char *dest,
              int max_packet_size, int interface_number, int endpoint);

    /* Tells the driver that a usb connection has been set up and is now
       ready to use. */
    void (*init_connection)(int interface,int endpoint);

    /* Initialises the driver. This can be called multiple times,
       and should not perform any action that can disturb other threads
       (like getting the audio buffer) */
    void (*init)(void);

    /* Tells the driver that the usb connection is no longer active */
    void (*disconnect)(void);

    /* Tells the driver that a usb transfer has been completed. Note that "in"
       is relative to the host */
    void (*transfer_complete)(bool in, int status, int length);

    /* Tells the driver that a control request has come in. If the driver is
       able to handle it, it should ack the request, and return true. Otherwise
       it should return false. */
    bool (*control_request)(struct usb_ctrlrequest* req);

#ifdef HAVE_HOTSWAP
    /* Tells the driver that a hotswappable disk/card was inserted or
       extracted */
    void (*notify_hotswap)(int volume, bool inserted);
#endif
};

#endif
