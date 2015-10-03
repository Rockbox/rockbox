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
#ifndef __HWSTUB_INTERNAL__
#define __HWSTUB_INTERNAL__

/**
 * INTERNAL use ONLY
 */

#include "hwstub.h"
#include <string.h>
#include <stdlib.h>

struct hwstub_device_t;

/* Release function */
typedef void (*hwstub_release_fn_t)(struct hwstub_device_t *dev);
/* Get descriptor: return <0 on error, amount read otherwise */
typedef int (*hwstub_get_desc_fn_t)(struct hwstub_device_t *dev, uint16_t desc, void *info, size_t sz);
/* Get log: return <0 on error, amount read otherwise */
typedef int (*hwstub_get_log_fn_t)(struct hwstub_device_t *dev, void *buf, size_t sz);
/* Execute function: return <0 on error, 0 on success */
typedef int (*hwstub_exec_fn_t)(struct hwstub_device_t *dev, uint32_t addr, uint16_t flags);
/* Read function: return <0 on error, amount read otherwise. Size is always lower
 * then maximum buffer size */
typedef int (*hwstub_read_fn_t)(struct hwstub_device_t *dev, uint8_t breq, uint32_t addr,
    void *buf, size_t sz);
/* Write function: same as read */
typedef int (*hwstub_write_fn_t)(struct hwstub_device_t *dev, uint8_t breq, uint32_t addr,
    const void *buf, size_t sz);


struct hwstub_device_vtable_t
{
    hwstub_release_fn_t release;
    hwstub_get_desc_fn_t get_desc;
    hwstub_get_log_fn_t get_log;
    hwstub_read_fn_t read;
    hwstub_write_fn_t write;
    hwstub_exec_fn_t exec;
};

struct hwstub_device_t
{
    struct hwstub_device_vtable_t vtable;
    unsigned buf_sz; /* device buffer size, no transfer can be greater than this */
    uint16_t id; /* transaction ID */
    uint8_t minor_ver; /* maximum level of compatibility between the device and library */
};

/**
 * Probing
 */

/* return <0 on failure, interface index on success */
int hwstub_rb_probe(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config);
/* return <0 on failure, 0 on success */
int hwstub_jz_probe(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config);

/**
 * Opening
 */
struct hwstub_device_t *hwstub_rb_open(libusb_device_handle *handle, int intf);
struct hwstub_device_t *hwstub_jz_open(libusb_device_handle *handle);
struct hwstub_device_t *hwstub_tcp_open(const char *host, const char *port);

#endif /* __HWSTUB_INTERNAL__ */
 
