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

/* Returns hwstub interface, or -1 if none was found */
int hwstub_probe(libusb_device *dev);
/* Helper function which returns a list of all hwstub devices found. The caller
 * must unref all of them when done, possibly using libusb_free_device_list().
 * Return number of devices or <0 on error */
ssize_t hwstub_get_device_list(libusb_context *ctx, libusb_device ***list);
/* Returns NULL on error */
struct hwstub_device_t *hwstub_open(libusb_device_handle *handle);
/* Returns 0 on success. Does *NOT* close the usb handle */
int hwstub_release(struct hwstub_device_t *dev);

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