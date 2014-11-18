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
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif

struct hwstub_device_t
{
    libusb_device_handle *handle;
    int intf;
    unsigned buf_sz;
    uint16_t id;
    uint8_t minor_ver;
};

int hwstub_probe(libusb_device *dev)
{
    struct libusb_config_descriptor *config = NULL;
    int ret = -1;
    if(libusb_get_config_descriptor(dev, 0, &config) != 0)
        goto Lend;
    /* search hwstub interface */
    for(unsigned i = 0; i < config->bNumInterfaces; i++)
    {
        /* hwstub interface has only one setting */
        if(config->interface[i].num_altsetting != 1)
            continue;
        const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
        /* check class/subclass/protocol */
        if(intf->bInterfaceClass != HWSTUB_CLASS ||
                intf->bInterfaceSubClass != HWSTUB_SUBCLASS ||
                intf->bInterfaceProtocol != HWSTUB_PROTOCOL)
            continue;
        /* found ! */
        ret = i;
        break;
    }
Lend:
    if(config)
        libusb_free_config_descriptor(config);
    return ret;
}

ssize_t hwstub_get_device_list(libusb_context *ctx, libusb_device ***list)
{
    libusb_device **great_list;
    ssize_t great_cnt = libusb_get_device_list(ctx, &great_list);
    if(great_cnt < 0)
        return great_cnt;
    /* allocate a list (size at least one NULL entry at the end) */
    libusb_device **mylist = malloc(sizeof(libusb_device *) * (great_cnt + 1));
    memset(mylist, 0, sizeof(libusb_device *) * (great_cnt + 1));
    /* list hwstub devices */
    ssize_t cnt = 0;
    for(int i = 0; i < great_cnt; i++)
        if(hwstub_probe(great_list[i]) >= 0)
        {
            libusb_ref_device(great_list[i]);
            mylist[cnt++] = great_list[i];
        }
    /* free old list */
    libusb_free_device_list(great_list, 1);
    /* return */
    *list = mylist;
    return cnt;
}

struct hwstub_device_t *hwstub_open(libusb_device_handle *handle)
{
    struct hwstub_device_t *dev = malloc(sizeof(struct hwstub_device_t));
    memset(dev, 0, sizeof(struct hwstub_device_t));
    dev->handle = handle;
    dev->intf = -1;
    dev->buf_sz = 1024; /* default size */
    libusb_device *mydev = libusb_get_device(dev->handle);
    dev->intf = hwstub_probe(mydev);
    if(dev->intf == -1)
        goto Lerr;
    /* try to get version */
    struct hwstub_version_desc_t m_hwdev_ver;
    int sz = hwstub_get_desc(dev, HWSTUB_DT_VERSION, &m_hwdev_ver, sizeof(m_hwdev_ver));
    if(sz != sizeof(m_hwdev_ver))
        goto Lerr;
    /* major version must match, minor version is taken to be the minimum between
     * what library and device support */
    if(m_hwdev_ver.bMajor != HWSTUB_VERSION_MAJOR)
        goto Lerr;
    dev->minor_ver = MIN(m_hwdev_ver.bMinor, HWSTUB_VERSION_MINOR);
    /* try to get actual buffer size */
    struct hwstub_layout_desc_t layout;
    sz = hwstub_get_desc(dev, HWSTUB_DT_LAYOUT, &layout, sizeof(layout));
    if(sz == (int)sizeof(layout))
        dev->buf_sz = layout.dBufferSize;
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
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_DESCRIPTOR, desc << 8, dev->intf, info, sz, 1000);
}

int hwstub_get_log(struct hwstub_device_t *dev, void *buf, size_t sz)
{
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        HWSTUB_GET_LOG, 0, dev->intf, buf, sz, 1000);
}

static int _hwstub_read(struct hwstub_device_t *dev, uint8_t breq, uint32_t addr,
    void *buf, size_t sz)
{
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

static int _hwstub_write(struct hwstub_device_t *dev, uint8_t breq, uint32_t addr,
    const void *buf, size_t sz)
{
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

int hwstub_read_atomic(struct hwstub_device_t *dev, uint32_t addr, void *buf, size_t sz)
{
    /* reject any read greater than the buffer, it makes no sense anyway */
    if(sz > dev->buf_sz)
        return -1;
    return _hwstub_read(dev, HWSTUB_READ2_ATOMIC, addr, buf, sz);
}

int hwstub_write_atomic(struct hwstub_device_t *dev, uint32_t addr, const void *buf, size_t sz)
{
    /* reject any write greater than the buffer, it makes no sense anyway */
    if(sz + sizeof(struct hwstub_write_req_t) > dev->buf_sz)
        return -1;
    return _hwstub_write(dev, HWSTUB_WRITE_ATOMIC, addr, buf, sz);
}

/* Intermediate function which make sure we don't overflow the device buffer */
int hwstub_read(struct hwstub_device_t *dev, uint32_t addr, void *buf, size_t sz)
{
    int cnt = 0;
    while(sz > 0)
    {
        int xfer = _hwstub_read(dev, HWSTUB_READ2, addr, buf, MIN(sz, dev->buf_sz));
        if(xfer <  0)
            return xfer;
        sz -= xfer;
        buf += xfer;
        addr += xfer;
        cnt += xfer;
    }
    return cnt;
}

/* Intermediate function which make sure we don't overflow the device buffer */
int hwstub_write(struct hwstub_device_t *dev, uint32_t addr, const void *buf, size_t sz)
{
    int cnt = 0;
    while(sz > 0)
    {
        int xfer = _hwstub_write(dev, HWSTUB_WRITE, addr, buf, 
            MIN(sz, dev->buf_sz - sizeof(struct hwstub_write_req_t)));
        if(xfer <  0)
            return xfer;
        sz -= xfer;
        buf += xfer;
        addr += xfer;
        cnt += xfer;
    }
    return cnt;
}

int hwstub_rw_mem(struct hwstub_device_t *dev, int read, uint32_t addr, void *buf, size_t sz)
{
    return read ? hwstub_read(dev, addr, buf, sz) : hwstub_write(dev, addr, buf, sz);
}

int hwstub_rw_mem_atomic(struct hwstub_device_t *dev, int read, uint32_t addr, void *buf, size_t sz)
{
    return read ? hwstub_read_atomic(dev, addr, buf, sz) :
        hwstub_write_atomic(dev, addr, buf, sz);
}

int hwstub_exec(struct hwstub_device_t *dev, uint32_t addr, uint16_t flags)
{
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

int hwstub_call(struct hwstub_device_t *dev, uint32_t addr)
{
    return hwstub_exec(dev, addr, HWSTUB_EXEC_CALL);
}

int hwstub_jump(struct hwstub_device_t *dev, uint32_t addr)
{
    return hwstub_exec(dev, addr, HWSTUB_EXEC_JUMP);
}
