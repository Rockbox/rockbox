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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _USB_CLASS_DRIVER_H_
#define _USB_CLASS_DRIVER_H_

/* Common api, implemented by all class drivers */

struct usb_class_driver {
	/* First some runtime data */
    bool enabled;
    int first_interface;
    int last_interface;

	/* Driver api starts here */

	/* Set this to true if the driver needs exclusive disk access (e.g. usb storage) */
    bool needs_exclusive_ata;

    /* Tells the driver what its first interface number will be. The driver
       returns the number of the first available interface for the next driver
       (i.e. a driver with one interface will return interface+1)
	   A driver must have at least one interface
	   Mandatory function */
    int (*set_first_interface)(int interface);

    /* Tells the driver what its first endpoint pair number will be. The driver
       returns the number of the first available endpoint pair for the next
       driver (i.e. a driver with one endpoint pair will return endpoint +1)
	   Mandatory function */
    int (*set_first_endpoint)(int endpoint);

    /* Asks the driver to put the interface descriptor and all other
       needed descriptor for this driver at dest.
       Returns the number of bytes taken by these descriptors.
	   Mandatory function */
    int (*get_config_descriptor)(unsigned char *dest, int max_packet_size);

    /* Tells the driver that a usb connection has been set up and is now
       ready to use. 
	   Optional function */
    void (*init_connection)(void);

    /* Initialises the driver. This can be called multiple times,
       and should not perform any action that can disturb other threads
       (like getting the audio buffer)
	   Optional function */
    void (*init)(void);

    /* Tells the driver that the usb connection is no longer active 
	   Optional function */
    void (*disconnect)(void);

    /* Tells the driver that a usb transfer has been completed. Note that "in"
       is relative to the host 
	   Optional function */
    void (*transfer_complete)(int ep,bool in, int status, int length);

    /* Tells the driver that a control request has come in. If the driver is
       able to handle it, it should ack the request, and return true. Otherwise
       it should return false. 
	   Optional function */
    bool (*control_request)(struct usb_ctrlrequest* req);

#ifdef HAVE_HOTSWAP
    /* Tells the driver that a hotswappable disk/card was inserted or
       extracted 
	   Optional function */
    void (*notify_hotswap)(int volume, bool inserted);
#endif
};

#endif
