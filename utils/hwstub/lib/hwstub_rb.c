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
#include "hwstub_internal.h"

struct hwstub_rb_device_t
{
    struct hwstub_device_t dev;
    libusb_device_handle *handle;
    int intf;
    uint16_t id;
};

#define RB_DEV(to, from) struct hwstub_rb_device_t *to = (void *)from;

int hwstub_rb_probe(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config)
{
    (void) dev;
    /* search hwstub interface */
    for(unsigned i = 0; i < config->bNumInterfaces; i++)
    {
        /* hwstub interface has only one setting */
        if(config->interface[i].num_altsetting != 1)
            continue;
        const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
        /* check class/subclass/protocol */
        if(intf->bInterfaceClass == HWSTUB_CLASS &&
                intf->bInterfaceSubClass == HWSTUB_SUBCLASS &&
                intf->bInterfaceProtocol == HWSTUB_PROTOCOL)
            /* found it ! */
            return i;
    }
    return -1;
}

static void hwstub_rb_release(struct hwstub_device_t *_dev)
{
    RB_DEV(dev, _dev)
    libusb_close(dev->handle);
    free(dev);
}

static int hwstub_rb_get_desc(struct hwstub_device_t *_dev, uint16_t desc, void *info, size_t sz)
{
    RB_DEV(dev, _dev)
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_DESCRIPTOR, desc << 8, dev->intf, info, sz, 1000);
}

static int hwstub_rb_get_log(struct hwstub_device_t *_dev, void *buf, size_t sz)
{
    RB_DEV(dev, _dev)
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        HWSTUB_GET_LOG, 0, dev->intf, buf, sz, 1000);
}

static int hwstub_rb_read(struct hwstub_device_t *_dev, uint8_t breq, uint32_t addr,
    void *buf, size_t sz)
{
    RB_DEV(dev, _dev)
    struct hwstub_read_req_t read;
    read.dAddress = addr;
    int size = libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_READ, dev->id, dev->intf, (void *)&read, sizeof(read), 1000);
    if(size != (int)sizeof(read))
        return -1;
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        breq, dev->id++, dev->intf, buf, sz, 1000);
}

static int hwstub_rb_write(struct hwstub_device_t *_dev, uint8_t breq, uint32_t addr,
    const void *buf, size_t sz)
{
    RB_DEV(dev, _dev)
    size_t hdr_sz = sizeof(struct hwstub_write_req_t);
    struct hwstub_write_req_t *req = malloc(sz + hdr_sz);
    req->dAddress = addr;
    memcpy(req + 1, buf, sz);
    int size = libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        breq, dev->id++, dev->intf, (void *)req, sz + hdr_sz, 1000);
    free(req);
    return size - hdr_sz;
}

static int hwstub_rb_exec(struct hwstub_device_t *_dev, uint32_t addr, uint16_t flags)
{
    RB_DEV(dev, _dev)
    struct hwstub_exec_req_t exec;
    exec.dAddress = addr;
    exec.bmFlags = flags;
    int size = libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_EXEC, dev->id, dev->intf, (void *)&exec, sizeof(exec), 1000);
    if(size != (int)sizeof(exec))
        return -1;
    return 0;
}

static struct hwstub_device_vtable_t hwstub_rb_vtable =
{
    .release = hwstub_rb_release,
    .get_log = hwstub_rb_get_log,
    .get_desc = hwstub_rb_get_desc,
    .read = hwstub_rb_read,
    .write = hwstub_rb_write,
    .exec = hwstub_rb_exec,
};

struct hwstub_device_t *hwstub_rb_open(libusb_device_handle *handle, int intf)
{
    struct hwstub_rb_device_t *dev = malloc(sizeof(struct hwstub_rb_device_t));
    memset(dev, 0, sizeof(struct hwstub_rb_device_t));
    dev->dev.vtable = hwstub_rb_vtable;
    dev->handle = handle;
    dev->intf = intf;
    return &dev->dev;
}
