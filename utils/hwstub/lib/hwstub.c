/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "hwstub.h"
#include <string.h>
#include <stdlib.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

struct hwstub_device_t
{
    libusb_device_handle *handle;
    int intf;
    int bulk_in;
    int bulk_out;
    int int_in;
};

struct hwstub_device_t *hwstub_open(libusb_device_handle *handle)
{
    struct hwstub_device_t *dev = malloc(sizeof(struct hwstub_device_t));
    memset(dev, 0, sizeof(struct hwstub_device_t));
    dev->handle = handle;
    libusb_device *mydev = libusb_get_device(dev->handle);

    int config_id;
    libusb_get_configuration(dev->handle, &config_id);
    struct libusb_device_descriptor dev_desc;
    libusb_get_device_descriptor(mydev, &dev_desc);
    if(dev_desc.bDeviceClass != HWSTUB_CLASS ||
            dev_desc.bDeviceSubClass != HWSTUB_SUBCLASS ||
            dev_desc.bDeviceProtocol != HWSTUB_PROTOCOL)
        goto Lerr;
    return dev;

Lerr:
    free(dev);
    return NULL;
}

int hwstub_release(struct hwstub_device_t *dev)
{
    free(dev);
    return 0;
}

int hwstub_get_desc(struct hwstub_device_t *dev, uint16_t desc, void *info, size_t sz)
{
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_DESCRIPTOR, desc << 8, 0, info, sz, 1000);
}

int hwstub_get_log(struct hwstub_device_t *dev, void *buf, size_t sz)
{
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
        HWSTUB_GET_LOG, 0, 0, buf, sz, 1000);
}

int hwstub_rw_mem(struct hwstub_device_t *dev, int read, uint32_t addr, void *buf, size_t sz)
{
    size_t tot_sz = 0;
    while(sz)
    {
        uint16_t xfer = MIN(1 * 1024, sz);
        int ret = libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            (read ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT),
            HWSTUB_RW_MEM, addr & 0xffff, addr >> 16, buf, xfer, 1000);
        if(ret != xfer)
            return ret;
        sz -= xfer;
        addr += xfer;
        buf += xfer;
        tot_sz += xfer;
    }
    return tot_sz;
}

int hwstub_call(struct hwstub_device_t *dev, uint32_t addr)
{
    return libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            LIBUSB_ENDPOINT_OUT, HWSTUB_CALL, addr & 0xffff, addr >> 16, NULL, 0,
            1000);
}

int hwstub_jump(struct hwstub_device_t *dev, uint32_t addr)
{
    return libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            LIBUSB_ENDPOINT_OUT, HWSTUB_JUMP, addr & 0xffff, addr >> 16, NULL, 0,
            1000);
}
