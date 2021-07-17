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
#include <string.h>

#define VR_GET_CPU_INFO     0
#define VR_SET_DATA_ADDRESS 1
#define VR_SET_DATA_LENGTH  2
#define VR_FLUSH_CACHES     3
#define VR_PROGRAM_START1   4
#define VR_PROGRAM_START2   5

/** \brief Open a USB device
 * \param jz        Context
 * \param devptr    Returns pointer to the USB device upon success
 * \param vend_id   USB vendor ID
 * \param prod_id   USB product ID
 * \return either JZ_SUCCESS if device was opened, or an error below
 * \retval JZ_ERR_OUT_OF_MEMORY     malloc failed
 * \retval JZ_ERR_USB               libusb error (details are logged)
 * \retval JZ_ERR_NO_DEVICE         can't unambiguously find the device
 */
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
        jz_log(jz, JZ_LOG_ERROR, "No device with ID %04x:%04x found",
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

    jz_log(jz, JZ_LOG_DEBUG, "Opened device (%p, ID %04x:%04x)",
           dev, (unsigned int)vend_id, (unsigned int)prod_id);
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

/** \brief Close a USB device
 * \param dev   Device to close; memory will be freed automatically
 */
void jz_usb_close(jz_usbdev* dev)
{
    jz_log(dev->jz, JZ_LOG_DEBUG, "Closing device (%p)", dev);
    libusb_release_interface(dev->handle, 0);
    libusb_close(dev->handle);
    jz_context_unref_libusb(dev->jz);
    free(dev);
}

// Does an Ingenic-specific vendor request
// Written with X1000 in mind but other Ingenic CPUs have the same commands
static int jz_usb_vendor_req(jz_usbdev* dev, int req, uint32_t arg,
                             void* buffer, int buflen)
{
    int rc = libusb_control_transfer(dev->handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        req, arg >> 16, arg & 0xffff, buffer, buflen, 1000);

    if(rc < 0) {
        jz_log(dev->jz, JZ_LOG_ERROR, "libusb_control_transfer: %s", libusb_strerror(rc));
        rc = JZ_ERR_USB;
    } else {
        static const char* req_names[] = {
            "GET_CPU_INFO",
            "SET_DATA_ADDRESS",
            "SET_DATA_LENGTH",
            "FLUSH_CACHES",
            "PROGRAM_START1",
            "PROGRAM_START2",
        };

        jz_log(dev->jz, JZ_LOG_DEBUG, "Issued %s %08lu",
               req_names[req], (unsigned long)arg);
        rc = JZ_SUCCESS;
    }

    return rc;
}

// Bulk transfer wrapper
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
        jz_log(dev->jz, JZ_LOG_DEBUG, "Transferred %zu bytes %s",
               len, write ? "to device" : "from device");
        rc = JZ_SUCCESS;
    }

    return rc;
}

// Memory send/receive primitive, performs the necessary vendor requests
// and then tranfers data using the bulk endpoint
static int jz_usb_sendrecv(jz_usbdev* dev, bool write, uint32_t addr,
                           size_t len, void* data)
{
    int rc;
    rc = jz_usb_vendor_req(dev, VR_SET_DATA_ADDRESS, addr, NULL, 0);
    if(rc < 0)
        return rc;

    rc = jz_usb_vendor_req(dev, VR_SET_DATA_LENGTH, len, NULL, 0);
    if(rc < 0)
        return rc;

    return jz_usb_transfer(dev, write, len, data);
}

/** \brief Write data to device memory
 * \param dev       USB device
 * \param addr      Address where data should be written
 * \param len       Length of the data, in bytes, should be positive
 * \param data      Data buffer
 * \return either JZ_SUCCESS on success or a failure code
 */
int jz_usb_send(jz_usbdev* dev, uint32_t addr, size_t len, const void* data)
{
    return jz_usb_sendrecv(dev, true, addr, len, (void*)data);
}

/** \brief Read data to device memory
 * \param dev       USB device
 * \param addr      Address to read from
 * \param len       Length of the data, in bytes, should be positive
 * \param data      Data buffer
 * \return either JZ_SUCCESS on success or a failure code
 */
int jz_usb_recv(jz_usbdev* dev, uint32_t addr, size_t len, void* data)
{
    return jz_usb_sendrecv(dev, false, addr, len, data);
}

/** \brief Execute stage1 program jumping to the specified address
 * \param dev       USB device
 * \param addr      Address to begin execution at
 * \return either JZ_SUCCESS on success or a failure code
 */
int jz_usb_start1(jz_usbdev* dev, uint32_t addr)
{
    return jz_usb_vendor_req(dev, VR_PROGRAM_START1, addr, NULL, 0);
}

/** \brief Execute stage2 program jumping to the specified address
 * \param dev       USB device
 * \param addr      Address to begin execution at
 * \return either JZ_SUCCESS on success or a failure code
 */
int jz_usb_start2(jz_usbdev* dev, uint32_t addr)
{
    return jz_usb_vendor_req(dev, VR_PROGRAM_START2, addr, NULL, 0);
}

/** \brief Ask device to flush CPU caches
 * \param dev       USB device
 * \return either JZ_SUCCESS on success or a failure code
 */
int jz_usb_flush_caches(jz_usbdev* dev)
{
    return jz_usb_vendor_req(dev, VR_FLUSH_CACHES, 0, NULL, 0);
}

/** \brief Ask device for CPU info string
 * \param dev       USB device
 * \param buffer    Buffer to hold the info string
 * \param buflen    Size of the buffer, in bytes
 * \return either JZ_SUCCESS on success or a failure code
 *
 * The buffer will always be null terminated, but to ensure the info string is
 * not truncated the buffer needs to be at least `JZ_CPUINFO_BUFLEN` byes long.
 */
int jz_usb_get_cpu_info(jz_usbdev* dev, char* buffer, size_t buflen)
{
    char tmpbuf[JZ_CPUINFO_BUFLEN];
    int rc = jz_usb_vendor_req(dev, VR_GET_CPU_INFO, 0, tmpbuf, 8);
    if(rc != JZ_SUCCESS)
        return rc;

    if(buflen > 0) {
        strncpy(buffer, tmpbuf, buflen);
        buffer[buflen - 1] = 0;
    }

    return JZ_SUCCESS;
}
