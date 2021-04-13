/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "jztool_private.h"
#include <stdlib.h>
#include <stdbool.h>

#define VR_GET_CPU_INFO     0
#define VR_SET_DATA_ADDRESS 1
#define VR_SET_DATA_LENGTH  2
#define VR_FLUSH_CACHES     3
#define VR_PROGRAM_START1   4
#define VR_PROGRAM_START2   5

int jz_usb_open(jz_context* jz, jz_usbdev** devptr, uint16_t vend_id, uint16_t prod_id)
{
    int rc;
    jz_usbdev* dev = NULL;
    libusb_device_handle* usb_handle = NULL;
    libusb_device** dev_list = NULL;
    ssize_t dev_index = -1, dev_count;

    rc = jz_context_ref_libusb(jz);
    if(rc < 0)
        return rc;

    dev = malloc(sizeof(struct jz_usbdev));
    if(!dev) {
        rc = JZ_ERR_OUT_OF_MEMORY;
        goto error;
    }

    dev_count = libusb_get_device_list(jz->usb_ctx, &dev_list);
    if(dev_count < 0) {
        jz_log(jz, JZ_LOG_ERROR, "libusb_get_device_list: %s", libusb_strerror(dev_count));
        rc = JZ_ERR_USB;
        goto error;
    }

    for(ssize_t i = 0; i < dev_count; ++i) {
        struct libusb_device_descriptor desc;
        rc = libusb_get_device_descriptor(dev_list[i], &desc);
        if(rc < 0) {
            jz_log(jz, JZ_LOG_WARNING, "libusb_get_device_descriptor: %s",
                   libusb_strerror(rc));
            continue;
        }

        if(desc.idVendor != vend_id || desc.idProduct != prod_id)
            continue;

        if(dev_index >= 0) {
            /* not the best, but it is the safest thing */
            jz_log(jz, JZ_LOG_ERROR, "Multiple devices match ID %04x:%04x",
                   (unsigned int)vend_id, (unsigned int)prod_id);
            jz_log(jz, JZ_LOG_ERROR, "Please ensure only one player is plugged in, and try again");
            rc = JZ_ERR_NO_DEVICE;
            goto error;
        }

        dev_index = i;
    }

    if(dev_index < 0) {
        jz_log(jz, JZ_LOG_ERROR, "No device with ID %04x:%05x found",
               (unsigned int)vend_id, (unsigned int)prod_id);
        rc = JZ_ERR_NO_DEVICE;
        goto error;
    }

    rc = libusb_open(dev_list[dev_index], &usb_handle);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "libusb_open: %s", libusb_strerror(rc));
        rc = JZ_ERR_USB;
        goto error;
    }

    rc = libusb_claim_interface(usb_handle, 0);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "libusb_claim_interface: %s", libusb_strerror(rc));
        rc = JZ_ERR_USB;
        goto error;
    }

    dev->jz = jz;
    dev->handle = usb_handle;
    *devptr = dev;
    rc = JZ_SUCCESS;

  exit:
    if(dev_list)
        libusb_free_device_list(dev_list, true);
    return rc;

  error:
    if(dev)
        free(dev);
    if(usb_handle)
        libusb_close(usb_handle);
    jz_context_unref_libusb(jz);
    goto exit;
}

void jz_usb_close(jz_usbdev* dev)
{
    libusb_release_interface(dev->handle, 0);
    libusb_close(dev->handle);
    jz_context_unref_libusb(dev->jz);
    free(dev);
}

static int jz_usb_vendor_req(jz_usbdev* dev, int req, uint32_t arg)
{
    int rc = libusb_control_transfer(dev->handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        req, arg >> 16, arg & 0xffff, NULL, 0, 1000);

    if(rc < 0) {
        jz_log(dev->jz, JZ_LOG_ERROR, "libusb_control_transfer: %s", libusb_strerror(rc));
        rc = JZ_ERR_USB;
    } else {
        rc = JZ_SUCCESS;
    }

    return rc;
}

static int jz_usb_transfer(jz_usbdev* dev, bool write, size_t len, void* buf)
{
    int xfered = 0;
    int ep = write ? LIBUSB_ENDPOINT_OUT|1 : LIBUSB_ENDPOINT_IN|1;
    int rc = libusb_bulk_transfer(dev->handle, ep, buf, len, &xfered, 10000);

    if(rc < 0) {
        jz_log(dev->jz, JZ_LOG_ERROR, "libusb_bulk_transfer: %s", libusb_strerror(rc));
        rc = JZ_ERR_USB;
    } else if(xfered != (int)len) {
        jz_log(dev->jz, JZ_LOG_ERROR, "libusb_bulk_transfer: incorrect amount of data transfered");
        rc = JZ_ERR_USB;
    } else {
        rc = JZ_SUCCESS;
    }

    return rc;
}

static int jz_usb_sendrecv(jz_usbdev* dev, bool write, uint32_t addr,
                           size_t len, void* data)
{
    int rc;
    rc = jz_usb_vendor_req(dev, VR_SET_DATA_ADDRESS, addr);
    if(rc < 0)
        return rc;

    rc = jz_usb_vendor_req(dev, VR_SET_DATA_LENGTH, len);
    if(rc < 0)
        return rc;

    return jz_usb_transfer(dev, write, len, data);
}

int jz_usb_send(jz_usbdev* dev, uint32_t addr, size_t len, const void* data)
{
    return jz_usb_sendrecv(dev, true, addr, len, (void*)data);
}

int jz_usb_recv(jz_usbdev* dev, uint32_t addr, size_t len, void* data)
{
    return jz_usb_sendrecv(dev, false, addr, len, data);
}

int jz_usb_start1(jz_usbdev* dev, uint32_t addr)
{
    return jz_usb_vendor_req(dev, VR_PROGRAM_START1, addr);
}

int jz_usb_start2(jz_usbdev* dev, uint32_t addr)
{
    return jz_usb_vendor_req(dev, VR_PROGRAM_START2, addr);
}

int jz_usb_flush_caches(jz_usbdev* dev)
{
    return jz_usb_vendor_req(dev, VR_FLUSH_CACHES, 0);
}
