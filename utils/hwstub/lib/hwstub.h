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
#ifndef __HWSTUB__
#define __HWSTUB__

#include <libusb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

#include "hwstub_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * Low-Level interface
 *
 */
struct hwstub_device_t;

/* Returns 0 if a device interface is usable, or -1 if none was found */
int hwstub_probe(libusb_device *dev);
/* Helper function that returns a list of all hwstub devices found. The caller
 * must unref all of them when done, possibly using libusb_free_device_list().
 * Return number of devices or <0 on error */
ssize_t hwstub_get_device_list(libusb_context *ctx, libusb_device ***list);
/* Helper function that filter the device list. The function will note destroy
 * or unref the previous list. The callack function must return 1 to keep the
 * device and 0 to drop it.
 * NOTE all devices in the new list will have one more reference */
typedef int (*hwstub_dev_list_filter_fn_t)(libusb_device *dev, void *user);
ssize_t hwstub_filter_device_list_by_vid_pid(libusb_device **list, ssize_t size,
    libusb_device ***new_list, hwstub_dev_list_filter_fn_t filter, void *user);
/* Returns NULL on error
 * NOTE: hwstub_open() will add a reference to the device and
 *       hwstub_release() will unref it */
struct hwstub_device_t *hwstub_open_usb(libusb_device *dev);
struct hwstub_device_t *hwstub_open_tcp(const char *host, const char *port);
/* Release device, will unref the usb device if any */
void hwstub_release(struct hwstub_device_t *dev);
/* Open an URI. The general format is as follows:
 *   scheme:[//domain:[port]][/][path[?query]]
 * The following schemes are recognized:
 *   usb     USB device
 *   tcp     Hwstub TCP server
 * If more than one device match the URI, NULL is returned and a message is printed
 * to the output. If the URI format is not valid, a message is printed to the output
 * and NULL is returned.
 */
struct hwstub_device_t *hwstub_open_uri(libusb_context *usb_context, FILE *msg, const char *uri);
/* Print URI usage() on output file */
void hwstub_usage_uri(FILE *f);

/* Returns number of bytes filled */
int hwstub_get_desc(struct hwstub_device_t *dev, uint16_t desc, void *info, size_t sz);
/* Returns number of bytes filled */
int hwstub_get_log(struct hwstub_device_t *dev, void *buf, size_t sz);
/* Returns number of bytes written/read or <0 on error */
int hwstub_read(struct hwstub_device_t *dev, uint32_t addr, void *buf, size_t sz);
int hwstub_read_atomic(struct hwstub_device_t *dev, uint32_t addr, void *buf, size_t sz);
int hwstub_write(struct hwstub_device_t *dev, uint32_t addr, const void *buf, size_t sz);
int hwstub_write_atomic(struct hwstub_device_t *dev, uint32_t addr, const void *buf, size_t sz);
int hwstub_rw_mem(struct hwstub_device_t *dev, int read, uint32_t addr, void *buf, size_t sz);
int hwstub_rw_mem_atomic(struct hwstub_device_t *dev, int read, uint32_t addr, void *buf, size_t sz);
/* Returns <0 on error */
int hwstub_exec(struct hwstub_device_t *dev, uint32_t addr, uint16_t flags);
int hwstub_call(struct hwstub_device_t *dev, uint32_t addr);
int hwstub_jump(struct hwstub_device_t *dev, uint32_t addr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __HWSTUB__ */
