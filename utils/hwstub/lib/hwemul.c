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
#include "hwemul.h"
#include "hwemul_soc.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* requires then ->handle field only */
int hwemul_probe(struct hwemul_device_t *dev)
{
    libusb_device *mydev = libusb_get_device(dev->handle);

    int config_id;
    libusb_get_configuration(dev->handle, &config_id);
    struct libusb_config_descriptor *config;
    libusb_get_active_config_descriptor(mydev, &config);

    const struct libusb_endpoint_descriptor *endp = NULL;
    int intf;
    for(intf = 0; intf < config->bNumInterfaces; intf++)
    {
        if(config->interface[intf].num_altsetting != 1)
            continue;
        const struct libusb_interface_descriptor *interface =
            &config->interface[intf].altsetting[0];
        if(interface->bNumEndpoints != 3 ||
                interface->bInterfaceClass != HWEMUL_CLASS ||
                interface->bInterfaceSubClass != HWEMUL_SUBCLASS ||
                interface->bInterfaceProtocol != HWEMUL_PROTOCOL)
            continue;
        dev->intf = intf;
        dev->bulk_in = dev->bulk_out = dev->int_in = -1;
        for(int ep = 0; ep < interface->bNumEndpoints; ep++)
        {
            endp = &interface->endpoint[ep];
            if((endp->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT &&
                    (endp->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                dev->int_in = endp->bEndpointAddress;
            if((endp->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK &&
                    (endp->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                dev->bulk_in = endp->bEndpointAddress;
            if((endp->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK &&
                    (endp->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT)
                dev->bulk_out = endp->bEndpointAddress;
        }
        if(dev->bulk_in == -1 || dev->bulk_out == -1 || dev->int_in == -1)
            continue;
        break;
    }
    if(intf == config->bNumInterfaces)
        return 1;

    return libusb_claim_interface(dev->handle, intf);
}

int hwemul_release(struct hwemul_device_t *dev)
{
    return libusb_release_interface(dev->handle, dev->intf);
}

int hwemul_get_info(struct hwemul_device_t *dev, uint16_t idx, void *info, size_t sz)
{
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
        HWEMUL_GET_INFO, 0, idx, info, sz, 1000);
}

int hwemul_get_log(struct hwemul_device_t *dev, void *buf, size_t sz)
{
    return libusb_control_transfer(dev->handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
        HWEMUL_GET_LOG, 0, 0, buf, sz, 1000);
}

int hwemul_rw_mem(struct hwemul_device_t *dev, int read, uint32_t addr, void *buf, size_t sz)
{
    size_t tot_sz = 0;
    while(sz)
    {
        uint16_t xfer = MIN(1 * 1024, sz);
        int ret = libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            (read ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT),
            HWEMUL_RW_MEM, addr & 0xffff, addr >> 16, buf, xfer, 1000);
        if(ret != xfer)
            return ret;
        sz -= xfer;
        addr += xfer;
        buf += xfer;
        tot_sz += xfer;
    }
    return tot_sz;
}

int hwemul_call(struct hwemul_device_t *dev, uint32_t addr)
{
    return libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            LIBUSB_ENDPOINT_OUT, HWEMUL_CALL, addr & 0xffff, addr >> 16, NULL, 0,
            1000);
}

int hwemul_jump(struct hwemul_device_t *dev, uint32_t addr)
{
    return libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            LIBUSB_ENDPOINT_OUT, HWEMUL_JUMP, addr & 0xffff, addr >> 16, NULL, 0,
            1000);
}

const char *hwemul_get_product_string(struct usb_resp_info_stmp_t *stmp)
{
    switch(stmp->chipid)
    {
        case 0x3700: return "STMP 3700";
        case 0x37b0: return "STMP 3770";
        case 0x3780: return "STMP 3780 / i.MX233";
        default: return "unknown";
    }
}

const char *hwemul_get_rev_string(struct usb_resp_info_stmp_t *stmp)
{
    switch(stmp->chipid)
    {
        case 0x37b0:
        case 0x3780:
            switch(stmp->rev)
            {
                case 0: return "TA1";
                case 1: return "TA2";
                case 2: return "TA3";
                case 3: return "TA4";
                default: return "unknown";
            }
            break;
        default:
            return "unknown";
    }
}

int hwemul_aes_otp(struct hwemul_device_t *dev, void *buf, size_t sz, uint16_t param)
{
    int ret = libusb_control_transfer(dev->handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE |
            LIBUSB_ENDPOINT_OUT, HWEMUL_AES_OTP, param, 0, buf, sz,
            1000);
    if(ret <0 || (unsigned)ret != sz)
        return -1;
    int xfer;
    ret = libusb_interrupt_transfer(dev->handle, dev->int_in, buf, sz, &xfer, 1000);
    if(ret < 0 || (unsigned)xfer != sz)
        return -1;
    return ret;
}
