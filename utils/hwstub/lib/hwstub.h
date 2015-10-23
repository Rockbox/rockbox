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

/** Errors */
#define HWSTUB_SUCCESS  0
#define HWSTUB_ERROR    -1

typedef void hwstub_context_t;
typedef void hwstub_device_t;
typedef void hwstub_handle_t;

/** Context
 * A context provides two major operations:
 * - device listing
 * - device opening
 * A typical context is USB, another is TCP to relay another remote context
 */

/* Open the default context */
int hwstub_init_default(hwstub_context_t **ctx);
/* Open a USB context, assuming the libusb context was initialised */
int hwstub_init_usb(libusb_context *ctx, hwstub_context_t **out_ctx);
/* Open a TCP context */
int hwstub_init_tcp(const char *host, const char *port, hwstub_context_t **out_ctx);
/* Exit a context, make sure that all handles and so on are closed */
void hwstub_exit(hwstub_context_t *ctx);
/* Return the list of available devices. The returned value is the number of
 * devices in the list (>=0) or an error (<0). The list is always NULL terminated
 * so the actual list size is one greater than the returned size */
ssize_t hwstub_get_device_list(hwstub_context_t *ctx, hwstub_device_t ***list);
/* Free a device list built by hwstub_get_device_list, optionally unreferences all devices in it */
void hwstub_free_device_list(hwstub_device_t *list, int unref_devices);
/* Get the target ID of a device. This function cannot fail or block */
uint32_t hwstub_get_target_id(hwstub_device_t *dev);
/* Get the target name of a device. This function cannot fail or block.
 * If the buffer is too small, it will be truncate and zero-terminated (except
 * if size is 0). It returns the size of *full* string in all cases (not including 0) */
size_t hwstub_get_target_name(hwstub_device_t *dev, char *buffer, size_t size);
/* Add a reference to the device, and return the device */
hwstub_device_t *hwstub_ref_device(hwstub_device_t *dev);
/* Remove a reference to the device. If there are no more references to the device,
 * it will be destroyed */
void hwstub_unref_device(hwstub_device_t *dev);
/* Check wether a device is still connected. Return 1 if connected, 0 otherwise */
int hwstub_is_connected(hwstub_device_t *dev);
/* Open a device and obtain a handle to communicate with it. Internally adds a
 * reference to the device that is removed on close. */
int hwstub_open(hwstub_device_t *dev, hwstub_handle_t **handle);
/* Close a device handle. Internally unreference the device added on open. */
void hwstub_close(hwstub_handle_t *handle);
/* Return the device associated with the handle */
hwstub_device_t *hwstub_get_device(hwstub_handle_t *handle);
/* Retrieve a descriptor from the device. Returns number of bytes filled or <0 on error */
int hwstub_get_desc(hwstub_handle_t *dev, uint16_t desc, void *info, size_t sz);
/* Retrieve part of the device log. Returns number of bytes filled or <0 on error */
int hwstub_get_log(hwstub_handle_t *dev, void *buf, size_t sz);
/* Read/write from/to the device. Returns number of bytes written/read or <0 on error.
 * Atomic operations guarantee that the entire buffer is read/written atomically
 * and as such might only be available on some targets and for a restricted set
 * of buffer sizes. */
int hwstub_read(hwstub_handle_t *dev, uint32_t addr, void *buf, size_t sz);
int hwstub_read_atomic(hwstub_handle_t *dev, uint32_t addr, void *buf, size_t sz);
int hwstub_write(hwstub_handle_t *dev, uint32_t addr, const void *buf, size_t sz);
int hwstub_write_atomic(hwstub_handle_t *dev, uint32_t addr, const void *buf, size_t sz);
int hwstub_rw_mem(hwstub_handle_t *dev, int read, uint32_t addr, void *buf, size_t sz);
int hwstub_rw_mem_atomic(hwstub_handle_t *dev, int read, uint32_t addr, void *buf, size_t sz);
/* Execute code on the device. Returns <0 on error */
int hwstub_exec(hwstub_handle_t *dev, uint32_t addr, uint16_t flags);
int hwstub_call(hwstub_handle_t *dev, uint32_t addr);
int hwstub_jump(hwstub_handle_t *dev, uint32_t addr);

/* Open an URI. The general format is as follows:
 *   scheme:[//domain:[port]][/][path[?query]]
 * The following schemes are recognized:
 *   usb     USB device
 *   tcp     Hwstub TCP server
 * If more than one device match the URI, NULL is returned and a message is printed
 * to the output. If the URI format is not valid, a message is printed to the output
 * and NULL is returned.
 */
hwstub_device_t *hwstub_open_uri(libusb_context *usb_context, FILE *msg, const char *uri);
/* Print URI usage() on output file */
void hwstub_usage_uri(FILE *f);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __HWSTUB__ */
