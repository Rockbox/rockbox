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
#include "hwstub_internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef MIN
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif

int hwstub_probe(libusb_device *dev)
{
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config = NULL;
    int ret = -1;
    if(libusb_get_device_descriptor(dev, &dev_desc) != 0)
        goto Lend;
    if(libusb_get_config_descriptor(dev, 0, &config) != 0)
        goto Lend;
    ret = hwstub_rb_probe(&dev_desc, config);
    if(ret == -1)
        ret = hwstub_jz_probe(&dev_desc, config);
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

ssize_t hwstub_filter_device_list_by_vid_pid(libusb_device **list, ssize_t size,
    libusb_device ***new_list, hwstub_dev_list_filter_fn_t filter, void *user)
{
    *new_list = malloc(size * sizeof(libusb_device *));
    ssize_t index = 0;
    for(int i = 0; i < size; i++)
        if(filter(list[i], user))
            (*new_list)[index++] = libusb_ref_device(list[i]);
    return index;
}

static struct hwstub_device_t *hwstub_open_internal(struct hwstub_device_t *dev)
{
    if(dev == NULL)
        return NULL;
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
    dev->vtable.release(dev);
    return NULL;
}

struct hwstub_device_t *hwstub_open_usb(libusb_device *mydev)
{
    libusb_device_handle *handle;
    if(libusb_open(mydev, &handle) != 0)
        return NULL;
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config = NULL;
    int ret = -1;
    struct hwstub_device_t *dev = NULL;
    if(libusb_get_device_descriptor(mydev, &dev_desc) != 0)
        goto Lend;
    if(libusb_get_config_descriptor(mydev, 0, &config) != 0)
        goto Lend;
    ret = hwstub_rb_probe(&dev_desc, config);
    if(ret >= 0)
    {
        dev = hwstub_rb_open(handle, ret);
        goto Lend;
    }
    ret = hwstub_jz_probe(&dev_desc, config);
    if(ret == 0)
    {
        dev = hwstub_jz_open(handle);
        goto Lend;
    }
Lend:
    if(config)
        libusb_free_config_descriptor(config);
    if(dev == NULL)
        libusb_close(handle);
    return hwstub_open_internal(dev);
}

struct hwstub_device_t *hwstub_open_tcp(const char *host, const char *port)
{
    return hwstub_tcp_open(host, port);
}

void hwstub_release(struct hwstub_device_t *dev)
{
    return dev->vtable.release(dev);
}

int hwstub_get_desc(struct hwstub_device_t *dev, uint16_t desc, void *info, size_t sz)
{
    return dev->vtable.get_desc(dev, desc, info, sz);
}

int hwstub_get_log(struct hwstub_device_t *dev, void *buf, size_t sz)
{
    return dev->vtable.get_log(dev, buf, sz);
}

static int _hwstub_read(struct hwstub_device_t *dev, uint8_t breq, uint32_t addr,
    void *buf, size_t sz)
{
    return dev->vtable.read(dev, breq, addr, buf, sz);
}

static int _hwstub_write(struct hwstub_device_t *dev, uint8_t breq, uint32_t addr,
    const void *buf, size_t sz)
{
    return dev->vtable.write(dev, breq, addr, buf, sz);
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
    return dev->vtable.exec(dev, addr, flags);
}

int hwstub_call(struct hwstub_device_t *dev, uint32_t addr)
{
    return hwstub_exec(dev, addr, HWSTUB_EXEC_CALL);
}

int hwstub_jump(struct hwstub_device_t *dev, uint32_t addr)
{
    return hwstub_exec(dev, addr, HWSTUB_EXEC_JUMP);
}
