/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
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
#ifndef __HWSTUB_INTERNAL__
#define __HWSTUB_INTERNAL__

/**
 * INTERNAL use ONLY
 */

#include "hwstub.h"

/**
 * Probing
 */

/* Returns 0 if a device interface is usable, or -1 if none was found */
int hwstub_probe(libusb_device *dev);
/* return <0 on failure, interface index on success */
int hwstub_rb_probe(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config);
/* return <0 on failure, 0 on success */
int hwstub_jz_probe(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config);

/**
 * Opening
 */
struct hwstub_device_t *hwstub_rb_open(libusb_device_handle *handle, int intf);
struct hwstub_device_t *hwstub_jz_open(libusb_device_handle *handle);
struct hwstub_device_t *hwstub_tcp_open(const char *host, const char *port);

#endif /* __HWSTUB_INTERNAL__ */
 
