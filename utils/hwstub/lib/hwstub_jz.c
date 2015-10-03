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

#ifndef MIN
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif

struct hwstub_jz_device_t
{
    struct hwstub_device_t dev;
    libusb_device_handle *handle;
};

#define JZ_DEV(to, from) struct hwstub_jz_device_t *to = (void *)from;

#define VR_GET_CPU_INFO         0
#define VR_SET_DATA_ADDRESS     1
#define VR_SET_DATA_LENGTH      2
#define VR_FLUSH_CACHES         3
#define VR_PROGRAM_START1       4
#define VR_PROGRAM_START2       5

static int jz_cpuinfo(libusb_device_handle *dev, char *cpuinfo)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_GET_CPU_INFO, 0, 0, (void *)cpuinfo, 8, 1000);
}

static int jz_set_addr(libusb_device_handle *dev, uint32_t addr)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_ADDRESS, addr >> 16, addr & 0xffff, NULL, 0, 1000);
}

static int jz_set_length(libusb_device_handle *dev, uint32_t length)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_LENGTH, length >> 16, length & 0xffff, NULL, 0, 1000);
}

static int jz_upload(libusb_device_handle *dev, void *data, size_t length)
{
    int xfered;
    if(libusb_bulk_transfer(dev, LIBUSB_ENDPOINT_IN | 1, data, length, 
            &xfered, 1000) == 0)
        return xfered;
    else
        return -1;
}

static int jz_download(libusb_device_handle *dev, const void *data, size_t length)
{
    int xfered;
    if(libusb_bulk_transfer(dev, LIBUSB_ENDPOINT_OUT | 1, (void *)data, length, 
            &xfered, 1000) == 0)
        return xfered;
    else
        return -1;
}

static int jz_start1(libusb_device_handle *dev, uint32_t addr)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START1, addr >> 16, addr & 0xffff, NULL, 0, 1000);
}

static int jz_flush_caches(libusb_device_handle *dev)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_FLUSH_CACHES, 0, 0, NULL, 0, 1000);
}

static int jz_start2(libusb_device_handle *dev, uint32_t addr)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START2, addr >> 16, addr & 0xffff, NULL, 0, 1000);
}

int hwstub_jz_probe(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config)
{
    (void) config;
    /* don't bother checking the config descriptor and use the device ID only */
    if(dev->idVendor ==  0x601a && dev->idProduct >= 0x4740 && dev->idProduct <= 0x4780)
        return 0;
    return -1;
}

static void hwstub_jz_release(struct hwstub_device_t *_dev)
{
    JZ_DEV(dev, _dev)
    libusb_close(dev->handle);
    free(dev);
}

static struct hwstub_version_desc_t hwstub_jz_desc_version =
{
    .bLength = sizeof(hwstub_jz_desc_version),
    .bDescriptorType = HWSTUB_DT_VERSION,
    .bMajor = HWSTUB_VERSION_MAJOR,
    .bMinor = HWSTUB_VERSION_MINOR,
    .bRevision = 0,
};

static struct hwstub_layout_desc_t hwstub_jz_desc_layout =
{
    .bLength = sizeof(hwstub_jz_desc_layout),
    .bDescriptorType = HWSTUB_DT_LAYOUT,
    .dCodeStart = 0xbfc00000, /* ROM */
    .dCodeSize = 0x2000, /* 8kB per datasheet */
    .dStackStart = 0, /* I think the ROM doesn't use a stack ?! */
    .dStackSize = 0,
    .dBufferStart =  0x080000000,
    .dBufferSize = 0x4000,
};

static struct hwstub_target_desc_t hwstub_jz_desc_target =
{
    .bLength = sizeof(hwstub_jz_desc_target),
    .bDescriptorType = HWSTUB_DT_TARGET,
    .dID = HWSTUB_TARGET_JZ,
    .bName = "Ingenic JZ47xx",
};

static struct hwstub_jz_desc_t hwstub_jz_desc_jz =
{
    .bLength = sizeof(hwstub_jz_desc_jz),
    .bDescriptorType = HWSTUB_DT_JZ,
    .wChipID = 0,
};

static uint16_t bcd(char *bcd)
{
    uint16_t v = 0;
    for(int i = 0; i < 4; i++)
        v = (bcd[i] - '0') | v << 4;
    return v;
}

static uint16_t parse_cpuinfo(struct hwstub_jz_device_t *dev, char *cpuinfo)
{
    /* if cpuinfo if of the form JZxxxxVy then extract xxxx */
    if(cpuinfo[0] == 'J' && cpuinfo[1] == 'Z' && cpuinfo[6] == 'V')
        return bcd(cpuinfo + 2);
    /* if cpuinfo if of the form Bootxxxx then extract xxxx */
    else if(strncmp(cpuinfo, "Boot", 4) == 4)
        return bcd(cpuinfo + 4);
    /* else use usb id */
    else
    {
        struct libusb_device_descriptor dev_desc;
        if(libusb_get_device_descriptor(libusb_get_device(dev->handle), &dev_desc) == 0)
            return dev_desc.idProduct;
        else
            return 0;
    }
}

static int hwstub_jz_get_desc(struct hwstub_device_t *_dev, uint16_t desc, void *info, size_t sz)
{
    (void) _dev;
    void *p = NULL;
    switch(desc)
    {
        case HWSTUB_DT_VERSION: p = &hwstub_jz_desc_version; break;
        case HWSTUB_DT_LAYOUT: p = &hwstub_jz_desc_layout; break;
        case HWSTUB_DT_TARGET: p = &hwstub_jz_desc_target; break;
        case HWSTUB_DT_JZ: p = &hwstub_jz_desc_jz; break;
        default: break;
    }
    if(p == NULL)
        return -1;
    /* size is in the bLength field of the descriptor */
    uint8_t desc_sz = *(uint8_t *)p;
    sz = MIN(sz, desc_sz);
    memcpy(info, p, sz);
    return sz;
}

static int hwstub_jz_get_log(struct hwstub_device_t *_dev, void *buf, size_t sz)
{
    (void) _dev;
    (void) buf;
    (void) sz;
    return 0;
}

static int hwstub_jz_read(struct hwstub_device_t *_dev, uint8_t breq, uint32_t addr,
    void *buf, size_t sz)
{
    (void) breq;
    JZ_DEV(dev, _dev)
    /* FIXME we cannot be sure how about the ROM handles reads, atomic transfers
     * may not be atomic */
    if(jz_set_addr(dev->handle, addr) != 0)
        return -1;
    if(jz_set_length(dev->handle, sz) != 0)
        return -1;
    return jz_upload(dev->handle, buf, sz);
}

static int hwstub_jz_write(struct hwstub_device_t *_dev, uint8_t breq, uint32_t addr,
    const void *buf, size_t sz)
{
    JZ_DEV(dev, _dev)
    (void) breq;
    /* FIXME we cannot be sure how about the ROM handles reads, atomic transfers
     * may not be atomic */
    if(jz_set_addr(dev->handle, addr) != 0)
        return -1;
    if(jz_set_length(dev->handle, sz) != 0)
        return -1;
    return jz_download(dev->handle, buf, sz);
}

static int hwstub_jz_exec(struct hwstub_device_t *_dev, uint32_t addr, uint16_t flags)
{
    JZ_DEV(dev, _dev)
    (void) flags;
    /* assume that exec at 0x80000000 is a first stage load with START1, otherwise
     * flush cache and use START2 */
    if(addr == 0x80000000)
        return jz_start1(dev->handle, addr);
    if(jz_flush_caches(dev->handle) != 0)
        return -1;
    return jz_start2(dev->handle, addr);
}

static struct hwstub_device_vtable_t hwstub_jz_vtable =
{
    .release = hwstub_jz_release,
    .get_log = hwstub_jz_get_log,
    .get_desc = hwstub_jz_get_desc,
    .read = hwstub_jz_read,
    .write = hwstub_jz_write,
    .exec = hwstub_jz_exec,
};


struct hwstub_device_t *hwstub_jz_open(libusb_device_handle *handle)
{
    struct hwstub_jz_device_t *dev = malloc(sizeof(struct hwstub_jz_device_t));
    memset(dev, 0, sizeof(struct hwstub_jz_device_t));
    dev->dev.vtable = hwstub_jz_vtable;
    dev->handle = handle;
    char cpuinfo[8];
    if(jz_cpuinfo(dev->handle, cpuinfo) != 8)
    {
        free(dev);
        return NULL;
    }
    hwstub_jz_desc_jz.wChipID = parse_cpuinfo(dev, cpuinfo);
    return &dev->dev;
}